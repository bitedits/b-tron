/*
 * v_vobj.c — Virtual Object API Verification Suite
 *
 * Tests the VOBJ subsystem types and functions.
 */

#include "../btron_verify.h"
#include <btron/vobj.h>
#include <string.h>

#define S "VirtualObject"

void vfy_suite_vobj(void)
{
    /* ── VOBJ_TYPE enum values ──────────────────────────────── */
    VFY_ASSERT_EQ(S, "VOBJ_TYPE_TEXT",     VOBJ_TYPE_TEXT,     1);
    VFY_ASSERT_EQ(S, "VOBJ_TYPE_DRAW",     VOBJ_TYPE_DRAW,     2);
    VFY_ASSERT_EQ(S, "VOBJ_TYPE_EXEC",     VOBJ_TYPE_EXEC,     3);
    VFY_ASSERT_EQ(S, "VOBJ_TYPE_FOLDER",   VOBJ_TYPE_FOLDER,   4);
    VFY_ASSERT_EQ(S, "VOBJ_TYPE_TERMINAL", VOBJ_TYPE_TERMINAL, 5);

    /* ── Struct sizes ───────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(ROBJ)>0",      sizeof(ROBJ) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(VOBJ_LINK)>0", sizeof(VOBJ_LINK) > 0);

    /* ── init_vobj_sys(valid) -> E_OK ────────────────────────── */
    ER er = init_vobj_sys("/tmp/btron_vfy_vobj");
    VFY_ASSERT_EQ(S, "init_vobj_sys(valid)", er, E_OK);

    /* ── cre_robj -> non-NULL ────────────────────────────────── */
    ROBJ *robj = cre_robj("test_object", VOBJ_TYPE_TEXT);
    VFY_ASSERT_NOTNULL(S, "cre_robj(valid)", robj);

    if (robj) {
        /* Verify fields were set */
        VFY_ASSERT_TRUE(S, "robj->robj_id>0", robj->robj_id > 0);
        VFY_ASSERT_EQ(S, "robj->type", robj->type, VOBJ_TYPE_TEXT);

        /* ── Write data ──────────────────────────────────────── */
        const char test_data[] = "BTRON 3.20 verification data";
        er = wr_vobj_data(robj, test_data, (UW)strlen(test_data));
        VFY_ASSERT_EQ(S, "wr_vobj_data(valid)", er, E_OK);
        VFY_ASSERT_EQ(S, "robj->size", robj->size, (UW)strlen(test_data));

        /* ── Read data ───────────────────────────────────────── */
        char buf[128];
        UW read_bytes = 0;
        memset(buf, 0, sizeof(buf));
        er = rd_vobj_data(robj, buf, sizeof(buf), &read_bytes);
        VFY_ASSERT_EQ(S, "rd_vobj_data(valid)", er, E_OK);
        VFY_ASSERT_TRUE(S, "rd_vobj_data len>0", read_bytes > 0);

        /* ── wr_vobj_data(NULL buf) -> E_PAR ─────────────────── */
        er = wr_vobj_data(robj, NULL, 10);
        VFY_ASSERT_EQ(S, "wr_vobj_data(NULL)", er, E_PAR);

        /* ── rd_vobj_data(NULL buf) -> E_PAR ─────────────────── */
        er = rd_vobj_data(robj, NULL, 10, &read_bytes);
        VFY_ASSERT_EQ(S, "rd_vobj_data(NULL)", er, E_PAR);

        /* ── cls_robj -> E_OK ────────────────────────────────── */
        er = cls_robj(robj);
        VFY_ASSERT_EQ(S, "cls_robj(valid)", er, E_OK);
    }

    /* ── opn_robj with bad ID -> NULL ────────────────────────── */
    ROBJ *bad = opn_robj((ID)999999);
    VFY_ASSERT_NULL(S, "opn_robj(bad_id)", bad);

    /* ── cre_vobj_link -> non-NULL ───────────────────────────── */
    VOBJ_LINK *lnk = cre_vobj_link(1, "TestLink", 10, 20);
    VFY_ASSERT_NOTNULL(S, "cre_vobj_link(valid)", lnk);

    if (lnk) {
        VFY_ASSERT_EQ(S, "link->pos.x", lnk->pos.x, 10);
        VFY_ASSERT_EQ(S, "link->pos.y", lnk->pos.y, 20);
    }
}
