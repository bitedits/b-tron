/*
 * B-System (BTRON 3.20) Paint & Vector Drawing (src/apps/paint.c)
 * Geometric Sketching & TAD Graphic Segment Export
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#include <btron/tad_browser.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

typedef struct {
    int tool_type;
    COLOR fg_color;
    COLOR bg_color;
    int stroke_width;
} PaintToolState;
