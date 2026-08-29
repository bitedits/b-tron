/*
 * BTRON Accessory: Real Object Cabinet & Virtual Object Explorer Window (vobj_manager)
 * Cleanroom implementation of Sakamura BTRON / BTRON3 Architecture & NASA JPL Scope.
 */

#include <btron/wnd.h>
#include <btron/vobj.h>
#include <btron/troncode.h>
#include <btron/dp.h>
#include <btron/event.h>
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

#define MAX_CABINET_ITEMS 128

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
    const char *category;
} CABINET_ITEM;

typedef struct {
    CABINET_ITEM items[MAX_CABINET_ITEMS];
    int item_count;
    int selected_idx;
    int hovered_idx;
    int scroll_offset;      /* Scroll item row offset */
    CAB_VIEW_MODE view_mode;
    char status_msg[128];
} CABINET_EXPLORER;

static CABINET_EXPLORER g_cabinet;

/* ── Natural String Comparison (Case-Insensitive & Numeric Aware) ───────── */
static int natural_compare(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (*s1 >= '0' && *s1 <= '9' && *s2 >= '0' && *s2 <= '9') {
            long n1 = 0, n2 = 0;
            while (*s1 >= '0' && *s1 <= '9') {
                n1 = n1 * 10 + (*s1 - '0');
                s1++;
            }
            while (*s2 >= '0' && *s2 <= '9') {
                n2 = n2 * 10 + (*s2 - '0');
                s2++;
            }
            if (n1 != n2) return (n1 < n2) ? -1 : 1;
        } else {
            char c1 = *s1;
            char c2 = *s2;
            if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
            if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
            if (c1 != c2) return (c1 < c2) ? -1 : 1;
            s1++;
            s2++;
        }
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : (*s1 == '\0' ? -1 : 1);
}

/* ── Group by First Column (icon_tag) & Sort by Natural Name Order ─────── */
static void cabinet_sort_items(CABINET_EXPLORER *cab) {
    if (!cab || cab->item_count <= 1) return;
    for (int i = 0; i < cab->item_count - 1; i++) {
        for (int j = i + 1; j < cab->item_count; j++) {
            int cmp_grp = strcmp(cab->items[i].icon_tag, cab->items[j].icon_tag);
            int swap = 0;
            if (cmp_grp > 0) {
                swap = 1;
            } else if (cmp_grp == 0) {
                if (natural_compare(cab->items[i].name, cab->items[j].name) > 0) {
                    swap = 1;
                }
            }
            if (swap) {
                CABINET_ITEM tmp = cab->items[i];
                cab->items[i] = cab->items[j];
                cab->items[j] = tmp;
            }
        }
    }
}

static void cabinet_init_defaults(CABINET_EXPLORER *cab) {
    memset(cab, 0, sizeof(CABINET_EXPLORER));
    cab->selected_idx = 0;
    cab->hovered_idx = -1;
    cab->scroll_offset = 0;
    cab->view_mode = CAB_VIEW_LIST;

    int n = 0;

    /* ── [b-free] Free Software BTRON3 Implementation (b-free/) ────────────── */
    cab->items[n++] = (CABINET_ITEM){ 104, VOBJ_TYPE_TEXT, "00_bfree_book.tad", "tad_bin/04_bfree_os_book.tad", 1472, "[b-free]", "[和文読本]" };
    cab->items[n++] = (CABINET_ITEM){ 106, VOBJ_TYPE_TEXT, "01_manifest.tad", "tad_bin/b-free/manifest.tad", 4198, "[b-free]", "[マニフェスト]" };
    cab->items[n++] = (CABINET_ITEM){ 301, VOBJ_TYPE_TEXT, "02_kernel.tad", "tad_bin/b-free/kernel.tad", 4362, "[b-free]", "[マイクロカーネル]" };
    cab->items[n++] = (CABINET_ITEM){ 302, VOBJ_TYPE_TEXT, "03_posix.tad", "tad_bin/b-free/posix.tad", 3838, "[b-free]", "[POSIX互換層]" };
    cab->items[n++] = (CABINET_ITEM){ 303, VOBJ_TYPE_TEXT, "04_btron_env.tad", "tad_bin/b-free/btron.tad", 3845, "[b-free]", "[統合環境]" };
    cab->items[n++] = (CABINET_ITEM){ 304, VOBJ_TYPE_TEXT, "05_boot_arch.tad", "tad_bin/b-free/boot_arch.tad", 2449, "[b-free]", "[ブート機構]" };
    cab->items[n++] = (CABINET_ITEM){ 305, VOBJ_TYPE_TEXT, "06_source_tree.tad", "tad_bin/b-free/source_tree.tad", 1923, "[b-free]", "[ソースツリー]" };
    cab->items[n++] = (CABINET_ITEM){ 306, VOBJ_TYPE_TEXT, "07_bfree_index.tad", "tad_bin/b-free/index.tad", 2386, "[b-free]", "[索引]" };

    /* ── [b-system] B-System POSIX & VirtIO Specs (b-system/) ──────────────── */
    cab->items[n++] = (CABINET_ITEM){ 107, VOBJ_TYPE_TEXT, "01_virtio.tad", "tad_bin/b-system/virtio.tad", 14871, "[b-system]", "[VirtIO仕様]" };
    cab->items[n++] = (CABINET_ITEM){ 401, VOBJ_TYPE_TEXT, "02_btron_spec.tad", "tad_bin/b-system/btron_spec.tad", 4378, "[b-system]", "[システムコール]" };
    cab->items[n++] = (CABINET_ITEM){ 402, VOBJ_TYPE_TEXT, "03_kernel_arch.tad", "tad_bin/b-system/kernel.tad", 4637, "[b-system]", "[カーネル構造]" };
    cab->items[n++] = (CABINET_ITEM){ 403, VOBJ_TYPE_TEXT, "04_license.tad", "tad_bin/b-system/license.tad", 3619, "[b-system]", "[ライセンス]" };
    cab->items[n++] = (CABINET_ITEM){ 404, VOBJ_TYPE_TEXT, "05_bsystem_index.tad", "tad_bin/b-system/index.tad", 3063, "[b-system]", "[索引]" };

    /* ── [doc] BTRON3 Standard Specifications (doc/) ───────────────────────── */
    cab->items[n++] = (CABINET_ITEM){ 101, VOBJ_TYPE_TEXT, "00_btron3_spec.tad", "tad_bin/01_btron3_spec.tad", 3996, "[doc]", "[和文仕様]" };
    cab->items[n++] = (CABINET_ITEM){ 111, VOBJ_TYPE_TEXT, "01_data_type.tad", "tad_bin/shared_data/data_type.tad", 10022, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 112, VOBJ_TYPE_TEXT, "02_tron_code.tad", "tad_bin/shared_data/tron_code.tad", 12995, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 113, VOBJ_TYPE_TEXT, "03_tad_spec.tad", "tad_bin/shared_data/tad1.tad", 23533, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 114, VOBJ_TYPE_TEXT, "04_text_fusen.tad", "tad_bin/shared_data/tad2.tad", 34198, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 115, VOBJ_TYPE_TEXT, "05_figure_fusen.tad", "tad_bin/shared_data/tad3.tad", 29942, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 116, VOBJ_TYPE_TEXT, "06_btron_fs.tad", "tad_bin/shared_data/fd_format.tad", 12484, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 117, VOBJ_TYPE_TEXT, "07_shared_index.tad", "tad_bin/shared_data/index.tad", 4588, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 118, VOBJ_TYPE_TEXT, "08_shared_fig.tad", "tad_bin/shared_data/indexfig.tad", 4648, "[doc]", "[Специфікація]" };
    cab->items[n++] = (CABINET_ITEM){ 121, VOBJ_TYPE_TEXT, "09_kernel_core.tad", "tad_bin/os_spec/kernel/kernel.tad", 6173, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 122, VOBJ_TYPE_TEXT, "10_kernel_proc.tad", "tad_bin/os_spec/kernel/proc.tad", 38370, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 123, VOBJ_TYPE_TEXT, "11_kernel_memory.tad", "tad_bin/os_spec/kernel/memory.tad", 17338, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 124, VOBJ_TYPE_TEXT, "12_kernel_file.tad", "tad_bin/os_spec/kernel/file.tad", 39206, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 125, VOBJ_TYPE_TEXT, "13_kernel_event.tad", "tad_bin/os_spec/kernel/event.tad", 20879, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 126, VOBJ_TYPE_TEXT, "14_kernel_clk.tad", "tad_bin/os_spec/kernel/clk.tad", 17689, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 127, VOBJ_TYPE_TEXT, "15_kernel_device.tad", "tad_bin/os_spec/kernel/device.tad", 18294, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 128, VOBJ_TYPE_TEXT, "16_kernel_message.tad", "tad_bin/os_spec/kernel/message.tad", 32854, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 129, VOBJ_TYPE_TEXT, "17_kernel_taskcomm.tad", "tad_bin/os_spec/kernel/taskcomm.tad", 23328, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 130, VOBJ_TYPE_TEXT, "18_kernel_system.tad", "tad_bin/os_spec/kernel/system.tad", 13837, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 1301, VOBJ_TYPE_TEXT, "19_kernel_gname.tad", "tad_bin/os_spec/kernel/gname.tad", 10475, "[doc]", "[Ядро]" };
    cab->items[n++] = (CABINET_ITEM){ 131, VOBJ_TYPE_TEXT, "20_dp_graphics.tad", "tad_bin/os_spec/dp/dp.tad", 3139, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 132, VOBJ_TYPE_TEXT, "21_dp_basic_concept.tad", "tad_bin/os_spec/dp/basic_concept.tad", 9237, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 133, VOBJ_TYPE_TEXT, "22_dp_basic_func.tad", "tad_bin/os_spec/dp/basic_func.tad", 26482, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 134, VOBJ_TYPE_TEXT, "23_dp_char_func.tad", "tad_bin/os_spec/dp/character_func.tad", 9397, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 135, VOBJ_TYPE_TEXT, "24_dp_figure_func.tad", "tad_bin/os_spec/dp/figure_func.tad", 15829, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 136, VOBJ_TYPE_TEXT, "25_dp_pointer_func.tad", "tad_bin/os_spec/dp/pointer_func.tad", 8339, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 137, VOBJ_TYPE_TEXT, "26_dp_intro.tad", "tad_bin/os_spec/dp/intro.tad", 4120, "[doc]", "[Графіка]" };
    cab->items[n++] = (CABINET_ITEM){ 141, VOBJ_TYPE_TEXT, "27_gui_shell.tad", "tad_bin/os_spec/shell/shell.tad", 4430, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 142, VOBJ_TYPE_TEXT, "28_shell_window.tad", "tad_bin/os_spec/shell/window.tad", 87963, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 143, VOBJ_TYPE_TEXT, "29_shell_menu.tad", "tad_bin/os_spec/shell/menu.tad", 32447, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 144, VOBJ_TYPE_TEXT, "30_shell_panel.tad", "tad_bin/os_spec/shell/panel.tad", 24067, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 145, VOBJ_TYPE_TEXT, "31_shell_parts.tad", "tad_bin/os_spec/shell/parts.tad", 58356, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 146, VOBJ_TYPE_TEXT, "32_shell_font_mgr.tad", "tad_bin/os_spec/shell/font_mgr.tad", 23609, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 147, VOBJ_TYPE_TEXT, "33_shell_printmgr.tad", "tad_bin/os_spec/shell/printmgr.tad", 18096, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 148, VOBJ_TYPE_TEXT, "34_shell_tip_ime.tad", "tad_bin/os_spec/shell/tip.tad", 17004, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 149, VOBJ_TYPE_TEXT, "35_shell_tray.tad", "tad_bin/os_spec/shell/tray.tad", 18136, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 150, VOBJ_TYPE_TEXT, "36_shell_tcpip.tad", "tad_bin/os_spec/shell/tcpip.tad", 23101, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 151, VOBJ_TYPE_TEXT, "37_shell_omgr.tad", "tad_bin/os_spec/shell/omgr.tad", 92692, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 152, VOBJ_TYPE_TEXT, "38_shell_data.tad", "tad_bin/os_spec/shell/data.tad", 15670, "[doc]", "[Оболонка]" };
    cab->items[n++] = (CABINET_ITEM){ 161, VOBJ_TYPE_TEXT, "39_static_analysis.tad", "tad_bin/os_spec/indexfig.tad", 7167, "[doc]", "[Надійність]" };

    /* ── [t-kernel] T-Kernel 2.0 Real-Time OS Specs (t-kernel/) ────────────── */
    cab->items[n++] = (CABINET_ITEM){ 102, VOBJ_TYPE_TEXT, "00_tkernel_book.tad", "tad_bin/02_tkernel_book.tad", 1745, "[t-kernel]", "[和文仕様]" };
    cab->items[n++] = (CABINET_ITEM){ 501, VOBJ_TYPE_TEXT, "01_tkernel_spec.tad", "tad_bin/t-kernel/tkernel_spec.tad", 4129, "[t-kernel]", "[T-Kernel]" };
    cab->items[n++] = (CABINET_ITEM){ 502, VOBJ_TYPE_TEXT, "02_tkernel_startup.tad", "tad_bin/t-kernel/tkernel_startup.tad", 2687, "[t-kernel]", "[T-Kernel]" };
    cab->items[n++] = (CABINET_ITEM){ 503, VOBJ_TYPE_TEXT, "03_tkernel_qemu.tad", "tad_bin/t-kernel/tkernel_qemu.tad", 2410, "[t-kernel]", "[T-Kernel]" };
    cab->items[n++] = (CABINET_ITEM){ 504, VOBJ_TYPE_TEXT, "04_tkernel_index.tad", "tad_bin/t-kernel/index.tad", 1506, "[t-kernel]", "[T-Kernel]" };

    cab->item_count = n;

    /* Execute automatic natural sorting grouped by first column */
    cabinet_sort_items(cab);

    strncpy(cab->status_msg, "Cabinet Ready. Double-click any Real Object to open in TAD Browser.", sizeof(cab->status_msg) - 1);
}

static void paint_vobj_manager(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);

    /* Title Bar / Header */
    RECT header_r = { 0, 0, dev->width, 26 };
    fill_rec(dev, &header_r, COLOR_NAVY);
    drw_tc_string(dev, 10, 6, "REAL OBJECT CABINET / HYPER-DATA STORE (実身・仮身キャビネット)", COLOR_WHITE, 0x00000000);

    /* Toolbar Header (y = 26..56) */
    RECT toolbar_r = { 0, 26, dev->width, 56 };
    fill_rec(dev, &toolbar_r, COLOR_LTGRAY);
    drw_lin(dev, 0, 56, dev->width, 56);

    drw_tc_string(dev, 10, 33, "[開く (Open)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 110, 33, "[閲覧 (View)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 210, 33, "[新規 (New)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 300, 33, (g_cabinet.view_mode == CAB_VIEW_LIST) ? "[表示: 一覧]" : "[表示: アイコン]", COLOR_BLUE, 0x00000000);

    /* List Content Viewport */
    int start_y = 60;
    int visible_rows = (dev->height - 84) / 22;
    if (visible_rows < 1) visible_rows = 1;

    int max_scroll = g_cabinet.item_count - visible_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (g_cabinet.scroll_offset > max_scroll) g_cabinet.scroll_offset = max_scroll;
    if (g_cabinet.scroll_offset < 0) g_cabinet.scroll_offset = 0;

    int end_i = g_cabinet.scroll_offset + visible_rows;
    if (end_i > g_cabinet.item_count) end_i = g_cabinet.item_count;

    for (int i = g_cabinet.scroll_offset; i < end_i; i++) {
        CABINET_ITEM *it = &g_cabinet.items[i];
        int row_idx = i - g_cabinet.scroll_offset;
        int item_y = start_y + (row_idx * 22);

        RECT row_rect = { 8, item_y - 2, dev->width - 24, item_y + 18 };

        if (i == g_cabinet.selected_idx) {
            fill_rec(dev, &row_rect, COLOR_NAVY);
        } else if (i == g_cabinet.hovered_idx) {
            fill_rec(dev, &row_rect, COLOR_LTGRAY);
            drw_rec(dev, &row_rect);
        }

        COLOR txt_col = (i == g_cabinet.selected_idx) ? COLOR_WHITE : COLOR_BLACK;

        char line[128];
        snprintf(line, sizeof(line), "%-11s %-16s #%-4d %-24s %6u B",
                 it->icon_tag, it->category, it->robj_id, it->name, it->size_bytes);

        drw_tc_string(dev, 14, item_y, line, txt_col, 0x00000000);
    }

    /* Scrollbar Indicator (Right Margin) */
    if (g_cabinet.item_count > visible_rows) {
        int sb_x = dev->width - 16;
        RECT sb_track = { sb_x, 56, dev->width - 4, dev->height - 24 };
        fill_rec(dev, &sb_track, COLOR_LTGRAY);
        drw_rec(dev, &sb_track);

        int track_h = (dev->height - 80);
        int thumb_h = (visible_rows * track_h) / g_cabinet.item_count;
        if (thumb_h < 15) thumb_h = 15;
        int thumb_y = 56 + (g_cabinet.scroll_offset * (track_h - thumb_h)) / max_scroll;

        RECT sb_thumb = { sb_x + 1, thumb_y, dev->width - 5, thumb_y + thumb_h };
        fill_rec(dev, &sb_thumb, COLOR_DKGRAY);
        drw_rec(dev, &sb_thumb);
    }

    /* Status Bar Footer */
    RECT status_r = { 0, dev->height - 22, dev->width, dev->height };
    fill_rec(dev, &status_r, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 22, dev->width, dev->height - 22);

    char foot_text[128];
    snprintf(foot_text, sizeof(foot_text), "Cabinet: %d Real Objects [tad_bin] | #%d: %s (%u B)",
             g_cabinet.item_count,
             g_cabinet.items[g_cabinet.selected_idx].robj_id,
             g_cabinet.items[g_cabinet.selected_idx].name,
             g_cabinet.items[g_cabinet.selected_idx].size_bytes);
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
                    int new_id = 300 + g_cabinet.item_count;
                    char new_name[32];
                    snprintf(new_name, sizeof(new_name), "New_Doc_%d.tad", new_id);
                    g_cabinet.items[g_cabinet.item_count] = (CABINET_ITEM){
                        new_id, VOBJ_TYPE_TEXT, "", "", 0, "[TAD]", "User"
                    };
                    strncpy(g_cabinet.items[g_cabinet.item_count].name, new_name, 63);
                    strncpy(g_cabinet.items[g_cabinet.item_count].path, "tad_bin/01_btron3_spec.tad", 127);
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
        int start_y = 60;
        int row = (rel_y - start_y) / 22;
        int idx = g_cabinet.scroll_offset + row;
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
        int start_y = 60;
        int row = (rel_y - start_y) / 22;
        int idx = g_cabinet.scroll_offset + row;
        if (idx >= 0 && idx < g_cabinet.item_count) {
            g_cabinet.hovered_idx = idx;
        } else {
            g_cabinet.hovered_idx = -1;
        }
        return;
    }

    if (evt->type == EV_KEY_DOWN) {
        UW key = evt->key;
        if (key == BTRON_KEY_UP || key == 'k') {
            if (g_cabinet.selected_idx > 0) {
                g_cabinet.selected_idx--;
                if (g_cabinet.selected_idx < g_cabinet.scroll_offset) {
                    g_cabinet.scroll_offset = g_cabinet.selected_idx;
                }
            }
        } else if (key == BTRON_KEY_DOWN || key == 'j') {
            if (g_cabinet.selected_idx < g_cabinet.item_count - 1) {
                g_cabinet.selected_idx++;
                int visible_rows = (wnd->dev ? wnd->dev->height - 85 : 240) / 22;
                if (g_cabinet.selected_idx >= g_cabinet.scroll_offset + visible_rows) {
                    g_cabinet.scroll_offset = g_cabinet.selected_idx - visible_rows + 1;
                }
            }
        } else if (key == BTRON_KEY_PAGE_UP) {
            g_cabinet.scroll_offset -= 8;
            if (g_cabinet.scroll_offset < 0) g_cabinet.scroll_offset = 0;
            g_cabinet.selected_idx = g_cabinet.scroll_offset;
        } else if (key == BTRON_KEY_PAGE_DOWN) {
            g_cabinet.scroll_offset += 8;
            if (g_cabinet.scroll_offset > g_cabinet.item_count - 1) g_cabinet.scroll_offset = g_cabinet.item_count - 1;
            g_cabinet.selected_idx = g_cabinet.scroll_offset;
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

    int start_y = 60;
    int row = (mouse_y - start_y) / 22;
    int idx = g_cabinet.scroll_offset + row;
    if (idx >= 0 && idx < g_cabinet.item_count) {
        g_cabinet.selected_idx = idx;
        if (out_robj_id) *out_robj_id = g_cabinet.items[idx].robj_id;
        if (out_path) strncpy(out_path, g_cabinet.items[idx].path, 127);

        if (is_double_click) {
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
    WND *wnd = opn_wnd("BTRON Cabinet Explorer (実身キャビネット)", 80, 60, 560, 360,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_vobj_manager;
        wnd->event_handler = handle_vobj_manager_event;
    }
    return wnd;
}
