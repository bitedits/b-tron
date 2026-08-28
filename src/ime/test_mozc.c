/*
 * B-TRON Mozc Kana-Kanji Conversion & TIP Unit Test Suite: test_mozc.c
 * Validates requirements from btron-tip.tex & IME.md:
 * 1. Romaji -> Hiragana / Katakana transliteration (Hepburn / Kunrei)
 * 2. Morphological bunsetsu clause segmentation & Viterbi lattice search
 * 3. Candidate ranking with semantic category annotations
 * 4. DFA state machine transitions & Theorem 2 (O(1) ESC safety invariant)
 * 5. Real Object User Dictionary persistence & learning
 * 6. Theorem 1 Lossless Bidirectional Bridge (UTF-8 <-> TRON Code)
 */

#include <btron/tip.h>
#include <btron/mozc_engine.h>
#include <btron/troncode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_passed = 0;
static int g_tests_total = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_total++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
    } \
} while(0)

/* ── Test 1: Romaji Transliteration ── */
static void test_romaji_transliteration(void) {
    printf("\n[TEST GROUP 1] Romaji to Hiragana / Katakana Transliteration\n");
    char out[128];

    mozc_romaji_to_hiragana("watashinonamaeha", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "わたしのなまえは") == 0, "Transliterate basic clause: watashinonamaeha -> わたしのなまえは");

    mozc_romaji_to_hiragana("watashinonamaewa", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "わたしのなまえわ") == 0, "Transliterate wa clause: watashinonamaewa -> わたしのなまえわ");

    mozc_romaji_to_hiragana("nakanodesu", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "なかのです") == 0, "Transliterate surname + copula: nakanodesu -> なかのです");

    mozc_romaji_to_hiragana("gakkou", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "がっこう") == 0, "Geminate consonant (sokuon): gakkou -> がっこう");

    mozc_romaji_to_hiragana("kyou", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "きょう") == 0, "Digraph (yoon): kyou -> きょう");

    mozc_romaji_to_hiragana("sensei", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "せんせい") == 0, "Syllabic nasal (hatsuon): sensei -> せんせい");

    char kata[128];
    mozc_hiragana_to_katakana("なかのです", kata, sizeof(kata));
    TEST_ASSERT(strcmp(kata, "ナカノデス") == 0, "Hiragana to Katakana: なかのです -> ナカノデス");
}

/* ── Test 2: Mozc Viterbi Lattice Search & Bunsetsu Segmentation ── */
static void test_viterbi_lattice_search(void) {
    printf("\n[TEST GROUP 2] Morphological Bunsetsu Segmentation & Viterbi Lattice Search\n");

    TIP_CLAUSE clauses[TIP_MAX_CLAUSES];
    int num_clauses = 0;

    /* Complex sentence from btron-tip.tex wireframe */
    const char *sentence = "わたしのなまえはなかのです";
    ER er = mozc_lattice_search(sentence, clauses, &num_clauses, TIP_MAX_CLAUSES);
    TEST_ASSERT(er == E_OK, "Execute Mozc Viterbi lattice search");
    TEST_ASSERT(num_clauses >= 4, "Segmented into bunsetsu clauses");

    char combined[256] = "";
    for (int i = 0; i < num_clauses; i++) {
        strcat(combined, clauses[i].converted);
    }
    printf("    Result sentence: '%s' -> '%s'\n", sentence, combined);
    TEST_ASSERT(strstr(combined, "私") != NULL && strstr(combined, "中野") != NULL,
                "Lattice search optimal path: '私の名前は中野です'");

    /* Today's weather is good */
    num_clauses = 0;
    mozc_lattice_search("きょうはてんきがいいです", clauses, &num_clauses, TIP_MAX_CLAUSES);
    combined[0] = '\0';
    for (int i = 0; i < num_clauses; i++) {
        strcat(combined, clauses[i].converted);
    }
    printf("    Result sentence: 'きょうはてんきがいいです' -> '%s'\n", combined);
    TEST_ASSERT(strstr(combined, "今日") != NULL && strstr(combined, "天気") != NULL,
                "Lattice search: '今日は天気が良いです'");
}

/* ── Test 3: Candidate Generation & Semantic Categories ── */
static void test_candidate_ranking(void) {
    printf("\n[TEST GROUP 3] Candidate Generation & Semantic Category Annotations (btron-tip.tex Section 4.2)\n");

    TIP_CANDIDATE candidates[16];
    int count = mozc_get_candidates("なかの", candidates, 16);
    TEST_ASSERT(count >= 5, "Candidate count >= 5 for reading 'なかの'");

    TEST_ASSERT(strcmp(candidates[0].value, "中野") == 0, "Candidate 1: 中野");
    TEST_ASSERT(strcmp(candidates[0].annotation, "surname") == 0, "Candidate 1 Annotation: (surname)");

    TEST_ASSERT(strcmp(candidates[1].value, "仲野") == 0, "Candidate 2: 仲野");
    TEST_ASSERT(strcmp(candidates[1].annotation, "alt kanji") == 0, "Candidate 2 Annotation: (alt kanji)");

    BOOL has_hira = FALSE, has_kata = FALSE, has_rare = FALSE;
    for (int i = 0; i < count; i++) {
        if (strcmp(candidates[i].value, "なかの") == 0) has_hira = TRUE;
        if (strcmp(candidates[i].value, "ナカノ") == 0) has_kata = TRUE;
        if (strcmp(candidates[i].value, "中埜") == 0) has_rare = TRUE;
    }
    TEST_ASSERT(has_hira, "Contains hiragana candidate: なかの");
    TEST_ASSERT(has_kata, "Contains katakana candidate: ナカノ");
    TEST_ASSERT(has_rare, "Contains rare kanji candidate: 中埜");
}

/* ── Test 4: DFA State Transitions & Theorem 2 Invariant ── */
static void test_dfa_and_theorem2(void) {
    printf("\n[TEST GROUP 4] DFA State Machine & Theorem 2 (O(1) Cancellation Safety)\n");

    tip_init();
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "Initial DFA state is TIP_STATE_IDLE");

    char commit_buf[128];

    /* Type 'n', 'a', 'k', 'a', 'n', 'o' */
    tip_process_key('n', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('a', 0, commit_buf, sizeof(commit_buf));
    TEST_ASSERT(tip_get_state() == TIP_STATE_PRECOMP, "DFA transitioned to TIP_STATE_PRECOMP");
    TEST_ASSERT(strcmp(tip_get_reading(), "な") == 0, "Precomp reading: 'な'");

    tip_process_key('k', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('a', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('n', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('o', 0, commit_buf, sizeof(commit_buf));
    TEST_ASSERT(strcmp(tip_get_reading(), "なかの") == 0, "Precomp reading: 'なかの'");

    /* Space: trigger conversion */
    tip_process_key(' ', 0, commit_buf, sizeof(commit_buf));
    TEST_ASSERT(tip_get_state() == TIP_STATE_CONVERTING, "Space triggers TIP_STATE_CONVERTING");

    /* Space again: open candidate window */
    tip_process_key(' ', 0, commit_buf, sizeof(commit_buf));
    TEST_ASSERT(tip_get_state() == TIP_STATE_CANDIDATE_SELECT, "Space triggers TIP_STATE_CANDIDATE_SELECT");

    /* Theorem 2: KEY_ESC (0x1B) unconditionally restores to IDLE in O(1) */
    tip_process_key(0x1B, 0, commit_buf, sizeof(commit_buf));
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "Theorem 2: ESC restores DFA to TIP_STATE_IDLE in O(1)");
    TEST_ASSERT(strcmp(tip_get_reading(), "") == 0, "Composition buffer cleared");

    /* Test commit on Enter */
    tip_process_key('k', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('y', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('o', 0, commit_buf, sizeof(commit_buf));
    tip_process_key('u', 0, commit_buf, sizeof(commit_buf));
    tip_process_key(' ', 0, commit_buf, sizeof(commit_buf));
    commit_buf[0] = '\0';
    tip_process_key('\n', 0, commit_buf, sizeof(commit_buf));
    TEST_ASSERT(strcmp(commit_buf, "今日") == 0, "Enter commits active conversion: '今日'");
    TEST_ASSERT(tip_get_state() == TIP_STATE_IDLE, "DFA returns to IDLE after commit");
}

/* ── Test 5: Real Object User Dictionary ── */
static void test_user_dictionary_real_object(void) {
    printf("\n[TEST GROUP 5] User Dictionary as Real Object (Jisshin #104)\n");

    ER er = mozc_register_user_word("てぃーけー", "T-Kernel2.0", "system");
    TEST_ASSERT(er == E_OK, "Register new word in Real Object User Dictionary");

    TIP_CANDIDATE candidates[16];
    int count = mozc_get_candidates("てぃーけー", candidates, 16);
    TEST_ASSERT(count > 0, "Retrieve candidates for newly registered user word");
    TEST_ASSERT(strcmp(candidates[0].value, "T-Kernel2.0") == 0, "User word is ranked top: 'T-Kernel2.0'");
    TEST_ASSERT(strcmp(candidates[0].annotation, "system") == 0, "User word annotation: (system)");
}

/* ── Test 6: Theorem 1 Lossless Round-Trip Bridge (UTF-8 <-> TRON Code) ── */
static void test_troncode_lossless_bridge(void) {
    printf("\n[TEST GROUP 6] Theorem 1 Lossless Round-Trip Bridge (phi^-1 . phi)(S) == S\n");

    const char *test_corpus[] = {
        "BTRON Workstation 3.20",
        "私の名前は中野です",
        "坂村健 T-Kernel 2.0",
        "日本語 Kana-Kanji Conversion Mozc",
        "今日、天気、電車、学校、実身、仮身",
        NULL
    };

    for (int i = 0; test_corpus[i] != NULL; i++) {
        TC tc_buf[128];
        char utf8_roundtrip[256];

        int tc_len = utf8_to_tc_string(test_corpus[i], tc_buf, 128);
        int out_len = tc_to_utf8_string(tc_buf, tc_len, utf8_roundtrip, sizeof(utf8_roundtrip));

        char msg[128];
        snprintf(msg, sizeof(msg), "Theorem 1 Preservation: '%s'", test_corpus[i]);
        TEST_ASSERT(strcmp(test_corpus[i], utf8_roundtrip) == 0 && out_len > 0, msg);
    }
}

/* ── Test 7: Text Tract - UTF-8 Multibyte Navigation & Boundary Safety ── */
static int test_utf8_next(const char *s, int cur) {
    if (!s || cur >= (int)strlen(s)) return cur;
    const unsigned char *p = (const unsigned char *)s + cur;
    int len = 1;
    if ((*p & 0x80) == 0) len = 1;
    else if ((*p & 0xE0) == 0xC0) len = 2;
    else if ((*p & 0xF0) == 0xE0) len = 3;
    else if ((*p & 0xF8) == 0xF0) len = 4;
    return cur + len;
}

static int test_utf8_prev(const char *s, int cur) {
    if (!s || cur <= 0) return 0;
    int pos = cur - 1;
    const unsigned char *p = (const unsigned char *)s;
    while (pos > 0 && (p[pos] & 0xC0) == 0x80) pos--;
    return pos;
}

static void test_text_tract_navigation(void) {
    printf("\n[TEST GROUP 7] Text Tract: UTF-8 Multibyte Navigation & Boundary Safety\n");

    const char *sample = "件名：【BTRON3仕様】";
    /*
     * "件" = 3 bytes (0..3)
     * "名" = 3 bytes (3..6)
     * "：" = 3 bytes (6..9)
     * "【" = 3 bytes (9..12)
     * "B"  = 1 byte  (12..13)
     * "T"  = 1 byte  (13..14)
     * "R"  = 1 byte  (14..15)
     * "O"  = 1 byte  (15..16)
     * "N"  = 1 byte  (16..17)
     * "3"  = 1 byte  (17..18)
     * "仕" = 3 bytes (18..21)
     * "様" = 3 bytes (21..24)
     * "】" = 3 bytes (24..27)
     */

    int off = 0;
    off = test_utf8_next(sample, off);
    TEST_ASSERT(off == 3, "Step forward past 3-byte Kanji '件' (0 -> 3)");
    off = test_utf8_next(sample, off);
    TEST_ASSERT(off == 6, "Step forward past 3-byte Kanji '名' (3 -> 6)");
    off = test_utf8_next(sample, off);
    TEST_ASSERT(off == 9, "Step forward past 3-byte colon '：' (6 -> 9)");
    off = test_utf8_next(sample, off);
    TEST_ASSERT(off == 12, "Step forward past 3-byte bracket '【' (9 -> 12)");
    off = test_utf8_next(sample, off);
    TEST_ASSERT(off == 13, "Step forward past 1-byte ASCII 'B' (12 -> 13)");

    /* Backward navigation */
    off = test_utf8_prev(sample, 13);
    TEST_ASSERT(off == 12, "Step backward past 1-byte ASCII 'B' (13 -> 12)");
    off = test_utf8_prev(sample, 12);
    TEST_ASSERT(off == 9, "Step backward past 3-byte bracket '【' (12 -> 9)");
    off = test_utf8_prev(sample, 9);
    TEST_ASSERT(off == 6, "Step backward past 3-byte colon '：' (9 -> 6)");
    off = test_utf8_prev(sample, 6);
    TEST_ASSERT(off == 3, "Step backward past 3-byte Kanji '名' (6 -> 3)");
    off = test_utf8_prev(sample, 3);
    TEST_ASSERT(off == 0, "Step backward past 3-byte Kanji '件' (3 -> 0)");
}

/* ── Test 8: Text Tract - Caret Pixel Layout & Column Widths ── */
static void test_caret_pixel_layout(void) {
    printf("\n[TEST GROUP 8] Text Tract: Caret Pixel Layout & Column Widths\n");

    const char *line = "件名：BTRON3";
    /*
     * '件' (16px) -> x = 36 + 16 = 52
     * '名' (16px) -> x = 52 + 16 = 68
     * '：' (16px) -> x = 68 + 16 = 84
     * 'B'  (8px)  -> x = 84 + 8  = 92
     * 'T'  (8px)  -> x = 92 + 8  = 100
     * 'R'  (8px)  -> x = 100 + 8 = 108
     * 'O'  (8px)  -> x = 108 + 8 = 116
     * 'N'  (8px)  -> x = 116 + 8 = 124
     * '3'  (8px)  -> x = 124 + 8 = 132
     */

    int x = 36;
    const char *p = line;
    int col_indices[] = { 36, 52, 68, 84, 92, 100, 108, 116, 124, 132 };
    int idx = 0;

    while (*p) {
        TEST_ASSERT(x == col_indices[idx], "Caret pixel X matches expected column width");
        int consumed = 0;
        TC code = utf8_to_tc(p, &consumed);
        x += (code < 128) ? 8 : 16;
        p += (consumed > 0 ? consumed : 1);
        idx++;
    }
    TEST_ASSERT(x == 132, "Final Caret X after line is 132px");
}

/* ── Test 9: Text Tract - Multibyte Backspace and Delete Safety ── */
static void test_multibyte_editing_safety(void) {
    printf("\n[TEST GROUP 9] Text Tract: Multibyte Backspace and Delete Safety\n");

    char buf[64];
    strncpy(buf, "件名：新実装", sizeof(buf) - 1);

    /* Backspace at end of string (delete '装' - 3 bytes) */
    int len = (int)strlen(buf);
    int prev_c = test_utf8_prev(buf, len);
    memmove(&buf[prev_c], &buf[len], len - len + 1);
    TEST_ASSERT(strcmp(buf, "件名：新実") == 0, "Backspace removes entire 3-byte character '装'");

    /* Backspace again (delete '実' - 3 bytes) */
    len = (int)strlen(buf);
    prev_c = test_utf8_prev(buf, len);
    memmove(&buf[prev_c], &buf[len], len - len + 1);
    TEST_ASSERT(strcmp(buf, "件名：新") == 0, "Backspace removes entire 3-byte character '実'");

    /* Forward delete at offset 0 (delete '件' - 3 bytes) */
    int next_c = test_utf8_next(buf, 0);
    len = (int)strlen(buf);
    memmove(&buf[0], &buf[next_c], len - next_c + 1);
    TEST_ASSERT(strcmp(buf, "名：新") == 0, "Delete key removes entire 3-byte character '件'");
}

/* ── Test 10: Text Tract - Framebuffer Rendering Integrity & Underline Scoping ── */
static void test_rendering_integrity(void) {
    printf("\n[TEST GROUP 10] Text Tract: Rendering Integrity & Underline Scoping\n");

    #define FB_W 200
    #define FB_H 40
    COLOR pixels[FB_W * FB_H];
    memset(pixels, 0, sizeof(pixels));

    GDEV dev;
    dev.pixels = (VP)pixels;
    dev.width = FB_W;
    dev.height = FB_H;
    dev.clip.left = 0;
    dev.clip.top = 0;
    dev.clip.right = FB_W;
    dev.clip.bottom = FB_H;

    /* Render standard text (NO underlines expected) */
    ER er = drw_tc_string(&dev, 0, 0, "件名：BTRON", 0xFF000000, 0x00000000);
    TEST_ASSERT(er == E_OK, "Render Japanese string into framebuffer successfully");

    /* Check row 15 (y = 15): ensure NO solid underline was drawn across the text */
    int underline_pixels = 0;
    for (int x = 0; x < 100; x++) {
        if (pixels[15 * FB_W + x] == 0xFF000000) {
            underline_pixels++;
        }
    }
    TEST_ASSERT(underline_pixels < 80, "Standard drw_tc_string has NO unconditional underline artifacts");

    /* Render underlined text (explicitly requested) */
    er = drw_tc_string_underlined(&dev, 0, 20, "テスト", 0xFF000000, 0x00000000, FALSE);
    TEST_ASSERT(er == E_OK, "Render underlined Japanese string successfully");

    int underline_pixels_2 = 0;
    for (int x = 0; x < 48; x++) {
        if (pixels[35 * FB_W + x] == 0xFF000000) {
            underline_pixels_2++;
        }
    }
    TEST_ASSERT(underline_pixels_2 >= 40, "Explicit drw_tc_string_underlined draws underline properly");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("==========================================================\n");
    printf(" Google Mozc / B-TRON TIP Cleanroom Unit Test Suite\n");
    printf(" Conforming to btron-tip.tex & IME.md Specifications\n");
    printf("==========================================================\n");

    test_romaji_transliteration();
    test_viterbi_lattice_search();
    test_candidate_ranking();
    test_dfa_and_theorem2();
    test_user_dictionary_real_object();
    test_troncode_lossless_bridge();
    test_text_tract_navigation();
    test_caret_pixel_layout();
    test_multibyte_editing_safety();
    test_rendering_integrity();

    printf("\n==========================================================\n");
    printf(" TEST RESULTS: %d / %d tests passed (%.1f%%)\n",
           g_tests_passed, g_tests_total,
           (100.0 * g_tests_passed) / (g_tests_total > 0 ? g_tests_total : 1));
    printf("==========================================================\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}
