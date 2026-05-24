#ifndef ENGINE_ANIM_H
#define ENGINE_ANIM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ANIM_NONE = 0,
    ANIM_SELECT_FLASH,
    ANIM_CELL_FLASH,
    ANIM_WRONG_SHAKE,
    ANIM_CORRECT_FLASH,
    ANIM_LAYOUT_REFLOW,
    ANIM_BAR_CASCADE,
    ANIM_STATS_FADE,
    ANIM_LOSE_REVEAL
} AnimType;

// Start an animation. data is type-specific payload (e.g., which cells, which tier).
// data may be NULL for animations that need no parameters. Up to 8 data bytes.
// Interrupts any currently-active animation.
void anim_start(AnimType type, const uint8_t *data, uint16_t duration);

// Advance the active animation by one frame. Call from VBlank handler.
// Type-dispatched: each AnimType has its own rendering logic.
void anim_tick(void);

// True if an animation is currently active. Scenes use this to gate input.
bool anim_is_playing(void);

AnimType anim_current(void);

#endif
