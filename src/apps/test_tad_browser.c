/*
 * B-TRON Unit Test Suite: Native TAD Document Browser & Cabinet Explorer
 * Conforming to BTRON3 SPEC 3.20 & NASA JPL Bounded Scope.
 */

#include <btron/tad_browser.h>
#include <btron/vobj.h>
#include <btron/troncode.h>
#include <btron/dp.h>
#include <btron/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_test_total = 0;
static int g_test_passed = 0;

#define TEST_ASSERT(cond, desc) do { \
    g_test_total++; \
    if (cond) { \
        printf("  [PASS] %s\n", desc); \
        g_test_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", desc, __LINE__); \
    } \
} while(0)

/* ── Test 1: Synthetic Binary TAD Segment Decoding ── */
static void test_binary_tad_stream_parser(void) {
    printf("\n[TEST GROUP 1] Synthetic Binary TAD Segment Decoding (SPEC 3.20)\n");

    /* Construct in-memory Binary TAD packet:
     * Header: Type=1, Length=52
     * Seg 1: TS_TPAGE (0xFFA0, len=8)
     * Seg 2: TS_TFONT (0xFFA2, len=4, font_id=1)
     * Seg 3: TS_TCHAR (0xFFA3, len=9, size=16, weight=700, color=0x003366)
     * Seg 4: TS_VOBJ  (0xFFA8, len=18, target=101, label="BTRON Spec")
     * Text: "Hello BTRON\n"
     */
    unsigned char tad_buf[256];
    int pos = 0;

    /* Record Header */
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x01; /* Record Type 1 */
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 60; /* Size */

    /* TS_TPAGE (0xFFA0) */
    tad_buf[pos++] = 0xFF; tad_buf[pos++] = 0xA0;
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 8;
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; /* subid, attr */
    tad_buf[pos++] = 0x03; tad_buf[pos++] = 0xE8; /* H=1000 */
    tad_buf[pos++] = 0x03; tad_buf[pos++] = 0x20; /* W=800 */
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x28; /* Margin=40 */

    /* TS_TCHAR (0xFFA3) */
    tad_buf[pos++] = 0xFF; tad_buf[pos++] = 0xA3;
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 9;
    tad_buf[pos++] = 0x00;
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 16;   /* 16pt */
    tad_buf[pos++] = 0x02; tad_buf[pos++] = 0xBC; /* 700 bold */
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x33; tad_buf[pos++] = 0x66; /* Color */

    /* TS_VOBJ (0xFFA8) */
    tad_buf[pos++] = 0xFF; tad_buf[pos++] = 0xA8;
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 16;
    tad_buf[pos++] = 0x00;
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 0x00; tad_buf[pos++] = 101; /* Target ROBJ 101 */
    tad_buf[pos++] = 0x00; tad_buf[pos++] = 10;   /* Label Len */
    memcpy(&tad_buf[pos], "BTRON Spec", 10); pos += 10;

    /* Text */
    memcpy(&tad_buf[pos], "Hello BTRON\n", 12); pos += 12;

    TAD_BROWSER tb;
    ER er = tad_browser_load_buffer(&tb, tad_buf, pos, "Synthetic TAD Test");
    TEST_ASSERT(er == E_OK, "tad_browser_load_buffer successfully parsed binary TAD stream");
    TEST_ASSERT(tb.is_binary_tad == TRUE, "Identified format as Binary BTRON3 TAD");
    TEST_ASSERT(tb.span_count >= 2, "Generated layout spans from binary segments");

    /* Verify VOBJ span */
    BOOL found_vobj = FALSE;
    for (int i = 0; i < tb.span_count; i++) {
        if (tb.spans[i].style.is_vobj && tb.spans[i].style.target_robj == 101) {
            found_vobj = TRUE;
            TEST_ASSERT(strcmp(tb.spans[i].style.vobj_label, "BTRON Spec") == 0, "Preserved Virtual Object label 'BTRON Spec'");
            break;
        }
    }
    TEST_ASSERT(found_vobj, "Discovered TS_VOBJ span with Target Real Object #101");
}

/* ── Test 2: Linear Stream Layout Calculation & Bounding Boxes ── */
static void test_linear_stream_layout(void) {
    printf("\n[TEST GROUP 2] Linear Stream Layout Engine & Box Computation\n");

    const char *doc_text =
        "■ 第1章 BTRON 共通データ仕様\n"
        "BTRONアーキテクチャの基本概念とデータ型定義。\n"
        "[仮身] #102 : Data Types Specification (doc/shared_data/data_type.html)\n"
        "────────────────────────────────────────\n"
        "End of Chapter 1\n";

    TAD_BROWSER tb;
    ER er = tad_browser_load_buffer(&tb, doc_text, (UW)strlen(doc_text), "Layout Test");
    TEST_ASSERT(er == E_OK, "Parsed symbolic TAD document");
    TEST_ASSERT(tb.span_count >= 4, "Generated spans for headings, text, links, and HR");

    /* Layout Verification */
    tad_browser_layout(&tb, 640);
    TEST_ASSERT(tb.doc_height > 80, "Computed document total height > 80px");

    /* Verify monotonic vertical bounds */
    BOOL strictly_increasing = TRUE;
    for (int i = 1; i < tb.span_count; i++) {
        if (tb.spans[i].bounds.top < tb.spans[i-1].bounds.top) {
            strictly_increasing = FALSE;
            break;
        }
    }
    TEST_ASSERT(strictly_increasing, "Layout spans have strictly monotonic vertical progression");
}

/* ── Test 3: Interactive Virtual Object Link Dispatch ── */
static void test_vobj_link_interaction(void) {
    printf("\n[TEST GROUP 3] Interactive Virtual Object Link Dispatch & Hover Detection\n");

    const char *doc_text =
        "Top Header\n"
        "[仮身] #201 : T-Kernel Startup Guide (t-kernel/tkernel_startup.html)\n"
        "Footer Note\n";

    TAD_BROWSER tb;
    tad_browser_load_buffer(&tb, doc_text, (UW)strlen(doc_text), "Link Test");
    tad_browser_layout(&tb, 640);

    /* Find VOBJ link span */
    int link_idx = -1;
    for (int i = 0; i < tb.span_count; i++) {
        if (tb.spans[i].is_link) {
            link_idx = i;
            break;
        }
    }
    TEST_ASSERT(link_idx >= 0, "Found interactive VOBJ link in document");

    /* Test Mouse Hover */
    RECT r = tb.spans[link_idx].bounds;
    int test_mx = r.left + 10;
    int test_my = r.top + 5;

    ID clicked_robj = 0;
    char clicked_path[128] = "";
    BOOL hit = tad_browser_handle_mouse(&tb, test_mx, test_my, FALSE, &clicked_robj, clicked_path);
    TEST_ASSERT(hit == FALSE, "Hover move does not trigger action");
    TEST_ASSERT(tb.hovered_link_idx == link_idx, "Hovered link index correctly updated");

    /* Test Mouse Click */
    BOOL clicked = tad_browser_handle_mouse(&tb, test_mx, test_my, TRUE, &clicked_robj, clicked_path);
    TEST_ASSERT(clicked == TRUE, "Mouse click triggers VOBJ dispatch");
    TEST_ASSERT(clicked_robj == 201, "Dispatched Real Object ID #201");
}

/* ── Test 4: Real Document Binary TAD Loading (Elixir Compiler Output) ── */
static void test_elixir_compiled_tad_loading(void) {
    printf("\n[TEST GROUP 4] Elixir-Compiled Binary TAD Verification (dharma/ & tad_bin/)\n");

    TAD_BROWSER tb;
    ER er = tad_browser_load_file(&tb, "dharma/01_btron3_spec.tad");
    TEST_ASSERT(er == E_OK, "Successfully loaded dharma/01_btron3_spec.tad");
    TEST_ASSERT(tb.span_count > 10, "Document loaded with >10 structured spans");

    er = tad_browser_load_file(&tb, "tad_bin/shared_data/data_type.tad");
    TEST_ASSERT(er == E_OK, "Successfully loaded compiled binary tad_bin/shared_data/data_type.tad");
    TEST_ASSERT(tb.is_binary_tad == TRUE, "Verified valid BTRON3 SPEC 3.20 binary TAD format");
}

/* ── Test 5: Scrolling & Viewport Clamping ── */
static void test_viewport_scrolling(void) {
    printf("\n[TEST GROUP 5] Deterministic Viewport Scrolling\n");

    TAD_BROWSER tb;
    tad_browser_load_file(&tb, "dharma/01_btron3_spec.tad");
    tb.page_height = 200;

    tad_browser_scroll(&tb, 50);
    TEST_ASSERT(tb.scroll_y == 50, "Scroll down +50px");

    tad_browser_scroll(&tb, -100);
    TEST_ASSERT(tb.scroll_y == 0, "Clamped at minimum scroll_y = 0");

    tad_browser_scroll(&tb, 100000);
    TEST_ASSERT(tb.scroll_y <= tb.doc_height, "Clamped at maximum scroll bounds");
}

/* ── Test 6: Cabinet Explorer Selection & Real Object Activation ── */
extern BOOL cabinet_handle_click(int mouse_x, int mouse_y, BOOL is_double_click, ID *out_robj_id, char *out_path);
extern WND* open_vobj_manager_window(void);

static void test_cabinet_explorer_selection(void) {
    printf("\n[TEST GROUP 6] Cabinet Explorer Interactive Selection & Dispatch\n");

    WND *wnd = open_vobj_manager_window();
    TEST_ASSERT(wnd != NULL, "Opened Cabinet Explorer window");
    TEST_ASSERT(wnd->event_handler != NULL, "Cabinet Explorer has active event_handler attached");

    /* Test item selection click at row 0 (y = 60 + 0*22 + 5 = 65) */
    ID robj = 0;
    char path[128] = "";
    BOOL handled = cabinet_handle_click(50, 65, FALSE, &robj, path);
    TEST_ASSERT(handled == FALSE, "Single click updates selection without modal block");
    TEST_ASSERT(robj == 101, "Selected Real Object #101 (01_btron3_spec.tad)");
    TEST_ASSERT(strcmp(path, "dharma/01_btron3_spec.tad") == 0, "Resolved path for #101");

    /* Test Ukrainian TAD selection at row 4 (y = 60 + 4*22 + 5 = 153) */
    handled = cabinet_handle_click(50, 153, FALSE, &robj, path);
    TEST_ASSERT(robj == 111, "Selected Ukrainian Real Object #111 (01_data_type.tad)");
    TEST_ASSERT(strcmp(path, "tad_bin/shared_data/data_type.tad") == 0, "Resolved path for Ukrainian TAD #111");

    /* Test double-click activation on Ukrainian TAD */
    handled = cabinet_handle_click(50, 153, TRUE, &robj, path);
    TEST_ASSERT(handled == TRUE, "Double click triggers Ukrainian TAD Real Object launch");
}

int main(void) {
    printf("==========================================================\n");
    printf(" B-TRON Native TAD Document Browser & Cabinet Test Suite\n");
    printf(" Conforming to BTRON3 SPEC 3.20 & NASA JPL Safety Rules\n");
    printf("==========================================================\n");

    test_binary_tad_stream_parser();
    test_linear_stream_layout();
    test_vobj_link_interaction();
    test_elixir_compiled_tad_loading();
    test_viewport_scrolling();
    test_cabinet_explorer_selection();

    printf("\n==========================================================\n");
    printf(" TEST RESULTS: %d / %d tests passed (%.1f%%)\n",
           g_test_passed, g_test_total, (g_test_passed * 100.0) / g_test_total);
    printf("==========================================================\n");

    return (g_test_passed == g_test_total) ? 0 : 1;
}
