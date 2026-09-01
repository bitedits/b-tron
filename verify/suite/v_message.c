/*
 * v_message.c — Message Passing Verification Suite
 *
 * The full snd_msg / rcv_msg API is [MISSING] per CERTIFICATION.md.
 * This suite verifies the structural requirements and event system
 * that exists as the current IPC mechanism.
 */

#include "../btron_verify.h"
#include <btron/event.h>

#define S "Message"

void vfy_suite_message(void)
{
    /* ── Event type enum validation ─────────────────────────── */
    VFY_ASSERT_EQ(S, "EV_NONE",        EV_NONE,        0);
    VFY_ASSERT_EQ(S, "EV_BUT_DOWN",    EV_BUT_DOWN,    1);
    VFY_ASSERT_EQ(S, "EV_BUT_UP",      EV_BUT_UP,      2);
    VFY_ASSERT_EQ(S, "EV_MOUSE_MOVE",  EV_MOUSE_MOVE,  3);
    VFY_ASSERT_EQ(S, "EV_KEY_DOWN",    EV_KEY_DOWN,    4);
    VFY_ASSERT_EQ(S, "EV_KEY_UP",      EV_KEY_UP,      5);
    VFY_ASSERT_EQ(S, "EV_WND_CLOSE",   EV_WND_CLOSE,   6);
    VFY_ASSERT_EQ(S, "EV_WND_MOVE",    EV_WND_MOVE,    7);
    VFY_ASSERT_EQ(S, "EV_WND_FOCUS",   EV_WND_FOCUS,   8);
    VFY_ASSERT_EQ(S, "EV_MENU_SELECT", EV_MENU_SELECT, 9);
    VFY_ASSERT_EQ(S, "EV_TIMER",       EV_TIMER,      10);

    /* ── EVT structure size ─────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(EVT)>0", sizeof(EVT) > 0);

    /* ── EVT field offsets — must contain all required fields ─ */
    EVT evt;
    memset(&evt, 0, sizeof(evt));
    evt.type   = EV_KEY_DOWN;
    evt.wndid  = 42;
    evt.pos.x  = 100;
    evt.pos.y  = 200;
    evt.key    = 0x41;
    evt.button = 1;

    VFY_ASSERT_EQ(S, "EVT.type",   evt.type,   EV_KEY_DOWN);
    VFY_ASSERT_EQ(S, "EVT.wndid",  evt.wndid,  42);
    VFY_ASSERT_EQ(S, "EVT.pos.x",  evt.pos.x,  100);
    VFY_ASSERT_EQ(S, "EVT.pos.y",  evt.pos.y,  200);
    VFY_ASSERT_EQ(S, "EVT.key",    evt.key,    0x41);
    VFY_ASSERT_EQ(S, "EVT.button", evt.button, 1);

    /* ── Keyboard constants ─────────────────────────────────── */
    VFY_ASSERT_EQ(S, "BTRON_KEY_BACKSPACE", BTRON_KEY_BACKSPACE, 8);
    VFY_ASSERT_EQ(S, "BTRON_KEY_TAB",       BTRON_KEY_TAB,       9);
    VFY_ASSERT_EQ(S, "BTRON_KEY_RETURN",     BTRON_KEY_RETURN,   13);
    VFY_ASSERT_EQ(S, "BTRON_KEY_ESCAPE",     BTRON_KEY_ESCAPE,   27);
    VFY_ASSERT_EQ(S, "BTRON_KEY_SPACE",      BTRON_KEY_SPACE,    32);
    VFY_ASSERT_EQ(S, "BTRON_KEY_DELETE",     BTRON_KEY_DELETE,  127);

    /* ── Modifier bitmask orthogonality ─────────────────────── */
    VFY_ASSERT_EQ(S, "KMOD_NONE",  BTRON_KMOD_NONE,  0);
    VFY_ASSERT_EQ(S, "KMOD_SHIFT", BTRON_KMOD_SHIFT,
                  BTRON_KMOD_LSHIFT | BTRON_KMOD_RSHIFT);
    VFY_ASSERT_EQ(S, "KMOD_CTRL",  BTRON_KMOD_CTRL,
                  BTRON_KMOD_LCTRL | BTRON_KMOD_RCTRL);

    /*
     * NOTE: Full snd_msg / rcv_msg / MSGBODY / MESSAGE are [MISSING].
     * When implemented, add:
     *   - snd_msg round-trip with ANYMSG body
     *   - MS_ABORT..MS_TYPE7 constant values
     *   - MSGMASK(t) macro behaviour
     */
}
