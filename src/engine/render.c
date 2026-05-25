#include "render.h"
#include "../assets_gen/font.h"
#include "../assets_gen/ui_tiles.h"
#include "../assets_gen/font_inv.h"
#include "../assets_gen/title.h"
#include <gb/gb.h>
#include <string.h>

static uint8_t tilemap_buf[SCREEN_TILES_W * SCREEN_TILES_H];

// Parallel attribute buffer for GBC tile-attribute writes (palette index
// in low 3 bits; bits 3-7 unused — we don't use VRAM bank 1 for tiles,
// X/Y flip, or priority). Maintained alongside tilemap_buf and pushed
// to VRAM bank 1 in render_flush. On DMG this buffer is harmlessly
// maintained but not pushed to VRAM (DMG has no bank 1).
static uint8_t attr_buf[SCREEN_TILES_W * SCREEN_TILES_H];

static uint8_t dirty = 0;

// GBC background palette data — 6 palettes × 4 colors × 2 bytes per color.
// Format is RGB555 little-endian (GBC native). Each palette's color 0 is
// the "lightest" shade (mapped to font glyph color) and color 3 is the
// "darkest" / most saturated (mapped to label backgrounds, solid fills).
//
// Tier color choices: NYT Connections uses bright, saturated colors that
// are clearly distinguishable. We pick close-to-canonical values that also
// have enough luminance contrast to keep text readable when overlaid.
static const uint16_t gbc_palette_data[6 * 4] = {
    // Palette 0: default greyscale — matches DMG appearance on GBC
    RGB(31, 31, 31), RGB(20, 20, 20), RGB(11, 11, 11), RGB( 0,  0,  0),

    // Palette 1: yellow tier — light cream → golden yellow
    RGB(31, 31, 24), RGB(31, 28, 12), RGB(28, 22,  4), RGB(18, 14,  0),

    // Palette 2: green tier — pale green → leaf green
    RGB(28, 31, 24), RGB(18, 28, 12), RGB( 8, 20,  4), RGB( 2, 12,  0),

    // Palette 3: blue tier — pale sky → deep blue
    RGB(24, 28, 31), RGB(12, 18, 28), RGB( 4,  8, 22), RGB( 0,  2, 14),

    // Palette 4: purple tier — pale lavender → royal purple
    RGB(28, 24, 31), RGB(20, 12, 26), RGB(14,  4, 20), RGB( 8,  0, 12),

    // Palette 5: title banner accent — bright red/orange ("GB" signature color)
    RGB(31, 28, 22), RGB(31, 18,  8), RGB(28,  8,  4), RGB(18,  0,  0),
};

bool is_gbc(void) {
    return _cpu == CGB_TYPE;
}

void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    set_bkg_data(FONT_INV_TILE_BASE, font_inv_TILE_COUNT, font_inv_tiles);
    set_bkg_data(TITLE_TILE_BASE, title_TILE_COUNT, title_tiles);

    // GBC: load all 6 background palettes. set_bkg_palette is a no-op on
    // DMG (checks _cpu internally), so this is safe to call unconditionally.
    set_bkg_palette(0, 6, gbc_palette_data);

    render_clear();
}

void render_clear(void) {
    memset(tilemap_buf, 0, sizeof(tilemap_buf));
    memset(attr_buf, 0, sizeof(attr_buf));
    dirty = 1;
}

void render_text(uint8_t x, uint8_t y, const char *s) {
    if (y >= SCREEN_TILES_H) return;
    uint8_t *dst = &tilemap_buf[y * SCREEN_TILES_W + x];
    while (*s && x < SCREEN_TILES_W) {
        char c = *s;
        if (c < 0x20 || c > 0x5F) c = 0x20;
        *dst++ = (uint8_t)(c - 0x20);
        s++;
        x++;
    }
    dirty = 1;
}

void render_text_inv(uint8_t x, uint8_t y, const char *s) {
    if (y >= SCREEN_TILES_H) return;
    uint8_t *dst = &tilemap_buf[y * SCREEN_TILES_W + x];
    while (*s && x < SCREEN_TILES_W) {
        char c = *s;
        if (c < 0x20 || c > 0x5F) c = 0x20;
        // Same ASCII-to-tile-index mapping as render_text, but offset by
        // FONT_INV_TILE_BASE so we draw from the inverted-palette tile set.
        *dst++ = (uint8_t)(FONT_INV_TILE_BASE + (c - 0x20));
        s++;
        x++;
    }
    dirty = 1;
}

void render_set_tile(uint8_t x, uint8_t y, uint8_t tile) {
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H) return;
    tilemap_buf[y * SCREEN_TILES_W + x] = tile;
    dirty = 1;
}

void render_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette_idx) {
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H) return;
    attr_buf[y * SCREEN_TILES_W + x] = palette_idx & 0x07;
    dirty = 1;
}

void render_flush(void) {
    if (!dirty) return;
    set_bkg_tiles(0, 0, SCREEN_TILES_W, SCREEN_TILES_H, tilemap_buf);
    if (is_gbc()) {
        // Switch to VRAM bank 1 to write tile-attribute data, then back
        // to bank 0 (which holds tile graphics and the regular tilemap).
        // GBDK's set_bkg_tiles writes to whichever bank is currently
        // selected, so the bank switch is essential.
        VBK_REG = 1;
        set_bkg_tiles(0, 0, SCREEN_TILES_W, SCREEN_TILES_H, attr_buf);
        VBK_REG = 0;
    }
    dirty = 0;
}
