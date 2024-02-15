#pragma once

#include "../include/global.h"

#define DEADZONE 10.0f

enum InputButtons {
    INPUT_CRIGHT,
    INPUT_CLEFT,
    INPUT_CDOWN,
    INPUT_CUP,
    INPUT_R,
    INPUT_L,
    INPUT_X,
    INPUT_Y,
    INPUT_DRIGHT,
    INPUT_DLEFT,
    INPUT_DDOWN,
    INPUT_DUP,
    INPUT_START,
    INPUT_Z,
    INPUT_B,
    INPUT_A,

    INPUT_TOTAL,

};

enum InputType {
    INPUT_PRESSED,
    INPUT_RELEASED,
    INPUT_HELD
};

enum ControllerType {
    CONTROLLER_N64,
    CONTROLLER_GAMECUBE
};

enum StickType {
    STICK_LEFT,
    STICK_RIGHT
};

typedef struct Input {
    float stickMag[2];
    unsigned char button[3][INPUT_TOTAL];
    char stickX[2];
    char stickY[2];
    char pak;
    char type;
    short stickAngle[2];
} Input;

extern int gCurrentController;
extern struct controller_data gController;
extern Input gInputData;

void input_update(int updateRate);
int input_pressed(int input, int numFrames);
int input_held(int input);
int input_released(int input, int numFrames);
int input_stick_x(int type);
int input_stick_y(int type);
short input_stick_angle(int type);
float input_stick_mag(int type);
void input_rumble(int timer);
void input_clear(int input);
int input_type(void);