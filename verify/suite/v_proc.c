/*
 * v_proc.c — BTRON 3.20 Process Management Verification Suite
 *
 * Tests cre_prc, ter_prc, chg_pri, get_tid, get_pid, P_USER, LINK.
 */

#include "../btron_verify.h"
#include <btron/proc.h>
#include <string.h>

#define S "Process"

void vfy_suite_proc(void)
{
    /* ── Structure Sizes ────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(P_USER)>0", sizeof(P_USER) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(LINK)>0",   sizeof(LINK) > 0);

    /* ── Process creation ───────────────────────────────────── */
    LINK lnk;
    memset(&lnk, 0, sizeof(lnk));
    lnk.vol_id = 1;
    lnk.rec_id = 100;

    MESSAGE msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_type = MS_TYPE1;

    ID pid = cre_prc(&lnk, 10, &msg);
    VFY_ASSERT_TRUE(S, "cre_prc(valid)>0", pid > 0);

    if (pid > 0) {
        /* Verify launch message was delivered to child process */
        MESSAGE recv_m;
        ER er = chk_msg(pid, &recv_m, MSGMASK_ALL);
        VFY_ASSERT_EQ(S, "cre_prc launch msg", er, E_OK);

        /* Change priority */
        er = chg_pri(pid, 20, 0);
        VFY_ASSERT_EQ(S, "chg_pri(valid)", er, E_OK);

        /* Terminate process */
        er = ter_prc(pid, 0, 0);
        VFY_ASSERT_EQ(S, "ter_prc(valid)", er, E_OK);

        /* Second termination should return error */
        er = ter_prc(pid, 0, 0);
        VFY_ASSERT_EQ(S, "ter_prc(already_dead)", er, ER_NOEXS);
    }

    /* Invalid priority */
    ID bad_pri = cre_prc(&lnk, -5, NULL);
    VFY_ASSERT_EQ(S, "cre_prc(bad_pri)", bad_pri, ER_PAR);

    /* Invalid pid termination */
    ER bad_ter = ter_prc(-1, 0, 0);
    VFY_ASSERT_EQ(S, "ter_prc(bad_pid)", bad_ter, ER_ID);

    /* Current IDs */
    VFY_ASSERT_TRUE(S, "get_tid()", get_tid() >= 0);
    VFY_ASSERT_TRUE(S, "get_pid()", get_pid() >= 0);
}
