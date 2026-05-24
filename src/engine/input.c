#include "input.h"
#include <gb/gb.h>

#define INITIAL_DELAY_FRAMES 15
#define REPEAT_PERIOD_FRAMES 5

static uint8_t current  = 0;
static uint8_t previous = 0;
static uint8_t hold_frames[8] = {0};

static uint8_t button_to_index(Button b) {
    switch (b) {
        case BTN_RIGHT:  return 0;
        case BTN_LEFT:   return 1;
        case BTN_UP:     return 2;
        case BTN_DOWN:   return 3;
        case BTN_A:      return 4;
        case BTN_B:      return 5;
        case BTN_SELECT: return 6;
        case BTN_START:  return 7;
        default:         return 0;
    }
}

void input_update(void) {
    previous = current;
    current  = joypad();

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t mask = (uint8_t)(1u << i);
        if (current & mask) {
            if (hold_frames[i] < 0xFF) hold_frames[i]++;
        } else {
            hold_frames[i] = 0;
        }
    }
}

bool input_pressed(Button b) {
    return (current & b) && !(previous & b);
}

bool input_held(Button b) {
    return (current & b) != 0;
}

bool input_released(Button b) {
    return !(current & b) && (previous & b);
}

bool input_repeat(Button b) {
    if (input_pressed(b)) return true;
    if (!(current & b)) return false;
    uint8_t idx = button_to_index(b);
    uint8_t f = hold_frames[idx];
    if (f < INITIAL_DELAY_FRAMES) return false;
    return ((f - INITIAL_DELAY_FRAMES) % REPEAT_PERIOD_FRAMES) == 0;
}
