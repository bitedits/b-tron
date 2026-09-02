/*
 * vesa.h — VESA BIOS Extensions (VBE 2.0/3.0) & Bochs DISPI Linear Framebuffer Driver
 * Cleanroom implementation for QEMU x86_64, Bochs, and bare-metal PC/Q35.
 */

#ifndef BTRON_DRIVERS_VESA_H
#define BTRON_DRIVERS_VESA_H

#include <stdint.h>
#include <stddef.h>

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF

#define VBE_DISPI_INDEX_ID          0x0
#define VBE_DISPI_INDEX_XRES        0x1
#define VBE_DISPI_INDEX_YRES        0x2
#define VBE_DISPI_INDEX_BPP         0x3
#define VBE_DISPI_INDEX_ENABLE      0x4
#define VBE_DISPI_INDEX_BANK        0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH  0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET    0x8
#define VBE_DISPI_INDEX_Y_OFFSET    0x9

#define VBE_DISPI_DISABLED          0x00
#define VBE_DISPI_ENABLED           0x01
#define VBE_DISPI_LFB_ENABLED       0x40
#define VBE_DISPI_NOCLEARMEM        0x80

/* Color Palette (32-bit ARGB) */
#define VESA_COLOR_BG          0x002B4C7E /* Classic BTRON Indigo Desktop */
#define VESA_COLOR_WHITE       0x00FFFFFF
#define VESA_COLOR_BLACK       0x00000000
#define VESA_COLOR_GRAY        0x00C6C6C6 /* Standard Bevel Gray */
#define VESA_COLOR_DKGRAY      0x00808080
#define VESA_COLOR_LTGRAY      0x00EFEFEF
#define VESA_COLOR_TITLE_ACT   0x001B3A68 /* Active Window Title Blue */
#define VESA_COLOR_TITLE_INACT 0x00707070
#define VESA_COLOR_YELLOW      0x00FFF200
#define VESA_COLOR_GREEN       0x0000CC66
#define VESA_COLOR_CYAN        0x0000D0FF
#define VESA_COLOR_RED         0x00EE3333
#define VESA_COLOR_SHADOW      0x00152438

typedef struct {
    uint16_t  width;
    uint16_t  height;
    uint16_t  bpp;
    uint32_t *framebuffer;
    uint8_t   is_active;
} vesa_info_t;

extern vesa_info_t g_vesa;

int  vesa_init(uint16_t width, uint16_t height, uint16_t bpp);
void vesa_restore_text(void);
void vesa_put_pixel(int x, int y, uint32_t color);
void vesa_fill_rect(int x, int y, int w, int h, uint32_t color);
void vesa_draw_rect(int x, int y, int w, int h, uint32_t color);
void vesa_draw_bevel_rect(int x, int y, int w, int h, int raised);
void vesa_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void vesa_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg);
void vesa_draw_window(int x, int y, int w, int h, const char *title, int focused);
void vesa_render_desktop(int num_cores);

#endif /* BTRON_DRIVERS_VESA_H */
