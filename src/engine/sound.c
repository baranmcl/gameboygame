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

// CH3 wave RAM template — a simple 50%-duty square wave (16 samples high,
// 16 samples low, packed as 16 bytes × 2 nibbles each). Gives CH3 a tonal
// character similar to CH1/CH2 square channels. Loaded once at boot;
// must not be touched while CH3 is playing.
static const uint8_t WAVE_RAM_SQUARE[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void sound_init(void) {
    NR52_REG = 0x80;  // master enable
    NR51_REG = 0xFF;  // route all channels to both speakers
    NR50_REG = 0x77;  // max volume both speakers

    // Load CH3 wave RAM with a square waveform for sfx_move (v1.2 Phase 7
    // fix — moving cursor SFX off CH2 so it stops clobbering the title
    // theme's harmony layer). CH3 must be disabled (NR30 = 0) while we
    // write wave RAM to avoid hardware undefined-read behavior.
    NR30_REG = 0x00;
    for (uint8_t i = 0; i < 16; i++) {
        _AUD3WAVERAM[i] = WAVE_RAM_SQUARE[i];
    }
    NR30_REG = 0x80;  // re-enable CH3 (no sound until NR34 trigger)
}

// ----- Pitch-byte constants (Plan C calibration) -----
//
// GB frequency formula: hz = 131072 / (2048 - x). Inverse: x = 2048 - 131072/hz.
// NR13 = low 8 bits of x. NR14 = trigger(0x80) | bit6 length(0) | low 3 bits = (x >> 8) & 7.
// For audible range C4..C6, x is in 1547..1923, so NR14 high-bits are 6 or 7.
//
//                                NR13   NR14
#define NOTE_C4_LO 0x0B
#define NOTE_C4_HI 0x86
#define NOTE_D4_LO 0x42
#define NOTE_D4_HI 0x86
#define NOTE_E4_LO 0x72
#define NOTE_E4_HI 0x86
#define NOTE_G4_LO 0xB2
#define NOTE_G4_HI 0x86
#define NOTE_A4_LO 0xD6
#define NOTE_A4_HI 0x86
#define NOTE_C5_LO 0x06
#define NOTE_C5_HI 0x87
#define NOTE_D5_LO 0x21
#define NOTE_D5_HI 0x87
#define NOTE_E5_LO 0x39
#define NOTE_E5_HI 0x87
#define NOTE_F5_LO 0x44
#define NOTE_F5_HI 0x87
#define NOTE_G5_LO 0x59
#define NOTE_G5_HI 0x87
#define NOTE_A5_LO 0x6B
#define NOTE_A5_HI 0x87
#define NOTE_C6_LO 0x83
#define NOTE_C6_HI 0x87

// ---------- Single-tick SFX (no scheduling needed) ----------

void sfx_move(void) {
    // v1.2 Phase 7 fix: moved from CH2 → CH3 (wave channel). The previous
    // CH2 implementation was clobbering the title-theme's CH2 harmony
    // every d-pad press; now CH2 stays available for harmony while
    // sfx_move plays on CH3.
    //
    // CH3 uses a different frequency formula (hz = 65536 / (2048 - x))
    // and has no envelope — just fixed volume + a length counter that
    // auto-disables the channel. ~50ms tap with full volume.
    //
    // Frequency: A5 (880 Hz) on CH3 → x = 2048 - 65536/880 = 1974
    //   NR33 = low 8 bits = 0xB6
    //   NR34 = trigger(0x80) | length-enable(0x40) | high 3 bits = 0xC7
    // Length: NR31 = 256 - 13 = 0xF3 → ~50ms playback then auto-stop.
    // Volume: NR32 = 0x40 → 100% (CH3 has no envelope, so we want full
    //   amplitude for the brief click).
    NR31_REG = 0xF3;
    NR32_REG = 0x40;
    NR33_REG = 0xB6;
    NR34_REG = 0xC7;  // trigger + length-enable + freq high (top 3 bits)
    active = SFX_NONE;
}

void sfx_select(void) {
    // E5 — mid-high "select" tone. Volume 6 (down from 10) so it sits
    // above the cursor tap but below game-event sounds.
    NR21_REG = 0x80;
    NR22_REG = 0x62;
    NR23_REG = NOTE_E5_LO;
    NR24_REG = NOTE_E5_HI;
    active = SFX_NONE;
}

void sfx_deselect(void) {
    // C5 — lower than select to suggest "going down/off"
    NR21_REG = 0x80;
    NR22_REG = 0x62;
    NR23_REG = NOTE_C5_LO;
    NR24_REG = NOTE_C5_HI;
    active = SFX_NONE;
}

void sfx_reject(void) {
    // CH4 noise. Volume slashed from 15 → 4 — the previous setting was
    // the abrasive sound the user flagged. NR43 = 0x77 gives a slower,
    // 15-bit (less metallic) noise pattern. Now reads as a soft "thunk"
    // instead of harsh static.
    NR41_REG = 0x00;
    NR42_REG = 0x43;
    NR43_REG = 0x77;
    NR44_REG = 0x80;
    active = SFX_NONE;
}

// ---------- Multi-step SFX (use sound_tick to advance) ----------

void sfx_correct(void) {
    // Rising C major arpeggio: C5 → E5 → G5
    active = SFX_PLAYING_CORRECT;
    step = 0;
    step_remaining = 5;
    NR11_REG = 0x80;
    NR12_REG = 0x93;  // volume 9 (down from 15), decay, step 3
    NR13_REG = NOTE_C5_LO;
    NR14_REG = NOTE_C5_HI;
}

void sfx_wrong(void) {
    // Descending "uh-oh": F5 → D5
    active = SFX_PLAYING_WRONG;
    step = 0;
    step_remaining = 6;
    NR11_REG = 0x80;
    NR12_REG = 0x93;  // volume 9 (down from 15), decay, step 3
    NR13_REG = NOTE_F5_LO;
    NR14_REG = NOTE_F5_HI;
}

void sfx_win(void) {
    // Celebratory ascending arpeggio + sustain: C5 → E5 → G5 → C6 (held)
    active = SFX_PLAYING_WIN;
    step = 0;
    step_remaining = 8;
    NR11_REG = 0x80;
    NR12_REG = 0x93;  // volume 9 (down from 15), decay, step 3
    NR13_REG = NOTE_C5_LO;
    NR14_REG = NOTE_C5_HI;
}

void sfx_lose(void) {
    // Descending sad: F5 → D5 → A4
    active = SFX_PLAYING_LOSE;
    step = 0;
    step_remaining = 12;
    NR11_REG = 0x80;
    NR12_REG = 0x93;  // volume 9 (down from 15), decay, step 3
    NR13_REG = NOTE_F5_LO;
    NR14_REG = NOTE_F5_HI;
}

void sfx_skip(void) {
    // Neutral 2-note descent: G4 → E4
    active = SFX_PLAYING_SKIP;
    step = 0;
    step_remaining = 10;
    NR11_REG = 0x80;
    NR12_REG = 0x93;  // volume 9 (down from 15), decay, step 3
    NR13_REG = NOTE_G4_LO;
    NR14_REG = NOTE_G4_HI;
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
                NR13_REG = NOTE_E5_LO; NR14_REG = NOTE_E5_HI;
                step_remaining = 5;
            } else if (step == 2) {
                NR13_REG = NOTE_G5_LO; NR14_REG = NOTE_G5_HI;
                step_remaining = 5;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_WRONG: {
            if (step == 1) {
                NR13_REG = NOTE_D5_LO; NR14_REG = NOTE_D5_HI;
                step_remaining = 6;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_WIN: {
            if (step == 1)      { NR13_REG = NOTE_E5_LO; NR14_REG = NOTE_E5_HI; step_remaining = 8; }
            else if (step == 2) { NR13_REG = NOTE_G5_LO; NR14_REG = NOTE_G5_HI; step_remaining = 8; }
            else if (step == 3) { NR13_REG = NOTE_C6_LO; NR14_REG = NOTE_C6_HI; step_remaining = 16; }
            else                { active = SFX_NONE; }
            break;
        }
        case SFX_PLAYING_LOSE: {
            if (step == 1)      { NR13_REG = NOTE_D5_LO; NR14_REG = NOTE_D5_HI; step_remaining = 12; }
            else if (step == 2) { NR13_REG = NOTE_A4_LO; NR14_REG = NOTE_A4_HI; step_remaining = 16; }
            else                { active = SFX_NONE; }
            break;
        }
        case SFX_PLAYING_SKIP: {
            if (step == 1) {
                NR13_REG = NOTE_E4_LO; NR14_REG = NOTE_E4_HI;
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
