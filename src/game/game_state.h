#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include "puzzles_types.h"

typedef enum {
    SCENE_TITLE = 0,
    SCENE_PLAY,
    SCENE_WIN,
    SCENE_LOSE,
    SCENE_ALL_DONE,
    SCENE_PUZZLE_SELECT
} Scene;

typedef struct {
    uint8_t  cursor_idx;
    uint16_t selected_mask;
    uint8_t  groups_solved;
    uint8_t  tries_remaining;
    uint16_t elapsed_seconds;
    uint8_t  show_quit_confirm;
} PlayState;

// GameSave v2 — 224 bytes, persisted to SRAM at 0xA000.
// v1.0/v1.1 used a 20-byte v1 layout (still readable for migration —
// see LegacyGameSaveV1 in save.c). Field order is load-bearing for
// SRAM-format compatibility — bytes [0..19] match v1 exactly so the
// migration can read from the same offsets.
typedef struct {
    // --- v1 fields (unchanged offsets 0..19) ---
    uint8_t  magic[4];                 // [0..3]   "GBCX"
    uint8_t  version;                  // [4]      2 (was 1 in v1.0/v1.1)
    uint8_t  current_puzzle_index;     // [5]      0..NUM_PUZZLES-1
    uint8_t  current_puzzle_fails;     // [6]      0..3
    uint8_t  puzzles_solved_total;     // [7]
    uint8_t  puzzles_skipped_total;    // [8]
    uint8_t  current_streak;           // [9]
    uint8_t  best_streak;              // [10]
    uint16_t total_tries_used;         // [11..12]
    uint8_t  ip_tries_remaining;       // [13]     0 = no in-progress
    uint8_t  ip_groups_solved;         // [14]
    uint16_t ip_selected_mask;         // [15..16]
    uint16_t ip_elapsed_seconds;       // [17..18]
    uint8_t  v1_checksum;              // [19]     legacy v1 checksum byte
                                       //          position; ignored in v2
                                       //          (v2 uses checksum at [223])
    // --- v2 additions ---
    uint8_t  completed_bits[MAX_PUZZLES_SUPPORTED / 8];   // [20..27]  1 bit per puzzle
    uint16_t puzzle_best_time[MAX_PUZZLES_SUPPORTED];     // [28..155] seconds, 0 = no record
    uint8_t  puzzle_best_tries[MAX_PUZZLES_SUPPORTED];    // [156..219] 1..4, 0 = no record
    uint8_t  reserved[3];                                 // [220..222] v3 headroom
    uint8_t  checksum;                                    // [223]     XOR of bytes [0..222]
} GameSave;

// SRAM magic sentinel: just 4 arbitrary bytes that confirm SRAM was
// initialized by THIS ROM (not garbage from a previous cartridge).
// "GBCX" predates the user's Plan C "GBCX" → "GB" branding rename;
// kept as-is to avoid invalidating existing saves. The cartridge header
// title is set separately via the Makefile's `-Wm-yn"GB"` flag.
#define SAVE_MAGIC_0 'G'
#define SAVE_MAGIC_1 'B'
#define SAVE_MAGIC_2 'C'
#define SAVE_MAGIC_3 'X'
#define SAVE_VERSION 2

#endif
