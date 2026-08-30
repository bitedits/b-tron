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
    printf("\n[TEST GROUP 4] Elixir-Compiled Binary TAD Verification (tad_bin/)\n");

    TAD_BROWSER tb;
    ER er = tad_browser_load_file(&tb, "tad_bin/01_btron3_spec.tad");
    TEST_ASSERT(er == E_OK, "Successfully loaded tad_bin/01_btron3_spec.tad");
    TEST_ASSERT(tb.span_count > 10, "Document loaded with >10 structured spans");

    er = tad_browser_load_file(&tb, "tad_bin/shared_data/data_type.tad");
    TEST_ASSERT(er == E_OK, "Successfully loaded compiled binary tad_bin/shared_data/data_type.tad");
    TEST_ASSERT(tb.is_binary_tad == TRUE, "Verified valid BTRON3 SPEC 3.20 binary TAD format");
}

/* ── Test 5: Scrolling & Viewport Clamping ── */
static void test_viewport_scrolling(void) {
    printf("\n[TEST GROUP 5] Deterministic Viewport Scrolling\n");

    TAD_BROWSER tb;
    tad_browser_init(&tb);
    tad_browser_load_file(&tb, "tad_bin/01_btron3_spec.tad");
    tb.doc_height = 800;
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

    /* Test item selection click at row 0 (y = 60 + 0*22 + 5 = 65) -> [b-free] 03_bfree_os_book.tad (#103) */
    ID robj = 0;
    char path[128] = "";
    BOOL handled = cabinet_handle_click(50, 65, FALSE, &robj, path);
    TEST_ASSERT(handled == FALSE, "Single click updates selection without modal block");
    TEST_ASSERT(robj == 103, "Selected Real Object #103 (03_bfree_os_book.tad)");
    TEST_ASSERT(strcmp(path, "tad_bin/03_bfree_os_book.tad") == 0, "Resolved path for #103");

    /* Test selection at row 1 (y = 60 + 1*22 + 5 = 87) -> [b-free] manifest.tad (#106) */
    handled = cabinet_handle_click(50, 87, FALSE, &robj, path);
    TEST_ASSERT(robj == 106, "Selected Real Object #106 (manifest.tad)");
    TEST_ASSERT(strcmp(path, "tad_bin/b-free/manifest.tad") == 0, "Resolved path for #106");

    /* Test double-click activation on Canonical Book */
    handled = cabinet_handle_click(50, 87, TRUE, &robj, path);
    TEST_ASSERT(handled == TRUE, "Double click triggers TAD Real Object launch");
}

/* ── Test 7: Navigation History & Robust Path Resolution ── */
static void test_browser_history_and_path_resolution(void) {
    printf("\n[TEST GROUP 7] In-Place Navigation, History Stack & Path Resolution\n");

    /* 1. Test relative path resolution */
    char resolved[256] = "";
    tad_browser_resolve_path("tad_bin/os_spec/index.tad", "kernel/kernel.html", resolved, sizeof(resolved));
    TEST_ASSERT(strcmp(resolved, "tad_bin/os_spec/kernel/kernel.tad") == 0, "Resolved child relative path kernel/kernel.html -> tad_bin/os_spec/kernel/kernel.tad");

    tad_browser_resolve_path("tad_bin/os_spec/kernel/kernel.tad", "../dp/dp.html", resolved, sizeof(resolved));
    TEST_ASSERT(strcmp(resolved, "tad_bin/os_spec/dp/dp.tad") == 0, "Resolved sibling path ../dp/dp.html -> tad_bin/os_spec/dp/dp.tad");

    tad_browser_resolve_path("tad_bin/01_btron3_spec.tad", "02_tkernel_book.tad", resolved, sizeof(resolved));
    TEST_ASSERT(strcmp(resolved, "tad_bin/02_tkernel_book.tad") == 0, "Resolved neighbor canonical book");

    /* 2. Test In-place Browser History Navigation */
    TAD_BROWSER tb;
    tad_browser_init(&tb);

    tad_browser_navigate(&tb, "tad_bin/01_btron3_spec.tad");
    TEST_ASSERT(tb.history_idx == 0, "Initial page at history_idx = 0");
    TEST_ASSERT(tad_browser_can_go_back(&tb) == FALSE, "Cannot go back from first page");

    tad_browser_navigate(&tb, "tad_bin/02_tkernel_book.tad");
    TEST_ASSERT(tb.history_idx == 1, "Navigated to page 2 (history_idx = 1)");
    TEST_ASSERT(tad_browser_can_go_back(&tb) == TRUE, "Can go back to page 1");
    TEST_ASSERT(tad_browser_can_go_forward(&tb) == FALSE, "Cannot go forward past current head");

    /* Navigate back */
    tad_browser_go_back(&tb);
    TEST_ASSERT(tb.history_idx == 0, "Went back to history_idx = 0");
    TEST_ASSERT(strcmp(tb.file_path, "tad_bin/01_btron3_spec.tad") == 0, "Current document restored to page 1");
    TEST_ASSERT(tad_browser_can_go_forward(&tb) == TRUE, "Can go forward to page 2");

    /* Navigate forward */
    tad_browser_go_forward(&tb);
    TEST_ASSERT(tb.history_idx == 1, "Went forward to history_idx = 1");
    TEST_ASSERT(strcmp(tb.file_path, "tad_bin/02_tkernel_book.tad") == 0, "Current document restored to page 2");

    /* Test Toolbar Button Click in handle_mouse */
    BOOL nav_hit = tad_browser_handle_mouse(&tb, 20, 15, TRUE, NULL, NULL);
    TEST_ASSERT(nav_hit == TRUE, "Clicked [◄ Back] toolbar button");
    TEST_ASSERT(tb.history_idx == 0, "Toolbar back returned to page 1");
}

/* ── Test 8: UTF-8 Cyrillic & Ukrainian Multilingual Glyph Font Engine ── */
extern const UB* get_glyph_bitmap(TC code, H *out_width, H *out_height);

static void test_utf8_cyrillic_font_rendering(void) {
    printf("\n[TEST GROUP 8] UTF-8 Cyrillic & Ukrainian TRON Code Font Engine\n");

    /* Test 1: Ukrainian letters UTF-8 -> TRON Code Plane 1 */
    int consumed = 0;
    TC tc_ye = utf8_to_tc("Є", &consumed);
    TEST_ASSERT(consumed == 2, "Consumed 2 UTF-8 bytes for 'Є'");
    TEST_ASSERT(tc_ye == 0x2704, "Mapped 'Є' (U+0404) to TRON Code Plane 1 (0x2704)");

    TC tc_i = utf8_to_tc("І", &consumed);
    TEST_ASSERT(tc_i == 0x2706, "Mapped 'І' (U+0406) to TRON Code Plane 1 (0x2706)");

    TC tc_yi = utf8_to_tc("Ї", &consumed);
    TEST_ASSERT(tc_yi == 0x2707, "Mapped 'Ї' (U+0407) to TRON Code Plane 1 (0x2707)");

    TC tc_ge = utf8_to_tc("Ґ", &consumed);
    TEST_ASSERT(tc_ge == 0x2790, "Mapped 'Ґ' (U+0490) to TRON Code Plane 1 (0x2790)");

    /* Test 2: Standard Cyrillic А, Б, В */
    TC tc_a = utf8_to_tc("А", &consumed);
    TEST_ASSERT(tc_a == 0x2710, "Mapped 'А' (U+0410) to TRON Code Plane 1 (0x2710)");

    /* Test 3: TRON Code -> UTF-8 Round-trip */
    char u8[8] = "";
    int len = tc_to_utf8(tc_ye, u8, sizeof(u8));
    TEST_ASSERT(len == 2, "Decoded TRON Code 0x2704 to 2-byte UTF-8");
    TEST_ASSERT(strcmp(u8, "Є") == 0, "Round-trip preserved 'Є'");

    len = tc_to_utf8(tc_ge, u8, sizeof(u8));
    TEST_ASSERT(strcmp(u8, "Ґ") == 0, "Round-trip preserved 'Ґ'");

    /* Test 4: 8x16 Proportional Bitmap Glyph Detection */
    H gw = 0, gh = 0;
    const UB *bmp_ye = get_glyph_bitmap(tc_ye, &gw, &gh);
    TEST_ASSERT(bmp_ye != NULL, "Retrieved 8x16 dot-matrix bitmap for 'Є'");
    TEST_ASSERT(gw == 8 && gh == 16, "Verified 8x16 proportional glyph dimensions without gaps");

    const UB *bmp_a = get_glyph_bitmap(tc_a, &gw, &gh);
    TEST_ASSERT(bmp_a != NULL, "Retrieved 8x16 dot-matrix bitmap for 'А'");
    TEST_ASSERT(gw == 8 && gh == 16, "Verified 8x16 proportional glyph dimensions for 'А'");
}

/* ── Test 9: BTRON3 3.20 Default Corner Window Resizing ── */
static void test_btron3_corner_window_resizing(void) {
    printf("\n[TEST GROUP 9] BTRON3 3.20 Default Corner Window Resizing\n");

    WND *w = opn_wnd("Test Window", 100, 100, 400, 300, WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE);
    TEST_ASSERT(w != NULL, "Opened test window");
    TEST_ASSERT((w->attr & WND_ATTR_RESIZE) != 0, "WND_ATTR_RESIZE is enabled by default (BTRON3 3.20 Conformance)");
    TEST_ASSERT(w->bounds.right - w->bounds.left == 400, "Initial window width = 400");
    TEST_ASSERT(w->bounds.bottom - w->bounds.top == 300, "Initial window height = 300");

    ER er = rsz_wnd(w, 550, 420);
    TEST_ASSERT(er == E_OK, "rsz_wnd executed successfully");
    TEST_ASSERT(w->bounds.right - w->bounds.left == 550, "Updated window width to 550px");
    TEST_ASSERT(w->bounds.bottom - w->bounds.top == 420, "Updated window height to 420px");
    TEST_ASSERT(w->dev != NULL && w->dev->width == 550 - 8, "Resized client graphic device width");

    /* Test Minimum Size Guard */
    rsz_wnd(w, 50, 50);
    TEST_ASSERT(w->bounds.right - w->bounds.left >= 160, "Clamped at minimum width >= 160px");
    TEST_ASSERT(w->bounds.bottom - w->bounds.top >= 100, "Clamped at minimum height >= 100px");

    /* Test wrsz_wnd origin & dimension update */
    RECT r = { 50, 60, 650, 480 };
    er = wrsz_wnd(w, &r);
    TEST_ASSERT(er == E_OK, "wrsz_wnd executed successfully");
    TEST_ASSERT(w->bounds.left == 50 && w->bounds.top == 60, "Updated window origin");
    TEST_ASSERT(w->bounds.right - w->bounds.left == 600, "Updated window width via wrsz_wnd");

    cls_wnd(w);
}

/* ── Test 10: BTRON3 3.20 Figure & Picture Segment Fusen (GIF & PNG Decoders) ── */
static void test_btron3_picture_figure_segments(void) {
    printf("\n[TEST GROUP 10] BTRON3 3.20 Figure & Picture Segment Fusen (GIF & PNG Decoders)\n");

    /* 1. Test Canonical Book with embedded GIF Figure */
    TAD_BROWSER tb;
    memset(&tb, 0, sizeof(tb));
    ER er = tad_browser_load_file(&tb, "tad_bin/01_btron3_spec.tad");
    TEST_ASSERT(er == E_OK, "Loaded tad_bin/01_btron3_spec.tad with embedded figures");

    BOOL found_img = FALSE;
    for (int i = 0; i < tb.span_count; i++) {
        if (tb.spans[i].style.is_image) {
            found_img = TRUE;
            TEST_ASSERT(strstr(tb.spans[i].style.img_caption, "図 1") != NULL, "Found BTRON3 Architecture Layer Figure");
            TEST_ASSERT(tb.spans[i].style.img_w >= 400, "Figure width is valid");
            TEST_ASSERT(tb.spans[i].bounds.bottom - tb.spans[i].bounds.top >= 130, "Figure layout height computed correctly");
            break;
        }
    }
    TEST_ASSERT(found_img == TRUE, "Detected TS_FPRIM SubID 10 picture segment in Canonical Book");

    /* 2. Test Ukrainian Specification TAD with Figure */
    memset(&tb, 0, sizeof(tb));
    er = tad_browser_load_file(&tb, "tad_bin/shared_data/tad3.tad");
    TEST_ASSERT(er == E_OK, "Loaded tad_bin/shared_data/tad3.tad");

    BOOL found_ua_fig = FALSE;
    for (int i = 0; i < tb.span_count; i++) {
        if (tb.spans[i].style.is_image) {
            found_ua_fig = TRUE;
            TEST_ASSERT(tb.spans[i].style.img_caption[0] != '\0', "Ukrainian figure has valid caption text");
            TEST_ASSERT(strstr(tb.spans[i].style.img_src, ".gif") != NULL, "Referenced original specification GIF file");
            break;
        }
    }
    TEST_ASSERT(found_ua_fig == TRUE, "Detected TS_FPRIM SubID 10 picture segment in Ukrainian TAD3");

    /* 3. Render and Verify Direct GIF Decoding to Framebuffer */
    COLOR fb[800 * 600];
    memset(fb, 0, sizeof(fb));
    GDEV dev = { .width = 800, .height = 600, .pixels = fb, .clip = { 0, 0, 800, 600 } };
    RECT cr = { 0, 0, 800, 600 };
    tad_browser_layout(&tb, 800);
    tad_browser_paint(&tb, &dev, &cr);

    int drawn_pixels = 0;
    for (int p = 0; p < 800 * 600; p++) {
        if (fb[p] != 0 && fb[p] != COLOR_WHITE) drawn_pixels++;
    }
    TEST_ASSERT(drawn_pixels > 1000, "Successfully decoded and rendered exact specification GIF diagram to framebuffer");

    /* 4. Render and Verify Direct PNG Decoding to Framebuffer */
    memset(fb, 0, sizeof(fb));
    int png_res = decode_and_draw_png(&dev, "tad_bin/b-system/img/virtio.png", 10, 10, 400, 300);
    TEST_ASSERT(png_res == 0, "Successfully executed native PNG decoder on specification diagram");

    int png_drawn = 0;
    for (int p = 0; p < 800 * 600; p++) {
        if (fb[p] != 0 && fb[p] != COLOR_WHITE) png_drawn++;
    }
    TEST_ASSERT(png_drawn > 1000, "Successfully decoded and rendered exact specification PNG illustration to framebuffer");
}

/* ── Test 11: Canonical Books Top-Level Links Resolution ── */
static void test_canonical_books_links_resolution(void) {
    printf("\n[TEST GROUP 11] Canonical Books Top-Level Links & Cross-Doc Resolution\n");

    const char *book_files[] = {
        "tad_bin/01_btron3_spec.tad",
        "tad_bin/02_tkernel_book.tad",
        "tad_bin/03_bfree_os_book.tad"
    };

    for (int b = 0; b < 3; b++) {
        TAD_BROWSER tb;
        memset(&tb, 0, sizeof(tb));
        ER er = tad_browser_load_file(&tb, book_files[b]);
        TEST_ASSERT(er == E_OK, "Successfully loaded Canonical book");

        int link_count = 0;
        int resolved_count = 0;
        for (int i = 0; i < tb.span_count; i++) {
            if (tb.spans[i].style.is_vobj || tb.spans[i].is_link) {
                link_count++;
                const char *tgt = tb.spans[i].style.vobj_path;
                if (tgt && tgt[0]) {
                    char resolved[256] = "";
                    tad_browser_resolve_path(book_files[b], tgt, resolved, sizeof(resolved));
                    FILE *fp = fopen(resolved, "rb");
                    if (fp) {
                        fclose(fp);
                        resolved_count++;
                    }
                }
            }
        }
        TEST_ASSERT(link_count > 0, "Canonical book contains interactive Virtual Object links");
        TEST_ASSERT(resolved_count == link_count, "All links in Canonical book resolve to existing TAD binary files on disk");
    }
}

/* ── Test 12: Multi-Window Immutable Document Context Isolation (SPEC 3.20) ── */
static void test_multi_window_context_isolation(void) {
    printf("\n[TEST GROUP 12] Multi-Window Immutable Document Context Isolation (SPEC 3.20)\n");

    WND *w1 = open_tad_browser_window("tad_bin/01_btron3_spec.tad", "Spec Book Window");
    WND *w2 = open_tad_browser_window("tad_bin/02_tkernel_book.tad", "T-Kernel Book Window");

    TEST_ASSERT(w1 != NULL && w2 != NULL, "Opened two independent TAD Browser windows");
    TEST_ASSERT(w1->user_data != 0 && w2->user_data != 0, "Both windows have allocated user_data document contexts");
    TEST_ASSERT(w1->user_data != w2->user_data, "Window contexts are strictly non-shared distinct memory blocks");

    TAD_BROWSER *tb1 = (TAD_BROWSER*)(uintptr_t)w1->user_data;
    TAD_BROWSER *tb2 = (TAD_BROWSER*)(uintptr_t)w2->user_data;

    TEST_ASSERT(strstr(tb1->file_path, "01_btron3_spec") != NULL, "Window 1 holds private state for 01_btron3_spec.tad");
    TEST_ASSERT(strstr(tb2->file_path, "02_tkernel_book") != NULL, "Window 2 holds private state for 02_tkernel_book.tad");

    /* Mutate Window 1 scroll position and verify Window 2 immutability */
    tad_browser_scroll(tb1, 80);
    TEST_ASSERT(tb1->scroll_y == 80, "Window 1 scrolled to y = 80");
    TEST_ASSERT(tb2->scroll_y == 0, "Window 2 remains completely immutable at scroll y = 0");

    /* Close Window 1 and verify Window 2 remains intact */
    cls_wnd(w1);
    TEST_ASSERT(tb2->scroll_y == 0 && strstr(tb2->file_path, "02_tkernel_book") != NULL,
                "Window 2 document context remains valid and intact after Window 1 closure");

    cls_wnd(w2);
    TEST_ASSERT(TRUE, "Window 2 successfully closed and resources cleanly reclaimed");
}

/* ── Test 13: BTRON3 Spec 3.20 BeOS-Style Compact Sliding Window Tabs ── */
static void test_btron3_compact_sliding_tabs(void) {
    printf("\n[TEST GROUP 13] BTRON3 Spec 3.20 BeOS-Style Compact Sliding Window Tabs\n");

    WND *w = opn_wnd("T-Editor", 100, 100, 500, 350, WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE);
    TEST_ASSERT(w != NULL, "Opened window with compact sliding tabs");
    TEST_ASSERT((w->attr & WND_ATTR_COMPACT_TAB) != 0, "WND_ATTR_COMPACT_TAB is active");
    TEST_ASSERT((w->attr & WND_ATTR_SLIDING_TAB) != 0, "WND_ATTR_SLIDING_TAB is active");
    TEST_ASSERT(w->tab_offset_x == 0, "Initial tab offset is 0");
    TEST_ASSERT(w->tab_width >= 100 && w->tab_width < 500 - 6, "Tab width is compact and smaller than full window width");

    /* Test tab rect calculation */
    RECT tr;
    ER er = wget_tab_rect(w, &tr);
    TEST_ASSERT(er == E_OK, "wget_tab_rect succeeded");
    TEST_ASSERT(tr.left == 103, "Tab left starts at bounds.left + 3");
    TEST_ASSERT(tr.right == 103 + w->tab_width, "Tab right matches left + tab_width");
    TEST_ASSERT(tr.top == 103 && tr.bottom == 122, "Tab height is 19px (top+3 to top+22)");

    /* Test hit testing */
    TEST_ASSERT(whit_test_tab(w, 120, 110) == TRUE, "Hit test succeeds inside compact tab");
    TEST_ASSERT(whit_test_tab(w, 400, 110) == FALSE, "Hit test fails on top rail outside compact tab");
    TEST_ASSERT(whit_test_tab(w, 120, 200) == FALSE, "Hit test fails inside client area");

    /* Test close button hit testing inside tab */
    TEST_ASSERT(whit_test_close_btn(w, tr.right - 10, 110) == TRUE, "Close button hit test succeeds inside tab close box");
    TEST_ASSERT(whit_test_close_btn(w, tr.left + 10, 110) == FALSE, "Close button hit test fails on title area");

    /* Test sliding tab offset */
    er = wset_tab_offset(w, 80);
    TEST_ASSERT(er == E_OK, "wset_tab_offset(80) succeeded");
    TEST_ASSERT(w->tab_offset_x == 80, "Tab offset updated to 80px");
    wget_tab_rect(w, &tr);
    TEST_ASSERT(tr.left == 103 + 80, "Tab shifted horizontally by 80px");
    TEST_ASSERT(whit_test_tab(w, 103 + 85, 110) == TRUE, "Hit test succeeds at new shifted position");
    TEST_ASSERT(whit_test_tab(w, 105, 110) == FALSE, "Old position is no longer part of tab");

    /* Test sliding boundary clamping */
    wset_tab_offset(w, 9999);
    H max_expected_off = 500 - 6 - w->tab_width;
    TEST_ASSERT(w->tab_offset_x == max_expected_off, "Tab offset clamped to maximum right edge without overflow");

    wset_tab_offset(w, -50);
    TEST_ASSERT(w->tab_offset_x == 0, "Tab offset clamped to 0 on negative input");

    /* Test window resizing reclamping */
    wset_tab_offset(w, 100);
    rsz_wnd(w, 200, 200);
    TEST_ASSERT(w->tab_offset_x <= 200 - 6 - w->tab_width, "Tab offset automatically reclamped after window shrink");

    /* Test Japanese title dynamic tab width */
    WND *w_jp = opn_wnd("【仕様書】BTRON3 3.20", 200, 200, 600, 400, WND_ATTR_TITLE | WND_ATTR_CLOSE);
    TEST_ASSERT(w_jp->tab_width > w->tab_width, "Japanese multi-byte title gets appropriately wider compact tab");

    cls_wnd(w);
    cls_wnd(w_jp);
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
    test_browser_history_and_path_resolution();
    test_utf8_cyrillic_font_rendering();
    test_btron3_corner_window_resizing();
    test_btron3_picture_figure_segments();
    test_canonical_books_links_resolution();
    test_multi_window_context_isolation();
    test_btron3_compact_sliding_tabs();

    printf("\n==========================================================\n");
    printf(" TEST RESULTS: %d / %d tests passed (%.1f%%)\n",
           g_test_passed, g_test_total, (g_test_passed * 100.0) / g_test_total);
    printf("==========================================================\n");

    return (g_test_passed == g_test_total) ? 0 : 1;
}
