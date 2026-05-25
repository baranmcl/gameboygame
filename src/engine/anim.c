#include "anim.h"
#include <gb/gb.h>
#include <string.h>

typedef struct {
    AnimType type;
    uint16_t frame;
    uint16_t duration;
    uint8_t  data[8];
} Anim;

static Anim active = { ANIM_NONE, 0, 0, {0} };

void anim_start(AnimType type, const uint8_t *data, uint16_t duration) {
    active.type     = type;
    active.frame    = 0;
    active.duration = duration;
    if (data) memcpy(active.data, data, 8);
    else      memset(active.data, 0, 8);
}

bool anim_is_playing(void) {
    return active.type != ANIM_NONE;
}

AnimType anim_current(void) {
    return active.type;
}

// ----- per-type tick handlers -----

static void tick_wrong_shake(void) {
    if (active.frame >= active.duration) {
        SCX_REG = 0;
        active.type = ANIM_NONE;
        return;
    }
    // Alternates +1 / -1 pixel horizontally per frame (255 == -1 mod 256)
    SCX_REG = (active.frame & 1) ? 1 : 255;
    active.frame++;
}

static void tick_select_flash(void) {
    // Real implementation: whole-screen palette inversion for the duration.
    // Cheap visual punctuation that doesn't require knowing cell coordinates
    // (anim engine has no scene context). At 4 frames duration this is a
    // single quick flash — toggles inverted/normal every 2 frames.
    //
    // Palette values:
    //   0xE4 = 11 10 01 00 — identity (color N → shade N)
    //   0x1B = 00 01 10 11 — inverted (color N → shade 3-N)
    if (active.frame >= active.duration) {
        BGP_REG = 0xE4;  // restore normal palette on end
        active.type = ANIM_NONE;
        return;
    }
    BGP_REG = ((active.frame / 2) & 1) ? 0xE4 : 0x1B;
    active.frame++;
}

// All other animations stub to "advance + end on duration" for Plan A.
// Plan B replaces these with real implementations as scenes need them.
static void tick_generic_stub(void) {
    if (active.frame >= active.duration) {
        active.type = ANIM_NONE;
        return;
    }
    active.frame++;
}

void anim_tick(void) {
    switch (active.type) {
        case ANIM_NONE:           return;
        case ANIM_WRONG_SHAKE:    tick_wrong_shake(); break;
        case ANIM_SELECT_FLASH:   tick_select_flash(); break;
        case ANIM_CELL_FLASH:
        case ANIM_CORRECT_FLASH:
        case ANIM_LAYOUT_REFLOW:
        case ANIM_BAR_CASCADE:
        case ANIM_STATS_FADE:
        case ANIM_LOSE_REVEAL:
            tick_generic_stub();
            break;
        default:
            active.type = ANIM_NONE;
    }
}
