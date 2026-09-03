#ifndef _BTRON_T_EDITOR_H_
#define _BTRON_T_EDITOR_H_

#include <btron/types.h>
#include <btron/wnd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEDITOR_MAX_ROWS  1000
#define TEDITOR_MAX_LINES 1000
#define TEDITOR_MAX_COLS  512
#define TEDITOR_VIEW_ROWS 14
#define TEDITOR_VIEW_COLS 80

typedef struct {
    char lines[TEDITOR_MAX_ROWS][TEDITOR_MAX_COLS];
    int total_lines;

    int cursor_row;
    int cursor_col;

    /* Selection */
    BOOL sel_active;
    int sel_start_r, sel_start_c;
    int sel_end_r, sel_end_c;

    /* Scrolling */
    int scroll_row;
    int scroll_col;

    /* File State */
    char filename[128];
    BOOL is_modified;

    /* VOBJ Embed */
    BOOL has_vobj;
    char vobj_name[64];
} TEditor;

WND* open_t_editor_window(void);
WND* open_t_editor_window_with_file(const char *filepath);
TEditor* teditor_get_current(void);
int teditor_load_file(TEditor *ed, const char *filepath);
int teditor_save_file(TEditor *ed, const char *filepath);
int teditor_close_file(TEditor *ed);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_T_EDITOR_H_ */
