// test/test_layout.c — host-side unit tests for src/game/layout.c
#include "game/layout.h"
#include <stdio.h>

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

static void test_layout_zero_solved(void) {
    LayoutInfo lay;
    bool ok = compute_play_layout(0x00, &lay);
    ASSERT_TRUE(ok, "0 solved should be a valid layout");
    ASSERT_EQ(lay.bars_count, 0, "0 solved → 0 bars");
    ASSERT_EQ(lay.cells_count, 16, "0 solved → 16 cells");
}

static void test_layout_one_solved_yellow(void) {
    LayoutInfo lay;
    compute_play_layout(0x01, &lay);
    ASSERT_EQ(lay.bars_count, 1, "1 group solved → 1 bar");
    ASSERT_EQ(lay.bar_tier[0], 0, "yellow → tier 0");
    ASSERT_EQ(lay.bar_y[0], 0, "first bar at row 0");
}

static void test_layout_two_solved_yellow_blue(void) {
    LayoutInfo lay;
    compute_play_layout(0x05, &lay);
    ASSERT_EQ(lay.bars_count, 2, "2 groups solved → 2 bars");
    ASSERT_EQ(lay.bar_tier[0], 0, "first bar = yellow (lower tier index)");
    ASSERT_EQ(lay.bar_tier[1], 2, "second bar = blue");
    ASSERT_EQ(lay.bar_y[0], 0, "first bar at row 0");
    ASSERT_EQ(lay.bar_y[1], 1, "second bar at row 1");
}

static void test_layout_three_solved_all_but_purple(void) {
    LayoutInfo lay;
    compute_play_layout(0x07, &lay);
    ASSERT_EQ(lay.bars_count, 3, "3 groups solved → 3 bars");
    ASSERT_EQ(lay.bar_tier[0], 0, "tier 0 first");
    ASSERT_EQ(lay.bar_tier[1], 1, "tier 1 second");
    ASSERT_EQ(lay.bar_tier[2], 2, "tier 2 third");
}

static void test_layout_four_solved_invalid(void) {
    LayoutInfo lay;
    bool ok = compute_play_layout(0x0F, &lay);
    ASSERT_TRUE(!ok, "4 groups solved → not a valid PLAY layout");
}

static void test_layout_zero_solved_cell_positions(void) {
    LayoutInfo lay;
    compute_play_layout(0x00, &lay);
    // 16 cells in 8 rows × 2 cols. With 0 bars, header reserves row 0,
    // gap row 1, so cells start at row 2. 16 rows available / 8 rows of
    // cells = 2 tile rows per cell.
    ASSERT_EQ(lay.cell_h, 2, "0 bars → 2 tile rows per cell");
    ASSERT_EQ(lay.cell_x[0], 1, "cell 0 at col 1");
    ASSERT_EQ(lay.cell_y[0], 1, "cell 0 at row 1 (header on row 0)");
    ASSERT_EQ(lay.cell_x[1], 11, "cell 1 at col 11");
    ASSERT_EQ(lay.cell_y[1], 1, "cell 1 at row 1");
    ASSERT_EQ(lay.cell_x[2], 1, "cell 2 at col 1");
    ASSERT_EQ(lay.cell_y[2], 3, "cell 2 at row 3");
}

static void test_layout_two_solved_cell_h(void) {
    LayoutInfo lay;
    compute_play_layout(0x05, &lay);
    // 8 cells remaining, 4 rows × 2 cols, 15 rows available (18 - 2 bars - 1 gap)
    // 15/4 ~ 3 rows per cell.
    ASSERT_EQ(lay.cell_h, 3, "2 bars → 3 tile rows per cell");
    ASSERT_EQ(lay.cells_count, 8, "8 cells remaining");
    ASSERT_EQ(lay.cell_y[0], 3, "cell 0 at row 3 (2 bars + 1 gap)");
}

int main(void) {
    test_layout_zero_solved();
    test_layout_one_solved_yellow();
    test_layout_two_solved_yellow_blue();
    test_layout_three_solved_all_but_purple();
    test_layout_four_solved_invalid();
    test_layout_zero_solved_cell_positions();
    test_layout_two_solved_cell_h();

    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
