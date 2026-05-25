#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include <stdint.h>
#include <stdbool.h>

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
// Plan C addition: title banner (~32 tiles) loaded by render_init.
// Tiles arranged 8 wide × 4 tall in title.png; tile index N maps to
// banner cell (N % 8, N / 8) relative to the banner's top-left.
// (User-driven retitle "GBCX" → "GB" mid-Plan C bumped letters from
// 2× to 4× scale, hence the larger 8×4 tile layout.)
#define TITLE_TILE_BASE       160
#define TITLE_BANNER_W        8
#define TITLE_BANNER_H        4

// GBC background palette indices. Used as the `palette_idx` argument
// to render_set_tile_palette(). On DMG these constants are ignored.
//
// Palette 0: default greyscale-equivalent (white/light/dark/black).
//   Used for all text, UI chrome, and unspecified tiles — including
//   the title banner (we intentionally leave it greyscale on GBC too;
//   the in-game tier colors are the visual signature).
// Palettes 1-4: NYT tier colors (yellow / green / blue / purple).
//   Used by render_solved_bar in scenes for the colored tier bands.
#define GBC_PAL_DEFAULT     0
#define GBC_PAL_TIER_YELLOW 1
#define GBC_PAL_TIER_GREEN  2
#define GBC_PAL_TIER_BLUE   3
#define GBC_PAL_TIER_PURPLE 4

// Returns true if running on a Game Boy Color (or compatible), false on
// original Game Boy (DMG) or Pocket (MGB). All color-code paths in this
// engine check this and no-op on DMG.
//
// Uses GBDK's `_cpu` global: 0x11 = CGB, 0x01 = DMG, 0xFF = MGB.
bool is_gbc(void);

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

// Set the GBC background palette index for a single tile. On DMG this
// is a no-op (DMG has no per-tile palettes). On GBC, color rendering of
// the tile at (x, y) uses palette `palette_idx` (one of the GBC_PAL_*
// constants above). Must be paired with a corresponding render_set_tile
// or render_text* call to actually write tile data at that cell —
// palette assignment alone doesn't draw anything.
void render_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette_idx);

// Push the in-memory tilemap buffer to VRAM. MUST be called only during VBlank.
// Typically called from the VBlank interrupt handler.
void render_flush(void);

#endif
