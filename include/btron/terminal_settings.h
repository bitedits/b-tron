/*
 * B-TRON Terminal Settings Master Header: terminal_settings.h
 * Global console configuration for gterm and Terminal Settings Applet
 */

#ifndef _BTRON_TERMINAL_SETTINGS_H_
#define _BTRON_TERMINAL_SETTINGS_H_

#include <btron/types.h>
#include <btron/dp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TERM_THEME_GREEN = 0,   /* Matrix Green on Black */
    TERM_THEME_AMBER,       /* Amber CRT on Black */
    TERM_THEME_WHITE,       /* High-Contrast White on Black */
    TERM_THEME_CYAN,        /* BTRON Cyan on Deep Navy */
    TERM_THEME_LIGHT        /* Paper Light (Black on Light Gray) */
} TERM_COLOR_THEME;

typedef enum {
    TERM_FONT_12 = 12,      /* Compact (32 rows x 100 cols) */
    TERM_FONT_16 = 16,      /* Standard (24 rows x 80 cols) */
    TERM_FONT_20 = 20       /* Large (18 rows x 64 cols) */
} TERM_FONT_SIZE;

typedef enum {
    TERM_CURSOR_UNDERLINE = 0,  /* Classic Underline '_' */
    TERM_CURSOR_BLOCK     = 1,  /* Solid VT100 Block '█' */
    TERM_CURSOR_BAR       = 2   /* Modern Vertical Bar '|' */
} TERM_CURSOR_STYLE;

typedef enum {
    TERM_TRANSPARENCY_OPAQUE = 0, /* 100% Opaque */
    TERM_TRANSPARENCY_80     = 1, /* 80% Dimmed (Default) */
    TERM_TRANSPARENCY_60     = 2  /* 60% Dimmed */
} TERM_TRANSPARENCY;

typedef struct {
    TERM_COLOR_THEME  theme;
    COLOR             fg_color;
    COLOR             bg_color;
    TERM_FONT_SIZE    font_size;
    int               scrollback_lines;
    TERM_CURSOR_STYLE cursor_style;
    TERM_TRANSPARENCY transparency;
} TERMINAL_SETTINGS;

void terminal_get_settings(TERMINAL_SETTINGS *out);
void terminal_set_settings(const TERMINAL_SETTINGS *in);
void terminal_reset_settings(void);

COLOR terminal_get_effective_bg(const TERMINAL_SETTINGS *st);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TERMINAL_SETTINGS_H_ */
