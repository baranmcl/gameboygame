#ifndef GAME_PUZZLES_TYPES_H
#define GAME_PUZZLES_TYPES_H

#include <stdint.h>

#define WORDS_PER_PUZZLE       16
#define GROUPS_PER_PUZZLE       4
#define WORDS_PER_GROUP         4
#define MAX_WORD_LEN            8
#define MAX_CATEGORY_NAME_LEN  12

// Compile-time upper bound on puzzle count. Save struct allocates space
// for per-puzzle records up to this many puzzles. Must be a multiple of
// 8 (completed_bits is a uint8_t array). Currently 64 — supports up to
// 64 puzzles without a save migration. NUM_PUZZLES (runtime, set by
// codegen) MUST be <= MAX_PUZZLES_SUPPORTED.
#define MAX_PUZZLES_SUPPORTED 64

typedef struct {
    char    words[WORDS_PER_PUZZLE][MAX_WORD_LEN + 1];
    uint8_t group_of[WORDS_PER_PUZZLE];
    char    category_names[GROUPS_PER_PUZZLE][MAX_CATEGORY_NAME_LEN + 1];
} Puzzle;

extern const uint8_t NUM_PUZZLES;
extern const Puzzle  PUZZLES[];

#endif
