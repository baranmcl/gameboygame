#include "layout.h"
#include <string.h>

bool compute_play_layout(uint8_t groups_solved, LayoutInfo *out) {
    memset(out, 0, sizeof(*out));

    uint8_t bars = 0;
    for (uint8_t tier = 0; tier < 4; tier++) {
        if (groups_solved & (1u << tier)) {
            if (bars >= LAYOUT_MAX_BARS) break;
            out->bar_tier[bars] = tier;
            out->bar_y[bars] = bars;
            bars++;
        }
    }
    if (bars > 3) return false;

    out->bars_count = bars;
    out->cells_count = (uint8_t)(16 - bars * 4);

    // Cell grid: 2 columns, (cells_count / 2) rows. Cells fill the area
    // below the bars (which occupy rows 0..bars-1). When bars=0, row 0
    // acts as a header gap; when bars>0, row `bars` is a 1-tile gap between
    // bars and the cell grid.
    uint8_t cell_rows = (uint8_t)(out->cells_count / 2);
    uint8_t available_rows = (uint8_t)(LAYOUT_SCREEN_H - (bars + 1));
    out->cell_h = (uint8_t)(available_rows / cell_rows);

    // Cell area starts at row (bars + 1): row 0..bars-1 are bars (or row 0
    // is the header when bars=0), row `bars` is a 1-tile gap.
    uint8_t cell_top = (uint8_t)(bars + 1);

    for (uint8_t i = 0; i < out->cells_count; i++) {
        uint8_t r = (uint8_t)(i / 2);
        uint8_t c = (uint8_t)(i % 2);
        out->cell_x[i] = (uint8_t)(1 + c * 10);
        out->cell_y[i] = (uint8_t)(cell_top + r * out->cell_h);
    }
    return true;
}
