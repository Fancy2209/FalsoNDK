/*
 * reimpl/controls.h
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef AFAKENATIVE_CONTROLS_H
#define AFAKENATIVE_CONTROLS_H

#include <SDL2/SDL.h>

#include <math.h>
#include "AFakeNative/AInput.h"

#define GRAVITY_CONSTANT 9.807f

#define kIdRawPointerCancel 0xe
#define kIdRawPointerDown 0x6000e
#define kIdRawPointerMove 0x4000e
#define kIdRawPointerUp 0x8000e
#define kIdUndefined 0

#define kModuleTypeIdTouchScreen 1000
#define kModuleTypeIdTouchPad 1100

enum {
    ACTION_DOWN = 1,
    ACTION_UP   = 2,
    ACTION_MOVE = 3,
};

typedef struct {
    uint32_t sce_button;
    int32_t android_button;
} ButtonMapping;

/** Enumeration for the digital controller buttons.
 * @note - L1/R1/L3/R3 only can bind using ::sceCtrlPeekBufferPositiveExt2 and ::sceCtrlReadBufferPositiveExt2
 * @note - Values bigger than 0x00010000 can be intercepted only with shell privileges
 * @note - Vita's L Trigger and R Trigger are mapped to L1 and R1 when using ::sceCtrlPeekBufferPositiveExt2 and ::sceCtrlReadBufferPositiveExt2
 */
typedef enum SceCtrlButtons {
	SCE_CTRL_SELECT      = 0x00000001,            //!< Select button.
	SCE_CTRL_L3          = 0x00000002,            //!< L3 button.
	SCE_CTRL_R3          = 0x00000004,            //!< R3 button.
	SCE_CTRL_START       = 0x00000008,            //!< Start button.
	SCE_CTRL_UP          = 0x00000010,            //!< Up D-Pad button.
	SCE_CTRL_RIGHT       = 0x00000020,            //!< Right D-Pad button.
	SCE_CTRL_DOWN        = 0x00000040,            //!< Down D-Pad button.
	SCE_CTRL_LEFT        = 0x00000080,            //!< Left D-Pad button.
	SCE_CTRL_LTRIGGER    = 0x00000100,            //!< Left trigger.
	SCE_CTRL_L2          = SCE_CTRL_LTRIGGER,     //!< L2 button.
	SCE_CTRL_RTRIGGER    = 0x00000200,            //!< Right trigger.
	SCE_CTRL_R2          = SCE_CTRL_RTRIGGER,     //!< R2 button.
	SCE_CTRL_L1          = 0x00000400,            //!< L1 button.
	SCE_CTRL_R1          = 0x00000800,            //!< R1 button.
	SCE_CTRL_TRIANGLE    = 0x00001000,            //!< Triangle button.
	SCE_CTRL_CIRCLE      = 0x00002000,            //!< Circle button.
	SCE_CTRL_CROSS       = 0x00004000,            //!< Cross button.
	SCE_CTRL_SQUARE      = 0x00008000,            //!< Square button.
	SCE_CTRL_INTERCEPTED = 0x00010000,            //!< Input not available because intercepted by another application
	SCE_CTRL_PSBUTTON    = SCE_CTRL_INTERCEPTED,  //!< Playstation (Home) button.
	SCE_CTRL_HEADPHONE   = 0x00080000,            //!< Headphone plugged in.
	SCE_CTRL_VOLUP       = 0x00100000,            //!< Volume up button.
	SCE_CTRL_VOLDOWN     = 0x00200000,            //!< Volume down button.
	SCE_CTRL_POWER       = 0x40000000             //!< Power button.
} SceCtrlButtons;

void controls_init(AInputQueue * queue);
void * controls_poll(void * arg);
void pollTouch();
void pollPad();
void pollAccel();
void runSilentStartHelper();

#endif // AFAKENATIVE
