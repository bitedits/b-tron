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

static H calculate_title_display_width(const char *s) {
    if (!s) return 0;
    H w = 0;
    int i = 0;
    while (s[i] != '\0') {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) {
            w += 8;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            w += 8;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            w += 16;
            i += 3;
        } else {
            w += 8;
            i += 1;
        }
    }
    return w;
}

#define WND_TITLE_HEIGHT 28

ER wget_tab_rect(const WND *wnd, RECT *tab_rect) {
    if (!wnd || !tab_rect) return E_PAR;
    if (!(wnd->attr & WND_ATTR_TITLE)) {
        tab_rect->left = tab_rect->right = tab_rect->top = tab_rect->bottom = 0;
        return E_OK;
    }
    H w = wnd->bounds.right - wnd->bounds.left;
    H tw = wnd->tab_width;
    if (tw <= 0 || tw > w || !(wnd->attr & WND_ATTR_COMPACT_TAB)) {
        tw = w;
    }
    H off_x = wnd->tab_offset_x;
    if (off_x < 0) off_x = 0;
    if (off_x + tw > w) {
        off_x = w - tw;
        if (off_x < 0) off_x = 0;
    }

    tab_rect->left = wnd->bounds.left + off_x;
    tab_rect->top = wnd->bounds.top;
    tab_rect->right = tab_rect->left + tw;
    tab_rect->bottom = wnd->bounds.top + WND_TITLE_HEIGHT;
    return E_OK;
}

BOOL whit_test_tab(const WND *wnd, H x, H y) {
    if (!wnd || !(wnd->attr & WND_ATTR_TITLE)) return FALSE;
    RECT tr;
    if (wget_tab_rect(wnd, &tr) != E_OK) return FALSE;
    return (x >= tr.left && x < tr.right && y >= tr.top && y < tr.bottom);
}

BOOL whit_test_close_btn(const WND *wnd, H x, H y) {
    if (!wnd || !(wnd->attr & WND_ATTR_TITLE) || !(wnd->attr & WND_ATTR_CLOSE)) return FALSE;
    RECT tr;
    if (wget_tab_rect(wnd, &tr) != E_OK) return FALSE;
    H btn_right = tr.right - 6;
    H btn_left = tr.right - 24;
    H btn_top = tr.top + 6;
    H btn_bottom = tr.top + 22;
    return (x >= btn_left && x < btn_right && y >= btn_top && y < btn_bottom);
}

ER wset_tab_offset(WND *wnd, H offset_x) {
    if (!wnd) return E_PAR;
    H w = wnd->bounds.right - wnd->bounds.left;
    H tw = wnd->tab_width;
    if (tw <= 0 || tw > w || !(wnd->attr & WND_ATTR_COMPACT_TAB)) {
        tw = w;
    }
    H max_off = (w > tw) ? (w - tw) : 0;
    if (offset_x < 0) offset_x = 0;
    if (offset_x > max_off) offset_x = max_off;
    wnd->tab_offset_x = offset_x;
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

    H title_h = (attr & WND_ATTR_TITLE) ? WND_TITLE_HEIGHT : 0;
    wnd->client.left = x + 4;
    wnd->client.top = y + title_h + 4;
    wnd->client.right = x + w - 4;
    wnd->client.bottom = y + h - 4;

    /* BTRON3 3.20 Conformance: All windows support corner resize and compact sliding tabs by default */
    attr |= WND_ATTR_RESIZE | WND_ATTR_COMPACT_TAB | WND_ATTR_SLIDING_TAB;
    wnd->attr = attr;
    wnd->visible = TRUE;
    wnd->focused = TRUE;
    wnd->tab_offset_x = 0;

    /* Calculate dynamic compact tab width based on title content */
    H text_w = calculate_title_display_width(wnd->title);
    H tw = text_w + 48; /* text + left margin/grip + close box + padding */
    if (tw < 100) tw = 100;
    if (tw > w) tw = w;
    wnd->tab_width = tw;

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

    if (wnd->destroy) {
        wnd->destroy(wnd);
    }

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

    /* Verify wnd is actually present in active window list */
    BOOL found = FALSE;
    for (WND *c = g_wnd_head; c; c = c->next) {
        if (c == wnd) {
            found = TRUE;
            break;
        }
    }
    if (!found) return E_PAR;

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

    H title_h = (wnd->attr & WND_ATTR_TITLE) ? WND_TITLE_HEIGHT : 0;
    wnd->client.left = x + 4;
    wnd->client.top = y + title_h + 4;
    wnd->client.right = x + w - 4;
    wnd->client.bottom = y + h - 4;

    return E_OK;
}

ER rsz_wnd(WND *wnd, H w, H h) {
    if (!wnd) return E_PAR;
    if (w < 160) w = 160;
    if (h < 100) h = 100;

    wnd->bounds.right = wnd->bounds.left + w;
    wnd->bounds.bottom = wnd->bounds.top + h;

    H title_h = (wnd->attr & WND_ATTR_TITLE) ? WND_TITLE_HEIGHT : 0;
    wnd->client.left = wnd->bounds.left + 4;
    wnd->client.top = wnd->bounds.top + title_h + 4;
    wnd->client.right = wnd->bounds.left + w - 4;
    wnd->client.bottom = wnd->bounds.top + h - 4;

    /* Re-clamp tab width and sliding offset */
    if (wnd->tab_width > w) {
        wnd->tab_width = w;
    }
    wset_tab_offset(wnd, wnd->tab_offset_x);

    H new_dev_w = w - 8;
    H new_dev_h = h - title_h - 8;
    if (new_dev_w < 10) new_dev_w = 10;
    if (new_dev_h < 10) new_dev_h = 10;

    if (wnd->dev) {
        if (wnd->dev->width != new_dev_w || wnd->dev->height != new_dev_h) {
            cls_dev(wnd->dev);
            wnd->dev = opn_dev(new_dev_w, new_dev_h);
        }
    } else {
        wnd->dev = opn_dev(new_dev_w, new_dev_h);
    }

    return E_OK;
}

ER wrsz_wnd(WND *wnd, const RECT *r) {
    if (!wnd || !r) return E_PAR;
    H w = r->right - r->left;
    H h = r->bottom - r->top;
    mov_wnd(wnd, r->left, r->top);
    return rsz_wnd(wnd, w, h);
}

ER inval_wnd(WND *wnd) {
    if (!wnd) return E_PAR;
    /* In immediate composite architecture, dirty window is redrawn on next frame */
    return E_OK;
}

static void draw_retro_window_frame(GDEV *dev, WND *wnd) {
    if (!dev || !wnd) return;

    H title_h = (wnd->attr & WND_ATTR_TITLE) ? WND_TITLE_HEIGHT : 0;

    /* Main Window Body: fills from bounds.top + title_h to bounds.bottom */
    RECT body;
    body.left = wnd->bounds.left;
    body.top = wnd->bounds.top + title_h;
    body.right = wnd->bounds.right;
    body.bottom = wnd->bounds.bottom;

    fill_rec(dev, &body, COLOR_LTGRAY);
    drw_rec(dev, &body);

    RECT inner_b;
    inner_b.left = body.left + 2;
    inner_b.top = body.top + 2;
    inner_b.right = body.right - 2;
    inner_b.bottom = body.bottom - 2;
    drw_rec(dev, &inner_b);

    if (wnd->attr & WND_ATTR_TITLE) {
        RECT tab_r;
        wget_tab_rect(wnd, &tab_r);

        /* BeOS / CWM Minimalist Bold Palette: Iconic Gold when focused, Clean Silver when unfocused */
        COLOR tab_bg = wnd->focused ? COLOR_GOLD : COLOR_LTGRAY;
        COLOR title_col = wnd->focused ? COLOR_BLACK : COLOR_DKGRAY;

        fill_rec(dev, &tab_r, tab_bg);
        drw_rec(dev, &tab_r);

        /* Replicate canonical Window Body double-line border with intermediary space */
        RECT inner_tab;
        inner_tab.left = tab_r.left + 2;
        inner_tab.top = tab_r.top + 2;
        inner_tab.right = tab_r.right - 2;
        inner_tab.bottom = tab_r.bottom - 2;
        drw_rec(dev, &inner_tab);

        /* Minimalist BeOS/CWM double-bar vertical grip on left */
        H text_start_x = tab_r.left + 7;
        if ((wnd->attr & WND_ATTR_SLIDING_TAB) && (wnd->tab_width < (wnd->bounds.right - wnd->bounds.left - 16))) {
            drw_lin(dev, tab_r.left + 5, tab_r.top + 5, tab_r.left + 5, tab_r.top + 22);
            drw_lin(dev, tab_r.left + 8, tab_r.top + 5, tab_r.left + 8, tab_r.top + 22);
            text_start_x = tab_r.left + 13;
        }

        /* Bold & crisp title string - positioned 1px up at tab_r.top + 6 */
        drw_tc_string(dev, text_start_x, tab_r.top + 6, wnd->title, title_col, 0x00000000);

        if (wnd->attr & WND_ATTR_CLOSE) {
            RECT close_btn;
            close_btn.left = tab_r.right - 24;
            close_btn.top = tab_r.top + 6;
            close_btn.right = tab_r.right - 6;
            close_btn.bottom = tab_r.top + 22;

            fill_rec(dev, &close_btn, wnd->focused ? COLOR_LTGRAY : COLOR_GRAY);
            drw_rec(dev, &close_btn);
            /* Centered cross inside close button shifted 2px up (tab_r.top + 6) */
            drw_tc_string(dev, close_btn.left + 5, tab_r.top + 6, "x", COLOR_BLACK, 0x00000000);
        }
    }

    /* BTRON3 3.20 Standard Bottom-Right Corner Resize Handle (右下角枠リサイズ) */
    if (wnd->attr & WND_ATTR_RESIZE) {
        H rx = body.right - 14;
        H ry = body.bottom - 14;
        RECT grip_r = { rx, ry, body.right - 2, body.bottom - 2 };
        fill_rec(dev, &grip_r, COLOR_LTGRAY);

        /* 3 Diagonal hatch grip lines */
        drw_lin(dev, body.right - 12, body.bottom - 3, body.right - 3, body.bottom - 12);
        drw_lin(dev, body.right - 8,  body.bottom - 3, body.right - 3, body.bottom - 8);
        drw_lin(dev, body.right - 4,  body.bottom - 3, body.right - 3, body.bottom - 4);
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
            H title_h = (wnd->attr & WND_ATTR_TITLE) ? WND_TITLE_HEIGHT : 0;
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
        if (curr->visible) {
            H title_h = (curr->attr & WND_ATTR_TITLE) ? WND_TITLE_HEIGHT : 0;
            /* 1. Main window body (below title tab row) */
            if (x >= curr->bounds.left && x < curr->bounds.right &&
                y >= curr->bounds.top + title_h && y < curr->bounds.bottom) {
                return curr;
            }
            /* 2. Compact title tab area (only inside tab rect) */
            if ((curr->attr & WND_ATTR_TITLE) && whit_test_tab(curr, x, y)) {
                return curr;
            }
        }
        curr = curr->next;
    }
    return NULL;
}

WND* get_top_wnd(void) {
    return g_wnd_head;
}

WND* get_wnd_list(void) {
    return g_wnd_head;
}
