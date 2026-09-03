/*
 * B-TRON Common Application Menu Subsystem
 * Unified in-window menu bar, BeOS fluid hover tracking, and multi-style rendering.
 */

#include <btron/app_menu.h>
#include <btron/troncode.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
extern void* Icalloc(size_t nmemb, size_t sz);
extern void  Ifree(void *ptr);
#define calloc   Icalloc
#define free     Ifree
#define memset   tkl_memset
#define memcpy   tkl_memcpy
#define strlen   tkl_strlen
#define strncpy  tkl_strncpy
#define snprintf tkl_snprintf
#endif

static APP_MENU_STYLE s_global_menu_style = APP_MENU_STYLE_CLASSIC_3D;

void app_menu_set_global_style(APP_MENU_STYLE style) {
    s_global_menu_style = style;
}

APP_MENU_STYLE app_menu_get_global_style(void) {
    return s_global_menu_style;
}

/* ── 3D Beveled Box (Classic Workstation Plate) ─────────────────────────── */
void app_menu_draw_3d_bevel_box(GDEV *dev, const RECT *r) {
    if (!dev || !r) return;
    fill_rec(dev, r, COLOR_LTGRAY);
    drw_rec(dev, r);
    /* 3D highlight: white top and left */
    drw_lin(dev, r->left + 1, r->top + 1, r->right - 2, r->top + 1);
    drw_lin(dev, r->left + 1, r->top + 1, r->left + 1, r->bottom - 2);
    /* 3D shadow: dark gray bottom and right */
    drw_lin(dev, r->left + 1, r->bottom - 2, r->right - 2, r->bottom - 2);
    drw_lin(dev, r->right - 2, r->top + 1, r->right - 2, r->bottom - 2);
}
#define draw_3d_bevel_box app_menu_draw_3d_bevel_box

/* ── 3D Dual-Line Etched Groove (Distance Separator) ────────────────────── */
static void draw_menu_separator_v(GDEV *dev, H x, H y1, H y2) {
    RECT shadow = { x, y1, x + 1, y2 };
    RECT highlight = { x + 1, y1, x + 2, y2 };
    fill_rec(dev, &shadow, COLOR_DKGRAY);
    fill_rec(dev, &highlight, COLOR_WHITE);
}

void app_menu_init(APP_MENU_BAR *bar, APP_MENU_STYLE style) {
    if (!bar) return;
    memset(bar, 0, sizeof(APP_MENU_BAR));
    bar->active_menu = -1;
    bar->hover_menu = -1;
    bar->hover_item = -1;
    bar->active_submenu = -1;
    bar->hover_subitem = -1;
    bar->style = style;
    bar->use_global_style = TRUE;
}

int app_menu_add_header(APP_MENU_BAR *bar, const char *title, H width) {
    if (!bar || bar->header_count >= APP_MENU_MAX_HEADERS) return -1;
    int idx = bar->header_count++;
    APP_MENU_HEADER *hdr = &bar->headers[idx];
    memset(hdr, 0, sizeof(APP_MENU_HEADER));
    strncpy(hdr->title, title ? title : "", sizeof(hdr->title) - 1);

    H left = 4;
    if (idx > 0) {
        left = bar->headers[idx - 1].rect.right + 6;
    }
    hdr->rect.left = left;
    hdr->rect.top = 0;
    hdr->rect.right = left + width;
    hdr->rect.bottom = APP_MENU_BAR_HEIGHT;
    return idx;
}

int app_menu_add_item(APP_MENU_BAR *bar, int header_idx, const char *label, const char *accel, int cmd_id, BOOL enabled) {
    if (!bar || header_idx < 0 || header_idx >= bar->header_count) return -1;
    APP_MENU_HEADER *hdr = &bar->headers[header_idx];
    if (hdr->item_count >= APP_MENU_MAX_ITEMS) return -1;

    int idx = hdr->item_count++;
    APP_MENU_ITEM *it = &hdr->items[idx];
    memset(it, 0, sizeof(APP_MENU_ITEM));
    strncpy(it->label, label ? label : "", sizeof(it->label) - 1);
    if (accel) strncpy(it->accel, accel, sizeof(it->accel) - 1);
    it->cmd_id = cmd_id;
    it->is_separator = FALSE;
    it->has_submenu = FALSE;
    it->submenu_id = -1;
    it->enabled = enabled;
    return idx;
}

int app_menu_add_separator(APP_MENU_BAR *bar, int header_idx) {
    if (!bar || header_idx < 0 || header_idx >= bar->header_count) return -1;
    APP_MENU_HEADER *hdr = &bar->headers[header_idx];
    if (hdr->item_count >= APP_MENU_MAX_ITEMS) return -1;

    int idx = hdr->item_count++;
    APP_MENU_ITEM *it = &hdr->items[idx];
    memset(it, 0, sizeof(APP_MENU_ITEM));
    it->is_separator = TRUE;
    it->cmd_id = 0;
    it->enabled = FALSE;
    return idx;
}

int app_menu_add_submenu_item(APP_MENU_BAR *bar, int header_idx, const char *label, int cmd_id, int submenu_id) {
    if (!bar || header_idx < 0 || header_idx >= bar->header_count) return -1;
    APP_MENU_HEADER *hdr = &bar->headers[header_idx];
    if (hdr->item_count >= APP_MENU_MAX_ITEMS) return -1;

    int idx = hdr->item_count++;
    APP_MENU_ITEM *it = &hdr->items[idx];
    memset(it, 0, sizeof(APP_MENU_ITEM));
    strncpy(it->label, label ? label : "", sizeof(it->label) - 1);
    it->cmd_id = cmd_id;
    it->has_submenu = TRUE;
    it->submenu_id = submenu_id;
    it->enabled = TRUE;
    return idx;
}

void app_menu_set_right_text(APP_MENU_BAR *bar, const char *text) {
    if (!bar) return;
    if (text) {
        strncpy(bar->right_text, text, sizeof(bar->right_text) - 1);
    } else {
        bar->right_text[0] = '\0';
    }
}

void app_menu_close(APP_MENU_BAR *bar) {
    if (!bar) return;
    bar->active_menu = -1;
    bar->hover_item = -1;
    bar->active_submenu = -1;
    bar->hover_subitem = -1;
}

void app_menu_open(APP_MENU_BAR *bar, int header_idx) {
    if (!bar || header_idx < 0 || header_idx >= bar->header_count) return;
    bar->active_menu = header_idx;
    bar->hover_menu = header_idx;
    bar->hover_item = -1;
    bar->active_submenu = -1;
    bar->hover_subitem = -1;
}

BOOL app_menu_handle_mouse_move(APP_MENU_BAR *bar, H rel_x, H rel_y) {
    if (!bar) return FALSE;

    /* When menu is open: track hot headers, dropdown items, and submenus */
    if (bar->active_menu >= 0) {
        /* 1. Hot header tracking (BeOS fluid gliding across headers) */
        if (rel_y >= 0 && rel_y <= APP_MENU_BAR_HEIGHT) {
            for (int h = 0; h < bar->header_count; h++) {
                if (rel_x >= bar->headers[h].rect.left && rel_x <= bar->headers[h].rect.right) {
                    if (bar->active_menu != h) {
                        bar->active_menu = h;
                        bar->hover_menu = h;
                        bar->hover_item = -1;
                        bar->active_submenu = -1;
                        bar->hover_subitem = -1;
                        return TRUE;
                    }
                }
            }
        }

        const APP_MENU_HEADER *hdr = &bar->headers[bar->active_menu];
        H menu_x = hdr->rect.left;
        H menu_y = APP_MENU_BAR_HEIGHT;
        H menu_w = APP_MENU_DROPDOWN_WIDTH;
        H menu_h = hdr->item_count * APP_MENU_ROW_HEIGHT + 6;

        /* 2. Cascading Submenu Hover Tracking */
        if (bar->active_submenu >= 0 && bar->active_submenu < hdr->item_count) {
            H sub_x = menu_x + menu_w - 2;
            H sub_y = menu_y + 3 + (bar->active_submenu * APP_MENU_ROW_HEIGHT);
            H sub_w = APP_MENU_SUBMENU_WIDTH;
            H sub_h = 32 * APP_MENU_ROW_HEIGHT + 6; /* bounding ceiling */

            if (rel_x >= sub_x && rel_x <= sub_x + sub_w && rel_y >= sub_y && rel_y <= sub_y + sub_h) {
                int sub_idx = (rel_y - (sub_y + 3)) / APP_MENU_ROW_HEIGHT;
                if (sub_idx >= 0) {
                    bar->hover_subitem = sub_idx;
                    return TRUE;
                }
            }
        }

        /* 3. Main Dropdown Item Tracking */
        if (rel_x >= menu_x && rel_x <= menu_x + menu_w && rel_y >= menu_y && rel_y <= menu_y + menu_h) {
            int idx = (rel_y - (menu_y + 3)) / APP_MENU_ROW_HEIGHT;
            if (idx >= 0 && idx < hdr->item_count) {
                if (!hdr->items[idx].is_separator && hdr->items[idx].enabled) {
                    bar->hover_item = idx;
                    if (hdr->items[idx].has_submenu) {
                        bar->active_submenu = idx;
                    } else {
                        bar->active_submenu = -1;
                        bar->hover_subitem = -1;
                    }
                    return TRUE;
                } else {
                    bar->hover_item = -1;
                }
            }
        }
        return FALSE;
    }

    /* When menu is closed: subtle feedback for hovered header */
    if (rel_y >= 0 && rel_y <= APP_MENU_BAR_HEIGHT) {
        int prev_hov = bar->hover_menu;
        bar->hover_menu = -1;
        for (int h = 0; h < bar->header_count; h++) {
            if (rel_x >= bar->headers[h].rect.left && rel_x <= bar->headers[h].rect.right) {
                bar->hover_menu = h;
                break;
            }
        }
        return (bar->hover_menu != prev_hov);
    } else {
        if (bar->hover_menu != -1) {
            bar->hover_menu = -1;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL app_menu_handle_mouse_down(APP_MENU_BAR *bar, H rel_x, H rel_y, int *out_cmd, int *out_subitem) {
    if (!bar) return FALSE;
    if (out_cmd) *out_cmd = 0;
    if (out_subitem) *out_subitem = -1;

    /* When menu is open: handle item clicks or submenu clicks */
    if (bar->active_menu >= 0) {
        const APP_MENU_HEADER *hdr = &bar->headers[bar->active_menu];
        H menu_x = hdr->rect.left;
        H menu_y = APP_MENU_BAR_HEIGHT;
        H menu_w = APP_MENU_DROPDOWN_WIDTH;
        H menu_h = hdr->item_count * APP_MENU_ROW_HEIGHT + 6;

        /* Click inside cascading submenu */
        if (bar->active_submenu >= 0 && bar->active_submenu < hdr->item_count) {
            H sub_x = menu_x + menu_w - 2;
            H sub_y = menu_y + 3 + (bar->active_submenu * APP_MENU_ROW_HEIGHT);
            H sub_w = APP_MENU_SUBMENU_WIDTH;
            H sub_h = 32 * APP_MENU_ROW_HEIGHT + 6;

            if (rel_x >= sub_x && rel_x <= sub_x + sub_w && rel_y >= sub_y && rel_y <= sub_y + sub_h) {
                int sub_idx = (rel_y - (sub_y + 3)) / APP_MENU_ROW_HEIGHT;
                if (sub_idx >= 0) {
                    if (out_cmd) *out_cmd = hdr->items[bar->active_submenu].cmd_id;
                    if (out_subitem) *out_subitem = sub_idx;
                    app_menu_close(bar);
                    return TRUE;
                }
            }
        }

        /* Click inside active dropdown menu */
        if (rel_x >= menu_x && rel_x <= menu_x + menu_w && rel_y >= menu_y && rel_y <= menu_y + menu_h) {
            int idx = (rel_y - (menu_y + 3)) / APP_MENU_ROW_HEIGHT;
            if (idx >= 0 && idx < hdr->item_count) {
                if (hdr->items[idx].has_submenu) {
                    bar->active_submenu = idx;
                    return TRUE;
                } else if (hdr->items[idx].enabled && !hdr->items[idx].is_separator) {
                    if (out_cmd) *out_cmd = hdr->items[idx].cmd_id;
                    app_menu_close(bar);
                    return TRUE;
                }
            }
        }

        /* Click on a menu bar header toggles/switches */
        if (rel_y >= 0 && rel_y <= APP_MENU_BAR_HEIGHT) {
            for (int h = 0; h < bar->header_count; h++) {
                if (rel_x >= bar->headers[h].rect.left && rel_x <= bar->headers[h].rect.right) {
                    if (bar->active_menu == h) {
                        app_menu_close(bar);
                    } else {
                        app_menu_open(bar, h);
                    }
                    return TRUE;
                }
            }
        }

        /* Clicked outside menu -> dismiss menu */
        app_menu_close(bar);
        return TRUE;
    }

    /* Menu is closed: check if clicking on menu bar header to open */
    if (rel_y >= 0 && rel_y <= APP_MENU_BAR_HEIGHT) {
        for (int h = 0; h < bar->header_count; h++) {
            if (rel_x >= bar->headers[h].rect.left && rel_x <= bar->headers[h].rect.right) {
                app_menu_open(bar, h);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL app_menu_handle_key(APP_MENU_BAR *bar, UW key, uint16_t mod, int *out_cmd) {
    (void)mod;
    if (!bar) return FALSE;
    if (out_cmd) *out_cmd = 0;

    if (key == BTRON_KEY_ESCAPE || key == 27) {
        if (bar->active_menu >= 0) {
            app_menu_close(bar);
            return TRUE;
        }
    }
    return FALSE;
}

/* ── Menu Bar Painting ─────────────────────────────────────────────────── */
void app_menu_paint_bar(const APP_MENU_BAR *bar, GDEV *dev) {
    if (!bar || !dev) return;

    RECT bar_rect = { 0, 0, dev->width, APP_MENU_BAR_HEIGHT };
    fill_rec(dev, &bar_rect, COLOR_LTGRAY);
    drw_lin(dev, 0, APP_MENU_BAR_HEIGHT, dev->width, APP_MENU_BAR_HEIGHT);

    for (int h = 0; h < bar->header_count; h++) {
        const APP_MENU_HEADER *hdr = &bar->headers[h];
        RECT hr = hdr->rect;
        if (bar->active_menu == h) {
            fill_rec(dev, &hr, COLOR_NAVY);
            drw_tc_string(dev, hr.left + 8, hr.top + 3, hdr->title, COLOR_WHITE, 0x00000000);
        } else if (bar->hover_menu == h) {
            fill_rec(dev, &hr, COLOR_WHITE);
            drw_rec(dev, &hr);
            drw_tc_string(dev, hr.left + 8, hr.top + 3, hdr->title, COLOR_NAVY, 0x00000000);
        } else {
            drw_tc_string(dev, hr.left + 8, hr.top + 3, hdr->title, COLOR_BLACK, 0x00000000);
        }

        /* 3D Etched Distance Groove between headers */
        if (h < bar->header_count - 1) {
            H sep_x = (hdr->rect.right + bar->headers[h + 1].rect.left) / 2;
            draw_menu_separator_v(dev, sep_x, 3, APP_MENU_BAR_HEIGHT - 2);
        }
    }

    if (bar->header_count > 0) {
        draw_menu_separator_v(dev, bar->headers[bar->header_count - 1].rect.right + 3, 3, APP_MENU_BAR_HEIGHT - 2);
    }

    /* Right Margin Status Text */
    if (bar->right_text[0] != '\0') {
        int text_w = tc_calc_string_width(bar->right_text, (int)strlen(bar->right_text));
        int text_x = dev->width - text_w - 14;
        if (bar->header_count == 0 || text_x > bar->headers[bar->header_count - 1].rect.right + 20) {
            draw_menu_separator_v(dev, text_x - 8, 3, APP_MENU_BAR_HEIGHT - 2);
            drw_tc_string(dev, text_x, 3, bar->right_text, COLOR_NAVY, COLOR_LTGRAY);
        }
    }
}

/* ── Dropdown Overlay Painting (Supports Classic 3D & Modern Card) ─────── */
void app_menu_paint_dropdown(const APP_MENU_BAR *bar, GDEV *dev) {
    if (!bar || !dev || bar->active_menu < 0 || bar->active_menu >= bar->header_count) return;

    APP_MENU_STYLE active_style = bar->use_global_style ? s_global_menu_style : bar->style;

    const APP_MENU_HEADER *hdr = &bar->headers[bar->active_menu];
    H menu_x = hdr->rect.left;
    H menu_y = APP_MENU_BAR_HEIGHT;
    H menu_w = APP_MENU_DROPDOWN_WIDTH;
    H menu_h = hdr->item_count * APP_MENU_ROW_HEIGHT + 6;

    RECT menu_box = { menu_x, menu_y, menu_x + menu_w, menu_y + menu_h };

    if (active_style == APP_MENU_STYLE_CLASSIC_3D) {
        /* Style 1: T-Editor Authentic 3D Beveled Box Plate */
        draw_3d_bevel_box(dev, &menu_box);
    } else {
        /* Style 2: Modern Flat Card with Soft Drop Shadow */
        RECT shadow = { menu_x + 3, menu_y + 3, menu_x + menu_w + 3, menu_y + menu_h + 3 };
        fill_rec(dev, &shadow, COLOR_DKGRAY);

        fill_rec(dev, &menu_box, COLOR_WHITE);
        drw_rec(dev, &menu_box);
        drw_lin(dev, menu_box.left + 1, menu_box.top + 1, menu_box.right - 2, menu_box.top + 1);
        drw_lin(dev, menu_box.left + 1, menu_box.top + 1, menu_box.left + 1, menu_box.bottom - 2);
    }

    for (int i = 0; i < hdr->item_count; i++) {
        const APP_MENU_ITEM *it = &hdr->items[i];
        RECT ir = { menu_x + 3, menu_y + 3 + i * APP_MENU_ROW_HEIGHT,
                    menu_x + menu_w - 3, menu_y + 3 + (i + 1) * APP_MENU_ROW_HEIGHT };

        if (it->is_separator) {
            H sep_y = (ir.top + ir.bottom) / 2;
            drw_lin(dev, ir.left + 4, sep_y, ir.right - 4, sep_y);
            continue;
        }

        BOOL is_hov = (bar->hover_item == i && it->enabled);
        if (is_hov) {
            fill_rec(dev, &ir, COLOR_NAVY);
        }

        COLOR txt_col = is_hov ? COLOR_WHITE : (it->enabled ? COLOR_BLACK : COLOR_GRAY);
        COLOR acc_col = is_hov ? COLOR_LTGRAY : (it->enabled ? COLOR_DKGRAY : COLOR_GRAY);

        drw_tc_string(dev, ir.left + 8, ir.top + 3, it->label, txt_col, 0x00000000);

        if (it->accel[0] != '\0') {
            int acc_w = tc_calc_string_width(it->accel, (int)strlen(it->accel));
            drw_tc_string(dev, ir.right - acc_w - 10, ir.top + 3, it->accel, acc_col, 0x00000000);
        }

        if (it->has_submenu) {
            drw_tc_string(dev, ir.right - 18, ir.top + 3, "▶", txt_col, 0x00000000);
        }
    }
}

/* ── Cascading Submenu Painting for Strings ────────────────────────────── */
void app_menu_paint_cascading_strings(const APP_MENU_BAR *bar, GDEV *dev, const char items[][64], int count) {
    if (!bar || !dev || bar->active_menu < 0 || bar->active_submenu < 0 || count <= 0) return;

    APP_MENU_STYLE active_style = bar->use_global_style ? s_global_menu_style : bar->style;

    const APP_MENU_HEADER *hdr = &bar->headers[bar->active_menu];
    H menu_x = hdr->rect.left;
    H menu_y = APP_MENU_BAR_HEIGHT;
    H menu_w = APP_MENU_DROPDOWN_WIDTH;

    H sub_x = menu_x + menu_w - 2;
    H sub_y = menu_y + 3 + (bar->active_submenu * APP_MENU_ROW_HEIGHT);
    H sub_w = APP_MENU_SUBMENU_WIDTH;
    H sub_h = count * APP_MENU_ROW_HEIGHT + 6;

    RECT sub_box = { sub_x, sub_y, sub_x + sub_w, sub_y + sub_h };

    if (active_style == APP_MENU_STYLE_CLASSIC_3D) {
        draw_3d_bevel_box(dev, &sub_box);
    } else {
        RECT shadow = { sub_x + 3, sub_y + 3, sub_x + sub_w + 3, sub_y + sub_h + 3 };
        fill_rec(dev, &shadow, COLOR_DKGRAY);

        fill_rec(dev, &sub_box, COLOR_WHITE);
        drw_rec(dev, &sub_box);
        drw_lin(dev, sub_box.left + 1, sub_box.top + 1, sub_box.right - 2, sub_box.top + 1);
        drw_lin(dev, sub_box.left + 1, sub_box.top + 1, sub_box.left + 1, sub_box.bottom - 2);
    }

    for (int f = 0; f < count; f++) {
        RECT sir = { sub_x + 3, sub_y + 3 + f * APP_MENU_ROW_HEIGHT,
                     sub_x + sub_w - 3, sub_y + 3 + (f + 1) * APP_MENU_ROW_HEIGHT };
        BOOL is_sub_hov = (bar->hover_subitem == f);
        if (is_sub_hov) {
            fill_rec(dev, &sir, COLOR_NAVY);
        }
        COLOR sub_txt_col = is_sub_hov ? COLOR_WHITE : COLOR_BLACK;
        drw_tc_string(dev, sir.left + 8, sir.top + 3, items[f], sub_txt_col, 0x00000000);
    }
}

/* ── Common Nano About Box Dialog Implementation ────────────────────────── */
typedef struct {
    char title_full[128];
    char app_name[64];
    char desc[64];
    char attribution[64];
} AboutDialogData;

static void paint_about_dialog(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    AboutDialogData *data = (AboutDialogData*)(uintptr_t)wnd->user_data;

    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_LTGRAY);
    drw_rec(dev, &r);

    /* Inner card with clean retro bevel */
    RECT card = { 6, 6, dev->width - 6, dev->height - 34 };
    fill_rec(dev, &card, COLOR_WHITE);
    drw_rec(dev, &card);

    if (data) {
        drw_tc_string(dev, 16, 12, data->app_name, COLOR_NAVY, 0x00000000);
        drw_tc_string(dev, 16, 30, data->desc, COLOR_DKGRAY, 0x00000000);
        drw_tc_string(dev, 16, 50, data->attribution, COLOR_BLACK, 0x00000000);
    }

    /* 3D OK Button */
    H btn_w = 64, btn_h = 20;
    H btn_x = (dev->width - btn_w) / 2;
    H btn_y = dev->height - 26;
    RECT ok_btn = { btn_x, btn_y, btn_x + btn_w, btn_y + btn_h };
    fill_rec(dev, &ok_btn, COLOR_LTGRAY);
    drw_rec(dev, &ok_btn);
    drw_lin(dev, ok_btn.left + 1, ok_btn.top + 1, ok_btn.right - 2, ok_btn.top + 1);
    drw_lin(dev, ok_btn.left + 1, ok_btn.top + 1, ok_btn.left + 1, ok_btn.bottom - 2);
    drw_tc_string(dev, btn_x + 22, btn_y + 2, "OK", COLOR_BLACK, 0x00000000);
}

static void handle_about_dialog_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    if (evt->type == EV_BUT_DOWN) {
        H rel_x = evt->pos.x - (wnd->bounds.left + 4);
        H rel_y = evt->pos.y - (wnd->bounds.top + 26);
        H btn_w = 64, btn_h = 20;
        H dev_w = wnd->dev ? wnd->dev->width : (wnd->bounds.right - wnd->bounds.left - 8);
        H dev_h = wnd->dev ? wnd->dev->height : (wnd->bounds.bottom - wnd->bounds.top - 30);
        H btn_x = (dev_w - btn_w) / 2;
        H btn_y = dev_h - 26;
        if (rel_x >= btn_x && rel_x <= btn_x + btn_w && rel_y >= btn_y && rel_y <= btn_y + btn_h) {
            cls_wnd(wnd);
        }
    } else if (evt->type == EV_KEY_DOWN) {
        if (evt->key == BTRON_KEY_ESCAPE || evt->key == 0x0D || evt->key == ' ' || evt->key == 27) {
            cls_wnd(wnd);
        }
    }
}

static void destroy_about_dialog(WND *wnd) {
    if (wnd && wnd->user_data) {
        free((void*)(uintptr_t)wnd->user_data);
        wnd->user_data = (VW)0;
    }
}

WND* app_menu_create_about_dialog(const char *app_name, const char *jp_title,
                                  const char *desc, const char *attribution,
                                  int x, int y) {
    AboutDialogData *data = (AboutDialogData*)calloc(1, sizeof(AboutDialogData));
    if (!data) return NULL;

    snprintf(data->title_full, sizeof(data->title_full), "About %s (%s)", app_name, jp_title ? jp_title : "バージョン情報");
    snprintf(data->app_name, sizeof(data->app_name), "%s 3.20 (%s)", app_name, jp_title ? jp_title : "バージョン情報");
    snprintf(data->desc, sizeof(data->desc), "%s", desc ? desc : "BTRON Application");
    snprintf(data->attribution, sizeof(data->attribution), "%s", attribution ? attribution : "Brought to B-System by 5HT");

    WND *wnd = opn_wnd(data->title_full, x, y, 280, 135,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->user_data = (VW)(uintptr_t)data;
        wnd->paint = paint_about_dialog;
        wnd->event_handler = handle_about_dialog_event;
        wnd->destroy = destroy_about_dialog;
    } else {
        free(data);
    }
    return wnd;
}
