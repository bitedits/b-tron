/*
 * B-System (BTRON 3.20) Control Panel & Settings Cabinet Explorer: control_panel.c
 * Central Settings Management Application conforming to ./b-system/settings/
 */

#include <btron/settings.h>
#include <btron/dp.h>
#include <btron/troncode.h>
#include <btron/wnd.h>

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

static const SETTINGS_APP_INFO g_app_registry[] = {
    { "appearance", "Appearance", "外観",        "[ART]", "Themes, colours, window style, fonts & animations", open_appearance_settings_window },
    { "desktop",    "Desktop",    "デスクトップ", "[DSK]", "Workbench behaviour, icon layout & background",     open_desktop_settings_window },
    { "display",    "Display",    "画面表示",     "[DSP]", "Resolution, DPI scaling, refresh rate & VESA",       open_display_settings_window },
    { "input",      "Input",      "入力環境",     "[INP]", "Keyboard layouts, key repeat & mouse acceleration",  open_input_settings_window },
    { "language",   "Language",   "言語・文字",   "[LAN]", "TRON Code, TIP Mozc, Tibetan Wylie & shortcuts",     open_language_settings_window },
    { "media",      "Media",      "メディア",     "[MED]", "Default handlers, TAD/PDF/Audio associations",       open_media_settings_window },
    { "network",    "Network",    "通信網",       "[NET]", "Interfaces, DHCP, DNS, VirtIO-Net & XMPP",           open_network_settings_window },
    { "security",   "Security",   "保全・権限",   "[SEC]", "Permissions, real-object ACLs & capability isolation", open_security_settings_window },
    { "sound",      "Sound",      "音響・音声",   "[SND]", "Audio devices, MediaPulse routing & master volume",   open_sound_settings_window },
    { "system",     "System",     "基本情報",     "[SYS]", "System Plane info, kernel tasks, memory & SMP",      open_system_settings_window }
};

#define APP_REGISTRY_COUNT (sizeof(g_app_registry) / sizeof(g_app_registry[0]))

const SETTINGS_APP_INFO* settings_get_app_info(SETTINGS_APP_ID app_id) {
    if (app_id >= 1 && app_id <= (SETTINGS_APP_ID)APP_REGISTRY_COUNT) {
        return &g_app_registry[app_id - 1];
    }
    return NULL;
}

int settings_get_app_count(void) {
    return (int)APP_REGISTRY_COUNT;
}

typedef struct {
    WND *wnd;
    int selected_index;
    int hover_index;
} ControlPanelApp;

static ControlPanelApp g_ctrl_panel;

static void paint_control_panel(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background surface */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_LTGRAY);
    drw_rec(dev, &r);

    /* Header Bar */
    RECT hdr = { 0, 0, dev->width, 30 };
    fill_rec(dev, &hdr, COLOR_WHITE);
    drw_lin(dev, 0, 30, dev->width, 30);
    drw_tc_string(dev, 10, 8, "B-System Settings Cabinet (環境設定キャビネット) - Control Panel", COLOR_NAVY, COLOR_WHITE);

    /* Sub-header instruction */
    drw_tc_string(dev, 12, 38, "Select a settings category below to configure system preferences:", COLOR_BLACK, COLOR_LTGRAY);

    /* Applet Grid Layout: 2 columns x 5 rows */
    H start_x = 16;
    H start_y = 60;
    H item_w = (dev->width - 48) / 2;
    H item_h = 58;

    for (int i = 0; i < (int)APP_REGISTRY_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        H x = start_x + col * (item_w + 16);
        H y = start_y + row * (item_h + 10);

        RECT item_r = { x, y, x + item_w, y + item_h };
        BOOL is_sel = (i == g_ctrl_panel.selected_index);

        COLOR bg_col = is_sel ? COLOR_WHITE : 0x00EFEFEF;
        fill_rec(dev, &item_r, bg_col);
        drw_rec(dev, &item_r);

        /* Icon Badge */
        RECT icon_r = { x + 6, y + 8, x + 44, y + 48 };
        fill_rec(dev, &icon_r, is_sel ? COLOR_NAVY : COLOR_GRAY);
        drw_rec(dev, &icon_r);
        drw_tc_string(dev, x + 8, y + 20, g_app_registry[i].icon_symbol, COLOR_WHITE, is_sel ? COLOR_NAVY : COLOR_GRAY);

        /* Title & Description */
        char title_buf[64];
        snprintf(title_buf, sizeof(title_buf), "%s (%s)", g_app_registry[i].title, g_app_registry[i].title_ja);
        drw_tc_string(dev, x + 50, y + 10, title_buf, is_sel ? COLOR_NAVY : COLOR_BLACK, bg_col);
        drw_tc_string(dev, x + 50, y + 30, g_app_registry[i].desc, COLOR_DKGRAY, bg_col);
    }

    /* Footer Bar with Open Button */
    RECT ftr = { 0, dev->height - 36, dev->width, dev->height };
    fill_rec(dev, &ftr, COLOR_WHITE);
    drw_lin(dev, 0, dev->height - 36, dev->width, dev->height - 36);

    RECT btn_open = { dev->width - 190, dev->height - 30, dev->width - 100, dev->height - 8 };
    fill_rec(dev, &btn_open, COLOR_LTGRAY);
    drw_rec(dev, &btn_open);
    drw_tc_string(dev, dev->width - 170, dev->height - 24, "Open (開く)", COLOR_BLACK, COLOR_LTGRAY);

    RECT btn_close = { dev->width - 90, dev->height - 30, dev->width - 10, dev->height - 8 };
    fill_rec(dev, &btn_close, COLOR_LTGRAY);
    drw_rec(dev, &btn_close);
    drw_tc_string(dev, dev->width - 75, dev->height - 24, "Close", COLOR_BLACK, COLOR_LTGRAY);
}

static void handle_control_panel_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - wnd->client.left;
        H rel_y = evt->pos.y - wnd->client.top;
        H client_w = wnd->client.right - wnd->client.left;
        H client_h = wnd->client.bottom - wnd->client.top;

        H start_x = 16;
        H start_y = 60;
        H item_w = (client_w - 48) / 2;
        H item_h = 58;

        /* Check Grid Clicks */
        for (int i = 0; i < (int)APP_REGISTRY_COUNT; i++) {
            int col = i % 2;
            int row = i / 2;
            H x = start_x + col * (item_w + 16);
            H y = start_y + row * (item_h + 10);

            if (rel_x >= x && rel_x <= x + item_w && rel_y >= y && rel_y <= y + item_h) {
                g_ctrl_panel.selected_index = i;
                /* Launch applet */
                if (g_app_registry[i].open_func) {
                    g_app_registry[i].open_func();
                }
                redraw_all_windows();
                return;
            }
        }

        /* Check Footer Buttons */
        H w = client_w;
        H h = client_h;
        if (rel_y >= h - 30 && rel_y <= h - 8) {
            if (rel_x >= w - 190 && rel_x <= w - 100) {
                if (g_ctrl_panel.selected_index >= 0 && g_ctrl_panel.selected_index < (int)APP_REGISTRY_COUNT) {
                    if (g_app_registry[g_ctrl_panel.selected_index].open_func) {
                        g_app_registry[g_ctrl_panel.selected_index].open_func();
                    }
                }
                return;
            }
            if (rel_x >= w - 90 && rel_x <= w - 10) {
                cls_wnd(wnd);
                return;
            }
        }
    }
}

WND* open_control_panel_window(void) {
    g_ctrl_panel.selected_index = 0;
    WND *wnd = opn_wnd("Settings Cabinet - Control Panel (環境設定キャビネット)",
                       60, 40, 680, 480,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (!wnd) return NULL;
    g_ctrl_panel.wnd = wnd;
    wnd->user_data = (VW)(uintptr_t)&g_ctrl_panel;
    wnd->paint = paint_control_panel;
    wnd->event_handler = handle_control_panel_event;
    return wnd;
}
