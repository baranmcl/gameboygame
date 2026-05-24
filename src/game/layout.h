#ifndef GAME_LAYOUT_H
#define GAME_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

#define LAYOUT_CELL_W    9   // tile columns per unsolved cell (incl. border)
#define LAYOUT_SCREEN_W  20  // total tile columns on screen
#define LAYOUT_SCREEN_H  18  // total tile rows on screen

// Maximum solved bars stacked at the top. 4 would mean PLAY ends; UI shows
// up to 3 simultaneously.
#define LAYOUT_MAX_BARS  4

typedef struct {
    // Solved bars at the top. bars_count == popcount(groups_solved).
    // Each bar occupies one tile row, indexed by tier (0=yellow .. 3=purple).
    uint8_t bars_count;
    uint8_t bar_tier[LAYOUT_MAX_BARS];     // tier index of each bar (0..3), in top-to-bottom order
    uint8_t bar_y[LAYOUT_MAX_BARS];        // tilemap row of each bar

    // Where the 16 unsolved-cell slots live. Cells solved-out of the puzzle
    // are NOT included in this list — only the (16 - 4*bars_count) remaining
    // unsolved cells. cells_count is set accordingly.
    uint8_t cells_count;
    uint8_t cell_x[16];                     // tilemap col of each remaining cell's top-left
    uint8_t cell_y[16];                     // tilemap row of each remaining cell's top-left
    uint8_t cell_h;                         // tilemap rows per remaining cell (varies with bars_count)
} LayoutInfo;

// Populate *out for the given groups_solved bitmask (bits 0..3).
// Returns false if groups_solved has more than 3 bits set (caller should
// transition to WIN before layout for 4 solved is meaningful).
bool compute_play_layout(uint8_t groups_solved, LayoutInfo *out);

#endif
