#include "tari/mugenanimationhandler.h"

#include "tari/collisionhandler.h"
#include "tari/math.h"

typedef int MugenDuration;

typedef struct {
	int mList;
	int mID;
	Collider mCollider;
} MugenAnimationHandlerHitboxElement;

typedef struct {

	MugenAnimation* mAnimation;
	MugenSpriteFile* mSprites;
	int mStep;

	int mIsFacingRight;

	MugenDuration mOverallTime;
	MugenDuration mStepTime;
	MugenSpriteFileSprite* mSprite;
	int mHasSprite;

	int mHasHitCB;
	void* mHitCaller;
	void(*mHitCB)(void* tCaller, void* tCollisionData);

	int mHasHitboxes;
	int mCollisionList;
	void* mCollisionData;

	List mActiveHitboxes;

	Position mPlayerPositionReference;

	double mDrawScale;

	Vector3D mBaseDrawScale;

	Position mOffset;

	int mHasRectangleWidth;
	int mRectangleWidth;

	int mHasCameraPositionReference;
	Position* mCameraPositionReference;

	int mIsInvisible;

	double mBaseDrawAngle;

	int mHasBasePositionReference;
	Position* mBasePositionReference;

	int mIsPaused;
} MugenAnimationHandlerElement;

static struct {
	IntMap mAnimations;
} gData;

static void loadMugenAnimationHandler(void* tData) {
	(void)tData;
	gData.mAnimations = new_int_map();
}

static MugenAnimationStep* getCurrentAnimationStep(MugenAnimationHandlerElement* e) {
	return vector_get(&e->mAnimation->mSteps, min(e->mStep, vector_size(&e->mAnimation->mSteps) - 1));
}


static void passiveAnimationHitCB(void* tCaller, void* tCollisionData) {
	MugenAnimationHandlerElement* e = tCaller;
	if (!e->mHasHitCB) return;
	e->mHitCB(e->mHitCaller, tCollisionData);
}


static void attackAnimationHitCB(void* tCaller, void* tCollisionData) {
	(void)tCaller;
	(void)tCollisionData;
}

typedef struct {
	MugenAnimationHandlerElement* mElement;
	int mList;
	CollisionCallback mCB;
} HitboxAdditionCaller;

static void addSingleHitbox(void* tCaller, void* tData) {
	HitboxAdditionCaller* caller = tCaller;
	CollisionRect* rect = tData;

	MugenAnimationHandlerHitboxElement* e = allocMemory(sizeof(MugenAnimationHandlerHitboxElement));

	CollisionRect scaledRectangle = *rect;
	scaledRectangle = scaleGeoRectangleByFactor(scaledRectangle, caller->mElement->mDrawScale);
	if (!caller->mElement->mIsFacingRight) {
		double xBuffer = scaledRectangle.mTopLeft.x;
		scaledRectangle.mTopLeft.x = -scaledRectangle.mBottomRight.x;
		scaledRectangle.mBottomRight.x = -xBuffer;
	}

	e->mCollider = makeColliderFromRect(scaledRectangle);

	e->mList = caller->mList;
	e->mID = addColliderToCollisionHandler(caller->mList, &caller->mElement->mPlayerPositionReference, e->mCollider, caller->mCB, caller->mElement, caller->mElement->mCollisionData);

	list_push_back_owned(&caller->mElement->mActiveHitboxes, e);
}

static void addNewSingleHitboxType(MugenAnimationHandlerElement* e, List* tHitboxes, int tList, CollisionCallback tCB) {
	HitboxAdditionCaller caller;
	caller.mElement = e;
	caller.mList = tList;
	caller.mCB = tCB;

	list_map(tHitboxes, addSingleHitbox, &caller);
}

static void addNewHitboxes(MugenAnimationHandlerElement* e, MugenAnimationStep* tStep) {
	if (!e->mHasHitboxes) return;
	addNewSingleHitboxType(e, &tStep->mPassiveHitboxes, e->mCollisionList, passiveAnimationHitCB);
	addNewSingleHitboxType(e, &tStep->mAttackHitboxes, e->mCollisionList, attackAnimationHitCB);
}

static int removeSingleHitbox(void* tCaller, void* tData) {
	(void)tCaller;
	MugenAnimationHandlerHitboxElement* hitbox = tData;

	removeFromCollisionHandler(hitbox->mList, hitbox->mID);
	destroyCollider(&hitbox->mCollider);
	return 1;
}

static void removeOldHitboxes(MugenAnimationHandlerElement* e) {
	list_remove_predicate(&e->mActiveHitboxes, removeSingleHitbox, NULL);
}

static void updateHitboxes(MugenAnimationHandlerElement* e) {
	removeOldHitboxes(e);
	addNewHitboxes(e, getCurrentAnimationStep(e));
}

static MugenDuration getTimeWhenStepStarts(MugenAnimationHandlerElement* e, int tStep) {
	MugenDuration sum = 0;
	int i;
	for (i = 0; i < tStep; i++) {
		MugenAnimationStep* step = vector_get(&e->mAnimation->mSteps, i);
		sum += step->mDuration;
	}
	return sum;

}

static void loadNextStep(MugenAnimationHandlerElement* e) {
	e->mStep++;
	if (e->mStep == vector_size(&e->mAnimation->mSteps)) {
		e->mStep = e->mAnimation->mLoopStart;
		e->mOverallTime = getTimeWhenStepStarts(e, e->mStep); // TODO: test
	}

	e->mStepTime = 0;

	MugenAnimationStep* step = getCurrentAnimationStep(e);
	e->mSprite = getMugenSpriteFileTextureReference(e->mSprites, step->mGroupNumber, step->mSpriteNumber);
	e->mHasSprite = e->mSprite != NULL;

	updateHitboxes(e);
}

static void increaseMugenDuration(MugenDuration* tDuration) {
	(*tDuration)++;
}

static void startNewAnimationWithStartStep(MugenAnimationHandlerElement* e, int tStartStep) {
	e->mOverallTime = getTimeWhenStepStarts(e, tStartStep);

	e->mStep = tStartStep - 1;
	loadNextStep(e);
}

int addMugenAnimation(MugenAnimation* tStartAnimation, MugenSpriteFile* tSprites, Position tPosition)
{
	MugenAnimationHandlerElement* e = allocMemory(sizeof(MugenAnimationHandlerElement));
	e->mAnimation = tStartAnimation;
	e->mSprites = tSprites;
	e->mStep = 0;

	e->mIsFacingRight = 1;

	e->mOverallTime = 0;
	e->mStepTime = 0;
	e->mHasSprite = 0;

	e->mHasHitCB = 0;

	e->mHasHitboxes = 0;
	e->mActiveHitboxes = new_list();

	e->mPlayerPositionReference = tPosition;
	e->mDrawScale = 1;
	e->mBaseDrawScale = makePosition(1, 1, 1);

	e->mOffset = tPosition;
	e->mHasRectangleWidth = 0;

	e->mHasCameraPositionReference = 0;
	e->mIsInvisible = 0;
	e->mBaseDrawAngle = 0;

	e->mHasBasePositionReference = 0;

	e->mIsPaused = 0;

	startNewAnimationWithStartStep(e, 0);

	return int_map_push_back_owned(&gData.mAnimations, e);
}

void removeMugenAnimation(int tID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	removeOldHitboxes(e);
	delete_list(&e->mActiveHitboxes);

	int_map_remove(&gData.mAnimations, tID);
}

int getMugenAnimationAnimationNumber(int tID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	return e->mAnimation->mID;
}

int getMugenAnimationRemainingAnimationTime(int tID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	int remainingTime = (e->mAnimation->mTotalDuration - e->mOverallTime) - 1;

	remainingTime = max(0, remainingTime); // TODO: fix when reading out

	return remainingTime;
}

void setMugenAnimationFaceDirection(int tID, int tIsFacingRight)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mIsFacingRight = tIsFacingRight;
}

void setMugenAnimationRectangleWidth(int tID, int tWidth)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mHasRectangleWidth = 1;
	e->mRectangleWidth = tWidth;
}

void setMugenAnimationCameraPositionReference(int tID, Position * tCameraPosition)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mHasCameraPositionReference = 1;
	e->mCameraPositionReference = tCameraPosition;
}

void setMugenAnimationInvisible(int tID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mIsInvisible = 1;
}

void setMugenAnimationDrawScale(int tID, Vector3D tScale)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mBaseDrawScale = tScale;
}

void setMugenAnimationDrawAngle(int tID, double tAngle)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mBaseDrawAngle = tAngle;
}

void setMugenAnimationBaseDrawScale(int tID, double tScale)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mDrawScale = tScale;
}

void setMugenAnimationBasePosition(int tID, Position * tBasePosition)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mHasBasePositionReference = 1;
	e->mBasePositionReference = tBasePosition;
}

void changeMugenAnimation(int tID, MugenAnimation * tNewAnimation)
{
	changeMugenAnimationWithStartStep(tID, tNewAnimation, 0);
}

void changeMugenAnimationWithStartStep(int tID, MugenAnimation * tNewAnimation, int tStartStep)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mAnimation = tNewAnimation;
	startNewAnimationWithStartStep(e, tStartStep);
}

int isStartingMugenAnimationElementWithID(int tID, int tStepID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	if (e->mIsPaused) return 0;

	int currentStep = e->mStep + 1;
	return currentStep == tStepID && e->mStepTime == 0;
}



int getTimeFromMugenAnimationElement(int tID, int tStep)
{
	tStep--;

	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	if (tStep >= vector_size(&e->mAnimation->mSteps)) {
		tStep = vector_size(&e->mAnimation->mSteps) - 1; // TODO: think about this
	}

	MugenDuration sum = getTimeWhenStepStarts(e, tStep);

	int offsetFromStep = e->mOverallTime - sum;

	return offsetFromStep;
}

static int getMugenAnimationElementFromTimeOffsetLoop(MugenAnimationHandlerElement* e, int tTime, int tCurrentStep, int dx) {
	int n = vector_size(&e->mAnimation->mSteps);

	int isRunning = 1;
	while (isRunning) {
		MugenAnimationStep* step = vector_get(&e->mAnimation->mSteps, tCurrentStep);
		if (tTime < step->mDuration) return tCurrentStep;
		tTime -= step->mDuration;

		tCurrentStep += dx;
		if (tCurrentStep == n) tCurrentStep = e->mAnimation->mLoopStart;
		if (tCurrentStep < e->mAnimation->mLoopStart) tCurrentStep = n - 1;
	}

	return -1;
}

int getMugenAnimationElementFromTimeOffset(int tID, int tTime)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);

	int ret;
	if (tTime > 0) {
		tTime += e->mStepTime;
		ret = getMugenAnimationElementFromTimeOffsetLoop(e, tTime, e->mStep, 1);
	}
	else {
		MugenAnimationStep* step = vector_get(&e->mAnimation->mSteps, e->mStep);
		tTime *= -1;
		tTime += (step->mDuration - 1) - e->mStepTime;
		ret = getMugenAnimationElementFromTimeOffsetLoop(e, tTime, e->mStep, -1);
	}

	return ret + 1;
}

static void updateSingleMugenAnimation(void* tCaller, void* tData) {
	(void)tCaller;
	MugenAnimationHandlerElement* e = tData;

	if (e->mIsPaused) return;

	MugenAnimationStep* step = getCurrentAnimationStep(e);
	increaseMugenDuration(&e->mOverallTime);
	increaseMugenDuration(&e->mStepTime);
	if (step->mDuration == -1) return;
	if (e->mStepTime >= step->mDuration) {
		loadNextStep(e);
	}
}

void pauseMugenAnimation(int tID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mIsPaused = 1;
}

void unpauseMugenAnimation(int tID)
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mIsPaused = 0;
}


static void updateMugenAnimationHandler(void* tData) {
	(void)tData;

	int_map_map(&gData.mAnimations, updateSingleMugenAnimation, NULL);
}

void setMugenAnimationCollisionActive(int tID, int tCollisionList, void(*tFunc)(void*, void*), void* tCaller, void* tCollisionData) 
{
	MugenAnimationHandlerElement* e = int_map_get(&gData.mAnimations, tID);
	e->mHasHitboxes = 1;
	e->mCollisionList = tCollisionList;
	e->mCollisionData = tCollisionData;
	e->mHasHitCB = 1;
	e->mHitCB = tFunc;
	e->mHitCaller = tCaller;
}


typedef struct {
	MugenAnimationHandlerElement* e;
	MugenAnimationStep* mStep;
	Position mScalePosition;
	Vector3D mScale;

	Position mBasePosition;

} DrawSingleMugenAnimationSpriteCaller;

static void drawSingleMugenAnimationSpriteCB(void* tCaller, void* tData) {
	DrawSingleMugenAnimationSpriteCaller* caller = tCaller;
	MugenSpriteFileSubSprite* sprite = tData;

	MugenAnimationHandlerElement* e = caller->e;
	MugenAnimationStep* step = caller->mStep;
	Position p = caller->mBasePosition;
	p = vecAdd(p, makePosition(sprite->mOffset.x, sprite->mOffset.y, 0));

	Rectangle texturePos = makeRectangleFromTexture(sprite->mTexture);
	if (e->mHasRectangleWidth) {
		int newWidth = e->mRectangleWidth - sprite->mOffset.x;
		if (newWidth <= 0) return;
		newWidth = min(newWidth, sprite->mTexture.mTextureSize.x);
		texturePos.bottomRight.x = texturePos.topLeft.x + newWidth;
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

	if (e->mHasCameraPositionReference) {
		p = vecSub(p, *e->mCameraPositionReference);
	}

	if (step->mIsAddition) {
		setDrawingBlendType(BLEND_TYPE_ADDITION);
	}

	scaleDrawing3D(caller->mScale, caller->mScalePosition);
	setDrawingRotationZ(e->mBaseDrawAngle, caller->mScalePosition);
	drawSprite(sprite->mTexture, p, texturePos);
	setDrawingParametersToIdentity();
}

static void drawSingleMugenAnimation(void* tCaller, void* tData) {
	(void)tCaller;
	MugenAnimationHandlerElement* e = tData;

	if (e->mIsInvisible) {
		return;
	}

	if (!e->mHasSprite) return;

	MugenAnimationStep* step = getCurrentAnimationStep(e);

	if (e->mHasBasePositionReference) {
		e->mPlayerPositionReference = *e->mBasePositionReference;
	}
	else {
		e->mPlayerPositionReference = makePosition(0, 0, 0);
	}

	e->mPlayerPositionReference = vecAdd(e->mPlayerPositionReference, e->mOffset);
	Position p = e->mPlayerPositionReference;

	Vector3D drawScale = e->mBaseDrawScale;
	drawScale = vecScale(drawScale, e->mDrawScale);
	drawScale.z = 1;

	if (e->mHasCameraPositionReference) {
		p = vecSub(p, *e->mCameraPositionReference);
	}

	Position scalePosition = p;
	p = vecSub(p, e->mSprite->mAxisOffset);

	if (e->mHasCameraPositionReference) {
		p = vecAdd(p, *e->mCameraPositionReference);
	}

	DrawSingleMugenAnimationSpriteCaller caller;
	caller.e = e;
	caller.mStep = step;
	caller.mScale = drawScale;
	caller.mScalePosition = scalePosition;
	caller.mBasePosition = p;

	list_map(&e->mSprite->mTextures, drawSingleMugenAnimationSpriteCB, &caller);
}

static void drawMugenAnimationHandler(void* tData) {
	(void)tData;
	int_map_map(&gData.mAnimations, drawSingleMugenAnimation, NULL);
}


static ActorBlueprint MugenAnimationHandler = {
	.mLoad = loadMugenAnimationHandler,
	.mUpdate = updateMugenAnimationHandler,
	.mDraw = drawMugenAnimationHandler,
};

fup ActorBlueprint getMugenAnimationHandlerActorBlueprint()
{
	return MugenAnimationHandler;
}