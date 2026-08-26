/*
 * B-TRON Specification Compatible Header: wnd.h
 * Sakamura Window Manager Primitives & Structure definitions.
 */

#ifndef _BTRON_WND_H_
#define _BTRON_WND_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/dp.h>
#include <btron/event.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WND_ATTR_TITLE    (1 << 0)
#define WND_ATTR_CLOSE    (1 << 1)
#define WND_ATTR_MAX      (1 << 2)
#define WND_ATTR_RESIZE   (1 << 3)
#define WND_ATTR_BORDER   (1 << 4)

typedef struct WND {
    ID    id;
    char  pad0[12];    /* 12 bytes alignment padding -> title starts at offset 16 (16-byte aligned!) */
    char  title[64];
    RECT  bounds;      /* starts at offset 80 (8-byte aligned!) */
    RECT  client;      /* starts at offset 88 (8-byte aligned!) */
    UW    attr;
    BOOL  visible;
    BOOL  focused;
    UW    pad1;        /* 4 bytes alignment padding for 8-byte pointer boundary! */
    GDEV  *dev;        /* starts at offset 112 (8-byte aligned!) */
    void (*paint)(struct WND *wnd, GDEV *dev);
    void (*event_handler)(struct WND *wnd, const EVT *evt);
    VW    user_data;
    struct WND *next;
    struct WND *prev;
} WND;

ER   init_wnd_mgr(GDEV *screen_dev);
WND* opn_wnd(const char *title, H x, H y, H w, H h, UW attr);
ER   cls_wnd(WND *wnd);
ER   top_wnd(WND *wnd);
ER   mov_wnd(WND *wnd, H x, H y);
ER   inval_wnd(WND *wnd);

void redraw_all_windows(void);
WND* find_wnd_at(H x, H y);
WND* get_top_wnd(void);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_WND_H_ */
