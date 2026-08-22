/*
 * B-TRON Retro OS Environment Main Launcher & Event Loop
 */

#include <btron/btron.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations for BTRON Accessories */
extern WND* open_vobj_manager_window(void);
extern WND* open_t_editor_window(void);
extern WND* open_gterm_window(void);

extern BOOL init_sdl_backend(H width, H height, const char *title);
extern void flush_gdev_to_sdl(GDEV *dev);
extern void shutdown_sdl_backend(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    H screen_w = 1024;
    H screen_h = 768;

    printf("[B-TRON] Initializing B-TRON Retro OS Environment...\n");

    if (!init_sdl_backend(screen_w, screen_h, "B-TRON Retro OS Desktop Environment (SDL2)")) {
        fprintf(stderr, "[B-TRON] Failed to initialize SDL2 backend.\n");
        return 1;
    }

    if (init_desktop(screen_w, screen_h) != E_OK) {
        fprintf(stderr, "[B-TRON] Desktop initialization failed.\n");
        shutdown_sdl_backend();
        return 1;
    }

    init_evt_sys();

    printf("[B-TRON] Launching Sakamura BTRON Desktop & Accessories...\n");

    BOOL running = TRUE;
    EVT ev;

    BOOL dragging = FALSE;
    WND *drag_wnd = NULL;
    H drag_off_x = 0, drag_off_y = 0;

    GDEV *screen_dev = opn_dev(screen_w, screen_h);
    init_wnd_mgr(screen_dev);

    /* Open initial BTRON desktop accessories */
    open_vobj_manager_window();
    open_t_editor_window();
    open_gterm_window();

    while (running) {
        /* Poll and process system events */
        if (get_evt(&ev, 16) == E_OK) {
            if (ev.type == EV_WND_CLOSE) {
                running = FALSE;
            } else if (ev.type == EV_BUT_DOWN) {
                WND *clicked = find_wnd_at(ev.pos.x, ev.pos.y);
                if (clicked) {
                    top_wnd(clicked);

                    /* Titlebar area drag check */
                    if (ev.pos.y >= clicked->bounds.top && ev.pos.y < clicked->bounds.top + 22) {
                        /* Close button click check */
                        if (ev.pos.x >= clicked->bounds.right - 20 && ev.pos.x < clicked->bounds.right - 6) {
                            cls_wnd(clicked);
                        } else {
                            dragging = TRUE;
                            drag_wnd = clicked;
                            drag_off_x = ev.pos.x - clicked->bounds.left;
                            drag_off_y = ev.pos.y - clicked->bounds.top;
                        }
                    }
                } else if (ev.pos.y > 50 && ev.pos.y < 100 && ev.pos.x < 70) {
                    /* Desktop Cabinet Icon click -> Open VOBJ Manager */
                    open_vobj_manager_window();
                } else if (ev.pos.y > 130 && ev.pos.y < 180 && ev.pos.x < 70) {
                    /* Desktop T-Editor Icon click -> Open Editor */
                    open_t_editor_window();
                } else if (ev.pos.y > 210 && ev.pos.y < 260 && ev.pos.x < 70) {
                    /* Desktop Terminal Icon click -> Open Terminal */
                    open_gterm_window();
                }
            } else if (ev.type == EV_BUT_UP) {
                dragging = FALSE;
                drag_wnd = NULL;
            } else if (ev.type == EV_MOUSE_MOVE && dragging && drag_wnd) {
                mov_wnd(drag_wnd, ev.pos.x - drag_off_x, ev.pos.y - drag_off_y);
            }
        }

        /* Render full BTRON desktop composition */
        render_desktop_background(screen_dev);
        redraw_all_windows();
        render_system_panel(screen_dev);

        /* Flush composite buffer to SDL window */
        flush_gdev_to_sdl(screen_dev);
    }

    printf("[B-TRON] Shutting down B-TRON Retro OS Environment.\n");
    cls_dev(screen_dev);
    shutdown_sdl_backend();
    return 0;
}
