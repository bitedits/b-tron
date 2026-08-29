/*
 * B-TRON HMI Standard Library: hmi_switch.c
 * Switches and button components (Momentary, Toggle, Standard Triad).
 */

#include <btron/hmi.h>
#include <string.h>

HMI_CTRL* hmi_add_push_switch(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, HMI_CALLBACK cb) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_PUSH_SWITCH;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE;
    c->trigger = HMI_TRIGGER_RELEASE_EDGE;
    c->on_change = cb;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    return c;
}

HMI_CTRL* hmi_add_toggle_switch(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, BOOL init_state, HMI_CALLBACK cb) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_TOGGLE_SWITCH;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE | (init_state ? HMI_STATE_CHECKED : 0);
    c->val = (init_state ? 1 : 0);
    c->trigger = HMI_TRIGGER_TOUCH_EDGE;
    c->on_change = cb;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    return c;
}

HMI_CTRL* hmi_add_standard_triad(HMI_PANEL *p, ID id, H x, H y, H w, H h, HMI_CALLBACK on_cancel, HMI_CALLBACK on_default, HMI_CALLBACK on_start) {
    if (!p) return NULL;

    H btn_w = (w - 16) / 3;
    if (btn_w < 30) btn_w = 30;

    /* 1. Cancel [取り消し] */
    HMI_CTRL *c_cancel = hmi_add_push_switch(p, id, "取り消し", x, y, btn_w, h, on_cancel);
    /* 2. Default [標準設定] */
    HMI_CTRL *c_default = hmi_add_push_switch(p, id + 1, "標準設定", x + btn_w + 8, y, btn_w, h, on_default);
    /* 3. Start [実行 / 開始] */
    HMI_CTRL *c_start = hmi_add_push_switch(p, id + 2, "実行", x + (btn_w + 8) * 2, y, btn_w, h, on_start);

    return c_start ? c_start : (c_default ? c_default : c_cancel);
}

void hmi_draw_push_switch(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    BOOL pressed = (ctrl->flags & HMI_STATE_PRESSED) != 0;
    COLOR bg = pressed ? COLOR_GRAY : COLOR_LTGRAY;

    /* Bevel 3D button border */
    fill_rec(dev, &ctrl->bounds, bg);
    drw_rec(dev, &ctrl->bounds);

    if (!pressed) {
        /* Light highlight on top-left */
        drw_lin(dev, ctrl->bounds.left + 1, ctrl->bounds.top + 1, ctrl->bounds.right - 2, ctrl->bounds.top + 1);
        drw_lin(dev, ctrl->bounds.left + 1, ctrl->bounds.top + 1, ctrl->bounds.left + 1, ctrl->bounds.bottom - 2);
    }

    /* Text label */
    H text_len = (H)strlen(ctrl->label);
    H tx = ctrl->bounds.left + (ctrl->bounds.right - ctrl->bounds.left - text_len * 8) / 2;
    H ty = ctrl->bounds.top + (ctrl->bounds.bottom - ctrl->bounds.top - 14) / 2;
    if (tx < ctrl->bounds.left + 4) tx = ctrl->bounds.left + 4;
    if (ty < ctrl->bounds.top + 2) ty = ctrl->bounds.top + 2;

    if (pressed) { tx += 1; ty += 1; }
    drw_tc_string(dev, tx, ty, ctrl->label, COLOR_BLACK, 0x00000000);
}

void hmi_draw_toggle_switch(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    BOOL on = (ctrl->flags & HMI_STATE_CHECKED) || (ctrl->val != 0);
    COLOR bg = on ? COLOR_NAVY : COLOR_LTGRAY;
    COLOR fg = on ? COLOR_WHITE : COLOR_BLACK;

    fill_rec(dev, &ctrl->bounds, bg);
    drw_rec(dev, &ctrl->bounds);

    /* Indicator dot on top corner */
    RECT led_r = { ctrl->bounds.right - 10, ctrl->bounds.top + 4, ctrl->bounds.right - 4, ctrl->bounds.top + 10 };
    fill_rec(dev, &led_r, on ? COLOR_GREEN : COLOR_DKGRAY);
    drw_rec(dev, &led_r);

    /* Text label */
    H tx = ctrl->bounds.left + 6;
    H ty = ctrl->bounds.top + (ctrl->bounds.bottom - ctrl->bounds.top - 14) / 2;
    drw_tc_string(dev, tx, ty, ctrl->label, fg, 0x00000000);
}

void hmi_draw_standard_triad(GDEV *dev, const HMI_CTRL *ctrl) {
    hmi_draw_push_switch(dev, ctrl);
}
