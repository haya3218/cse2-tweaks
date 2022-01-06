// THIS IS DECOMPILED PROPRIETARY CODE - USE AT YOUR OWN RISK.
//
// The original code belongs to Daisuke "Pixel" Amaya.
//
// Modifications and custom code are under the MIT licence.
// See LICENCE.txt for details.

#pragma once

#include <string>

#include "WindowsWrapper.h"

#include "Config.h"
#include "Input.h"

extern std::string gModulePath;
extern std::string gDataPath;
extern std::string gSpritePath;

extern BOOL bFullscreen;
extern BOOL gbUseJoystick;

extern int gJoystickButtonTable[8];

extern CONFIG_BINDING bindings[BINDING_TOTAL];

void PutFramePerSecound(void);

BOOL SystemTask(void);
