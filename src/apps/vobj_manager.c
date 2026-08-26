/*
 * BTRON Accessory: Real Object Cabinet & Virtual Object Explorer Window (vobj_manager)
 * Pure Specification-based implementation of Sakamura BTRON / BTRON3 Architecture.
 */

#include <btron/wnd.h>
#include <btron/vobj.h>
#include <btron/troncode.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

static void paint_vobj_manager(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Fill client area background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    drw_tc_string(dev, 10, 10, "REAL OBJECT CABINET / HYPER-DATA STORE", COLOR_NAVY, 0x00000000);
    drw_lin(dev, 10, 30, dev->width - 10, 30);

    /* Display Virtual Objects & Links */
    drw_tc_string(dev, 15, 40, "[F] Cabinet / Main Root Folder", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 60, "-> [T] README.txt (RealObject #101)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 80, "-> [X] Terminal Shell (RealObject #102)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 35, 100, "-> [D] BTRON Spec Diagram (VirtualLink #103)", COLOR_BLACK, 0x00000000);

    RECT link_box = { 30, 130, dev->width - 30, 170 };
    fill_rec(dev, &link_box, COLOR_LTGRAY);
    drw_rec(dev, &link_box);
    drw_tc_string(dev, 40, 140, "Hyper-Data Model Status: Active", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 40, 155, "Click any Virtual Object pointer to open.", COLOR_GRAY, 0x00000000);
}

WND* open_vobj_manager_window(void) {
    WND *wnd = opn_wnd("BTRON Cabinet Explorer", 100, 80, 420, 240,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_vobj_manager;
    }
    return wnd;
}
