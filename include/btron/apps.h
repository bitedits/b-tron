#ifndef _BTRON_APPS_H_
#define _BTRON_APPS_H_

#include <btron/wnd.h>
#include <btron/core.h>

WND* open_t_editor_window(void);
WND* open_gterm_window(void);
WND* open_vobj_manager_window(void);
WND* open_audio_player_window(void);
WND* open_tad_browser_window(const char *filepath, const char *title);
WND* launch_beos_chat(void);

void shell_execute_cmd(const char *cmd_line, ShellOutputFn out_fn, void *user_data, WND *wnd);

/* Kernel Info & Hardware Query Interfaces */
void sys_get_devconf(char *buf, size_t bufsz);
void sys_get_mem_stats(uint32_t *base, uint32_t *limit, uint32_t *used);
void sys_mouse_get_pos(H *x, H *y);
void sys_mouse_set_pos(H x, H y);
void sys_mouse_click(H x, H y);

#endif /* _BTRON_APPS_H_ */
