#include "sound.h"
#include <gb/gb.h>
#include <gb/hardware.h>
#include <stdint.h>

// One active multi-step SFX at a time. step_remaining counts down per tick;
// when it reaches 0, advance to the next step.
typedef enum {
    SFX_NONE = 0,
    SFX_PLAYING_CORRECT,
    SFX_PLAYING_WRONG,
    SFX_PLAYING_WIN,
    SFX_PLAYING_LOSE,
    SFX_PLAYING_SKIP
} ActiveSfx;

static ActiveSfx active = SFX_NONE;
static uint8_t   step = 0;
static uint8_t   step_remaining = 0;

void sound_init(void) {
    NR52_REG = 0x80;  // master enable
    NR51_REG = 0xFF;  // route all channels to both speakers
    NR50_REG = 0x77;  // max volume both speakers
}

// ---------- Single-tick SFX (no scheduling needed) ----------

void sfx_move(void) {
    NR21_REG = 0x80;
    NR22_REG = 0x84;
    NR23_REG = 0x00;
    NR24_REG = 0x87;
    active = SFX_NONE;
}

void sfx_select(void) {
    NR21_REG = 0x80;
    NR22_REG = 0xA4;
    NR23_REG = 0x80;
    NR24_REG = 0x87;
    active = SFX_NONE;
}

void sfx_deselect(void) {
    NR21_REG = 0x80;
    NR22_REG = 0xA4;
    NR23_REG = 0x00;
    NR24_REG = 0x86;
    active = SFX_NONE;
}

void sfx_reject(void) {
    NR41_REG = 0x00;
    NR42_REG = 0xF3;
    NR43_REG = 0x6B;
    NR44_REG = 0x80;
    active = SFX_NONE;
}

// ---------- Multi-step SFX (use sound_tick to advance) ----------

void sfx_correct(void) {
    active = SFX_PLAYING_CORRECT;
    step = 0;
    step_remaining = 5;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0xC1;
    NR14_REG = 0x87;
}

void sfx_wrong(void) {
    active = SFX_PLAYING_WRONG;
    step = 0;
    step_remaining = 6;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x44;
    NR14_REG = 0x87;
}

void sfx_win(void) {
    active = SFX_PLAYING_WIN;
    step = 0;
    step_remaining = 8;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0xC1;
    NR14_REG = 0x87;
}

void sfx_lose(void) {
    active = SFX_PLAYING_LOSE;
    step = 0;
    step_remaining = 12;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x44;
    NR14_REG = 0x87;
}

void sfx_skip(void) {
    active = SFX_PLAYING_SKIP;
    step = 0;
    step_remaining = 10;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x83;
    NR14_REG = 0x87;
}

// ---------- Tick driver ----------

void sound_tick(void) {
    if (active == SFX_NONE) return;
    if (step_remaining > 0) {
        step_remaining--;
        return;
    }

    step++;

    switch (active) {
        case SFX_PLAYING_CORRECT: {
            if (step == 1) {
                NR13_REG = 0x83; NR14_REG = 0x87;
                step_remaining = 5;
            } else if (step == 2) {
                NR13_REG = 0xC1; NR14_REG = 0x87;
                step_remaining = 5;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_WRONG: {
            if (step == 1) {
                NR13_REG = 0xC1; NR14_REG = 0x86;
                step_remaining = 6;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_WIN: {
            if (step == 1) {
                NR13_REG = 0x83; NR14_REG = 0x87;
                step_remaining = 8;
            } else if (step == 2) {
                NR13_REG = 0xC1; NR14_REG = 0x87;
                step_remaining = 8;
            } else if (step == 3) {
                NR13_REG = 0xC1; NR14_REG = 0x88;
                step_remaining = 16;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_LOSE: {
            if (step == 1) {
                NR13_REG = 0xC1; NR14_REG = 0x86;
                step_remaining = 12;
            } else if (step == 2) {
                NR13_REG = 0x44; NR14_REG = 0x86;
                step_remaining = 16;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_SKIP: {
            if (step == 1) {
                NR13_REG = 0x44; NR14_REG = 0x87;
                step_remaining = 10;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        default:
            active = SFX_NONE;
    }
}
