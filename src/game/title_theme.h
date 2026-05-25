#ifndef GAME_TITLE_THEME_H
#define GAME_TITLE_THEME_H

#include "../engine/music.h"

// 8-bar upbeat title theme, ~12.8 second loop at 60fps. Each step is
// a quarter-note (24 frames at 60fps ≈ 150 BPM). CH1-only. Authored
// by hand in this file because hUGETracker / other trackers are out
// of scope for this project.
extern const MusicTrack TITLE_THEME;

#endif
