#ifndef GAME_PUZZLE_LOGIC_H
#define GAME_PUZZLE_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "puzzles_types.h"
#include "game_state.h"

// True if the 4 bits set in selected_mask all correspond to words in the same group.
// Returns false if popcount(selected_mask) != 4 or words span multiple groups.
bool is_group_correct(const Puzzle *p, uint16_t selected_mask);

// Returns the number of bits set in mask (popcount).
uint8_t count_selected(uint16_t mask);

// Toggle the bit at cursor_idx in state->selected_mask. If 4 are already
// selected and the bit at cursor_idx is currently 0, the toggle is rejected
// (function returns false). Otherwise returns true.
bool toggle_selection(PlayState *state, uint8_t cursor_idx);

// Returns the group index (0..3) that word_idx belongs to in puzzle p.
uint8_t find_group_of_word(const Puzzle *p, uint8_t word_idx);

#endif
