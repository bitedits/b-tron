/*
 * B-TRON Specification Compatible Header: dp.h
 * Display Primitives (Graphics Engine) Header.
 */

#ifndef _BTRON_DP_H_
#define _BTRON_DP_H_

#include <btron/types.h>
#include <btron/error.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#define uart_puts(s) printf("%s", (s))
#else
void uart_puts(const char *s);
#endif

/* Raster Operations (ROP) */
#define ROP_COPY   0
#define ROP_OR     1
#define ROP_XOR    2
#define ROP_AND    3
#define ROP_INVERT 4

/* TRON Standard Palette Colors */
#define COLOR_BLACK     0x000000FF
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_DKGRAY    0x404040FF
#define COLOR_GRAY      0x808080FF
#define COLOR_LTGRAY    0xC0C0C0FF
#define COLOR_TEAL      0x008080FF   /* Classic BTRON Desktop Background */
#define COLOR_NAVY      0x000080FF   /* Active Title Bar */
#define COLOR_YELLOW    0xFFFF00FF
#define COLOR_RED       0xFF0000FF
#define COLOR_GREEN     0x00FF00FF
#define COLOR_CYAN      0x00FFFFFF

typedef struct {
    H width;
    H height;
    UW pad0;
    COLOR *pixels;
    RECT clip;
} GDEV;

/* Display Primitive Operations */
GDEV* opn_dev(H w, H h);
GDEV* opn_dev_vram(H w, H h, COLOR *vram_buffer);
void  cls_dev(GDEV *dev);

void  set_col(GDEV *dev, COLOR fg, COLOR bg);
void  set_pat(GDEV *dev, const PAT *pat);
void  set_clip(GDEV *dev, const RECT *clip);

ER    drw_pnt(GDEV *dev, H x, H y);
ER    drw_lin(GDEV *dev, H x1, H y1, H x2, H y2);
ER    drw_rec(GDEV *dev, const RECT *r);
ER    fill_rec(GDEV *dev, const RECT *r, COLOR col);
ER    drw_ovl(GDEV *dev, const RECT *r);
ER    fill_ovl(GDEV *dev, const RECT *r, COLOR col);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_DP_H_ */
