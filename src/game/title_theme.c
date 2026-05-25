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

const MusicTrack TITLE_THEME = {
    .steps      = TITLE_THEME_STEPS,
    .length     = 32,
    .loop_start = 0,  // full-track loop
};
