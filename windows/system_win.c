#include "tari/system.h"

#include <direct.h>
#include <string.h>
#include <windows.h>

#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>

#include "tari/log.h"
#include "tari/pvr.h"

void abortSystem(){
	exit(0);
}	

static struct {

	int mIsLoaded;
	
	int mScreenSizeX;
	int mScreenSizeY;

	int mIsFullscreen;

	char mGameName[100];
} gData;

SDL_Window* gSDLWindow;

static void initScreenDefault() {
	gData.mIsLoaded = 1;
	gData.mScreenSizeX = 640;
	gData.mScreenSizeY = 480;

	gData.mIsFullscreen = 0;
}

void setGameName(char* tName) {
	strcpy(gData.mGameName, tName);
}

static void setToProgramDirectory() {
	TCHAR wbuf[1024];
	char buf[1024];
	GetModuleFileName(NULL, wbuf, 1024);

	int len = wcstombs(buf, wbuf, 1024);
	buf[len] = '\0';
	char* end = strrchr(buf, '\\');
	end[1] = '\0';

	_chdir(buf);
}

void initSystem() {

	setToProgramDirectory();
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	

	if (gData.mGameName[0] == '\0') {
		sprintf(gData.mGameName, "Unnamed libtari game port");
	}
	gSDLWindow = SDL_CreateWindow(gData.mGameName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, getScreenSize().x, getScreenSize().y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
}

void shutdownSystem() {
	SDL_DestroyWindow(gSDLWindow);

	IMG_Quit();
	SDL_Quit();
}

extern SDL_Renderer* gRenderer;

static void resizeWindow(SDL_Event* e) {
	if (gRenderer == NULL) return;

	ScreenSize sz = getScreenSize();

	double scaleX = e->window.data1 /(double)sz.x;
	double scaleY = e->window.data2 / (double)sz.y;

	SDL_RenderSetScale(gRenderer, (float)scaleX, (float)scaleY);
}
static void checkWindowEvents(SDL_Event* e) {
	if (e->window.event == SDL_WINDOWEVENT_RESIZED) {
		resizeWindow(e);
	}
	
}

static void switchFullscreen() {
	if (!gData.mIsFullscreen) {
		SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN);
	}
	else {
		SDL_SetWindowFullscreen(gSDLWindow, 0);
	}

	gData.mIsFullscreen ^= 1;
}

static void checkFullscreen() {
	const Uint8* kstates = SDL_GetKeyboardState(NULL);

	if (kstates[SDL_SCANCODE_RETURN] && kstates[SDL_SCANCODE_LALT]) {
		switchFullscreen();
	}
}

void updateSystem() {
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) { 
		if( e.type == SDL_QUIT ) 
		{  
			abortSystem();
		}
		else if (e.type == SDL_WINDOWEVENT) {
			checkWindowEvents(&e);
		}
	}

	checkFullscreen();
	
}

void setScreen(int tX, int tY, int tFramerate, int tIsVGA) {
	(void)tIsVGA;
	(void)tFramerate;
	if(!gData.mIsLoaded) initScreenDefault();
	gData.mScreenSizeX = tX;
	gData.mScreenSizeY = tY;
}

void setScreenSize(int tX, int tY) {
	if(!gData.mIsLoaded) initScreenDefault();

	gData.mScreenSizeX = tX;
	gData.mScreenSizeY = tY;
}

ScreenSize getScreenSize() {
	if(!gData.mIsLoaded) initScreenDefault();
	ScreenSize ret;
	ret.x = gData.mScreenSizeX;
	ret.y = gData.mScreenSizeY;
	return ret;
}

void setScreenFramerate(int tFramerate) {
	(void)tFramerate;
}

void setVGA() {
	
}

void returnToMenu() {
	exit(0);
}