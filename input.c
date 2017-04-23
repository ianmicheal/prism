#include "include/input.h"

#include "include/log.h"

typedef struct {
	int mAFlank;
	int mBFlank;
	int mXFlank;
	int mYFlank;
	int mLFlank;
	int mRFlank;
	int mLeftFlank;
	int mRightFlank;
	int mUpFlank;
	int mDownFlank;
	int mStartFlank;
	int mAbortFlank;
} InputFlanks;

static struct {
	InputFlanks mFlanks[MAXIMUM_CONTROLLER_AMOUNT];

	int mMainController;
} gData;

void resetInput() {
	int i;
	for (i = 0; i < MAXIMUM_CONTROLLER_AMOUNT; i++) {
		hasPressedAFlankSingle(i);
		hasPressedBFlankSingle(i);
		hasPressedXFlankSingle(i);
		hasPressedYFlankSingle(i);
		hasPressedLFlankSingle(i);
		hasPressedRFlankSingle(i);
		hasPressedLeftFlankSingle(i);
		hasPressedRightFlankSingle(i);
		hasPressedUpFlankSingle(i);
		hasPressedDownFlankSingle(i);
		hasPressedStartFlankSingle(i);
		hasPressedAbortFlankSingle(i);
	}
}

static int hasPressedFlank(int tCurrent, int* tFlank) {
	int returnValue = 0;

	debugInteger(tCurrent);
	debugInteger((*tFlank));
	if (tCurrent && !(*tFlank)) {
		returnValue = 1;
	}

	(*tFlank) = tCurrent;
	return returnValue;
}


int hasPressedAFlankSingle(int i) {
	return hasPressedFlank(hasPressedASingle(i), &gData.mFlanks[i].mAFlank);
}

int hasPressedBFlankSingle(int i) {
	return hasPressedFlank(hasPressedBSingle(i), &gData.mFlanks[i].mBFlank);
}
int hasPressedXFlankSingle(int i) {
	return hasPressedFlank(hasPressedXSingle(i), &gData.mFlanks[i].mXFlank);
}

int hasPressedYFlankSingle(int i) {
	return hasPressedFlank(hasPressedYSingle(i), &gData.mFlanks[i].mYFlank);
}

int hasPressedLeftFlankSingle(int i) {
	return hasPressedFlank(hasPressedLeftSingle(i), &gData.mFlanks[i].mLeftFlank);
}

int hasPressedRightFlankSingle(int i) {
	return hasPressedFlank(hasPressedRightSingle(i), &gData.mFlanks[i].mRightFlank);
}

int hasPressedUpFlankSingle(int i) {
	return hasPressedFlank(hasPressedUpSingle(i), &gData.mFlanks[i].mUpFlank);
}

int hasPressedDownFlankSingle(int i) {
	return hasPressedFlank(hasPressedDownSingle(i), &gData.mFlanks[i].mDownFlank);
}

int hasPressedLFlankSingle(int i) {
	return hasPressedFlank(hasPressedLSingle(i), &gData.mFlanks[i].mLFlank);
}

int hasPressedRFlankSingle(int i) {
	return hasPressedFlank(hasPressedRSingle(i), &gData.mFlanks[i].mRFlank);
}

int hasPressedStartFlankSingle(int i) {
	return hasPressedFlank(hasPressedStartSingle(i), &gData.mFlanks[i].mStartFlank);
}

int hasPressedAbortFlankSingle(int i) {
	return hasPressedFlank(hasPressedAbortSingle(i), &gData.mFlanks[i].mAbortFlank);
}


int hasPressedAFlank() {
	return hasPressedAFlankSingle(getMainController());
}

int hasPressedBFlank() {
	return hasPressedBFlankSingle(getMainController());
}

int hasPressedXFlank() {
	return hasPressedXFlankSingle(getMainController());
}

int hasPressedYFlank() {
	return hasPressedYFlankSingle(getMainController());
}

int hasPressedLeftFlank() {
	return hasPressedLeftFlankSingle(getMainController());
}

int hasPressedRightFlank() {
	return hasPressedRightFlankSingle(getMainController());
}

int hasPressedUpFlank() {
	return hasPressedUpFlankSingle(getMainController());
}

int hasPressedDownFlank() {
	return hasPressedDownFlankSingle(getMainController());
}

int hasPressedLFlank() {
	return hasPressedLFlankSingle(getMainController());
}

int hasPressedRFlank() {
	return hasPressedRFlankSingle(getMainController());
}

int hasPressedStartFlank() {
	return hasPressedStartFlankSingle(getMainController());
}

int hasPressedAbortFlank() {
	return hasPressedAbortFlankSingle(getMainController());
}



int hasPressedA() {
	return hasPressedASingle(getMainController());
}

int hasPressedB() {
	return hasPressedBSingle(getMainController());
}

int hasPressedX() {
	return hasPressedXSingle(getMainController());
}

int hasPressedY() {
	return hasPressedYSingle(getMainController());
}

int hasPressedLeft() {
	return hasPressedLeftSingle(getMainController());
}

int hasPressedRight() {
	return hasPressedRightSingle(getMainController());
}

int hasPressedUp() {
	return hasPressedUpSingle(getMainController());
}

int hasPressedDown() {
	return hasPressedDownSingle(getMainController());
}

int hasPressedL() {
	return hasPressedLSingle(getMainController());
}

int hasPressedR() {
	return hasPressedRSingle(getMainController());
}

int hasPressedStart() {
	return hasPressedStartSingle(getMainController());
}

int hasPressedAbort() {
	return hasPressedAbortSingle(getMainController());
}

double getLeftStickNormalizedX() {
	return getSingleLeftStickNormalizedX(getMainController());
}

double getLeftStickNormalizedY() {
	return getSingleLeftStickNormalizedY(getMainController());
}

double getLNormalized() {
	return getSingleLNormalized(getMainController());
}

double getRNormalized() {
	return getSingleRNormalized(getMainController());
}


void setMainController(int i) {
	gData.mMainController = i;
}

int getMainController() {
	return gData.mMainController;
}