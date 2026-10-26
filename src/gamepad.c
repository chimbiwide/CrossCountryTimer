#include <gamepad.h>
#include <stdbool.h>
#include <string.h>

void zero_state(GamepadState *pad) {
    memset(pad, 0, sizeof(GamepadState));
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
    if (IsGamepadButtonPressed(gamepad, DPAD_DOWN)) {
        pad->dpadDown = true;
    }
    if (IsGamepadButtonPressed(gamepad, DPAD_UP)) {
        pad->dpadUp = true;
    }
}

void dpad_scroll(GamepadState *pad, int *lap_count, int visible, int *scroll, int *active) {
    if (!pad->dpadUp && !pad->dpadDown) return;
    if (*lap_count <= 0) return;

    if (pad->dpadUp) (*active)--;
    if (pad->dpadDown) (*active)++;
    if (*active >= *lap_count || *active < 0) {
        *active = 0;
    }

    if (*active < *scroll) *scroll = *active;
    if (*active >= *scroll + visible) *scroll = *active - visible + 1;
}
