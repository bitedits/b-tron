/*
 * ps2_gs.c — Cleanroom Sony PlayStation 2 Graphics Synthesizer Implementation
 *
 * Implements hardware display controller initialization, privileged PCRTC registers,
 * and 2D GIF DMA primitive rasterization for PCSX2 and real PS2 hardware.
 *
 * Cleanroom implementation referencing open specifications in third_party/ps2sdk
 * and ps2tek (https://ps2.5ht.co/ps2-hacking.htm).
 * Zero proprietary Sony SDK dependencies.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "ps2_gs.h"

/* Static main memory framebuffer (640 x 480 @ 32-bpp = 1,228,800 bytes) */
static uint32_t ps2_fb_memory[PS2_SCREEN_WIDTH * PS2_SCREEN_HEIGHT] __attribute__((aligned(128)));

/* EE DMAC Hardware Registers */
#define D_CTRL      (*(volatile uint32_t *)0x1000E000)
#define D_STAT      (*(volatile uint32_t *)0x1000E010)
#define D2_CHCR     (*(volatile uint32_t *)0x1000A000)
#define D2_MADR     (*(volatile uint32_t *)0x1000A010)
#define D2_QWC      (*(volatile uint32_t *)0x1000A020)

/* Helper to get uncached KSEG1 pointer */
#define UNCACHED(p) ((void *)((uintptr_t)(p) | 0x20000000UL))

/* Cleanroom EE BIOS Syscall Prototypes implemented in boot_ps2.s */
extern void ps2_set_gs_crt(int16_t interlace, int16_t pal_ntsc, int16_t field);
extern void ps2_gs_put_imr(uint32_t mask);

/* Transmit packet of quadwords to Graphics Interface (GIF) via DMAC Channel 2 */
static void ps2_dma_gif_send(const void *packet, uint32_t qwc)
{
    /* Clear Channel 2 status bit in D_STAT */
    D_STAT = (1 << 2);

    /* Write physical address to MADR (strip KSEG bits) */
    D2_MADR = ((uint32_t)(uintptr_t)packet) & 0x1FFFFFFF;
    D2_QWC  = qwc;

    __asm__ volatile("" : : : "memory");

    /* Start DMA: direction = 1 (to peripheral/GIF), STR = 1 (bit 8) */
    D2_CHCR = (1 << 0) | (1 << 8);

    /* Wait for transfer to complete (STR bit clears when done) */
    while (D2_CHCR & (1 << 8))
        ;

    __asm__ volatile("" : : : "memory");
}

void ps2_gs_init(uint32_t width, uint32_t height)
{
    (void)width;
    (void)height;

    /* 1. Reset GS via CSR */
    GS_REG(GS_CSR_OFFSET) = (1ULL << 9);
    for (volatile int i = 0; i < 1000; i++) {}

    /* 2. Unmask GS interrupts via BIOS syscall */
    ps2_gs_put_imr(0xFF00);

    /* 3. Configure PCRTC video mode via BIOS SetGsCrt syscall:
     *    interlace = 1 (PS2_INTERLACED)
     *    pal_ntsc  = 2 (PS2_NTSC)
     *    field     = 1 (PS2_FRAME)
     */
    ps2_set_gs_crt(1, 2, 1);

    /* 4. Setup PMODE: Circuit 1 enabled, alpha blending on, show BGCOLOR */
    GS_REG(GS_PMODE_OFFSET) = 0xFFE5ULL;

    /* 5. Setup SMODE2: Interlaced, Frame mode */
    GS_REG(GS_SMODE2_OFFSET) = 0x00000003ULL;

    /* 6. Setup DISPFB1: FBP=0 (eDRAM base), FBW=10 (640/64), PSM=0 (CT32) */
    uint64_t dispfb = (0ULL << 0) | ((uint64_t)(PS2_SCREEN_WIDTH / 64) << 9) | ((uint64_t)GS_PSM_CT32 << 15);
    GS_REG(GS_DISPFB1_OFFSET) = dispfb;

    /* 7. Setup DISPLAY1: Standard NTSC display window */
    uint64_t dx = 656;
    uint64_t dy = 36;
    uint64_t magh = 3;
    uint64_t magv = 0;
    uint64_t dw = 2559;
    uint64_t dh = 479;
    uint64_t display = (dx << 0) | (dy << 12) | (magh << 23) | (magv << 27) | (dw << 32) | (dh << 44);
    GS_REG(GS_DISPLAY1_OFFSET) = display;

    /* 8. Default Background Color (Deep TRON Navy Blue) */
    ps2_gs_set_bgcolor(24, 48, 80);

    /* 9. Enable EE DMAC Controller */
    D_CTRL |= 1; /* DMAE = 1 */
    D2_CHCR = 0;

    /* 10. Send GS Drawing Environment Setup Packet via GIF DMA */
    static uint64_t env_pkt[7 * 2] __attribute__((aligned(16)));
    uint64_t *p = (uint64_t *)UNCACHED(env_pkt);

    /* QW0: GIFTAG (6 registers, EOP=1, FLG=PACKED(0), NREG=1, REG=A+D(0x0E)) */
    p[0] = (6ULL << 0) | (1ULL << 15) | (0ULL << 58) | (1ULL << 60);
    p[1] = 0x0EULL;

    /* QW1: FRAME_1 (0x4C) - FBA=0, FBW=10, PSM=0 (CT32), FMSK=0 */
    p[2] = (0ULL << 0) | ((uint64_t)(PS2_SCREEN_WIDTH / 64) << 16) | (0ULL << 24) | (0ULL << 32);
    p[3] = 0x4CULL;

    /* QW2: ZBUF_1 (0x4E) - ZBA=0, ZSM=0, ZMSK=1 (writes masked/disabled) */
    p[4] = (0ULL << 0) | (0ULL << 24) | (1ULL << 32);
    p[5] = 0x4EULL;

    /* QW3: XYOFFSET_1 (0x18) - Center offset: X = 2048, Y = 2048 (in 16ths of pixel) */
    p[6] = ((uint64_t)(2048 << 4) << 0) | ((uint64_t)(2048 << 4) << 32);
    p[7] = 0x18ULL;

    /* QW4: SCISSOR_1 (0x40) - 0..639, 0..479 */
    p[8] = ((uint64_t)0 << 0) | ((uint64_t)(PS2_SCREEN_WIDTH - 1) << 16) |
           ((uint64_t)0 << 32) | ((uint64_t)(PS2_SCREEN_HEIGHT - 1) << 48);
    p[9] = 0x40ULL;

    /* QW5: PRMODECONT (0x1A) - 1 (Use primitive attributes) */
    p[10] = 1ULL;
    p[11] = 0x1AULL;

    /* QW6: COLCLAMP (0x46) - 1 (Color clamping enabled) */
    p[12] = 1ULL;
    p[13] = 0x46ULL;

    ps2_dma_gif_send(env_pkt, 7);
}

void ps2_gs_set_bgcolor(uint8_t r, uint8_t g, uint8_t b)
{
    GS_REG(GS_BGCOLOR_OFFSET) = ((uint64_t)r << 0) | ((uint64_t)g << 8) | ((uint64_t)b << 16);
}

void ps2_gs_wait_vsync(void)
{
    GS_REG(GS_CSR_OFFSET) = (1ULL << 3);
    for (volatile int i = 0; i < 200000; i++) {
        if (GS_REG(GS_CSR_OFFSET) & (1ULL << 3)) {
            break;
        }
    }
}

uint32_t *ps2_gs_get_framebuffer(void)
{
    return ps2_fb_memory;
}

void ps2_gs_flip(void)
{
    ps2_gs_wait_vsync();
}

/* Rasterize solid filled 2D rectangle via GS PRIM_SPRITE through GIF DMA */
void ps2_gs_draw_rect(int x, int y, int w, int h, uint32_t argb)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > PS2_SCREEN_WIDTH)  w = PS2_SCREEN_WIDTH - x;
    if (y + h > PS2_SCREEN_HEIGHT) h = PS2_SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    /* Keep CPU RDRAM framebuffer synchronized */
    for (int j = y; j < y + h; j++) {
        uint32_t *row = &ps2_fb_memory[j * PS2_SCREEN_WIDTH + x];
        for (int i = 0; i < w; i++) {
            row[i] = argb;
        }
    }

    static uint64_t draw_pkt[3 * 2] __attribute__((aligned(16)));
    uint64_t *p = (uint64_t *)UNCACHED(draw_pkt);

    uint8_t a = (argb >> 24) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8)  & 0xFF;
    uint8_t b = (argb >> 0)  & 0xFF;

    /* QW0: GIFTAG(NLOOP=1, EOP=1, PRE=0, PRIM=0, FLG=REGLIST(1), NREG=4)
     * REGLIST: 0=PRIM (0), 1=RGBAQ (1), 2=XYZ2 (5), 3=XYZ2 (5)
     */
    p[0] = (1ULL << 0) | (1ULL << 15) | (1ULL << 58) | (4ULL << 60);
    p[1] = (0x00ULL << 0) | (0x01ULL << 4) | (0x05ULL << 8) | (0x05ULL << 12);

    /* QW1: PRIM (dw0) + RGBAQ (dw1) */
    p[2] = 6ULL; /* PRIM_SPRITE */
    p[3] = (uint64_t)r | ((uint64_t)g << 8) | ((uint64_t)b << 16) | ((uint64_t)a << 24) | (0x3F800000ULL << 32);

    /* QW2: XYZ2 v0 (dw0) + XYZ2 v1 (dw1) (in 16ths of pixel, offset by 2048) */
    int x0 = (x + 2048) << 4;
    int y0 = (y + 2048) << 4;
    int x1 = (x + w + 2048) << 4;
    int y1 = (y + h + 2048) << 4;
    p[4] = ((uint64_t)x0 << 0) | ((uint64_t)y0 << 16);
    p[5] = ((uint64_t)x1 << 0) | ((uint64_t)y1 << 16);

    ps2_dma_gif_send(draw_pkt, 3);
}

void ps2_gs_fill_rect(int x, int y, int w, int h, uint32_t color)
{
    ps2_gs_draw_rect(x, y, w, h, color);
}

void ps2_gs_putpixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < PS2_SCREEN_WIDTH && y >= 0 && y < PS2_SCREEN_HEIGHT) {
        ps2_fb_memory[y * PS2_SCREEN_WIDTH + x] = color;
        ps2_gs_draw_rect(x, y, 1, 1, color);
    }
}

/* Draw 16x16 standard B-System arrow cursor */
void ps2_gs_draw_cursor(int x, int y)
{
    static const uint16_t cursor_mask[16] = {
        0b1000000000000000,
        0b1100000000000000,
        0b1110000000000000,
        0b1111000000000000,
        0b1111100000000000,
        0b1111110000000000,
        0b1111111000000000,
        0b1111111100000000,
        0b1111111110000000,
        0b1111110000000000,
        0b1101111000000000,
        0b1000111100000000,
        0b0000011110000000,
        0b0000001111000000,
        0b0000000111000000,
        0b0000000011000000
    };

    static const uint16_t cursor_outline[16] = {
        0b1100000000000000,
        0b1010000000000000,
        0b1001000000000000,
        0b1000100000000000,
        0b1000010000000000,
        0b1000001000000000,
        0b1000000100000000,
        0b1000000010000000,
        0b1000000001000000,
        0b1000011111100000,
        0b1101010000000000,
        0b0110101000000000,
        0b0000010100000000,
        0b0000001010000000,
        0b0000000101000000,
        0b0000000011000000
    };

    for (int cy = 0; cy < 16; cy++) {
        uint16_t m = cursor_mask[cy];
        uint16_t o = cursor_outline[cy];
        for (int cx = 0; cx < 16; cx++) {
            if ((o >> (15 - cx)) & 1) {
                ps2_gs_draw_rect(x + cx, y + cy, 1, 1, 0xFF000000);
            } else if ((m >> (15 - cx)) & 1) {
                ps2_gs_draw_rect(x + cx, y + cy, 1, 1, 0xFFFFFFFF);
            }
        }
    }
}
