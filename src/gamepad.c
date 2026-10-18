#include <gamepad.h>
#include <stdbool.h>
#include <string.h>

#define SCROLL_CAP 5

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

void dpad_scroll(GamepadState *pad, int *lap_count, int *scroll, int *active) {
    if (pad->dpadUp) (*active)--;
    if (pad->dpadDown) (*active)++;
    if (*active >= *lap_count || *active < 0) {
        *active = 0;
    }
    if (*active % SCROLL_CAP) {
        *scroll = *active;
        int last_page = *lap_count - SCROLL_CAP;
        if (last_page < 0) last_page = 0;
        if (*scroll > last_page) *scroll = last_page;
    }
}
