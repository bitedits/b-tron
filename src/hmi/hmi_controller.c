/*
 * B-TRON HMI Standard Library: hmi_controller.c
 * TRON Universal Controller (万能コントローラ) navigation engine and remote UI.
 */

#include <btron/hmi.h>
#include <string.h>

ER hmi_handle_universal_key(HMI_PANEL *panel, HMI_UNIVERSAL_KEY key) {
    if (!panel) return E_PAR;

    switch (key) {
        case HMI_KEY_PREV_ITEM:
            /* [▲/前] Move focus to previous control */
            return hmi_focus_prev(panel);

        case HMI_KEY_NEXT_ITEM:
            /* [▼/次] Move focus to next control */
            return hmi_focus_next(panel);

        case HMI_KEY_DEC_VALUE:
            /* [◄/-] Decrease value of focused control */
            if (panel->focused_index >= 0 && panel->focused_index < panel->num_controls) {
                HMI_CTRL *c = &panel->controls[panel->focused_index];
                if (c->type == HMI_TYPE_UPDOWN_SELECTOR || c->type == HMI_TYPE_SLIDER_VOLUME || c->type == HMI_TYPE_DIAL_VOLUME) {
                    c->val -= (c->step > 0 ? c->step : 1);
                    if (c->val < c->min_val) c->val = c->min_val;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->type == HMI_TYPE_RADIO_SELECTOR) {
                    c->val = (c->val <= 0) ? (c->num_options - 1) : (c->val - 1);
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->type == HMI_TYPE_TOGGLE_SWITCH) {
                    c->val = 0;
                    c->flags &= ~HMI_STATE_CHECKED;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                }
            }
            return E_OK;

        case HMI_KEY_INC_VALUE:
            /* [►/+] Increase value of focused control */
            if (panel->focused_index >= 0 && panel->focused_index < panel->num_controls) {
                HMI_CTRL *c = &panel->controls[panel->focused_index];
                if (c->type == HMI_TYPE_UPDOWN_SELECTOR || c->type == HMI_TYPE_SLIDER_VOLUME || c->type == HMI_TYPE_DIAL_VOLUME) {
                    c->val += (c->step > 0 ? c->step : 1);
                    if (c->val > c->max_val) c->val = c->max_val;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->type == HMI_TYPE_RADIO_SELECTOR) {
                    c->val = (c->val + 1) % c->num_options;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                } else if (c->type == HMI_TYPE_TOGGLE_SWITCH) {
                    c->val = 1;
                    c->flags |= HMI_STATE_CHECKED;
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                }
            }
            return E_OK;

        case HMI_KEY_EXECUTE:
            /* [O] Execute / Trigger focused button */
            if (panel->focused_index >= 0 && panel->focused_index < panel->num_controls) {
                HMI_CTRL *c = &panel->controls[panel->focused_index];
                if (c->type == HMI_TYPE_PUSH_SWITCH || c->type == HMI_TYPE_TOGGLE_SWITCH) {
                    if (c->type == HMI_TYPE_TOGGLE_SWITCH) {
                        c->val = !c->val;
                        if (c->val) c->flags |= HMI_STATE_CHECKED;
                        else c->flags &= ~HMI_STATE_CHECKED;
                    }
                    if (c->on_change) c->on_change(c, panel, c->user_data);
                }
            }
            return E_OK;

        case HMI_KEY_CANCEL:
            /* [X] Reset or Cancel */
            if (panel->focused_index >= 0 && panel->focused_index < panel->num_controls) {
                HMI_CTRL *c = &panel->controls[panel->focused_index];
                c->val = c->min_val;
                if (c->on_change) c->on_change(c, panel, c->user_data);
            }
            return E_OK;

        case HMI_KEY_COMMAND:
            /* [命名] Toggle Remote Overlay / Menu */
            panel->show_remote = !panel->show_remote;
            return E_OK;

        default:
            break;
    }

    return E_OK;
}

void hmi_draw_universal_remote(GDEV *dev, H x, H y, H w, H h, HMI_UNIVERSAL_KEY pressed_key) {
    if (!dev) return;

    /* Remote chassis */
    RECT body = { x, y, x + w, y + h };
    fill_rec(dev, &body, COLOR_DKGRAY);
    drw_rec(dev, &body);

    /* Remote Header */
    drw_tc_string(dev, x + 8, y + 6, "万能コントローラ", COLOR_WHITE, 0x00000000);

    /* 1. D-Pad [▲/前] */
    RECT btn_up = { x + w / 2 - 14, y + 30, x + w / 2 + 14, y + 54 };
    fill_rec(dev, &btn_up, (pressed_key == HMI_KEY_PREV_ITEM) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_up);
    drw_tc_string(dev, btn_up.left + 8, btn_up.top + 4, "▲", COLOR_BLACK, 0x00000000);

    /* 2. D-Pad [▼/次] */
    RECT btn_down = { x + w / 2 - 14, y + 86, x + w / 2 + 14, y + 110 };
    fill_rec(dev, &btn_down, (pressed_key == HMI_KEY_NEXT_ITEM) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_down);
    drw_tc_string(dev, btn_down.left + 8, btn_down.top + 4, "▼", COLOR_BLACK, 0x00000000);

    /* 3. D-Pad [◄/-] */
    RECT btn_left = { x + 10, y + 58, x + 38, y + 82 };
    fill_rec(dev, &btn_left, (pressed_key == HMI_KEY_DEC_VALUE) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_left);
    drw_tc_string(dev, btn_left.left + 8, btn_left.top + 4, "◄", COLOR_BLACK, 0x00000000);

    /* 4. D-Pad [►/+] */
    RECT btn_right = { x + w - 38, y + 58, x + w - 10, y + 82 };
    fill_rec(dev, &btn_right, (pressed_key == HMI_KEY_INC_VALUE) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_right);
    drw_tc_string(dev, btn_right.left + 8, btn_right.top + 4, "►", COLOR_BLACK, 0x00000000);

    /* Center indicator */
    RECT center = { x + w / 2 - 6, y + 64, x + w / 2 + 6, y + 76 };
    fill_rec(dev, &center, COLOR_GRAY);

    /* 5. Cancel [X] */
    RECT btn_x = { x + 16, y + 120, x + 44, y + 144 };
    fill_rec(dev, &btn_x, (pressed_key == HMI_KEY_CANCEL) ? COLOR_WHITE : COLOR_RED);
    drw_rec(dev, &btn_x);
    drw_tc_string(dev, btn_x.left + 8, btn_x.top + 4, "Ｘ", COLOR_WHITE, 0x00000000);

    /* 6. Execute [O] */
    RECT btn_o = { x + w - 44, y + 120, x + w - 16, y + 144 };
    fill_rec(dev, &btn_o, (pressed_key == HMI_KEY_EXECUTE) ? COLOR_WHITE : COLOR_GREEN);
    drw_rec(dev, &btn_o);
    drw_tc_string(dev, btn_o.left + 8, btn_o.top + 4, "〇", COLOR_WHITE, 0x00000000);

    /* 7. Command [命名] */
    RECT btn_cmd = { x + w / 2 - 24, y + 152, x + w / 2 + 24, y + 172 };
    fill_rec(dev, &btn_cmd, (pressed_key == HMI_KEY_COMMAND) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_cmd);
    drw_tc_string(dev, btn_cmd.left + 8, btn_cmd.top + 2, "命名", COLOR_BLACK, 0x00000000);
}

BOOL hmi_remote_hit_test(H rx, H ry, H x, H y, HMI_UNIVERSAL_KEY *out_key) {
    if (!out_key) return FALSE;

    /* Up */
    if (x >= rx + 46 && x < rx + 74 && y >= ry + 30 && y < ry + 54) {
        *out_key = HMI_KEY_PREV_ITEM; return TRUE;
    }
    /* Down */
    if (x >= rx + 46 && x < rx + 74 && y >= ry + 86 && y < ry + 110) {
        *out_key = HMI_KEY_NEXT_ITEM; return TRUE;
    }
    /* Left */
    if (x >= rx + 10 && x < rx + 38 && y >= ry + 58 && y < ry + 82) {
        *out_key = HMI_KEY_DEC_VALUE; return TRUE;
    }
    /* Right */
    if (x >= rx + 82 && x < rx + 110 && y >= ry + 58 && y < ry + 82) {
        *out_key = HMI_KEY_INC_VALUE; return TRUE;
    }
    /* Cancel [X] */
    if (x >= rx + 16 && x < rx + 44 && y >= ry + 120 && y < ry + 144) {
        *out_key = HMI_KEY_CANCEL; return TRUE;
    }
    /* Exec [O] */
    if (x >= rx + 76 && x < rx + 104 && y >= ry + 120 && y < ry + 144) {
        *out_key = HMI_KEY_EXECUTE; return TRUE;
    }
    /* Command */
    if (x >= rx + 36 && x < rx + 84 && y >= ry + 152 && y < ry + 172) {
        *out_key = HMI_KEY_COMMAND; return TRUE;
    }

    return FALSE;
}
