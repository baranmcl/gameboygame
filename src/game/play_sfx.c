#include "play_sfx.h"

// 4 ascending notes — short, energetic. C → E → G → C↑ at quick tempo.
// Each note ~250ms (15 frames at 60fps), final held slightly longer
// for landing feel. CH1-only; CH2 and CH4 stay silent.
static const MusicStep PLAY_STINGER_STEPS[4] = {
    { MUSIC_NOTE_C5, 15 },
    { MUSIC_NOTE_E5, 15 },
    { MUSIC_NOTE_G5, 15 },
    { MUSIC_NOTE_C6, 20 },
};

const MusicTrack PLAY_STINGER = {
    .steps      = PLAY_STINGER_STEPS,
    .ch2_notes  = 0,   // melody only — no harmony for a stinger
    .ch4_drums  = 0,   // no drums
    .length     = 4,
    .loop_start = 4,   // non-looping (loop_start == length stops player)
};
