#include "all_done_theme.h"

// ~9 seconds of celebration. Structured in 5 phrases across 16 steps:
//   Bar A (4 steps, energetic): rising C major fanfare C5 E5 G5 C6
//   Bar B (3 steps, echo):      descending answer G5 E5 C5
//   Bar C (3 steps, sustained): savored ascent E5 G5 C6
//   Bar D (3 steps, descent):   gentle drop G5 E5 C5
//   Bar E (3 steps, resolution): C5 G5 final-held C6
//
// Tempo varies by phrase: opening is quick (20 frames/step), middle
// phrases slower (30-40 frames/step), final note held (120 frames).
// Total ~560 frames at 60fps ≈ 9.3 seconds.
//
// CH2 harmony layer follows the standard "thirds below the melody"
// pattern from title_theme.c for tonal consistency.
static const MusicStep ALL_DONE_FANFARE_STEPS[16] = {
    // Bar A: rising fanfare
    { MUSIC_NOTE_C5, 20 }, { MUSIC_NOTE_E5, 20 }, { MUSIC_NOTE_G5, 20 }, { MUSIC_NOTE_C6, 20 },
    // Bar B: descending echo
    { MUSIC_NOTE_G5, 30 }, { MUSIC_NOTE_E5, 30 }, { MUSIC_NOTE_C5, 30 },
    // Bar C: sustained ascent
    { MUSIC_NOTE_E5, 40 }, { MUSIC_NOTE_G5, 40 }, { MUSIC_NOTE_C6, 40 },
    // Bar D: gentle descent
    { MUSIC_NOTE_G5, 30 }, { MUSIC_NOTE_E5, 30 }, { MUSIC_NOTE_C5, 30 },
    // Bar E: resolution + held final
    { MUSIC_NOTE_C5, 30 }, { MUSIC_NOTE_G5, 30 }, { MUSIC_NOTE_C6, 120 },
};

static const uint8_t ALL_DONE_FANFARE_CH2[16] = {
    // Bar A harmony: thirds below the rising melody
    MUSIC_NOTE_G4, MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    // Bar B harmony: parallel descent
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G4,
    // Bar C harmony: thirds below ascent
    MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    // Bar D harmony: parallel descent
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G4,
    // Bar E harmony: held G beneath the final C6 resolution
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G5,
};

const MusicTrack ALL_DONE_FANFARE = {
    .steps      = ALL_DONE_FANFARE_STEPS,
    .ch2_notes  = ALL_DONE_FANFARE_CH2,
    .ch4_drums  = 0,    // no drums — melody + harmony alone carry the moment
    .length     = 16,
    .loop_start = 16,   // non-looping (player auto-stops after final note)
};
