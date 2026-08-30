/*
 * B-TRON System Desktop Compositor: desktop.c
 * Pure Specification-based implementation of Sakamura BTRON / BTRON3 Architecture.
 */

#include <btron/desktop.h>
#include <btron/troncode.h>
#include <btron/vobj.h>
#include <btron/wnd.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <time.h>
#endif

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

ER init_desktop_vram(H width, H height, COLOR *vram_ptr) {
    g_desktop.width = width;
    g_desktop.height = height;
    g_desktop.screen = opn_dev_vram(width, height, vram_ptr);
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
    drw_tc_string(dev, 25, 60, "実身", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 12, 102, "実身・仮身", COLOR_WHITE, 0x00000000);

    RECT edit_icon = { 20, 130, 70, 175 };
    fill_rec(dev, &edit_icon, COLOR_LTGRAY);
    drw_rec(dev, &edit_icon);
    drw_tc_string(dev, 25, 140, "文書", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 10, 182, "基本エディタ", COLOR_WHITE, 0x00000000);

    RECT term_icon = { 20, 210, 70, 255 };
    fill_rec(dev, &term_icon, COLOR_LTGRAY);
    drw_rec(dev, &term_icon);
    drw_tc_string(dev, 25, 220, "端末", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 12, 262, "端末シェル", COLOR_WHITE, 0x00000000);

    RECT audio_icon = { 20, 290, 70, 335 };
    fill_rec(dev, &audio_icon, COLOR_LTGRAY);
    drw_rec(dev, &audio_icon);
    drw_tc_string(dev, 25, 300, "音響", COLOR_RED, 0x00000000);
    drw_tc_string(dev, 12, 342, "音響機器", COLOR_WHITE, 0x00000000);

    RECT chat_icon = { 20, 370, 70, 415 };
    fill_rec(dev, &chat_icon, COLOR_LTGRAY);
    drw_rec(dev, &chat_icon);
    drw_tc_string(dev, 25, 380, "対話", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 12, 422, "会話通信", COLOR_WHITE, 0x00000000);
}

void render_system_panel(GDEV *dev) {
    if (!dev) return;

    /* Top BTRON Panel Bar */
    RECT panel = { 0, 0, dev->width, 26 };
    fill_rec(dev, &panel, COLOR_LTGRAY);
    drw_lin(dev, 0, 25, dev->width, 25);

    /* TRON Logo & System Menu */
    RECT sys_btn = { 4, 3, 76, 22 };
    fill_rec(dev, &sys_btn, COLOR_GRAY);
    drw_rec(dev, &sys_btn);
    drw_tc_string(dev, 8, 5, "［B-TRON］", COLOR_WHITE, 0x00000000);

    /* Top Menus in Japanese */
    drw_tc_string(dev, 85, 5, "ファイル(F)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 175, 5, "編集(E)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 245, 5, "表示(V)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 315, 5, "実身・仮身(O)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 420, 5, "ウィンドウ(W)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 520, 5, "ヘルプ(H)", COLOR_BLACK, 0x00000000);

    /* System Real-Time Clock */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    drw_tc_string(dev, dev->width - 90, 8, time_buf, COLOR_WHITE, COLOR_NAVY);
#else
    drw_tc_string(dev, dev->width - 90, 8, "12:00:00", COLOR_WHITE, COLOR_NAVY);
#endif
}


#if (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0) && (defined(__arm__) || defined(__aarch64__))
#include <btron/tip.h>
#include <btron/apps.h>
#ifndef ARGB
#define ARGB(a,r,g,b) (((uint32_t)(a)<<24)|((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b))
#endif

static H g_mouse_x = 460;
static H g_mouse_y = 280;

void set_baremetal_mouse_pos(H x, H y) {
    if (x < 0) x = 0;
    if (x >= 1024) x = 1023;
    if (y < 0) y = 0;
    if (y >= 768) y = 767;
    g_mouse_x = x;
    g_mouse_y = y;
}

void get_baremetal_mouse_pos(H *x, H *y) {
    if (x) *x = g_mouse_x;
    if (y) *y = g_mouse_y;
}

void draw_baremetal_mouse_cursor(GDEV *screen, H mx, H my, H w, H h) {
    if (!screen) return;
    static const uint16_t cur_mask[16] = {
        0x8000, 0xC000, 0xE000, 0xF000,
        0xF800, 0xFC00, 0xFE00, 0xFF00,
        0xFF80, 0xFE00, 0xDF00, 0x8F80,
        0x0780, 0x03C0, 0x0180, 0x0000
    };
    static const uint16_t cur_outline[16] = {
        0xC000, 0xE000, 0xF000, 0xF800,
        0xFC00, 0xFE00, 0xFF00, 0xFF80,
        0xFFC0, 0xFFE0, 0xFF80, 0xDFC0,
        0xCFE0, 0x07E0, 0x03C0, 0x0180
    };
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            H px = mx + x;
            H py = my + y;
            if (px < 0 || px >= w || py < 0 || py >= h) continue;
            uint16_t bit = (0x8000 >> x);
            if (cur_mask[y] & bit) {
                screen->pixels[py * w + px] = COLOR_WHITE;
            } else if (cur_outline[y] & bit) {
                screen->pixels[py * w + px] = COLOR_BLACK;
            }
        }
    }
}

void redraw_baremetal_desktop(GDEV *screen, H w, H h) {
    if (!screen) return;
    render_desktop_background(screen);
    redraw_all_windows();

    /* Floating candidate window for active window */
    WND *top = get_top_wnd();
    if (top && top->focused && (tip_get_state() == TIP_STATE_CONVERTING || tip_get_state() == TIP_STATE_CANDIDATE_SELECT)) {
        tip_render_candidate_window(screen, tip_get_caret_x(), tip_get_caret_y());
    }

    render_system_panel(screen);

    /* Gold accent bar below top panel */
    RECT gold_bar = { 0, 26, (H)w, 28 };
    fill_rec(screen, &gold_bar, COLOR_GOLD);

    /* Color Test Bar (bottom 40px) */
    COLOR bars[8] = {
        COLOR_WHITE,
        COLOR_YELLOW,
        COLOR_CYAN,
        COLOR_GREEN,
        ARGB(0xFF, 0xFF, 0x00, 0xFF), /* Magenta */
        COLOR_RED,
        ARGB(0xFF, 0x00, 0x00, 0xFF), /* Blue */
        COLOR_BLACK
    };
    H bar_h = 40;
    H bar_y = (H)h - bar_h;
    for (int bi = 0; bi < 8; bi++) {
        H bx0 = (bi * (H)w) / 8;
        H bx1 = ((bi + 1) * (H)w) / 8;
        RECT br = { bx0, bar_y, bx1, (H)h };
        fill_rec(screen, &br, bars[bi]);
    }

    /* Dynamic Classic B-TRON Mouse Cursor Rendering */
    draw_baremetal_mouse_cursor(screen, g_mouse_x, g_mouse_y, w, h);

    /* Data Cache Barrier */
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" : : : "memory");
#else
    __asm__ volatile("dsb" : : : "memory");
#endif
}

GDEV* init_baremetal_desktop(uint32_t *fb, uint32_t w, uint32_t h) {
    if (!fb) return NULL;
    GDEV *screen = opn_dev_vram((H)w, (H)h, (COLOR*)fb);
    if (!screen) return NULL;

    init_wnd_mgr(screen);
    init_vobj_sys("./btron_store");
    tip_init();
    init_evt_sys();

    open_vobj_manager_window();
    open_t_editor_window();
    WND *w_cli = open_gterm_window();
    if (w_cli) top_wnd(w_cli);

    redraw_baremetal_desktop(screen, w, h);
    return screen;
}

void draw_btron_pattern(uint32_t *fb, uint32_t w, uint32_t h) {
    init_baremetal_desktop(fb, w, h);
}


#endif /* __arm__ */
