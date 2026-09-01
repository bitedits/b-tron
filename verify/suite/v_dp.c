/*
 * v_dp.c — Display Primitives Verification Suite
 *
 * Tests opn_dev / cls_dev / drawing operations / ROP and COLOR constants.
 * Most DP functions are [IMPL] per CERTIFICATION.md.
 */

#include "../btron_verify.h"
#include <btron/dp.h>

#define S "DisplayPrim"

void vfy_suite_dp(void)
{
    /* ── ROP constant values ────────────────────────────────── */
    VFY_ASSERT_EQ(S, "ROP_COPY",   ROP_COPY,   0);
    VFY_ASSERT_EQ(S, "ROP_OR",     ROP_OR,     1);
    VFY_ASSERT_EQ(S, "ROP_XOR",    ROP_XOR,    2);
    VFY_ASSERT_EQ(S, "ROP_AND",    ROP_AND,    3);
    VFY_ASSERT_EQ(S, "ROP_INVERT", ROP_INVERT, 4);

    /* ── COLOR palette constants ────────────────────────────── */
    VFY_ASSERT_EQ(S, "COLOR_BLACK", (long long)COLOR_BLACK, (long long)0xFF000000UL);
    VFY_ASSERT_EQ(S, "COLOR_WHITE", (long long)COLOR_WHITE, (long long)0xFFFFFFFFUL);
    VFY_ASSERT_EQ(S, "COLOR_TEAL",  (long long)COLOR_TEAL,  (long long)0xFF008080UL);
    VFY_ASSERT_EQ(S, "COLOR_NAVY",  (long long)COLOR_NAVY,  (long long)0xFF000080UL);

    /* ── GDEV struct size ───────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(GDEV)>0", sizeof(GDEV) > 0);

    /* ── opn_dev(0,0) → NULL (invalid dimensions) ──────────── */
    GDEV *bad = opn_dev(0, 0);
    VFY_ASSERT_NULL(S, "opn_dev(0,0)", bad);

    /* ── opn_dev(320,240) → valid GDEV ──────────────────────── */
    GDEV *dev = opn_dev(320, 240);
    VFY_ASSERT_NOTNULL(S, "opn_dev(320,240)", dev);

    if (dev) {
        VFY_ASSERT_EQ(S, "dev->width",  dev->width,  320);
        VFY_ASSERT_EQ(S, "dev->height", dev->height, 240);
        VFY_ASSERT_NOTNULL(S, "dev->pixels", dev->pixels);

        /* ── fill_rec on valid rect → E_OK ──────────────────── */
        RECT r = { 10, 10, 50, 50 };
        ER er = fill_rec(dev, &r, COLOR_TEAL);
        VFY_ASSERT_EQ(S, "fill_rec(valid)", er, E_OK);

        /* Verify pixel was actually written */
        if (dev->pixels) {
            COLOR px = dev->pixels[10 * 320 + 10];
            VFY_ASSERT_EQ(S, "fill_rec pixel", (long long)px, (long long)COLOR_TEAL);
        }

        /* ── drw_lin → E_OK ─────────────────────────────────── */
        er = drw_lin(dev, 0, 0, 319, 239);
        VFY_ASSERT_EQ(S, "drw_lin(valid)", er, E_OK);

        /* ── drw_rec → E_OK ─────────────────────────────────── */
        RECT r2 = { 5, 5, 100, 100 };
        er = drw_rec(dev, &r2);
        VFY_ASSERT_EQ(S, "drw_rec(valid)", er, E_OK);

        /* ── drw_pnt → E_OK ─────────────────────────────────── */
        er = drw_pnt(dev, 160, 120);
        VFY_ASSERT_EQ(S, "drw_pnt(valid)", er, E_OK);

        /* ── set_clip ───────────────────────────────────────── */
        RECT clip = { 20, 20, 200, 200 };
        set_clip(dev, &clip);
        set_col(dev, COLOR_BLACK, COLOR_WHITE);
        vfy_record(S, "set_col(valid)", 1, "");
        PAT p = { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 } };
        set_pat(dev, &p);
        vfy_record(S, "set_pat(valid)", 1, "");
        VFY_ASSERT_EQ(S, "set_clip.left",  dev->clip.left,  20);
        VFY_ASSERT_EQ(S, "set_clip.right", dev->clip.right, 200);

        /* ── cls_dev → cleanup ──────────────────────────────── */
        cls_dev(dev);
        vfy_record(S, "cls_dev(valid)", 1, "");
    }

    /* ── cls_dev(NULL) → no crash ───────────────────────────── */
    cls_dev(NULL);
    vfy_record(S, "cls_dev(NULL)", 1, "");

    /* ── opn_dev_vram ───────────────────────────────────────── */
    COLOR vram_buf[64 * 64];
    memset(vram_buf, 0, sizeof(vram_buf));
    GDEV *vram_dev = opn_dev_vram(64, 64, vram_buf);
    VFY_ASSERT_NOTNULL(S, "opn_dev_vram(64,64)", vram_dev);
    if (vram_dev) {
        VFY_ASSERT_TRUE(S, "vram_dev->pixels==buf", vram_dev->pixels == vram_buf);
        cls_dev(vram_dev);
    }
}
