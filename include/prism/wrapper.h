#pragma once

#include <stdio.h>

typedef void (*LoadScreenFunction)();
typedef void (*UpdateScreenFunction)();
typedef void (*DrawScreenFunction)();
typedef void (*UnloadScreenFunction)();

typedef struct Screen_internal{
	LoadScreenFunction mLoad;
	UpdateScreenFunction mUpdate;
	DrawScreenFunction mDraw;
	UnloadScreenFunction mUnload;
	struct Screen_internal* (*mGetNextScreen)();
} Screen;

void initPrismWrapperWithDefaultFlags();
void initPrismWrapperWithMugenFlags();
void initPrismWrapperWithoutMemoryManagement();
void initPrismWrapperWithConfigFile(char* tPath);
void shutdownPrismWrapper();
void pauseWrapper();
void resumeWrapper();
int isWrapperPaused();
int isUsingWrapper();

Screen makeScreen(LoadScreenFunction tLoad, UpdateScreenFunction tUpdate = NULL, DrawScreenFunction tDraw = NULL, UnloadScreenFunction tUnload = NULL, Screen*(*tGetNextScreen)() = NULL);
void startScreenHandling(Screen* tScreen);
void abortScreenHandling();
void setNewScreen(Screen* tScreen);

void setWrapperTimeDilatation(double tDilatation);
void setWrapperBetweenScreensCB(void(*tCB)(void*), void* tCaller);
void setWrapperIsPausingTracksBetweenScreens(int tIsPausingTracks);
void setWrapperToExhibitionMode();

void setWrapperTitleScreen(Screen* tTitleScreen);
void recoverWrapperError();
void gotoNextScreenAfterWrapperError();
void disableWrapperErrorRecovery();

#define EXPORT_TEST(ScreenName) { void func_##ScreenName(); }

#define EXPORT_SCREEN_CLASS(tScreenName) \
	static std::unique_ptr<tScreenName> g##tScreenName; \
		\
	static void loadScreenContainer##tClassName() { \
		g##tScreenName = std::make_unique<tScreenName>(); \
	} \
		\
	static void updateScreenContainer##tClassName() { \
		g##tScreenName->update(); \
	} \
		\
	static void unloadScreenContainer##tClassName() { \
		g##tScreenName = nullptr;  \
	} \
		\
	static Screen gScreenContainer##tClassName; \
		\
	Screen* get##tScreenName() \
	{ \
		gScreenContainer##tClassName = makeScreen(loadScreenContainer##tClassName, updateScreenContainer##tClassName, NULL, unloadScreenContainer##tClassName); \
		return &gScreenContainer##tClassName; \
	}
