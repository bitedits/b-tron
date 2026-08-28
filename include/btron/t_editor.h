/*
 * BTRON CUA Text Editor Header: t_editor.h
 * Shared interface between T-Editor and UI Test Suites.
 */

#ifndef _BTRON_T_EDITOR_H_
#define _BTRON_T_EDITOR_H_

#include <btron/types.h>
#include <btron/wnd.h>

#define TEDITOR_MAX_LINES 500
#define TEDITOR_MAX_COLS  512
#define TEDITOR_VIEW_ROWS 14
#define TEDITOR_VIEW_COLS 80

typedef struct {
    char lines[TEDITOR_MAX_LINES][TEDITOR_MAX_COLS];
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

void teditor_init_default(TEditor *ed);
int  teditor_get_char_width(const char *str, int *out_bytes);
int  teditor_find_byte_offset_from_x(const char *line, int target_x);
int  teditor_find_x_from_byte_offset(const char *line, int byte_offset);
int  utf8_next_offset(const char *s, int cur_offset);
int  utf8_prev_offset(const char *s, int cur_offset);
void teditor_ensure_cursor_visible(TEditor *ed);
void teditor_insert_char(TEditor *ed, char c);
void teditor_insert_text(TEditor *ed, const char *txt);
void teditor_insert_newline(TEditor *ed);
void teditor_delete_char_forward(TEditor *ed);
void teditor_backspace(TEditor *ed);
void teditor_move_cursor(TEditor *ed, int dr, int dc, BOOL select);
void teditor_delete_selection(TEditor *ed);
void teditor_copy_selection(TEditor *ed);
void teditor_paste_clipboard(TEditor *ed);
char get_ascii_char_with_shift(UW key, uint16_t mod);
extern char g_clipboard[2048];
void handle_t_editor_event(WND *wnd, const EVT *evt);
void paint_t_editor(WND *wnd, GDEV *dev);
WND* open_t_editor_window(void);

#endif /* _BTRON_T_EDITOR_H_ */
