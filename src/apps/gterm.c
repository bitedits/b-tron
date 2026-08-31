#include <btron/wnd.h>
#include <btron/troncode.h>
#include <btron/dp.h>
#include <btron/event.h>
#include <btron/tip.h>
#include <btron/apps.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
extern void* Imalloc(size_t sz);
extern void Ifree(void *ptr);
extern void* Icalloc(size_t nmemb, size_t sz);
extern int snprintf(char *str, size_t size, const char *format, ...);
extern void* tkl_memmove(void *dest, const void *src, size_t n);
#define malloc  Imalloc
#define free    Ifree
#define calloc  Icalloc
#define strncpy tkl_strncpy
#define strncat tkl_strncat
#define strcat  tkl_strcat
#define strcmp  tkl_strcmp
#define strncmp tkl_strncmp
#define memset  tkl_memset
#define memcpy  tkl_memcpy
#define memmove tkl_memmove
#define strlen  tkl_strlen
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\f' || (c) == '\v')

static inline char* gterm_strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}
#define strstr  gterm_strstr
#endif

#define GTERM_MAX_COLS     256
#define GTERM_MAX_ROWS     32
#define GTERM_HIST_MAX     300
#define GTERM_CMD_HIST_MAX 32
#define COLOR_TERM_BG      0xCC000000 /* 20% dimmed background (0xCC instead of 0xFF) across all modes */

typedef struct {
    char lines[GTERM_HIST_MAX][GTERM_MAX_COLS + 1];
    COLOR line_cols[GTERM_HIST_MAX];
    int total_lines;

    char input_buf[GTERM_MAX_COLS + 1];
    int input_len;

    char prompt[32];

    char cmd_history[GTERM_CMD_HIST_MAX][256];
    int cmd_hist_count;
    int cmd_hist_idx;
} GTermState;

void gterm_append_line(GTermState *st, const char *text, COLOR col);

static int gterm_calc_text_pixel_width(const char *str) {
    if (!str) return 0;
    int w = 0;
    int i = 0;
    while (str[i] != '\0') {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x80) {
            w += 8;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            w += 8;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            w += 16;
            i += 3;
        } else {
            w += 8;
            i += 1;
        }
    }
    return w;
}

static void gterm_init_banner(GTermState *st) {
    memset(st, 0, sizeof(GTermState));
    strncpy(st->prompt, "btron:/> ", sizeof(st->prompt) - 1);

    gterm_append_line(st, "+==================================================+", COLOR_CYAN);
    gterm_append_line(st, "| B-System 3.0 Workstation Shell (gterm)           |", COLOR_WHITE);
    gterm_append_line(st, "+==================================================+", COLOR_CYAN);
    gterm_append_line(st, "Type 'help' or '?' for available system commands.", COLOR_GREEN);
}

void gterm_append_line(GTermState *st, const char *text, COLOR col) {
    if (!st || !text) return;

    /* Handle multi-line strings by splitting on '\n' */
    const char *p = text;
    while (*p) {
        char seg[GTERM_MAX_COLS + 1];
        int seg_i = 0;
        while (*p && *p != '\n' && seg_i < GTERM_MAX_COLS) {
            seg[seg_i++] = *p++;
        }
        seg[seg_i] = '\0';
        if (*p == '\n') p++;

        if (st->total_lines >= GTERM_HIST_MAX) {
            memmove(&st->lines[0], &st->lines[1], sizeof(st->lines[0]) * (GTERM_HIST_MAX - 1));
            memmove(&st->line_cols[0], &st->line_cols[1], sizeof(st->line_cols[0]) * (GTERM_HIST_MAX - 1));
            st->total_lines = GTERM_HIST_MAX - 1;
        }

        int idx = st->total_lines++;
        strncpy(st->lines[idx], seg, GTERM_MAX_COLS);
        st->lines[idx][GTERM_MAX_COLS] = '\0';
        st->line_cols[idx] = col;
    }
}

static void gterm_shell_out(const char *line, COLOR col, void *user_data) {
    GTermState *st = (GTermState*)user_data;
    if (st) {
        gterm_append_line(st, line, col);
    }
}

void shell_execute_cmd(const char *cmd_line, ShellOutputFn out_fn, void *user_data, WND *wnd) {
    if (!out_fn || !cmd_line) return;

    const char *p = cmd_line;
    while (*p && isspace((unsigned char)*p)) p++;

    if (*p == '\0') {
        return;
    }

    char cmd[64] = {0};
    char arg[256] = {0};
    int cmd_i = 0;
    while (*p && !isspace((unsigned char)*p) && cmd_i < 63) {
        cmd[cmd_i++] = *p++;
    }
    cmd[cmd_i] = '\0';
    while (*p && isspace((unsigned char)*p)) p++;
    int arg_i = 0;
    while (*p && arg_i < 255) {
        arg[arg_i++] = *p++;
    }
    arg[arg_i] = '\0';
    int n = (cmd_i > 0) ? (arg_i > 0 ? 2 : 1) : 0;

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        out_fn("B-System Shell Commands:", COLOR_GREEN, user_data);
        out_fn("  help, ?             - Show command help list", COLOR_LTGRAY, user_data);
        out_fn("  ver, uname          - System version & kernel info", COLOR_LTGRAY, user_data);
        out_fn("  devconf             - Registered hardware and device drivers", COLOR_LTGRAY, user_data);
        out_fn("  ps                  - Dynamic process and window task table", COLOR_LTGRAY, user_data);
        out_fn("  vobj, ls, dir       - Virtual Bodys and file listings", COLOR_LTGRAY, user_data);
        out_fn("  cat <file>          - Display text file contents", COLOR_LTGRAY, user_data);
        out_fn("  pwd                 - Print current working directory", COLOR_LTGRAY, user_data);
        out_fn("  cd <dir>            - Change working directory", COLOR_LTGRAY, user_data);
        out_fn("  echo <text>         - Print string", COLOR_LTGRAY, user_data);
        out_fn("  mem                 - Memory pool allocation statistics", COLOR_LTGRAY, user_data);
        out_fn("  mouse status        - Display current mouse cursor position", COLOR_LTGRAY, user_data);
        out_fn("  mouse move <X> <Y>  - Move mouse cursor to coordinates", COLOR_LTGRAY, user_data);
        out_fn("  mouse click <X> <Y> - Simulate mouse click at coordinates", COLOR_LTGRAY, user_data);
        out_fn("  edit, editor        - Launch new T-Editor instance", COLOR_LTGRAY, user_data);
        out_fn("  tad, browser        - Launch new TAD Browser instance", COLOR_LTGRAY, user_data);
        out_fn("  chat                - Launch new BeOS Chat instance", COLOR_LTGRAY, user_data);
        out_fn("  audio               - Launch Audio Player instance", COLOR_LTGRAY, user_data);
        out_fn("  cabinet             - Launch Cabinet Explorer instance", COLOR_LTGRAY, user_data);
        out_fn("  term, gterm         - Launch new Terminal instance", COLOR_LTGRAY, user_data);
        out_fn("  date                - Current system time & date", COLOR_LTGRAY, user_data);
        out_fn("  clear, cls          - Clear terminal screen", COLOR_LTGRAY, user_data);
        out_fn("  history             - Show command history", COLOR_LTGRAY, user_data);
        out_fn("  exit, quit          - Close terminal window", COLOR_LTGRAY, user_data);
    } else if (strcmp(cmd, "ver") == 0 || strcmp(cmd, "uname") == 0) {
        btron_core_print_ver(out_fn, user_data, arg);
    } else if (strcmp(cmd, "devconf") == 0) {
        char dbuf[512];
        sys_get_devconf(dbuf, sizeof(dbuf));
        out_fn(dbuf, COLOR_CYAN, user_data);
    } else if (strcmp(cmd, "mem") == 0) {
        uint32_t base = 0, limit = 0, used = 0;
        sys_get_mem_stats(&base, &limit, &used);
        out_fn("T-Kernel 2.0 Memory Pool Statistics:", COLOR_CYAN, user_data);
        char mbuf[128];
        snprintf(mbuf, sizeof(mbuf), "  Heap Base  : 0x%08X", base);
        out_fn(mbuf, COLOR_LTGRAY, user_data);
        snprintf(mbuf, sizeof(mbuf), "  Heap Limit : 0x%08X", limit);
        out_fn(mbuf, COLOR_LTGRAY, user_data);
        snprintf(mbuf, sizeof(mbuf), "  Heap Used  : 0x%08X (%u bytes)", used, used);
        out_fn(mbuf, COLOR_LTGRAY, user_data);
    } else if (strcmp(cmd, "mouse") == 0) {
        if (strcmp(arg, "status") == 0 || arg[0] == '\0') {
            H mx = 0, my = 0;
            sys_mouse_get_pos(&mx, &my);
            char pos_buf[128];
            snprintf(pos_buf, sizeof(pos_buf), "Mouse Cursor Position: X=%d, Y=%d", mx, my);
            out_fn(pos_buf, COLOR_CYAN, user_data);
        } else if (strncmp(arg, "move", 4) == 0) {
            const char *sp = arg + 4;
            while (*sp == ' ' || *sp == '\t') sp++;
            int mx = 0, my = 0;
            while (*sp >= '0' && *sp <= '9') { mx = mx * 10 + (*sp - '0'); sp++; }
            while (*sp == ' ' || *sp == '\t') sp++;
            while (*sp >= '0' && *sp <= '9') { my = my * 10 + (*sp - '0'); sp++; }
            sys_mouse_set_pos((H)mx, (H)my);
            char mbuf[128];
            snprintf(mbuf, sizeof(mbuf), "Moved mouse cursor to (%d, %d)", mx, my);
            out_fn(mbuf, COLOR_GREEN, user_data);
        } else if (strncmp(arg, "click", 5) == 0) {
            const char *sp = arg + 5;
            while (*sp == ' ' || *sp == '\t') sp++;
            int mx = 0, my = 0;
            while (*sp >= '0' && *sp <= '9') { mx = mx * 10 + (*sp - '0'); sp++; }
            while (*sp == ' ' || *sp == '\t') sp++;
            while (*sp >= '0' && *sp <= '9') { my = my * 10 + (*sp - '0'); sp++; }
            if (mx > 0 || my > 0) {
                sys_mouse_click((H)mx, (H)my);
                char cbuf[128];
                snprintf(cbuf, sizeof(cbuf), "Simulated mouse click at (%d, %d)", mx, my);
                out_fn(cbuf, COLOR_GREEN, user_data);
            } else {
                H cur_x = 0, cur_y = 0;
                sys_mouse_get_pos(&cur_x, &cur_y);
                sys_mouse_click(cur_x, cur_y);
                out_fn("Simulated mouse click at current cursor position", COLOR_GREEN, user_data);
            }
        }
    } else if (strcmp(cmd, "pwd") == 0) {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd))) {
            out_fn(cwd, COLOR_LTGRAY, user_data);
        } else {
            out_fn("Error: cannot get working directory", COLOR_RED, user_data);
        }
#else
        out_fn("/sys/btron_root", COLOR_LTGRAY, user_data);
#endif
    } else if (strcmp(cmd, "cd") == 0) {
        const char *dir = (n > 1 && arg[0]) ? arg : ".";
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        if (chdir(dir) == 0) {
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd))) {
                out_fn(cwd, COLOR_GREEN, user_data);
            }
        } else {
            char err[280];
            snprintf(err, sizeof(err), "cd: no such file or directory: %s", dir);
            out_fn(err, COLOR_RED, user_data);
        }
#else
        out_fn(dir, COLOR_GREEN, user_data);
#endif
    } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0) {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        const char *dir_path = (n > 1 && arg[0]) ? arg : ".";
        DIR *dir = opendir(dir_path);
        if (!dir) {
            char err[280];
            snprintf(err, sizeof(err), "ls: cannot access '%s'", dir_path);
            out_fn(err, COLOR_RED, user_data);
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
                    out_fn(line_buf, COLOR_LTGRAY, user_data);
                    line_buf[0] = '\0';
                }
            }
            if (line_buf[0] != '\0') {
                out_fn(line_buf, COLOR_LTGRAY, user_data);
            }
            closedir(dir);
        }
#else
        out_fn("BTRON3_Report.txt  README.txt          btron_store/       ", COLOR_LTGRAY, user_data);
        out_fn("dev/screen0        dev/uart0           kernel.sys         ", COLOR_LTGRAY, user_data);
#endif
    } else if (strcmp(cmd, "cat") == 0) {
        if (n <= 1 || !arg[0]) {
            out_fn("cat: missing file argument", COLOR_RED, user_data);
        } else {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
            FILE *f = fopen(arg, "r");
            if (!f) {
                char err[280];
                snprintf(err, sizeof(err), "cat: %s: No such file", arg);
                out_fn(err, COLOR_RED, user_data);
            } else {
                char buf[256];
                int max_read = 30;
                while (fgets(buf, sizeof(buf), f) && max_read-- > 0) {
                    out_fn(buf, COLOR_LTGRAY, user_data);
                }
                fclose(f);
            }
#else
            out_fn("【BTRON3仕様の新実装】報告文書", COLOR_CYAN, user_data);
            out_fn("宛先：ノルティアオーダー／TADワーキンググループ 小島秀樹様", COLOR_LTGRAY, user_data);
#endif
        }
    } else if (strcmp(cmd, "echo") == 0) {
        out_fn(arg, COLOR_LTGRAY, user_data);
    } else if (strcmp(cmd, "ps") == 0) {
        out_fn("PID   TASK           STAT   ADDR         BOUNDS    TITLE", COLOR_CYAN, user_data);
        out_fn("----------------------------------------------------------------", COLOR_DKGRAY, user_data);
        out_fn("  1   tk_desktop     RUN    0x01020000   1024x768  [B-System Desktop]", COLOR_GREEN, user_data);
        out_fn("  2   tk_wnd_mgr     READY  0x01040000   1024x768  [Window Compositor]", COLOR_GREEN, user_data);
        out_fn("  3   tk_tip_ime     READY  0x010A0000   Candidate [Mozc Japanese IME]", COLOR_GREEN, user_data);

        WND *w = get_wnd_list();
        if (!w) {
            out_fn("  (No active user application instances)", COLOR_LTGRAY, user_data);
        } else {
            WND *w_list[64];
            int count = 0;
            for (WND *curr = w; curr && count < 64; curr = curr->next) {
                w_list[count++] = curr;
            }

            for (int i = 0; i < count; i++) {
                WND *cw = w_list[i];
                const char *tname = "btron_app";
                if (strstr(cw->title, "gterm") || strstr(cw->title, "Terminal")) tname = "gterm";
                else if (strstr(cw->title, "T-Editor") || strstr(cw->title, "Editor")) tname = "t_editor";
                else if (strstr(cw->title, "TAD") || strstr(cw->title, "仕様書")) tname = "tad_browser";
                else if (strstr(cw->title, "Cabinet") || strstr(cw->title, "キャビネット")) tname = "vobj_mgr";
                else if (strstr(cw->title, "TC-K777ES") || strstr(cw->title, "SONY")) tname = "audio_player";
                else if (strstr(cw->title, "Chat") || strstr(cw->title, "対話") || strstr(cw->title, "会話") || strstr(cw->title, "Blabber")) tname = "beos_chat";

                /* Calculate instance index for this task type */
                int inst_num = 1;
                for (int j = 0; j < i; j++) {
                    WND *prev_w = w_list[j];
                    const char *pname = "btron_app";
                    if (strstr(prev_w->title, "gterm") || strstr(prev_w->title, "Terminal")) pname = "gterm";
                    else if (strstr(prev_w->title, "T-Editor") || strstr(prev_w->title, "Editor")) pname = "t_editor";
                    else if (strstr(prev_w->title, "TAD") || strstr(prev_w->title, "仕様書")) pname = "tad_browser";
                    else if (strstr(prev_w->title, "Cabinet") || strstr(prev_w->title, "キャビネット")) pname = "vobj_mgr";
                    else if (strstr(prev_w->title, "TC-K777ES") || strstr(prev_w->title, "SONY")) pname = "audio_player";
                    else if (strstr(prev_w->title, "Chat") || strstr(prev_w->title, "対話") || strstr(prev_w->title, "会話") || strstr(prev_w->title, "Blabber")) pname = "beos_chat";

                    if (strcmp(pname, tname) == 0) inst_num++;
                }

                char task_with_inst[32];
                snprintf(task_with_inst, sizeof(task_with_inst), "%s#%d", tname, inst_num);

                H ww = cw->bounds.right - cw->bounds.left;
                H wh = cw->bounds.bottom - cw->bounds.top;
                char bounds_str[32];
                snprintf(bounds_str, sizeof(bounds_str), "%dx%d", ww, wh);

                char line[256];
                snprintf(line, sizeof(line), "%3d   %-14s %-6s 0x%08lx %-9s %s",
                         cw->id,
                         task_with_inst,
                         cw->focused ? "RUN" : "SLEEP",
                         (unsigned long)((uintptr_t)cw & 0xFFFFFFFF),
                         bounds_str,
                         cw->title);

                COLOR col = cw->focused ? COLOR_YELLOW : COLOR_LTGRAY;
                out_fn(line, col, user_data);
            }
        }
    } else if (strcmp(cmd, "editor") == 0 || strcmp(cmd, "edit") == 0) {
        open_t_editor_window();
        out_fn("Started new T-Editor instance", COLOR_GREEN, user_data);
    } else if (strcmp(cmd, "tad") == 0 || strcmp(cmd, "browser") == 0) {
        open_tad_browser_window("tad_bin/01_btron3_spec.tad", "【仕様書】BTRON3 3.20");
        out_fn("Started new TAD Browser instance", COLOR_GREEN, user_data);
    } else if (strcmp(cmd, "chat") == 0) {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        launch_beos_chat();
        out_fn("Started new BeOS Chat instance", COLOR_GREEN, user_data);
#else
        out_fn("BeOS Chat is supported in POSIX / Hosted mode", COLOR_YELLOW, user_data);
#endif
    } else if (strcmp(cmd, "audio") == 0 || strcmp(cmd, "player") == 0) {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        open_audio_player_window();
        out_fn("Started Audio Deck instance", COLOR_GREEN, user_data);
#else
        out_fn("Audio Player is supported in POSIX / Hosted mode", COLOR_YELLOW, user_data);
#endif
    } else if (strcmp(cmd, "cabinet") == 0 || strcmp(cmd, "vobjmgr") == 0) {
        open_vobj_manager_window();
        out_fn("Started Cabinet Explorer instance", COLOR_GREEN, user_data);
    } else if (strcmp(cmd, "gterm") == 0 || strcmp(cmd, "term") == 0) {
        open_gterm_window();
        out_fn("Started new Terminal instance", COLOR_GREEN, user_data);
    } else if (strcmp(cmd, "vobj") == 0) {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        DIR *dir = opendir("./btron_store");
        if (!dir) {
            out_fn("vobj: btron_store directory not found", COLOR_RED, user_data);
        } else {
            out_fn("B-System Real/Virtual Body Store:", COLOR_CYAN, user_data);
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] != '.') {
                    char item[128];
                    snprintf(item, sizeof(item), "  [VOBJ] %s", entry->d_name);
                    out_fn(item, COLOR_GREEN, user_data);
                }
            }
            closedir(dir);
        }
#else
        out_fn("B-System Real/Virtual Body Store:", COLOR_CYAN, user_data);
        out_fn("  [VOBJ] BTRON3_Report.txt (RealObject #101)", COLOR_GREEN, user_data);
        out_fn("  [VOBJ] Kojima_Hideki_Link.vlk (VirtualLink #102)", COLOR_GREEN, user_data);
        out_fn("  [VOBJ] TKernel_Subsystem.sys (RealObject #103)", COLOR_GREEN, user_data);
#endif
    } else if (strcmp(cmd, "date") == 0) {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char tbuf[128];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S %Z", tm_info);
        out_fn(tbuf, COLOR_LTGRAY, user_data);
#else
        out_fn("2026-08-28 12:00:00 JST", COLOR_LTGRAY, user_data);
#endif
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        /* Screen cleared via shell caller */
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        if (wnd) {
            cls_wnd(wnd);
        }
    } else {
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
        /* Fallback: Execute external shell command via popen */
        FILE *pipe = popen(cmd_line, "r");
        if (!pipe) {
            char err[280];
            snprintf(err, sizeof(err), "gterm: command not found: %s", cmd);
            out_fn(err, COLOR_RED, user_data);
        } else {
            char linebuf[256];
            int max_lines = 40;
            while (fgets(linebuf, sizeof(linebuf), pipe) && max_lines-- > 0) {
                out_fn(linebuf, COLOR_LTGRAY, user_data);
            }
            pclose(pipe);
        }
#else
        char err[280];
        snprintf(err, sizeof(err), "gterm: command not found: %s", cmd);
        out_fn(err, COLOR_RED, user_data);
#endif
    }
}

static void gterm_execute_cmd(WND *wnd, GTermState *st, const char *cmd_line) {
    if (!st || !cmd_line) return;

    /* Echo command line into terminal */
    char echo_buf[300];
    snprintf(echo_buf, sizeof(echo_buf), "%s%s", st->prompt, cmd_line);
    gterm_append_line(st, echo_buf, COLOR_WHITE);

    /* Record in history */
    if (cmd_line[0] != '\0') {
        if (st->cmd_hist_count < GTERM_CMD_HIST_MAX) {
            strncpy(st->cmd_history[st->cmd_hist_count++], cmd_line, 255);
        } else {
            memmove(&st->cmd_history[0], &st->cmd_history[1], sizeof(st->cmd_history[0]) * (GTERM_CMD_HIST_MAX - 1));
            strncpy(st->cmd_history[GTERM_CMD_HIST_MAX - 1], cmd_line, 255);
        }
    }
    st->cmd_hist_idx = st->cmd_hist_count;

    if (strcmp(cmd_line, "history") == 0) {
        gterm_append_line(st, "Command History:", COLOR_CYAN);
        for (int i = 0; i < st->cmd_hist_count; i++) {
            char hline[280];
            snprintf(hline, sizeof(hline), " %2d  %s", i + 1, st->cmd_history[i]);
            gterm_append_line(st, hline, COLOR_LTGRAY);
        }
    } else if (strcmp(cmd_line, "clear") == 0 || strcmp(cmd_line, "cls") == 0) {
        st->total_lines = 0;
    } else {
        shell_execute_cmd(cmd_line, gterm_shell_out, st, wnd);
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
    GTermState *st = (GTermState*)(uintptr_t)wnd->user_data;
    if (!st) return;

    if (evt->type == EV_KEY_DOWN) {
        char commit_buf[128] = "";
        UW key_code = evt->key;
        uint16_t mod = (uint16_t)(uintptr_t)evt->data;

        BOOL ctrl = (mod & BTRON_KMOD_CTRL) != 0;

        /* Control key combinations */
        if (ctrl) {
            if (key_code == 'c' || key_code == 'C') {
                char cancel_msg[280];
                snprintf(cancel_msg, sizeof(cancel_msg), "%s%s^C", st->prompt, st->input_buf);
                gterm_append_line(st, cancel_msg, COLOR_RED);
                st->input_buf[0] = '\0';
                st->input_len = 0;
                tip_cancel();
                return;
            } else if (key_code == 'l' || key_code == 'L') {
                st->total_lines = 0;
                return;
            } else if (key_code == 'u' || key_code == 'U') {
                st->input_buf[0] = '\0';
                st->input_len = 0;
                tip_cancel();
                return;
            }
        }

        /* Check if TIP handles key (Japanese IME mode or F10 / Ctrl+Space toggle) */
        if (tip_process_key(key_code, mod, commit_buf, sizeof(commit_buf))) {
            if (commit_buf[0] != '\0') {
                int clen = (int)strlen(commit_buf);
                if (st->input_len + clen < GTERM_MAX_COLS - (int)strlen(st->prompt) - 2) {
                    strcat(st->input_buf, commit_buf);
                    st->input_len += clen;
                }
            }
            return;
        }

        UW sym = key_code;

        /* Non-printable navigation & action keys */
        if (sym == BTRON_KEY_RETURN || sym == BTRON_KEY_KP_ENTER || sym == '\r' || sym == '\n') {
            gterm_execute_cmd(wnd, st, st->input_buf);
            st->input_buf[0] = '\0';
            st->input_len = 0;
            return;
        } else if (sym == BTRON_KEY_BACKSPACE || sym == 0x08) {
            if (st->input_len > 0) {
                int prev_c = st->input_len - 1;
                while (prev_c > 0 && ((unsigned char)st->input_buf[prev_c] & 0xC0) == 0x80) {
                    prev_c--;
                }
                st->input_buf[prev_c] = '\0';
                st->input_len = prev_c;
            }
            return;
        } else if (sym == BTRON_KEY_UP) {
            if (st->cmd_hist_count > 0 && st->cmd_hist_idx > 0) {
                st->cmd_hist_idx--;
                strncpy(st->input_buf, st->cmd_history[st->cmd_hist_idx], sizeof(st->input_buf) - 1);
                st->input_len = (int)strlen(st->input_buf);
            }
            return;
        } else if (sym == BTRON_KEY_DOWN) {
            if (st->cmd_hist_idx < st->cmd_hist_count - 1) {
                st->cmd_hist_idx++;
                strncpy(st->input_buf, st->cmd_history[st->cmd_hist_idx], sizeof(st->input_buf) - 1);
                st->input_len = (int)strlen(st->input_buf);
            } else {
                st->cmd_hist_idx = st->cmd_hist_count;
                st->input_buf[0] = '\0';
                st->input_len = 0;
            }
            return;
        } else if (sym == BTRON_KEY_TAB || sym == '\t') {
            if (st->input_len + 4 < GTERM_MAX_COLS - (int)strlen(st->prompt) - 2) {
                strcat(st->input_buf, "    ");
                st->input_len += 4;
            }
            return;
        }

        /* Direct English / Printable ASCII Text Input */
        if (!ctrl && sym >= 32 && sym <= 126) {
            char ch = get_ascii_char_with_shift((UW)sym, mod);
            if (st->input_len < GTERM_MAX_COLS - (int)strlen(st->prompt) - 2) {
                st->input_buf[st->input_len++] = ch;
                st->input_buf[st->input_len] = '\0';
            }
            return;
        }
    }
}

static void paint_gterm(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    GTermState *st = (GTermState*)(uintptr_t)wnd->user_data;
    if (!st) return;

    /* Black Terminal Screen Background across full window canvas (20% dimmed) */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_TERM_BG);

    /* Visual Mode Indicator in Terminal Header */
    const char *mode_tag = (tip_get_mode() == TIP_MODE_HIRAGANA) ? "[Mode: JP (あ) | F10: Switch]" :
                           ((tip_get_mode() == TIP_MODE_KATAKANA) ? "[Mode: JP (ア) | F10: Switch]" :
                            "[Mode: EN (A) | F10: Switch]");
    drw_tc_string(dev, dev->width - 240, 6, mode_tag, COLOR_CYAN, COLOR_TERM_BG);

    int row_h = 16;
    int max_disp_rows = (dev->height - 40) / row_h;
    if (max_disp_rows < 5) max_disp_rows = 5;

    int start_line = 0;
    if (st->total_lines > max_disp_rows) {
        start_line = st->total_lines - max_disp_rows;
    }

    int y = 24;
    for (int i = start_line; i < st->total_lines; i++) {
        COLOR col = st->line_cols[i];
        if (col == 0) col = COLOR_WHITE;
        drw_tc_string(dev, 8, y, st->lines[i], col, COLOR_TERM_BG);
        y += row_h;
    }

    /* Draw Active Prompt Line with Cursor and TIP Inline Preview */
    char prompt_line[300];
    snprintf(prompt_line, sizeof(prompt_line), "%s%s", st->prompt, st->input_buf);
    drw_tc_string(dev, 8, y, prompt_line, COLOR_WHITE, COLOR_TERM_BG);

    int prompt_pixel_w = 8 + gterm_calc_text_pixel_width(prompt_line);
    if (wnd->focused && tip_get_state() != TIP_STATE_IDLE) {
        char comp_buf[128];
        tip_get_converted_text(comp_buf, sizeof(comp_buf));
        BOOL is_dotted = (tip_get_state() == TIP_STATE_PRECOMP);
        drw_tc_string_underlined(dev, prompt_pixel_w, y, comp_buf, COLOR_CYAN, COLOR_TERM_BG, is_dotted);
        tip_set_caret_pos(wnd->bounds.left + prompt_pixel_w, wnd->bounds.top + y);
    } else if (wnd->focused) {
        drw_tc_string(dev, prompt_pixel_w, y, "_", COLOR_WHITE, COLOR_TERM_BG);
    }
}

static void destroy_gterm(WND *wnd) {
    if (!wnd) return;
    GTermState *st = (GTermState*)(uintptr_t)wnd->user_data;
    if (st) {
        free(st);
        wnd->user_data = 0;
    }
}

WND* open_gterm_window(void) {
    static int s_gterm_spawn_count = 0;
    H x = 220 + (s_gterm_spawn_count % 6) * 24;
    H y = 160 + (s_gterm_spawn_count % 6) * 24;
    s_gterm_spawn_count++;

    WND *wnd = opn_wnd("B-System Terminal (gterm)", x, y, 560, 360,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER | WND_ATTR_RESIZE);
    if (wnd) {
        GTermState *st = (GTermState*)calloc(1, sizeof(GTermState));
        if (st) {
            gterm_init_banner(st);
            gterm_execute_cmd(wnd, st, "ver");
            wnd->user_data = (VW)(uintptr_t)st;
        }
        wnd->paint = paint_gterm;
        wnd->event_handler = handle_gterm_event;
        wnd->destroy = destroy_gterm;
        tip_cancel();
        tip_set_mode(TIP_MODE_ASCII); /* Terminal strictly starts in EN mode */
        top_wnd(wnd);
    }
    return wnd;
}
