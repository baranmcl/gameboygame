// src/main.c
#include <gb/gb.h>
#include <stdint.h>

void main(void) {
    // Set the background palette to standard 4-shade
    BGP_REG = 0xE4;  // 11 10 01 00 — dark to light

    // For now, just display a banner via SCREEN_ON; later phases hook in real rendering.
    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();
    }
}
