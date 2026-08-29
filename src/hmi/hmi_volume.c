/*
 * B-TRON HMI Standard Library: hmi_volume.c
 * Continuous volume components (Linear Slider and Rotary Dials).
 */

#include <btron/hmi.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

HMI_CTRL* hmi_add_slider_volume(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v, int init_v, HMI_CALLBACK cb) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_SLIDER_VOLUME;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE;
    c->min_val = min_v;
    c->max_val = max_v;
    c->val = init_v;
    c->step = 1;
    c->on_change = cb;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    return c;
}

HMI_CTRL* hmi_add_dial_volume(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v, int init_v, HMI_CALLBACK cb) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_DIAL_VOLUME;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE;
    c->min_val = min_v;
    c->max_val = max_v;
    c->val = init_v;
    c->step = 1;
    c->on_change = cb;
    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    return c;
}

void hmi_draw_slider_volume(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    /* Label above slider (Chapter 11: non-occlusion guideline) */
    H ty = ctrl->bounds.top;
    if (ctrl->label[0]) {
        drw_tc_string(dev, ctrl->bounds.left, ty, ctrl->label, COLOR_BLACK, 0x00000000);
        char v_str[16];
        snprintf(v_str, sizeof(v_str), "%d", ctrl->val);
        drw_tc_string(dev, ctrl->bounds.right - 24, ty, v_str, COLOR_NAVY, 0x00000000);
    }

    /* Slider track */
    H track_y = ctrl->bounds.top + 18;
    H track_left = ctrl->bounds.left + 10;
    H track_right = ctrl->bounds.right - 10;
    H track_w = track_right - track_left;

    RECT track_r = { track_left, track_y, track_right, track_y + 4 };
    fill_rec(dev, &track_r, COLOR_GRAY);
    drw_rec(dev, &track_r);

    /* Tick marks along bottom */
    for (int i = 0; i <= 10; i++) {
        H tick_x = track_left + (i * track_w) / 10;
        H tick_h = (i % 5 == 0) ? 6 : 3;
        drw_lin(dev, tick_x, track_y + 5, tick_x, track_y + 5 + tick_h);
    }

    /* Thumb position */
    int range = (ctrl->max_val - ctrl->min_val);
    if (range <= 0) range = 1;
    int cur_norm = ctrl->val - ctrl->min_val;
    if (cur_norm < 0) cur_norm = 0;
    if (cur_norm > range) cur_norm = range;

    H thumb_x = track_left + (H)(((long)cur_norm * track_w) / range);
    RECT thumb_r = { thumb_x - 5, track_y - 6, thumb_x + 5, track_y + 10 };
    fill_rec(dev, &thumb_r, COLOR_LTGRAY);
    drw_rec(dev, &thumb_r);
    drw_lin(dev, thumb_x, thumb_r.top + 2, thumb_x, thumb_r.bottom - 2);
}

void hmi_draw_dial_volume(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    H cx = (ctrl->bounds.left + ctrl->bounds.right) / 2;
    H cy = ctrl->bounds.top + (ctrl->bounds.bottom - ctrl->bounds.top) / 2;
    H r = (ctrl->bounds.right - ctrl->bounds.left) / 2 - 8;
    if (r < 10) r = 10;

    /* Knob body */
    RECT knob_box = { cx - r, cy - r, cx + r, cy + r };
    fill_rec(dev, &knob_box, COLOR_LTGRAY);
    drw_rec(dev, &knob_box);

    /* Bevel shading */
    drw_lin(dev, cx - r + 1, cy - r + 1, cx + r - 2, cy - r + 1);
    drw_lin(dev, cx - r + 1, cy - r + 1, cx - r + 1, cy + r - 2);

    /* Angle calculation: 210 deg (min) to 330 deg (max) across bottom -> top */
    int range = ctrl->max_val - ctrl->min_val;
    if (range <= 0) range = 1;
    int cur = ctrl->val - ctrl->min_val;
    if (cur < 0) cur = 0;
    if (cur > range) cur = range;

    /* Map to angle: 135 deg to 405 deg (sweep of 270 deg) */
    double angle_deg = 135.0 + ((double)cur / range) * 270.0;
    double rad = angle_deg * 3.14159265 / 180.0;

    H ind_x = cx + (H)((r - 3) * cos(rad));
    H ind_y = cy + (H)((r - 3) * sin(rad));

    /* Indicator dot/line on knob */
    drw_lin(dev, cx, cy, ind_x, ind_y);

    /* Label below dial */
    if (ctrl->label[0]) {
        H tlen = (H)strlen(ctrl->label);
        drw_tc_string(dev, cx - tlen * 4, ctrl->bounds.bottom - 12, ctrl->label, COLOR_BLACK, 0x00000000);
    }
}
