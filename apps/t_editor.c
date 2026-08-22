/*
 * BTRON Accessory: TRON Text Editor Window (t_editor)
 */

#include <btron/wnd.h>
#include <btron/troncode.h>
#include <stdio.h>

static void paint_t_editor(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    /* Text Editor Toolbar */
    RECT tb = { 0, 0, dev->width, 22 };
    fill_rec(dev, &tb, COLOR_LTGRAY);
    drw_lin(dev, 0, 22, dev->width, 22);
    drw_tc_string(dev, 10, 3, "Font: TRON-Code  [VOBJ Embed]  [Save]", COLOR_BLACK, 0x00000000);

    /* Document Body */
    drw_tc_string(dev, 10, 30, "B-TRON Specification Text Editor (T-Editor)", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 10, 50, "---------------------------------------------", COLOR_GRAY, 0x00000000);
    drw_tc_string(dev, 10, 70, "TRON (The Real-time Operating system Nucleus)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 90, "Personal computer OS architecture designed for", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 110, "human-machine interaction and multilingual support.", COLOR_BLACK, 0x00000000);

    /* Embedded Virtual Object Icon inside Document! */
    RECT embed_vobj = { 10, 135, 180, 175 };
    fill_rec(dev, &embed_vobj, COLOR_LTGRAY);
    drw_rec(dev, &embed_vobj);
    drw_tc_string(dev, 15, 145, "[VOBJ: Diagram.draw]", COLOR_NAVY, 0x00000000);
}

WND* open_t_editor_window(void) {
    WND *wnd = opn_wnd("T-Editor - README.txt", 400, 140, 440, 260,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_t_editor;
    }
    return wnd;
}
