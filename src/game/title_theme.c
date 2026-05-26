#include "title_theme.h"

// 32 steps × 24 frames per step = 768 frames = ~12.8 seconds at 60 fps.
// Structure:
//   Bars 1-2:  ascending C major arpeggio with descending answer
//   Bars 3-4:  motif repeat with variation
//   Bars 5-6:  arpeggio up a note (Dm) for tension
//   Bars 7-8:  return to C and resolve down
//
// All notes are quarter-notes (24 frames). REST is included for
// breathing room between phrases.
static const MusicStep TITLE_THEME_STEPS[32] = {
    // Bar 1: C E G C↑
    { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    // Bar 2: G E C rest
    { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_REST, 24 },
    // Bar 3: C E G C↑ (repeat opening)
    { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    // Bar 4: G C↑ G E (variation)
    { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 },
    // Bar 5: D F A D (Dm arpeggio)
    { MUSIC_NOTE_D5, 24 }, { MUSIC_NOTE_F5, 24 }, { MUSIC_NOTE_A5, 24 }, { MUSIC_NOTE_D5, 24 },
    // Bar 6: A F D rest
    { MUSIC_NOTE_A5, 24 }, { MUSIC_NOTE_F5, 24 }, { MUSIC_NOTE_D5, 24 }, { MUSIC_NOTE_REST, 24 },
    // Bar 7: C E G C↑ (return)
    { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    // Bar 8: C↑ G E C (resolution)
    { MUSIC_NOTE_C6, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_C5, 24 },
};

// v1.2 Phase 5: CH2 harmony layer — lockstep with TITLE_THEME_STEPS.
// 32 entries to match the melody length. Designed as supportive thirds
// below the melody for arpeggio bars, held tones for static moments,
// and parallel descents for the resolution phrases. Stays within C/Dm
// tonality so it never clashes with the lead.
static const uint8_t TITLE_THEME_CH2[32] = {
    // Bar 1 (melody C E G C↑): held G beneath — stable foundation
    MUSIC_NOTE_G4, MUSIC_NOTE_G4, MUSIC_NOTE_G4, MUSIC_NOTE_G4,
    // Bar 2 (melody G E C rest): echoes the descent in parallel
    MUSIC_NOTE_E4, MUSIC_NOTE_C4, MUSIC_NOTE_G4, MUSIC_NOTE_REST,
    // Bar 3 (melody C E G C↑): thirds below the melody
    MUSIC_NOTE_A4, MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    // Bar 4 (melody G C↑ G E): held E for tension under the variation
    MUSIC_NOTE_E5, MUSIC_NOTE_E5, MUSIC_NOTE_E5, MUSIC_NOTE_C5,
    // Bar 5 (melody D F A D — Dm arpeggio): held A for D-minor color
    MUSIC_NOTE_A4, MUSIC_NOTE_A4, MUSIC_NOTE_A4, MUSIC_NOTE_A4,
    // Bar 6 (melody A F D rest): descends in parallel with the lead
    MUSIC_NOTE_F4, MUSIC_NOTE_D4, MUSIC_NOTE_A4, MUSIC_NOTE_REST,
    // Bar 7 (melody C E G C↑, return): thirds below the melody
    MUSIC_NOTE_A4, MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    // Bar 8 (melody C↑ G E C, resolution): descends to root C
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G4, MUSIC_NOTE_C4,
};

// v1.2 Phase 5: CH4 drum layer — half-time rock beat.
// Each "bar" in the comments above is 4 steps (4 beats at quarter-note).
// Pattern: KICK on beat 1, SNARE on beat 3, REST on beats 2 and 4.
// 16 hits over 32 steps — sparse enough to support the melody/harmony
// without competing with them.
static const uint8_t TITLE_THEME_CH4[32] = {
    // Bar 1
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 2
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 3
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 4
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 5
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 6
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 7
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
    // Bar 8
    MUSIC_DRUM_KICK,  MUSIC_DRUM_REST, MUSIC_DRUM_SNARE, MUSIC_DRUM_REST,
};

const MusicTrack TITLE_THEME = {
    .steps      = TITLE_THEME_STEPS,
    .ch2_notes  = TITLE_THEME_CH2,
    .ch4_drums  = TITLE_THEME_CH4,
    .length     = 32,
    .loop_start = 0,  // full-track loop
};
