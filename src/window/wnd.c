/*
 * B-TRON Window Manager: wnd.c
 * Sakamura style retro double-bordered windows and client region management.
 */

#include <btron/wnd.h>
#include <btron/troncode.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#else
#include <stddef.h>
#include <stdint.h>
extern void* Imalloc(size_t sz);
extern void Ifree(void *ptr);
extern void* Icalloc(size_t nmemb, size_t sz);
extern char* tkl_strncpy(char *dst, const char *src, size_t n);
#define malloc Imalloc
#define free Ifree
#define calloc Icalloc
#define strncpy tkl_strncpy
#endif

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
    wnd->paint = NULL;
    wnd->event_handler = NULL;
    wnd->user_data = 0;

    const char *src_t = title ? title : "BTRON Window";
    int ti = 0;
    for (; ti < 63 && src_t[ti] != '\0'; ti++) {
        wnd->title[ti] = src_t[ti];
    }
    wnd->title[ti] = '\0';

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
    wnd->next = g_wnd_head;
    wnd->prev = NULL;
    if (g_wnd_head) {
        g_wnd_head->focused = FALSE;
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
    if (!dev || !wnd) return;

    RECT outer;
    outer.left = wnd->bounds.left;
    outer.top = wnd->bounds.top;
    outer.right = wnd->bounds.right;
    outer.bottom = wnd->bounds.bottom;

    fill_rec(dev, &outer, COLOR_LTGRAY);
    drw_rec(dev, &outer);

    RECT inner_b;
    inner_b.left = outer.left + 2;
    inner_b.top = outer.top + 2;
    inner_b.right = outer.right - 2;
    inner_b.bottom = outer.bottom - 2;
    drw_rec(dev, &inner_b);

    if (wnd->attr & WND_ATTR_TITLE) {
        RECT title_r;
        title_r.left = outer.left + 3;
        title_r.top = outer.top + 3;
        title_r.right = outer.right - 3;
        title_r.bottom = outer.top + 22;

        COLOR title_col = wnd->focused ? COLOR_NAVY : COLOR_GRAY;
        fill_rec(dev, &title_r, title_col);

        drw_tc_string(dev, title_r.left + 6, title_r.top + 3, wnd->title, COLOR_WHITE, 0x00000000);

        if (wnd->attr & WND_ATTR_CLOSE) {
            RECT close_btn;
            close_btn.left = outer.right - 20;
            close_btn.top = outer.top + 5;
            close_btn.right = outer.right - 6;
            close_btn.bottom = outer.top + 19;

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
        if (!wnd || !wnd->visible) continue;

        /* Draw window decoration frame */
        draw_retro_window_frame(g_screen_dev, wnd);

        /* Render application client area */
        if (wnd->paint && wnd->dev && wnd->dev->pixels) {
            wnd->paint(wnd, wnd->dev);

            /* Composite client pixels onto main screen */
            H title_h = (wnd->attr & WND_ATTR_TITLE) ? 22 : 0;
            H dest_x = wnd->bounds.left + 4;
            H dest_y = wnd->bounds.top + title_h + 4;

            for (H cy = 0; cy < wnd->dev->height; cy++) {
                for (H cx = 0; cx < wnd->dev->width; cx++) {
                    H px = dest_x + cx;
                    H py = dest_y + cy;
                    if (px >= 0 && px < g_screen_dev->width && py >= 0 && py < g_screen_dev->height) {
                        COLOR c = wnd->dev->pixels[cy * wnd->dev->width + cx];
                        if (c != 0x00000000) {
                            ((volatile COLOR*)g_screen_dev->pixels)[py * g_screen_dev->width + px] = c;
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
