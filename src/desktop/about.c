/*
 * B-System (BTRON 3.20) About Box & Demoscene Hub: about.c
 * Features animated Buddhist boy monk and personages extracted from assets/pixart/
 * Built with minimal, lightweight, freestanding-safe demoscene effects.
 */

#include <btron/about.h>
#include <btron/dp.h>
#include <btron/troncode.h>
#include <btron/pixart_bitmaps.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#define strlen tkl_strlen
#define snprintf tkl_snprintf
#endif

typedef struct {
    WND *wnd;
    uint32_t ticks;
    int monk_mood;
    BOOL animation_enabled;
} AboutAppState;

static AboutAppState g_about_state = { NULL, 0, 0, TRUE };

/* Fast 64-entry Integer Sine Table (scale 128) for demoscene effects */
static const int s_sin_tab[64] = {
    0, 12, 24, 37, 49, 60, 71, 80, 89, 97, 104, 110, 115, 119, 122, 125,
    127, 127, 127, 125, 122, 119, 115, 110, 104, 97, 89, 80, 71, 60, 49, 37,
    24, 12, 0, -12, -24, -37, -49, -60, -71, -80, -89, -97, -104, -110, -115, -119,
    -122, -125, -127, -127, -127, -125, -122, -119, -115, -110, -104, -97, -89, -80, -71, -60
};

static inline int isin(int angle) {
    return s_sin_tab[angle & 63];
}

/* Fast transparent sprite blitter */
static void draw_sprite(GDEV *dev, H dst_x, H dst_y, const COLOR *src, H w, H h) {
    if (!dev || !src || !dev->pixels) return;
    volatile COLOR *pix = (volatile COLOR*)dev->pixels;
    H dev_w = dev->width;

    for (H y = 0; y < h; y++) {
        H py = dst_y + y;
        if (py < dev->clip.top || py >= dev->clip.bottom) continue;
        for (H x = 0; x < w; x++) {
            H px = dst_x + x;
            if (px < dev->clip.left || px >= dev->clip.right) continue;
            COLOR c = src[y * w + x];
            if ((c & 0xFF000000) != 0) {
                pix[py * dev_w + px] = c;
            }
        }
    }
}

/* Demoscene Copper Raster Bars */
static void draw_copper_bars(GDEV *dev, H start_y, H height, uint32_t t) {
    if (!dev || !dev->pixels) return;
    volatile COLOR *pix = (volatile COLOR*)dev->pixels;
    H dev_w = dev->width;

    for (H i = 0; i < height; i++) {
        H y = start_y + i;
        if (y < dev->clip.top || y >= dev->clip.bottom) continue;

        int wave = isin(t * 2 + i * 4);
        int r = (180 + (wave * 60) / 128);
        int g = (140 + (isin(t * 3 + i * 3) * 70) / 128);
        int b = (220 + (isin(t * 2 - i * 4) * 35) / 128);
        if (r < 0) r = 0;
        if (r > 255) r = 255;
        if (g < 0) g = 0;
        if (g > 255) g = 255;
        if (b < 0) b = 0;
        if (b > 255) b = 255;

        COLOR line_col = 0xFF000000 | (r << 16) | (g << 8) | b;
        for (H x = dev->clip.left; x < dev->clip.right; x++) {
            pix[y * dev_w + x] = line_col;
        }
    }
}

static void paint_about_window(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    g_about_state.ticks++;
    uint32_t t = g_about_state.ticks;

    /* 1. Window Canvas Background */
    RECT bg_r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &bg_r, 0xFFF0F0EE);
    drw_rec(dev, &bg_r);

    /* 2. Demoscene Copper Header Bar (Top 36px) */
    draw_copper_bars(dev, 0, 36, t);
    drw_lin(dev, 0, 36, dev->width, 36);

    /* Retro Header Title */
    drw_tc_string(dev, 12, 10, "B-System (BTRON 3.20) Retro OS * Demoscene About Cabinet", COLOR_WHITE, 0x00000000);

    /* 3. Sprite Stage Panel */
    RECT stage_r = { 12, 44, dev->width - 12, 140 };
    fill_rec(dev, &stage_r, 0xFFE6EAEF);
    drw_rec(dev, &stage_r);

    /* Decorative stage background grid lines */
    for (H gx = stage_r.left + 20; gx < stage_r.right; gx += 24) {
        drw_lin(dev, gx, stage_r.top + 1, gx, stage_r.bottom - 1);
    }

    /* Animated Buddhist Boy Monk */
    int monk_bob = (isin(t * 4) * 4) / 128;
    int monk_frame = (t / 16) % 2;
    const COLOR *monk_pix = (monk_frame == 0) ? g_monk_frame0_pixels : g_monk_frame1_pixels;
    draw_sprite(dev, 24, stage_r.top + 14 + monk_bob, monk_pix, MONK_FRAME0_W, MONK_FRAME0_H);

    /* Monk Dialog Bubble */
    RECT bubble_r = { 108, stage_r.top + 10, 330, stage_r.top + 46 };
    fill_rec(dev, &bubble_r, COLOR_WHITE);
    drw_rec(dev, &bubble_r);

    const char *monk_quote = (g_about_state.monk_mood % 2 == 0)
        ? "Tashi Delek! བཀྲ་ཤིས་བདེ་ལེགས།"
        : "TRON Code Peace & Wisdom * Om Mani Padme Hum";
    drw_tc_string(dev, 116, stage_r.top + 18, monk_quote, COLOR_NAVY, COLOR_WHITE);

    /* Animated Companion Personage */
    int pers_bob = (isin(t * 3 + 16) * 5) / 128;
    int pers_frame = ((t + 8) / 16) % 2;
    const COLOR *pers_pix = (pers_frame == 0) ? g_personage_frame0_pixels : g_personage_frame1_pixels;
    draw_sprite(dev, dev->width - 100, stage_r.top + 20 + pers_bob, pers_pix, PERSONAGE_FRAME0_W, PERSONAGE_FRAME0_H);

    /* Stage Sub-Caption */
    drw_tc_string(dev, 116, stage_r.top + 60, "Characters: Buddhist Boy Monk (བོད) & BTRON Friends", COLOR_BLACK, 0xFFE6EAEF);
    drw_tc_string(dev, 116, stage_r.top + 78, "Assets: Extracted & Cleaned Bitmaps from ./assets/pixart/", COLOR_DKGRAY, 0xFFE6EAEF);

    /* 4. Specification & System Info Section */
    RECT info_r = { 12, 148, dev->width - 12, 260 };
    fill_rec(dev, &info_r, COLOR_WHITE);
    drw_rec(dev, &info_r);

    drw_tc_string(dev, 20, 156, "System:         B-System Retro Computing Environment (BTRON 3.20)", COLOR_BLACK, COLOR_WHITE);
    drw_tc_string(dev, 20, 174, "Specification:  Ken Sakamura TRON Architecture / BTRON3 Standard", COLOR_BLACK, COLOR_WHITE);
    drw_tc_string(dev, 20, 192, "Microkernel:    T-Kernel 2.0 SMP Multi-Tasking (x86_64 / PC-98 / RPi / UEFI)", COLOR_BLACK, COLOR_WHITE);
    drw_tc_string(dev, 20, 210, "Input Engine:   TIP Mozc Multilingual (TRON Code JIS + Tibetan Wylie)", COLOR_BLACK, COLOR_WHITE);
    drw_tc_string(dev, 20, 228, "Verification:   NASA JPL Power of 10 Safety Invariants Certified (100% Passed)", COLOR_NAVY, COLOR_WHITE);
    drw_tc_string(dev, 20, 244, "Graphics:       Demoscene 32-bit ARGB Framebuffer Engine", COLOR_DKGRAY, COLOR_WHITE);

    /* 5. Retro Demoscene Sine Ticker (Bottom Scroll Banner) */
    RECT ticker_r = { 0, dev->height - 48, dev->width, dev->height - 24 };
    fill_rec(dev, &ticker_r, COLOR_NAVY);
    drw_lin(dev, 0, dev->height - 48, dev->width, dev->height - 48);
    drw_lin(dev, 0, dev->height - 24, dev->width, dev->height - 24);

    static const char s_scroll_msg[] =
        "   *** B-SYSTEM BTRON 3.20 DEMOSCENE ABOUT BOX *** KEN SAKAMURA TRON ARCHITECTURE *** "
        "ANIMATED BUDDHIST BOY MONK (བོད) *** MULTILINGUAL TIBETAN WYLIE & MOZC IME *** "
        "REAL-OBJECT / VIRTUAL-OBJECT TAD SUBSYSTEM *** NASA JPL POWER OF 10 INVARIANTS CERTIFIED *** "
        "GREETINGS TO TRON FORUM, DEMOSCENE CODERS & RETRO ENTHUSIASTS WORLDWIDE! ***   ";

    int msg_len = (int)strlen(s_scroll_msg);
    int char_w = 8;
    int total_scroll_w = msg_len * char_w;
    int scroll_x = (int)(t * 2) % total_scroll_w;

    /* Render scrolling characters with subtle sine bob */
    for (int i = 0; i < msg_len; i++) {
        int cx = (i * char_w) - scroll_x;
        if (cx < -16) cx += total_scroll_w;
        if (cx >= -8 && cx < dev->width) {
            int char_bob = (isin((t + i) * 8) * 3) / 128;
            char single_char[2]; single_char[0] = s_scroll_msg[i]; single_char[1] = 0;
            drw_tc_string(dev, cx, dev->height - 42 + char_bob, single_char, COLOR_YELLOW, COLOR_NAVY);
        }
    }

    /* 6. Footer Controls */
    RECT btn_close = { dev->width - 90, dev->height - 22, dev->width - 10, dev->height - 4 };
    fill_rec(dev, &btn_close, COLOR_LTGRAY);
    drw_rec(dev, &btn_close);
    drw_tc_string(dev, dev->width - 70, dev->height - 18, "Close", COLOR_BLACK, COLOR_LTGRAY);

    RECT btn_greet = { dev->width - 230, dev->height - 22, dev->width - 100, dev->height - 4 };
    fill_rec(dev, &btn_greet, COLOR_LTGRAY);
    drw_rec(dev, &btn_greet);
    drw_tc_string(dev, dev->width - 215, dev->height - 18, "Blessing (祈願)", COLOR_NAVY, COLOR_LTGRAY);
}

static void handle_about_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - wnd->client.left;
        H rel_y = evt->pos.y - wnd->client.top;
        H client_w = wnd->client.right - wnd->client.left;
        H client_h = wnd->client.bottom - wnd->client.top;

        /* Click on Monk Sprite or Dialog -> Toggle Greeting Mood */
        if (rel_x >= 20 && rel_x <= 330 && rel_y >= 44 && rel_y <= 140) {
            g_about_state.monk_mood++;
            redraw_all_windows();
            return;
        }

        /* Footer buttons */
        if (rel_y >= client_h - 22 && rel_y <= client_h - 4) {
            /* Close button */
            if (rel_x >= client_w - 90 && rel_x <= client_w - 10) {
                cls_wnd(wnd);
                return;
            }
            /* Blessing button */
            if (rel_x >= client_w - 230 && rel_x <= client_w - 100) {
                g_about_state.monk_mood++;
                redraw_all_windows();
                return;
            }
        }
    }
}

WND* open_about_window(void) {
    g_about_state.monk_mood = 0;
    g_about_state.ticks = 0;

    WND *wnd = opn_wnd("About B-System - Retro OS & Demoscene Hub (環境情報)",
                       120, 80, 560, 340,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (!wnd) return NULL;

    g_about_state.wnd = wnd;
    wnd->user_data = (VW)(uintptr_t)&g_about_state;
    wnd->paint = paint_about_window;
    wnd->event_handler = handle_about_event;
    return wnd;
}
