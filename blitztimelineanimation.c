#include "prism/blitztimelineanimation.h"

#include "prism/datastructures.h"
#include "prism/blitzentity.h"
#include "prism/blitzcamerahandler.h"
#include "prism/log.h"
#include "prism/system.h"
#include "prism/blitzmugenanimation.h"
#include "prism/blitzphysics.h"

typedef struct {
	void(*mCB)(void*);
	void* mCBCaller;
} ActiveAnimationCB;

typedef struct {
	int mAnimationNumber;
	Tick mTime;

	IntMap mCBMap;
} ActiveAnimation;

typedef struct {
	int mEntityID;

	BlitzTimelineAnimations* mAnimations;

	IntMap mActiveAnimations;
} BlitzTimelineAnimationEntry;

static struct {
	IntMap mEntries;
} gData;

static void loadBlitzTimelineAnimationHandler(void* tData) {
	(void)tData;
	gData.mEntries = new_int_map();
}

static void interpolateFloatAnimationStep(BlitzTimelineAnimationStep* tStep, double t, int tEntityID, void(*tFunc)(int, double)) {
	double val1 = atof(tStep->mStartValue);
	double val2 = atof(tStep->mEndValue);
	
	double trueVal = val1 + t*(val2 - val1);
	tFunc(tEntityID, trueVal);
}

static void interpolateIntegerAnimationStep(BlitzTimelineAnimationStep* tStep, double t, int tEntityID, void(*tFunc)(int, int)) {
	int val1 = atoi(tStep->mStartValue);
	int val2 = atoi(tStep->mEndValue);

	int trueVal = (int)(val1 + t*(val2 - val1));
	tFunc(tEntityID, trueVal);
}

static void callCallback(ActiveAnimation* tActiveAnimation, BlitzTimelineAnimationStep* tStep) {

	int value = atoi(tStep->mStartValue);

	if (int_map_contains(&tActiveAnimation->mCBMap, value)) {
		ActiveAnimationCB* e = int_map_get(&tActiveAnimation->mCBMap, value);
		e->mCB(e->mCBCaller);
	}
}

static void updateSingleActiveAnimationStepTarget(BlitzTimelineAnimationStep* tStep, double t, int tEntityID, ActiveAnimation* tActiveAnimation) {
	if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_POSITION_X) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzEntityPositionX);
	} else 	if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_POSITION_Y) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzEntityPositionY);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_SCALE) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzEntityScale2D);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_SCALE_X) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzEntityScaleX);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_SCALE_Y) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzEntityScaleY);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_ANGLE) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzEntityRotationZ);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_CALLBACK) {
		callCallback(tActiveAnimation, tStep);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_VELOCITY_X) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzPhysicsVelocityX);
	}
	else 	if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_VELOCITY_Y) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzPhysicsVelocityY);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_MUGEN_TRANSPARENCY) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzMugenAnimationTransparency);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_MUGEN_ANIMATION) {
		interpolateIntegerAnimationStep(tStep, t, tEntityID, changeBlitzMugenAnimation);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_MUGEN_FACE_DIRECTION) {
		interpolateIntegerAnimationStep(tStep, t, tEntityID, setBlitzMugenAnimationFaceDirection);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_MUGEN_POSITION_X) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzMugenAnimationPositionX);
	}
	else if (tStep->mTargetType == BLITZ_TIMELINE_ANIMATION_STEP_TARGET_TYPE_MUGEN_POSITION_Y) {
		interpolateFloatAnimationStep(tStep, t, tEntityID, setBlitzMugenAnimationPositionY);
	}
	else {
		logWarningFormat("Unimplemented target type: %d", tStep->mTargetType);
	}
}

static void unloadActiveAnimation(ActiveAnimation* e) {
	delete_int_map(&e->mCBMap);
}

static int unloadSingleActiveAnimationCB(void* tCaller, void* tData) {
	(void)tCaller;
	ActiveAnimation* e = tData;
	unloadActiveAnimation(e);
	return 1;
}

typedef struct {
	ActiveAnimation* mActiveAnimation;
	BlitzTimelineAnimationEntry* mEntry;
} BlitzTimelineActiveAnimationStepUpdateCaller;

static void updateSingleActiveAnimationStep(void* tCaller, void* tData) {
	BlitzTimelineActiveAnimationStepUpdateCaller* caller = tCaller;
	BlitzTimelineAnimationStep* step = tData;
	ActiveAnimation* activeAnimation = caller->mActiveAnimation;

	if (activeAnimation->mTime < step->mTime) return;
	Tick endTime = step->mTime + step->mDuration;
	if (activeAnimation->mTime > endTime) return;
	
	Tick timeOffset = activeAnimation->mTime - step->mTime;
	double t;
	if (step->mDuration <= 1) t = 1;
	else t = timeOffset / (double)(step->mDuration - 1);

	updateSingleActiveAnimationStepTarget(step, t, caller->mEntry->mEntityID, activeAnimation);
}

static int updateSingleActiveAnimation(void* tCaller, void* tData) {
	BlitzTimelineAnimationEntry* entry = tCaller;
	ActiveAnimation* e = tData;
	BlitzTimelineAnimation* animation = getBlitzTimelineAnimation(entry->mAnimations, e->mAnimationNumber);
	
	BlitzTimelineActiveAnimationStepUpdateCaller caller;
	caller.mActiveAnimation = e;
	caller.mEntry = entry;
	vector_map(&animation->BlitzTimelineAnimationSteps, updateSingleActiveAnimationStep, &caller);

	if (handleTickDurationAndCheckIfOver(&e->mTime, animation->mDuration)) {
		if (animation->mIsLooping) {
			e->mTime = 0;
		}
		else {
			unloadActiveAnimation(e);
			return 1;
		}
		
	}

	return 0;
}

static void updateSingleBlitzTimelineAnimationEntry(void* tCaller, void* tData) {
	(void)tCaller;
	BlitzTimelineAnimationEntry* e = tData;
	int_map_remove_predicate(&e->mActiveAnimations, updateSingleActiveAnimation, e);
}

static void updateBlitzTimelineAnimationHandler(void* tData) {
	(void)tData;
	int_map_map(&gData.mEntries, updateSingleBlitzTimelineAnimationEntry, NULL);
}

ActorBlueprint BlitzTimelineAnimationHandler = {
	.mLoad = loadBlitzTimelineAnimationHandler,
	.mUpdate = updateBlitzTimelineAnimationHandler,
};

static void unregisterEntity(int tEntityID);

static BlitzComponent BlitzTimelineAnimationComponent = {
	.mUnregisterEntity = unregisterEntity,
};

static BlitzTimelineAnimationEntry* getBlitzTimelineAnimationEntry(int tEntityID) {
	if (!int_map_contains(&gData.mEntries, tEntityID)) {
		logErrorFormat("Entity with ID %d does not have a timeline animation component.", tEntityID);
		abortSystem();
	}

	return int_map_get(&gData.mEntries, tEntityID);
}

static ActiveAnimation* getBlitzTimelineActiveAnimation(int tEntityID, int tAnimationID) {
	BlitzTimelineAnimationEntry* e = getBlitzTimelineAnimationEntry(tEntityID);
	
	if (!int_map_contains(&e->mActiveAnimations, tAnimationID)) {
		logWarningFormat("Entity with ID %d does not have an active animation with ID %d.", tAnimationID);
		return NULL;
	}

	return int_map_get(&e->mActiveAnimations, tAnimationID);
}


void addBlitzTimelineComponent(int tEntityID, BlitzTimelineAnimations* tAnimations) {
	BlitzTimelineAnimationEntry* e = allocMemory(sizeof(BlitzTimelineAnimationEntry));
	e->mEntityID = tEntityID;
	e->mAnimations = tAnimations;
	e->mActiveAnimations = new_int_map();

	registerBlitzComponent(tEntityID, BlitzTimelineAnimationComponent);
	int_map_push_owned(&gData.mEntries, tEntityID, e);
}



static void unregisterEntity(int tEntityID) {
	BlitzTimelineAnimationEntry* e = getBlitzTimelineAnimationEntry(tEntityID);
	int_map_remove_predicate(&e->mActiveAnimations, unloadSingleActiveAnimationCB, NULL);
	delete_int_map(&e->mActiveAnimations);
	int_map_remove(&gData.mEntries, tEntityID);
}

int playBlitzTimelineAnimation(int tEntityID, int tAnimation) {
	BlitzTimelineAnimationEntry* entry = getBlitzTimelineAnimationEntry(tEntityID);
	
	ActiveAnimation* e = allocMemory(sizeof(ActiveAnimation));
	e->mAnimationNumber = tAnimation;
	e->mTime = 0;
	e->mCBMap = new_int_map();

	return int_map_push_back_owned(&entry->mActiveAnimations, e);
}

void stopBlitzTimelineAnimation(int tEntityID, int tAnimationID)
{
	(void)tEntityID;
	(void)tAnimationID;
	// TODO
}

void setBlitzTimelineAnimationCB(int tEntityID, int tAnimationID, int tCBID, void * tCB, void * tCaller)
{
	ActiveAnimation* e = getBlitzTimelineActiveAnimation(tEntityID, tAnimationID);
	if (!int_map_contains(&e->mCBMap, tCBID)) {
		ActiveAnimationCB* cb = allocMemory(sizeof(ActiveAnimationCB));
		int_map_push_owned(&e->mCBMap, tCBID, cb);
	}

	ActiveAnimationCB* cb = int_map_get(&e->mCBMap, tCBID);
	cb->mCB = tCB;
	cb->mCBCaller = tCaller;
}
