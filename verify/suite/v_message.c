/*
 * v_message.c — BTRON 3.20 Message Passing (IPC) & Event Suite
 *
 * Full behavioral testing of snd_msg, rcv_msg, chk_msg, MSGBODY, and MESSAGE.
 */

#include "../btron_verify.h"
#include <btron/message.h>
#include <btron/event.h>
#include <string.h>

#define S "Message"

void vfy_suite_message(void)
{
    /* ── Message Type Constants ─────────────────────────────── */
    VFY_ASSERT_EQ(S, "MS_ABORT",   MS_ABORT,   1);
    VFY_ASSERT_EQ(S, "MS_EXIT",    MS_EXIT,    2);
    VFY_ASSERT_EQ(S, "MS_TERM",    MS_TERM,    3);
    VFY_ASSERT_EQ(S, "MS_TMOUT",   MS_TMOUT,   4);
    VFY_ASSERT_EQ(S, "MS_SYSEVT",  MS_SYSEVT,  5);
    VFY_ASSERT_EQ(S, "MS_TYPE1",   MS_TYPE1,   25);
    VFY_ASSERT_EQ(S, "MS_TYPE7",   MS_TYPE7,   31);

    /* ── MSGMASK macro ──────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "MSGMASK(1)",  MSGMASK(1),  (1U << 0));
    VFY_ASSERT_EQ(S, "MSGMASK(5)",  MSGMASK(5),  (1U << 4));
    VFY_ASSERT_EQ(S, "MSGMASK(31)", MSGMASK(31), (1U << 30));

    /* ── Structure Sizes ────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(MSGBODY)>0", sizeof(MSGBODY) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(MESSAGE)>0", sizeof(MESSAGE) > 0);

    /* ── snd_msg / rcv_msg round-trip ───────────────────────── */
    MESSAGE send_m;
    memset(&send_m, 0, sizeof(send_m));
    send_m.msg_type = MS_TYPE1;
    send_m.msg_size = 16;
    strcpy((char*)send_m.msg_body.ANYMSG.msg_str, "Hello BTRON3");

    /* Send to PID 2 */
    ER er = snd_msg(2, &send_m);
    VFY_ASSERT_EQ(S, "snd_msg(valid)", er, E_OK);

    /* Non-blocking peek with chk_msg */
    MESSAGE recv_m;
    memset(&recv_m, 0, sizeof(recv_m));
    er = chk_msg(2, &recv_m, MSGMASK(MS_TYPE1));
    VFY_ASSERT_EQ(S, "chk_msg(found)", er, E_OK);
    VFY_ASSERT_EQ(S, "chk_msg.type", recv_m.msg_type, MS_TYPE1);
    VFY_ASSERT_TRUE(S, "chk_msg payload",
                    strcmp((char*)recv_m.msg_body.ANYMSG.msg_str, "Hello BTRON3") == 0);

    /* Queue should now be empty for PID 2 */
    er = chk_msg(2, &recv_m, MSGMASK_ALL);
    VFY_ASSERT_EQ(S, "chk_msg(empty)", er, E_TMOUT);

    /* ── Error parameter checks ─────────────────────────────── */
    er = snd_msg(2, NULL);
    VFY_ASSERT_EQ(S, "snd_msg(NULL)", er, E_PAR);

    er = snd_msg(-1, &send_m);
    VFY_ASSERT_EQ(S, "snd_msg(bad_pid)", er, ER_ID);

    er = rcv_msg(2, NULL, MSGMASK_ALL, 0);
    VFY_ASSERT_EQ(S, "rcv_msg(NULL)", er, E_PAR);

    /* ── Event subsystem basic definitions ──────────────────── */
    VFY_ASSERT_EQ(S, "EV_NONE",        EV_NONE,        0);
    VFY_ASSERT_EQ(S, "EV_BUT_DOWN",    EV_BUT_DOWN,    1);
    VFY_ASSERT_EQ(S, "EV_KEY_DOWN",    EV_KEY_DOWN,    4);
    VFY_ASSERT_EQ(S, "BTRON_KEY_RETURN", BTRON_KEY_RETURN, 13);
}
