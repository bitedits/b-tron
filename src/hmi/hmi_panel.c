/*
 * B-TRON HMI Standard Library: hmi_panel.c
 * High-level Modeless Panel composition helpers.
 */

#include <btron/hmi.h>
#include <string.h>

/* Helper to clear all values in panel */
void hmi_reset_panel_defaults(HMI_PANEL *panel) {
    if (!panel) return;
    for (int i = 0; i < panel->num_controls; i++) {
        HMI_CTRL *c = &panel->controls[i];
        if (c->type == HMI_TYPE_SLIDER_VOLUME || c->type == HMI_TYPE_DIAL_VOLUME || c->type == HMI_TYPE_UPDOWN_SELECTOR) {
            c->val = (c->min_val + c->max_val) / 2;
        } else if (c->type == HMI_TYPE_TOGGLE_SWITCH) {
            c->val = 0;
            c->flags &= ~HMI_STATE_CHECKED;
        } else if (c->type == HMI_TYPE_RADIO_SELECTOR) {
            c->val = 0;
        }
        if (c->on_change) c->on_change(c, panel, c->user_data);
    }
}
