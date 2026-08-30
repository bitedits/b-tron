#ifndef _BTRON_APPS_H_
#define _BTRON_APPS_H_

#include <btron/wnd.h>

WND* open_t_editor_window(void);
WND* open_gterm_window(void);
WND* open_vobj_manager_window(void);
WND* open_audio_player_window(void);
WND* open_tad_browser_window(const char *filepath, const char *title);
void launch_beos_chat(void);

#endif /* _BTRON_APPS_H_ */
