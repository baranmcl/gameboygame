#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include <stdint.h>

#define SCREEN_TILES_W 20
#define SCREEN_TILES_H 18

// VRAM tile layout:
//   tiles 0..63    = font (ASCII 0x20-0x5F, normal palette)
//   tiles 64..95   = UI chrome (cursor, borders, tier patterns, etc.)
//   tiles 96..159  = font_inv (same glyphs as font, palette inverted —
//                    light glyphs on dark background, for text overlaying
//                    solved bars where normal-font glyphs are invisible)
//   tiles 160+     = available for future use (title art etc.)
#define UI_TILE_BASE          64
#define UI_TILE_CURSOR        (UI_TILE_BASE + 0)
#define UI_TILE_CORNER_TL     (UI_TILE_BASE + 1)
#define UI_TILE_CORNER_TR     (UI_TILE_BASE + 2)
#define UI_TILE_CORNER_BL     (UI_TILE_BASE + 3)
#define UI_TILE_CORNER_BR     (UI_TILE_BASE + 4)
#define UI_TILE_EDGE_HORIZ    (UI_TILE_BASE + 5)
#define UI_TILE_EDGE_VERT     (UI_TILE_BASE + 6)
#define UI_TILE_FILL_LIGHT    (UI_TILE_BASE + 7)
#define UI_TILE_FILL_SEL      (UI_TILE_BASE + 8)
#define UI_TILE_PATTERN_BASE  (UI_TILE_BASE + 9)   // +0..+3 = yellow, green, blue, purple
#define UI_TILE_SOLID_DARK    (UI_TILE_BASE + 13)
#define FONT_INV_TILE_BASE    96

// Initialize render subsystem: load font + UI tiles into VRAM, clear tilemap buffer.
// Must be called once at boot before any other render_* function.
void render_init(void);

// Clear the tilemap buffer (does not touch VRAM until next render_flush).
void render_clear(void);

// Write a null-terminated ASCII string starting at tile column x, row y.
// Characters outside printable ASCII (0x20-0x5F) are rendered as space.
// x and y must satisfy 0 <= x < SCREEN_TILES_W, 0 <= y < SCREEN_TILES_H.
void render_text(uint8_t x, uint8_t y, const char *s);

// Write a null-terminated ASCII string starting at tile column x, row y,
// using the INVERTED font (light glyphs on dark background). Same character
// mapping as render_text. Used for text overlay on dark backgrounds where
// the normal font's dark glyphs would be invisible.
void render_text_inv(uint8_t x, uint8_t y, const char *s);

// Set a single tile by index in the tilemap buffer. For UI chrome
// (cells, bars, borders) where text rendering doesn't apply.
void render_set_tile(uint8_t x, uint8_t y, uint8_t tile);

// Push the in-memory tilemap buffer to VRAM. MUST be called only during VBlank.
// Typically called from the VBlank interrupt handler.
void render_flush(void);

#endif
