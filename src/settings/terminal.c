/*
 * B-System (BTRON 3.20) Settings Applet: terminal (Terminal / 端末・通信)
 * Conforming to ./b-system/settings/Terminal.html and B-right/V / Chokanji specs.
 */

#include <btron/settings.h>
#include <btron/terminal_settings.h>
#include <btron/dp.h>
#include <btron/troncode.h>
#include <btron/wnd.h>
#include <btron/settings_icon.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#define memset tkl_memset
#define strlen tkl_strlen
#define snprintf tkl_snprintf
#endif

static TERMINAL_SETTINGS g_term_settings = {
    .theme            = TERM_THEME_WHITE,
    .fg_color         = 0xFFFFFFFF,
    .bg_color         = 0xCC000000,
    .font_size        = TERM_FONT_16,
    .scrollback_lines = 300,
    .cursor_style     = TERM_CURSOR_UNDERLINE,
    .transparency     = TERM_TRANSPARENCY_80
};

COLOR terminal_get_effective_bg(const TERMINAL_SETTINGS *st) {
    if (!st) return 0xCC000000;
    uint32_t alpha = 0xCC;
    if (st->transparency == TERM_TRANSPARENCY_OPAQUE) alpha = 0xFF;
    else if (st->transparency == TERM_TRANSPARENCY_60) alpha = 0x99;

    uint32_t base_rgb = 0x000000;
    if (st->theme == TERM_THEME_CYAN)  base_rgb = 0x0A1120;
    else if (st->theme == TERM_THEME_LIGHT) base_rgb = 0xE2E8F0;

    return (alpha << 24) | (base_rgb & 0x00FFFFFF);
}

void terminal_get_settings(TERMINAL_SETTINGS *out) {
    if (!out) return;
    *out = g_term_settings;
}

void terminal_set_settings(const TERMINAL_SETTINGS *in) {
    if (!in) return;
    g_term_settings = *in;
    g_term_settings.bg_color = terminal_get_effective_bg(&g_term_settings);
}

void terminal_reset_settings(void) {
    g_term_settings.theme = TERM_THEME_WHITE;
    g_term_settings.fg_color = 0xFFFFFFFF;
    g_term_settings.bg_color = 0xCC000000;
    g_term_settings.font_size = TERM_FONT_16;
    g_term_settings.scrollback_lines = 300;
    g_term_settings.cursor_style = TERM_CURSOR_UNDERLINE;
    g_term_settings.transparency = TERM_TRANSPARENCY_80;
}

typedef struct {
    WND *wnd;
    TERMINAL_SETTINGS local_cfg;
    BOOL is_dirty;
} AppletState_terminal;

static AppletState_terminal g_state_terminal;

/* 3D Crisp Graphical Radio Button */
static void paint_ui_radio(GDEV *dev, H x, H y, const char *label, BOOL checked, BOOL focused) {
    RECT box = { x, y + 1, x + 15, y + 16 };
    fill_rec(dev, &box, COLOR_WHITE);
    drw_rec(dev, &box);

    if (checked) {
        RECT dot = { x + 4, y + 5, x + 11, y + 12 };
        fill_rec(dev, &dot, COLOR_NAVY);
    }

    COLOR text_col = focused ? COLOR_NAVY : COLOR_BLACK;
    drw_tc_string(dev, x + 22, y, label, text_col, COLOR_WHITE);
}

/* 3D Push Button */
static void paint_ui_button(GDEV *dev, H x, H y, H w, H h, const char *label, BOOL pressed) {
    RECT btn = { x, y, x + w, y + h };
    fill_rec(dev, &btn, pressed ? COLOR_DKGRAY : COLOR_LTGRAY);
    drw_rec(dev, &btn);

    if (!pressed) {
        drw_lin(dev, x + 1, y + 1, x + w - 2, y + 1);
        drw_lin(dev, x + 1, y + 1, x + 1, y + h - 2);
    }

    H tx = x + (w - (H)strlen(label) * 8) / 2;
    drw_tc_string(dev, tx > x ? tx : x + 4, y + 4, label, COLOR_BLACK, pressed ? COLOR_DKGRAY : COLOR_LTGRAY);
}

static void paint_terminal_settings(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    /* Header Bar: Settings window icon is ALWAYS 32x32 */
    RECT hdr = { 0, 0, dev->width, 40 };
    fill_rec(dev, &hdr, COLOR_LTGRAY);
    drw_lin(dev, 0, 40, dev->width, 40);
    draw_setting_gif_icon_scaled(dev, "terminal", 6, 4, 32, 32);

    drw_tc_string(dev, 44, 4, "Terminal Settings – 端末・通信環境設定", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 44, 22, "B-System Workstation Console, VT100 Emulator & Colours", COLOR_DKGRAY, COLOR_LTGRAY);

    TERMINAL_SETTINGS *cfg = &g_state_terminal.local_cfg;

    /* Section 1: 配色テーマ (Color Theme) */
    RECT sec1 = { 10, 48, dev->width - 10, 118 };
    fill_rec(dev, &sec1, COLOR_WHITE);
    drw_rec(dev, &sec1);
    drw_tc_string(dev, 18, 42, " [1] 配色テーマ (Colour Theme) ", COLOR_NAVY, COLOR_WHITE);

    paint_ui_radio(dev, 20, 60, "グリーン (Matrix Green)", cfg->theme == TERM_THEME_GREEN, FALSE);
    paint_ui_radio(dev, 230, 60, "アンバー (Amber CRT)", cfg->theme == TERM_THEME_AMBER, FALSE);
    paint_ui_radio(dev, 20, 80, "白黒 (White on Black)", cfg->theme == TERM_THEME_WHITE, FALSE);
    paint_ui_radio(dev, 230, 80, "BTRON青 (Cyan on Navy)", cfg->theme == TERM_THEME_CYAN, FALSE);
    paint_ui_radio(dev, 20, 100, "ライト (Paper Light)", cfg->theme == TERM_THEME_LIGHT, FALSE);

    /* Section 2: 文字サイズ (Font Size) */
    RECT sec2 = { 10, 126, dev->width - 10, 180 };
    fill_rec(dev, &sec2, COLOR_WHITE);
    drw_rec(dev, &sec2);
    drw_tc_string(dev, 18, 120, " [2] 文字・行間 (Font Size) ", COLOR_NAVY, COLOR_WHITE);

    paint_ui_radio(dev, 20, 138, "小 12px (32行×100桁)", cfg->font_size == TERM_FONT_12, FALSE);
    paint_ui_radio(dev, 200, 138, "標準 16px (24行×80桁)", cfg->font_size == TERM_FONT_16, FALSE);
    paint_ui_radio(dev, 380, 138, "大 20px (18行×64桁)", cfg->font_size == TERM_FONT_20, FALSE);

    /* Section 3: スクロール履歴・カーソル (Scrollback & Cursor) */
    RECT sec3 = { 10, 188, dev->width - 10, 260 };
    fill_rec(dev, &sec3, COLOR_WHITE);
    drw_rec(dev, &sec3);
    drw_tc_string(dev, 18, 182, " [3] 履歴・カーソル (Scrollback & Cursor) ", COLOR_NAVY, COLOR_WHITE);

    paint_ui_radio(dev, 20, 200, "履歴 100行", cfg->scrollback_lines == 100, FALSE);
    paint_ui_radio(dev, 160, 200, "履歴 300行 (標準)", cfg->scrollback_lines == 300, FALSE);
    paint_ui_radio(dev, 340, 200, "履歴 1000行 (大)", cfg->scrollback_lines == 1000, FALSE);

    paint_ui_radio(dev, 20, 226, "カーソル下線 (_)", cfg->cursor_style == TERM_CURSOR_UNDERLINE, FALSE);
    paint_ui_radio(dev, 200, 226, "ブロックカーソル (█)", cfg->cursor_style == TERM_CURSOR_BLOCK, FALSE);

    /* Section 4: 画面透過度 (Dimming / Transparency) */
    RECT sec4 = { 10, 268, dev->width - 10, 326 };
    fill_rec(dev, &sec4, COLOR_WHITE);
    drw_rec(dev, &sec4);
    drw_tc_string(dev, 18, 262, " [4] 背景透過・減光 (Background Transparency) ", COLOR_NAVY, COLOR_WHITE);

    paint_ui_radio(dev, 20, 280, "不透明 (100% Opaque)", cfg->transparency == TERM_TRANSPARENCY_OPAQUE, FALSE);
    paint_ui_radio(dev, 200, 280, "20%減光 (80% Dimmed)", cfg->transparency == TERM_TRANSPARENCY_80, FALSE);
    paint_ui_radio(dev, 380, 280, "40%減光 (60% Dimmed)", cfg->transparency == TERM_TRANSPARENCY_60, FALSE);

    /* Action Buttons */
    paint_ui_button(dev, dev->width - 240, dev->height - 34, 110, 24, "標準に戻す", FALSE);
    paint_ui_button(dev, dev->width - 120, dev->height - 34, 110, 24, "適用 (Apply)", FALSE);
}

static void handle_terminal_settings_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - wnd->client.left;
        H rel_y = evt->pos.y - wnd->client.top;
        TERMINAL_SETTINGS *cfg = &g_state_terminal.local_cfg;

        /* Section 1: Themes */
        if (rel_y >= 58 && rel_y <= 76) {
            if (rel_x >= 20 && rel_x < 220) { cfg->theme = TERM_THEME_GREEN; cfg->fg_color = 0xFF22C55E; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 230 && rel_x < 430) { cfg->theme = TERM_THEME_AMBER; cfg->fg_color = 0xFFF59E0B; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        } else if (rel_y >= 78 && rel_y <= 96) {
            if (rel_x >= 20 && rel_x < 220) { cfg->theme = TERM_THEME_WHITE; cfg->fg_color = 0xFFFFFFFF; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 230 && rel_x < 430) { cfg->theme = TERM_THEME_CYAN; cfg->fg_color = 0xFF38BDF8; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        } else if (rel_y >= 98 && rel_y <= 116) {
            if (rel_x >= 20 && rel_x < 220) { cfg->theme = TERM_THEME_LIGHT; cfg->fg_color = 0xFF0F172A; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        }

        /* Section 2: Font Size */
        if (rel_y >= 136 && rel_y <= 156) {
            if (rel_x >= 20 && rel_x < 190) { cfg->font_size = TERM_FONT_12; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 200 && rel_x < 370) { cfg->font_size = TERM_FONT_16; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 380 && rel_x < 510) { cfg->font_size = TERM_FONT_20; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        }

        /* Section 3: Scrollback & Cursor */
        if (rel_y >= 198 && rel_y <= 218) {
            if (rel_x >= 20 && rel_x < 150) { cfg->scrollback_lines = 100; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 160 && rel_x < 330) { cfg->scrollback_lines = 300; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 340 && rel_x < 500) { cfg->scrollback_lines = 1000; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        } else if (rel_y >= 224 && rel_y <= 246) {
            if (rel_x >= 20 && rel_x < 190) { cfg->cursor_style = TERM_CURSOR_UNDERLINE; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 200 && rel_x < 400) { cfg->cursor_style = TERM_CURSOR_BLOCK; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        }

        /* Section 4: Transparency */
        if (rel_y >= 278 && rel_y <= 298) {
            if (rel_x >= 20 && rel_x < 190) { cfg->transparency = TERM_TRANSPARENCY_OPAQUE; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 200 && rel_x < 370) { cfg->transparency = TERM_TRANSPARENCY_80; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
            else if (rel_x >= 380 && rel_x < 510) { cfg->transparency = TERM_TRANSPARENCY_60; g_state_terminal.is_dirty = TRUE; wnd->paint(wnd, wnd->dev); }
        }

        /* Buttons */
        H client_w = wnd->client.right - wnd->client.left;
        H btn_y = (wnd->client.bottom - wnd->client.top) - 34;
        if (rel_y >= btn_y && rel_y <= btn_y + 24) {
            if (rel_x >= client_w - 240 && rel_x < client_w - 130) {
                terminal_reset_settings();
                terminal_get_settings(&g_state_terminal.local_cfg);
                g_state_terminal.is_dirty = FALSE;
                wnd->paint(wnd, wnd->dev);
            } else if (rel_x >= client_w - 120 && rel_x < client_w - 10) {
                terminal_set_settings(&g_state_terminal.local_cfg);
                g_state_terminal.is_dirty = FALSE;
                cls_wnd(wnd);
            }
        }
    }
}

WND* open_terminal_settings_window(void) {
    terminal_get_settings(&g_state_terminal.local_cfg);
    g_state_terminal.is_dirty = FALSE;

    H win_w = 540;
    H win_h = 420;
    WND *wnd = opn_wnd("Terminal Settings (端末・通信環境設定)",
                       (1280 - win_w) / 2 + 20, (800 - win_h) / 2 + 20, win_w, win_h,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (!wnd) return NULL;

    g_state_terminal.wnd = wnd;
    wnd->paint = paint_terminal_settings;
    wnd->event_handler = handle_terminal_settings_event;
    return wnd;
}
