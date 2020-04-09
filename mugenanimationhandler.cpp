#include "prism/mugenanimationhandler.h"

#include <algorithm>
#include <map>

#include "prism/collisionhandler.h"
#include "prism/math.h"
#include <prism/log.h>
#include <prism/system.h>
#include <prism/stlutil.h>

using namespace std;

static struct {
	map<int, MugenAnimationHandlerElement> mAnimations;
	int mIsPaused;
} gMugenAnimationHandler;

static void loadMugenAnimationHandler(void* tData) {
	(void)tData;
	stl_new_map(gMugenAnimationHandler.mAnimations);
	gMugenAnimationHandler.mIsPaused = 0;
}

static void unloadMugenAnimation(MugenAnimationHandlerElement* e);

static int unloadSingleMugenAnimationCB(void* tCaller, MugenAnimationHandlerElement& tData) {
	(void)tCaller;
	MugenAnimationHandlerElement* e = &tData;
	unloadMugenAnimation(e);
	return 1;
}

static void unloadMugenAnimationHandler(void* tData) {
	(void)tData;
	stl_int_map_remove_predicate(gMugenAnimationHandler.mAnimations, unloadSingleMugenAnimationCB);
	stl_delete_map(gMugenAnimationHandler.mAnimations);
}

static MugenAnimationStep* getCurrentAnimationStep(MugenAnimationHandlerElement* e) {
	int i = min(e->mStep, vector_size(&e->mAnimation->mSteps) - 1);
	if (i < 0) return NULL;
	return (MugenAnimationStep*)vector_get(&e->mAnimation->mSteps, i);
}


static void passiveAnimationHitCB(void* tCaller, void* tCollisionData, int tOtherCollisionList) {
	MugenAnimationHandlerElement* e = (MugenAnimationHandlerElement*)tCaller;
	if (!e->mHasPassiveHitCB || e->mPassiveHitCB == NULL) return;
	e->mPassiveHitCB(e->mPassiveHitCaller, tCollisionData, tOtherCollisionList);
}


static void attackAnimationHitCB(void* tCaller, void* tCollisionData, int tOtherCollisionList) {
	(void)tCaller;
	(void)tCollisionData;
	MugenAnimationHandlerElement* e = (MugenAnimationHandlerElement*)tCaller;
	if (!e->mHasAttackHitCB || e->mAttackHitCB == NULL) return;
	e->mAttackHitCB(e->mAttackHitCaller, tCollisionData, tOtherCollisionList);
}

typedef struct {
	MugenAnimationHandlerElement* mElement;
	CollisionListData* mList;
	CollisionCallback mCB;
	void* mCollisionData;
} HitboxAdditionCaller;

static void addSingleHitbox(void* tCaller, void* tData) {
	HitboxAdditionCaller* caller = (HitboxAdditionCaller*)tCaller;
	CollisionRect* rect = (CollisionRect*)tData;


	CollisionRect scaledRectangle = *rect;
	scaledRectangle = scaleGeoRectangleByFactor(scaledRectangle, caller->mElement->mDrawScale);
	scaledRectangle = scaleGeoRectangleByFactor2D(scaledRectangle, caller->mElement->mBaseDrawScale);
	if (!caller->mElement->mIsFacingRight) {
		double xBuffer = scaledRectangle.mTopLeft.x;
		scaledRectangle.mTopLeft.x = -scaledRectangle.mBottomRight.x;
		scaledRectangle.mBottomRight.x = -xBuffer;
	}
	if (!caller->mElement->mIsFacingDown) {
		double yBuffer = scaledRectangle.mTopLeft.y;
		scaledRectangle.mTopLeft.y = -scaledRectangle.mBottomRight.y;
		scaledRectangle.mBottomRight.y = -yBuffer;
	}

	caller->mElement->mActiveHitboxes.push_back(MugenAnimationHandlerHitboxElement());
	MugenAnimationHandlerHitboxElement& e = caller->mElement->mActiveHitboxes.back();

	e.mCollider = makeColliderFromRect(scaledRectangle);

	e.mList = caller->mList;
	e.mElement = addColliderToCollisionHandler(caller->mList, &caller->mElement->mPlayerPositionReference, e.mCollider, caller->mCB, caller->mElement, caller->mCollisionData);

	
}

static void addNewSingleHitboxType(MugenAnimationHandlerElement* e, List* tHitboxes, CollisionListData* tList, CollisionCallback tCB, void* tCollisionData) {
	HitboxAdditionCaller caller;
	caller.mElement = e;
	caller.mList = tList;
	caller.mCB = tCB;
	caller.mCollisionData = tCollisionData;

	list_map(tHitboxes, addSingleHitbox, &caller);
}

static void addNewHitboxes(MugenAnimationHandlerElement* e, MugenAnimationStep* tStep) {
	if (!tStep) return;

	if (e->mHasPassiveHitboxes) {
		addNewSingleHitboxType(e, &tStep->mPassiveHitboxes, e->mPassiveCollisionList, passiveAnimationHitCB, e->mPassiveCollisionData);
	}
	if (e->mHasAttackHitboxes) {
		addNewSingleHitboxType(e, &tStep->mAttackHitboxes, e->mAttackCollisionList, attackAnimationHitCB, e->mAttackCollisionData);
	}
}

static int removeSingleHitbox(void* tCaller, MugenAnimationHandlerHitboxElement& tData) {
	(void)tCaller;
	MugenAnimationHandlerHitboxElement* hitbox = &tData;

	removeFromCollisionHandler(hitbox->mElement);
	destroyCollider(&hitbox->mCollider);
	return 1;
}

static void removeOldHitboxes(MugenAnimationHandlerElement* e) {
	stl_list_remove_predicate(e->mActiveHitboxes, removeSingleHitbox);
}

static void updateHitboxes(MugenAnimationHandlerElement* e) {
	removeOldHitboxes(e);
	addNewHitboxes(e, getCurrentAnimationStep(e));
}

static MugenDuration getTimeWhenStepStarts(MugenAnimationHandlerElement* e, int tStep) {
	MugenDuration sum = 0;
	tStep = min(tStep, vector_size(&e->mAnimation->mSteps));
	int i;
	for (i = 0; i < tStep; i++) {
		MugenAnimationStep* step = (MugenAnimationStep*)vector_get(&e->mAnimation->mSteps, i);
		sum += step->mDuration;
	}
	return sum + 1;

}

static void unloadMugenAnimation(MugenAnimationHandlerElement* e) {
	removeOldHitboxes(e);
	e->mActiveHitboxes.clear();
}

static void updateStepSpriteAndSpriteValidity(MugenAnimationHandlerElement* e) {
	MugenAnimationStep* step = getCurrentAnimationStep(e);
	e->mSprite = step ? getMugenSpriteFileTextureReference(e->mSprites, step->mGroupNumber, step->mSpriteNumber) : NULL;
	e->mHasSprite = e->mSprite != NULL;
}

static void increaseMugenDuration(MugenDuration* tDuration) {
	if (gMugenAnimationHandler.mIsPaused) return;

	(*tDuration)++;
}

static int loadNextStepAndReturnIfShouldBeRemoved(MugenAnimationHandlerElement* e) {
	e->mStepTime = 1;
	
	e->mStep++;
	if (e->mStep == vector_size(&e->mAnimation->mSteps)) {
		if (e->mHasAnimationFinishedCallback) {
			e->mAnimationFinishedCB(e->mAnimationFinishedCaller);
		}

		if (e->mIsLooping) {
			e->mStep = e->mAnimation->mLoopStart;
			e->mStepTime = 1;
			e->mHasLooped = 1;
			e->mOverallTime = getTimeWhenStepStarts(e, e->mStep);
		}
		else {
			unloadMugenAnimation(e);
			return 1;
		}
	}
	else {
		increaseMugenDuration(&e->mOverallTime);
	}

	updateStepSpriteAndSpriteValidity(e);

	updateHitboxes(e);

	return 0;
}

static void startNewAnimationWithStartStep(MugenAnimationHandlerElement* e, int tStartStep) {
	tStartStep--;

	if (tStartStep < 0) {
		logWarningFormat("Trying to start animation with negative start step: %d. Defaulting to zero.\n", tStartStep);
		tStartStep = 0;
	}

	e->mOverallTime = getTimeWhenStepStarts(e, tStartStep) - 1;
	e->mHasLooped = 0;

	e->mStep = tStartStep - 1;
	if (loadNextStepAndReturnIfShouldBeRemoved(e)) {
		logError("Unable to start animation, is already over.");
		logErrorInteger(e->mAnimation->mID);
		logErrorInteger(tStartStep);
		recoverFromError();
	}
}

MugenAnimationHandlerElement* addMugenAnimation(MugenAnimation* tStartAnimation, MugenSpriteFile* tSprites, Position tPosition)
{
	MugenAnimationHandlerElement e;
	e.mAnimation = tStartAnimation;
	e.mSprites = tSprites;
	e.mStep = 0;

	e.mIsFacingRight = 1;
	e.mIsFacingDown = 1;

	e.mOverallTime = 0;
	e.mStepTime = 0;
	e.mHasSprite = 0;

	e.mHasPassiveHitCB = 0;
	e.mHasAttackHitCB = 0;
	e.mHasAnimationFinishedCallback = 0;

	e.mHasPassiveHitboxes = 0;
	e.mHasAttackHitboxes = 0;
	e.mActiveHitboxes.clear();

	e.mPlayerPositionReference = tPosition;
	e.mDrawScale = 1;
	e.mBaseDrawScale = makePosition(1, 1, 1);

	e.mOffset = tPosition;
	e.mHasRectangleWidth = 0;
	e.mHasRectangleHeight = 0;

	e.mHasCameraPositionReference = 0;
	e.mHasCameraScaleReference = 0;
	e.mHasCameraAngleReference = 0;
	e.mHasCameraEffectPositionReference = 0;

	e.mIsInvisible = 0;
	e.mIsColorSolid = 0;
	e.mBaseDrawAngle = 0;

	e.mHasBasePositionReference = 0;
	e.mHasScaleReference = 0;
	e.mHasAngleReference = 0;
	e.mHasBlendType = 0;
	e.mHasConstraintRectangle = 0;

	e.mIsPaused = 0;
	e.mTimeDilatation = 1.0;
	e.mTimeDilatationNow = 0.0;
	e.mIsLooping = 1;
	e.mHasLooped = 0;
	e.mIsCollisionDebugActive = 0;

	e.mR = e.mG = e.mB = e.mAlpha = e.mDestinationAlpha = 1;


	startNewAnimationWithStartStep(&e, 1);

	int id = stl_int_map_push_back(gMugenAnimationHandler.mAnimations, e);
	auto& ret = gMugenAnimationHandler.mAnimations[id];
	ret.mID = id;
	return &ret;
}

void removeMugenAnimation(MugenAnimationHandlerElement* e)
{
	unloadMugenAnimation(e);
	gMugenAnimationHandler.mAnimations.erase(e->mID);
}

int isRegisteredMugenAnimation(MugenAnimationHandlerElement* e) {
	return stl_map_contains(gMugenAnimationHandler.mAnimations, e->mID);
}

int getMugenAnimationAnimationNumber(MugenAnimationHandlerElement* e)
{
	return e->mAnimation->mID;
}

int getMugenAnimationAnimationStep(MugenAnimationHandlerElement* e) {
	return e->mStep;
}

int getMugenAnimationAnimationStepAmount(MugenAnimationHandlerElement* e) {
	return vector_size(&e->mAnimation->mSteps);
}

int getMugenAnimationAnimationStepDuration(MugenAnimationHandlerElement* e)
{
	MugenAnimationStep* step = getCurrentAnimationStep(e);
	return step->mDuration;
}

int getMugenAnimationRemainingAnimationTime(MugenAnimationHandlerElement* e)
{
	if (e->mAnimation->mID == -1) return 0;
	int remainingTime = (e->mAnimation->mTotalDuration - e->mOverallTime) + 1;
	remainingTime = max(0, remainingTime);

	if (e->mHasLooped) return 0;
	return remainingTime;
}

int hasMugenAnimationLooped(MugenAnimationHandlerElement* tElement)
{
	return tElement->mHasLooped;
}

int getMugenAnimationTime(MugenAnimationHandlerElement* e) {
	return e->mOverallTime;
}

int getMugenAnimationDuration(MugenAnimationHandlerElement* e) {
	return e->mAnimation->mTotalDuration;
}

int isMugenAnimationDurationInfinite(MugenAnimationHandlerElement* e)
{
	return e->mAnimation->mTotalDuration >= INF;
}

Vector3DI getMugenAnimationSprite(MugenAnimationHandlerElement* e) {
	MugenAnimationStep* step = getCurrentAnimationStep(e);
	if (!step) return makeVector3DI(-1, -1, 0);
	else return makeVector3DI(step->mGroupNumber, step->mSpriteNumber, 0);
}

void setMugenAnimationFaceDirection(MugenAnimationHandlerElement* e, int tIsFacingRight)
{
	e->mIsFacingRight = tIsFacingRight;
}

void setMugenAnimationVerticalFaceDirection(MugenAnimationHandlerElement* e, int tIsFacingDown) {
	e->mIsFacingDown = tIsFacingDown;
}

void setMugenAnimationRectangleWidth(MugenAnimationHandlerElement* e, int tWidth)
{
	e->mHasRectangleWidth = 1;
	e->mRectangleWidth = tWidth;
}

void setMugenAnimationRectangleHeight(MugenAnimationHandlerElement* e, int tHeight)
{
	e->mHasRectangleHeight = 1;
	e->mRectangleHeight = tHeight;
}

void setMugenAnimationCameraPositionReference(MugenAnimationHandlerElement* e, Position * tCameraPosition)
{
	e->mHasCameraPositionReference = 1;
	e->mCameraPositionReference = tCameraPosition;
}

void removeMugenAnimationCameraPositionReference(MugenAnimationHandlerElement* e)
{
	e->mHasCameraPositionReference = 0;
}

void setMugenAnimationCameraScaleReference(MugenAnimationHandlerElement* e, Position * tCameraScale)
{
	e->mHasCameraScaleReference = 1;
	e->mCameraScaleReference = tCameraScale;
}

void removeMugenAnimationCameraScaleReference(MugenAnimationHandlerElement* e)
{
	e->mHasCameraScaleReference = 0;
}

void setMugenAnimationCameraAngleReference(MugenAnimationHandlerElement* e, double * tCameraAngle)
{
	e->mHasCameraAngleReference = 1;
	e->mCameraAngleReference = tCameraAngle;
}

void removeMugenAnimationCameraAngleReference(MugenAnimationHandlerElement* e)
{
	e->mHasCameraAngleReference = 0;
}

void setMugenAnimationCameraEffectPositionReference(MugenAnimationHandlerElement* e, Position * tCameraEffectPosition)
{
	e->mHasCameraEffectPositionReference = 1;
	e->mCameraEffectPositionReference = tCameraEffectPosition;
}

void removeMugenAnimationCameraEffectPositionReference(MugenAnimationHandlerElement* e)
{
	e->mHasCameraEffectPositionReference = 0;
}

void setMugenAnimationInvisible(MugenAnimationHandlerElement* e)
{
	e->mIsInvisible = 1;
}

void setMugenAnimationVisibility(MugenAnimationHandlerElement* e, int tIsVisible) {
	e->mIsInvisible = !tIsVisible;
}

void setMugenAnimationDrawScale(MugenAnimationHandlerElement* e, Vector3D tScale)
{
	e->mBaseDrawScale = tScale;
}

void setMugenAnimationDrawSize(MugenAnimationHandlerElement* e, Vector3D tSize)
{
	if (!e->mHasSprite) {
		logWarning("Trying to set draw size on element without sprite. Ignoring.");
		return;
	}
	double scaleX = tSize.x / e->mSprite->mOriginalTextureSize.x;
	double scaleY = tSize.y / e->mSprite->mOriginalTextureSize.y;
	double scaleZ = tSize.z / 1;
	setMugenAnimationDrawScale(e, makePosition(scaleX, scaleY, scaleZ));
}

void setMugenAnimationDrawAngle(MugenAnimationHandlerElement* e, double tAngle)
{
	e->mBaseDrawAngle = tAngle;
}

void setMugenAnimationBaseDrawScale(MugenAnimationHandlerElement* e, double tScale)
{
	e->mDrawScale = tScale;
}

void setMugenAnimationBasePosition(MugenAnimationHandlerElement* e, Position * tBasePosition)
{
	e->mHasBasePositionReference = 1;
	e->mBasePositionReference = tBasePosition;
}

void setMugenAnimationScaleReference(MugenAnimationHandlerElement* e, Vector3D * tScale)
{
	e->mHasScaleReference = 1;
	e->mScaleReference = tScale;
}

void setMugenAnimationAngleReference(MugenAnimationHandlerElement* e, double * tAngle)
{
	e->mHasAngleReference = 1;
	e->mAngleReference = tAngle;
}

void setMugenAnimationColor(MugenAnimationHandlerElement* e, double tR, double tG, double tB) {
	e->mR = tR;
	e->mG = tG;
	e->mB = tB;
}

void setMugenAnimationColorSolid(MugenAnimationHandlerElement* e, double tR, double tG, double tB)
{
	setMugenAnimationColor(e, tR, tG, tB);
	e->mIsColorSolid = 1;
}

void setMugenAnimationTransparency(MugenAnimationHandlerElement* e, double tOpacity) {
	e->mAlpha = tOpacity;
}

void setMugenAnimationDestinationTransparency(MugenAnimationHandlerElement* e, double tOpacity)
{
	e->mDestinationAlpha = tOpacity;
}

void setMugenAnimationPosition(MugenAnimationHandlerElement* e, Position tPosition)
{
	e->mOffset = tPosition;
}

void setMugenAnimationBlendType(MugenAnimationHandlerElement* e, BlendType tBlendType)
{
	e->mBlendType = tBlendType;
	e->mHasBlendType = 1;
}

void setMugenAnimationSprites(MugenAnimationHandlerElement* e, MugenSpriteFile * tSprites)
{
	e->mSprites = tSprites;
	updateStepSpriteAndSpriteValidity(e);
}

void setMugenAnimationConstraintRectangle(MugenAnimationHandlerElement* e, GeoRectangle tConstraintRectangle)
{
	e->mConstraintRectangle = tConstraintRectangle;
	e->mHasConstraintRectangle = 1;
}

void setMugenAnimationSpeed(MugenAnimationHandlerElement* tElement, double tSpeed)
{
	tElement->mTimeDilatation = tSpeed;
}

Position getMugenAnimationPosition(MugenAnimationHandlerElement* e)
{
	return e->mOffset;
}

int getMugenAnimationIsFacingRight(MugenAnimationHandlerElement* e) {
	return e->mIsFacingRight;
}

int getMugenAnimationIsFacingDown(MugenAnimationHandlerElement* e) {
	return e->mIsFacingDown;
}

int getMugenAnimationVisibility(MugenAnimationHandlerElement* e) {
	return !e->mIsInvisible;
}

Vector3D getMugenAnimationDrawScale(MugenAnimationHandlerElement* e)
{
	return e->mBaseDrawScale;
}

BlendType getMugenAnimationBlendType(MugenAnimationHandlerElement* e)
{
	if (e->mHasBlendType) {
		return e->mBlendType;
	}
	else {
		return BLEND_TYPE_NORMAL;
	}
}

double getMugenAnimationTransparency(MugenAnimationHandlerElement* e)
{
	return e->mAlpha;
}

double getMugenAnimationDrawAngle(MugenAnimationHandlerElement* e) 
{
	return e->mBaseDrawAngle;
}

double getMugenAnimationColorRed(MugenAnimationHandlerElement* e)
{
	return e->mR;
}

double getMugenAnimationColorGreen(MugenAnimationHandlerElement* e)
{
	return e->mG;
}

double getMugenAnimationColorBlue(MugenAnimationHandlerElement* e)
{
	return e->mB;
}

double * getMugenAnimationColorRedReference(MugenAnimationHandlerElement* e)
{
	return &e->mR;
}

double * getMugenAnimationColorGreenReference(MugenAnimationHandlerElement* e)
{
	return &e->mG;
}

double * getMugenAnimationColorBlueReference(MugenAnimationHandlerElement* e)
{
	return &e->mB;
}

double * getMugenAnimationTransparencyReference(MugenAnimationHandlerElement* e)
{
	return &e->mAlpha;
}

double * getMugenAnimationScaleXReference(MugenAnimationHandlerElement* e)
{
	return &e->mBaseDrawScale.x;
}

double* getMugenAnimationScaleYReference(MugenAnimationHandlerElement* e) {
	return &e->mBaseDrawScale.y;
}

double * getMugenAnimationBaseScaleReference(MugenAnimationHandlerElement* e)
{
	return &e->mDrawScale;
}

Position * getMugenAnimationPositionReference(MugenAnimationHandlerElement* e)
{
	return &e->mOffset;
}

void setMugenAnimationAnimationStepDuration(MugenAnimationHandlerElement * e, int tDuration)
{
	MugenAnimationStep* step = getCurrentAnimationStep(e);
	step->mDuration = tDuration;
}

void changeMugenAnimation(MugenAnimationHandlerElement* e, MugenAnimation * tNewAnimation)
{
	changeMugenAnimationWithStartStep(e, tNewAnimation, 1);
}

void changeMugenAnimationWithStartStep(MugenAnimationHandlerElement* e, MugenAnimation * tNewAnimation, int tStartStep)
{
	e->mAnimation = tNewAnimation;
	startNewAnimationWithStartStep(e, tStartStep);
}

int isStartingMugenAnimationElementWithID(MugenAnimationHandlerElement* e, int tStepID)
{
	if (e->mIsPaused) return 0;
	if (e->mHasLooped) return 0;

	int currentStep = e->mStep + 1;
	return currentStep == tStepID && e->mStepTime == 1;
}

int getTimeFromMugenAnimationElement(MugenAnimationHandlerElement* e, int tStep)
{
	tStep--;

	if (tStep >= vector_size(&e->mAnimation->mSteps)) {
		tStep = vector_size(&e->mAnimation->mSteps) - 1;
	}

	MugenDuration time;
	if (e->mHasLooped) {
		time = getMugenAnimationDuration(e);
	}
	else {
		time = e->mOverallTime;
	}

	MugenDuration sum = getTimeWhenStepStarts(e, tStep);
	int offsetFromStep = time - sum;

	return offsetFromStep;
}

static int getMugenAnimationElementFromTimeOffsetLoop(MugenAnimationHandlerElement* e, int tTime, int tCurrentStep, int dx) {
	int n = vector_size(&e->mAnimation->mSteps);
	if (!n) return 0;

	int isRunning = 1;
	while (isRunning) {
		MugenAnimationStep* step = (MugenAnimationStep*)vector_get(&e->mAnimation->mSteps, tCurrentStep);
		if (tTime < step->mDuration) return tCurrentStep;
		tTime -= step->mDuration;

		tCurrentStep += dx;
		if (tCurrentStep == n) tCurrentStep = e->mAnimation->mLoopStart;
		if (tCurrentStep < e->mAnimation->mLoopStart) tCurrentStep = n - 1;
	}

	return -1;
}

int getMugenAnimationElementFromTimeOffset(MugenAnimationHandlerElement* e, int tTime)
{
	if (!vector_size(&e->mAnimation->mSteps)) return 0;

	int ret;
	if (tTime > 0) {
		tTime += e->mStepTime;
		ret = getMugenAnimationElementFromTimeOffsetLoop(e, tTime, e->mStep, 1);
	}
	else { 
		MugenAnimationStep* step = (MugenAnimationStep*)vector_get(&e->mAnimation->mSteps, e->mStep);
		tTime *= -1;
		tTime += (step->mDuration - 1) - e->mStepTime;
		ret = getMugenAnimationElementFromTimeOffsetLoop(e, tTime, e->mStep, -1);
	}

	return ret + 1;
}

int isMugenAnimationTimeOffsetInAnimation(MugenAnimationHandlerElement* e, int tTime) {
	MugenDuration sum = getTimeWhenStepStarts(e, vector_size(&e->mAnimation->mSteps));
	return tTime < sum;
}

int getMugenAnimationTimeWhenStepStarts(MugenAnimationHandlerElement* e, int tStep) {
	tStep = min(tStep, vector_size(&e->mAnimation->mSteps));
	return getTimeWhenStepStarts(e, tStep);
}

static int updateSingleMugenAnimation(MugenAnimationHandlerElement* e) {
	if (e->mIsPaused) return 0;
	e->mTimeDilatationNow += e->mTimeDilatation;
	int updateAmount = (int)e->mTimeDilatationNow;
	e->mTimeDilatationNow -= updateAmount;
	while (updateAmount--) {
		int ret = 0;
		e->mHasLooped = 0;
		MugenAnimationStep* step = getCurrentAnimationStep(e);
		if (step && !isMugenAnimationStepDurationInfinite(step->mDuration) && e->mStepTime >= step->mDuration) {
			ret = loadNextStepAndReturnIfShouldBeRemoved(e);
		}
		else {
			increaseMugenDuration(&e->mStepTime);
			increaseMugenDuration(&e->mOverallTime);
		}
		if (ret) return 1;
	}
	return 0;
}

static int updateSingleMugenAnimationCB(void* tCaller, MugenAnimationHandlerElement& tData) {
	(void)tCaller;
	MugenAnimationHandlerElement* e = &tData;

	return updateSingleMugenAnimation(e);
}

void advanceMugenAnimationOneTick(MugenAnimationHandlerElement* e)
{
	int shouldBeRemoved = updateSingleMugenAnimation(e);
	if (shouldBeRemoved) {
		removeMugenAnimation(e);
	}
}

static int areMugenAnimationsEqual(const MugenAnimationHandlerElement& a, const MugenAnimationHandlerElement& b) {
	if (a.mAnimation != b.mAnimation) return 0;
	if (a.mStep != b.mStep) return 0;
	if (a.mStepTime != b.mStepTime) return 0;
	return 1;
}

int hasMugenAnimationChanged(MugenAnimationHandlerElement* tElement, const MugenAnimationHandlerElement& tCompareData)
{
	if (tElement->mID != tCompareData.mID) {
		return 0;
	}
	return !areMugenAnimationsEqual(*tElement, tCompareData);
}

MugenAnimationHandlerElement saveMugenAnimation(MugenAnimationHandlerElement * tElement)
{
	return *tElement;
}

void restoreMugenAnimation(MugenAnimationHandlerElement* tElement, const MugenAnimationHandlerElement& tRestorationData)
{
	if (tElement->mID != tRestorationData.mID) {
		logErrorFormat("Unable to restore animation %d, called with invalid ID %d. Ignoring restore request.", tElement->mID, tRestorationData.mID);
		return;
	}
	const auto activeHitDataList = tElement->mActiveHitboxes;
	*tElement = tRestorationData;
	tElement->mActiveHitboxes = activeHitDataList;
	updateHitboxes(tElement);
}

void setMugenAnimationCollisionDebug(MugenAnimationHandlerElement* e, int tIsActive) {
	e->mIsCollisionDebugActive = tIsActive;
}

void pauseMugenAnimation(MugenAnimationHandlerElement* e)
{
	e->mIsPaused = 1;
}

void unpauseMugenAnimation(MugenAnimationHandlerElement* e)
{
	e->mIsPaused = 0;
}

void pauseMugenAnimationHandler() {
	gMugenAnimationHandler.mIsPaused = 1;
}
void unpauseMugenAnimationHandler() {
	gMugenAnimationHandler.mIsPaused = 0;
}

static void updateMugenAnimationHandler(void* tData) {
	(void)tData;

	stl_int_map_remove_predicate(gMugenAnimationHandler.mAnimations, updateSingleMugenAnimationCB);
}

void setMugenAnimationCollisionActive(MugenAnimationHandlerElement* e, CollisionListData* tCollisionList, void(*tFunc)(void*, void*, int), void* tCaller, void* tCollisionData)
{
	setMugenAnimationPassiveCollisionActive(e, tCollisionList, tFunc, tCaller, tCollisionData);
	setMugenAnimationAttackCollisionActive(e, tCollisionList, NULL, NULL, NULL);
}

void setMugenAnimationPassiveCollisionActive(MugenAnimationHandlerElement* e, CollisionListData* tCollisionList, void(*tFunc)(void *, void *, int), void * tCaller, void * tCollisionData)
{
	e->mHasPassiveHitboxes = 1;
	e->mPassiveCollisionList = tCollisionList;
	e->mPassiveCollisionData = tCollisionData;
	e->mHasPassiveHitCB = 1;
	e->mPassiveHitCB = tFunc;
	e->mPassiveHitCaller = tCaller;
}

void setMugenAnimationAttackCollisionActive(MugenAnimationHandlerElement* e, CollisionListData* tCollisionList, void(*tFunc)(void *, void *, int), void * tCaller, void * tCollisionData)
{
	e->mHasAttackHitboxes = 1;
	e->mAttackCollisionList = tCollisionList;
	e->mAttackCollisionData = tCollisionData;
	e->mHasAttackHitCB = 1;
	e->mAttackHitCB = tFunc;
	e->mAttackHitCaller = tCaller;
}

void setMugenAnimationNoLoop(MugenAnimationHandlerElement* e) {
	e->mIsLooping = 0;
}

void setMugenAnimationCallback(MugenAnimationHandlerElement* e, void(*tFunc)(void*), void* tCaller) {
	e->mHasAnimationFinishedCallback = 1;
	e->mAnimationFinishedCB = tFunc;
	e->mAnimationFinishedCaller = tCaller;
}

typedef struct {
	MugenAnimationHandlerElement* e;
	MugenAnimationStep* mStep;
	Position mScalePosition;
	Vector3D mScale;
	double mAngle;

	Position mBasePosition;

	Position mCameraCenter;
} DrawSingleMugenAnimationSpriteCaller;

static void drawSingleMugenAnimationSpriteCB(void* tCaller, void* tData) {
	DrawSingleMugenAnimationSpriteCaller* caller = (DrawSingleMugenAnimationSpriteCaller*)tCaller;
	MugenSpriteFileSubSprite* sprite = (MugenSpriteFileSubSprite*)tData;

	MugenAnimationHandlerElement* e = caller->e;
	MugenAnimationStep* step = caller->mStep;
	Position p = caller->mBasePosition;
	p = vecAdd(p, makePosition(sprite->mOffset.x, sprite->mOffset.y, 0));

	const auto realSpriteSize = vecMinI2D(makeVector3DI(e->mSprite->mOriginalTextureSize.x, e->mSprite->mOriginalTextureSize.y, 0) - sprite->mOffset, makeVector3DI(sprite->mTexture.mTextureSize.x, sprite->mTexture.mTextureSize.y, 0));
	auto texturePos = makeRectangle(0, 0, realSpriteSize.x - 1, realSpriteSize.y - 1);

	if (e->mHasRectangleWidth) {
		int newWidth = e->mRectangleWidth - sprite->mOffset.x;
		if (newWidth <= 0) return;
		newWidth = min(newWidth, sprite->mTexture.mTextureSize.x);
		texturePos.bottomRight.x = texturePos.topLeft.x + newWidth;
	}

	if (e->mHasRectangleHeight) {
		int newHeight = e->mRectangleHeight - sprite->mOffset.y;
		if (newHeight <= 0) return;
		newHeight = min(newHeight, sprite->mTexture.mTextureSize.y);
		texturePos.bottomRight.y = texturePos.topLeft.y + newHeight;
	}

	if (e->mHasConstraintRectangle) {

		int minWidth = texturePos.topLeft.x;
		int maxWidth = texturePos.bottomRight.x;

		const auto leftX = max(minWidth, (int)(e->mConstraintRectangle.mTopLeft.x - p.x));
		const auto rightX = min(maxWidth, (int)(e->mConstraintRectangle.mBottomRight.x - p.x));
		if (leftX > rightX) return;

		p.x += leftX - texturePos.topLeft.x;
		texturePos.topLeft.x = leftX;
		texturePos.bottomRight.x = rightX;

		int minHeight = texturePos.topLeft.y;
		int maxHeight = texturePos.bottomRight.y;

		const auto upY = max(minHeight, (int)(e->mConstraintRectangle.mTopLeft.y - p.y));
		const auto downY = min(maxHeight, (int)(e->mConstraintRectangle.mBottomRight.y - p.y));
		if (upY > downY) return;

		p.y += upY - texturePos.topLeft.y;
		texturePos.topLeft.y = upY;
		texturePos.bottomRight.y = downY;
	}

	int isFacingRight = e->mIsFacingRight;
	if (step->mIsFlippingHorizontally) isFacingRight ^= 1;

	if (!isFacingRight) {
		Rectangle originalTexturePos = texturePos;
		Position center = e->mPlayerPositionReference;
		double deltaX = center.x - p.x;
		double nRightX = center.x + deltaX;
		double nLeftX = nRightX - abs(originalTexturePos.bottomRight.x - originalTexturePos.topLeft.x);
		p.x = nLeftX;
		texturePos.topLeft.x = originalTexturePos.bottomRight.x;
		texturePos.bottomRight.x = originalTexturePos.topLeft.x;
	}

	int isFacingDown = e->mIsFacingDown;
	if (step->mIsFlippingVertically) isFacingDown ^= 1;

	if (!isFacingDown) {
		Rectangle originalTexturePos = texturePos;
		Position center = e->mPlayerPositionReference;
		double deltaY = center.y - p.y;
		double nRightY = center.y + deltaY;
		double nLeftY = nRightY - abs(originalTexturePos.bottomRight.y - originalTexturePos.topLeft.y);
		p.y = nLeftY;
		texturePos.topLeft.y = originalTexturePos.bottomRight.y;
		texturePos.bottomRight.y = originalTexturePos.topLeft.y;
	}

	Vector3D animationStepDelta = step->mDelta;
	if (!e->mIsFacingRight) {
		animationStepDelta.x = -animationStepDelta.x;
	}
	p = vecAdd2D(p, animationStepDelta); 

	if (e->mHasCameraPositionReference) {
		p = vecSub(p, *e->mCameraPositionReference);
	}

	if (step->mIsAddition) {
		setDrawingBlendType(BLEND_TYPE_ADDITION);
	}
	else if (e->mHasBlendType && e->mBlendType != BLEND_TYPE_NORMAL) {
		setDrawingBlendType(e->mBlendType);
	}

	setDrawingColorSolidity(e->mIsColorSolid);
	setDrawingBaseColorAdvanced(e->mR, e->mG, e->mB);
	setDrawingTransparency(e->mAlpha);
	setDrawingDestinationTransparency(e->mDestinationAlpha);

	if (e->mHasCameraScaleReference && *e->mCameraScaleReference != makePosition(1, 1, 1)) {
		scaleDrawing3D(*e->mCameraScaleReference, caller->mCameraCenter + makePosition(0.5, 0.5, 0));
	}
	if (e->mHasCameraAngleReference && *e->mCameraAngleReference) {
		setDrawingRotationZ(*e->mCameraAngleReference, caller->mCameraCenter + makePosition(0.5, 0.5, 0));
	}

	if (caller->mScale != makePosition(1, 1, 1)) {
		scaleDrawing3D(caller->mScale, caller->mScalePosition + makePosition(0.5, 0.5, 0));
	}

	if (caller->mAngle) {
		setDrawingRotationZ(caller->mAngle, caller->mScalePosition + makePosition(0.5, 0.5, 0));
	}

	drawSprite(sprite->mTexture, p, texturePos);
	setDrawingParametersToIdentity();
}

typedef struct {
	MugenAnimationHandlerElement* e;
	Position mPosition;

} DebugCollisionHitboxDrawCaller;

static void drawSingleAnimationSingleDebugCollisionHitbox(DebugCollisionHitboxDrawCaller* tCaller, MugenAnimationHandlerHitboxElement& tData) {
	MugenAnimationHandlerHitboxElement* element = &tData;

	Position cameraOffset;
	if (tCaller->e->mHasCameraPositionReference) {
		cameraOffset = *tCaller->e->mCameraPositionReference;
	}
	else {
		cameraOffset = makePosition(0, 0, 0);
	}

	Vector3D color;
	if (element->mList == tCaller->e->mPassiveCollisionList) {
		color = makePosition(0, 0, 1);
	}
	else {
		color = makePosition(1, 0, 0);
	}

	double alpha = 0.3;


	drawColliderSolid(element->mCollider, tCaller->e->mPlayerPositionReference, cameraOffset, color, alpha);
}

static void drawSingleAnimationDebugCollisionHitboxes(MugenAnimationHandlerElement* e) {

	DebugCollisionHitboxDrawCaller caller;
	caller.e = e;
	stl_list_map(e->mActiveHitboxes, drawSingleAnimationSingleDebugCollisionHitbox, &caller);
}

static void drawSingleMugenAnimation(void* tCaller, MugenAnimationHandlerElement& tData) {
	(void)tCaller;
	MugenAnimationHandlerElement* e = &tData;

	if (e->mIsCollisionDebugActive) {
		drawSingleAnimationDebugCollisionHitboxes(e);
	}
	
	if (e->mHasBasePositionReference) {
		e->mPlayerPositionReference = *e->mBasePositionReference + e->mOffset;
	}
	else {
		e->mPlayerPositionReference = e->mOffset;
	}

	if (e->mIsInvisible) {
		return;
	}
	if (!e->mHasSprite) {
		return;
	}

	MugenAnimationStep* step = getCurrentAnimationStep(e);
	if (!step) return;

	Vector3D drawScale = e->mBaseDrawScale;
	drawScale = vecScale(drawScale, e->mDrawScale);
	if (e->mHasScaleReference) {
		drawScale.x *= e->mScaleReference->x;
		drawScale.y *= e->mScaleReference->y;
	}
	drawScale.z = 1;
	
	Position p = e->mPlayerPositionReference;

	if (e->mHasCameraPositionReference) {
		p = vecSub(p, *e->mCameraPositionReference);
	}

	Position scalePosition = p;
	p = vecSub(p, e->mSprite->mAxisOffset);
	Vector3D cameraCenter;

	Position effectPosition;
	if (!e->mHasCameraEffectPositionReference) {
		ScreenSize sz = getScreenSize();
		effectPosition = makePosition(sz.x / 2, sz.y / 2, 0);
	}
	else {
		effectPosition = *e->mCameraEffectPositionReference;
	}

	if (e->mHasCameraPositionReference) {
		p = vecAdd(p, *e->mCameraPositionReference);
		cameraCenter = vecAdd(*e->mCameraPositionReference, effectPosition);
	}
	else {
		cameraCenter = effectPosition;
	}

	double angle = e->mBaseDrawAngle;
	if (e->mHasAngleReference) {
		angle += *e->mAngleReference;
	}

	DrawSingleMugenAnimationSpriteCaller caller;
	caller.e = e;
	caller.mStep = step;
	caller.mScale = drawScale;
	caller.mScalePosition = scalePosition;
	caller.mBasePosition = p;
	caller.mAngle = angle;
	caller.mCameraCenter = cameraCenter;
	
	list_map(&e->mSprite->mTextures, drawSingleMugenAnimationSpriteCB, &caller);
}

static void drawMugenAnimationHandler(void* tData) {
	(void)tData;
	stl_int_map_map(gMugenAnimationHandler.mAnimations, drawSingleMugenAnimation);
}


ActorBlueprint getMugenAnimationHandler() {
	return makeActorBlueprint(loadMugenAnimationHandler, unloadMugenAnimationHandler, updateMugenAnimationHandler, drawMugenAnimationHandler);
}
