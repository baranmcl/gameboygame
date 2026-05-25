#include "render.h"
#include "../assets_gen/font.h"
#include "../assets_gen/ui_tiles.h"
#include "../assets_gen/font_inv.h"
#include <gb/gb.h>
#include <string.h>

static uint8_t tilemap_buf[SCREEN_TILES_W * SCREEN_TILES_H];
static uint8_t dirty = 0;

void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    set_bkg_data(FONT_INV_TILE_BASE, font_inv_TILE_COUNT, font_inv_tiles);
    render_clear();
}

void render_clear(void) {
    memset(tilemap_buf, 0, sizeof(tilemap_buf));
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

void render_flush(void) {
    if (!dirty) return;
    set_bkg_tiles(0, 0, SCREEN_TILES_W, SCREEN_TILES_H, tilemap_buf);
    dirty = 0;
}
