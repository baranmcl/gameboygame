#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BTN_RIGHT  = 0x01,
    BTN_LEFT   = 0x02,
    BTN_UP     = 0x04,
    BTN_DOWN   = 0x08,
    BTN_A      = 0x10,
    BTN_B      = 0x20,
    BTN_SELECT = 0x40,
    BTN_START  = 0x80
} Button;

// Call once per frame, after VBlank, before scene update().
// Reads the joypad and updates internal previous/current state.
void input_update(void);

// True if button was newly pressed this frame (edge-triggered).
bool input_pressed(Button b);

// True if button is currently held (raw, no edge detection).
bool input_held(Button b);

// True if button was newly released this frame (edge-triggered).
bool input_released(Button b);

// True if button was newly pressed this frame OR has been held long enough
// to fire an auto-repeat tick (initial delay ~250ms = 15 frames; repeat rate
// ~80ms = 5 frames). Used for D-pad in scenes with cursors.
bool input_repeat(Button b);

#endif
