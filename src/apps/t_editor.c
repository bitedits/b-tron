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
#endif

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
    while (line[i]) {
        int consumed = 0;
        TC code = utf8_to_tc(&line[i], &consumed);
        int step = (consumed > 0 ? consumed : 1);
        int gw = (code < 128) ? 8 : 16;
        if (target_x < cur_x + gw / 2) {
            return i;
        }
        cur_x += gw;
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

static void handle_t_editor_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;
    TEditor *ed = (wnd->user_data) ? (TEditor*)(uintptr_t)wnd->user_data : &g_teditor;

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - (wnd->bounds.left + 4);
        H rel_y = evt->pos.y - (wnd->bounds.top + 26);

        /* Toolbar button click checks */
        if (rel_y >= 0 && rel_y <= 24) {
            if (rel_x >= 4 && rel_x <= 40) {
                /* [New] */
                teditor_init_default(ed);
                ed->total_lines = 1;
                ed->lines[0][0] = '\0';
            } else if (rel_x >= 44 && rel_x <= 84) {
                /* [Open] */
                teditor_init_default(ed);
            } else if (rel_x >= 88 && rel_x <= 128) {
                /* [Save] */
                ed->is_modified = FALSE;
            } else if (rel_x >= 132 && rel_x <= 168) {
                /* [Cut] */
                teditor_copy_selection(ed);
                teditor_delete_selection(ed);
            } else if (rel_x >= 172 && rel_x <= 212) {
                /* [Copy] */
                teditor_copy_selection(ed);
            } else if (rel_x >= 216 && rel_x <= 260) {
                /* [Paste] */
                teditor_paste_clipboard(ed);
            } else if (rel_x >= 266 && rel_x <= 306) {
                /* [ - ] Shrink Editor Window */
                if (wnd->bounds.right - wnd->bounds.left > 480) wnd->bounds.right -= 60;
                if (wnd->bounds.bottom - wnd->bounds.top > 300) wnd->bounds.bottom -= 40;
            } else if (rel_x >= 310 && rel_x <= 350) {
                /* [ + ] Expand Editor Window */
                if (wnd->bounds.right - wnd->bounds.left < 980) wnd->bounds.right += 60;
                if (wnd->bounds.bottom - wnd->bounds.top < 650) wnd->bounds.bottom += 40;
            } else if (rel_x >= 354 && rel_x <= 480) {
                /* [IME: あ/A (F10)] Toolbar Button Click */
                tip_toggle_mode();
            }
            return;
        }

        /* Status Bar Footer click -> Toggle JP / EN mode */
        H client_h = wnd->dev ? wnd->dev->height : (wnd->bounds.bottom - wnd->bounds.top - 26);
        if (rel_y >= client_h - 22) {
            tip_toggle_mode();
            return;
        }

        /* Editor client click -> Move cursor */
        if (rel_y >= 30) {
            int click_r = (rel_y - 30) / 18 + ed->scroll_row;
            if (click_r >= 0 && click_r < ed->total_lines) {
                ed->cursor_row = click_r;
                ed->cursor_col = teditor_find_byte_offset_from_x(ed->lines[click_r], rel_x);
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

static void paint_t_editor(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    TEditor *ed = (wnd->user_data) ? (TEditor*)(uintptr_t)wnd->user_data : &g_teditor;

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
    drw_tc_string(dev, 268, 4, "[-]", COLOR_BLACK, COLOR_LTGRAY);
    drw_tc_string(dev, 312, 4, "[+]", COLOR_BLACK, COLOR_LTGRAY);

    /* Dedicated Toolbar IME Switcher Button */
    RECT ime_tb_btn = { 354, 2, 476, 22 };
    COLOR ime_bg = (tip_get_mode() == TIP_MODE_ASCII) ? COLOR_LTGRAY : COLOR_CYAN;
    fill_rec(dev, &ime_tb_btn, ime_bg);
    drw_rec(dev, &ime_tb_btn);
    const char *tb_ime_tag = (tip_get_mode() == TIP_MODE_HIRAGANA) ? "[IME: あ (F10)]" :
                             ((tip_get_mode() == TIP_MODE_KATAKANA) ? "[IME: ア (F10)]" :
                              "[IME: A (F10)]");
    drw_tc_string(dev, 358, 4, tb_ime_tag, COLOR_BLACK, 0x00000000);

    /* Document Status Title */
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "%s%s", ed->filename, ed->is_modified ? " *" : "");
    drw_tc_string(dev, dev->width - 120, 4, title_buf, COLOR_NAVY, COLOR_LTGRAY);

    /* Render Gutter & Text Lines */
    int view_rows = (dev->height - 60) / 18;
    if (view_rows < 1) view_rows = 1;
    int start_r = ed->scroll_row;
    int end_r = start_r + view_rows;
    if (end_r > ed->total_lines) end_r = ed->total_lines;

    int y = 30;
    for (int r_idx = start_r; r_idx < end_r; r_idx++) {
        /* Gutter line number */
        char num_str[10];
        snprintf(num_str, sizeof(num_str), "%2d|", r_idx + 1);
        drw_tc_string(dev, 6, y, num_str, COLOR_GRAY, COLOR_WHITE);

        /* Line content */
        const char *line = ed->lines[r_idx];
        const char *p = line;
        int byte_idx = 0;
        int x = 36;

        while (*p && x < dev->width - 16) {
            int consumed = 0;
            TC code = utf8_to_tc(p, &consumed);
            int step = (consumed > 0 ? consumed : 1);
            H gw = (code < 128) ? 8 : 16;

            char ch_str[8];
            int n = (step < 7) ? step : 7;
            for (int k = 0; k < n; k++) ch_str[k] = p[k];
            ch_str[n] = '\0';

            /* Check if character is inside CUA selection range */
            BOOL is_selected = FALSE;
            if (ed->sel_active) {
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
            }

            COLOR fg = is_selected ? COLOR_WHITE : COLOR_BLACK;
            COLOR bg = is_selected ? COLOR_NAVY : COLOR_WHITE;

            drw_tc_string(dev, x, y, ch_str, fg, bg);

            p += step;
            byte_idx += step;
            x += gw;
        }

        /* Draw Blinking/Solid Cursor and Inline TIP Composition */
        if (r_idx == ed->cursor_row) {
            int cur_x = 36;
            const char *lp = line;
            int bi = 0;
            while (*lp && bi < ed->cursor_col) {
                int consumed = 0;
                TC code = utf8_to_tc(lp, &consumed);
                int step = (consumed > 0 ? consumed : 1);
                cur_x += (code < 128) ? 8 : 16;
                lp += step;
                bi += step;
            }
            if (cur_x >= 36 && cur_x < dev->width - 10) {
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
                           ((tip_get_mode() == TIP_MODE_KATAKANA) ? "ア" : "A");
    char status_buf[128];
    snprintf(status_buf, sizeof(status_buf), " Line %d, Col %d  |  TRON-Code  |  [CUA INS]",
             ed->cursor_row + 1, ed->cursor_col + 1);
    drw_tc_string(dev, 8, dev->height - 16, status_buf, COLOR_BLACK, COLOR_LTGRAY);

    /* Interactive Status Bar Mozc Mode Badge Button */
    RECT mode_badge = { dev->width - 160, dev->height - 19, dev->width - 4, dev->height - 2 };
    fill_rec(dev, &mode_badge, (tip_get_mode() == TIP_MODE_ASCII) ? COLOR_LTGRAY : COLOR_CYAN);
    drw_rec(dev, &mode_badge);
    char badge_str[32];
    snprintf(badge_str, sizeof(badge_str), "[Mozc: %s (F10)]", mode_str);
    drw_tc_string(dev, mode_badge.left + 6, mode_badge.top + 2, badge_str, COLOR_BLACK, 0x00000000);
}

WND* open_t_editor_window(void) {
    TEditor *ed = (TEditor*)calloc(1, sizeof(TEditor));
    if (!ed) return NULL;
    teditor_init_default(ed);

    WND *wnd = opn_wnd("T-Editor - BTRON3_Report.txt", 220, 50, 760, 480,
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
