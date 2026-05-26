#ifndef GAME_SCENE_HANDOFF_H
#define GAME_SCENE_HANDOFF_H

#include <stdint.h>

// Lightweight transient data passed between scenes via a single static
// instance. Used because adding fields to GameSave for post-puzzle data
// would muddy the SRAM schema and require save-version migration, and
// using globals avoids threading a result pointer through scene
// transitions.
//
// Protocol: PLAY's submission code populates `last_puzzle_result` just
// before setting *next_scene = SCENE_WIN. WIN's win_init reads it.
// Initial value (zero-filled) is safe — readers always populate it
// before relying on it.

typedef struct {
    uint8_t  tries_used;          // 0..4 (= 4 - tries_remaining at submit time)
    uint16_t elapsed_seconds;     // copy of PlayState.elapsed_seconds at submit
    uint8_t  attempt_number;      // current_puzzle_fails + 1 (1-based for display)
} PuzzleResult;

extern PuzzleResult last_puzzle_result;

// v1.2 Phase 7: replay-mode handoff. Set by SCENE_PUZZLE_SELECT before
// transitioning to SCENE_PLAY. Sentinel REPLAY_NONE = 0xFF means "linear
// flow, not a replay" — scene_play uses pg_save.current_puzzle_index as
// the puzzle to load. Any other value is the puzzle index to replay;
// scene_play uses that puzzle, doesn't advance current_puzzle_index on
// WIN, doesn't update lifetime/streak stats, and scene_win/lose
// transition back to SCENE_PUZZLE_SELECT instead of next linear puzzle.
#define REPLAY_NONE 0xFF
extern uint8_t replay_puzzle_index;

#endif
