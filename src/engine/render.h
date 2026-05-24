#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include <stdint.h>

#define SCREEN_TILES_W 20
#define SCREEN_TILES_H 18

// Initialize render subsystem: load font tiles into VRAM, clear tilemap buffer.
// Must be called once at boot before any other render_* function.
void render_init(void);

// Clear the tilemap buffer (does not touch VRAM until next render_flush).
void render_clear(void);

// Write a null-terminated ASCII string starting at tile column x, row y.
// Characters outside printable ASCII (0x20-0x5F) are rendered as space.
// x and y must satisfy 0 <= x < SCREEN_TILES_W, 0 <= y < SCREEN_TILES_H.
void render_text(uint8_t x, uint8_t y, const char *s);

// Push the in-memory tilemap buffer to VRAM. MUST be called only during VBlank.
// Typically called from the VBlank interrupt handler.
void render_flush(void);

#endif
