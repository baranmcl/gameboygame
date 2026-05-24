// test/test_puzzle_logic.c — host-side unit tests for src/game/puzzle_logic.c
#include "game/puzzle_logic.h"
#include <stdio.h>

// Mock puzzle: words 0-3 in group 0, 4-7 in group 1, 8-11 in group 2, 12-15 in group 3.
static const Puzzle TEST_PUZZLE = {
    .words = {
        "A","B","C","D","E","F","G","H",
        "I","J","K","L","M","N","O","P"
    },
    .group_of = {
        0,0,0,0,
        1,1,1,1,
        2,2,2,2,
        3,3,3,3
    },
    .category_names = {"G0","G1","G2","G3"}
};

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { tests_failed++; fprintf(stderr, "FAIL: %s (line %d): %s\n", __func__, __LINE__, msg); } \
} while (0)

#define ASSERT_EQ(actual, expected, msg) do { \
    tests_run++; \
    if ((actual) != (expected)) { tests_failed++; fprintf(stderr, "FAIL: %s (line %d): %s — expected %d, got %d\n", __func__, __LINE__, msg, (int)(expected), (int)(actual)); } \
} while (0)

static void test_is_group_correct_all_same_group(void) {
    // Bits 0,1,2,3 set → all group 0
    ASSERT_TRUE(is_group_correct(&TEST_PUZZLE, 0x000F), "all-group-0 should be correct");
}

static void test_is_group_correct_three_plus_one(void) {
    // Bits 0,1,2,4 → 3 from group 0, 1 from group 1
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x0017), "3+1 should not be correct");
}

static void test_is_group_correct_only_three_selected(void) {
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x0007), "3 selected should not be correct");
}

static void test_is_group_correct_only_five_selected(void) {
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x001F), "5 selected should not be correct");
}

static void test_is_group_correct_zero_selected(void) {
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x0000), "0 selected should not be correct");
}

static void test_count_selected(void) {
    ASSERT_EQ(count_selected(0x0000), 0, "popcount(0)");
    ASSERT_EQ(count_selected(0xFFFF), 16, "popcount(0xFFFF)");
    ASSERT_EQ(count_selected(0x000F), 4, "popcount(0x000F)");
    ASSERT_EQ(count_selected(0xAAAA), 8, "popcount(alternating)");
}

static void test_toggle_selection_adds_when_under_limit(void) {
    PlayState s = {0};
    s.selected_mask = 0x0001;
    bool ok = toggle_selection(&s, 5);
    ASSERT_TRUE(ok, "should accept toggle when under 4");
    ASSERT_EQ(s.selected_mask, 0x0021, "bit 5 should be set");
}

static void test_toggle_selection_removes_existing(void) {
    PlayState s = {0};
    s.selected_mask = 0x0001;
    bool ok = toggle_selection(&s, 0);
    ASSERT_TRUE(ok, "should accept toggle of existing selection");
    ASSERT_EQ(s.selected_mask, 0x0000, "bit 0 should be cleared");
}

static void test_toggle_selection_rejects_fifth(void) {
    PlayState s = {0};
    s.selected_mask = 0x000F;
    bool ok = toggle_selection(&s, 5);
    ASSERT_TRUE(!ok, "should reject 5th selection");
    ASSERT_EQ(s.selected_mask, 0x000F, "mask should be unchanged");
}

static void test_toggle_selection_allows_deselect_when_at_limit(void) {
    PlayState s = {0};
    s.selected_mask = 0x000F;
    bool ok = toggle_selection(&s, 2);
    ASSERT_TRUE(ok, "should allow deselect at limit");
    ASSERT_EQ(s.selected_mask, 0x000B, "bit 2 should be cleared");
}

static void test_find_group_of_word(void) {
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 0), 0, "word 0 → group 0");
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 5), 1, "word 5 → group 1");
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 11), 2, "word 11 → group 2");
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 15), 3, "word 15 → group 3");
}

int main(void) {
    test_is_group_correct_all_same_group();
    test_is_group_correct_three_plus_one();
    test_is_group_correct_only_three_selected();
    test_is_group_correct_only_five_selected();
    test_is_group_correct_zero_selected();
    test_count_selected();
    test_toggle_selection_adds_when_under_limit();
    test_toggle_selection_removes_existing();
    test_toggle_selection_rejects_fifth();
    test_toggle_selection_allows_deselect_when_at_limit();
    test_find_group_of_word();

    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
