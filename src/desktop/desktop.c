/*
 * B-TRON Desktop Compositor: desktop.c
 * Authentic Sakamura Cho-Kanji Teal Desktop Shell & Top Panel.
 */

#include <btron/desktop.h>
#include <btron/troncode.h>
#include <btron/vobj.h>
#include <btron/wnd.h>
#include <stdio.h>
#include <time.h>

static BTRON_DESKTOP g_desktop;

ER init_desktop(H width, H height) {
    g_desktop.width = width;
    g_desktop.height = height;
    g_desktop.screen = opn_dev(width, height);
    g_desktop.running = TRUE;

    init_wnd_mgr(g_desktop.screen);
    init_vobj_sys("./btron_store");

    return E_OK;
}

void render_desktop_background(GDEV *dev) {
    if (!dev) return;

    /* Fill background with classic Sakamura B-TRON Teal palette */
    RECT bg_rect = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &bg_rect, COLOR_TEAL);

    /* Render Retro Grid / Desktop Wallpaper Pattern */
    for (H y = 30; y < dev->height; y += 32) {
        for (H x = 0; x < dev->width; x += 32) {
            drw_pnt(dev, x, y);
        }
    }

    /* Desktop Icons / Cabinet Real Objects */
    RECT cab_icon = { 20, 50, 70, 95 };
    fill_rec(dev, &cab_icon, COLOR_LTGRAY);
    drw_rec(dev, &cab_icon);
    drw_tc_string(dev, 25, 60, "CAB", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 15, 102, "RealObject", COLOR_WHITE, 0x00000000);

    RECT edit_icon = { 20, 130, 70, 175 };
    fill_rec(dev, &edit_icon, COLOR_LTGRAY);
    drw_rec(dev, &edit_icon);
    drw_tc_string(dev, 25, 140, "TXT", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 15, 182, "T-Editor", COLOR_WHITE, 0x00000000);

    RECT term_icon = { 20, 210, 70, 255 };
    fill_rec(dev, &term_icon, COLOR_LTGRAY);
    drw_rec(dev, &term_icon);
    drw_tc_string(dev, 25, 220, "CLI", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 15, 262, "Terminal", COLOR_WHITE, 0x00000000);
}

void render_system_panel(GDEV *dev) {
    if (!dev) return;

    /* Top BTRON Panel Bar */
    RECT panel = { 0, 0, dev->width, 26 };
    fill_rec(dev, &panel, COLOR_LTGRAY);
    drw_lin(dev, 0, 25, dev->width, 25);

    /* TRON Logo & System Menu */
    RECT sys_btn = { 4, 3, 70, 22 };
    fill_rec(dev, &sys_btn, COLOR_GRAY);
    drw_rec(dev, &sys_btn);
    drw_tc_string(dev, 8, 5, "B-TRON", COLOR_WHITE, 0x00000000);

    /* Top Menus */
    drw_tc_string(dev, 85, 5, "File", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 130, 5, "Edit", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 175, 5, "View", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 220, 5, "VirtualObject", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 340, 5, "Window", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 400, 5, "Help", COLOR_BLACK, 0x00000000);

    /* System Real-Time Clock */
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    drw_tc_string(dev, dev->width - 80, 5, time_buf, COLOR_NAVY, 0x00000000);
}
