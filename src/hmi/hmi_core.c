/*
 * B-TRON HMI Standard Library: hmi_core.c
 * Core lifecycle, focus management, and event dispatcher.
 */

#include <btron/hmi.h>
#include <string.h>

ER hmi_init_panel(HMI_PANEL *panel, const char *title, H x, H y, H w, H h, COLOR bg_col) {
    if (!panel) return E_PAR;

    memset(panel, 0, sizeof(HMI_PANEL));
    panel->bounds.left = x;
    panel->bounds.top = y;
    panel->bounds.right = x + w;
    panel->bounds.bottom = y + h;
    panel->bg_color = bg_col;
    panel->num_controls = 0;
    panel->focused_index = -1;
    panel->show_remote = FALSE;

    if (title) {
        strncpy(panel->title, title, sizeof(panel->title) - 1);
    }
    return E_OK;
}

ER hmi_set_focus(HMI_PANEL *panel, int ctrl_index) {
    if (!panel) return E_PAR;

    /* Clear existing focus */
    if (panel->focused_index >= 0 && panel->focused_index < panel->num_controls) {
        panel->controls[panel->focused_index].flags &= ~HMI_STATE_FOCUSED;
    }

    if (ctrl_index >= 0 && ctrl_index < panel->num_controls) {
        /* Only focusable controls can receive focus */
        HMI_CTRL *c = &panel->controls[ctrl_index];
        if (!(c->flags & HMI_STATE_DISABLED)) {
            c->flags |= HMI_STATE_FOCUSED;
            panel->focused_index = ctrl_index;
            return E_OK;
        }
    }

    panel->focused_index = -1;
    return E_OK;
}

ER hmi_focus_next(HMI_PANEL *panel) {
    if (!panel || panel->num_controls == 0) return E_PAR;

    int start = (panel->focused_index < 0) ? 0 : (panel->focused_index + 1);
    for (int i = 0; i < panel->num_controls; i++) {
        int idx = (start + i) % panel->num_controls;
        HMI_CTRL *c = &panel->controls[idx];
        if (!(c->flags & HMI_STATE_DISABLED) && c->type != HMI_TYPE_STATUS_LED && c->type != HMI_TYPE_BAR_METER) {
            return hmi_set_focus(panel, idx);
        }
    }
    return E_OK;
}

ER hmi_focus_prev(HMI_PANEL *panel) {
    if (!panel || panel->num_controls == 0) return E_PAR;

    int start = (panel->focused_index <= 0) ? (panel->num_controls - 1) : (panel->focused_index - 1);
    for (int i = 0; i < panel->num_controls; i++) {
        int idx = (start - i + panel->num_controls) % panel->num_controls;
        HMI_CTRL *c = &panel->controls[idx];
        if (!(c->flags & HMI_STATE_DISABLED) && c->type != HMI_TYPE_STATUS_LED && c->type != HMI_TYPE_BAR_METER) {
            return hmi_set_focus(panel, idx);
        }
    }
    return E_OK;
}

static BOOL pt_in_rect(const RECT *r, H x, H y) {
    return (x >= r->left && x < r->right && y >= r->top && y < r->bottom);
}

BOOL hmi_dispatch_event(HMI_PANEL *panel, const EVT *evt) {
    if (!panel || !evt) return FALSE;

    /* 1. Handle Keyboard & Universal Controller Navigation */
    if (evt->type == EV_KEY_DOWN) {
        UW key = evt->key;
        if (key == 0x40000052 || key == 'w' || key == 'k') { /* Up Arrow */
            hmi_handle_universal_key(panel, HMI_KEY_PREV_ITEM);
            return TRUE;
        } else if (key == 0x40000051 || key == 's' || key == 'j' || key == '\t') { /* Down Arrow / Tab */
            hmi_handle_universal_key(panel, HMI_KEY_NEXT_ITEM);
            return TRUE;
        } else if (key == 0x40000050 || key == 'a' || key == 'h' || key == '-') { /* Left Arrow / Minus */
            hmi_handle_universal_key(panel, HMI_KEY_DEC_VALUE);
            return TRUE;
        } else if (key == 0x4000004F || key == 'd' || key == 'l' || key == '+' || key == '=') { /* Right Arrow / Plus */
            hmi_handle_universal_key(panel, HMI_KEY_INC_VALUE);
            return TRUE;
        } else if (key == '\r' || key == ' ' || key == 'o') { /* Enter / Space / O */
            hmi_handle_universal_key(panel, HMI_KEY_EXECUTE);
            return TRUE;
        } else if (key == 27 || key == 'x' || key == 'q') { /* ESC / X / Cancel */
            hmi_handle_universal_key(panel, HMI_KEY_CANCEL);
            return TRUE;
        }
    }

    /* 2. Handle Mouse Button Press / Release */
    if (evt->type == EV_BUT_DOWN) {
        for (int i = 0; i < panel->num_controls; i++) {
            HMI_CTRL *c = &panel->controls[i];
            if (c->flags & HMI_STATE_DISABLED) continue;

            if (pt_in_rect(&c->bounds, evt->pos.x, evt->pos.y)) {
                hmi_set_focus(panel, i);
                c->flags |= HMI_STATE_PRESSED;

                if (c->type == HMI_TYPE_TOGGLE_SWITCH) {
                    c->val = !c->val;
                    if (c->val) c->flags |= HMI_STATE_CHECKED;
                    else c->flags &= ~HMI_STATE_CHECKED;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->type == HMI_TYPE_RADIO_SELECTOR) {
                    /* Calculate which radio option was clicked */
                    H item_h = (c->bounds.bottom - c->bounds.top) / (c->num_options > 0 ? c->num_options : 1);
                    if (item_h <= 0) item_h = 20;
                    int opt_idx = (evt->pos.y - c->bounds.top) / item_h;
                    if (opt_idx >= 0 && opt_idx < c->num_options) {
                        c->val = opt_idx;
                        if (c->on_change) c->on_change(c, panel, c->user_data);
                    }
                } else if (c->type == HMI_TYPE_UPDOWN_SELECTOR) {
                    /* Check if upper half (▲) or lower half (▼) was clicked */
                    H mid_y = (c->bounds.top + c->bounds.bottom) / 2;
                    if (evt->pos.y < mid_y) {
                        c->val += (c->step > 0 ? c->step : 1);
                        if (c->val > c->max_val) c->val = c->max_val;
                    } else {
                        c->val -= (c->step > 0 ? c->step : 1);
                        if (c->val < c->min_val) c->val = c->min_val;
                    }
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->type == HMI_TYPE_SLIDER_VOLUME) {
                    /* Set slider value from click position */
                    H slider_w = c->bounds.right - c->bounds.left - 20;
                    if (slider_w > 0) {
                        H click_offset = evt->pos.x - (c->bounds.left + 10);
                        if (click_offset < 0) click_offset = 0;
                        if (click_offset > slider_w) click_offset = slider_w;
                        c->val = c->min_val + (int)(((long)click_offset * (c->max_val - c->min_val)) / slider_w);
                        if (c->on_change) c->on_change(c, panel, c->user_data);
                    }
                } else if (c->type == HMI_TYPE_DIAL_VOLUME) {
                    /* Rotate dial value */
                    c->val += (c->step > 0 ? c->step : 1);
                    if (c->val > c->max_val) c->val = c->min_val;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->trigger == HMI_TRIGGER_TOUCH_EDGE) {
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                }
                return TRUE;
            }
        }
    } else if (evt->type == EV_BUT_UP) {
        BOOL handled = FALSE;
        for (int i = 0; i < panel->num_controls; i++) {
            HMI_CTRL *c = &panel->controls[i];
            if (c->flags & HMI_STATE_PRESSED) {
                c->flags &= ~HMI_STATE_PRESSED;
                if (c->trigger == HMI_TRIGGER_RELEASE_EDGE && pt_in_rect(&c->bounds, evt->pos.x, evt->pos.y)) {
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                }
                handled = TRUE;
            }
        }
        return handled;
    }

    return FALSE;
}

ER hmi_draw_panel(HMI_PANEL *panel, GDEV *dev) {
    if (!panel || !dev) return E_PAR;

    /* Panel background */
    fill_rec(dev, &panel->bounds, panel->bg_color);
    drw_rec(dev, &panel->bounds);

    /* Render custom decorative panel graphics (e.g. SONY cassette deck chassis) */
    if (panel->on_paint_custom) {
        panel->on_paint_custom(panel, dev);
    }

    /* Draw each control */
    for (int i = 0; i < panel->num_controls; i++) {
        const HMI_CTRL *c = &panel->controls[i];
        switch (c->type) {
            case HMI_TYPE_PUSH_SWITCH:
                hmi_draw_push_switch(dev, c);
                break;
            case HMI_TYPE_TOGGLE_SWITCH:
                hmi_draw_toggle_switch(dev, c);
                break;
            case HMI_TYPE_STANDARD_TRIAD:
                hmi_draw_standard_triad(dev, c);
                break;
            case HMI_TYPE_UPDOWN_SELECTOR:
                hmi_draw_updown_selector(dev, c);
                break;
            case HMI_TYPE_RADIO_SELECTOR:
                hmi_draw_radio_selector(dev, c);
                break;
            case HMI_TYPE_SLIDER_VOLUME:
                hmi_draw_slider_volume(dev, c);
                break;
            case HMI_TYPE_DIAL_VOLUME:
                hmi_draw_dial_volume(dev, c);
                break;
            case HMI_TYPE_BAR_METER:
                hmi_draw_bar_meter(dev, c);
                break;
            case HMI_TYPE_STATUS_LED:
                hmi_draw_status_led(dev, c);
                break;
            case HMI_TYPE_DIGITAL_DISPLAY:
                hmi_draw_digital_display(dev, c);
                break;
            default:
                break;
        }

        /* Highlight Current Item (カレント項目) with TRON focus frame */
        if (c->flags & HMI_STATE_FOCUSED) {
            RECT f_rect = { c->bounds.left - 2, c->bounds.top - 2, c->bounds.right + 2, c->bounds.bottom + 2 };
            drw_rec(dev, &f_rect);
        }
    }

    return E_OK;
}
