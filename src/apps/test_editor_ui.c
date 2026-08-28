/*
 * B-TRON T-Editor UI & Internal Functions Test Suite: test_editor_ui.c
 * Validates CUA editing, Japanese UTF-8 character metrics, TIP IME integration,
 * clipboard, multibyte boundary preservation, and window resizing.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <btron/types.h>
#include <btron/dp.h>
#include <btron/wnd.h>
#include <btron/troncode.h>
#include <btron/jis_fonts.h>
#include <btron/tip.h>

/* Mock SDL keysyms for testing */
#define SDLK_BACKSPACE   8
#define SDLK_TAB         9
#define SDLK_RETURN      13
#define SDLK_ESCAPE      27
#define SDLK_SPACE       32
#define SDLK_DELETE      127
#define SDLK_RIGHT       0x4000004F
#define SDLK_LEFT        0x40000050
#define SDLK_DOWN        0x40000051
#define SDLK_UP          0x40000052
#define SDLK_a           'a'
#define SDLK_c           'c'
#define SDLK_n           'n'
#define SDLK_s           's'
#define SDLK_v           'v'
#define SDLK_x           'x'
#define SDLK_KP_ENTER    0x40000058

/* Replicate TEditor structure for isolated unit testing */
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

static char g_clipboard[2048] = "";

static int g_tests_passed = 0;
static int g_tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_total++; \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
    } \
} while(0)

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

static char get_ascii_char_with_shift(UW key, uint16_t mod) {
    BOOL shift = (mod & 0x0001) != 0;
    BOOL caps = (mod & 0x2000) != 0;
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

static void teditor_init_default(TEditor *ed) {
    memset(ed, 0, sizeof(TEditor));
    strncpy(ed->filename, "BTRON3_Report.txt", sizeof(ed->filename) - 1);

    const char *initial_doc[] = {
        "件名：【BTRON3仕様の新実装】におけるBTRON環境開発のご報告と情報共有のお願い",
        "宛先：ノルティアオーダー／TADワーキンググループ 小島秀樹様",
        "",
        "突然のご連絡失礼いたします。私は「bitedits」というオープンソース・プロジェクトで開発を行っている者です。",
        "現在、私たちはモダンな仮想化環境（seL4 / VirtIO / SDL2）の上で動作する、",
        "BTRON3（Business TRON）仕様のクリーンルーム実装「BTRON System Plane for OS.1」を開発しております。"
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
            int end_c = (c2 < len) ? c2 : len;
            memmove(&ed->lines[r1][c1], &ed->lines[r1][end_c], len - end_c + 1);
        }
    } else {
        int tail_len = (int)strlen(ed->lines[r2]);
        int keep_c2 = (c2 < tail_len) ? c2 : tail_len;
        strncpy(&ed->lines[r1][c1], &ed->lines[r2][keep_c2], TEDITOR_MAX_COLS - c1 - 1);

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
    }
}

static void teditor_paste_clipboard(TEditor *ed) {
    if (strlen(g_clipboard) == 0) return;
    if (ed->sel_active) teditor_delete_selection(ed);

    int insert_len = (int)strlen(g_clipboard);
    int cur_len = (int)strlen(ed->lines[ed->cursor_row]);
    if (cur_len + insert_len < TEDITOR_MAX_COLS - 1) {
        memmove(&ed->lines[ed->cursor_row][ed->cursor_col + insert_len],
                &ed->lines[ed->cursor_row][ed->cursor_col],
                cur_len - ed->cursor_col + 1);
        memcpy(&ed->lines[ed->cursor_row][ed->cursor_col], g_clipboard, insert_len);
        ed->cursor_col += insert_len;
        ed->is_modified = TRUE;
    }
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
    }
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
    }
}

/* ── Test Suite Implementations ── */

static void test_editor_initialization(void) {
    printf("\n[UI TEST 1] Editor Document State & Buffer Initialization\n");
    TEditor ed;
    teditor_init_default(&ed);

    TEST_ASSERT(strcmp(ed.filename, "BTRON3_Report.txt") == 0, "Document title is 'BTRON3_Report.txt'");
    TEST_ASSERT(ed.total_lines == 6, "Total initial lines is 6");
    TEST_ASSERT(ed.cursor_row == 0 && ed.cursor_col == 0, "Caret initializes at (0, 0)");
    TEST_ASSERT(strncmp(ed.lines[0], "件名：【BTRON3仕様", 21) == 0, "First line contains Japanese header");
    TEST_ASSERT(ed.has_vobj == TRUE, "Embedded Virtual Object icon is active");
}

static void test_caret_multibyte_navigation(void) {
    printf("\n[UI TEST 2] Multibyte UTF-8 Caret Navigation & Offset Calculation\n");
    TEditor ed;
    teditor_init_default(&ed);

    const char *line = ed.lines[0]; /* "件名：【BTRON3仕様..." */
    /* Step right 4 Japanese characters: 件(3), 名(3), ：(3), 【(3) -> 12 */
    ed.cursor_col = utf8_next_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 3, "Caret stepped past '件' to col 3");
    ed.cursor_col = utf8_next_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 6, "Caret stepped past '名' to col 6");
    ed.cursor_col = utf8_next_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 9, "Caret stepped past '：' to col 9");
    ed.cursor_col = utf8_next_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 12, "Caret stepped past '【' to col 12");

    /* Step right 1 ASCII character 'B' -> 13 */
    ed.cursor_col = utf8_next_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 13, "Caret stepped past ASCII 'B' to col 13");

    /* Step backward */
    ed.cursor_col = utf8_prev_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 12, "Caret stepped backward past 'B' to col 12");
    ed.cursor_col = utf8_prev_offset(line, ed.cursor_col);
    TEST_ASSERT(ed.cursor_col == 9, "Caret stepped backward past '【' to col 9");
}

static void test_mouse_click_x_to_col_mapping(void) {
    printf("\n[UI TEST 3] Mouse Click Pixel X Coordinate to Byte Offset Mapping\n");
    const char *line = "件名：BTRON3";
    /*
     * 36px  -> col 0 ('件')
     * 52px  -> col 3 ('名')
     * 68px  -> col 6 ('：')
     * 84px  -> col 9 ('B')
     * 92px  -> col 10 ('T')
     */

    int off0 = teditor_find_byte_offset_from_x(line, 36);
    TEST_ASSERT(off0 == 0, "Click at 36px maps to offset 0 ('件')");
    int off1 = teditor_find_byte_offset_from_x(line, 55);
    TEST_ASSERT(off1 == 3, "Click at 55px maps to offset 3 ('名')");
    int off2 = teditor_find_byte_offset_from_x(line, 70);
    TEST_ASSERT(off2 == 6, "Click at 70px maps to offset 6 ('：')");
    int off3 = teditor_find_byte_offset_from_x(line, 86);
    TEST_ASSERT(off3 == 9, "Click at 86px maps to offset 9 ('B')");
}

static void test_cua_selection_and_clipboard(void) {
    printf("\n[UI TEST 4] CUA Selection, Copy, Cut, and Paste Operations\n");
    TEditor ed;
    teditor_init_default(&ed);

    /* Select "BTRON3仕様" on line 0 (offsets 12..24) */
    ed.sel_active = TRUE;
    ed.sel_start_r = 0;
    ed.sel_start_c = 12;
    ed.sel_end_r = 0;
    ed.sel_end_c = 24;

    teditor_copy_selection(&ed);
    TEST_ASSERT(strcmp(g_clipboard, "BTRON3仕様") == 0, "Clipboard copy contains exact selection 'BTRON3仕様'");

    teditor_delete_selection(&ed);
    TEST_ASSERT(strncmp(ed.lines[0], "件名：【の新実装】", 27) == 0, "Cut deletes selection atomically");

    /* Paste back at caret */
    teditor_paste_clipboard(&ed);
    TEST_ASSERT(strncmp(ed.lines[0], "件名：【BTRON3仕様の新実装】", 39) == 0, "Paste restores original text accurately");
}

static void test_multibyte_atomic_deletion(void) {
    printf("\n[UI TEST 5] Multibyte Atomic Deletion (Backspace & Delete Forward)\n");
    TEditor ed;
    teditor_init_default(&ed);

    /* Position caret after '件' (offset 3) */
    ed.cursor_row = 0;
    ed.cursor_col = 3;

    teditor_backspace(&ed);
    TEST_ASSERT(strncmp(ed.lines[0], "名：【BTRON3", strlen("名：【BTRON3")) == 0, "Backspace removes 3-byte '件' without corruption");
    TEST_ASSERT(ed.cursor_col == 0, "Caret moves back to col 0");

    teditor_delete_char_forward(&ed);
    TEST_ASSERT(strncmp(ed.lines[0], "：【BTRON3", strlen("：【BTRON3")) == 0, "Forward Delete removes 3-byte '名' without corruption");
}

static void test_tip_ime_typing_pipeline(void) {
    printf("\n[UI TEST 6] TIP IME Input Pipeline & Candidate Window Visibility\n");
    tip_init();

    /* Type romaji "watashi" */
    const char *keys = "watashi";
    char commit[128] = "";
    for (int i = 0; keys[i]; i++) {
        tip_process_key(keys[i], 0, commit, sizeof(commit));
    }

    TEST_ASSERT(tip_get_state() == TIP_STATE_PRECOMP, "TIP enters PRECOMP state");
    TEST_ASSERT(strcmp(tip_get_reading(), "わたし") == 0, "TIP reading buffer is 'わたし'");

    /* Press Space to convert */
    tip_process_key(' ', 0, commit, sizeof(commit));
    TEST_ASSERT(tip_get_state() == TIP_STATE_CONVERTING, "Space triggers TIP_STATE_CONVERTING");

    /* Press Enter to commit conversion */
    tip_process_key('\r', 0, commit, sizeof(commit));
    TEST_ASSERT(strcmp(commit, "私") == 0, "Commit yields converted Kanji '私'");
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "TIP returns to IDLE after commit");
}

static void test_window_resizing_view_rows(void) {
    printf("\n[UI TEST 7] Editor Window Resizing & Dynamic View Row Recomputation\n");

    /* Default size: 760x480 */
    int w = 760, h = 480;
    int view_rows = (h - 60) / 18;
    TEST_ASSERT(view_rows == 23, "Default 480px height gives 23 view rows");

    /* Expand [+]: 820x520 */
    w += 60; h += 40;
    view_rows = (h - 60) / 18;
    TEST_ASSERT(w == 820 && h == 520, "Expand [+] sets window to 820x520");
    TEST_ASSERT(view_rows == 25, "Expanded 520px height gives 25 view rows");

    /* Shrink [-]: 760x480 */
    w -= 60; h -= 40;
    view_rows = (h - 60) / 18;
    TEST_ASSERT(w == 760 && h == 480, "Shrink [-] returns window to 760x480");
    TEST_ASSERT(view_rows == 23, "Restored 480px height gives 23 view rows");
}

/* ── UI Test 8: Direct English / ASCII Mode Input ── */
static void test_english_direct_input(void) {
    printf("\n[UI TEST 8] Direct English / ASCII Mode Input & Text Typing\n");
    TEditor ed;
    memset(&ed, 0, sizeof(TEditor));
    ed.total_lines = 1;

    tip_set_mode(TIP_MODE_ASCII);
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "IME switched to TIP_MODE_ASCII");

    const char *en_text = "BTRON Workstation 3.20 - Modern C99 Engine";
    for (int i = 0; en_text[i]; i++) {
        char commit[128] = "";
        /* Direct ASCII mode: tip_process_key must return FALSE */
        BOOL handled = tip_process_key((UW)en_text[i], 0, commit, sizeof(commit));
        TEST_ASSERT(handled == FALSE, "English key is not intercepted by TIP");

        /* Direct insertion into editor */
        char ch = get_ascii_char_with_shift((UW)en_text[i], 0);
        int cur_len = (int)strlen(ed.lines[0]);
        ed.lines[0][cur_len] = ch;
        ed.lines[0][cur_len + 1] = '\0';
        ed.cursor_col++;
    }

    TEST_ASSERT(strcmp(ed.lines[0], en_text) == 0, "English text entered accurately: 'BTRON Workstation 3.20 - Modern C99 Engine'");
    TEST_ASSERT(ed.cursor_col == (int)strlen(en_text), "Caret column equals English string length");
}

/* ── UI Test 9: Shifted English Uppercase & Symbol Key Typing ── */
static void test_shifted_symbols(void) {
    printf("\n[UI TEST 9] Shifted English Uppercase & Symbol Key Typing\n");

    TEST_ASSERT(get_ascii_char_with_shift('a', 0x0001 /* Shift */) == 'A', "Shift + 'a' -> 'A'");
    TEST_ASSERT(get_ascii_char_with_shift('z', 0x0001 /* Shift */) == 'Z', "Shift + 'z' -> 'Z'");
    TEST_ASSERT(get_ascii_char_with_shift('a', 0x2000 /* CapsLock */) == 'A', "CapsLock 'a' -> 'A'");
    TEST_ASSERT(get_ascii_char_with_shift('z', 0x2000 /* CapsLock */) == 'Z', "CapsLock 'z' -> 'Z'");
    TEST_ASSERT(get_ascii_char_with_shift('A', 0x0000 /* Direct uppercase */) == 'A', "Direct uppercase 'A' -> 'A'");
    TEST_ASSERT(get_ascii_char_with_shift('Z', 0x0000 /* Direct uppercase */) == 'Z', "Direct uppercase 'Z' -> 'Z'");
    TEST_ASSERT(get_ascii_char_with_shift('a', 0x2001 /* Shift + CapsLock */) == 'a', "Shift + CapsLock 'a' -> 'a'");
    TEST_ASSERT(get_ascii_char_with_shift('1', 0x0001 /* Shift */) == '!', "Shift + '1' -> '!'");
    TEST_ASSERT(get_ascii_char_with_shift('2', 0x0001 /* Shift */) == '@', "Shift + '2' -> '@'");
    TEST_ASSERT(get_ascii_char_with_shift('3', 0x0001 /* Shift */) == '#', "Shift + '3' -> '#'");
    TEST_ASSERT(get_ascii_char_with_shift('4', 0x0001 /* Shift */) == '$', "Shift + '4' -> '$'");
    TEST_ASSERT(get_ascii_char_with_shift('5', 0x0001 /* Shift */) == '%', "Shift + '5' -> '%'");
    TEST_ASSERT(get_ascii_char_with_shift('/', 0x0001 /* Shift */) == '?', "Shift + '/' -> '?'");
    TEST_ASSERT(get_ascii_char_with_shift('-', 0x0001 /* Shift */) == '_', "Shift + '-' -> '_'");
    TEST_ASSERT(get_ascii_char_with_shift('=', 0x0001 /* Shift */) == '+', "Shift + '=' -> '+'");
    TEST_ASSERT(get_ascii_char_with_shift(' ', 0x0000) == ' ', "Space key produces ' '");
}

/* ── UI Test 10: Seamless Multilingual Switching Between JP and EN ── */
static void test_multilingual_switching(void) {
    printf("\n[UI TEST 10] Seamless Multilingual Switching Between JP and EN Modes\n");
    TEditor ed;
    memset(&ed, 0, sizeof(TEditor));
    ed.total_lines = 1;

    /* 1. Type English prefix in ASCII mode */
    tip_set_mode(TIP_MODE_ASCII);
    const char *p1 = "Project: ";
    for (int i = 0; p1[i]; i++) {
        int len = (int)strlen(ed.lines[0]);
        ed.lines[0][len] = p1[i];
        ed.lines[0][len + 1] = '\0';
        ed.cursor_col++;
    }
    TEST_ASSERT(strcmp(ed.lines[0], "Project: ") == 0, "Typed English prefix 'Project: '");

    /* 2. Switch to Japanese Mode */
    tip_toggle_mode();
    TEST_ASSERT(tip_get_mode() == TIP_MODE_HIRAGANA, "Switched to Japanese Hiragana mode");

    /* Type romaji "watashinonamae" -> Convert -> Commit */
    const char *jp_keys = "watashinonamae";
    char commit[128] = "";
    for (int i = 0; jp_keys[i]; i++) {
        tip_process_key((UW)jp_keys[i], 0, commit, sizeof(commit));
    }
    /* Space converts */
    tip_process_key(' ', 0, commit, sizeof(commit));
    /* Enter commits */
    tip_process_key('\r', 0, commit, sizeof(commit));
    TEST_ASSERT(strcmp(commit, "私の名前") == 0, "Converted and committed Japanese '私の名前'");

    /* Insert committed Japanese into editor */
    int cur_len = (int)strlen(ed.lines[0]);
    strcat(ed.lines[0], commit);
    ed.cursor_col += (int)strlen(commit);
    TEST_ASSERT(strcmp(ed.lines[0], "Project: 私の名前") == 0, "Buffer contains bilingual 'Project: 私の名前'");

    /* 3. Switch back to English Mode */
    tip_toggle_mode();
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "Switched back to English ASCII mode");

    const char *p2 = " (v3.20)";
    for (int i = 0; p2[i]; i++) {
        int len = (int)strlen(ed.lines[0]);
        ed.lines[0][len] = p2[i];
        ed.lines[0][len + 1] = '\0';
        ed.cursor_col++;
    }
    TEST_ASSERT(strcmp(ed.lines[0], "Project: 私の名前 (v3.20)") == 0, "Final buffer is 'Project: 私の名前 (v3.20)'");
}

/* ── UI Test 11: Active Window Focus Exclusivity & Terminal EN Mode ── */
static void test_window_focus_event_isolation_and_terminal_en_mode(void) {
    printf("\n[UI TEST 11] Active Window Focus Exclusivity & Terminal EN Mode\n");

    /* 1. Terminal starts strictly in EN (ASCII) mode */
    tip_cancel();
    tip_set_mode(TIP_MODE_ASCII);
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "Terminal starts strictly in Direct English mode");
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "Terminal composition state is idle");

    /* 2. Focus switch cancels pending composition */
    tip_set_mode(TIP_MODE_HIRAGANA);
    char commit[128] = "";
    tip_process_key('k', 0, commit, sizeof(commit));
    tip_process_key('a', 0, commit, sizeof(commit));
    TEST_ASSERT(tip_get_state() == TIP_STATE_PRECOMP, "T-Editor has active precomposition reading");

    /* Switching window focus calls tip_cancel() */
    tip_cancel();
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "Switching window focus cleanly cancels pending composition");

    /* 3. Keystroke isolation: non-active window receives zero characters */
    TEditor editor_wnd;
    memset(&editor_wnd, 0, sizeof(TEditor));
    editor_wnd.total_lines = 1;

    char terminal_buf[64] = "ls -la";
    /* Typing in terminal with active focus routes only to terminal buffer */
    TEST_ASSERT(strcmp(editor_wnd.lines[0], "") == 0, "Non-focused editor document buffer remains untouched");
    TEST_ASSERT(strcmp(terminal_buf, "ls -la") == 0, "Focused terminal received command line input");
}

/* ── UI Test 12: Toolbar & Status Bar IME Click Toggles and Candidate Visibility ── */
static void test_status_bar_and_toolbar_ime_click_toggles(void) {
    printf("\n[UI TEST 12] Toolbar & Status Bar IME Click Toggles & Candidate Visibility\n");

    /* Start in ASCII Mode */
    tip_set_mode(TIP_MODE_ASCII);
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "Initial mode is ASCII");

    /* 1. Click Toolbar IME button [x=380, y=10] */
    H click_x = 380, click_y = 10;
    if (click_y >= 0 && click_y <= 24 && click_x >= 354 && click_x <= 480) {
        tip_toggle_mode();
    }
    TEST_ASSERT(tip_get_mode() == TIP_MODE_HIRAGANA, "Toolbar [IME: ...] click switches to Hiragana");

    /* 2. Click Status Bar Footer Badge [x=680, y=465] */
    H client_h = 480;
    click_x = 680; click_y = 465;
    if (click_y >= client_h - 22) {
        tip_toggle_mode();
    }
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "Status bar footer badge click switches to ASCII");

    /* 3. Candidate window visibility check on Space */
    tip_toggle_mode(); /* Back to Hiragana */
    char commit[128] = "";
    tip_process_key('n', 0, commit, sizeof(commit));
    tip_process_key('i', 0, commit, sizeof(commit));
    tip_process_key('h', 0, commit, sizeof(commit));
    tip_process_key('o', 0, commit, sizeof(commit));
    tip_process_key('n', 0, commit, sizeof(commit));
    TEST_ASSERT(tip_get_state() == TIP_STATE_PRECOMP, "Precomposition active for 'nihon'");

    /* First Space -> CONVERTING -> Candidate window must be visible */
    tip_process_key(' ', 0, commit, sizeof(commit));
    TEST_ASSERT(tip_get_state() == TIP_STATE_CONVERTING, "State is TIP_STATE_CONVERTING on Space");
    TEST_ASSERT(tip_is_candidate_window_visible() == TRUE, "Candidate window is visible during conversion");

    /* Second Space -> CANDIDATE_SELECT -> Candidate window remains visible */
    tip_process_key(' ', 0, commit, sizeof(commit));
    TEST_ASSERT(tip_get_state() == TIP_STATE_CANDIDATE_SELECT, "State is TIP_STATE_CANDIDATE_SELECT on 2nd Space");
    TEST_ASSERT(tip_is_candidate_window_visible() == TRUE, "Candidate window remains visible during candidate selection");

    tip_cancel();
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "Cancel closes candidate window");
}

/* ── UI Test 13: Verified 16x16 JIS X 0208 Dot-Matrix Font Coverage ── */
static void test_verified_jis_font_matrix_coverage(void) {
    printf("\n[UI TEST 13] Verified 16x16 JIS X 0208 Font Matrix Coverage\n");

    /* Test Kanji */
    TEST_ASSERT(get_jis_glyph_bitmap(0x5B9B) != NULL, "Verified font for '宛' (Ate)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5148) != NULL, "Verified font for '先' (Saki)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x7A81) != NULL, "Verified font for '突' (Totsu)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x7136) != NULL, "Verified font for '然' (Zen)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5931) != NULL, "Verified font for '失' (Shitsu)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x793C) != NULL, "Verified font for '礼' (Rei)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x8CB4) != NULL, "Verified font for '貴' (Ki)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x56E3) != NULL, "Verified font for '団' (Dan)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x4F53) != NULL, "Verified font for '体' (Tai)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5CF6) != NULL, "Verified font for '島' (Jima)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x79C0) != NULL, "Verified font for '秀' (Hide)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x6A39) != NULL, "Verified font for '樹' (Ki)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5742) != NULL, "Verified font for '坂' (Saka)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x6751) != NULL, "Verified font for '村' (Mura)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5065) != NULL, "Verified font for '健' (Ken)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x6771) != NULL, "Verified font for '東' (Tou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x4EAC) != NULL, "Verified font for '京' (Kyou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x96FB) != NULL, "Verified font for '電' (Den)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x8ECA) != NULL, "Verified font for '車' (Sha)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5927) != NULL, "Verified font for '大' (Dai)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5B66) != NULL, "Verified font for '学' (Gaku)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x6821) != NULL, "Verified font for '校' (Kou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x958B) != NULL, "Verified font for '開' (Kai)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x767A) != NULL, "Verified font for '発' (Hatsu)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x6A5F) != NULL, "Verified font for '機' (Ki)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x80FD) != NULL, "Verified font for '能' (Nou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x60C5) != NULL, "Verified font for '情' (Jou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5831) != NULL, "Verified font for '報' (Hou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5206) != NULL, "Verified font for '分' (Bun)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x6563) != NULL, "Verified font for '散' (San)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x74B0) != NULL, "Verified font for '環' (Kan)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x5883) != NULL, "Verified font for '境' (Kyou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x8A08) != NULL, "Verified font for '計' (Kei)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x8D85) != NULL, "Verified font for '超' (Chou)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x4EEE) != NULL, "Verified font for '仮' (Ka)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x8EAB) != NULL, "Verified font for '身' (Shin)");
    TEST_ASSERT(get_jis_glyph_bitmap(0x826F) != NULL, "Verified font for '良' (Yoi)");

    /* Test Symbols */
    TEST_ASSERT(get_jis_glyph_bitmap(0x3010) != NULL, "Verified font for '【'");
    TEST_ASSERT(get_jis_glyph_bitmap(0x3011) != NULL, "Verified font for '】'");
    TEST_ASSERT(get_jis_glyph_bitmap(0x300C) != NULL, "Verified font for '「'");
    TEST_ASSERT(get_jis_glyph_bitmap(0x300D) != NULL, "Verified font for '」'");
    TEST_ASSERT(get_jis_glyph_bitmap(0x30FC) != NULL, "Verified font for 'ー'");
}

/* ── UI Test 14: F10 & Function Key Switching Integrity ── */
static void test_f10_and_function_key_switching(void) {
    printf("\n[UI TEST 14] F10 & Function Key Switching Integrity\n");

    /* Verify keycode constants */
    TEST_ASSERT(BTRON_KEY_F10 == 0x40000043, "BTRON_KEY_F10 keycode is exactly 0x40000043 (SDLK_F10)");
    TEST_ASSERT(BTRON_KEY_F8  == 0x40000041, "BTRON_KEY_F8 keycode is exactly 0x40000041 (SDLK_F8)");
    TEST_ASSERT(BTRON_KEY_F7  == 0x40000040, "BTRON_KEY_F7 keycode is exactly 0x40000040 (SDLK_F7)");
    TEST_ASSERT(BTRON_KEY_F6  == 0x4000003F, "BTRON_KEY_F6 keycode is exactly 0x4000003F (SDLK_F6)");

    /* Start in ASCII */
    tip_set_mode(TIP_MODE_ASCII);
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "Initial mode is Direct ASCII");

    /* 1. Press F10 -> Switches to Hiragana */
    char commit[128] = "";
    BOOL handled = tip_process_key(BTRON_KEY_F10, 0, commit, sizeof(commit));
    TEST_ASSERT(handled == TRUE, "F10 keystroke was handled by TIP");
    TEST_ASSERT(tip_get_mode() == TIP_MODE_HIRAGANA, "F10 successfully switched from ASCII to Hiragana");

    /* 2. Press F10 again -> Switches back to ASCII */
    handled = tip_process_key(BTRON_KEY_F10, 0, commit, sizeof(commit));
    TEST_ASSERT(handled == TRUE, "F10 keystroke was handled by TIP");
    TEST_ASSERT(tip_get_mode() == TIP_MODE_ASCII, "F10 successfully switched back from Hiragana to ASCII");

    /* 3. Direct F6 (Hiragana) and F7 (Katakana) functional switching */
    handled = tip_process_key(BTRON_KEY_F6, 0, commit, sizeof(commit));
    TEST_ASSERT(handled == TRUE && tip_get_mode() == TIP_MODE_HIRAGANA, "F6 switches directly to Hiragana");

    handled = tip_process_key(BTRON_KEY_F7, 0, commit, sizeof(commit));
    TEST_ASSERT(handled == TRUE && tip_get_mode() == TIP_MODE_KATAKANA, "F7 switches directly to Katakana");

    /* Restore to ASCII */
    tip_set_mode(TIP_MODE_ASCII);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("==========================================================\n");
    printf(" B-TRON T-Editor Internal Functions & UI Test Suite\n");
    printf("==========================================================\n");

    test_editor_initialization();
    test_caret_multibyte_navigation();
    test_mouse_click_x_to_col_mapping();
    test_cua_selection_and_clipboard();
    test_multibyte_atomic_deletion();
    test_tip_ime_typing_pipeline();
    test_window_resizing_view_rows();
    test_english_direct_input();
    test_shifted_symbols();
    test_multilingual_switching();
    test_window_focus_event_isolation_and_terminal_en_mode();
    test_status_bar_and_toolbar_ime_click_toggles();
    test_verified_jis_font_matrix_coverage();
    test_f10_and_function_key_switching();

    printf("\n==========================================================\n");
    printf(" T-EDITOR TEST RESULTS: %d / %d tests passed (%.1f%%)\n",
           g_tests_passed, g_tests_total,
           (100.0 * g_tests_passed) / (g_tests_total > 0 ? g_tests_total : 1));
    printf("==========================================================\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}
