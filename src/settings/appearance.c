/*
 * B-System (BTRON 3.20) Settings Applet: appearance (Appearance / 外観)
 * Conforming to ./b-system/settings/Appearance.html
 */

#include <btron/settings.h>
#include <btron/dp.h>
#include <btron/troncode.h>
#include <btron/wnd.h>
#include <btron/app_menu.h>

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

typedef struct {
    WND *wnd;
    BOOL checks[11];
    BOOL is_dirty;
} AppletState_appearance;

static AppletState_appearance g_state_appearance;

/* 3D Crisp Graphical Checkbox */
static void paint_ui_checkbox(GDEV *dev, H x, H y, const char *label, BOOL checked, BOOL focused) {
    RECT box = { x, y + 1, x + 15, y + 16 };
    fill_rec(dev, &box, COLOR_WHITE);
    drw_rec(dev, &box);

    /* Sunken 3D shadow lines */
    drw_lin(dev, x + 1, y + 2, x + 14, y + 2);
    drw_lin(dev, x + 1, y + 2, x + 1, y + 15);

    if (checked) {
        /* Bold checkmark [✔] */
        drw_lin(dev, x + 3, y + 8, x + 6, y + 12);
        drw_lin(dev, x + 3, y + 9, x + 6, y + 13);
        drw_lin(dev, x + 4, y + 8, x + 7, y + 12);

        drw_lin(dev, x + 6, y + 12, x + 12, y + 4);
        drw_lin(dev, x + 6, y + 13, x + 12, y + 5);
        drw_lin(dev, x + 7, y + 12, x + 13, y + 4);
    }

    COLOR text_col = focused ? COLOR_NAVY : COLOR_BLACK;
    drw_tc_string(dev, x + 22, y, label, text_col, COLOR_WHITE);
}

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

static void paint_appearance_settings(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    /* Header Bar */
    RECT hdr = { 0, 0, dev->width, 28 };
    fill_rec(dev, &hdr, COLOR_LTGRAY);
    drw_lin(dev, 0, 28, dev->width, 28);
    char hdr_str[128];
    snprintf(hdr_str, sizeof(hdr_str), "[Settings Cabinet] %s (%s) - %s", "Appearance", "外観", "Themes, colours & window styles");
    drw_tc_string(dev, 8, 6, hdr_str, COLOR_BLACK, COLOR_LTGRAY);

    /* Section 1: System Themes & Visual Appearance */
    RECT s1 = { 10, 34, dev->width - 10, 186 };
    fill_rec(dev, &s1, COLOR_WHITE);
    drw_rec(dev, &s1);
    drw_tc_string(dev, 16, 26, " [1. System Themes & Visual Appearance] ", COLOR_NAVY, COLOR_WHITE);
    paint_ui_radio(dev, 18, 44, "Classic Teal & Navy (BTRON Standard 3.20)", g_state_appearance.checks[0], FALSE);
    paint_ui_radio(dev, 18, 66, "Dark Navy High-Contrast Theme (Chapter 7)", g_state_appearance.checks[1], FALSE);
    paint_ui_radio(dev, 18, 88, "Retro Amber Phosphor Display Palette", g_state_appearance.checks[2], FALSE);
    paint_ui_checkbox(dev, 18, 110, "Enable 3D Double Bezel Window Frame Shadows", g_state_appearance.checks[3], FALSE);

    /* Menu Style Toggle Options */
    BOOL is_classic = (app_menu_get_global_style() == APP_MENU_STYLE_CLASSIC_3D);
    paint_ui_radio(dev, 18, 134, "Menu Style: Classic Chokanji 3D (超漢字3Dベベル)", is_classic, FALSE);
    paint_ui_radio(dev, 18, 156, "Menu Style: Modern Flat Card (現代風カード)", !is_classic, FALSE);

    /* Section 2: Typography & Glyph Rendering */
    RECT s2 = { 10, 196, dev->width - 10, 280 };
    fill_rec(dev, &s2, COLOR_WHITE);
    drw_rec(dev, &s2);
    drw_tc_string(dev, 16, 188, " [2. Typography & Glyph Rendering] ", COLOR_NAVY, COLOR_WHITE);
    paint_ui_checkbox(dev, 18, 206, "Enable TRON Multilingual 16px Vector/Bitmap Engine", g_state_appearance.checks[4], FALSE);
    paint_ui_checkbox(dev, 18, 228, "Load Classical Tibetan Jomolhari Unicode Plane", g_state_appearance.checks[5], FALSE);
    paint_ui_checkbox(dev, 18, 250, "Enable Subpixel Vector Font Anti-Aliasing", g_state_appearance.checks[6], FALSE);

    /* Action Buttons */
    H btn_y = dev->height - 35;
    paint_ui_button(dev, dev->width - 240, btn_y, 70, 24, "Default", FALSE);
    paint_ui_button(dev, dev->width - 160, btn_y, 70, 24, "Apply", g_state_appearance.is_dirty);
    paint_ui_button(dev, dev->width - 80, btn_y, 70, 24, "Close", FALSE);
}

static void handle_appearance_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;
    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - wnd->client.left;
        H rel_y = evt->pos.y - wnd->client.top;
        H client_w = wnd->client.right - wnd->client.left;
        H client_h = wnd->client.bottom - wnd->client.top;

        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 42 && rel_y <= 60) {
            g_state_appearance.checks[0] = !g_state_appearance.checks[0];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 64 && rel_y <= 82) {
            g_state_appearance.checks[1] = !g_state_appearance.checks[1];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 86 && rel_y <= 104) {
            g_state_appearance.checks[2] = !g_state_appearance.checks[2];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 108 && rel_y <= 126) {
            g_state_appearance.checks[3] = !g_state_appearance.checks[3];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 132 && rel_y <= 150) {
            app_menu_set_global_style(APP_MENU_STYLE_CLASSIC_3D);
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 154 && rel_y <= 172) {
            app_menu_set_global_style(APP_MENU_STYLE_MODERN_CARD);
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 204 && rel_y <= 222) {
            g_state_appearance.checks[4] = !g_state_appearance.checks[4];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 226 && rel_y <= 244) {
            g_state_appearance.checks[5] = !g_state_appearance.checks[5];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 248 && rel_y <= 266) {
            g_state_appearance.checks[6] = !g_state_appearance.checks[6];
            g_state_appearance.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        /* Action Buttons */
        H btn_y = client_h - 35;
        if (rel_y >= btn_y && rel_y <= btn_y + 24) {
            if (rel_x >= client_w - 240 && rel_x <= client_w - 170) {
                /* Default */
                for (int i = 0; i < 7; i++) g_state_appearance.checks[i] = TRUE;
                g_state_appearance.is_dirty = TRUE;
                redraw_all_windows();
                return;
            }
            if (rel_x >= client_w - 160 && rel_x <= client_w - 90) {
                /* Apply */
                g_state_appearance.is_dirty = FALSE;
                redraw_all_windows();
                return;
            }
            if (rel_x >= client_w - 80 && rel_x <= client_w - 10) {
                /* Close */
                cls_wnd(wnd);
                return;
            }
        }
    }
}

WND* open_appearance_settings_window(void) {
    memset(&g_state_appearance, 0, sizeof(AppletState_appearance));
    for (int i = 0; i < 7; i++) g_state_appearance.checks[i] = TRUE;

    WND *wnd = opn_wnd("Appearance (外観)",
                       80, 45, 640, 430,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (!wnd) return NULL;
    g_state_appearance.wnd = wnd;
    wnd->user_data = (VW)(uintptr_t)&g_state_appearance;
    wnd->paint = paint_appearance_settings;
    wnd->event_handler = handle_appearance_event;
    return wnd;
}
