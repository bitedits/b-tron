/*
 * v_sys.c — BTRON 3.20 System Management Verification Suite
 *
 * Tests def_exc, ret_exc, get_cnf, set_cnf.
 */

#include "../btron_verify.h"
#include <btron/sys_mgmt.h>
#include <string.h>

#define S "SysMgmt"

static void dummy_exc_handler(VW exinf) {
    (void)exinf;
}

void vfy_suite_sys(void)
{
    /* ── def_exc ────────────────────────────────────────────── */
    ER er = def_exc(0, dummy_exc_handler);
    VFY_ASSERT_EQ(S, "def_exc(valid)", er, E_OK);

    er = def_exc(-1, dummy_exc_handler);
    VFY_ASSERT_EQ(S, "def_exc(bad_code)", er, ER_PAR);

    /* ── ret_exc ────────────────────────────────────────────── */
    ret_exc();
    vfy_record(S, "ret_exc()", 1, "");

    /* ── get_cnf / set_cnf ──────────────────────────────────── */
    char name[64];
    W sz = 0;
    er = get_cnf(CNF_SYS_NAME, name, &sz);
    VFY_ASSERT_EQ(S, "get_cnf(SYS_NAME)", er, E_OK);
    VFY_ASSERT_TRUE(S, "sys_name length > 0", sz > 0);

    int procs = 0;
    er = get_cnf(CNF_MAX_PROCS, &procs, &sz);
    VFY_ASSERT_EQ(S, "get_cnf(MAX_PROCS)", er, E_OK);
    VFY_ASSERT_TRUE(S, "procs > 0", procs > 0);

    int new_procs = 64;
    er = set_cnf(CNF_MAX_PROCS, &new_procs, sizeof(int));
    VFY_ASSERT_EQ(S, "set_cnf(MAX_PROCS)", er, E_OK);

    er = get_cnf(CNF_MAX_PROCS, &procs, &sz);
    VFY_ASSERT_EQ(S, "get_cnf verified", procs, 64);
}
