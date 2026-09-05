/*
 * ps2_gs.c — Cleanroom Sony PlayStation 2 Graphics Synthesizer Implementation
 *
 * Implements hardware display controller initialization, privileged PCRTC registers,
 * and high-performance Host-to-Local GIF DMA VRAM blitting for PCSX2 and real PS2 hardware.
 *
 * Cleanroom implementation referencing open specifications in third_party/ps2sdk
 * and ps2tek (https://ps2.5ht.co/ps2-hacking.htm).
 * Zero proprietary Sony SDK dependencies.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "ps2_gs.h"

/* Static main memory framebuffer (640 x 448 @ 32-bpp RGBA = 1,146,880 bytes) */
static uint32_t ps2_fb_memory[PS2_SCREEN_WIDTH * PS2_SCREEN_HEIGHT] __attribute__((aligned(128)));

/* EE DMAC Hardware Registers */
#define D_CTRL      (*(volatile uint32_t *)0x1000E000)
#define D_STAT      (*(volatile uint32_t *)0x1000E010)
#define D2_CHCR     (*(volatile uint32_t *)0x1000A000)
#define D2_MADR     (*(volatile uint32_t *)0x1000A010)
#define D2_QWC      (*(volatile uint32_t *)0x1000A020)

/* Helper to get uncached KSEG1 pointer to bypass EE L1 D-Cache */
#define UNCACHED(p) ((void *)((uintptr_t)(p) | 0x20000000UL))

static inline uint32_t *ps2_fb(void)
{
    return (uint32_t *)UNCACHED(ps2_fb_memory);
}

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

/* Stream entire 640x448 RDRAM framebuffer to GS VRAM via Host->Local GIF DMA */
void ps2_gs_flush(void)
{
    /* 1. Send BITBLTBUF Host->Local Blit Setup Packet (5 QWs) */
    static uint64_t setup_pkt[5 * 2] __attribute__((aligned(16)));
    uint64_t *p = (uint64_t *)UNCACHED(setup_pkt);

    /* QW0: GIFTAG (4 registers, EOP=1, FLG=PACKED(0), NREG=1, REGS=A+D(0x0E)) */
    p[0] = (4ULL << 0) | (1ULL << 15) | (0ULL << 58) | (1ULL << 60);
    p[1] = 0x0EULL;

    /* QW1: BITBLTBUF (0x50) - SBA=0, SBW=0, SPSM=0, DBA=0, DBW=10 (640/64), DPSM=0 (CT32) */
    p[2] = ((uint64_t)(PS2_SCREEN_WIDTH / 64) << 48) | ((uint64_t)GS_PSM_CT32 << 56);
    p[3] = 0x50ULL;

    /* QW2: TRXPOS (0x51) - DSAX=0, DSAY=0 */
    p[4] = 0ULL;
    p[5] = 0x51ULL;

    /* QW3: TRXREG (0x52) - RRW=640, RRH=448 */
    p[6] = ((uint64_t)PS2_SCREEN_WIDTH << 0) | ((uint64_t)PS2_SCREEN_HEIGHT << 32);
    p[7] = 0x52ULL;

    /* QW4: TRXDIR (0x53) - Host -> Local (0) */
    p[8] = 0ULL;
    p[9] = 0x53ULL;

    ps2_dma_gif_send(setup_pkt, 5);

    /* 2. Stream entire 640x448 image (71,680 QWs) in 8 chunks of 8,960 QWs */
    /* 640 * 448 * 4 / 16 = 71,680 QWs = 8 chunks * 8,960 QWs */
    #define CHUNK_QWC 8960
    #define NUM_CHUNKS 8

    static uint64_t img_tag[2] __attribute__((aligned(16)));
    uint64_t *tag = (uint64_t *)UNCACHED(img_tag);
    tag[0] = ((uint64_t)CHUNK_QWC << 0) | (1ULL << 15) | (2ULL << 58); /* IMAGE mode, EOP=1 */
    tag[1] = 0ULL;

    const uint8_t *src = (const uint8_t *)UNCACHED(ps2_fb_memory);
    for (int i = 0; i < NUM_CHUNKS; i++) {
        ps2_dma_gif_send(img_tag, 1);
        ps2_dma_gif_send(src + i * (CHUNK_QWC * 16), CHUNK_QWC);
    }
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
     *    field     = 0 (PS2_FIELD)
     */
    ps2_set_gs_crt(1, 2, 0);

    /* 4. Setup PMODE: Circuit 1 + Circuit 2 enabled, SLBG = 0 (Display Framebuffer) */
    GS_REG(GS_PMODE_OFFSET) = 0xFF63ULL;

    /* 5. Setup SMODE2: Interlaced, Field mode */
    GS_REG(GS_SMODE2_OFFSET) = 0x00000001ULL;

    /* 6. Setup DISPFB1 & DISPFB2: FBP=0 (eDRAM base), FBW=10 (640/64), PSM=0 (CT32) */
    uint64_t dispfb = (0ULL << 0) | ((uint64_t)(PS2_SCREEN_WIDTH / 64) << 9) | ((uint64_t)GS_PSM_CT32 << 15);
    GS_REG(GS_DISPFB1_OFFSET) = dispfb;
    GS_REG(GS_DISPFB2_OFFSET) = dispfb;

    /* 7. Setup DISPLAY1 & DISPLAY2: Standard NTSC 640x448 Display Window
     *    DX=636, DY=50, MAGH=3 (4x -> 2560), MAGV=1 (2x -> 448), DW=2559, DH=447
     */
    uint64_t display = 0x001bf9ff0983227cULL;
    GS_REG(GS_DISPLAY1_OFFSET) = display;
    GS_REG(GS_DISPLAY2_OFFSET) = display;

    /* 8. Default Background Color (Deep TRON Navy Blue) */
    ps2_gs_set_bgcolor(24, 48, 80);

    /* 9. Enable EE DMAC Controller */
    D_CTRL |= 1; /* DMAE = 1 */
    D2_CHCR = 0;

    /* 10. Clear RDRAM framebuffer to default background */
    uint32_t *fb = ps2_fb();
    for (int i = 0; i < PS2_SCREEN_WIDTH * PS2_SCREEN_HEIGHT; i++) {
        fb[i] = 0xFF483024; /* TRON Workbench Slate Blue */
    }

    /* 11. Initial hardware flush to GS eDRAM */
    ps2_gs_flush();
}

void ps2_gs_set_bgcolor(uint8_t r, uint8_t g, uint8_t b)
{
    GS_REG(GS_BGCOLOR_OFFSET) = ((uint64_t)r << 0) | ((uint64_t)g << 8) | ((uint64_t)b << 16);
}

void ps2_gs_vsync(void)
{
    /* Clear GS CSR VSync interrupt (bit 3) */
    GS_REG(GS_CSR_OFFSET) = (1ULL << 3);
    for (volatile int i = 0; i < 200000; i++) {
        if (GS_REG(GS_CSR_OFFSET) & (1ULL << 3)) {
            break;
        }
    }
}

void ps2_gs_wait_vsync(void)
{
    ps2_gs_vsync();
}

void ps2_gs_swap_buffers(void)
{
    ps2_gs_wait_vsync();
    ps2_gs_flush();
}

uint32_t *ps2_gs_get_framebuffer(void)
{
    return ps2_fb();
}

void ps2_gs_flip(void)
{
    ps2_gs_wait_vsync();
    ps2_gs_flush();
}

/* Rasterize solid filled 2D rectangle directly into RDRAM backing store */
void ps2_gs_draw_rect(int x, int y, int w, int h, uint32_t color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > PS2_SCREEN_WIDTH)  w = PS2_SCREEN_WIDTH - x;
    if (y + h > PS2_SCREEN_HEIGHT) h = PS2_SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    uint32_t *fb = ps2_fb();
    for (int j = y; j < y + h; j++) {
        uint32_t *row = &fb[j * PS2_SCREEN_WIDTH + x];
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

void ps2_gs_fill_rect(int x, int y, int w, int h, uint32_t color)
{
    ps2_gs_draw_rect(x, y, w, h, color);
}

void ps2_gs_putpixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < PS2_SCREEN_WIDTH && y >= 0 && y < PS2_SCREEN_HEIGHT) {
        ps2_fb()[y * PS2_SCREEN_WIDTH + x] = color;
    }
}

/* Mouse cursor backing store for non-destructive cursor rendering */
static uint32_t cursor_saved[16 * 16];
static int cursor_saved_x = -1;
static int cursor_saved_y = -1;

void ps2_gs_erase_cursor(int x, int y)
{
    (void)x;
    (void)y;
    if (cursor_saved_x < 0 || cursor_saved_y < 0) return;

    uint32_t *fb = ps2_fb();
    for (int cy = 0; cy < 16; cy++) {
        int py = cursor_saved_y + cy;
        if (py >= PS2_SCREEN_HEIGHT) break;
        for (int cx = 0; cx < 16; cx++) {
            int px = cursor_saved_x + cx;
            if (px >= PS2_SCREEN_WIDTH) break;
            fb[py * PS2_SCREEN_WIDTH + px] = cursor_saved[cy * 16 + cx];
        }
    }
    cursor_saved_x = -1;
    cursor_saved_y = -1;
}

void ps2_gs_draw_cursor(int x, int y)
{
    if (cursor_saved_x >= 0) {
        ps2_gs_erase_cursor(0, 0);
    }
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= PS2_SCREEN_WIDTH - 16) x = PS2_SCREEN_WIDTH - 16;
    if (y >= PS2_SCREEN_HEIGHT - 16) y = PS2_SCREEN_HEIGHT - 16;

    uint32_t *fb = ps2_fb();
    for (int cy = 0; cy < 16; cy++) {
        int py = y + cy;
        if (py >= PS2_SCREEN_HEIGHT) break;
        for (int cx = 0; cx < 16; cx++) {
            int px = x + cx;
            if (px >= PS2_SCREEN_WIDTH) break;
            cursor_saved[cy * 16 + cx] = fb[py * PS2_SCREEN_WIDTH + px];
        }
    }
    cursor_saved_x = x;
    cursor_saved_y = y;

    static const uint16_t cursor_mask[16] = {
        0b1000000000000000, 0b1100000000000000, 0b1110000000000000, 0b1111000000000000,
        0b1111100000000000, 0b1111110000000000, 0b1111111000000000, 0b1111111100000000,
        0b1111111110000000, 0b1111110000000000, 0b1101111000000000, 0b1000111100000000,
        0b0000011110000000, 0b0000001111000000, 0b0000000111000000, 0b0000000011000000
    };
    static const uint16_t cursor_outline[16] = {
        0b1100000000000000, 0b1010000000000000, 0b1001000000000000, 0b1000100000000000,
        0b1000010000000000, 0b1000001000000000, 0b1000000100000000, 0b1000000010000000,
        0b1000000001000000, 0b1000011111100000, 0b1101010000000000, 0b0110101000000000,
        0b0000010100000000, 0b0000001010000000, 0b0000000101000000, 0b0000000011000000
    };

    for (int cy = 0; cy < 16; cy++) {
        uint16_t m = cursor_mask[cy];
        uint16_t o = cursor_outline[cy];
        int py = y + cy;
        if (py >= PS2_SCREEN_HEIGHT) break;
        for (int cx = 0; cx < 16; cx++) {
            int px = x + cx;
            if (px >= PS2_SCREEN_WIDTH) break;
            if ((o >> (15 - cx)) & 1) {
                fb[py * PS2_SCREEN_WIDTH + px] = 0xFF000000; /* Outline: Black */
            } else if ((m >> (15 - cx)) & 1) {
                fb[py * PS2_SCREEN_WIDTH + px] = 0xFFFFFFFF; /* Body: White */
            }
        }
    }
}
