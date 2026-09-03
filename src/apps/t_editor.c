#include <btron/t_editor.h>
/*
 * B-System (BTRON 3.20) BTRON Accessory: TRON CUA Text Editor Window (t_editor)
 * Pure Specification-based implementation of Sakamura BTRON / BTRON3 Architecture.
 */

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
#include <dirent.h>
#include <sys/stat.h>
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
#define memset  tkl_memset
#define memcpy  tkl_memcpy
#define memmove tkl_memmove
#define strlen  tkl_strlen
#define strstr  tkl_strstr
#endif

/* TEditor struct is defined in <btron/t_editor.h> */

static TEditor g_teditor;
static char g_clipboard[2048] = "";

static void teditor_init_default(TEditor *ed) {
    memset(ed, 0, sizeof(TEditor));
    strncpy(ed->filename, "BTRON3_Report.txt", sizeof(ed->filename) - 1);

    const char *initial_doc[] = {
        "件名：【BTRON3仕様の新実装】におけるBTRON環境開発のご報告と情報共有のお願い",
        "宛先：ノルティアオーダー／TADワーキンググループ 小島秀樹様",
        "",
        "突然のご連絡失礼いたします。私は「bitedits」というオープンソース・プロジェクトで開発を行っている者です。",
        "現在、私たちはモダンな仮想化環境（seL4 / VirtIO / SDL2）の上で動作する、",
        "BTRON3（Business TRON）仕様のクリーンルーム実装「BTRON System Plane for OS.1」を開発しております。",
        "",
        "貴団体が長年にわたり超機能分散環境（HFDS）の基盤整備やTAD仕様の策定・維持にご尽力されていることを知り、",
        "私たちの成果をご報告するとともに、BTRONの技術的エコシステムの方々にぜひ共有させていただきたくご連絡いたしました。",
        "プロジェクトのリポジトリ・ドキュメント https://bitedits.github.io/btron/",
        "",
        "私たちの実装では、以下のようなBTRONの特徴的なアーキテクチャをモダンなC99ベースで再現することを目指しています：",
        "・μITRONリアルタイムタスク生成およびカーネルプリミティブのシミュレーション",
        "・Sakamuraグラフィックエンジン（DP.H）に基づく2Dベクトル/ラスタ描画",
        "・実オブジェクト／仮想オブジェクト（Real/Virt Body）のハイパーデータエンジンの再現",
        "・TRONコードによる多言語文字システムとTADセグメントのサポート",
        "",
        "私たちは、BTRONが持つ「直感的で先進的なハイパーデータ構造」という思想を、現代のセキュアなマイクロカーネル環境で復活させたいと考えております。",
        "もしよろしければ、仕様の解釈やTADデータの互換性などについて、貴団体の知見を拝見させていただけますと幸いです。",
        "また、日本のBTRONコミュニティや開発者の方々にご紹介いただけますと大変光栄に思います。",
        "お忙しいところ恐縮ですが、ご一読いただけますと幸いです。何卒よろしくお願い申し上げます。",
        "",
        "プロジェクト開発チーム（Namdak Tonpa Norbu）https://bitedits.github.io/btron/"
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
    ed->active_menu = -1;
    ed->hover_menu = -1;
    ed->hover_item = -1;
    ed->active_submenu = -1;
    ed->hover_subitem = -1;
    ed->show_line_nums = TRUE;
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

    const char *p = g_clipboard;
    BOOL first = TRUE;
    while (*p) {
        /* Find next line boundary */
        char line[TEDITOR_MAX_COLS];
        int line_len = 0;
        while (*p && *p != '\n' && line_len < TEDITOR_MAX_COLS - 1) {
            line[line_len++] = *p++;
        }
        line[line_len] = '\0';
        if (*p == '\n') p++;

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

        int insert_len = line_len;
        int cur_len = (int)strlen(ed->lines[ed->cursor_row]);
        if (cur_len + insert_len < TEDITOR_MAX_COLS - 1) {
            memmove(&ed->lines[ed->cursor_row][ed->cursor_col + insert_len],
                    &ed->lines[ed->cursor_row][ed->cursor_col],
                    cur_len - ed->cursor_col + 1);
            memcpy(&ed->lines[ed->cursor_row][ed->cursor_col], line, insert_len);
            ed->cursor_col += insert_len;
        }

        first = FALSE;
    }

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

static void teditor_insert_text(TEditor *ed, const char *text) {
    if (!text || !*text) return;
    if (ed->sel_active) teditor_delete_selection(ed);

    int r = ed->cursor_row;
    int c = ed->cursor_col;
    int cur_len = (int)strlen(ed->lines[r]);
    int ins_len = (int)strlen(text);

    if (cur_len + ins_len < TEDITOR_MAX_COLS - 1) {
        memmove(&ed->lines[r][c + ins_len], &ed->lines[r][c], cur_len - c + 1);
        memcpy(&ed->lines[r][c], text, ins_len);
        ed->cursor_col += ins_len;
        ed->is_modified = TRUE;
    }
    teditor_ensure_cursor_visible(ed);
}

/* UTF-8 Character Boundary Helpers */
static int utf8_next_offset(const char *s, int cur_offset) {
    if (!s || cur_offset >= (int)strlen(s)) return cur_offset;
    const unsigned char *p = (const unsigned char *)s + cur_offset;
    int len = 1;
    if ((*p & 0x80) == 0) len = 1;
    else if ((*p & 0xE0) == 0xC0) len = 2;
    else if ((*p & 0xF0) == 0xE0) len = 3;
    else if ((*p & 0xF8) == 0xF0) len = 4;
    return cur_offset + len;
}

static int utf8_prev_offset(const char *s, int cur_offset) {
    if (!s || cur_offset <= 0) return 0;
    int pos = cur_offset - 1;
    const unsigned char *p = (const unsigned char *)s;
    while (pos > 0 && (p[pos] & 0xC0) == 0x80) {
        pos--;
    }
    return pos;
}

static int teditor_find_byte_offset_from_x(const char *line, int target_x) {
    if (!line || target_x <= 36) return 0;
    int cur_x = 36;
    int i = 0;
    TC prev_code = 0;
    while (line[i]) {
        int consumed = 0;
        TC code = utf8_to_tc(&line[i], &consumed);
        int step = (consumed > 0 ? consumed : 1);
        int gw = tc_get_char_advance(code, prev_code);
        if (gw > 0 && target_x < cur_x + gw / 2) {
            return i;
        }
        cur_x += gw;
        if (gw > 0 || (code >> 8) != 0x6F) {
            prev_code = code;
        }
        i += step;
    }
    return i;
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
        int prev_c = utf8_prev_offset(ed->lines[r], c);
        int len = (int)strlen(ed->lines[r]);
        memmove(&ed->lines[r][prev_c], &ed->lines[r][c], len - c + 1);
        ed->cursor_col = prev_c;
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

static void teditor_delete_char_forward(TEditor *ed) {
    if (ed->sel_active) {
        teditor_delete_selection(ed);
        return;
    }

    int r = ed->cursor_row;
    int c = ed->cursor_col;
    int len = (int)strlen(ed->lines[r]);

    if (c < len) {
        int next_c = utf8_next_offset(ed->lines[r], c);
        memmove(&ed->lines[r][c], &ed->lines[r][next_c], len - next_c + 1);
        ed->is_modified = TRUE;
    } else if (r < ed->total_lines - 1) {
        int next_len = (int)strlen(ed->lines[r + 1]);
        if (len + next_len < TEDITOR_MAX_COLS - 1) {
            strncat(ed->lines[r], ed->lines[r + 1], TEDITOR_MAX_COLS - len - 1);
            for (int i = r + 1; i < ed->total_lines - 1; i++) {
                strncpy(ed->lines[i], ed->lines[i + 1], TEDITOR_MAX_COLS - 1);
            }
            ed->total_lines--;
            ed->is_modified = TRUE;
        }
    }
    teditor_ensure_cursor_visible(ed);
}

/* ASCII Key with Shift / CapsLock Translation */
static char get_ascii_char_with_shift(UW key, uint16_t mod) {
    BOOL shift = (mod & BTRON_KMOD_SHIFT) != 0;
    BOOL caps  = (mod & BTRON_KMOD_CAPS) != 0;

    /* If key is already uppercase 'A'..'Z' */
    if (key >= 'A' && key <= 'Z') {
        if (shift ^ caps) return (char)key;
        return (char)(key - 'A' + 'a');
    }

    /* If key is lowercase 'a'..'z' */
    if (key >= 'a' && key <= 'z') {
        if (shift ^ caps) return (char)(key - 'a' + 'A');
        return (char)key;
    }

    /* Shifted punctuation and numbers */
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

static void destroy_t_editor(WND *wnd) {
    if (wnd && wnd->user_data) {
        TEditor *ed = (TEditor*)(uintptr_t)wnd->user_data;
        free(ed);
        wnd->user_data = 0;
    }
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) WND* open_vobj_manager_window(void) {
    return NULL;
}
#else
extern WND* open_vobj_manager_window(void);
#endif

/* ── BTRON 3.20 & BeOS-Style Menu System ─────────────────────────────── */
typedef enum {
    TMENU_FILE = 0,
    TMENU_EDIT,
    TMENU_VIEW,
    TMENU_VOBJ,
    TMENU_HELP,
    TMENU_COUNT
} TMenuId;

enum {
    TCMD_NONE = 0,
    TCMD_FILE_NEW,
    TCMD_FILE_OPEN_SUBMENU,
    TCMD_FILE_OPEN_ASSET,
    TCMD_FILE_SAVE,
    TCMD_FILE_CLOSE,
    TCMD_EDIT_UNDO,
    TCMD_EDIT_CUT,
    TCMD_EDIT_COPY,
    TCMD_EDIT_PASTE,
    TCMD_EDIT_SELECT_ALL,
    TCMD_VIEW_ZOOM_IN,
    TCMD_VIEW_ZOOM_OUT,
    TCMD_VIEW_TOGGLE_LINES,
    TCMD_VOBJ_INSERT,
    TCMD_VOBJ_CABINET,
    TCMD_HELP_ABOUT
};

typedef struct {
    const char *label;
    const char *accel;
    int cmd;
    BOOL enabled;
    BOOL has_submenu;
} TMenuItem;

typedef struct {
    const char *title;
    RECT rect; /* in window coordinates: top=0, bottom=21 */
    int item_count;
    TMenuItem items[8];
} TMenuHeader;

static const TMenuHeader g_menus[TMENU_COUNT] = {
    {
        "ファイル(F)", { 6, 0, 82, 21 }, 5,
        {
            { "新規作成 (New)",         "Ctrl+N", TCMD_FILE_NEW,          TRUE,  FALSE },
            { "開く (Open)",             "Ctrl+O", TCMD_FILE_OPEN_SUBMENU,  TRUE,  TRUE  },
            { "---",                     "",       TCMD_NONE,              FALSE, FALSE },
            { "上書き保存 (Save)",       "Ctrl+S", TCMD_FILE_SAVE,         TRUE,  FALSE },
            { "閉じる (Close)",         "Ctrl+W", TCMD_FILE_CLOSE,        TRUE,  FALSE }
        }
    },
    {
        "編集(E)", { 86, 0, 146, 21 }, 6,
        {
            { "元に戻す (Undo)",         "Ctrl+Z", TCMD_EDIT_UNDO,         FALSE, FALSE },
            { "---",                     "",       TCMD_NONE,              FALSE, FALSE },
            { "切り取り (Cut)",         "Ctrl+X", TCMD_EDIT_CUT,          TRUE,  FALSE },
            { "コピー (Copy)",           "Ctrl+C", TCMD_EDIT_COPY,         TRUE,  FALSE },
            { "貼り付け (Paste)",         "Ctrl+V", TCMD_EDIT_PASTE,        TRUE,  FALSE },
            { "すべて選択 (Select All)", "Ctrl+A", TCMD_EDIT_SELECT_ALL,    TRUE,  FALSE }
        }
    },
    {
        "表示(V)", { 150, 0, 210, 21 }, 4,
        {
            { "拡大 (Zoom In)",         "+",      TCMD_VIEW_ZOOM_IN,      TRUE,  FALSE },
            { "縮小 (Zoom Out)",        "-",      TCMD_VIEW_ZOOM_OUT,     TRUE,  FALSE },
            { "---",                     "",       TCMD_NONE,              FALSE, FALSE },
            { "行番号表示 (Line Nums)",  "",       TCMD_VIEW_TOGGLE_LINES, TRUE,  FALSE }
        }
    },
    {
        "仮身(O)", { 214, 0, 278, 21 }, 2,
        {
            { "仮身を挿入 (Insert Fusen)", "",     TCMD_VOBJ_INSERT,       TRUE,  FALSE },
            { "実身キャビネット (Cabinet)", "",    TCMD_VOBJ_CABINET,      TRUE,  FALSE }
        }
    },
    {
        "ヘルプ(H)", { 282, 0, 352, 21 }, 1,
        {
            { "T-Editor について (About)", "",     TCMD_HELP_ABOUT,        TRUE,  FALSE }
        }
    }
};

int teditor_get_asset_files(char files[][64], int max_files) {
    if (!files || max_files <= 0) return 0;
    int count = 0;

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    const char *dirs[] = { "assets/texts", "assets", NULL };
    for (int d_idx = 0; dirs[d_idx] && count < max_files; d_idx++) {
        DIR *d = opendir(dirs[d_idx]);
        if (!d) continue;
        struct dirent *de;
        while ((de = readdir(d)) != NULL && count < max_files) {
            if (de->d_name[0] == '.') continue;
            size_t nlen = strlen(de->d_name);
            if (nlen > 4 && strcmp(de->d_name + nlen - 4, ".txt") == 0) {
                BOOL dup = FALSE;
                for (int i = 0; i < count; i++) {
                    if (strcmp(files[i], de->d_name) == 0) { dup = TRUE; break; }
                }
                if (!dup) {
                    strncpy(files[count], de->d_name, 63);
                    files[count][63] = '\0';
                    count++;
                }
            }
        }
        closedir(d);
    }
#endif

    if (count == 0) {
        strncpy(files[0], "BTRON3_Report.txt", 63);
        count++;
        if (max_files > 1) {
            strncpy(files[1], "Heart_Sutra_Tibetan.txt", 63);
            count++;
        }
    }
    return count;
}

void teditor_open_menu(TEditor *ed, int menu_idx) {
    if (!ed || menu_idx < 0 || menu_idx >= TMENU_COUNT) return;
    ed->active_menu = menu_idx;
    ed->hover_menu = menu_idx;
    ed->hover_item = -1;
    ed->active_submenu = -1;
    ed->hover_subitem = -1;
}

void teditor_close_menu(TEditor *ed) {
    if (!ed) return;
    ed->active_menu = -1;
    ed->hover_menu = -1;
    ed->hover_item = -1;
    ed->active_submenu = -1;
    ed->hover_subitem = -1;
}

static void teditor_execute_menu_cmd(TEditor *ed, WND *wnd, int cmd, int sub_idx) {
    switch (cmd) {
        case TCMD_FILE_NEW:
            teditor_init_default(ed);
            ed->total_lines = 1;
            ed->lines[0][0] = '\0';
            snprintf(wnd->title, sizeof(wnd->title), "T-Editor - Untitled.txt");
            break;
        case TCMD_FILE_OPEN_ASSET: {
            char files[32][64];
            int cnt = teditor_get_asset_files(files, 32);
            if (sub_idx >= 0 && sub_idx < cnt) {
                char path[128];
                snprintf(path, sizeof(path), "assets/texts/%s", files[sub_idx]);
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
                FILE *fp = fopen(path, "r");
                if (!fp) {
                    snprintf(path, sizeof(path), "assets/%s", files[sub_idx]);
                } else {
                    fclose(fp);
                }
#endif
                teditor_load_file(ed, path);
                snprintf(wnd->title, sizeof(wnd->title), "T-Editor - %s", ed->filename);
            }
            break;
        }
        case TCMD_FILE_SAVE:
            teditor_save_file(ed, ed->filename);
            ed->is_modified = FALSE;
            break;
        case TCMD_FILE_CLOSE:
            cls_wnd(wnd);
            break;
        case TCMD_EDIT_CUT:
            teditor_copy_selection(ed);
            teditor_delete_selection(ed);
            break;
        case TCMD_EDIT_COPY:
            teditor_copy_selection(ed);
            break;
        case TCMD_EDIT_PASTE:
            teditor_paste_clipboard(ed);
            break;
        case TCMD_EDIT_SELECT_ALL:
            ed->sel_active = TRUE;
            ed->sel_start_r = 0;
            ed->sel_start_c = 0;
            ed->sel_end_r = ed->total_lines - 1;
            ed->sel_end_c = (int)strlen(ed->lines[ed->total_lines - 1]);
            ed->cursor_row = ed->sel_end_r;
            ed->cursor_col = ed->sel_end_c;
            break;
        case TCMD_VIEW_ZOOM_IN:
            if (wnd->bounds.right - wnd->bounds.left < 980) wnd->bounds.right += 60;
            if (wnd->bounds.bottom - wnd->bounds.top < 650) wnd->bounds.bottom += 40;
            break;
        case TCMD_VIEW_ZOOM_OUT:
            if (wnd->bounds.right - wnd->bounds.left > 480) wnd->bounds.right -= 60;
            if (wnd->bounds.bottom - wnd->bounds.top > 300) wnd->bounds.bottom -= 40;
            break;
        case TCMD_VIEW_TOGGLE_LINES:
            ed->show_line_nums = !ed->show_line_nums;
            break;
        case TCMD_VOBJ_INSERT:
            teditor_insert_text(ed, "[仮身: #101 図形 (Diagram.draw)]\n");
            break;
        case TCMD_VOBJ_CABINET:
            if (open_vobj_manager_window) open_vobj_manager_window();
            break;
        case TCMD_HELP_ABOUT:
            teditor_insert_text(ed, "\n--- BTRON3 3.20 T-Editor Word Processor (Cleanroom Edition) ---\n");
            break;
        default:
            break;
    }
}

static BOOL teditor_handle_menu_mouse_move(TEditor *ed, H x, H y) {
    if (!ed || ed->active_menu < 0) return FALSE;

    /* 1. Check if moving over another top-level menu header (BeOS fluid tracking) */
    if (y >= 0 && y <= 21) {
        for (int m = 0; m < TMENU_COUNT; m++) {
            if (x >= g_menus[m].rect.left && x <= g_menus[m].rect.right) {
                if (ed->active_menu != m) {
                    ed->active_menu = m;
                    ed->hover_menu = m;
                    ed->hover_item = -1;
                    ed->active_submenu = -1;
                    ed->hover_subitem = -1;
                    return TRUE;
                }
            }
        }
    }

    const TMenuHeader *hdr = &g_menus[ed->active_menu];
    H menu_x = hdr->rect.left;
    H menu_y = 21;
    H menu_w = 210;
    H menu_h = hdr->item_count * 22 + 6;

    /* 2. Check if moving inside cascading submenu (if open) */
    if (ed->active_menu == TMENU_FILE && ed->active_submenu == 1) {
        char files[32][64];
        int file_cnt = teditor_get_asset_files(files, 32);
        H sub_x = menu_x + menu_w - 2;
        H sub_y = menu_y + 3 + (1 * 22);
        H sub_w = 240;
        H sub_h = file_cnt * 22 + 6;

        if (x >= sub_x && x <= sub_x + sub_w && y >= sub_y && y <= sub_y + sub_h) {
            int sub_idx = (y - (sub_y + 3)) / 22;
            if (sub_idx >= 0 && sub_idx < file_cnt) {
                ed->hover_subitem = sub_idx;
                return TRUE;
            }
        }
    }

    /* 3. Check if moving inside active dropdown menu */
    if (x >= menu_x && x <= menu_x + menu_w && y >= menu_y && y <= menu_y + menu_h) {
        int idx = (y - (menu_y + 3)) / 22;
        if (idx >= 0 && idx < hdr->item_count) {
            if (hdr->items[idx].cmd != TCMD_NONE) {
                ed->hover_item = idx;
                if (hdr->items[idx].has_submenu) {
                    ed->active_submenu = idx;
                } else {
                    ed->active_submenu = -1;
                    ed->hover_subitem = -1;
                }
                return TRUE;
            }
        }
    }

    return FALSE;
}

static BOOL teditor_handle_menu_mouse_down(TEditor *ed, WND *wnd, H x, H y) {
    if (!ed) return FALSE;

    /* If a menu is open, handle clicks inside dropdown or submenu */
    if (ed->active_menu >= 0) {
        const TMenuHeader *hdr = &g_menus[ed->active_menu];
        H menu_x = hdr->rect.left;
        H menu_y = 21;
        H menu_w = 210;
        H menu_h = hdr->item_count * 22 + 6;

        /* Click inside cascading submenu */
        if (ed->active_menu == TMENU_FILE && ed->active_submenu == 1) {
            char files[32][64];
            int file_cnt = teditor_get_asset_files(files, 32);
            H sub_x = menu_x + menu_w - 2;
            H sub_y = menu_y + 3 + (1 * 22);
            H sub_w = 240;
            H sub_h = file_cnt * 22 + 6;

            if (x >= sub_x && x <= sub_x + sub_w && y >= sub_y && y <= sub_y + sub_h) {
                int sub_idx = (y - (sub_y + 3)) / 22;
                if (sub_idx >= 0 && sub_idx < file_cnt) {
                    teditor_execute_menu_cmd(ed, wnd, TCMD_FILE_OPEN_ASSET, sub_idx);
                    teditor_close_menu(ed);
                    return TRUE;
                }
            }
        }

        /* Click inside active dropdown menu */
        if (x >= menu_x && x <= menu_x + menu_w && y >= menu_y && y <= menu_y + menu_h) {
            int idx = (y - (menu_y + 3)) / 22;
            if (idx >= 0 && idx < hdr->item_count) {
                if (hdr->items[idx].has_submenu) {
                    ed->active_submenu = idx;
                    return TRUE;
                } else if (hdr->items[idx].enabled) {
                    teditor_execute_menu_cmd(ed, wnd, hdr->items[idx].cmd, -1);
                    teditor_close_menu(ed);
                    return TRUE;
                }
            }
        }

        /* Click on another menu header */
        if (y >= 0 && y <= 21) {
            for (int m = 0; m < TMENU_COUNT; m++) {
                if (x >= g_menus[m].rect.left && x <= g_menus[m].rect.right) {
                    if (ed->active_menu == m) {
                        teditor_close_menu(ed);
                    } else {
                        teditor_open_menu(ed, m);
                    }
                    return TRUE;
                }
            }
        }

        /* Clicked outside menu -> dismiss menu */
        teditor_close_menu(ed);
        return TRUE;
    }

    /* Menu is not currently open; check if clicking on menu bar header */
    if (y >= 0 && y <= 21) {
        for (int m = 0; m < TMENU_COUNT; m++) {
            if (x >= g_menus[m].rect.left && x <= g_menus[m].rect.right) {
                teditor_open_menu(ed, m);
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void handle_t_editor_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;
    TEditor *ed = (wnd->user_data) ? (TEditor*)(uintptr_t)wnd->user_data : &g_teditor;

    if (evt->type == EV_MOUSE_MOVE) {
        H rel_x = evt->pos.x - (wnd->bounds.left + 4);
        H rel_y = evt->pos.y - (wnd->bounds.top + 26);
        if (ed->active_menu >= 0) {
            teditor_handle_menu_mouse_move(ed, rel_x, rel_y);
        }
        return;
    }

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - (wnd->bounds.left + 4);
        H rel_y = evt->pos.y - (wnd->bounds.top + 26);

        /* 1. If an active menu or cascading submenu is open, handle click */
        if (ed->active_menu >= 0) {
            if (teditor_handle_menu_mouse_down(ed, wnd, rel_x, rel_y)) return;
        }

        /* 2. Menu Bar Header Click (y = 0..21) */
        if (rel_y >= 0 && rel_y <= 21) {
            if (teditor_handle_menu_mouse_down(ed, wnd, rel_x, rel_y)) return;
        }

        /* 3. Toolbar button click checks (y = 22..44) */
        if (rel_y >= 22 && rel_y <= 44) {
            if (rel_x >= 4 && rel_x <= 40) {
                /* [New] */
                teditor_execute_menu_cmd(ed, wnd, TCMD_FILE_NEW, -1);
            } else if (rel_x >= 44 && rel_x <= 84) {
                /* [Open] Open File Menu with cascading document list immediately */
                teditor_open_menu(ed, TMENU_FILE);
                ed->active_submenu = 1;
            } else if (rel_x >= 88 && rel_x <= 128) {
                /* [Save] */
                teditor_execute_menu_cmd(ed, wnd, TCMD_FILE_SAVE, -1);
            } else if (rel_x >= 132 && rel_x <= 168) {
                /* [Cut] */
                teditor_execute_menu_cmd(ed, wnd, TCMD_EDIT_CUT, -1);
            } else if (rel_x >= 172 && rel_x <= 212) {
                /* [Copy] */
                teditor_execute_menu_cmd(ed, wnd, TCMD_EDIT_COPY, -1);
            } else if (rel_x >= 216 && rel_x <= 260) {
                /* [Paste] */
                teditor_execute_menu_cmd(ed, wnd, TCMD_EDIT_PASTE, -1);
            } else if (rel_x >= 266 && rel_x <= 306) {
                /* [ - ] Shrink Editor Window */
                teditor_execute_menu_cmd(ed, wnd, TCMD_VIEW_ZOOM_OUT, -1);
            } else if (rel_x >= 310 && rel_x <= 350) {
                /* [ + ] Expand Editor Window */
                teditor_execute_menu_cmd(ed, wnd, TCMD_VIEW_ZOOM_IN, -1);
            } else if (rel_x >= 354 && rel_x <= 480) {
                /* [IME: あ/A (F10)] Toolbar Button Click */
                tip_toggle_mode();
            }
            return;
        }

        /* 4. Status Bar Footer click -> Toggle JP / EN mode */
        H client_h = wnd->dev ? wnd->dev->height : (wnd->bounds.bottom - wnd->bounds.top - 26);
        if (rel_y >= client_h - 22) {
            tip_toggle_mode();
            return;
        }

        /* 5. Editor client click -> Move cursor */
        if (rel_y >= 46) {
            int click_r = (rel_y - 48) / 18 + ed->scroll_row;
            if (click_r >= 0 && click_r < ed->total_lines) {
                ed->cursor_row = click_r;
                int text_x_offset = ed->show_line_nums ? 36 : 10;
                ed->cursor_col = teditor_find_byte_offset_from_x(ed->lines[click_r], rel_x - (text_x_offset - 36));
                ed->sel_active = FALSE;
                teditor_ensure_cursor_visible(ed);
            }
        }
        return;
    }

    if (evt->type == EV_KEY_DOWN) {
        char commit_buf[128] = "";
        UW key_code = evt->key;
        uint16_t mod = (uint16_t)(uintptr_t)evt->data;

        BOOL shift = (mod & BTRON_KMOD_SHIFT) != 0;
        BOOL ctrl = (mod & BTRON_KMOD_CTRL) != 0;

        /* Check if TIP handles the key event (Japanese IME mode) */
        if (tip_process_key(key_code, mod, commit_buf, sizeof(commit_buf))) {
            if (commit_buf[0] != '\0') {
                teditor_insert_text(ed, commit_buf);
            }
            return;
        }

        UW sym = key_code;

        if (sym == BTRON_KEY_ESCAPE || sym == 27) {
            if (ed->active_menu >= 0) {
                teditor_close_menu(ed);
                return;
            }
        }

        if (ctrl) {
            if (sym == 'c' || sym == 'C') {
                teditor_copy_selection(ed);
                return;
            } else if (sym == 'x' || sym == 'X') {
                teditor_copy_selection(ed);
                teditor_delete_selection(ed);
                return;
            } else if (sym == 'v' || sym == 'V') {
                teditor_paste_clipboard(ed);
                return;
            } else if (sym == 'a' || sym == 'A') {
                ed->sel_active = TRUE;
                ed->sel_start_r = 0;
                ed->sel_start_c = 0;
                ed->sel_end_r = ed->total_lines - 1;
                ed->sel_end_c = (int)strlen(ed->lines[ed->total_lines - 1]);
                ed->cursor_row = ed->sel_end_r;
                ed->cursor_col = ed->sel_end_c;
                return;
            } else if (sym == 's' || sym == 'S') {
                ed->is_modified = FALSE;
                return;
            } else if (sym == 'n' || sym == 'N') {
                teditor_init_default(ed);
                ed->total_lines = 1;
                ed->lines[0][0] = '\0';
                snprintf(wnd->title, sizeof(wnd->title), "T-Editor - Untitled.txt");
                return;
            } else if (sym == 'o' || sym == 'O') {
                /* Ctrl+O: Open File Menu with cascading document list */
                teditor_open_menu(ed, TMENU_FILE);
                ed->active_submenu = 1;
                return;
            } else if (sym == 'w' || sym == 'W') {
                cls_wnd(wnd);
                return;
            }
        }

        if (sym == BTRON_KEY_RETURN || sym == BTRON_KEY_KP_ENTER || sym == '\r' || sym == '\n') {
            teditor_insert_newline(ed);
            return;
        } else if (sym == BTRON_KEY_BACKSPACE || sym == 0x08) {
            teditor_backspace(ed);
            return;
        } else if (sym == BTRON_KEY_DELETE || sym == 0x7F) {
            teditor_delete_char_forward(ed);
            return;
        } else if (sym == BTRON_KEY_LEFT) {
            if (shift && !ed->sel_active) {
                ed->sel_active = TRUE;
                ed->sel_start_r = ed->cursor_row;
                ed->sel_start_c = ed->cursor_col;
            }
            if (ed->cursor_col > 0) {
                ed->cursor_col = utf8_prev_offset(ed->lines[ed->cursor_row], ed->cursor_col);
            } else if (ed->cursor_row > 0) {
                ed->cursor_row--;
                ed->cursor_col = (int)strlen(ed->lines[ed->cursor_row]);
            }
            if (shift) {
                ed->sel_end_r = ed->cursor_row;
                ed->sel_end_c = ed->cursor_col;
            } else {
                ed->sel_active = FALSE;
            }
            teditor_ensure_cursor_visible(ed);
            return;
        } else if (sym == BTRON_KEY_RIGHT) {
            if (shift && !ed->sel_active) {
                ed->sel_active = TRUE;
                ed->sel_start_r = ed->cursor_row;
                ed->sel_start_c = ed->cursor_col;
            }
            int len = (int)strlen(ed->lines[ed->cursor_row]);
            if (ed->cursor_col < len) {
                ed->cursor_col = utf8_next_offset(ed->lines[ed->cursor_row], ed->cursor_col);
            } else if (ed->cursor_row < ed->total_lines - 1) {
                ed->cursor_row++;
                ed->cursor_col = 0;
            }
            if (shift) {
                ed->sel_end_r = ed->cursor_row;
                ed->sel_end_c = ed->cursor_col;
            } else {
                ed->sel_active = FALSE;
            }
            teditor_ensure_cursor_visible(ed);
            return;
        } else if (sym == BTRON_KEY_UP) {
            if (ed->cursor_row > 0) {
                ed->cursor_row--;
                int len = (int)strlen(ed->lines[ed->cursor_row]);
                if (ed->cursor_col > len) ed->cursor_col = len;
            }
            ed->sel_active = FALSE;
            teditor_ensure_cursor_visible(ed);
            return;
        } else if (sym == BTRON_KEY_DOWN) {
            if (ed->cursor_row < ed->total_lines - 1) {
                ed->cursor_row++;
                int len = (int)strlen(ed->lines[ed->cursor_row]);
                if (ed->cursor_col > len) ed->cursor_col = len;
            }
            ed->sel_active = FALSE;
            teditor_ensure_cursor_visible(ed);
            return;
        } else if (sym == BTRON_KEY_HOME) {
            ed->cursor_col = 0;
            ed->sel_active = FALSE;
            teditor_ensure_cursor_visible(ed);
            return;
        } else if (sym == BTRON_KEY_END) {
            ed->cursor_col = (int)strlen(ed->lines[ed->cursor_row]);
            ed->sel_active = FALSE;
            teditor_ensure_cursor_visible(ed);
            return;
        } else if (sym == BTRON_KEY_TAB || sym == '\t') {
            teditor_insert_text(ed, "    ");
            return;
        }

        /* Direct English / Printable ASCII Text Input */
        if (!ctrl && sym >= 32 && sym <= 126) {
            char ch = get_ascii_char_with_shift((UW)sym, mod);
            teditor_insert_char(ed, ch);
            return;
        }
    }
}

static void draw_3d_bevel_box(GDEV *dev, const RECT *r) {
    fill_rec(dev, r, COLOR_LTGRAY);
    drw_rec(dev, r);
    /* 3D highlight: white top and left */
    drw_lin(dev, r->left + 1, r->top + 1, r->right - 2, r->top + 1);
    drw_lin(dev, r->left + 1, r->top + 1, r->left + 1, r->bottom - 2);
    /* 3D shadow: dark gray bottom and right */
    drw_lin(dev, r->left + 1, r->bottom - 2, r->right - 2, r->bottom - 2);
    drw_lin(dev, r->right - 2, r->top + 1, r->right - 2, r->bottom - 2);
}

static void paint_menu_bar(TEditor *ed, GDEV *dev) {
    RECT bar_rect = { 0, 0, dev->width, 21 };
    fill_rec(dev, &bar_rect, COLOR_LTGRAY);
    drw_lin(dev, 0, 21, dev->width, 21);

    for (int m = 0; m < TMENU_COUNT; m++) {
        const TMenuHeader *hdr = &g_menus[m];
        RECT hr = hdr->rect;
        if (ed->active_menu == m) {
            fill_rec(dev, &hr, COLOR_NAVY);
            drw_tc_string(dev, hr.left + 6, hr.top + 3, hdr->title, COLOR_WHITE, 0x00000000);
        } else if (ed->hover_menu == m && ed->active_menu >= 0) {
            fill_rec(dev, &hr, COLOR_WHITE);
            drw_tc_string(dev, hr.left + 6, hr.top + 3, hdr->title, COLOR_BLACK, 0x00000000);
        } else {
            drw_tc_string(dev, hr.left + 6, hr.top + 3, hdr->title, COLOR_BLACK, 0x00000000);
        }
    }
}

static void paint_menu_dropdown_overlay(TEditor *ed, GDEV *dev) {
    if (!ed || ed->active_menu < 0 || ed->active_menu >= TMENU_COUNT) return;

    const TMenuHeader *hdr = &g_menus[ed->active_menu];
    H menu_x = hdr->rect.left;
    H menu_y = 21;
    H menu_w = 210;
    H menu_h = hdr->item_count * 22 + 6;

    RECT menu_box = { menu_x, menu_y, menu_x + menu_w, menu_y + menu_h };
    draw_3d_bevel_box(dev, &menu_box);

    for (int i = 0; i < hdr->item_count; i++) {
        const TMenuItem *it = &hdr->items[i];
        RECT ir = { menu_x + 3, menu_y + 3 + i * 22, menu_x + menu_w - 3, menu_y + 3 + (i + 1) * 22 };

        if (it->cmd == TCMD_NONE) {
            /* Separator */
            drw_lin(dev, ir.left + 4, ir.top + 10, ir.right - 4, ir.top + 10);
            continue;
        }

        BOOL is_hov = (ed->hover_item == i && it->enabled);
        if (is_hov) {
            fill_rec(dev, &ir, COLOR_NAVY);
        }

        COLOR txt_col = is_hov ? COLOR_WHITE : (it->enabled ? COLOR_BLACK : COLOR_GRAY);
        COLOR acc_col = is_hov ? COLOR_LTGRAY : (it->enabled ? COLOR_DKGRAY : COLOR_GRAY);

        drw_tc_string(dev, ir.left + 8, ir.top + 3, it->label, txt_col, 0x00000000);

        if (it->accel && it->accel[0] != '\0') {
            drw_tc_string(dev, ir.right - 56, ir.top + 3, it->accel, acc_col, 0x00000000);
        }

        if (it->has_submenu) {
            drw_tc_string(dev, ir.right - 16, ir.top + 3, "▶", txt_col, 0x00000000);
        }
    }

    /* Draw cascading submenu if active */
    if (ed->active_menu == TMENU_FILE && ed->active_submenu == 1) {
        char files[32][64];
        int file_cnt = teditor_get_asset_files(files, 32);
        H sub_x = menu_x + menu_w - 2;
        H sub_y = menu_y + 3 + (1 * 22);
        H sub_w = 240;
        H sub_h = file_cnt * 22 + 6;

        RECT sub_box = { sub_x, sub_y, sub_x + sub_w, sub_y + sub_h };
        draw_3d_bevel_box(dev, &sub_box);

        for (int f = 0; f < file_cnt; f++) {
            RECT sir = { sub_x + 3, sub_y + 3 + f * 22, sub_x + sub_w - 3, sub_y + 3 + (f + 1) * 22 };
            BOOL is_sub_hov = (ed->hover_subitem == f);
            if (is_sub_hov) {
                fill_rec(dev, &sir, COLOR_NAVY);
            }
            COLOR sub_txt_col = is_sub_hov ? COLOR_WHITE : COLOR_BLACK;

            char fname_buf[80];
            snprintf(fname_buf, sizeof(fname_buf), "📄 %s", files[f]);
            drw_tc_string(dev, sir.left + 8, sir.top + 3, fname_buf, sub_txt_col, 0x00000000);
        }
    }
}

static void paint_t_editor(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    TEditor *ed = (wnd->user_data) ? (TEditor*)(uintptr_t)wnd->user_data : &g_teditor;

    /* Background page */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);
    drw_rec(dev, &r);

    /* ── 1. BTRON 3.20 Standard Menu Bar (BeOS BMenuBar style) ───────────── */
    paint_menu_bar(ed, dev);

    /* ── 2. Quick Action Toolbar Header (y = 21..44) ─────────────────────── */
    RECT tb = { 0, 21, dev->width, 44 };
    fill_rec(dev, &tb, COLOR_LTGRAY);
    drw_lin(dev, 0, 44, dev->width, 44);

    /* CUA Action Buttons */
    drw_tc_string(dev, 6, 25, "[New]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 46, 25, "[Open]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 90, 25, "[Save]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 134, 25, "[Cut]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 174, 25, "[Copy]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 218, 25, "[Paste]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 268, 25, "[-]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 312, 25, "[+]", COLOR_BLACK, COLOR_LTGRAY);

    /* Dedicated Toolbar IME Switcher Button */
    RECT ime_tb_btn = { 354, 23, 476, 42 };
    COLOR ime_bg = (tip_get_mode() == TIP_MODE_ASCII) ? COLOR_LTGRAY : COLOR_CYAN;
    fill_rec(dev, &ime_tb_btn, ime_bg);
    drw_rec(dev, &ime_tb_btn);
    const char *tb_ime_tag = (tip_get_mode() == TIP_MODE_HIRAGANA) ? "[IME: あ (F10)]" :
                             ((tip_get_mode() == TIP_MODE_KATAKANA) ? "[IME: ア (F10)]" :
                              "[IME: A (F10)]");
    drw_tc_string(dev, 358, 25, tb_ime_tag, COLOR_BLACK, 0x00000000);

    /* Document Status Title */
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "%s%s", ed->filename, ed->is_modified ? " *" : "");
    drw_tc_string(dev, dev->width - 120, 25, title_buf, COLOR_NAVY, COLOR_LTGRAY);

    /* ── 3. Render Gutter & Text Lines ────────────────────────────────────── */
    int view_rows = (dev->height - 60) / 18;
    if (view_rows < 1) view_rows = 1;
    int start_r = ed->scroll_row;
    int end_r = start_r + view_rows;
    if (end_r > ed->total_lines) end_r = ed->total_lines;

    int y = 48;
    for (int r_idx = start_r; r_idx < end_r; r_idx++) {
        /* Gutter line number */
        if (ed->show_line_nums) {
            char num_str[10];
            snprintf(num_str, sizeof(num_str), "%2d|", r_idx + 1);
            drw_tc_string(dev, 6, y, num_str, COLOR_GRAY, COLOR_WHITE);
        }

        int text_x_start = ed->show_line_nums ? 36 : 12;

        /* Line content */
        const char *line = ed->lines[r_idx];
        if (!ed->sel_active) {
            drw_tc_string(dev, text_x_start, y, line, COLOR_BLACK, COLOR_WHITE);
        } else {
            const char *p = line;
            int byte_idx = 0;
            int x = text_x_start;
            while (*p && x < dev->width - 16) {
                int consumed = 0;
                TC code = utf8_to_tc(p, &consumed);
                int step = (consumed > 0 ? consumed : 1);
                H gw = (code < 128) ? 8 : 16;
                if ((code >> 8) == 0x6F) {
                    UW cp = 0x0F00 + (code & 0xFF);
                    if ((cp >= 0x0F71 && cp <= 0x0F84) || (cp >= 0x0F90 && cp <= 0x0FBC)) gw = 0;
                    else if (cp == 0x0F0B || cp == 0x0F0D || cp == 0x0F0E) gw = 6;
                    else gw = 14;
                }

                char ch_str[8];
                int n = (step < 7) ? step : 7;
                for (int k = 0; k < n; k++) ch_str[k] = p[k];
                ch_str[n] = '\0';

                BOOL is_selected = FALSE;
                int r1 = ed->sel_start_r, c1 = ed->sel_start_c;
                int r2 = ed->sel_end_r, c2 = ed->sel_end_c;
                if (r1 > r2 || (r1 == r2 && c1 > c2)) {
                    int tr = r1; r1 = r2; r2 = tr;
                    int tc = c1; c1 = c2; c2 = tc;
                }
                if (r_idx > r1 && r_idx < r2) is_selected = TRUE;
                else if (r_idx == r1 && r_idx == r2 && byte_idx >= c1 && byte_idx < c2) is_selected = TRUE;
                else if (r_idx == r1 && r_idx < r2 && byte_idx >= c1) is_selected = TRUE;
                else if (r_idx == r2 && r_idx > r1 && byte_idx < c2) is_selected = TRUE;

                COLOR fg = is_selected ? COLOR_WHITE : COLOR_BLACK;
                COLOR bg = is_selected ? COLOR_NAVY : COLOR_WHITE;

                drw_tc_string(dev, x, y, ch_str, fg, bg);

                p += step;
                byte_idx += step;
                x += gw;
            }
        }

        /* Draw Blinking/Solid Cursor and Inline TIP Composition */
        if (r_idx == ed->cursor_row) {
            int text_x_start = ed->show_line_nums ? 36 : 12;
            int cur_x = text_x_start + tc_calc_string_width(line, ed->cursor_col);
            if (cur_x >= text_x_start && cur_x < dev->width - 10) {
                if (wnd->focused && tip_get_state() != TIP_STATE_IDLE) {
                    char comp_buf[128];
                    tip_get_converted_text(comp_buf, sizeof(comp_buf));
                    BOOL is_dotted = (tip_get_state() == TIP_STATE_PRECOMP);
                    drw_tc_string_underlined(dev, cur_x, y, comp_buf, COLOR_NAVY, COLOR_WHITE, is_dotted);
                    tip_set_caret_pos(wnd->bounds.left + cur_x, wnd->bounds.top + y);
                } else if (wnd->focused) {
                    RECT cursor_rect = { cur_x, y, cur_x + 2, y + 16 };
                    fill_rec(dev, &cursor_rect, COLOR_NAVY);
                }
            }
        }
        y += 18;
    }

    /* Embedded Virtual Body Icon inside Document */
    if (ed->has_vobj) {
        RECT embed_vobj = { 36, dev->height - 45, 230, dev->height - 20 };
        fill_rec(dev, &embed_vobj, COLOR_LTGRAY);
        drw_rec(dev, &embed_vobj);
        char vobj_str[64];
        snprintf(vobj_str, sizeof(vobj_str), "[VOBJ: %s]", ed->vobj_name);
        drw_tc_string(dev, 42, dev->height - 38, vobj_str, COLOR_NAVY, COLOR_LTGRAY);
    }

    /* Status Bar Footer with Mozc indicator (REQ-5) */
    RECT sb = { 0, dev->height - 20, dev->width, dev->height };
    fill_rec(dev, &sb, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 20, dev->width, dev->height - 20);

    const char *mode_str = (tip_get_mode() == TIP_MODE_HIRAGANA) ? "あ" :
                           ((tip_get_mode() == TIP_MODE_KATAKANA) ? "ア" :
                            ((tip_get_mode() == TIP_MODE_TIBETAN) ? "བོད" : "A"));
    char status_buf[128];
    snprintf(status_buf, sizeof(status_buf), " Line %d, Col %d  |  TRON-Code (Tibetan/JIS)  |  [CUA INS]",
             ed->cursor_row + 1, ed->cursor_col + 1);
    drw_tc_string(dev, 8, dev->height - 16, status_buf, COLOR_BLACK, COLOR_LTGRAY);

    /* Interactive Status Bar Mozc Mode Badge Button */
    RECT mode_badge = { dev->width - 160, dev->height - 19, dev->width - 4, dev->height - 2 };
    fill_rec(dev, &mode_badge, (tip_get_mode() == TIP_MODE_ASCII) ? COLOR_LTGRAY : COLOR_CYAN);
    drw_rec(dev, &mode_badge);
    char badge_str[32];
    snprintf(badge_str, sizeof(badge_str), "[TIP: %s (F10)]", mode_str);
    drw_tc_string(dev, mode_badge.left + 6, mode_badge.top + 2, badge_str, COLOR_BLACK, 0x00000000);

    /* ── 4. Floating Menu & Cascading Submenu Overlay (Topmost Layer) ─── */
    paint_menu_dropdown_overlay(ed, dev);
}

WND* open_t_editor_window_with_file(const char *filepath) {
    TEditor *ed = (TEditor*)calloc(1, sizeof(TEditor));
    if (!ed) return NULL;

    ed->active_menu = -1;
    ed->hover_menu = -1;
    ed->hover_item = -1;
    ed->active_submenu = -1;
    ed->hover_subitem = -1;
    ed->show_line_nums = TRUE;

    if (!filepath || teditor_load_file(ed, filepath) != 0) {
        teditor_init_default(ed);
    }

    char title[128];
    snprintf(title, sizeof(title), "T-Editor - %s", ed->filename);

    WND *wnd = opn_wnd(title, 220, 50, 760, 480,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->user_data = (VW)(uintptr_t)ed;
        wnd->paint = paint_t_editor;
        wnd->event_handler = handle_t_editor_event;
        wnd->destroy = destroy_t_editor;
    } else {
        free(ed);
    }
    return wnd;
}

WND* open_t_editor_window(void) {
    return open_t_editor_window_with_file("assets/texts/BTRON3_Report.txt");
}

TEditor* teditor_get_current(void) {
    return &g_teditor;
}


/* ── File Management & Real Object/Virtual Object Storage Subsystem ── */

int teditor_load_file(TEditor *ed, const char *filepath) {
    if (!ed || !filepath) return -1;

    ed->active_menu = -1;
    ed->hover_menu = -1;
    ed->hover_item = -1;
    ed->active_submenu = -1;
    ed->hover_subitem = -1;

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        return -1;
    }

    ed->total_lines = 0;
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->scroll_row = 0;
    ed->scroll_col = 0;
    ed->sel_active = FALSE;
    ed->is_modified = FALSE;

    char line_buf[TEDITOR_MAX_COLS * 2];
    while (fgets(line_buf, sizeof(line_buf), fp) && ed->total_lines < TEDITOR_MAX_ROWS) {
        /* Strip trailing newline */
        size_t len = strlen(line_buf);
        while (len > 0 && (line_buf[len - 1] == '\n' || line_buf[len - 1] == '\r')) {
            line_buf[--len] = '\0';
        }
        strncpy(ed->lines[ed->total_lines], line_buf, TEDITOR_MAX_COLS - 1);
        ed->lines[ed->total_lines][TEDITOR_MAX_COLS - 1] = '\0';
        ed->total_lines++;
    }
    fclose(fp);

    if (ed->total_lines == 0) {
        ed->total_lines = 1;
        ed->lines[0][0] = '\0';
    }

    /* Extract base filename */
    const char *slash = strrchr(filepath, '/');
#ifdef _WIN32
    const char *bslash = strrchr(filepath, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;
#endif
    const char *base = slash ? slash + 1 : filepath;
    strncpy(ed->filename, base, sizeof(ed->filename) - 1);
    ed->filename[sizeof(ed->filename) - 1] = '\0';

    return 0;
#else
    return 0;
#endif
}

int teditor_save_file(TEditor *ed, const char *filepath) {
    if (!ed) return -1;
    const char *target = filepath ? filepath : ed->filename;

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    FILE *fp = fopen(target, "w");
    if (!fp) return -1;

    for (int i = 0; i < ed->total_lines; i++) {
        fprintf(fp, "%s\n", ed->lines[i]);
    }
    fclose(fp);
    ed->is_modified = FALSE;
    return 0;
#else
    ed->is_modified = FALSE;
    return 0;
#endif
}

int teditor_close_file(TEditor *ed) {
    if (!ed) return -1;
    memset(ed->lines, 0, sizeof(ed->lines));
    ed->total_lines = 1;
    ed->lines[0][0] = '\0';
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->scroll_row = 0;
    ed->scroll_col = 0;
    ed->sel_active = FALSE;
    ed->is_modified = FALSE;
    strncpy(ed->filename, "Untitled.txt", sizeof(ed->filename) - 1);
    return 0;
}


