/*
 * v_hmi.c — HMI Component API Verification Suite
 *
 * All HMI controls and panel lifecycle APIs are tested here.
 */

#include "../btron_verify.h"
#include <btron/hmi.h>
#include <btron/dp.h>

#define S "HMI"

static int dummy_cb_called = 0;
static void test_hmi_cb(HMI_CTRL *ctrl, HMI_PANEL *panel, void *user_data)
{
    (void)ctrl;
    (void)panel;
    (void)user_data;
    dummy_cb_called++;
}

void vfy_suite_hmi(void)
{
    /* ── Enum Values ────────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "HMI_TYPE_NONE",             HMI_TYPE_NONE,             0);
    VFY_ASSERT_EQ(S, "HMI_TYPE_PUSH_SWITCH",       HMI_TYPE_PUSH_SWITCH,       1);
    VFY_ASSERT_EQ(S, "HMI_TYPE_TOGGLE_SWITCH",     HMI_TYPE_TOGGLE_SWITCH,     2);
    VFY_ASSERT_EQ(S, "HMI_TYPE_STANDARD_TRIAD",    HMI_TYPE_STANDARD_TRIAD,    3);
    VFY_ASSERT_EQ(S, "HMI_TYPE_UPDOWN_SELECTOR",   HMI_TYPE_UPDOWN_SELECTOR,   4);
    VFY_ASSERT_EQ(S, "HMI_TYPE_RADIO_SELECTOR",    HMI_TYPE_RADIO_SELECTOR,    5);
    VFY_ASSERT_EQ(S, "HMI_TYPE_ROTARY_SELECTOR",   HMI_TYPE_ROTARY_SELECTOR,   6);
    VFY_ASSERT_EQ(S, "HMI_TYPE_SLIDER_VOLUME",     HMI_TYPE_SLIDER_VOLUME,     7);
    VFY_ASSERT_EQ(S, "HMI_TYPE_DIAL_VOLUME",       HMI_TYPE_DIAL_VOLUME,       8);
    VFY_ASSERT_EQ(S, "HMI_TYPE_BAR_METER",         HMI_TYPE_BAR_METER,         9);
    VFY_ASSERT_EQ(S, "HMI_TYPE_STATUS_LED",        HMI_TYPE_STATUS_LED,        10);
    VFY_ASSERT_EQ(S, "HMI_TYPE_DIGITAL_DISPLAY",   HMI_TYPE_DIGITAL_DISPLAY,   11);
    VFY_ASSERT_EQ(S, "HMI_TYPE_UNIVERSAL_PAD",     HMI_TYPE_UNIVERSAL_PAD,     12);

    /* ── Trigger Modes ──────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "HMI_TRIGGER_TOUCH_EDGE",   HMI_TRIGGER_TOUCH_EDGE,   0);
    VFY_ASSERT_EQ(S, "HMI_TRIGGER_RELEASE_EDGE", HMI_TRIGGER_RELEASE_EDGE, 1);

    /* ── LED Colors ─────────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "HMI_LED_OFF",    HMI_LED_OFF,    0);
    VFY_ASSERT_EQ(S, "HMI_LED_GREEN",  HMI_LED_GREEN,  1);
    VFY_ASSERT_EQ(S, "HMI_LED_YELLOW", HMI_LED_YELLOW, 2);
    VFY_ASSERT_EQ(S, "HMI_LED_RED",    HMI_LED_RED,    3);

    /* ── Universal Keys ─────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "HMI_KEY_NONE",      HMI_KEY_NONE,      0);
    VFY_ASSERT_EQ(S, "HMI_KEY_PREV_ITEM", HMI_KEY_PREV_ITEM, 1);
    VFY_ASSERT_EQ(S, "HMI_KEY_NEXT_ITEM", HMI_KEY_NEXT_ITEM, 2);
    VFY_ASSERT_EQ(S, "HMI_KEY_DEC_VALUE", HMI_KEY_DEC_VALUE, 3);
    VFY_ASSERT_EQ(S, "HMI_KEY_INC_VALUE", HMI_KEY_INC_VALUE, 4);
    VFY_ASSERT_EQ(S, "HMI_KEY_EXECUTE",   HMI_KEY_EXECUTE,   5);
    VFY_ASSERT_EQ(S, "HMI_KEY_CANCEL",    HMI_KEY_CANCEL,    6);
    VFY_ASSERT_EQ(S, "HMI_KEY_COMMAND",   HMI_KEY_COMMAND,   7);

    /* ── State Bitmasks ─────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "HMI_STATE_ACTIVE",     HMI_STATE_ACTIVE,     (1 << 0));
    VFY_ASSERT_EQ(S, "HMI_STATE_FOCUSED",    HMI_STATE_FOCUSED,    (1 << 1));
    VFY_ASSERT_EQ(S, "HMI_STATE_PRESSED",    HMI_STATE_PRESSED,    (1 << 2));
    VFY_ASSERT_EQ(S, "HMI_STATE_DISABLED",   HMI_STATE_DISABLED,   (1 << 3));
    VFY_ASSERT_EQ(S, "HMI_STATE_CHECKED",    HMI_STATE_CHECKED,    (1 << 4));
    VFY_ASSERT_EQ(S, "HMI_STATE_ENABLEWARE", HMI_STATE_ENABLEWARE, (1 << 5));

    /* ── Struct Sizes ───────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(HMI_CTRL)>0",  sizeof(HMI_CTRL) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(HMI_PANEL)>0", sizeof(HMI_PANEL) > 0);

    /* ── Panel Initialization ───────────────────────────────── */
    HMI_PANEL panel;
    ER er = hmi_init_panel(&panel, "Test Panel", 10, 10, 300, 200, COLOR_LTGRAY);
    VFY_ASSERT_EQ(S, "hmi_init_panel(valid)", er, E_OK);
    VFY_ASSERT_EQ(S, "panel.num_controls", panel.num_controls, 0);

    er = hmi_init_panel(NULL, "Null Panel", 0, 0, 100, 100, COLOR_LTGRAY);
    VFY_ASSERT_EQ(S, "hmi_init_panel(NULL)", er, E_PAR);

    /* ── Control Factories ──────────────────────────────────── */
    HMI_CTRL *btn = hmi_add_push_switch(&panel, 1, "Click Me", 20, 20, 80, 30, test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_push_switch", btn);
    VFY_ASSERT_EQ(S, "panel.num_controls after btn", panel.num_controls, 1);

    HMI_CTRL *tog = hmi_add_toggle_switch(&panel, 2, "Toggle", 120, 20, 80, 30, TRUE, test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_toggle_switch", tog);

    HMI_CTRL *tri = hmi_add_standard_triad(&panel, 3, 20, 60, 200, 30, test_hmi_cb, test_hmi_cb, test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_standard_triad", tri);

    HMI_CTRL *updn = hmi_add_updown_selector(&panel, 4, "Speed", 20, 100, 100, 30, 0, 100, 50, "mph", test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_updown_selector", updn);

    const char *opts[] = { "Low", "Med", "High" };
    HMI_CTRL *rad = hmi_add_radio_selector(&panel, 5, "Power", 20, 140, 120, 30, 3, opts, 1, test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_radio_selector", rad);

    HMI_CTRL *sld = hmi_add_slider_volume(&panel, 6, "Vol", 150, 100, 120, 30, 0, 100, 75, test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_slider_volume", sld);

    HMI_CTRL *dial = hmi_add_dial_volume(&panel, 7, "Tone", 150, 140, 40, 40, 0, 10, 5, test_hmi_cb);
    VFY_ASSERT_NOTNULL(S, "hmi_add_dial_volume", dial);

    HMI_CTRL *meter = hmi_add_bar_meter(&panel, 8, "Signal", 200, 140, 80, 20, 0, 100);
    VFY_ASSERT_NOTNULL(S, "hmi_add_bar_meter", meter);

    HMI_CTRL *led = hmi_add_status_led(&panel, 9, "Status", 200, 20, HMI_LED_GREEN);
    VFY_ASSERT_NOTNULL(S, "hmi_add_status_led", led);

    HMI_CTRL *disp = hmi_add_digital_display(&panel, 10, "Freq", 20, 180, 100, 25, "101.5");
    VFY_ASSERT_NOTNULL(S, "hmi_add_digital_display", disp);

    /* ── Focus Management ───────────────────────────────────── */
    er = hmi_set_focus(&panel, 0);
    VFY_ASSERT_EQ(S, "hmi_set_focus(0)", er, E_OK);
    VFY_ASSERT_EQ(S, "panel.focused_index", panel.focused_index, 0);

    er = hmi_focus_next(&panel);
    VFY_ASSERT_EQ(S, "hmi_focus_next", er, E_OK);
    VFY_ASSERT_EQ(S, "panel.focused_index after next", panel.focused_index, 1);

    er = hmi_focus_prev(&panel);
    VFY_ASSERT_EQ(S, "hmi_focus_prev", er, E_OK);
    VFY_ASSERT_EQ(S, "panel.focused_index after prev", panel.focused_index, 0);

    /* ── Universal Key Navigation ───────────────────────────── */
    er = hmi_handle_universal_key(&panel, HMI_KEY_NEXT_ITEM);
    VFY_ASSERT_EQ(S, "hmi_handle_universal_key(NEXT)", er, E_OK);

    /* ── Panel Drawing ──────────────────────────────────────── */
    GDEV *dev = opn_dev(400, 300);
    if (dev) {
        er = hmi_draw_panel(&panel, dev);
        VFY_ASSERT_EQ(S, "hmi_draw_panel(valid)", er, E_OK);
        cls_dev(dev);
    }
}
