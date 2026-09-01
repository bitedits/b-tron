/*
 * v_wnd.c — Window Manager API Verification Suite
 *
 * All Window Manager APIs are [IMPL] per CERTIFICATION.md.
 * Tests lifecycle, positioning, z-order, hit-testing, and tab management.
 */

#include "../btron_verify.h"
#include <btron/wnd.h>
#include <btron/dp.h>

#define S "WindowMgr"

void vfy_suite_wnd(void)
{
    /* ── WND_ATTR flags ─────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "WND_ATTR_TITLE",       WND_ATTR_TITLE,       (1 << 0));
    VFY_ASSERT_EQ(S, "WND_ATTR_CLOSE",       WND_ATTR_CLOSE,       (1 << 1));
    VFY_ASSERT_EQ(S, "WND_ATTR_MAX",         WND_ATTR_MAX,         (1 << 2));
    VFY_ASSERT_EQ(S, "WND_ATTR_RESIZE",      WND_ATTR_RESIZE,      (1 << 3));
    VFY_ASSERT_EQ(S, "WND_ATTR_BORDER",      WND_ATTR_BORDER,      (1 << 4));
    VFY_ASSERT_EQ(S, "WND_ATTR_COMPACT_TAB", WND_ATTR_COMPACT_TAB, (1 << 5));
    VFY_ASSERT_EQ(S, "WND_ATTR_SLIDING_TAB", WND_ATTR_SLIDING_TAB, (1 << 6));

    /* ── WND struct size ────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(WND)>0", sizeof(WND) > 0);

    /* ── init_wnd_mgr(NULL) -> E_PAR ────────────────────────── */
    ER er = init_wnd_mgr(NULL);
    VFY_ASSERT_EQ(S, "init_wnd_mgr(NULL)", er, E_PAR);

    /* ── Setup test display device ──────────────────────────── */
    GDEV *screen = opn_dev(640, 480);
    VFY_ASSERT_NOTNULL(S, "screen_dev", screen);

    if (screen) {
        er = init_wnd_mgr(screen);
        VFY_ASSERT_EQ(S, "init_wnd_mgr(valid)", er, E_OK);

        /* ── opn_wnd ────────────────────────────────────────── */
        UW attr = WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE;
        WND *w1 = opn_wnd("Test Window 1", 50, 50, 200, 150, attr);
        VFY_ASSERT_NOTNULL(S, "opn_wnd(w1)", w1);

        WND *w2 = opn_wnd("Test Window 2", 100, 100, 250, 180, attr);
        VFY_ASSERT_NOTNULL(S, "opn_wnd(w2)", w2);

        if (w1 && w2) {
            /* ── Bounds validation ──────────────────────────── */
            VFY_ASSERT_EQ(S, "w1->bounds.left",   w1->bounds.left,   50);
            VFY_ASSERT_EQ(S, "w1->bounds.top",    w1->bounds.top,    50);
            VFY_ASSERT_EQ(S, "w1->bounds.right",  w1->bounds.right,  250);
            VFY_ASSERT_EQ(S, "w1->bounds.bottom", w1->bounds.bottom, 200);

            /* ── get_top_wnd ────────────────────────────────── */
            WND *top = get_top_wnd();
            VFY_ASSERT_TRUE(S, "get_top_wnd==w2", top == w2);

            /* ── top_wnd (bring w1 to top) ──────────────────── */
            er = top_wnd(w1);
            VFY_ASSERT_EQ(S, "top_wnd(w1)", er, E_OK);
            top = get_top_wnd();
            VFY_ASSERT_TRUE(S, "top_wnd w1 is top", top == w1);

            /* ── mov_wnd ────────────────────────────────────── */
            er = mov_wnd(w1, 80, 90);
            VFY_ASSERT_EQ(S, "mov_wnd(w1, 80, 90)", er, E_OK);
            VFY_ASSERT_EQ(S, "w1 moved left", w1->bounds.left, 80);
            VFY_ASSERT_EQ(S, "w1 moved top",  w1->bounds.top,  90);

            /* ── rsz_wnd ────────────────────────────────────── */
            er = rsz_wnd(w1, 300, 200);
            VFY_ASSERT_EQ(S, "rsz_wnd(w1, 300, 200)", er, E_OK);
            VFY_ASSERT_EQ(S, "w1 width resized",  w1->bounds.right - w1->bounds.left, 300);
            VFY_ASSERT_EQ(S, "w1 height resized", w1->bounds.bottom - w1->bounds.top, 200);

            /* ── find_wnd_at ────────────────────────────────── */
            WND *found = find_wnd_at(90, 100);
            VFY_ASSERT_TRUE(S, "find_wnd_at inside w1", found == w1);

            /* ── Hit testing ────────────────────────────────── */
            RECT tab_rect;
            er = wget_tab_rect(w1, &tab_rect);
            VFY_ASSERT_EQ(S, "wget_tab_rect", er, E_OK);

            /* ── inval_wnd & redraw_all_windows ─────────────── */
            er = inval_wnd(w1);
            VFY_ASSERT_EQ(S, "inval_wnd(w1)", er, E_OK);
            redraw_all_windows();
            vfy_record(S, "redraw_all_windows()", 1, "");

            /* ── cls_wnd ────────────────────────────────────── */
            er = cls_wnd(w1);
            VFY_ASSERT_EQ(S, "cls_wnd(w1)", er, E_OK);

            er = cls_wnd(w2);
            VFY_ASSERT_EQ(S, "cls_wnd(w2)", er, E_OK);
        }

        /* ── cls_wnd(NULL) -> E_PAR ─────────────────────────── */
        er = cls_wnd(NULL);
        VFY_ASSERT_EQ(S, "cls_wnd(NULL)", er, E_PAR);

        cls_dev(screen);
    }
}
