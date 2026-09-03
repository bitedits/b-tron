/*
 * B-System (BTRON 3.20) Settings Applet: network (Network / 通信網)
 * Conforming to ./b-system/settings/Network.html
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
#define memset tkl_memset
#define strlen tkl_strlen
#define snprintf tkl_snprintf
#endif

typedef struct {
    WND *wnd;
    BOOL checks[9];
    BOOL is_dirty;
} AppletState_network;

static AppletState_network g_state_network;

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

static void paint_network_settings(WND *wnd, GDEV *dev) {
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
    snprintf(hdr_str, sizeof(hdr_str), "[Settings Cabinet] %s (%s) - %s", "Network", "通信網", "Interfaces, TCP/IP & XMPP connectivity");
    drw_tc_string(dev, 8, 6, hdr_str, COLOR_BLACK, COLOR_LTGRAY);

    /* Section 1: Network Interface Configuration */
    RECT s1 = { 10, 38, dev->width - 10, 132 };
    fill_rec(dev, &s1, COLOR_WHITE);
    drw_rec(dev, &s1);
    drw_tc_string(dev, 16, 30, " [1. Network Interface Configuration] ", COLOR_NAVY, COLOR_WHITE);
    paint_ui_checkbox(dev, 18, 52, "Enable Primary Interface: eth0 (VirtIO-Net / Realtek)", g_state_network.checks[0], FALSE);
    paint_ui_checkbox(dev, 18, 74, "Automatic DHCP Configuration (IPv4 / IPv6)", g_state_network.checks[1], FALSE);
    paint_ui_checkbox(dev, 18, 96, "Primary DNS Resolution: 1.1.1.1, 8.8.8.8", g_state_network.checks[2], FALSE);

    /* Section 2: Messaging & Remote Services */
    RECT s2 = { 10, 146, dev->width - 10, 218 };
    fill_rec(dev, &s2, COLOR_WHITE);
    drw_rec(dev, &s2);
    drw_tc_string(dev, 16, 138, " [2. Messaging & Remote Services] ", COLOR_NAVY, COLOR_WHITE);
    paint_ui_checkbox(dev, 18, 160, "Enable XMPP Instant Messaging Client Protocol", g_state_network.checks[3], FALSE);
    paint_ui_checkbox(dev, 18, 182, "Non-blocking Sockets Subsystem (T-Kernel TCP/IP)", g_state_network.checks[4], FALSE);

    /* Action Buttons */
    H btn_y = dev->height - 35;
    paint_ui_button(dev, dev->width - 240, btn_y, 70, 24, "Default", FALSE);
    paint_ui_button(dev, dev->width - 160, btn_y, 70, 24, "Apply", g_state_network.is_dirty);
    paint_ui_button(dev, dev->width - 80, btn_y, 70, 24, "Close", FALSE);
}

static void handle_network_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;
    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - wnd->client.left;
        H rel_y = evt->pos.y - wnd->client.top;
        H client_w = wnd->client.right - wnd->client.left;
        H client_h = wnd->client.bottom - wnd->client.top;

        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 50 && rel_y <= 68) {
            g_state_network.checks[0] = !g_state_network.checks[0];
            g_state_network.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 72 && rel_y <= 90) {
            g_state_network.checks[1] = !g_state_network.checks[1];
            g_state_network.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 94 && rel_y <= 112) {
            g_state_network.checks[2] = !g_state_network.checks[2];
            g_state_network.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 158 && rel_y <= 176) {
            g_state_network.checks[3] = !g_state_network.checks[3];
            g_state_network.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        if (rel_x >= 18 && rel_x <= 480 && rel_y >= 180 && rel_y <= 198) {
            g_state_network.checks[4] = !g_state_network.checks[4];
            g_state_network.is_dirty = TRUE;
            redraw_all_windows();
            return;
        }
        /* Action Buttons */
        H btn_y = client_h - 35;
        if (rel_y >= btn_y && rel_y <= btn_y + 24) {
            if (rel_x >= client_w - 240 && rel_x <= client_w - 170) {
                /* Default */
                for (int i = 0; i < 5; i++) g_state_network.checks[i] = TRUE;
                g_state_network.is_dirty = TRUE;
                redraw_all_windows();
                return;
            }
            if (rel_x >= client_w - 160 && rel_x <= client_w - 90) {
                /* Apply */
                g_state_network.is_dirty = FALSE;
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

WND* open_network_settings_window(void) {
    memset(&g_state_network, 0, sizeof(AppletState_network));
    for (int i = 0; i < 5; i++) g_state_network.checks[i] = TRUE;

    WND *wnd = opn_wnd("Settings Cabinet - Network (通信網)",
                       80, 45, 640, 430,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (!wnd) return NULL;
    g_state_network.wnd = wnd;
    wnd->user_data = (VW)(uintptr_t)&g_state_network;
    wnd->paint = paint_network_settings;
    wnd->event_handler = handle_network_event;
    return wnd;
}
