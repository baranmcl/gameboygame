#ifndef GAME_PUZZLES_TYPES_H
#define GAME_PUZZLES_TYPES_H

#include <stdint.h>

#define WORDS_PER_PUZZLE       16
#define GROUPS_PER_PUZZLE       4
#define WORDS_PER_GROUP         4
#define MAX_WORD_LEN            8
#define MAX_CATEGORY_NAME_LEN  12

typedef struct {
    char    words[WORDS_PER_PUZZLE][MAX_WORD_LEN + 1];
    uint8_t group_of[WORDS_PER_PUZZLE];
    char    category_names[GROUPS_PER_PUZZLE][MAX_CATEGORY_NAME_LEN + 1];
} Puzzle;

extern const uint8_t NUM_PUZZLES;
extern const Puzzle  PUZZLES[];

#endif
