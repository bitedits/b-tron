/*
 * B-TRON HMI Standard Library: hmi_selector.c
 * Selector components (Up/Down Steppers, Radio button matrix, Source selectors).
 */

#include <btron/hmi.h>
#include <string.h>
#include <stdio.h>

HMI_CTRL* hmi_add_updown_selector(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v, int init_v, const char *unit, HMI_CALLBACK cb) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_UPDOWN_SELECTOR;
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
    if (unit) strncpy(c->unit, unit, sizeof(c->unit) - 1);
    return c;
}

HMI_CTRL* hmi_add_radio_selector(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int num_opts, const char **opts, int init_idx, HMI_CALLBACK cb) {
    if (!p || p->num_controls >= HMI_PANEL_MAX_CTRLS) return NULL;

    HMI_CTRL *c = &p->controls[p->num_controls++];
    memset(c, 0, sizeof(HMI_CTRL));
    c->id = id;
    c->type = HMI_TYPE_RADIO_SELECTOR;
    c->bounds.left = x;
    c->bounds.top = y;
    c->bounds.right = x + w;
    c->bounds.bottom = y + h;
    c->flags = HMI_STATE_ACTIVE;
    c->num_options = (num_opts > 16) ? 16 : num_opts;
    c->val = (init_idx >= 0 && init_idx < c->num_options) ? init_idx : 0;
    c->on_change = cb;

    if (label) strncpy(c->label, label, sizeof(c->label) - 1);
    if (opts) {
        for (int i = 0; i < c->num_options; i++) {
            c->options[i] = opts[i];
        }
    }
    return c;
}

void hmi_draw_updown_selector(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl) return;

    /* Background and frame */
    fill_rec(dev, &ctrl->bounds, COLOR_WHITE);
    drw_rec(dev, &ctrl->bounds);

    /* Label on left */
    H tx = ctrl->bounds.left + 4;
    H ty = ctrl->bounds.top + (ctrl->bounds.bottom - ctrl->bounds.top - 14) / 2;
    if (ctrl->label[0]) {
        drw_tc_string(dev, tx, ty, ctrl->label, COLOR_BLACK, 0x00000000);
        tx += (H)strlen(ctrl->label) * 8 + 6;
    }

    /* Current numeric value */
    char v_buf[32];
    if (ctrl->unit[0]) {
        snprintf(v_buf, sizeof(v_buf), "%2d %s", ctrl->val, ctrl->unit);
    } else {
        snprintf(v_buf, sizeof(v_buf), "%2d", ctrl->val);
    }
    drw_tc_string(dev, tx, ty, v_buf, COLOR_NAVY, 0x00000000);

    /* Stepper Buttons [▲] / [▼] on right */
    H btn_box_w = 20;
    H bx = ctrl->bounds.right - btn_box_w;
    H mid_y = (ctrl->bounds.top + ctrl->bounds.bottom) / 2;

    RECT top_btn = { bx, ctrl->bounds.top, ctrl->bounds.right, mid_y };
    RECT bot_btn = { bx, mid_y, ctrl->bounds.right, ctrl->bounds.bottom };

    fill_rec(dev, &top_btn, COLOR_LTGRAY);
    drw_rec(dev, &top_btn);
    drw_tc_string(dev, bx + 4, top_btn.top + 2, "▲", COLOR_BLACK, 0x00000000);

    fill_rec(dev, &bot_btn, COLOR_LTGRAY);
    drw_rec(dev, &bot_btn);
    drw_tc_string(dev, bx + 4, bot_btn.top + 2, "▼", COLOR_BLACK, 0x00000000);
}

void hmi_draw_radio_selector(GDEV *dev, const HMI_CTRL *ctrl) {
    if (!dev || !ctrl || ctrl->num_options <= 0) return;

    H opt_h = (ctrl->bounds.bottom - ctrl->bounds.top) / ctrl->num_options;
    if (opt_h < 18) opt_h = 18;

    for (int i = 0; i < ctrl->num_options; i++) {
        H item_y = ctrl->bounds.top + i * opt_h;
        RECT item_rect = { ctrl->bounds.left, item_y, ctrl->bounds.right, item_y + opt_h - 2 };

        BOOL selected = (ctrl->val == i);
        COLOR bg = selected ? COLOR_DKGRAY : COLOR_LTGRAY;
        COLOR fg = selected ? COLOR_WHITE : COLOR_BLACK;

        fill_rec(dev, &item_rect, bg);
        drw_rec(dev, &item_rect);

        /* Indicator LED square/circle */
        RECT led_r = { item_rect.left + 4, item_rect.top + (opt_h - 10) / 2, item_rect.left + 12, item_rect.top + (opt_h - 10) / 2 + 8 };
        fill_rec(dev, &led_r, selected ? COLOR_GREEN : COLOR_GRAY);
        drw_rec(dev, &led_r);

        /* Option text */
        if (ctrl->options[i]) {
            drw_tc_string(dev, item_rect.left + 18, item_rect.top + (opt_h - 14) / 2, ctrl->options[i], fg, 0x00000000);
        }
    }
}
