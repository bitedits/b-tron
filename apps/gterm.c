/*
 * BTRON Accessory: System Terminal Console Window (gterm)
 */

#include <btron/wnd.h>
#include <btron/troncode.h>
#include <stdio.h>

static void paint_gterm(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Black Terminal Screen */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_BLACK);
    drw_rec(dev, &r);

    /* Terminal Output Text */
    drw_tc_string(dev, 8, 8, "B-TRON OS Shell v1.0 (gterm)", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 26, "Kernel: uITRON 4.0 RTOS Engine", COLOR_GRAY, 0x00000000);
    drw_tc_string(dev, 8, 44, "Graphics: Display Primitives (DP) SDL2", COLOR_GRAY, 0x00000000);
    drw_tc_string(dev, 8, 62, "Storage: VOBJ Real/Virtual Object Store", COLOR_GRAY, 0x00000000);
    drw_tc_string(dev, 8, 88, "btron# uname -a", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 106, "BTRON 1.0 btron-sdl2-posix #1 2026 x86_64", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 130, "btron# ps -a", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 148, "PID  TASK      PRI  STAT   TIME", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 166, "  1  desktop    10  RUN    00:01", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 184, "  2  wnd_mgr    12  SLEEP  00:00", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 202, "  3  gterm      15  RUN    00:00", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 8, 226, "btron# _", COLOR_WHITE, 0x00000000);
}

WND* open_gterm_window(void) {
    WND *wnd = opn_wnd("BTRON Terminal (gterm)", 180, 260, 480, 270,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_gterm;
    }
    return wnd;
}
