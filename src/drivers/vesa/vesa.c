/*
 * vesa.c — VESA VBE 2.0/3.0 & Bochs DISPI Linear Framebuffer Driver
 * High-performance graphical engine for BTRON3 Workstation GUI on QEMU / Baremetal x86_64.
 */

#include <drivers/vesa.h>
#include <libstr.h>

vesa_info_t g_vesa = {0, 0, 0, NULL, 0};

static inline void outw_v(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw_v(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl_v(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl_v(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void vbe_write(uint16_t index, uint16_t val) {
    outw_v(VBE_DISPI_IOPORT_INDEX, index);
    outw_v(VBE_DISPI_IOPORT_DATA, val);
}

static uint32_t pci_scan_vga_lfb(void) {
    /* Scan PCI bus 0-3 for display controller (Class 0x03) */
    for (uint32_t bus = 0; bus < 4; bus++) {
        for (uint32_t slot = 0; slot < 32; slot++) {
            for (uint32_t func = 0; func < 8; func++) {
                uint32_t addr = (1U << 31) | (bus << 16) | (slot << 11) | (func << 8) | 0x08;
                outl_v(0x0CF8, addr);
                uint32_t class_reg = inl_v(0x0CFC);
                if ((class_reg >> 24) == 0x03) { /* Display class */
                    /* Enable PCI Memory Space, I/O Space, and Bus Master */
                    uint32_t cmd_addr = (1U << 31) | (bus << 16) | (slot << 11) | (func << 8) | 0x04;
                    outl_v(0x0CF8, cmd_addr);
                    uint32_t cmd_val = inl_v(0x0CFC);
                    outl_v(0x0CF8, cmd_addr);
                    outl_v(0x0CFC, cmd_val | 0x07);

                    /* Read BAR0 */
                    uint32_t bar_addr = (1U << 31) | (bus << 16) | (slot << 11) | (func << 8) | 0x10;
                    outl_v(0x0CF8, bar_addr);
                    uint32_t bar0 = inl_v(0x0CFC) & 0xFFFFFFF0;
                    if (bar0 != 0 && bar0 != 0xFFFFFFF0) return bar0;
                }
            }
        }
    }
    return 0xFD000000; /* Standard Q35 PCI BAR0 Fallback */
}

int vesa_init(uint16_t width, uint16_t height, uint16_t bpp) {
    uint32_t lfb_addr = pci_scan_vga_lfb();

    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, width);
    vbe_write(VBE_DISPI_INDEX_YRES, height);
    vbe_write(VBE_DISPI_INDEX_BPP, bpp);
    vbe_write(VBE_DISPI_INDEX_VIRT_WIDTH, width);
    vbe_write(VBE_DISPI_INDEX_VIRT_HEIGHT, height);
    vbe_write(VBE_DISPI_INDEX_X_OFFSET, 0);
    vbe_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    g_vesa.width = width;
    g_vesa.height = height;
    g_vesa.bpp = bpp;
    g_vesa.framebuffer = (uint32_t *)(uintptr_t)lfb_addr;
    g_vesa.is_active = 1;

    return 0;
}

void vesa_restore_text(void) {
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    g_vesa.is_active = 0;
}

void vesa_put_pixel(int x, int y, uint32_t color) {
    if (!g_vesa.is_active || !g_vesa.framebuffer) return;
    if (x < 0 || x >= (int)g_vesa.width || y < 0 || y >= (int)g_vesa.height) return;
    g_vesa.framebuffer[y * g_vesa.width + x] = color;
}

void vesa_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!g_vesa.is_active || !g_vesa.framebuffer) return;
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > (int)g_vesa.width) ? (int)g_vesa.width : x + w;
    int y1 = (y + h > (int)g_vesa.height) ? (int)g_vesa.height : y + h;

    for (int cy = y0; cy < y1; cy++) {
        volatile uint32_t *row = &g_vesa.framebuffer[cy * g_vesa.width];
        for (int cx = x0; cx < x1; cx++) {
            row[cx] = color;
        }
    }
}

void vesa_draw_rect(int x, int y, int w, int h, uint32_t color) {
    vesa_fill_rect(x, y, w, 1, color);
    vesa_fill_rect(x, y + h - 1, w, 1, color);
    vesa_fill_rect(x, y, 1, h, color);
    vesa_fill_rect(x + w - 1, y, 1, h, color);
}

void vesa_draw_bevel_rect(int x, int y, int w, int h, int raised) {
    uint32_t top_left = raised ? VESA_COLOR_WHITE : VESA_COLOR_DKGRAY;
    uint32_t bot_right = raised ? VESA_COLOR_DKGRAY : VESA_COLOR_WHITE;

    vesa_fill_rect(x, y, w, 1, top_left);
    vesa_fill_rect(x, y, 1, h, top_left);
    vesa_fill_rect(x, y + h - 1, w, 1, bot_right);
    vesa_fill_rect(x + w - 1, y, 1, h, bot_right);
}

/* 8x16 Basic Glyph Engine */
static const uint8_t font_8x16_basic[128][16] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['A'] = {0x00,0x00,0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00},
    ['B'] = {0x00,0x00,0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['C'] = {0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['D'] = {0x00,0x00,0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['E'] = {0x00,0x00,0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['F'] = {0x00,0x00,0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['G'] = {0x00,0x00,0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['H'] = {0x00,0x00,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['I'] = {0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['K'] = {0x00,0x00,0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['L'] = {0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['M'] = {0x00,0x00,0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['N'] = {0x00,0x00,0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['O'] = {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['P'] = {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['R'] = {0x00,0x00,0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['S'] = {0x00,0x00,0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['T'] = {0x00,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['U'] = {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['V'] = {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['W'] = {0x00,0x00,0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['X'] = {0x00,0x00,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['Y'] = {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['Z'] = {0x00,0x00,0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['0'] = {0x00,0x00,0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['1'] = {0x00,0x00,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['2'] = {0x00,0x00,0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['3'] = {0x00,0x00,0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['4'] = {0x00,0x00,0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['5'] = {0x00,0x00,0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['6'] = {0x00,0x00,0x3C,0x60,0x7C,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['7'] = {0x00,0x00,0x7E,0x06,0x0C,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['8'] = {0x00,0x00,0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['9'] = {0x00,0x00,0x3C,0x66,0x66,0x3E,0x06,0x06,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    [':'] = {0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['-'] = {0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['['] = {0x00,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    [']'] = {0x00,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['#'] = {0x00,0x00,0x24,0x24,0x7E,0x24,0x7E,0x24,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

void vesa_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    if ((uint8_t)c >= 128) c = ' ';
    const uint8_t *glyph = font_8x16_basic[(uint8_t)c];

    for (int r = 0; r < 16; r++) {
        uint8_t bits = glyph[r];
        for (int b = 0; b < 8; b++) {
            uint32_t col = (bits & (0x80 >> b)) ? fg : bg;
            if (col != 0xFF000000) {
                vesa_put_pixel(x + b, y + r, col);
            }
        }
    }
}

void vesa_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg) {
    int cx = x;
    while (*str) {
        char c = *str++;
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        vesa_draw_char(cx, y, c, fg, bg);
        cx += 8;
    }
}

void vesa_draw_window(int x, int y, int w, int h, const char *title, int focused) {
    /* Shadow */
    vesa_fill_rect(x + 4, y + 4, w, h, VESA_COLOR_SHADOW);

    /* Frame */
    vesa_fill_rect(x, y, w, h, VESA_COLOR_GRAY);
    vesa_draw_bevel_rect(x, y, w, h, 1);

    /* Title */
    uint32_t tcol = focused ? VESA_COLOR_TITLE_ACT : VESA_COLOR_TITLE_INACT;
    vesa_fill_rect(x + 3, y + 3, w - 6, 22, tcol);
    vesa_draw_string(x + 8, y + 6, title, VESA_COLOR_WHITE, tcol);

    /* Close */
    vesa_fill_rect(x + w - 22, y + 5, 16, 16, VESA_COLOR_GRAY);
    vesa_draw_bevel_rect(x + w - 22, y + 5, 16, 16, 1);
    vesa_draw_char(x + w - 18, y + 5, 'X', VESA_COLOR_BLACK, VESA_COLOR_GRAY);

    /* Inner canvas */
    vesa_fill_rect(x + 4, y + 28, w - 8, h - 32, VESA_COLOR_WHITE);
    vesa_draw_bevel_rect(x + 4, y + 28, w - 8, h - 32, 0);
}

void vesa_render_desktop(int num_cores) {
    if (!g_vesa.is_active) return;

    /* Authentic BTRON Teal background */
    vesa_fill_rect(0, 0, g_vesa.width, g_vesa.height, 0x00008080);

    /* Top menu bar */
    vesa_fill_rect(0, 0, g_vesa.width, 26, VESA_COLOR_GRAY);
    vesa_draw_bevel_rect(0, 0, g_vesa.width, 26, 1);

    vesa_draw_string(10, 5, "BTRON3 3.20", VESA_COLOR_BLACK, VESA_COLOR_GRAY);
    vesa_draw_string(120, 5, "[HFDS]", VESA_COLOR_BLACK, VESA_COLOR_GRAY);
    vesa_draw_string(180, 5, "[EDIT]", VESA_COLOR_BLACK, VESA_COLOR_GRAY);
    vesa_draw_string(240, 5, "[VIEW]", VESA_COLOR_BLACK, VESA_COLOR_GRAY);
    vesa_draw_string(300, 5, "[SYS]", VESA_COLOR_BLACK, VESA_COLOR_GRAY);
    vesa_draw_string(360, 5, "[TERM]", VESA_COLOR_BLACK, VESA_COLOR_GRAY);

    char core_buf[32];
    core_buf[0] = 'S'; core_buf[1] = 'M'; core_buf[2] = 'P'; core_buf[3] = ':';
    core_buf[4] = ' '; core_buf[5] = (char)('0' + num_cores);
    core_buf[6] = ' '; core_buf[7] = 'C'; core_buf[8] = 'O'; core_buf[9] = 'R'; core_buf[10] = 'E'; core_buf[11] = 'S'; core_buf[12] = '\0';
    vesa_draw_string(880, 5, core_buf, VESA_COLOR_TITLE_ACT, VESA_COLOR_GRAY);

    /* Window 1: Cabinet Explorer */
    vesa_draw_window(40, 60, 360, 280, "HFDS CABINET - ROOT", 0);
    vesa_fill_rect(48, 92, 344, 240, 0x00F8F9FA);
    vesa_draw_string(60, 110, "[VOBJ] 01_BTRON3_SPEC.TAD", VESA_COLOR_BLACK, 0x00F8F9FA);
    vesa_draw_string(60, 135, "[VOBJ] 02_T_KERNEL_20.TAD", VESA_COLOR_BLACK, 0x00F8F9FA);
    vesa_draw_string(60, 160, "[VOBJ] 03_MOZC_DICT.DAT", VESA_COLOR_BLACK, 0x00F8F9FA);
    vesa_draw_string(60, 185, "[VOBJ] 04_SKI_BOOTMAN.SYS", VESA_COLOR_BLACK, 0x00F8F9FA);

    /* Window 2: GTerm Terminal */
    vesa_draw_window(440, 80, 520, 360, "GTERM - TERMINAL CONSOLE", 1);
    vesa_fill_rect(448, 112, 504, 320, 0x00101820);
    vesa_draw_string(460, 125, "B-SYSTEM BTRON3 3.20 (X86_64 UEFI SMP)", VESA_COLOR_GREEN, 0x00101820);
    vesa_draw_string(460, 145, "ACPI 6.5 MADT ONLINE - 4 CORES SCHEDULED", VESA_COLOR_CYAN, 0x00101820);
    vesa_draw_string(460, 175, "BTRON3# PS", VESA_COLOR_WHITE, 0x00101820);
    vesa_draw_string(460, 195, "PID  CORE  TASK         STAT   ADDR", VESA_COLOR_YELLOW, 0x00101820);
    vesa_draw_string(460, 215, "  1  #0    TK_DESKTOP   RUN    0X01020000", VESA_COLOR_WHITE, 0x00101820);
    vesa_draw_string(460, 235, "  2  #1    TK_WND_MGR   READY  0X01040000", VESA_COLOR_WHITE, 0x00101820);
    vesa_draw_string(460, 255, "  3  #2    TK_TIP_IME   READY  0X010A0000", VESA_COLOR_WHITE, 0x00101820);
    vesa_draw_string(460, 275, "  4  #3    GTERM#1      RUN    0X010C0000", VESA_COLOR_GREEN, 0x00101820);
    vesa_draw_string(460, 310, "BTRON3# _", VESA_COLOR_YELLOW, 0x00101820);

    /* Window 3: T-Editor */
    vesa_draw_window(70, 380, 340, 260, "T-EDITOR - RELEASE NOTES", 0);
    vesa_fill_rect(78, 412, 324, 220, VESA_COLOR_WHITE);
    vesa_draw_string(90, 430, "WELCOME TO B-SYSTEM 3.20!", VESA_COLOR_TITLE_ACT, VESA_COLOR_WHITE);
    vesa_draw_string(90, 460, "VESA VBE 1024X768 32-BPP LFB", VESA_COLOR_BLACK, VESA_COLOR_WHITE);
    vesa_draw_string(90, 485, "CLEANROOM SMP + LAPIC ENGINE", VESA_COLOR_BLACK, VESA_COLOR_WHITE);
    vesa_draw_string(90, 510, "SAKAMURA TRON SPEC COMPLIANT", VESA_COLOR_BLACK, VESA_COLOR_WHITE);

    /* Taskbar */
    vesa_fill_rect(0, g_vesa.height - 36, g_vesa.width, 36, VESA_COLOR_GRAY);
    vesa_draw_bevel_rect(0, g_vesa.height - 36, g_vesa.width, 36, 1);

    vesa_fill_rect(6, g_vesa.height - 30, 80, 24, VESA_COLOR_LTGRAY);
    vesa_draw_bevel_rect(6, g_vesa.height - 30, 80, 24, 1);
    vesa_draw_string(16, g_vesa.height - 26, "[TRON]", VESA_COLOR_TITLE_ACT, VESA_COLOR_LTGRAY);

    vesa_fill_rect(95, g_vesa.height - 30, 130, 24, VESA_COLOR_LTGRAY);
    vesa_draw_bevel_rect(95, g_vesa.height - 30, 130, 24, 0);
    vesa_draw_string(105, g_vesa.height - 26, "GTERM CONSOLE", VESA_COLOR_BLACK, VESA_COLOR_LTGRAY);

    vesa_fill_rect(235, g_vesa.height - 30, 130, 24, VESA_COLOR_LTGRAY);
    vesa_draw_bevel_rect(235, g_vesa.height - 30, 130, 24, 1);
    vesa_draw_string(245, g_vesa.height - 26, "HFDS CABINET", VESA_COLOR_BLACK, VESA_COLOR_LTGRAY);

    vesa_fill_rect(g_vesa.width - 240, g_vesa.height - 30, 232, 24, VESA_COLOR_LTGRAY);
    vesa_draw_bevel_rect(g_vesa.width - 240, g_vesa.height - 30, 232, 24, 0);
    vesa_draw_string(g_vesa.width - 230, g_vesa.height - 26, "[MOZC: KANA]  12:30 PM", VESA_COLOR_BLACK, VESA_COLOR_LTGRAY);
}
