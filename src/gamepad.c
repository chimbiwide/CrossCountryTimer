#include <gamepad.h>

void zero_state(GamepadState *pad) {
    pad->rightShoulder = false;
    pad->leftShoulder = false;
    pad->rightTrigger = false;
    pad->leftTrigger = false;
}


void update_pad_state(int gamepad, GamepadState *pad) {
    if (IsGamepadButtonPressed(gamepad, RIGHT_SHOULDER)) {
        pad->rightShoulder = true;
    }
    if (IsGamepadButtonPressed(gamepad, LEFT_SHOULDER)) {
        pad->leftShoulder = true;
    }
    if (IsGamepadButtonPressed(gamepad, RIGHT_TRIGGER)) {
        pad->rightTrigger = true;
    }
    if (IsGamepadButtonPressed(gamepad, LEFT_TRIGGER)) {
        pad->leftTrigger = true;
    }
}
