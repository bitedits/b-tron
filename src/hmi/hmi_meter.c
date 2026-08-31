/*
 * B-System (BTRON 3.20) HMI Standard Library: hmi_meter.c
 * Display and meter components (Dual L/R VU meters, 3-color LED, Digital displays).
 */

#include <btron/hmi.h>
#include <string.h>
#include <stdio.h>

HMI_CTRL* hmi_add_bar_meter(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_BAR_METER;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE;
    c->min_val = min_v;
    c->max_val = max_v;
    c->val = min_v;
    c->peak_val = min_v;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    return c;
}

HMI_CTRL* hmi_add_status_led(HMI_PANEL *p, ID id, const char *label, H x, H y, HMI_LED_COLOR init_color) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_STATUS_LED;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + 14;
    c->bounds.bottom = y + 14;
    c->flags = HMI_STATE_ACTIVE;
    c->led_color = init_color;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    return c;
}

HMI_CTRL* hmi_add_digital_display(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, const char *init_text) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_DIGITAL_DISPLAY;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    if (init_text) strncpy(c->unit, init_text, sizeof(c->unit) - 1);
    return c;
}

void hmi_draw_bar_meter(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    /* Bezel / frame */
    fill_rec(dev, &ctrl->bounds, COLOR_BLACK);
    drw_rec(dev, &ctrl->bounds);

    H mw = ctrl->bounds.right - ctrl->bounds.left - 4;
    H mh = ctrl->bounds.bottom - ctrl->bounds.top - 4;
    int num_segs = 12;
    H seg_w = (mw - (num_segs - 1) * 2) / num_segs;
    if (seg_w < 3) seg_w = 3;

    int range = ctrl->max_val - ctrl->min_val;
    if (range <= 0) range = 1;
    int active_segs = ((ctrl->val - ctrl->min_val) * num_segs) / range;
    if (active_segs < 0) active_segs = 0;
    if (active_segs > num_segs) active_segs = num_segs;

    int peak_seg = ((ctrl->peak_val - ctrl->min_val) * num_segs) / range;
    if (peak_seg >= num_segs) peak_seg = num_segs - 1;

    for (int i = 0; i < num_segs; i++) {
        H sx = ctrl->bounds.left + 2 + i * (seg_w + 2);
        RECT seg_r = { sx, ctrl->bounds.top + 2, sx + seg_w, ctrl->bounds.top + 2 + mh };

        COLOR col = COLOR_DKGRAY;
        if (i < active_segs || i == peak_seg) {
            /* 3-Color zone: 0..7 Green, 8..9 Yellow, 10..11 Red */
            if (i < 8) col = COLOR_GREEN;
            else if (i < 10) col = COLOR_YELLOW;
            else col = COLOR_RED;
        }

        fill_rec(dev, &seg_r, col);
    }
}

void hmi_draw_status_led(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    COLOR c = COLOR_DKGRAY;
    switch (ctrl->led_color) {
        case HMI_LED_GREEN: c = COLOR_GREEN; break;
        case HMI_LED_YELLOW: c = COLOR_YELLOW; break;
        case HMI_LED_RED: c = COLOR_RED; break;
        default: c = COLOR_DKGRAY; break;
    }

    fill_rec(dev, &ctrl->bounds, c);
    drw_rec(dev, &ctrl->bounds);

    if (ctrl->label[0]) {
        drw_tc_string(dev, ctrl->bounds.right + 4, ctrl->bounds.top, ctrl->label, COLOR_BLACK, 0x00000000);
    }
}

void hmi_draw_digital_display(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    /* Fluorescent VFD black panel */
    fill_rec(dev, &ctrl->bounds, COLOR_BLACK);
    drw_rec(dev, &ctrl->bounds);

    /* Text / 7-segment readout in amber/cyan */
    H tx = ctrl->bounds.left + 6;
    H ty = ctrl->bounds.top + (ctrl->bounds.bottom - ctrl->bounds.top - 14) / 2;
    drw_tc_string(dev, tx, ty, ctrl->unit, COLOR_CYAN, 0x00000000);
}
