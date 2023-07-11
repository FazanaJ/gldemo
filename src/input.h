#pragma once

#define DEADZONE 10.0f

enum InputButtons {
    INPUT_A,
    INPUT_B,
    INPUT_START,
    INPUT_L,
    INPUT_R,
    INPUT_Z,
    INPUT_CUP,
    INPUT_CLEFT,
    INPUT_CRIGHT,
    INPUT_CDOWN,
    INPUT_DUP,
    INPUT_DLEFT,
    INPUT_DRIGHT,
    INPUT_DDOWN,

    INPUT_TOTAL,

    INPUT_X = INPUT_CUP,
    INPUT_Y,
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

void init_controller(void);
void update_inputs(int updateRate);
int get_input_pressed(int input, int numFrames);
int get_input_held(int input);
int get_input_released(int input, int numFrames);
int get_stick_x(int type);
int get_stick_y(int type);
short get_stick_angle(int type);
float get_stick_mag(int type);
void rumble_set(int timer);
void clear_input(int input);
int get_controller_type(void);