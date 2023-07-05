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

    INPUT_TOTAL
};

enum InputType {
    INPUT_PRESSED,
    INPUT_RELEASED,
    INPUT_HELD
};

typedef struct Input {
    float stickMag;
    unsigned char button[3][INPUT_TOTAL];
    char stickX;
    char stickY;
    char pak;
    short stickAngle;
} Input;

extern int gCurrentController;
extern struct controller_data gController;

void init_controller(void);
void update_inputs(int updateRate);
int get_input_pressed(int input, int numFrames);
int get_input_held(int input);
int get_input_released(int input, int numFrames);
int get_stick_x(void);
int get_stick_y(void);
short get_stick_angle(void);
float get_stick_mag(void);
void rumble_set(int timer);