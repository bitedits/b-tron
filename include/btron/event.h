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

ER init_evt_sys(void);
ER get_evt(EVT *p_evt, W timeout_ms);
ER snd_evt(const EVT *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_EVENT_H_ */
