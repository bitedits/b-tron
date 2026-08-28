#include <btron/wnd.h>
#include <btron/troncode.h>
#include <btron/dp.h>
#include <btron/event.h>
#include <btron/tip.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <SDL.h>
#else
#include <stddef.h>
#include <stdint.h>
extern void* Imalloc(size_t sz);
extern void Ifree(void *ptr);
extern void* Icalloc(size_t nmemb, size_t sz);
extern char* tkl_strncpy(char *dst, const char *src, size_t n);
extern void* tkl_memset(void *s, int c, size_t n);
extern void* tkl_memcpy(void *dst, const void *src, size_t n);
#define malloc Imalloc
#define free Ifree
#define calloc Icalloc
#define strncpy tkl_strncpy
#define memset tkl_memset
#define memcpy tkl_memcpy
static inline size_t strlen(const char *s) { size_t n = 0; while (s && s[n]) n++; return n; }
#endif

#define GTERM_MAX_COLS     64
#define GTERM_MAX_ROWS     18
#define GTERM_HIST_MAX     200
#define GTERM_CMD_HIST_MAX 32

typedef struct {
    char lines[GTERM_HIST_MAX][GTERM_MAX_COLS + 1];
    COLOR line_cols[GTERM_HIST_MAX];
    int total_lines;

    char input_buf[256];
    int input_len;

    char cmd_history[GTERM_CMD_HIST_MAX][256];
    int cmd_hist_count;
    int cmd_hist_idx;

    char prompt[16];
} GTERM_STATE;

static GTERM_STATE g_gterm;

static void gterm_append_line(GTERM_STATE *st, const char *text, COLOR color) {
    if (!st || !text) return;

    const char *p = text;
    while (*p) {
        /* If scrollback buffer full, shift lines up */
        if (st->total_lines >= GTERM_HIST_MAX) {
            memmove(&st->lines[0], &st->lines[1], sizeof(st->lines[0]) * (GTERM_HIST_MAX - 1));
            memmove(&st->line_cols[0], &st->line_cols[1], sizeof(st->line_cols[0]) * (GTERM_HIST_MAX - 1));
            st->total_lines = GTERM_HIST_MAX - 1;
        }

        char *dst = st->lines[st->total_lines];
        int col = 0;

        while (*p && *p != '\n' && col < GTERM_MAX_COLS) {
            dst[col++] = *p++;
        }
        dst[col] = '\0';
        st->line_cols[st->total_lines] = color;
        st->total_lines++;

        if (*p == '\n') {
            p++;
        }
    }
}

static void gterm_init_banner(GTERM_STATE *st) {
    st->total_lines = 0;
    st->input_len = 0;
    st->input_buf[0] = '\0';
    st->cmd_hist_count = 0;
    st->cmd_hist_idx = -1;
    strncpy(st->prompt, "btron# ", sizeof(st->prompt) - 1);

    gterm_append_line(st, "B-TRON OS Shell v1.0 (gterm console)", COLOR_WHITE);
    gterm_append_line(st, "Sakamura BTRON3 Specification Terminal Emulator", COLOR_CYAN);
    gterm_append_line(st, "Type 'help' or '?' to list commands.", COLOR_GRAY);
    gterm_append_line(st, "--------------------------------------------------", COLOR_GRAY);
}

static void gterm_execute_cmd(WND *wnd, GTERM_STATE *st, const char *cmd_line) {
    /* Echo prompt + input */
    char echo_buf[280];
    snprintf(echo_buf, sizeof(echo_buf), "%s%s", st->prompt, cmd_line);
    gterm_append_line(st, echo_buf, COLOR_WHITE);

    /* Save to history */
    if (cmd_line[0] != '\0') {
        if (st->cmd_hist_count < GTERM_CMD_HIST_MAX) {
            strncpy(st->cmd_history[st->cmd_hist_count++], cmd_line, 255);
        } else {
            memmove(&st->cmd_history[0], &st->cmd_history[1], sizeof(st->cmd_history[0]) * (GTERM_CMD_HIST_MAX - 1));
            strncpy(st->cmd_history[GTERM_CMD_HIST_MAX - 1], cmd_line, 255);
        }
    }
    st->cmd_hist_idx = st->cmd_hist_count;

    /* Trim leading whitespace */
    const char *p = cmd_line;
    while (*p && isspace((unsigned char)*p)) p++;

    if (*p == '\0') {
        return;
    }

    char cmd[64] = {0};
    char arg[256] = {0};
    int n = sscanf(p, "%63s %[^\n]", cmd, arg);

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        gterm_append_line(st, "B-TRON Shell Commands:", COLOR_GREEN);
        gterm_append_line(st, "  help, ?     - Show command help list", COLOR_LTGRAY);
        gterm_append_line(st, "  ver, uname  - System version info", COLOR_LTGRAY);
        gterm_append_line(st, "  pwd         - Print current working directory", COLOR_LTGRAY);
        gterm_append_line(st, "  cd <dir>    - Change working directory", COLOR_LTGRAY);
        gterm_append_line(st, "  ls, dir     - List directory contents", COLOR_LTGRAY);
        gterm_append_line(st, "  cat <file>  - Display text file contents", COLOR_LTGRAY);
        gterm_append_line(st, "  echo <text> - Print string", COLOR_LTGRAY);
        gterm_append_line(st, "  ps          - System process task table", COLOR_LTGRAY);
        gterm_append_line(st, "  vobj        - List virtual objects in store", COLOR_LTGRAY);
        gterm_append_line(st, "  date        - Current system time & date", COLOR_LTGRAY);
        gterm_append_line(st, "  clear, cls  - Clear terminal screen", COLOR_LTGRAY);
        gterm_append_line(st, "  history     - Show command history", COLOR_LTGRAY);
        gterm_append_line(st, "  exit, quit  - Close terminal window", COLOR_LTGRAY);
        gterm_append_line(st, "  <shell-cmd> - Execute POSIX host command", COLOR_LTGRAY);
    } else if (strcmp(cmd, "ver") == 0 || strcmp(cmd, "uname") == 0) {
        gterm_append_line(st, "BTRON 1.0 (btron-sdl2-posix x86_64)", COLOR_CYAN);
        gterm_append_line(st, "Kernel: uITRON 4.0 Specification Engine", COLOR_CYAN);
        gterm_append_line(st, "Graphics: DP SDL2 Compositor Architecture", COLOR_CYAN);
        gterm_append_line(st, "Specification: Sakamura BTRON3 Specification", COLOR_CYAN);
    } else if (strcmp(cmd, "pwd") == 0) {
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd))) {
            gterm_append_line(st, cwd, COLOR_LTGRAY);
        } else {
            gterm_append_line(st, "Error: cannot get working directory", COLOR_RED);
        }
    } else if (strcmp(cmd, "cd") == 0) {
        const char *dir = (n > 1 && arg[0]) ? arg : ".";
        if (chdir(dir) == 0) {
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd))) {
                gterm_append_line(st, cwd, COLOR_GREEN);
            }
        } else {
            char err[280];
            snprintf(err, sizeof(err), "cd: no such file or directory: %s", dir);
            gterm_append_line(st, err, COLOR_RED);
        }
    } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0) {
        const char *dir_path = (n > 1 && arg[0]) ? arg : ".";
        DIR *dir = opendir(dir_path);
        if (!dir) {
            char err[280];
            snprintf(err, sizeof(err), "ls: cannot access '%s'", dir_path);
            gterm_append_line(st, err, COLOR_RED);
        } else {
            struct dirent *entry;
            char line_buf[256] = "";
            int count = 0;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.' && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    continue;
                }
                char name_fmt[64];
                snprintf(name_fmt, sizeof(name_fmt), "%-18s ", entry->d_name);
                strncat(line_buf, name_fmt, sizeof(line_buf) - strlen(line_buf) - 1);
                count++;
                if (count % 3 == 0) {
                    gterm_append_line(st, line_buf, COLOR_LTGRAY);
                    line_buf[0] = '\0';
                }
            }
            if (line_buf[0] != '\0') {
                gterm_append_line(st, line_buf, COLOR_LTGRAY);
            }
            closedir(dir);
        }
    } else if (strcmp(cmd, "cat") == 0) {
        if (n <= 1 || !arg[0]) {
            gterm_append_line(st, "cat: missing file argument", COLOR_RED);
        } else {
            FILE *f = fopen(arg, "r");
            if (!f) {
                char err[280];
                snprintf(err, sizeof(err), "cat: %s: No such file", arg);
                gterm_append_line(st, err, COLOR_RED);
            } else {
                char buf[256];
                int max_read = 30;
                while (fgets(buf, sizeof(buf), f) && max_read-- > 0) {
                    gterm_append_line(st, buf, COLOR_LTGRAY);
                }
                fclose(f);
            }
        }
    } else if (strcmp(cmd, "echo") == 0) {
        gterm_append_line(st, arg, COLOR_LTGRAY);
    } else if (strcmp(cmd, "ps") == 0) {
        gterm_append_line(st, "PID  TASK          PRI  STAT   TIME", COLOR_CYAN);
        gterm_append_line(st, "  1  desktop        10  RUN    00:01", COLOR_LTGRAY);
        gterm_append_line(st, "  2  wnd_mgr        12  SLEEP  00:00", COLOR_LTGRAY);
        gterm_append_line(st, "  3  gterm          15  RUN    00:00", COLOR_LTGRAY);
        gterm_append_line(st, "  4  t_editor       15  SLEEP  00:00", COLOR_LTGRAY);
        gterm_append_line(st, "  5  vobj_mgr       15  SLEEP  00:00", COLOR_LTGRAY);
    } else if (strcmp(cmd, "vobj") == 0) {
        DIR *dir = opendir("./btron_store");
        if (!dir) {
            gterm_append_line(st, "vobj: btron_store directory not found", COLOR_RED);
        } else {
            gterm_append_line(st, "B-TRON Real/Virtual Object Store:", COLOR_CYAN);
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] != '.') {
                    char item[128];
                    snprintf(item, sizeof(item), "  [VOBJ] %s", entry->d_name);
                    gterm_append_line(st, item, COLOR_GREEN);
                }
            }
            closedir(dir);
        }
    } else if (strcmp(cmd, "date") == 0) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char tbuf[128];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S %Z", tm_info);
        gterm_append_line(st, tbuf, COLOR_LTGRAY);
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        st->total_lines = 0;
    } else if (strcmp(cmd, "history") == 0) {
        gterm_append_line(st, "Command History:", COLOR_CYAN);
        for (int i = 0; i < st->cmd_hist_count; i++) {
            char hline[280];
            snprintf(hline, sizeof(hline), " %2d  %s", i + 1, st->cmd_history[i]);
            gterm_append_line(st, hline, COLOR_LTGRAY);
        }
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        if (wnd) {
            cls_wnd(wnd);
        }
    } else {
        /* Fallback: Execute external shell command via popen */
        FILE *pipe = popen(cmd_line, "r");
        if (!pipe) {
            char err[280];
            snprintf(err, sizeof(err), "gterm: command not found: %s", cmd);
            gterm_append_line(st, err, COLOR_RED);
        } else {
            char linebuf[256];
            int max_lines = 40;
            while (fgets(linebuf, sizeof(linebuf), pipe) && max_lines-- > 0) {
                gterm_append_line(st, linebuf, COLOR_LTGRAY);
            }
            pclose(pipe);
        }
    }
}

/* ASCII Key with Shift / CapsLock Translation */
static char get_ascii_char_with_shift(UW key, uint16_t mod) {
    BOOL shift = (mod & BTRON_KMOD_SHIFT) != 0;
    BOOL caps  = (mod & BTRON_KMOD_CAPS) != 0;

    if (key >= 'A' && key <= 'Z') {
        if (shift ^ caps) return (char)key;
        return (char)(key - 'A' + 'a');
    }

    if (key >= 'a' && key <= 'z') {
        if (shift ^ caps) return (char)(key - 'a' + 'A');
        return (char)key;
    }

    if (shift) {
        switch (key) {
            case '1': return '!';
            case '2': return '@';
            case '3': return '#';
            case '4': return '$';
            case '5': return '%';
            case '6': return '^';
            case '7': return '&';
            case '8': return '*';
            case '9': return '(';
            case '0': return ')';
            case '-': return '_';
            case '=': return '+';
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case ';': return ':';
            case '\'': return '"';
            case ',': return '<';
            case '.': return '>';
            case '/': return '?';
            case '`': return '~';
            default: break;
        }
    }
    return (char)key;
}

static void handle_gterm_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    if (evt->type == EV_KEY_DOWN) {
        char commit_buf[128] = "";
        UW key_code = evt->key;
        uint16_t mod = (uint16_t)(uintptr_t)evt->data;

        BOOL ctrl = (mod & BTRON_KMOD_CTRL) != 0;

        /* Control key combinations */
        if (ctrl) {
            if (key_code == 'c' || key_code == 'C' || key_code == SDLK_c) {
                char cancel_msg[280];
                snprintf(cancel_msg, sizeof(cancel_msg), "%s%s^C", g_gterm.prompt, g_gterm.input_buf);
                gterm_append_line(&g_gterm, cancel_msg, COLOR_RED);
                g_gterm.input_buf[0] = '\0';
                g_gterm.input_len = 0;
                tip_cancel();
                return;
            } else if (key_code == 'l' || key_code == 'L' || key_code == SDLK_l) {
                g_gterm.total_lines = 0;
                return;
            } else if (key_code == 'u' || key_code == 'U' || key_code == SDLK_u) {
                g_gterm.input_buf[0] = '\0';
                g_gterm.input_len = 0;
                tip_cancel();
                return;
            }
        }

        /* Check if TIP handles key (Japanese IME mode or F10 / Ctrl+Space toggle) */
        if (tip_process_key(key_code, mod, commit_buf, sizeof(commit_buf))) {
            if (commit_buf[0] != '\0') {
                int clen = (int)strlen(commit_buf);
                if (g_gterm.input_len + clen < GTERM_MAX_COLS - (int)strlen(g_gterm.prompt) - 2) {
                    strcat(g_gterm.input_buf, commit_buf);
                    g_gterm.input_len += clen;
                }
            }
            return;
        }

        SDL_Keycode sym = (SDL_Keycode)key_code;

        /* Non-printable navigation & action keys */
        if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER || sym == '\r' || sym == '\n') {
            gterm_execute_cmd(wnd, &g_gterm, g_gterm.input_buf);
            g_gterm.input_buf[0] = '\0';
            g_gterm.input_len = 0;
            return;
        } else if (sym == SDLK_BACKSPACE || sym == 0x08) {
            if (g_gterm.input_len > 0) {
                int prev_c = g_gterm.input_len - 1;
                while (prev_c > 0 && ((unsigned char)g_gterm.input_buf[prev_c] & 0xC0) == 0x80) {
                    prev_c--;
                }
                g_gterm.input_buf[prev_c] = '\0';
                g_gterm.input_len = prev_c;
            }
            return;
        } else if (sym == SDLK_UP) {
            if (g_gterm.cmd_hist_count > 0 && g_gterm.cmd_hist_idx > 0) {
                g_gterm.cmd_hist_idx--;
                strncpy(g_gterm.input_buf, g_gterm.cmd_history[g_gterm.cmd_hist_idx], sizeof(g_gterm.input_buf) - 1);
                g_gterm.input_len = (int)strlen(g_gterm.input_buf);
            }
            return;
        } else if (sym == SDLK_DOWN) {
            if (g_gterm.cmd_hist_idx < g_gterm.cmd_hist_count - 1) {
                g_gterm.cmd_hist_idx++;
                strncpy(g_gterm.input_buf, g_gterm.cmd_history[g_gterm.cmd_hist_idx], sizeof(g_gterm.input_buf) - 1);
                g_gterm.input_len = (int)strlen(g_gterm.input_buf);
            } else if (g_gterm.cmd_hist_idx == g_gterm.cmd_hist_count - 1) {
                g_gterm.cmd_hist_idx = g_gterm.cmd_hist_count;
                g_gterm.input_buf[0] = '\0';
                g_gterm.input_len = 0;
            }
            return;
        } else if (sym == SDLK_TAB || sym == '\t') {
            if (g_gterm.input_len + 4 < GTERM_MAX_COLS - (int)strlen(g_gterm.prompt) - 2) {
                strcat(g_gterm.input_buf, "    ");
                g_gterm.input_len += 4;
            }
            return;
        }

        /* Direct English / Printable ASCII Text Input */
        if (!ctrl && sym >= 32 && sym <= 126) {
            char ch = get_ascii_char_with_shift((UW)sym, mod);
            if (g_gterm.input_len < GTERM_MAX_COLS - (int)strlen(g_gterm.prompt) - 2) {
                g_gterm.input_buf[g_gterm.input_len++] = ch;
                g_gterm.input_buf[g_gterm.input_len] = '\0';
            }
            return;
        }
    }
}

static void paint_gterm(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Black Terminal Screen Background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_BLACK);

    /* Visual Mode Indicator in Terminal Header */
    const char *mode_tag = (tip_get_mode() == TIP_MODE_HIRAGANA) ? "[Mode: JP (あ) | F10: Switch]" :
                           ((tip_get_mode() == TIP_MODE_KATAKANA) ? "[Mode: JP (ア) | F10: Switch]" :
                            "[Mode: EN (A) | F10: Switch]");
    drw_tc_string(dev, dev->width - 240, 6, mode_tag, COLOR_CYAN, COLOR_BLACK);

    int visible_rows = GTERM_MAX_ROWS - 1; /* Reserve bottom line for prompt */
    int start_line = 0;
    if (g_gterm.total_lines > visible_rows) {
        start_line = g_gterm.total_lines - visible_rows;
    }

    int y = 24;
    for (int i = start_line; i < g_gterm.total_lines; i++) {
        COLOR col = g_gterm.line_cols[i];
        if (col == 0) col = COLOR_WHITE;
        drw_tc_string(dev, 8, y, g_gterm.lines[i], col, COLOR_BLACK);
        y += 16;
    }

    /* Draw Active Prompt Line with Cursor and TIP Inline Preview */
    char prompt_line[300];
    snprintf(prompt_line, sizeof(prompt_line), "%s%s", g_gterm.prompt, g_gterm.input_buf);
    drw_tc_string(dev, 8, y, prompt_line, COLOR_WHITE, COLOR_BLACK);

    int prompt_pixel_w = 8 + (int)strlen(prompt_line) * 8;
    if (wnd->focused && tip_get_state() != TIP_STATE_IDLE) {
        char comp_buf[128];
        tip_get_converted_text(comp_buf, sizeof(comp_buf));
        BOOL is_dotted = (tip_get_state() == TIP_STATE_PRECOMP);
        drw_tc_string_underlined(dev, prompt_pixel_w, y, comp_buf, COLOR_CYAN, COLOR_BLACK, is_dotted);
        tip_set_caret_pos(wnd->bounds.left + prompt_pixel_w, wnd->bounds.top + y);
    } else if (wnd->focused) {
        drw_tc_string(dev, prompt_pixel_w, y, "_", COLOR_WHITE, COLOR_BLACK);
    }
}

WND* open_gterm_window(void) {
    WND *wnd = opn_wnd("BTRON Terminal (gterm)", 220, 160, 560, 360,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        gterm_init_banner(&g_gterm);
        wnd->paint = paint_gterm;
        wnd->event_handler = handle_gterm_event;
        tip_cancel();
        tip_set_mode(TIP_MODE_ASCII); /* Terminal strictly starts in EN mode */
        top_wnd(wnd);
    }
    return wnd;
}
