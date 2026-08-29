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
#include <dirent.h>
#include <sys/stat.h>
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

/* ── Canonical Table of Contents (TOC) Sequence Weighting ───────────────── */
static int get_toc_order(const char *path, const char *name) {
    if (!path) return 1000;

    /* 1. Canonical Foundational Books & Main Portals */
    if (strstr(path, "01_btron3_spec")) return 10;
    if (strstr(path, "02_tkernel_book")) return 20;
    if (strstr(path, "03_bfree_os_book")) return 30;

    /* 2. Top-level portal index */
    if (name && strcmp(name, "index.tad") == 0 && strcmp(path, "tad_bin/index.tad") == 0) return 40;

    /* 3. [doc] Shared Data Specifications (Chapter 1..6) */
    if (strstr(path, "shared_data/data_type")) return 100;
    if (strstr(path, "shared_data/tron_code")) return 110;
    if (strstr(path, "shared_data/tad1"))      return 120;
    if (strstr(path, "shared_data/tad2"))      return 130;
    if (strstr(path, "shared_data/tad3"))      return 140;
    if (strstr(path, "shared_data/fd_format")) return 150;
    if (strstr(path, "shared_data/index.tad")) return 160;
    if (strstr(path, "shared_data/indexfig"))  return 170;

    /* 4. [doc] OS Spec Portals */
    if (strstr(path, "os_spec/index.tad"))     return 200;
    if (strstr(path, "os_spec/indexfig.tad"))  return 205;

    /* 5. [doc] μITRON 3.0 Real-Time Kernel Subsystems */
    if (strstr(path, "os_spec/kernel/kernel"))   return 210;
    if (strstr(path, "os_spec/kernel/proc"))     return 220;
    if (strstr(path, "os_spec/kernel/memory"))   return 230;
    if (strstr(path, "os_spec/kernel/file"))     return 240;
    if (strstr(path, "os_spec/kernel/event"))    return 250;
    if (strstr(path, "os_spec/kernel/clk"))      return 260;
    if (strstr(path, "os_spec/kernel/device"))   return 270;
    if (strstr(path, "os_spec/kernel/message"))  return 280;
    if (strstr(path, "os_spec/kernel/taskcomm")) return 290;
    if (strstr(path, "os_spec/kernel/system"))   return 300;
    if (strstr(path, "os_spec/kernel/gname"))    return 310;

    /* 6. [doc] 2D Display Primitives Graphics */
    if (strstr(path, "os_spec/dp/dp.tad"))           return 400;
    if (strstr(path, "os_spec/dp/basic_concept"))    return 410;
    if (strstr(path, "os_spec/dp/basic_func"))       return 420;
    if (strstr(path, "os_spec/dp/character_func"))   return 430;
    if (strstr(path, "os_spec/dp/figure_func"))      return 440;
    if (strstr(path, "os_spec/dp/pointer_func"))     return 450;
    if (strstr(path, "os_spec/dp/intro"))            return 460;

    /* 7. [doc] BTRON3 Graphical User Interface & Shell */
    if (strstr(path, "os_spec/shell/shell.tad"))     return 500;
    if (strstr(path, "os_spec/shell/window"))        return 510;
    if (strstr(path, "os_spec/shell/menu"))          return 520;
    if (strstr(path, "os_spec/shell/panel"))         return 530;
    if (strstr(path, "os_spec/shell/parts"))         return 540;
    if (strstr(path, "os_spec/shell/font_mgr"))      return 550;
    if (strstr(path, "os_spec/shell/printmgr"))      return 560;
    if (strstr(path, "os_spec/shell/tip"))           return 570;
    if (strstr(path, "os_spec/shell/tray"))          return 580;
    if (strstr(path, "os_spec/shell/tcpip"))         return 590;
    if (strstr(path, "os_spec/shell/omgr"))          return 600;
    if (strstr(path, "os_spec/shell/data"))          return 610;

    /* 8. [b-free] B-Free OS Architecture & Manifesto */
    if (strstr(path, "b-free/manifest"))   return 700;
    if (strstr(path, "b-free/kernel"))     return 710;
    if (strstr(path, "b-free/posix"))      return 720;
    if (strstr(path, "b-free/btron"))      return 730;
    if (strstr(path, "b-free/boot_arch"))  return 740;
    if (strstr(path, "b-free/source_tree"))return 750;
    if (strstr(path, "b-free/index"))      return 760;

    /* 9. [b-system] B-System POSIX & VirtIO Specs */
    if (strstr(path, "b-system/virtio"))     return 800;
    if (strstr(path, "b-system/btron_spec")) return 810;
    if (strstr(path, "b-system/kernel"))     return 820;
    if (strstr(path, "b-system/license"))    return 830;
    if (strstr(path, "b-system/index"))      return 840;

    /* 10. [t-kernel] T-Kernel 2.0 Real-Time OS Specs */
    if (strstr(path, "t-kernel/tkernel_spec"))    return 900;
    if (strstr(path, "t-kernel/tkernel_startup")) return 910;
    if (strstr(path, "t-kernel/tkernel_qemu"))    return 920;
    if (strstr(path, "t-kernel/index"))           return 930;

    return 1000;
}

/* ── Sort Cabinet Items: Group by First Column (icon_tag) & TOC Order ──── */
static void cabinet_sort_items(CABINET_EXPLORER *cab) {
    if (!cab || cab->item_count <= 1) return;
    for (int i = 0; i < cab->item_count - 1; i++) {
        for (int j = i + 1; j < cab->item_count; j++) {
            /* 1. Primary: Group by First Column (icon_tag) */
            int cmp_grp = strcmp(cab->items[i].icon_tag, cab->items[j].icon_tag);
            int swap = 0;
            if (cmp_grp > 0) {
                swap = 1;
            } else if (cmp_grp == 0) {
                /* 2. Secondary: Canonical Table of Contents (TOC) Sequence */
                int order_i = get_toc_order(cab->items[i].path, cab->items[i].name);
                int order_j = get_toc_order(cab->items[j].path, cab->items[j].name);
                if (order_i > order_j) {
                    swap = 1;
                } else if (order_i == order_j) {
                    /* 3. Tertiary: Natural Name Order */
                    if (natural_compare(cab->items[i].name, cab->items[j].name) > 0) {
                        swap = 1;
                    }
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

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1

static ID deduce_robj_id(const char *path) {
    if (strstr(path, "01_btron3_spec")) return 101;
    if (strstr(path, "02_tkernel_book")) return 102;
    if (strstr(path, "03_bfree_os_book")) return 103;
    if (strstr(path, "b-free/manifest.tad")) return 106;
    if (strstr(path, "b-system/virtio.tad")) return 107;
    if (strstr(path, "data_type.tad")) return 111;
    if (strstr(path, "tron_code.tad")) return 112;
    if (strstr(path, "tad1.tad")) return 113;
    if (strstr(path, "tad2.tad")) return 114;
    if (strstr(path, "tad3.tad")) return 115;
    if (strstr(path, "fd_format.tad")) return 116;
    if (strstr(path, "kernel/kernel.tad")) return 121;
    if (strstr(path, "kernel/proc.tad")) return 122;
    if (strstr(path, "kernel/memory.tad")) return 123;
    if (strstr(path, "dp/dp.tad")) return 131;
    if (strstr(path, "shell/shell.tad")) return 141;
    if (strstr(path, "shell/window.tad")) return 142;
    if (strstr(path, "indexfig.tad")) return 161;

    UW h = 5381;
    for (int i = 0; path[i]; i++) {
        h = ((h << 5) + h) + (UB)path[i];
    }
    return 100 + (h % 900);
}

static const char* deduce_toc_path(const char *path) {
    if (strstr(path, "01_btron3_spec") || strstr(path, "02_tkernel_book") ||
        strstr(path, "03_bfree_os_book")) {
        return "books/";
    }
    if (strstr(path, "shared_data/")) return "shared_data/";
    if (strstr(path, "os_spec/kernel/")) return "os_spec/kernel/";
    if (strstr(path, "os_spec/dp/")) return "os_spec/dp/";
    if (strstr(path, "os_spec/shell/")) return "os_spec/shell/";
    if (strstr(path, "os_spec/")) return "os_spec/";
    if (strstr(path, "b-free/")) return "b-free/";
    if (strstr(path, "b-system/")) return "b-system/";
    if (strstr(path, "t-kernel/")) return "t-kernel/";
    return "root/";
}

static const char* deduce_icon_tag(const char *path) {
    if (strstr(path, "03_bfree") || strstr(path, "b-free")) return "[b-free]";
    if (strstr(path, "b-system")) return "[b-system]";
    if (strstr(path, "02_tkernel") || strstr(path, "t-kernel")) return "[t-kernel]";
    if (strstr(path, "01_btron3") || strstr(path, "shared_data") || strstr(path, "os_spec") || strstr(path, "doc")) return "[doc]";
    return "[doc]";
}

/* ── Dynamic Recursive Filesystem Discovery (Discovered not Hardcoded) ── */
static void cabinet_discover_dir(CABINET_EXPLORER *cab, const char *dir_path) {
    DIR *d = opendir(dir_path);
    if (!d) return;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;

        char sub_path[256];
        snprintf(sub_path, sizeof(sub_path), "%s/%s", dir_path, de->d_name);

        struct stat st;
        if (stat(sub_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            cabinet_discover_dir(cab, sub_path);
        } else if (S_ISREG(st.st_mode)) {
            int len = strlen(de->d_name);
            if (len > 4 && strcmp(de->d_name + len - 4, ".tad") == 0) {
                if (cab->item_count < MAX_CABINET_ITEMS) {
                    CABINET_ITEM *it = &cab->items[cab->item_count++];
                    it->robj_id = deduce_robj_id(sub_path);
                    it->type = VOBJ_TYPE_TEXT;
                    strncpy(it->name, de->d_name, sizeof(it->name) - 1);
                    strncpy(it->path, sub_path, sizeof(it->path) - 1);
                    it->size_bytes = (UW)st.st_size;
                    it->icon_tag = deduce_icon_tag(sub_path);
                    it->category = deduce_toc_path(sub_path);
                }
            }
        }
    }
    closedir(d);
}
#endif

static void cabinet_init_defaults(CABINET_EXPLORER *cab) {
    memset(cab, 0, sizeof(CABINET_EXPLORER));
    cab->selected_idx = 0;
    cab->hovered_idx = -1;
    cab->scroll_offset = 0;
    cab->view_mode = CAB_VIEW_LIST;

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    /* Dynamic discovery from tad_bin/ directory tree */
    cabinet_discover_dir(cab, "tad_bin");
#endif

    if (cab->item_count > 0) {
        /* Group by first column & sort by TOC order */
        cabinet_sort_items(cab);
        snprintf(cab->status_msg, sizeof(cab->status_msg),
                 "Cabinet Ready. Discovered %d Real Objects across tad_bin/.", cab->item_count);
        return;
    }

    /* Fallback static list for freestanding embedded environments */
    int n = 0;
    cab->items[n++] = (CABINET_ITEM){ 101, VOBJ_TYPE_TEXT, "01_btron3_spec.tad", "tad_bin/01_btron3_spec.tad", 3996, "[doc]", "books/" };
    cab->items[n++] = (CABINET_ITEM){ 102, VOBJ_TYPE_TEXT, "02_tkernel_book.tad", "tad_bin/02_tkernel_book.tad", 1745, "[t-kernel]", "books/" };
    cab->items[n++] = (CABINET_ITEM){ 103, VOBJ_TYPE_TEXT, "03_bfree_os_book.tad", "tad_bin/03_bfree_os_book.tad", 1472, "[b-free]", "books/" };
    cab->items[n++] = (CABINET_ITEM){ 111, VOBJ_TYPE_TEXT, "01_data_type.tad", "tad_bin/shared_data/data_type.tad", 10022, "[doc]", "shared_data/" };
    cab->item_count = n;
    cabinet_sort_items(cab);
    strncpy(cab->status_msg, "Cabinet Ready (Embedded Static Fallback).", sizeof(cab->status_msg) - 1);
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
    drw_tc_string(dev, 100, 33, "[閲覧 (View)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 190, 33, "[新規 (New)]", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 275, 33, (g_cabinet.view_mode == CAB_VIEW_LIST) ? "[表示: 一覧]" : "[表示: アイコン]", COLOR_BLUE, 0x00000000);
    drw_tc_string(dev, 385, 33, "[↻ 再走査 (Rescan)]", COLOR_NAVY, 0x00000000);

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

        /* Col 1: Icon Tag (e.g. [b-system], [t-kernel], [b-free], [doc]) */
        drw_tc_string(dev, 14, item_y, it->icon_tag, txt_col, 0x00000000);

        /* Col 2: TOC Hierarchy Path (e.g. os_spec/kernel/, shared_data/, books/) */
        drw_tc_string(dev, 115, item_y, it->category, txt_col, 0x00000000);

        /* Col 3: Real Object ID */
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "#%-4d", it->robj_id);
        drw_tc_string(dev, 265, item_y, id_str, txt_col, 0x00000000);

        /* Col 4: Document Name */
        drw_tc_string(dev, 315, item_y, it->name, txt_col, 0x00000000);

        /* Col 5: File Size */
        char sz_str[24];
        snprintf(sz_str, sizeof(sz_str), "%6u B", it->size_bytes);
        drw_tc_string(dev, dev->width - 85, item_y, sz_str, txt_col, 0x00000000);
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
            } else if (rel_x >= 275 && rel_x <= 375) {
                /* Toggle View Mode */
                g_cabinet.view_mode = (g_cabinet.view_mode == CAB_VIEW_LIST) ? CAB_VIEW_GRID : CAB_VIEW_LIST;
            } else if (rel_x >= 380 && rel_x <= 500) {
                /* [↻ 再走査 (Rescan)] */
                cabinet_init_defaults(&g_cabinet);
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
        if (mouse_x >= 275 && mouse_x <= 375) {
            g_cabinet.view_mode = (g_cabinet.view_mode == CAB_VIEW_LIST) ? CAB_VIEW_GRID : CAB_VIEW_LIST;
            return FALSE;
        } else if (mouse_x >= 380 && mouse_x <= 500) {
            cabinet_init_defaults(&g_cabinet);
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
