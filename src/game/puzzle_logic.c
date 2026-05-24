#include "puzzle_logic.h"

bool is_group_correct(const Puzzle *p, uint16_t selected_mask) {
    if (count_selected(selected_mask) != 4) return false;

    int8_t target_group = -1;
    for (uint8_t i = 0; i < WORDS_PER_PUZZLE; i++) {
        if (selected_mask & (1u << i)) {
            if (target_group == -1) {
                target_group = (int8_t)p->group_of[i];
            } else if (p->group_of[i] != (uint8_t)target_group) {
                return false;
            }
        }
    }
    return target_group != -1;
}

uint8_t count_selected(uint16_t mask) {
    uint8_t count = 0;
    while (mask) {
        count += (uint8_t)(mask & 1);
        mask >>= 1;
    }
    return count;
}

bool toggle_selection(PlayState *state, uint8_t cursor_idx) {
    uint16_t bit = (uint16_t)(1u << cursor_idx);
    bool currently_set = (state->selected_mask & bit) != 0;

    if (!currently_set && count_selected(state->selected_mask) >= 4) {
        return false;  // reject 5th selection
    }
    state->selected_mask ^= bit;
    return true;
}

uint8_t find_group_of_word(const Puzzle *p, uint8_t word_idx) {
    return p->group_of[word_idx];
}
