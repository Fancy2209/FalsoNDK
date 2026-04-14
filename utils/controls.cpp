/*
 * utils/controls.cpp
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "controls.h"

#include <falso_jni/FalsoJNI.h>
#include <pthread.h>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../keycodes.h"
#include "../AInput.h"

extern "C" {
    float L_INNER_DEADZONE __attribute__((weak)) = 0.20f;
    float R_INNER_DEADZONE __attribute__((weak)) = 0.20f;

    int AInput_enableLeftStick __attribute__((weak)) = 1;
    int AInput_enableRightStick __attribute__((weak)) = 1;
}

#define L_OUTER_DEADZONE 0.99f
#define R_OUTER_DEADZONE 0.99f

#define TOUCHPAD_X_RADIUS 98
#define TOUCHPAD_Y_RADIUS 98

#define TOUCHPAD_LX_BASE 104
#define TOUCHPAD_LY_BASE 438
#define TOUCHPAD_RX_BASE 852
#define TOUCHPAD_RY_BASE 438

#define LSTICK_PTR_ID 88
#define RSTICK_PTR_ID 89


AInputQueue * inputQueue;

float lerp(float x1, float y1, float x3, float y3, float x2) {
    return ((x2-x1)*(y3-y1) / (x3-x1)) + y1;
}

float coord_normalize(float val, float deadzone_min, float deadzone_max) {
    float sign = (val < 0) ? -1.0f : 1.0f;

    if (fabsf(val) < deadzone_min) return 0.f;
    if (fabsf(val) > deadzone_max) return 1.0f*sign;
    return lerp(0.f, deadzone_min * sign, 1.0f*sign, deadzone_max*sign, val);
}

void controls_init(AInputQueue * queue) {
    // Enable analog sticks and touchscreen
    inputQueue = queue;

    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32*1024);
    pthread_create(&t, &attr, controls_poll, nullptr);
    pthread_detach(t);
}

void * controls_poll(void * arg) {
    while (1) {
        pollPad();
        SDL_Sleep(16666);
    }
}

static ButtonMapping mapping[] = {
        { SCE_CTRL_UP,        AKEYCODE_DPAD_UP },
        { SCE_CTRL_DOWN,      AKEYCODE_DPAD_DOWN },
        { SCE_CTRL_LEFT,      AKEYCODE_DPAD_LEFT },
        { SCE_CTRL_RIGHT,     AKEYCODE_DPAD_RIGHT },
        { SCE_CTRL_CROSS,     AKEYCODE_BUTTON_A },
        { SCE_CTRL_CIRCLE,    AKEYCODE_BUTTON_B },
        { SCE_CTRL_SQUARE,    AKEYCODE_BUTTON_X },
        { SCE_CTRL_TRIANGLE,  AKEYCODE_BUTTON_Y },
        { SCE_CTRL_L1,        AKEYCODE_BUTTON_L1 },
        { SCE_CTRL_R1,        AKEYCODE_BUTTON_R1 },
        { SCE_CTRL_START,     AKEYCODE_BUTTON_START },
        { SCE_CTRL_SELECT,    AKEYCODE_BUTTON_SELECT },
};

uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;
float lx = 0, ly = 0, rx = 0, ry = 0, lastLx = 0, lastLy = 0, lastRx = 0, lastRy = 0;

inputEvent stickInputEvent;
int sticksDown = 0;
float x_old = 0.0f, y_old = 0.0f, z_old = 0.0f, rz_old = 0.0f, hat_x_old = 0.0f, hat_y_old = 0.0f;
bool ltPressed_old = false, rtPressed_old = false;

void sendJoyEvent(float x, float y, float z, float rz, float hat_x, float hat_y, bool ltPressed, bool rtPressed) {
    if (x != x_old || y != y_old || z != z_old || rz != rz_old || hat_x != hat_x_old || hat_y != hat_y_old || ltPressed != ltPressed_old || rtPressed != rtPressed_old) {
        stickInputEvent.source = AINPUT_SOURCE_JOYSTICK;
        stickInputEvent.motion_ptrcount = sticksDown + 1;
        stickInputEvent.motion_x[0] = x;
        stickInputEvent.motion_y[0] = y;
        stickInputEvent.motion_z[0] = z;
        stickInputEvent.motion_rz[0] = rz;
        stickInputEvent.motion_hat_x[0] = hat_x;
        stickInputEvent.motion_hat_y[0] = hat_y;
        stickInputEvent.motion_lt[0] = ltPressed ? 1.0 : 0.0;
        stickInputEvent.motion_rt[0] = rtPressed ? 1.0 : 0.0;
        stickInputEvent.motion_ptridx[0] = 0;
        stickInputEvent.type = AINPUT_EVENT_TYPE_MOTION;

        stickInputEvent.motion_action = AMOTION_EVENT_ACTION_MOVE;
        AInputEvent* aie = AInputEvent_create(&stickInputEvent);
        AInputQueue_enqueueEvent(inputQueue, aie);

        x_old = x;
        y_old = y;
        z_old = z;
        rz_old = rz;
        hat_x_old = hat_x;
        hat_y_old = hat_y;
        ltPressed_old = ltPressed;
        rtPressed_old = rtPressed;
    }
}

static SDL_GameController* controller = nullptr;
#define GET_SDL_BUTTON_STATE(button) (controller != nullptr && SDL_GameControllerGetButton(controller, b))
void pollPad() {
    if(!controller && SDL_NumJoysticks() > 0) controller = SDL_GameControllerOpen(0);
    const bool *key_states = SDL_GetKeyboardState();

    old_buttons = current_buttons;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_A) || key_states[SDL_SCANCODE_S])
        current_buttons |= SCE_CTRL_CROSS;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_B) || key_states[SDL_SCANCODE_B])
        current_buttons |= SCE_CTRL_CIRCLE;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_X) || key_states[SDL_SCANCODE_A])
        current_buttons |= SCE_CTRL_SQUARE;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_Y) || key_states[SDL_SCANCODE_W])
        current_buttons |= SCE_CTRL_TRIANGLE;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_DPAD_UP)) || key_states[SDL_SCANCODE_UP]
        current_buttons |= SCE_CTRL_UP;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_DPAD_DOWN) || key_states[SDL_SCANCODE_DOWN])
        current_buttons |= SCE_CTRL_DOWN;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_DPAD_LEFT) || key_states[SDL_SCANCODE_LEFT])
        current_buttons |= SCE_CTRL_LEFT;

    if (GET_SDL_BUTTON_STATE(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || key_states[SDL_SCANCODE_RIGHT])
        current_buttons |= SCE_CTRL_RIGHT;

    pressed_buttons = current_buttons & ~old_buttons;
    released_buttons = ~current_buttons & old_buttons;

    for (auto & i : mapping) {
        if (pressed_buttons & i.sce_button) {
            inputEvent e;
            e.source = AINPUT_SOURCE_KEYBOARD; // Warning: some games may want distinction between AINPUT_SOURCE_KEYBOARD and AINPUT_SOURCE_DPAD
            e.keycode = i.android_button;
            e.action = AKEY_EVENT_ACTION_DOWN;
            e.type = AINPUT_EVENT_TYPE_KEY;

            AInputEvent* aie = AInputEvent_create(&e);
            AInputQueue_enqueueEvent(inputQueue, aie);
        } else if (released_buttons & i.sce_button) {
            inputEvent e;
            e.source = AINPUT_SOURCE_KEYBOARD; // Warning: some games may want distinction between AINPUT_SOURCE_KEYBOARD and AINPUT_SOURCE_DPAD
            e.keycode = i.android_button;
            e.action = AKEY_EVENT_ACTION_UP;
            e.type = AINPUT_EVENT_TYPE_KEY;

            AInputEvent *aie = AInputEvent_create(&e);
            AInputQueue_enqueueEvent(inputQueue, aie);
        }
    }

    lastLx = lx;
    lastLy = ly;
    lastRx = rx;
    lastRy = ry;

    lx = coord_normalize(
        ((float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)) / 32767.0f, 
        L_INNER_DEADZONE, L_OUTER_DEADZONE
    );
    ly = coord_normalize(
        ((float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)) / 32767.0f, 
        L_INNER_DEADZONE, L_OUTER_DEADZONE
    );
    rx = coord_normalize(
        ((float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX)) / 32767.0f, 
        R_INNER_DEADZONE, R_OUTER_DEADZONE
    );
    ry = coord_normalize(
        ((float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY)) / 32767.0f, 
        R_INNER_DEADZONE, R_OUTER_DEADZONE
    );

    stickInputEvent.motion_action = AMOTION_EVENT_ACTION_MOVE;
    stickInputEvent.type = AINPUT_EVENT_TYPE_MOTION;

    sendJoyEvent(lx,
                 ly,
                 rx,
                 ry,
                 0,
                 0,
                 current_buttons & SCE_CTRL_L1,
                 current_buttons & SCE_CTRL_R1);
}
