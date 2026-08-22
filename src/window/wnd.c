/*
 * B-TRON Window Manager: wnd.c
 * Sakamura style retro double-bordered windows and client region management.
 */

#include <btron/wnd.h>
#include <btron/troncode.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static WND *g_wnd_head = NULL;
static ID g_next_wnd_id = 1;
static GDEV *g_screen_dev = NULL;

ER init_wnd_mgr(GDEV *screen_dev) {
    g_screen_dev = screen_dev;
    g_wnd_head = NULL;
    return E_OK;
}

WND* opn_wnd(const char *title, H x, H y, H w, H h, UW attr) {
    WND *wnd = (WND*)calloc(1, sizeof(WND));
    if (!wnd) return NULL;

    wnd->id = g_next_wnd_id++;
    strncpy(wnd->title, title ? title : "BTRON Window", sizeof(wnd->title) - 1);
    wnd->bounds.left = x;
    wnd->bounds.top = y;
    wnd->bounds.right = x + w;
    wnd->bounds.bottom = y + h;

    H title_h = (attr & WND_ATTR_TITLE) ? 22 : 0;
    wnd->client.left = x + 4;
    wnd->client.top = y + title_h + 4;
    wnd->client.right = x + w - 4;
    wnd->client.bottom = y + h - 4;

    wnd->attr = attr;
    wnd->visible = TRUE;
    wnd->focused = TRUE;

    wnd->dev = opn_dev(w - 8, h - title_h - 8);

    /* Insert at head of z-stack */
    if (g_wnd_head) {
        g_wnd_head->focused = FALSE;
        wnd->next = g_wnd_head;
        g_wnd_head->prev = wnd;
    }
    g_wnd_head = wnd;

    return wnd;
}

ER cls_wnd(WND *wnd) {
    if (!wnd) return E_PAR;

    if (wnd->prev) wnd->prev->next = wnd->next;
    if (wnd->next) wnd->next->prev = wnd->prev;
    if (g_wnd_head == wnd) g_wnd_head = wnd->next;

    if (g_wnd_head) g_wnd_head->focused = TRUE;

    if (wnd->dev) cls_dev(wnd->dev);
    free(wnd);
    return E_OK;
}

ER top_wnd(WND *wnd) {
    if (!wnd || g_wnd_head == wnd) return E_OK;

    /* Remove from current position */
    if (wnd->prev) wnd->prev->next = wnd->next;
    if (wnd->next) wnd->next->prev = wnd->prev;

    /* Move to top */
    if (g_wnd_head) g_wnd_head->focused = FALSE;
    wnd->next = g_wnd_head;
    wnd->prev = NULL;
    if (g_wnd_head) g_wnd_head->prev = wnd;
    g_wnd_head = wnd;
    wnd->focused = TRUE;

    return E_OK;
}

ER mov_wnd(WND *wnd, H x, H y) {
    if (!wnd) return E_PAR;

    H w = wnd->bounds.right - wnd->bounds.left;
    H h = wnd->bounds.bottom - wnd->bounds.top;

    wnd->bounds.left = x;
    wnd->bounds.top = y;
    wnd->bounds.right = x + w;
    wnd->bounds.bottom = y + h;

    H title_h = (wnd->attr & WND_ATTR_TITLE) ? 22 : 0;
    wnd->client.left = x + 4;
    wnd->client.top = y + title_h + 4;
    wnd->client.right = x + w - 4;
    wnd->client.bottom = y + h - 4;

    return E_OK;
}

static void draw_retro_window_frame(GDEV *dev, WND *wnd) {
    RECT outer = wnd->bounds;

    /* Window outer shadow and frame */
    fill_rec(dev, &outer, COLOR_LTGRAY);
    drw_rec(dev, &outer);

    /* Double retro border line */
    RECT inner_b = { outer.left + 2, outer.top + 2, outer.right - 2, outer.bottom - 2 };
    drw_rec(dev, &inner_b);

    /* Titlebar */
    if (wnd->attr & WND_ATTR_TITLE) {
        RECT title_r = { outer.left + 3, outer.top + 3, outer.right - 3, outer.top + 22 };
        COLOR title_col = wnd->focused ? COLOR_NAVY : COLOR_GRAY;
        fill_rec(dev, &title_r, title_col);

        /* Title text */
        drw_tc_string(dev, title_r.left + 6, title_r.top + 3, wnd->title, COLOR_WHITE, 0x00000000);

        /* Close Button [X] */
        if (wnd->attr & WND_ATTR_CLOSE) {
            RECT close_btn = { outer.right - 20, outer.top + 5, outer.right - 6, outer.top + 19 };
            fill_rec(dev, &close_btn, COLOR_LTGRAY);
            drw_rec(dev, &close_btn);
            drw_tc_string(dev, close_btn.left + 4, close_btn.top + 1, "x", COLOR_BLACK, 0x00000000);
        }
    }
}

void redraw_all_windows(void) {
    if (!g_screen_dev) return;

    /* Draw windows back to front */
    WND *stack[32];
    int count = 0;
    WND *curr = g_wnd_head;
    while (curr && count < 32) {
        stack[count++] = curr;
        curr = curr->next;
    }

    for (int i = count - 1; i >= 0; i--) {
        WND *wnd = stack[i];
        if (!wnd->visible) continue;

        /* Draw window decoration frame */
        draw_retro_window_frame(g_screen_dev, wnd);

        /* Render application client area */
        if (wnd->paint && wnd->dev) {
            wnd->paint(wnd, wnd->dev);

            /* Composite client pixels onto main screen */
            H title_h = (wnd->attr & WND_ATTR_TITLE) ? 22 : 0;
            H dest_x = wnd->bounds.left + 4;
            H dest_y = wnd->bounds.top + title_h + 4;

            for (H cy = 0; cy < wnd->dev->height; cy++) {
                for (H cx = 0; cx < wnd->dev->width; cx++) {
                    H px = dest_x + cx;
                    H py = dest_y + cy;
                    if (px >= 0 && px < g_screen_dev->width &&
                        py >= 0 && py < g_screen_dev->height) {
                        COLOR c = wnd->dev->pixels[cy * wnd->dev->width + cx];
                        if (c != 0x00000000) {
                            g_screen_dev->pixels[py * g_screen_dev->width + px] = c;
                        }
                    }
                }
            }
        }
    }
}

WND* find_wnd_at(H x, H y) {
    WND *curr = g_wnd_head;
    while (curr) {
        if (curr->visible &&
            x >= curr->bounds.left && x < curr->bounds.right &&
            y >= curr->bounds.top && y < curr->bounds.bottom) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

WND* get_top_wnd(void) {
    return g_wnd_head;
}
