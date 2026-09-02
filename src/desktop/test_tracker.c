/*
 * B-System (BTRON 3.20) Unit Test Suite for Tracker Start Button & Task Manager
 * Verifies Haiku-style Start button, root menu dispatch, window tracking,
 * low-latency O(1) hit testing, and NASA JPL invariant safety.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <btron/tracker.h>
#include <btron/wnd.h>

/* Mock / Stub BTRON accessory window entry points for standalone testing */
static int s_vobj_opened = 0;
static int s_tedit_opened = 0;
static int s_term_opened = 0;

WND* open_vobj_manager_window(void) { s_vobj_opened++; return NULL; }
WND* open_t_editor_window(void)     { s_tedit_opened++; return NULL; }
WND* open_gterm_window(void)        { s_term_opened++; return NULL; }
WND* open_audio_player_window(void) { return NULL; }
WND* launch_beos_chat(void)         { return NULL; }

static int s_tests_passed = 0;
static int s_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        s_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (line %d)\n", msg, __LINE__); \
        s_tests_failed++; \
    } \
} while(0)

int main(void) {
    printf("==========================================================\n");
    printf(" Running BTRON Deskbar Tracker Unit Tests...\n");
    printf("==========================================================\n");

    /* [TEST GROUP 1] Lifecycle & Invariant Safety */
    printf("\n[TEST GROUP 1] Lifecycle & Invariant Safety\n");
    ER err = tracker_init();
    TEST_ASSERT(err == E_OK, "tracker_init returns E_OK");
    TEST_ASSERT(tracker_verify_invariants() == TRUE, "Initial state satisfies all NASA JPL invariants");
    TEST_ASSERT(tracker_is_menu_open() == FALSE, "Start menu initially closed");

    const TRACKER *st = tracker_get_state();
    TEST_ASSERT(st->btn_rect.left == 4 && st->btn_rect.top == 3, "Button positioned at top-left (4, 3)");
    TEST_ASSERT(st->btn_rect.right == 84 && st->btn_rect.bottom == 23, "Button dimensions 80x20");
    TEST_ASSERT(st->item_count > 0 && st->item_count <= TRACKER_MAX_ITEMS, "Menu populated with static items");

    /* [TEST GROUP 2] O(1) Hit Testing Accuracy */
    printf("\n[TEST GROUP 2] O(1) Hit Testing Accuracy\n");
    TEST_ASSERT(tracker_hit_button(4, 3) == TRUE, "Top-left edge of START button hits");
    TEST_ASSERT(tracker_hit_button(84, 23) == TRUE, "Bottom-right edge of START button hits");
    TEST_ASSERT(tracker_hit_button(44, 13) == TRUE, "Center of START button hits");
    TEST_ASSERT(tracker_hit_button(3, 3) == FALSE, "Left of button outside bounds");
    TEST_ASSERT(tracker_hit_button(85, 23) == FALSE, "Right of button outside bounds");
    TEST_ASSERT(tracker_hit_button(44, 2) == FALSE, "Above button outside bounds");
    TEST_ASSERT(tracker_hit_button(44, 24) == FALSE, "Below button outside bounds");

    /* [TEST GROUP 3] Start Menu Toggle & State Machine */
    printf("\n[TEST GROUP 3] Start Menu Toggle & State Machine\n");
    /* Click button to open menu */
    BOOL handled = tracker_handle_mouse_down(40, 10);
    TEST_ASSERT(handled == TRUE, "Mouse down on button handled");
    TEST_ASSERT(tracker_is_menu_open() == TRUE, "Menu is now OPEN");
    TEST_ASSERT(tracker_verify_invariants() == TRUE, "Invariants hold in OPEN state");

    /* Click outside menu to close */
    handled = tracker_handle_mouse_down(500, 500);
    TEST_ASSERT(handled == TRUE, "Click outside open menu handled");
    TEST_ASSERT(tracker_is_menu_open() == FALSE, "Menu closed on outside click");
    TEST_ASSERT(tracker_verify_invariants() == TRUE, "Invariants hold after close");

    /* Toggle directly via API */
    tracker_toggle_menu();
    TEST_ASSERT(tracker_is_menu_open() == TRUE, "tracker_toggle_menu opens menu");
    tracker_toggle_menu();
    TEST_ASSERT(tracker_is_menu_open() == FALSE, "tracker_toggle_menu closes menu");

    /* [TEST GROUP 4] Keyboard Navigation */
    printf("\n[TEST GROUP 4] Keyboard Navigation\n");
    /* F1 key toggles menu */
    tracker_handle_key(0x101);
    TEST_ASSERT(tracker_is_menu_open() == TRUE, "F1 key toggles menu OPEN");

    H initial_hover = tracker_get_state()->hover_index;
    TEST_ASSERT(initial_hover == 0, "Initial hover item is index 0");

    /* Down arrow navigation */
    tracker_handle_key('s');
    TEST_ASSERT(tracker_get_state()->hover_index > initial_hover, "Down key advances hover index");

    /* Escape closes menu */
    tracker_handle_key(0x1B);
    TEST_ASSERT(tracker_is_menu_open() == FALSE, "Escape key closes menu");

    /* [TEST GROUP 5] Window / Task Tracking Logic */
    printf("\n[TEST GROUP 5] Window / Task Tracking Logic\n");
    /* Mock window setup */
    WND mock_w1;
    memset(&mock_w1, 0, sizeof(mock_w1));
    mock_w1.id = 101;
    strncpy(mock_w1.title, "Test Terminal", sizeof(mock_w1.title) - 1);
    mock_w1.focused = TRUE;
    mock_w1.next = NULL;

    /* Tracker refresh incorporates active windows */
    tracker_open_menu();
    st = tracker_get_state();
    TEST_ASSERT(st->item_count >= 5, "Menu contains core application items");
    TEST_ASSERT(tracker_verify_invariants() == TRUE, "Window tracking invariants verified");
    tracker_close_menu();

    /* [TEST GROUP 6] Widest Item Calculation & Overflow Prevention */
    printf("\n[TEST GROUP 6] Widest Item Calculation & Overflow Prevention\n");
    H w_ascii = tracker_calc_text_width("Hello");
    TEST_ASSERT(w_ascii == 40, "ASCII string 'Hello' width is 40px (5 chars * 8px)");

    H w_kanji = tracker_calc_text_width("日本語");
    TEST_ASSERT(w_kanji == 48, "Kanji string '日本語' width is 48px (3 chars * 16px)");

    H widest_w = tracker_calc_widest_item_width();
    TEST_ASSERT(widest_w >= TRACKER_MENU_MIN_WIDTH, "Widest item width >= TRACKER_MENU_MIN_WIDTH (190px)");

    tracker_open_menu();
    st = tracker_get_state();
    H current_menu_w = st->menu_rect.right - st->menu_rect.left;
    TEST_ASSERT(current_menu_w == widest_w, "Menu width dynamically matches computed widest item width");

    /* Verify every active menu item fits strictly inside menu width without overflow */
    BOOL all_fit = TRUE;
    for (int i = 0; i < st->item_count; i++) {
        if (st->items[i].type == TRACKER_CMD_SEPARATOR) continue;
        H item_w = tracker_calc_text_width(st->items[i].label);
        if (item_w + 26 > current_menu_w) {
            all_fit = FALSE;
            break;
        }
    }
    TEST_ASSERT(all_fit == TRUE, "All menu items fit completely within menu bounds without overflow");
    tracker_close_menu();

    /* [TEST GROUP 7] Low-Latency Execution Benchmarking */
    printf("\n[TEST GROUP 7] Low-Latency Execution Benchmarking\n");
    clock_t start = clock();
    const int ITERS = 100000;
    for (int i = 0; i < ITERS; i++) {
        tracker_hit_button(40, 10);
        tracker_hit_menu(50, 50);
    }
    clock_t end = clock();
    double elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    printf("  [BENCH] 100,000 hit tests completed in %.2f ms (%.4f us / call)\n", elapsed_ms, (elapsed_ms * 1000.0) / (ITERS * 2));
    TEST_ASSERT(elapsed_ms < 50.0, "O(1) hit testing operates under 50ms for 100k calls");

    printf("\n==========================================================\n");
    printf(" TRACKER TEST RESULTS: %d / %d tests passed (%.1f%%)\n",
           s_tests_passed, s_tests_passed + s_tests_failed,
           (s_tests_passed * 100.0) / (s_tests_passed + s_tests_failed));
    printf("==========================================================\n");

    return (s_tests_failed == 0) ? 0 : 1;
}
