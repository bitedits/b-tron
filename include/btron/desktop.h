/*
 * B-TRON Specification Compatible Header: desktop.h
 * BTRON Desktop Shell & System Panel definitions.
 */

#ifndef _BTRON_DESKTOP_H_
#define _BTRON_DESKTOP_H_

#include <btron/types.h>
#include <btron/dp.h>
#include <btron/wnd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    H width;
    H height;
    GDEV *screen;
    BOOL running;
} BTRON_DESKTOP;

ER init_desktop(H width, H height);
ER init_desktop_vram(H width, H height, COLOR *vram_ptr);
void run_desktop_loop(void);
void render_desktop_background(GDEV *dev);
void render_system_panel(GDEV *dev);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_DESKTOP_H_ */
