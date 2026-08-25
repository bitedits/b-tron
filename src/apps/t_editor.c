/*
 * BTRON Accessory: TRON CUA Text Editor Window (t_editor)
 * Pure Specification-based implementation of Sakamura BTRON / BTRON3 Architecture.
 */

#include <btron/wnd.h>
#include <btron/troncode.h>
#include <btron/dp.h>
#include <btron/event.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>

#define TEDITOR_MAX_LINES 300
#define TEDITOR_MAX_COLS  200
#define TEDITOR_VIEW_ROWS 14
#define TEDITOR_VIEW_COLS 50

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

static TEditor g_teditor;
static char g_clipboard[2048] = "";

static void teditor_init_default(TEditor *ed) {
    memset(ed, 0, sizeof(TEditor));
    strncpy(ed->filename, "README.txt", sizeof(ed->filename) - 1);

    const char *initial_doc[] = {
        "B-TRON Specification Text Editor (T-Editor)",
        "============================================",
        "TRON (The Real-time Operating system Nucleus)",
        "Personal computer OS architecture designed for",
        "human-machine interaction and multilingual support.",
        "",
        "CUA Editing Features Supported:",
        " - Standard Navigation: Arrows, Home, End, PageUp/Down",
        " - Selection: Shift + Arrow keys",
        " - Clipboard: Ctrl+C (Copy), Ctrl+X (Cut), Ctrl+V (Paste)",
        " - Select All: Ctrl+A",
        " - File Control: Ctrl+S (Save), Ctrl+N (New)",
        "",
        "Hyper-Data VOBJ Embed Status: Active [VOBJ: Diagram.draw]"
    };

    int n = sizeof(initial_doc) / sizeof(initial_doc[0]);
    ed->total_lines = n;
    for (int i = 0; i < n; i++) {
        strncpy(ed->lines[i], initial_doc[i], TEDITOR_MAX_COLS - 1);
    }

    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->scroll_row = 0;
    ed->scroll_col = 0;
    ed->has_vobj = TRUE;
    strncpy(ed->vobj_name, "Diagram.draw", sizeof(ed->vobj_name) - 1);
}

static void teditor_ensure_cursor_visible(TEditor *ed) {
    if (ed->cursor_row < ed->scroll_row) {
        ed->scroll_row = ed->cursor_row;
    }
    if (ed->cursor_row >= ed->scroll_row + TEDITOR_VIEW_ROWS) {
        ed->scroll_row = ed->cursor_row - TEDITOR_VIEW_ROWS + 1;
    }
    if (ed->scroll_row < 0) ed->scroll_row = 0;

    if (ed->cursor_col < ed->scroll_col) {
        ed->scroll_col = ed->cursor_col;
    }
    if (ed->cursor_col >= ed->scroll_col + TEDITOR_VIEW_COLS) {
        ed->scroll_col = ed->cursor_col - TEDITOR_VIEW_COLS + 1;
    }
    if (ed->scroll_col < 0) ed->scroll_col = 0;
}

static void teditor_delete_selection(TEditor *ed) {
    if (!ed->sel_active) return;

    int r1 = ed->sel_start_r, c1 = ed->sel_start_c;
    int r2 = ed->sel_end_r, c2 = ed->sel_end_c;

    if (r1 > r2 || (r1 == r2 && c1 > c2)) {
        int tr = r1; r1 = r2; r2 = tr;
        int tc = c1; c1 = c2; c2 = tc;
    }

    if (r1 == r2) {
        int len = (int)strlen(ed->lines[r1]);
        if (c1 < len) {
            int remove_len = (c2 > len ? len : c2) - c1;
            memmove(&ed->lines[r1][c1], &ed->lines[r1][c1 + remove_len], len - (c1 + remove_len) + 1);
        }
    } else {
        /* Multi-line deletion */
        int tail_len = (int)strlen(ed->lines[r2]);
        int keep_c2 = (c2 < tail_len) ? c2 : tail_len;

        /* Append remainder of r2 to r1 */
        strncpy(&ed->lines[r1][c1], &ed->lines[r2][keep_c2], TEDITOR_MAX_COLS - c1 - 1);

        /* Remove lines from r1+1 to r2 */
        int remove_lines = r2 - r1;
        for (int i = r1 + 1; i + remove_lines < ed->total_lines; i++) {
            strncpy(ed->lines[i], ed->lines[i + remove_lines], TEDITOR_MAX_COLS - 1);
        }
        ed->total_lines -= remove_lines;
        if (ed->total_lines < 1) ed->total_lines = 1;
    }

    ed->cursor_row = r1;
    ed->cursor_col = c1;
    ed->sel_active = FALSE;
    ed->is_modified = TRUE;
}

static void teditor_copy_selection(TEditor *ed) {
    if (!ed->sel_active) {
        /* Copy current line if no selection */
        strncpy(g_clipboard, ed->lines[ed->cursor_row], sizeof(g_clipboard) - 1);
        return;
    }

    int r1 = ed->sel_start_r, c1 = ed->sel_start_c;
    int r2 = ed->sel_end_r, c2 = ed->sel_end_c;
    if (r1 > r2 || (r1 == r2 && c1 > c2)) {
        int tr = r1; r1 = r2; r2 = tr;
        int tc = c1; c1 = c2; c2 = tc;
    }

    g_clipboard[0] = '\0';
    if (r1 == r2) {
        int len = (int)strlen(ed->lines[r1]);
        int end_c = (c2 < len) ? c2 : len;
        if (c1 < end_c) {
            snprintf(g_clipboard, sizeof(g_clipboard), "%.*s", end_c - c1, &ed->lines[r1][c1]);
        }
    } else {
        int pos = 0;
        for (int r = r1; r <= r2 && r < ed->total_lines; r++) {
            const char *str = ed->lines[r];
            int len = (int)strlen(str);
            int start = (r == r1) ? c1 : 0;
            int end = (r == r2) ? ((c2 < len) ? c2 : len) : len;
            if (start < end) {
                int count = end - start;
                if (pos + count < (int)sizeof(g_clipboard) - 2) {
                    strncpy(&g_clipboard[pos], &str[start], count);
                    pos += count;
                }
            }
            if (r < r2 && pos < (int)sizeof(g_clipboard) - 2) {
                g_clipboard[pos++] = '\n';
                g_clipboard[pos] = '\0';
            }
        }
    }
}

static void teditor_paste_clipboard(TEditor *ed) {
    if (strlen(g_clipboard) == 0) return;
    if (ed->sel_active) teditor_delete_selection(ed);

    char *copy = strdup(g_clipboard);
    if (!copy) return;

    char *line = strtok(copy, "\n");
    BOOL first = TRUE;
    while (line) {
        if (!first) {
            /* Split current line and insert new line */
            if (ed->total_lines < TEDITOR_MAX_LINES - 1) {
                for (int i = ed->total_lines; i > ed->cursor_row + 1; i--) {
                    strncpy(ed->lines[i], ed->lines[i - 1], TEDITOR_MAX_COLS - 1);
                }
                ed->total_lines++;
                ed->cursor_row++;
                strncpy(ed->lines[ed->cursor_row], &ed->lines[ed->cursor_row - 1][ed->cursor_col], TEDITOR_MAX_COLS - 1);
                ed->lines[ed->cursor_row - 1][ed->cursor_col] = '\0';
                ed->cursor_col = 0;
            }
        }

        int insert_len = (int)strlen(line);
        int cur_len = (int)strlen(ed->lines[ed->cursor_row]);
        if (cur_len + insert_len < TEDITOR_MAX_COLS - 1) {
            memmove(&ed->lines[ed->cursor_row][ed->cursor_col + insert_len],
                    &ed->lines[ed->cursor_row][ed->cursor_col],
                    cur_len - ed->cursor_col + 1);
            memcpy(&ed->lines[ed->cursor_row][ed->cursor_col], line, insert_len);
            ed->cursor_col += insert_len;
        }

        first = FALSE;
        line = strtok(NULL, "\n");
    }

    free(copy);
    ed->is_modified = TRUE;
    teditor_ensure_cursor_visible(ed);
}

static void teditor_insert_char(TEditor *ed, char ch) {
    if (ed->sel_active) teditor_delete_selection(ed);

    int r = ed->cursor_row;
    int c = ed->cursor_col;
    int len = (int)strlen(ed->lines[r]);

    if (len < TEDITOR_MAX_COLS - 1) {
        memmove(&ed->lines[r][c + 1], &ed->lines[r][c], len - c + 1);
        ed->lines[r][c] = ch;
        ed->cursor_col++;
        ed->is_modified = TRUE;
    }
    teditor_ensure_cursor_visible(ed);
}

static void teditor_insert_newline(TEditor *ed) {
    if (ed->sel_active) teditor_delete_selection(ed);
    if (ed->total_lines >= TEDITOR_MAX_LINES - 1) return;

    int r = ed->cursor_row;
    int c = ed->cursor_col;

    for (int i = ed->total_lines; i > r + 1; i--) {
        strncpy(ed->lines[i], ed->lines[i - 1], TEDITOR_MAX_COLS - 1);
    }
    ed->total_lines++;

    strncpy(ed->lines[r + 1], &ed->lines[r][c], TEDITOR_MAX_COLS - 1);
    ed->lines[r][c] = '\0';

    ed->cursor_row++;
    ed->cursor_col = 0;
    ed->is_modified = TRUE;
    teditor_ensure_cursor_visible(ed);
}

static void teditor_backspace(TEditor *ed) {
    if (ed->sel_active) {
        teditor_delete_selection(ed);
        return;
    }

    int r = ed->cursor_row;
    int c = ed->cursor_col;

    if (c > 0) {
        int len = (int)strlen(ed->lines[r]);
        memmove(&ed->lines[r][c - 1], &ed->lines[r][c], len - c + 1);
        ed->cursor_col--;
        ed->is_modified = TRUE;
    } else if (r > 0) {
        int prev_len = (int)strlen(ed->lines[r - 1]);
        int cur_len = (int)strlen(ed->lines[r]);

        if (prev_len + cur_len < TEDITOR_MAX_COLS - 1) {
            strncat(ed->lines[r - 1], ed->lines[r], TEDITOR_MAX_COLS - prev_len - 1);
            for (int i = r; i < ed->total_lines - 1; i++) {
                strncpy(ed->lines[i], ed->lines[i + 1], TEDITOR_MAX_COLS - 1);
            }
            ed->total_lines--;
            ed->cursor_row--;
            ed->cursor_col = prev_len;
            ed->is_modified = TRUE;
        }
    }
    teditor_ensure_cursor_visible(ed);
}

static void handle_t_editor_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - (wnd->bounds.left + 4);
        H rel_y = evt->pos.y - (wnd->bounds.top + 26);

        /* Toolbar button click checks */
        if (rel_y >= 0 && rel_y <= 24) {
            if (rel_x >= 4 && rel_x <= 40) {
                /* [New] */
                teditor_init_default(&g_teditor);
                g_teditor.total_lines = 1;
                g_teditor.lines[0][0] = '\0';
            } else if (rel_x >= 44 && rel_x <= 84) {
                /* [Open] */
                teditor_init_default(&g_teditor);
            } else if (rel_x >= 88 && rel_x <= 128) {
                /* [Save] */
                g_teditor.is_modified = FALSE;
            } else if (rel_x >= 132 && rel_x <= 168) {
                /* [Cut] */
                teditor_copy_selection(&g_teditor);
                teditor_delete_selection(&g_teditor);
            } else if (rel_x >= 172 && rel_x <= 212) {
                /* [Copy] */
                teditor_copy_selection(&g_teditor);
            } else if (rel_x >= 216 && rel_x <= 260) {
                /* [Paste] */
                teditor_paste_clipboard(&g_teditor);
            }
            return;
        }

        /* Editor client click -> Move cursor */
        if (rel_y >= 30) {
            int click_r = (rel_y - 30) / 16 + g_teditor.scroll_row;
            int click_c = (rel_x - 36) / 8 + g_teditor.scroll_col;

            if (click_r >= 0 && click_r < g_teditor.total_lines) {
                g_teditor.cursor_row = click_r;
                int line_len = (int)strlen(g_teditor.lines[click_r]);
                g_teditor.cursor_col = (click_c < 0) ? 0 : ((click_c > line_len) ? line_len : click_c);
                g_teditor.sel_active = FALSE;
                teditor_ensure_cursor_visible(&g_teditor);
            }
        }
        return;
    }

    if (evt->type == EV_KEY_DOWN) {
        if (evt->data == (VW)1) {
            /* Direct printable text input character */
            char ch = (char)evt->key;
            if (ch >= 32 && ch <= 126) {
                teditor_insert_char(&g_teditor, ch);
            }
            return;
        }

        SDL_Keycode sym = (SDL_Keycode)evt->key;
        uint16_t mod = (uint16_t)(uintptr_t)evt->data;

        BOOL shift = (mod & KMOD_SHIFT) != 0;
        BOOL ctrl = (mod & KMOD_CTRL) != 0;

        if (ctrl) {
            if (sym == SDLK_c) {
                teditor_copy_selection(&g_teditor);
                return;
            } else if (sym == SDLK_x) {
                teditor_copy_selection(&g_teditor);
                teditor_delete_selection(&g_teditor);
                return;
            } else if (sym == SDLK_v) {
                teditor_paste_clipboard(&g_teditor);
                return;
            } else if (sym == SDLK_a) {
                g_teditor.sel_active = TRUE;
                g_teditor.sel_start_r = 0;
                g_teditor.sel_start_c = 0;
                g_teditor.sel_end_r = g_teditor.total_lines - 1;
                g_teditor.sel_end_c = (int)strlen(g_teditor.lines[g_teditor.total_lines - 1]);
                g_teditor.cursor_row = g_teditor.sel_end_r;
                g_teditor.cursor_col = g_teditor.sel_end_c;
                return;
            } else if (sym == SDLK_s) {
                g_teditor.is_modified = FALSE;
                return;
            } else if (sym == SDLK_n) {
                teditor_init_default(&g_teditor);
                g_teditor.total_lines = 1;
                g_teditor.lines[0][0] = '\0';
                return;
            }
        }

        if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
            teditor_insert_newline(&g_teditor);
        } else if (sym == SDLK_BACKSPACE) {
            teditor_backspace(&g_teditor);
        } else if (sym == SDLK_DELETE) {
            if (g_teditor.sel_active) {
                teditor_delete_selection(&g_teditor);
            } else {
                int r = g_teditor.cursor_row;
                int c = g_teditor.cursor_col;
                int len = (int)strlen(g_teditor.lines[r]);
                if (c < len) {
                    memmove(&g_teditor.lines[r][c], &g_teditor.lines[r][c + 1], len - c);
                    g_teditor.is_modified = TRUE;
                }
            }
        } else if (sym == SDLK_LEFT) {
            if (shift && !g_teditor.sel_active) {
                g_teditor.sel_active = TRUE;
                g_teditor.sel_start_r = g_teditor.cursor_row;
                g_teditor.sel_start_c = g_teditor.cursor_col;
            }
            if (g_teditor.cursor_col > 0) {
                g_teditor.cursor_col--;
            } else if (g_teditor.cursor_row > 0) {
                g_teditor.cursor_row--;
                g_teditor.cursor_col = (int)strlen(g_teditor.lines[g_teditor.cursor_row]);
            }
            if (shift) {
                g_teditor.sel_end_r = g_teditor.cursor_row;
                g_teditor.sel_end_c = g_teditor.cursor_col;
            } else {
                g_teditor.sel_active = FALSE;
            }
            teditor_ensure_cursor_visible(&g_teditor);
        } else if (sym == SDLK_RIGHT) {
            if (shift && !g_teditor.sel_active) {
                g_teditor.sel_active = TRUE;
                g_teditor.sel_start_r = g_teditor.cursor_row;
                g_teditor.sel_start_c = g_teditor.cursor_col;
            }
            int len = (int)strlen(g_teditor.lines[g_teditor.cursor_row]);
            if (g_teditor.cursor_col < len) {
                g_teditor.cursor_col++;
            } else if (g_teditor.cursor_row < g_teditor.total_lines - 1) {
                g_teditor.cursor_row++;
                g_teditor.cursor_col = 0;
            }
            if (shift) {
                g_teditor.sel_end_r = g_teditor.cursor_row;
                g_teditor.sel_end_c = g_teditor.cursor_col;
            } else {
                g_teditor.sel_active = FALSE;
            }
            teditor_ensure_cursor_visible(&g_teditor);
        } else if (sym == SDLK_UP) {
            if (g_teditor.cursor_row > 0) {
                g_teditor.cursor_row--;
                int len = (int)strlen(g_teditor.lines[g_teditor.cursor_row]);
                if (g_teditor.cursor_col > len) g_teditor.cursor_col = len;
            }
            g_teditor.sel_active = FALSE;
            teditor_ensure_cursor_visible(&g_teditor);
        } else if (sym == SDLK_DOWN) {
            if (g_teditor.cursor_row < g_teditor.total_lines - 1) {
                g_teditor.cursor_row++;
                int len = (int)strlen(g_teditor.lines[g_teditor.cursor_row]);
                if (g_teditor.cursor_col > len) g_teditor.cursor_col = len;
            }
            g_teditor.sel_active = FALSE;
            teditor_ensure_cursor_visible(&g_teditor);
        } else if (sym == SDLK_HOME) {
            g_teditor.cursor_col = 0;
            g_teditor.sel_active = FALSE;
            teditor_ensure_cursor_visible(&g_teditor);
        } else if (sym == SDLK_END) {
            g_teditor.cursor_col = (int)strlen(g_teditor.lines[g_teditor.cursor_row]);
            g_teditor.sel_active = FALSE;
            teditor_ensure_cursor_visible(&g_teditor);
        }
    }
}

static void paint_t_editor(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background page */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    /* CUA Toolbar Header */
    RECT tb = { 0, 0, dev->width, 24 };
    fill_rec(dev, &tb, COLOR_LTGRAY);
    drw_lin(dev, 0, 24, dev->width, 24);

    /* CUA Action Buttons */
    drw_tc_string(dev, 6, 4, "[New]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 46, 4, "[Open]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 90, 4, "[Save]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 134, 4, "[Cut]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 174, 4, "[Copy]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 218, 4, "[Paste]", COLOR_BLACK, COLOR_LTGRAY);

    /* Document Status Title */
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "%s%s", g_teditor.filename, g_teditor.is_modified ? " *" : "");
    drw_tc_string(dev, dev->width - 120, 4, title_buf, COLOR_NAVY, COLOR_LTGRAY);

    /* Render Gutter & Text Lines */
    int start_r = g_teditor.scroll_row;
    int end_r = start_r + TEDITOR_VIEW_ROWS;
    if (end_r > g_teditor.total_lines) end_r = g_teditor.total_lines;

    int y = 30;
    for (int r_idx = start_r; r_idx < end_r; r_idx++) {
        /* Gutter line number */
        char num_str[10];
        snprintf(num_str, sizeof(num_str), "%2d|", r_idx + 1);
        drw_tc_string(dev, 6, y, num_str, COLOR_GRAY, COLOR_WHITE);

        /* Line content */
        const char *line = g_teditor.lines[r_idx];
        int len = (int)strlen(line);

        int start_c = g_teditor.scroll_col;
        int end_c = start_c + TEDITOR_VIEW_COLS;
        if (end_c > len) end_c = len;

        int x = 36;
        for (int c_idx = start_c; c_idx < end_c; c_idx++) {
            char ch_str[2] = { line[c_idx], '\0' };

            /* Check if character is inside CUA selection range */
            BOOL is_selected = FALSE;
            if (g_teditor.sel_active) {
                int r1 = g_teditor.sel_start_r, c1 = g_teditor.sel_start_c;
                int r2 = g_teditor.sel_end_r, c2 = g_teditor.sel_end_c;
                if (r1 > r2 || (r1 == r2 && c1 > c2)) {
                    int tr = r1; r1 = r2; r2 = tr;
                    int tc = c1; c1 = c2; c2 = tc;
                }
                if (r_idx > r1 && r_idx < r2) is_selected = TRUE;
                else if (r_idx == r1 && r_idx == r2 && c_idx >= c1 && c_idx < c2) is_selected = TRUE;
                else if (r_idx == r1 && r_idx < r2 && c_idx >= c1) is_selected = TRUE;
                else if (r_idx == r2 && r_idx > r1 && c_idx < c2) is_selected = TRUE;
            }

            COLOR fg = is_selected ? COLOR_WHITE : COLOR_BLACK;
            COLOR bg = is_selected ? COLOR_NAVY : COLOR_WHITE;

            drw_tc_string(dev, x, y, ch_str, fg, bg);
            x += 8;
        }

        /* Draw Blinking/Solid Cursor */
        if (r_idx == g_teditor.cursor_row) {
            int cur_x = 36 + (g_teditor.cursor_col - g_teditor.scroll_col) * 8;
            if (cur_x >= 36 && cur_x < dev->width - 10) {
                RECT cursor_rect = { cur_x, y, cur_x + 2, y + 16 };
                fill_rec(dev, &cursor_rect, COLOR_NAVY);
            }
        }

        y += 16;
    }

    /* Embedded Virtual Object Icon inside Document */
    if (g_teditor.has_vobj) {
        RECT embed_vobj = { 36, dev->height - 45, 230, dev->height - 20 };
        fill_rec(dev, &embed_vobj, COLOR_LTGRAY);
        drw_rec(dev, &embed_vobj);
        char vobj_str[64];
        snprintf(vobj_str, sizeof(vobj_str), "[VOBJ: %s]", g_teditor.vobj_name);
        drw_tc_string(dev, 42, dev->height - 38, vobj_str, COLOR_NAVY, COLOR_LTGRAY);
    }

    /* Status Bar Footer */
    RECT sb = { 0, dev->height - 18, dev->width, dev->height };
    fill_rec(dev, &sb, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 18, dev->width, dev->height - 18);

    char status_buf[128];
    snprintf(status_buf, sizeof(status_buf), " Line %d, Col %d  |  TRON-Code  |  [CUA INS]",
             g_teditor.cursor_row + 1, g_teditor.cursor_col + 1);
    drw_tc_string(dev, 8, dev->height - 15, status_buf, COLOR_BLACK, COLOR_LTGRAY);
}

WND* open_t_editor_window(void) {
    WND *wnd = opn_wnd("T-Editor - README.txt", 400, 140, 480, 320,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        teditor_init_default(&g_teditor);
        wnd->paint = paint_t_editor;
        wnd->event_handler = handle_t_editor_event;
    }
    return wnd;
}
