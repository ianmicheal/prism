#include "prism/input.h"

#include <stdint.h>
#include <stdarg.h>

#include <SDL.h>

#include "prism/log.h"
#include "prism/math.h"
#include "prism/clipboardhandler.h"

typedef struct {
	int mIsUsingController;
	SDL_GameController* mController;
	SDL_Haptic* mHaptic;

	int mIsRumbling;
	SDL_HapticEffect mHapticEffect;
	int mHapticEffectID;
	Duration mRumbleNow;
	Duration mRumbleDuration;
} Controller;

static struct {
	const Uint8* mKeyStatePointer;
	Uint8* mCurrentKeyStates;
	Uint8* mPreviousKeyStates;
	Controller mControllers[MAXIMUM_CONTROLLER_AMOUNT];
} gData;



static int evaluateSDLButtonA(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	return SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_A);
}

static int evaluateSDLButtonB(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	return SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_B);
}

static int evaluateSDLButtonX(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	return SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_X);
}

static int evaluateSDLButtonY(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	return SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_Y);
}

static int evaluateSDLButtonL(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;	
	double axis = SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0;
	return (axis > 0.5);
}

static int evaluateSDLButtonR(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	double axis = SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0;
	return (axis > 0.5);
}

static int evaluateSDLButtonLeft(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	double axis = SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0;
	int ret = (axis < -0.5);
	ret |= SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
	return ret;
}

static int evaluateSDLButtonRight(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	double axis = SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0;
	int ret = (axis > 0.5);
	ret |= SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
	return ret;
}

static int evaluateSDLButtonUp(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	double axis = SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0;
	int ret = (axis < -0.5);
	ret |= SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_DPAD_UP);
	return ret;
}

static int evaluateSDLButtonDown(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	double axis = SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0;
	int ret = (axis > 0.5);
	ret |= SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	return ret;
}

static int evaluateSDLButtonStart(int i) {
	if (!gData.mControllers[i].mIsUsingController) return 0;
	return SDL_GameControllerGetButton(gData.mControllers[i].mController, SDL_CONTROLLER_BUTTON_START);
}

typedef int(*InputEvaluationFunction)(int);

static InputEvaluationFunction gSDLButtonMapping[] = {
	evaluateSDLButtonA,
	evaluateSDLButtonB,
	evaluateSDLButtonX,
	evaluateSDLButtonY,
	evaluateSDLButtonL,
	evaluateSDLButtonR,
	evaluateSDLButtonLeft,
	evaluateSDLButtonRight,
	evaluateSDLButtonUp,
	evaluateSDLButtonDown,
	evaluateSDLButtonStart,
};

static ControllerButtonPrism gButtonMapping[MAXIMUM_CONTROLLER_AMOUNT][CONTROLLER_BUTTON_AMOUNT_PRISM] = {
	{
		CONTROLLER_A_PRISM,
		CONTROLLER_B_PRISM,
		CONTROLLER_X_PRISM,
		CONTROLLER_Y_PRISM,
		CONTROLLER_L_PRISM,
		CONTROLLER_R_PRISM,
		CONTROLLER_LEFT_PRISM,
		CONTROLLER_RIGHT_PRISM,
		CONTROLLER_UP_PRISM,
		CONTROLLER_DOWN_PRISM,
		CONTROLLER_START_PRISM,
	},
	{
		CONTROLLER_A_PRISM,
		CONTROLLER_B_PRISM,
		CONTROLLER_X_PRISM,
		CONTROLLER_Y_PRISM,
		CONTROLLER_L_PRISM,
		CONTROLLER_R_PRISM,
		CONTROLLER_LEFT_PRISM,
		CONTROLLER_RIGHT_PRISM,
		CONTROLLER_UP_PRISM,
		CONTROLLER_DOWN_PRISM,
		CONTROLLER_START_PRISM,
	},
};

static SDL_Scancode const gPrismToSDLKeyboardMapping[] = {
	SDL_SCANCODE_A,
	SDL_SCANCODE_B,
	SDL_SCANCODE_C,
	SDL_SCANCODE_D,
	SDL_SCANCODE_E,
	SDL_SCANCODE_F,
	SDL_SCANCODE_G,
	SDL_SCANCODE_H,
	SDL_SCANCODE_I,
	SDL_SCANCODE_J,
	SDL_SCANCODE_K,
	SDL_SCANCODE_L,
	SDL_SCANCODE_M ,
	SDL_SCANCODE_N,
	SDL_SCANCODE_O,
	SDL_SCANCODE_P,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_R,
	SDL_SCANCODE_S,
	SDL_SCANCODE_T,
	SDL_SCANCODE_U,
	SDL_SCANCODE_V,
	SDL_SCANCODE_W,
	SDL_SCANCODE_X,
	SDL_SCANCODE_Y,
	SDL_SCANCODE_Z,
	SDL_SCANCODE_0,
	SDL_SCANCODE_1,
	SDL_SCANCODE_2,
	SDL_SCANCODE_3,
	SDL_SCANCODE_4,
	SDL_SCANCODE_5,
	SDL_SCANCODE_6,
	SDL_SCANCODE_7,
	SDL_SCANCODE_8,
	SDL_SCANCODE_9,
	SDL_SCANCODE_SPACE,
	SDL_SCANCODE_LEFT,
	SDL_SCANCODE_RIGHT,
	SDL_SCANCODE_UP,
	SDL_SCANCODE_DOWN,
	SDL_SCANCODE_F1,
	SDL_SCANCODE_F2,
	SDL_SCANCODE_F3,
	SDL_SCANCODE_F4,
	SDL_SCANCODE_F5,
	SDL_SCANCODE_F6,
	SDL_SCANCODE_SCROLLLOCK,
	SDL_SCANCODE_PAUSE,
	SDL_SCANCODE_LCTRL,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_RETURN,
	SDL_NUM_SCANCODES,
};

static KeyboardKeyPrism gKeys[MAXIMUM_CONTROLLER_AMOUNT][KEYBOARD_AMOUNT_PRISM] = {
	{
		KEYBOARD_A_PRISM,
		KEYBOARD_S_PRISM,
		KEYBOARD_Q_PRISM,
		KEYBOARD_W_PRISM,
		KEYBOARD_E_PRISM,
		KEYBOARD_D_PRISM,
		KEYBOARD_LEFT_PRISM,
		KEYBOARD_RIGHT_PRISM,
		KEYBOARD_UP_PRISM,
		KEYBOARD_DOWN_PRISM,
		KEYBOARD_RETURN_PRISM,
	},
	{
		KEYBOARD_H_PRISM,
		KEYBOARD_J_PRISM,
		KEYBOARD_Y_PRISM,
		KEYBOARD_U_PRISM,
		KEYBOARD_I_PRISM,
		KEYBOARD_K_PRISM,
		KEYBOARD_4_PRISM,
		KEYBOARD_6_PRISM,
		KEYBOARD_8_PRISM,
		KEYBOARD_2_PRISM,
		KEYBOARD_3_PRISM,
	},
};

void initInput() {
	gData.mPreviousKeyStates = (Uint8*)allocClearedMemory(SDL_NUM_SCANCODES, 1);
	gData.mCurrentKeyStates = (Uint8*)allocClearedMemory(SDL_NUM_SCANCODES, 1);
	gData.mKeyStatePointer = SDL_GetKeyboardState(NULL);
}

static void loadController(int i) {
	if (gData.mControllers[i].mIsUsingController) return;

	gData.mControllers[i].mController = SDL_GameControllerOpen(i);
	gData.mControllers[i].mHaptic = SDL_HapticOpenFromJoystick(SDL_GameControllerGetJoystick(gData.mControllers[i].mController));
	gData.mControllers[i].mIsRumbling = 0;
	gData.mControllers[i].mIsUsingController = 1;
}

static void unloadController(int i) {
	if (!gData.mControllers[i].mIsUsingController) return;
	turnControllerRumbleOffSingle(i);
	if(gData.mControllers[i].mHaptic) SDL_HapticClose(gData.mControllers[i].mHaptic);
	SDL_GameControllerClose(gData.mControllers[i].mController);
	gData.mControllers[i].mController = NULL;
	gData.mControllers[i].mIsUsingController = 0;
}

static void updateSingleControllerInput(int i) {
	if (i >= SDL_NumJoysticks()) {
		unloadController(i);
		return;
	}

	loadController(i);
}

static void updateSingleControllerRumble(int i) {
	if (!isUsingControllerSingle(i)) return;
	if (!gData.mControllers[i].mIsRumbling) return;

	if (handleDurationAndCheckIfOver(&gData.mControllers[i].mRumbleNow, gData.mControllers[i].mRumbleDuration)) {
		turnControllerRumbleOffSingle(i);
	}
}

static void updateControllers() {
	int i;
	for (i = 0; i < MAXIMUM_CONTROLLER_AMOUNT; i++) {
		updateSingleControllerInput(i);
		updateSingleControllerRumble(i);
	}
}

void updateInputPlatform() {
	memcpy(gData.mPreviousKeyStates, gData.mCurrentKeyStates, SDL_NUM_SCANCODES);
	memcpy(gData.mCurrentKeyStates, gData.mKeyStatePointer, SDL_NUM_SCANCODES);
	updateControllers();
}

int hasPressedASingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_A_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_A_PRISM]](i);
	return state;
}

int hasPressedBSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_B_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_B_PRISM]](i);
	return state;
}

int hasPressedXSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_X_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_X_PRISM]](i);
	return state;
}

int hasPressedYSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_Y_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_Y_PRISM]](i);
	return state;
}

int hasPressedLeftSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_LEFT_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_LEFT_PRISM]](i);
	return state;
}

int hasPressedRightSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_RIGHT_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_RIGHT_PRISM]](i);
	return state;
}

int hasPressedUpSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_UP_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_UP_PRISM]](i);
	return state;
}

int hasPressedDownSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_DOWN_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_DOWN_PRISM]](i);
	return state;
}

int hasPressedLSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_L_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_L_PRISM]](i);
	return state;
}

int hasPressedRSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_R_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_R_PRISM]](i);
	return state;
}

int hasPressedStartSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[gPrismToSDLKeyboardMapping[gKeys[i][CONTROLLER_START_PRISM]]];
	state |= gSDLButtonMapping[gButtonMapping[i][CONTROLLER_START_PRISM]](i);
	return state;
}

int hasPressedAbortSingle(int i) {
	if (gData.mCurrentKeyStates == NULL) return 0;
	int state = gData.mCurrentKeyStates[SDL_SCANCODE_ESCAPE];
	if (gData.mControllers[i].mIsUsingController) {
		state |= (hasPressedASingle(i) && hasPressedBSingle(i) && hasPressedXSingle(i) && hasPressedYSingle(i) && hasPressedStartSingle(i));
	}
	return state;
}

int hasShotGunSingle(int i)
{
	(void)i;
	uint32_t mask = SDL_GetMouseState(NULL, NULL);
	return mask & SDL_BUTTON(SDL_BUTTON_LEFT); // TODO: fix multiplayer
}

extern Vector3D correctSDLWindowPosition(Vector3D v);

Vector3D getShotPositionSingle(int i) {
	(void)i; // TODO: fix multiplayer

	int x, y;
	SDL_GetMouseState(&x, &y);
	Vector3D ret = makePosition(x, y, 0);
	return correctSDLWindowPosition(ret);
}


static double getStickNormalizedBinary(int tCodeMinus, int tCodePlus) {
	if (gData.mCurrentKeyStates == NULL) return 0;

	if (gData.mCurrentKeyStates[tCodeMinus]) return -1;
	else if (gData.mCurrentKeyStates[tCodePlus]) return 1;
	else return 0;
}

double getSingleLeftStickNormalizedX(int i) {
	if (gData.mControllers[i].mIsUsingController) {
		return SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0;
	}
	else return getStickNormalizedBinary(gKeys[i][CONTROLLER_LEFT_PRISM], gKeys[i][CONTROLLER_RIGHT_PRISM]);
}

double getSingleLeftStickNormalizedY(int i) {
	if (gData.mControllers[i].mIsUsingController) {
		return SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0;
	}
	else return getStickNormalizedBinary(gKeys[i][CONTROLLER_UP_PRISM], gKeys[i][CONTROLLER_DOWN_PRISM]);
}

double getSingleLNormalized(int i) {
	if (gData.mControllers[i].mIsUsingController) {
		return SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0;
	}
	else return gData.mCurrentKeyStates[gKeys[i][CONTROLLER_L_PRISM]];
}

double getSingleRNormalized(int i) {
	if (gData.mControllers[i].mIsUsingController) {
		return SDL_GameControllerGetAxis(gData.mControllers[i].mController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0;
	}
	else return gData.mCurrentKeyStates[gKeys[i][CONTROLLER_R_PRISM]];
}

extern SDL_Window* gSDLWindow;

void forceMouseCursorToWindow() {
#ifdef __EMSCRIPTEN__
	return;
#endif
	SDL_SetWindowGrab(gSDLWindow, SDL_TRUE);
}

void releaseMouseCursorFromWindow() {
#ifdef __EMSCRIPTEN__
	return;
#endif
	SDL_SetWindowGrab(gSDLWindow, SDL_FALSE);
}

int isUsingControllerSingle(int i) {
	return gData.mControllers[i].mIsUsingController;
}

void addControllerRumbleSingle(int i, Duration tDuration, int tFrequency, double tAmplitude) {
	if (!isUsingControllerSingle(i)) return;
	if (!gData.mControllers[i].mHaptic) return;

	turnControllerRumbleOffSingle(i);

	SDL_HapticEffect* effect = &gData.mControllers[i].mHapticEffect;
	memset(effect, 0, sizeof(SDL_HapticEffect));

	// TODO: fix
	if ((SDL_HapticQuery(gData.mControllers[i].mHaptic) & SDL_HAPTIC_SINE)) {
		effect->type = SDL_HAPTIC_SINE;
	}
	else {
		effect->type = SDL_HAPTIC_LEFTRIGHT;
	}	
	effect->periodic.direction.type = SDL_HAPTIC_POLAR; 
	effect->periodic.direction.dir[0] = 18000; 
	effect->periodic.period = (uint16_t)tFrequency; 
	
	effect->periodic.magnitude = (int16_t)(INT16_MAX * fclamp(tAmplitude, 0, 1)); 
	effect->periodic.length = 5000;
	effect->periodic.attack_length = 1000;
	effect->periodic.fade_length = 1000;

	gData.mControllers[i].mHapticEffectID = SDL_HapticNewEffect(gData.mControllers[i].mHaptic, effect);

	SDL_HapticRunEffect(gData.mControllers[i].mHaptic, gData.mControllers[i].mHapticEffectID, 1);

	gData.mControllers[i].mRumbleNow = 0;
	gData.mControllers[i].mRumbleDuration = tDuration;
	gData.mControllers[i].mIsRumbling = 1;
}

void turnControllerRumbleOffSingle(int i) {
	if (!gData.mControllers[i].mIsRumbling) return;

	SDL_HapticDestroyEffect(gData.mControllers[i].mHaptic, gData.mControllers[i].mHapticEffectID);
	gData.mControllers[i].mIsRumbling = 0;
}

int hasPressedRawButton(int i, ControllerButtonPrism tButton) {
	return gSDLButtonMapping[tButton](i);
}

int hasPressedRawKeyboardKey(KeyboardKeyPrism tKey) {
	int id = gPrismToSDLKeyboardMapping[tKey];
	return gData.mCurrentKeyStates[id];
}

int hasPressedKeyboardKeyFlank(KeyboardKeyPrism tKey) {
	int id = gPrismToSDLKeyboardMapping[tKey];
	return !gData.mPreviousKeyStates[id] && gData.mCurrentKeyStates[id];
}

int hasPressedKeyboardMultipleKeyFlank(int tKeyAmount, ...) {
	if (!tKeyAmount) return 0;

	int i;
	va_list vl;
	va_start(vl, tKeyAmount);

	int previousKeyPressed = 1, currentKeyPressed = 1;
	for (i = 0; i < tKeyAmount; i++)
	{
		KeyboardKeyPrism singleKey = va_arg(vl, KeyboardKeyPrism);
		int id = gPrismToSDLKeyboardMapping[singleKey];
		previousKeyPressed = previousKeyPressed && gData.mPreviousKeyStates[id];
		currentKeyPressed = currentKeyPressed && gData.mCurrentKeyStates[id];
	}
	va_end(vl);

	return !previousKeyPressed && currentKeyPressed;
}

ControllerButtonPrism getButtonForController(int i, ControllerButtonPrism tTargetButton)
{
	return gButtonMapping[i][tTargetButton];
}

void setButtonForController(int i, ControllerButtonPrism tTargetButton, ControllerButtonPrism tButtonValue)
{
	gButtonMapping[i][tTargetButton] = tButtonValue;
}

KeyboardKeyPrism getButtonForKeyboard(int i, ControllerButtonPrism tTargetButton)
{
	return gKeys[i][tTargetButton];
}

void setButtonForKeyboard(int i, ControllerButtonPrism tTargetButton, KeyboardKeyPrism tKeyValue)
{
	gKeys[i][tTargetButton] = tKeyValue;
}