#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <raylib.h>
#include <stdbool.h>

#define RIGHT_SHOULDER GAMEPAD_BUTTON_RIGHT_TRIGGER_1
#define LEFT_SHOULDER  GAMEPAD_BUTTON_LEFT_TRIGGER_1
#define RIGHT_TRIGGER  GAMEPAD_BUTTON_RIGHT_TRIGGER_2
#define LEFT_TRIGGER   GAMEPAD_BUTTON_LEFT_TRIGGER_2
#define DPAD_UP        GAMEPAD_BUTTON_LEFT_FACE_UP
#define DPAD_DOWN      GAMEPAD_BUTTON_LEFT_FACE_DOWN
#define CROSS          GAMEPAD_BUTTON_RIGHT_FACE_DOWN


typedef struct {
    bool rightShoulder;
    bool leftShoulder;
    bool rightTrigger;
    bool leftTrigger;
    bool dpadUp;
    bool dpadDown;
    bool cross;

} GamepadState;

void zero_state(GamepadState *pad);
void update_pad_state(int gamepad, GamepadState *pad);
void dpad_scroll(GamepadState *pad, int *lap_count, int visible, int *scroll, int *active);

#endif
