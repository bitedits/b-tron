/*
 * ps2_gs.c — Cleanroom Sony PlayStation 2 Graphics Synthesizer Implementation
 *
 * Implements hardware display controller initialization, framebuffer management,
 * and VBlank synchronization for PCSX2 and real PS2 hardware.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "ps2_gs.h"

/* Static main memory framebuffer (640 x 480 @ 32-bpp = 1,228,800 bytes) */
static uint32_t ps2_fb_memory[PS2_SCREEN_WIDTH * PS2_SCREEN_HEIGHT] __attribute__((aligned(128)));

void ps2_gs_init(uint32_t width, uint32_t height)
{
    (void)width;
    (void)height;

    /* 1. Reset GS via CSR */
    GS_REG(GS_CSR_OFFSET) = (1ULL << 9); /* Reset GS */
    for (volatile int i = 0; i < 1000; i++) {}

    /* 2. Setup PMODE: Enable Read Circuit 1, fixed alpha blending */
    /* PMODE:
     *   EN1 = 1 (bit 0)
     *   EN2 = 0 (bit 1)
     *   CRTMD = 1 (bit 2)
     *   MMOD = 1 (bit 3)
     *   AMOD = 1 (bit 4)
     *   SLBG = 0 (bit 5)
     *   ALP = 0xFF (bits 8-15)
     */
    GS_REG(GS_PMODE_OFFSET) = 0x0000FF1DULL;

    /* 3. Setup SMODE2: Non-interlaced, DTV 480p / NTSC progressive */
    /* INT = 0, FFMD = 1, DPMS = 0 */
    GS_REG(GS_SMODE2_OFFSET) = 0x00000002ULL;

    /* 4. Setup DISPFB1:
     *   FBP  = 0 (GS local memory block 0)
     *   FBW  = 640 / 64 = 10 blocks (bits 9-14)
     *   PSM  = 0 (CT32 RGBA32, bits 15-19)
     *   DBX  = 0 (bits 32-42)
     *   DBY  = 0 (bits 43-53)
     */
    uint64_t dispfb = (0ULL << 0) | ((uint64_t)(PS2_SCREEN_WIDTH / 64) << 9) | ((uint64_t)GS_PSM_CT32 << 15);
    GS_REG(GS_DISPFB1_OFFSET) = dispfb;

    /* 5. Setup DISPLAY1:
     *   Standard NTSC / DTV display window coordinates:
     *   DX = 636, DY = 50, MAGH = 3, MAGV = 0, DW = 2559, DH = 479
     */
    uint64_t dx = 636;
    uint64_t dy = 50;
    uint64_t magh = 3;
    uint64_t magv = 0;
    uint64_t dw = 2559;
    uint64_t dh = 479;
    uint64_t display = (dx << 0) | (dy << 12) | (magh << 23) | (magv << 27) | (dw << 32) | (dh << 44);
    GS_REG(GS_DISPLAY1_OFFSET) = display;

    /* 6. Default Background Color (Deep TRON Navy Blue: R=16, G=32, B=64) */
    ps2_gs_set_bgcolor(16, 32, 64);
}

void ps2_gs_set_bgcolor(uint8_t r, uint8_t g, uint8_t b)
{
    GS_REG(GS_BGCOLOR_OFFSET) = ((uint64_t)r << 0) | ((uint64_t)g << 8) | ((uint64_t)b << 16);
}

void ps2_gs_wait_vsync(void)
{
    /* Clear VSINT status bit */
    GS_REG(GS_CSR_OFFSET) = (1ULL << 3);

    /* Poll until VBlank start occurs */
    while (!(GS_REG(GS_CSR_OFFSET) & (1ULL << 3))) {
        /* Busy wait */
    }
}

uint32_t *ps2_gs_get_framebuffer(void)
{
    return ps2_fb_memory;
}

void ps2_gs_flip(void)
{
    /* Wait for vertical retrace */
    ps2_gs_wait_vsync();
}

void ps2_gs_putpixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < PS2_SCREEN_WIDTH && y >= 0 && y < PS2_SCREEN_HEIGHT) {
        ps2_fb_memory[y * PS2_SCREEN_WIDTH + x] = color;
    }
}

void ps2_gs_fill_rect(int x, int y, int w, int h, uint32_t color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > PS2_SCREEN_WIDTH)  w = PS2_SCREEN_WIDTH - x;
    if (y + h > PS2_SCREEN_HEIGHT) h = PS2_SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    for (int j = y; j < y + h; j++) {
        uint32_t *row = &ps2_fb_memory[j * PS2_SCREEN_WIDTH + x];
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}
