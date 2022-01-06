// THIS IS DECOMPILED PROPRIETARY CODE - USE AT YOUR OWN RISK.
//
// The original code belongs to Daisuke "Pixel" Amaya.
//
// Modifications and custom code are under the MIT licence.
// See LICENCE.txt for details.

#include "Main.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "WindowsWrapper.h"

#include "Backends/Misc.h"
#include "Bitmap.h"
#include "CommonDefines.h"
#include "Config.h"
#include "Draw.h"
#ifdef EXTRA_SOUND_FORMATS
#include "ExtraSoundFormats.h"
#endif
#include "Game.h"
#include "Generic.h"
#include "Input.h"
#include "KeyControl.h"
#include "MyChar.h"
#include "Organya.h"
#include "Profile.h"
#include "Resource.h"
#include "Stage.h"
#include "Sound.h"
#include "Triangle.h"

void InactiveWindow(void);
void ActiveWindow(void);

std::string gModulePath;
std::string gDataPath;
std::string gSpritePath;

BOOL bFullscreen;
BOOL gbUseJoystick;

CONFIG_BINDING bindings[BINDING_TOTAL];

static BOOL bActive = TRUE;
static BOOL bFPS = FALSE;

#ifdef JAPANESE
static const char* const lpWindowName = "洞窟物語";	// "Cave Story"
#else
static const char* const lpWindowName = "Cave Story ~ Doukutsu Monogatari";
#endif

static void DragAndDropCallback(const char *path)
{
	LoadProfile(path);
}

static void WindowFocusCallback(bool focus)
{
	if (focus)
		ActiveWindow();
	else
		InactiveWindow();
}

// Framerate stuff
static unsigned long CountFramePerSecound(void)
{
	unsigned long current_tick;	// The original name for this variable is unknown
	static BOOL first = TRUE;
	static unsigned long max_count;
	static unsigned long count;
	static unsigned long wait;

	if (first)
	{
		wait = Backend_GetTicks();
		first = FALSE;
	}

	current_tick = Backend_GetTicks();
	++count;

	if (wait + 1000 <= current_tick)
	{
		wait += 1000;
		max_count = count;
		count = 0;
	}

	return max_count;
}

void PutFramePerSecound(void)
{
	if (bFPS)
	{
		const unsigned long fps = CountFramePerSecound();
		PutNumber4(WINDOW_WIDTH - 40, 8, fps, FALSE);
	}
}

// TODO - Inaccurate stack frame
int main(int argc, char *argv[])
{
	(void)argc;

	if (!Backend_Init(DragAndDropCallback, WindowFocusCallback))
		return EXIT_FAILURE;


	// Get executable's path, and path of the data fol|der
	if (!Backend_GetPaths(&gModulePath, &gDataPath))
	{
		// Fall back on argv[0] if the backend cannot provide a path
		gModulePath = argv[0];

		for (size_t i = gModulePath.length();; --i)
		{
			if (i == 0 || gModulePath[i] == '\\' || gModulePath[i] == '/')
			{
				gModulePath.resize(i);
				break;
			}
		}

		gDataPath = gModulePath + "/data";
	}

	CONFIGDATA conf;
	if (!LoadConfigData(&conf))
		DefaultConfigData(&conf);

	gSpriteScale = 1 + conf.graphics_set;

	gSpritePath = gDataPath + (gSpriteScale == 2? "/sprites_up": "/sprites_og");

	gSoundtrack = conf.soundtrack;
	gDimmingFactor = (4.0 - conf.backgroundBrightness)/4;

	memcpy(bindings, conf.bindings, sizeof(bindings));

	RECT unused_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

	switch (conf.display_mode)
	{
		default:
			// Windowed

		#ifdef FIX_MAJOR_BUGS
			if (!StartDirectDraw(lpWindowName, conf.display_mode, conf.b60fps, conf.bSmoothScrolling, conf.bVsync))
			{
				Backend_Deinit();
				return EXIT_FAILURE;
			}
		#else
			// Doesn't handle StartDirectDraw failing
			StartDirectDraw(lpWindowName, conf.display_mode, conf.b60fps, conf.bSmoothScrolling, conf.bVsync);
		#endif

			break;

		case 0:
			// Fullscreen

		#ifdef FIX_MAJOR_BUGS
			if (!StartDirectDraw(lpWindowName, 0, conf.b60fps, conf.bSmoothScrolling, conf.bVsync))
			{
				Backend_Deinit();
				return EXIT_FAILURE;
			}
		#else
			// Doesn't handle StartDirectDraw failing
			StartDirectDraw(lpWindowName, 0, conf.b60fps, conf.bSmoothScrolling, conf.bVsync);
		#endif

			bFullscreen = TRUE;

			Backend_HideMouse();
			break;
	}

#ifdef DEBUG_SAVE
	Backend_EnableDragAndDrop();
#endif

	// Set up window icon
#ifndef _WIN32	// On Windows, we use native icons instead (so we can give the taskbar and window separate icons, like the original EXE does)
	size_t window_icon_resource_size;
	const unsigned char *window_icon_resource_data = FindResource("ICON_MINI", "ICON", &window_icon_resource_size);

	if (window_icon_resource_data != NULL)
	{
		size_t window_icon_width, window_icon_height;
		unsigned char *window_icon_rgb_pixels = DecodeBitmap(window_icon_resource_data, window_icon_resource_size, &window_icon_width, &window_icon_height, 3);

		if (window_icon_rgb_pixels != NULL)
		{
			Backend_SetWindowIcon(window_icon_rgb_pixels, window_icon_width, window_icon_height);
			FreeBitmap(window_icon_rgb_pixels);
		}
	}
#endif

	// Set up the cursor
	std::string cursor_path = gSpritePath + "/Resource/CURSOR/CURSOR_NORMAL.png";

	size_t cursor_width, cursor_height;
	unsigned char *cursor_rgba_pixels = DecodeBitmapFromFile(cursor_path.c_str(), &cursor_width, &cursor_height, 4);

	if (cursor_rgba_pixels != NULL)
	{
		Backend_SetCursor(cursor_rgba_pixels, cursor_width, cursor_height);
		FreeBitmap(cursor_rgba_pixels);
	}

	if (IsKeyFile("fps"))
		bFPS = TRUE;

	// Set rects
	RECT rcLoading = {0, 0, 64, 8};
	RECT rcFull = {0, 0, 0, 0};
	rcFull.right = WINDOW_WIDTH;
	rcFull.bottom = WINDOW_HEIGHT;

	// Load the "LOADING" text
	BOOL b = MakeSurface_File("Loading", SURFACE_ID_LOADING, false);

	// Draw loading screen
	CortBox(&rcFull, 0x000000);
	PutBitmap3(&rcFull, PixelToScreenCoord((WINDOW_WIDTH / 2) - 32), PixelToScreenCoord((WINDOW_HEIGHT / 2) - 4), &rcLoading, SURFACE_ID_LOADING);

	// Draw to screen
	if (!Flip_SystemTask())
	{
		Backend_Deinit();
		return EXIT_SUCCESS;
	}

	// Initialize sound
	InitDirectSound();

	// Initialize joystick
	InitDirectInput();

	// Initialize stuff
#ifdef JAPANESE
	const FONT_DATA f_data = {"Zpix", 10, 10, 18, 18};

	gbFontData = f_data;
#else
	const FONT_DATA f_data[] = {
		{"opendyslexic", 15, 15, 29, 30},
		{"monogram", 13, 13, 26, 26},
		{"lekton", 12, 12, 26, 26},
		{"courier", 12, 12, 26, 26},
	};

	gbFontData = f_data[conf.font_select];
#endif

	InitTextObject("hi");

	InitTriangleTable();

	// Run game code
	Game();

	// End stuff
	EndTextObject();
	EndDirectSound();
	EndDirectDraw();

	Backend_Deinit();

	return EXIT_SUCCESS;
}

void InactiveWindow(void)
{
	if (bActive)
	{
		bActive = FALSE;
		StopOrganyaMusic();
#ifdef EXTRA_SOUND_FORMATS
		ExtraSound_Stop();
#endif
		SleepNoise();
	}

	PlaySoundObject(7, SOUND_MODE_STOP);
}

void ActiveWindow(void)
{
	if (!bActive)
	{
		bActive = TRUE;
		StopOrganyaMusic();
		PlayOrganyaMusic();
#ifdef EXTRA_SOUND_FORMATS
		ExtraSound_Play();
#endif
		ResetNoise();
	}

	PlaySoundObject(7, SOUND_MODE_PLAY_LOOP);
}

void JoystickProc(void);

BOOL SystemTask(void)
{
	static bool previous_keyboard_state[BACKEND_KEYBOARD_TOTAL];

	do
	{
		if (!Backend_SystemTask(bActive))
		{
			StopOrganyaMusic();
			return FALSE;
		}
	} while(!bActive);

	Backend_GetKeyboardState(gKeyboardState);

	for (unsigned int i = 0; i < BACKEND_KEYBOARD_TOTAL; ++i)
	{
		if (gKeyboardState[i] && !previous_keyboard_state[i])
		{
			if (i == BACKEND_KEYBOARD_ESCAPE)
				gKey |= KEY_ESCAPE;
			else if (i == BACKEND_KEYBOARD_F1)
				gKey |= KEY_F1;
			else if (i == BACKEND_KEYBOARD_F2)
				gKey |= KEY_F2;

			if (i == bindings[BINDING_MAP].keyboard)
				gKey |= KEY_MAP;
			if (i == bindings[BINDING_LEFT].keyboard)
				gKey |= KEY_LEFT;
			if (i == bindings[BINDING_RIGHT].keyboard)
				gKey |= KEY_RIGHT;
			if (i == bindings[BINDING_UP].keyboard)
				gKey |= KEY_UP;
			if (i == bindings[BINDING_DOWN].keyboard)
				gKey |= KEY_DOWN;
			if (i == bindings[BINDING_SHOT].keyboard)
				gKey |= KEY_SHOT;
			if (i == bindings[BINDING_JUMP].keyboard)
				gKey |= KEY_JUMP;
			if (i == bindings[BINDING_ARMS].keyboard)
				gKey |= KEY_ARMS;
			if (i == bindings[BINDING_ARMSREV].keyboard)
				gKey |= KEY_ARMSREV;
			if (i == bindings[BINDING_ITEM].keyboard)
				gKey |= KEY_ITEM;
			if (i == bindings[BINDING_CANCEL].keyboard)
				gKey |= KEY_CANCEL;
			if (i == bindings[BINDING_OK].keyboard)
				gKey |= KEY_OK;
			if (i == bindings[BINDING_PAUSE].keyboard)
				gKey |= KEY_PAUSE;
			if (i == bindings[BINDING_STRAFE].keyboard)
				gKey |= KEY_STRAFE;
		}
		else if (!gKeyboardState[i] && previous_keyboard_state[i])
		{
			if (i == BACKEND_KEYBOARD_ESCAPE)
				gKey &= ~KEY_ESCAPE;
			else if (i == BACKEND_KEYBOARD_F1)
				gKey &= ~KEY_F1;
			else if (i == BACKEND_KEYBOARD_F2)
				gKey &= ~KEY_F2;

			if (i == bindings[BINDING_MAP].keyboard)
				gKey &= ~KEY_MAP;
			if (i == bindings[BINDING_LEFT].keyboard)
				gKey &= ~KEY_LEFT;
			if (i == bindings[BINDING_RIGHT].keyboard)
				gKey &= ~KEY_RIGHT;
			if (i == bindings[BINDING_UP].keyboard)
				gKey &= ~KEY_UP;
			if (i == bindings[BINDING_DOWN].keyboard)
				gKey &= ~KEY_DOWN;
			if (i == bindings[BINDING_SHOT].keyboard)
				gKey &= ~KEY_SHOT;
			if (i == bindings[BINDING_JUMP].keyboard)
				gKey &= ~KEY_JUMP;
			if (i == bindings[BINDING_ARMS].keyboard)
				gKey &= ~KEY_ARMS;
			if (i == bindings[BINDING_ARMSREV].keyboard)
				gKey &= ~KEY_ARMSREV;
			if (i == bindings[BINDING_ITEM].keyboard)
				gKey &= ~KEY_ITEM;
			if (i == bindings[BINDING_CANCEL].keyboard)
				gKey &= ~KEY_CANCEL;
			if (i == bindings[BINDING_OK].keyboard)
				gKey &= ~KEY_OK;
			if (i == bindings[BINDING_PAUSE].keyboard)
				gKey &= ~KEY_PAUSE;
			if (i == bindings[BINDING_STRAFE].keyboard)
				gKey &= ~KEY_STRAFE;
		}
	}

	memcpy(previous_keyboard_state, gKeyboardState, sizeof(gKeyboardState));

	// Run joystick code
	if(gbUseJoystick){
		JoystickProc();
	}

	return TRUE;
}

void JoystickProc(void)
{
	int i;
	static DIRECTINPUTSTATUS old_status;

	if (!GetJoystickStatus(&gJoystickState))
		memset(&gJoystickState, 0, sizeof(gJoystickState));

	// Set held buttons
	for (i = 0; i < sizeof(gJoystickState.bButton) / sizeof(gJoystickState.bButton[0]); ++i)
	{
		if (gJoystickState.bButton[i] && !old_status.bButton[i])
		{
			if (i == bindings[BINDING_MAP].controller)
				gKey |= KEY_MAP;
			if (i == bindings[BINDING_LEFT].controller)
				gKey |= KEY_LEFT;
			if (i == bindings[BINDING_RIGHT].controller)
				gKey |= KEY_RIGHT;
			if (i == bindings[BINDING_UP].controller)
				gKey |= KEY_UP;
			if (i == bindings[BINDING_DOWN].controller)
				gKey |= KEY_DOWN;
			if (i == bindings[BINDING_SHOT].controller)
				gKey |= KEY_SHOT;
			if (i == bindings[BINDING_JUMP].controller)
				gKey |= KEY_JUMP;
			if (i == bindings[BINDING_ARMS].controller)
				gKey |= KEY_ARMS;
			if (i == bindings[BINDING_ARMSREV].controller)
				gKey |= KEY_ARMSREV;
			if (i == bindings[BINDING_ITEM].controller)
				gKey |= KEY_ITEM;
			if (i == bindings[BINDING_CANCEL].controller)
				gKey |= KEY_CANCEL;
			if (i == bindings[BINDING_OK].controller)
				gKey |= KEY_OK;
			if (i == bindings[BINDING_PAUSE].controller)
				gKey |= KEY_PAUSE;
			if (i == bindings[BINDING_STRAFE].controller)
				gKey |= KEY_STRAFE;
		}
		else if (!gJoystickState.bButton[i] && old_status.bButton[i])
		{
			if (i == bindings[BINDING_MAP].controller)
				gKey &= ~KEY_MAP;
			if (i == bindings[BINDING_LEFT].controller)
				gKey &= ~KEY_LEFT;
			if (i == bindings[BINDING_RIGHT].controller)
				gKey &= ~KEY_RIGHT;
			if (i == bindings[BINDING_UP].controller)
				gKey &= ~KEY_UP;
			if (i == bindings[BINDING_DOWN].controller)
				gKey &= ~KEY_DOWN;
			if (i == bindings[BINDING_SHOT].controller)
				gKey &= ~KEY_SHOT;
			if (i == bindings[BINDING_JUMP].controller)
				gKey &= ~KEY_JUMP;
			if (i == bindings[BINDING_ARMS].controller)
				gKey &= ~KEY_ARMS;
			if (i == bindings[BINDING_ARMSREV].controller)
				gKey &= ~KEY_ARMSREV;
			if (i == bindings[BINDING_ITEM].controller)
				gKey &= ~KEY_ITEM;
			if (i == bindings[BINDING_CANCEL].controller)
				gKey &= ~KEY_CANCEL;
			if (i == bindings[BINDING_OK].controller)
				gKey &= ~KEY_OK;
			if (i == bindings[BINDING_PAUSE].controller)
				gKey &= ~KEY_PAUSE;
			if (i == bindings[BINDING_STRAFE].controller)
				gKey &= ~KEY_STRAFE;
		}
	}

	old_status = gJoystickState;
}
