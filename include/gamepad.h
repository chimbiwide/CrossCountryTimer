#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <raylib.h>
#include <stdbool.h>

#define RIGHT_SHOULDER GAMEPAD_BUTTON_RIGHT_TRIGGER_1
#define LEFT_SHOULDER GAMEPAD_BUTTON_LEFT_TRIGGER_1
#define RIGHT_TRIGGER GAMEPAD_BUTTON_RIGHT_TRIGGER_2
#define LEFT_TRIGGER GAMEPAD_BUTTON_LEFT_TRIGGER_2

typedef struct {
    bool rightShoulder;
    bool leftShoulder;
    bool rightTrigger;
    bool leftTrigger;
} GamepadState;

void zero_state(GamepadState *pad);
void update_pad_state(int gamepad, GamepadState *pad);

#endif
