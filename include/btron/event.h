/*
 * B-TRON Specification Compatible Header: event.h
 * System Event Queue & Types.
 */

#ifndef _BTRON_EVENT_H_
#define _BTRON_EVENT_H_

#include <btron/types.h>
#include <btron/error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EV_NONE = 0,
    EV_BUT_DOWN,
    EV_BUT_UP,
    EV_MOUSE_MOVE,
    EV_KEY_DOWN,
    EV_KEY_UP,
    EV_WND_CLOSE,
    EV_WND_MOVE,
    EV_WND_FOCUS,
    EV_MENU_SELECT,
    EV_TIMER
} EV_TYPE;

typedef struct {
    EV_TYPE type;
    ID      wndid;
    PNT     pos;
    UW      key;
    UW      button;
    VW      data;
} EVT;

/* Standard Keyboard Modifier Bitmasks */
#define BTRON_KMOD_NONE   0x0000
#define BTRON_KMOD_LSHIFT 0x0001
#define BTRON_KMOD_RSHIFT 0x0002
#define BTRON_KMOD_SHIFT  0x0003
#define BTRON_KMOD_LCTRL  0x0040
#define BTRON_KMOD_RCTRL  0x0080
#define BTRON_KMOD_CTRL   0x00C0
#define BTRON_KMOD_LALT   0x0100
#define BTRON_KMOD_RALT   0x0200
#define BTRON_KMOD_CAPS   0x2000

/* Standard Functional Keys (SDL Scancode compatible) */
#define BTRON_KEY_F1      0x4000003A
#define BTRON_KEY_F2      0x4000003B
#define BTRON_KEY_F3      0x4000003C
#define BTRON_KEY_F4      0x4000003D
#define BTRON_KEY_F5      0x4000003E
#define BTRON_KEY_F6      0x4000003F /* Hiragana */
#define BTRON_KEY_F7      0x40000040 /* Fullwidth Katakana */
#define BTRON_KEY_F8      0x40000041 /* Halfwidth Katakana */
#define BTRON_KEY_F9      0x40000042 /* Fullwidth Alphanumeric */
#define BTRON_KEY_F10     0x40000043 /* Plane 0 <-> Plane 1 Toggle (EN <-> JP) */
#define BTRON_KEY_F11     0x40000044
#define BTRON_KEY_F12     0x40000045

ER init_evt_sys(void);
ER get_evt(EVT *p_evt, W timeout_ms);
ER snd_evt(const EVT *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_EVENT_H_ */
