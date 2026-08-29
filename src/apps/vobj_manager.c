/*
 * BTRON Accessory: Real Object Cabinet & Virtual Object Explorer Window (vobj_manager)
 * Cleanroom implementation of Sakamura BTRON / BTRON3 Architecture & NASA JPL Scope.
 */

#include <btron/wnd.h>
#include <btron/vobj.h>
#include <btron/troncode.h>
#include <btron/dp.h>
#include <btron/error.h>
#include <btron/tad_browser.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#define memset  tkl_memset
#define memcpy  tkl_memcpy
#define strlen  tkl_strlen
#define strncpy tkl_strncpy
#define snprintf snprintf
#endif

#define MAX_CABINET_ITEMS 32

typedef enum {
    CAB_VIEW_LIST = 0,
    CAB_VIEW_GRID = 1
} CAB_VIEW_MODE;

typedef struct {
    ID robj_id;
    VOBJ_TYPE type;
    char name[64];
    char path[128];
    UW size_bytes;
    const char *icon_tag;
} CABINET_ITEM;

typedef struct {
    CABINET_ITEM items[MAX_CABINET_ITEMS];
    int item_count;
    int selected_idx;
    int hovered_idx;
    CAB_VIEW_MODE view_mode;
    char status_msg[128];
} CABINET_EXPLORER;

static CABINET_EXPLORER g_cabinet;

static void cabinet_init_defaults(CABINET_EXPLORER *cab) {
    memset(cab, 0, sizeof(CABINET_EXPLORER));
    cab->selected_idx = 0;
    cab->hovered_idx = -1;
    cab->view_mode = CAB_VIEW_LIST;

    /* Register Dharma Books as Real Objects in the Cabinet */
    cab->items[0] = (CABINET_ITEM){ 101, VOBJ_TYPE_TEXT, "01_btron3_spec.tad", "dharma/01_btron3_spec.tad", 10647, "[TAD]" };
    cab->items[1] = (CABINET_ITEM){ 102, VOBJ_TYPE_TEXT, "02_tkernel_book.tad", "dharma/02_tkernel_book.tad", 5821, "[TAD]" };
    cab->items[2] = (CABINET_ITEM){ 103, VOBJ_TYPE_TEXT, "03_tron_hmi_book.tad", "dharma/03_tron_hmi_book.tad", 6695, "[TAD]" };
    cab->items[3] = (CABINET_ITEM){ 104, VOBJ_TYPE_TEXT, "04_bfree_os_book.tad", "dharma/04_bfree_os_book.tad", 7027, "[TAD]" };
    cab->items[4] = (CABINET_ITEM){ 105, VOBJ_TYPE_TEXT, "shared_data_spec.tad", "tad_bin/shared_data/index.tad", 4838, "[DOC]" };
    cab->items[5] = (CABINET_ITEM){ 106, VOBJ_TYPE_TEXT, "os_spec_kernel.tad", "tad_bin/os_spec/kernel/kernel.tad", 6501, "[SYS]" };
    cab->items[6] = (CABINET_ITEM){ 107, VOBJ_TYPE_TERMINAL, "Terminal_Shell.x", "/bin/gterm", 4096, "[APP]" };
    cab->items[7] = (CABINET_ITEM){ 108, VOBJ_TYPE_FOLDER, "System_Preferences", "prefs/", 1024, "[DIR]" };
    cab->item_count = 8;

    strncpy(cab->status_msg, "Cabinet Ready. Double-click any Real Object to open.", sizeof(cab->status_msg) - 1);
}

static void paint_vobj_manager(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);

    /* Header Banner */
    RECT h_bar = { 0, 0, dev->width, 28 };
    fill_rec(dev, &h_bar, COLOR_NAVY);
    drw_tc_string(dev, 10, 6, "REAL OBJECT CABINET / HYPER-DATA STORE (実身管理)", COLOR_WHITE, 0x00000000);

    /* Toolbar */
    RECT tb_bar = { 0, 28, dev->width, 52 };
    fill_rec(dev, &tb_bar, COLOR_LTGRAY);
    drw_lin(dev, 0, 52, dev->width, 52);

    drw_tc_string(dev, 10, 33, "[開く (Open)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 110, 33, "[閲覧 (View)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 210, 33, "[新規 (New)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 300, 33, (g_cabinet.view_mode == CAB_VIEW_LIST) ? "[表示: 一覧]" : "[表示: アイコン]", COLOR_BLUE, 0x00000000);

    /* Render Items */
    int start_y = 62;
    for (int i = 0; i < g_cabinet.item_count; i++) {
        CABINET_ITEM *it = &g_cabinet.items[i];
        int item_y = start_y + (i * 22);
        if (item_y + 20 > dev->height - 24) break;

        RECT row_rect = { 8, item_y - 2, dev->width - 8, item_y + 18 };

        if (i == g_cabinet.selected_idx) {
            fill_rec(dev, &row_rect, COLOR_NAVY);
        } else if (i == g_cabinet.hovered_idx) {
            fill_rec(dev, &row_rect, COLOR_LTGRAY);
            drw_rec(dev, &row_rect);
        }

        COLOR txt_col = (i == g_cabinet.selected_idx) ? COLOR_WHITE : COLOR_BLACK;

        char line[128];
        snprintf(line, sizeof(line), "%-6s  #%-4d  %-26s  %6u B",
                 it->icon_tag, it->robj_id, it->name, it->size_bytes);

        drw_tc_string(dev, 14, item_y, line, txt_col, 0x00000000);
    }

    /* Status Bar Footer */
    RECT status_r = { 0, dev->height - 22, dev->width, dev->height };
    fill_rec(dev, &status_r, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 22, dev->width, dev->height - 22);

    char foot_text[128];
    snprintf(foot_text, sizeof(foot_text), "Cabinet: %d Real Objects | Selected: #%d (%s)",
             g_cabinet.item_count,
             g_cabinet.items[g_cabinet.selected_idx].robj_id,
             g_cabinet.items[g_cabinet.selected_idx].name);
    drw_tc_string(dev, 8, dev->height - 17, foot_text, COLOR_BLACK, 0x00000000);
}

static void handle_vobj_manager_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    H rel_x = evt->pos.x - (wnd->bounds.left + 4);
    H rel_y = evt->pos.y - (wnd->bounds.top + 26);

    if (evt->type == EV_BUT_DOWN) {
        /* Toolbar click */
        if (rel_y >= 28 && rel_y <= 52) {
            if (rel_x >= 10 && rel_x <= 100) {
                /* [開く (Open)] */
                if (g_cabinet.selected_idx >= 0 && g_cabinet.selected_idx < g_cabinet.item_count) {
                    open_tad_browser_window(g_cabinet.items[g_cabinet.selected_idx].path,
                                           g_cabinet.items[g_cabinet.selected_idx].name);
                }
            } else if (rel_x >= 110 && rel_x <= 200) {
                /* [閲覧 (View)] */
                if (g_cabinet.selected_idx >= 0 && g_cabinet.selected_idx < g_cabinet.item_count) {
                    open_tad_browser_window(g_cabinet.items[g_cabinet.selected_idx].path,
                                           g_cabinet.items[g_cabinet.selected_idx].name);
                }
            } else if (rel_x >= 210 && rel_x <= 290) {
                /* [新規 (New)] */
                if (g_cabinet.item_count < MAX_CABINET_ITEMS) {
                    int new_id = 200 + g_cabinet.item_count;
                    char new_name[32];
                    snprintf(new_name, sizeof(new_name), "New_Doc_%d.tad", new_id);
                    g_cabinet.items[g_cabinet.item_count] = (CABINET_ITEM){
                        new_id, VOBJ_TYPE_TEXT, "", "", 0, "[TAD]"
                    };
                    strncpy(g_cabinet.items[g_cabinet.item_count].name, new_name, 63);
                    strncpy(g_cabinet.items[g_cabinet.item_count].path, "dharma/01_btron3_spec.tad", 127);
                    g_cabinet.selected_idx = g_cabinet.item_count;
                    g_cabinet.item_count++;
                }
            } else if (rel_x >= 300 && rel_x <= 420) {
                /* Toggle View Mode */
                g_cabinet.view_mode = (g_cabinet.view_mode == CAB_VIEW_LIST) ? CAB_VIEW_GRID : CAB_VIEW_LIST;
            }
            return;
        }

        /* Item Selection Click */
        int start_y = 62;
        int idx = (rel_y - start_y) / 22;
        if (idx >= 0 && idx < g_cabinet.item_count) {
            static int s_last_click_idx = -1;
            static UW s_last_click_time = 0;
            UW cur_time = (UW)(uintptr_t)evt->data;
            BOOL is_double = (s_last_click_idx == idx && (cur_time - s_last_click_time < 400 || s_last_click_time == 0));

            g_cabinet.selected_idx = idx;
            s_last_click_idx = idx;
            s_last_click_time = cur_time;

            if (is_double && idx >= 0 && idx < g_cabinet.item_count) {
                open_tad_browser_window(g_cabinet.items[idx].path, g_cabinet.items[idx].name);
            }
        }
        return;
    }

    if (evt->type == EV_MOUSE_MOVE) {
        int start_y = 62;
        int idx = (rel_y - start_y) / 22;
        if (idx >= 0 && idx < g_cabinet.item_count) {
            g_cabinet.hovered_idx = idx;
        } else {
            g_cabinet.hovered_idx = -1;
        }
        return;
    }

    if (evt->type == EV_KEY_DOWN) {
        UW key = evt->key;
        if (key == BTRON_KEY_UP || key == 'k' || key == 'K') {
            if (g_cabinet.selected_idx > 0) g_cabinet.selected_idx--;
        } else if (key == BTRON_KEY_DOWN || key == 'j' || key == 'J') {
            if (g_cabinet.selected_idx < g_cabinet.item_count - 1) g_cabinet.selected_idx++;
        } else if (key == '\n' || key == '\r' || key == ' ') {
            if (g_cabinet.selected_idx >= 0 && g_cabinet.selected_idx < g_cabinet.item_count) {
                open_tad_browser_window(g_cabinet.items[g_cabinet.selected_idx].path,
                                       g_cabinet.items[g_cabinet.selected_idx].name);
            }
        }
    }
}

BOOL cabinet_handle_click(int mouse_x, int mouse_y, BOOL is_double_click, ID *out_robj_id, char *out_path) {
    (void)mouse_x;
    if (mouse_y >= 28 && mouse_y <= 52) {
        /* Toolbar click */
        if (mouse_x >= 300 && mouse_x <= 400) {
            g_cabinet.view_mode = (g_cabinet.view_mode == CAB_VIEW_LIST) ? CAB_VIEW_GRID : CAB_VIEW_LIST;
            return FALSE;
        }
    }

    int start_y = 62;
    int idx = (mouse_y - start_y) / 22;
    if (idx >= 0 && idx < g_cabinet.item_count) {
        g_cabinet.selected_idx = idx;
        if (out_robj_id) *out_robj_id = g_cabinet.items[idx].robj_id;
        if (out_path) strncpy(out_path, g_cabinet.items[idx].path, 127);

        if (is_double_click) {
            /* Open TAD Document Browser for the selected real object */
            open_tad_browser_window(g_cabinet.items[idx].path, g_cabinet.items[idx].name);
            return TRUE;
        }
    }
    return FALSE;
}

WND* open_vobj_manager_window(void) {
    if (g_cabinet.item_count == 0) {
        cabinet_init_defaults(&g_cabinet);
    }
    WND *wnd = opn_wnd("BTRON Cabinet Explorer (実身キャビネット)", 80, 60, 520, 320,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_vobj_manager;
        wnd->event_handler = handle_vobj_manager_event;
    }
    return wnd;
}
