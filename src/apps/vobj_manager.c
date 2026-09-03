/*
 * B-System (BTRON 3.20) BTRON Accessory: Real Body Cabinet & Virtual Body Explorer Window (vobj_manager)
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
#define memset   tkl_memset
#define memcpy   tkl_memcpy
#define strlen   tkl_strlen
#define strcmp   tkl_strcmp
#define strncpy  tkl_strncpy
extern int tkl_snprintf(char *str, size_t size, const char *format, ...);
#define snprintf tkl_snprintf

static inline char* local_strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return (void*)0;
    size_t nlen = tkl_strlen(needle);
    if (nlen == 0) return (char*)haystack;
    while (*haystack) {
        if (tkl_memcmp(haystack, needle, nlen) == 0) return (char*)haystack;
        haystack++;
    }
    return (void*)0;
}
#define strstr local_strstr
#endif

#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) WND* open_t_editor_window(void) {
    return (void*)0;
}
#else
extern WND* open_t_editor_window(void);
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

    /* In-Window Application Menu State */
    int active_menu;       /* -1 = closed, 0..4 = active header index */
    int hover_menu;        /* -1 = none, 0..4 = hovered header in closed state */
    int hover_item;        /* -1 = none, 0..N = hovered dropdown item */
    int sort_mode;         /* 0=Name, 1=Date/ID, 2=Size, 3=TOC/Type */
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
    if (strstr(path, "04_tron_hmi_book")) return 35;

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

    /* 8. [b-hmi] TRON HMI Design Guidelines & Parts Book */
    if (strstr(path, "b-hmi/index"))      return 640;
    if (strstr(path, "b-hmi/part1"))      return 650;
    if (strstr(path, "b-hmi/part2"))      return 660;
    if (strstr(path, "b-hmi/part_book"))  return 670;
    if (strstr(path, "b-hmi"))            return 680;

    /* 9. [b-free] B-Free OS Architecture & Manifesto */
    if (strstr(path, "b-free/manifest"))   return 700;
    if (strstr(path, "b-free/kernel"))     return 710;
    if (strstr(path, "b-free/posix"))      return 720;
    if (strstr(path, "b-free/btron"))      return 730;
    if (strstr(path, "b-free/boot_arch"))  return 740;
    if (strstr(path, "b-free/source_tree"))return 750;
    if (strstr(path, "b-free/index"))      return 760;

    /* 10. [b-system] B-System POSIX & VirtIO Specs */
    if (strstr(path, "b-system/virtio"))     return 800;
    if (strstr(path, "b-system/btron_spec")) return 810;
    if (strstr(path, "b-system/kernel"))     return 820;
    if (strstr(path, "b-system/license"))    return 830;
    if (strstr(path, "b-system/index"))      return 840;

    /* 11. [t-kernel] T-Kernel 2.0 Real-Time OS Specs */
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
            int swap = 0;
            if (cab->sort_mode == 1) {
                /* Sort by Name */
                if (natural_compare(cab->items[i].name, cab->items[j].name) > 0) swap = 1;
            } else if (cab->sort_mode == 2) {
                /* Sort by ID / Date */
                if (cab->items[i].robj_id > cab->items[j].robj_id) swap = 1;
            } else if (cab->sort_mode == 3) {
                /* Sort by Size */
                if (cab->items[i].size_bytes < cab->items[j].size_bytes) swap = 1;
            } else {
                /* 0 = Default: Group by First Column (icon_tag) & TOC Order */
                int cmp_grp = strcmp(cab->items[i].icon_tag, cab->items[j].icon_tag);
                if (cmp_grp > 0) {
                    swap = 1;
                } else if (cmp_grp == 0) {
                    int order_i = get_toc_order(cab->items[i].path, cab->items[i].name);
                    int order_j = get_toc_order(cab->items[j].path, cab->items[j].name);
                    if (order_i > order_j) swap = 1;
                    else if (order_i == order_j && natural_compare(cab->items[i].name, cab->items[j].name) > 0) swap = 1;
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
    if (strstr(path, "04_tron_hmi_book")) return 104;
    if (strstr(path, "b-hmi/index.tad")) return 105;
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
        strstr(path, "03_bfree_os_book") || strstr(path, "04_tron_hmi_book")) {
        return "books/";
    }
    if (strstr(path, "shared_data/")) return "shared_data/";
    if (strstr(path, "os_spec/kernel/")) return "os_spec/kernel/";
    if (strstr(path, "os_spec/dp/")) return "os_spec/dp/";
    if (strstr(path, "os_spec/shell/")) return "os_spec/shell/";
    if (strstr(path, "os_spec/")) return "os_spec/";
    if (strstr(path, "b-hmi/part1")) return "b-hmi/part1/";
    if (strstr(path, "b-hmi/part2")) return "b-hmi/part2/";
    if (strstr(path, "b-hmi/part_book")) return "b-hmi/parts/";
    if (strstr(path, "b-hmi/")) return "b-hmi/";
    if (strstr(path, "b-free/")) return "b-free/";
    if (strstr(path, "b-system/")) return "b-system/";
    if (strstr(path, "t-kernel/")) return "t-kernel/";
    return "root/";
}

static const char* deduce_icon_tag(const char *path) {
    if (strstr(path, "04_tron_hmi") || strstr(path, "b-hmi")) return "[b-hmi]";
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
                 "Cabinet Ready. Discovered %d Real Bodys across tad_bin/.", cab->item_count);
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

#define CMENU_HDR_COUNT     5
#define CMENU_HDR_FILE      0
#define CMENU_HDR_EDIT      1
#define CMENU_HDR_VIEW      2
#define CMENU_HDR_VOBJ      3
#define CMENU_HDR_HELP      4

#define CMENU_DROPDOWN_WIDTH 250
#define CMENU_ROW_HEIGHT    20

enum {
    CCMD_NONE = 0,
    /* File */
    CCMD_FILE_OPEN = 10,
    CCMD_FILE_VIEW_TAD,
    CCMD_FILE_NEW,
    CCMD_FILE_DUPLICATE,
    CCMD_FILE_DELETE,
    CCMD_FILE_CLOSE,
    /* Edit */
    CCMD_EDIT_SELECT_ALL = 20,
    CCMD_EDIT_DESELECT,
    CCMD_EDIT_RENAME,
    CCMD_EDIT_PROPERTIES,
    /* View */
    CCMD_VIEW_LIST = 30,
    CCMD_VIEW_GRID,
    CCMD_VIEW_SORT_NAME,
    CCMD_VIEW_SORT_DATE,
    CCMD_VIEW_SORT_SIZE,
    CCMD_VIEW_SORT_TYPE,
    CCMD_VIEW_REFRESH,
    /* VObj */
    CCMD_VOBJ_CREATE_LINK = 40,
    CCMD_VOBJ_MOVE,
    CCMD_VOBJ_GLOBAL_INDEX,
    /* Help */
    CCMD_HELP_ABOUT = 50,
    CCMD_HELP_GUIDE
};

typedef struct {
    char label[64];
    char shortcut[16];
    int cmd_id;
    BOOL is_separator;
    BOOL is_checked;
    BOOL enabled;
} CabMenuItem;

typedef struct {
    const char *title;
    RECT rect;
    int item_count;
    CabMenuItem items[10];
} CabMenuHeader;

static CabMenuHeader s_cab_headers[CMENU_HDR_COUNT] = {
    {
        .title = "ファイル(F)",
        .rect = { 4, 0, 108, 21 },
        .item_count = 7,
        .items = {
            { "実身を開く (Open)", "Enter", CCMD_FILE_OPEN, FALSE, FALSE, TRUE },
            { "実身を閲覧 (View)", "Space", CCMD_FILE_VIEW_TAD, FALSE, FALSE, TRUE },
            { "新規実身の作成 (New)", "Ctrl+N", CCMD_FILE_NEW, FALSE, FALSE, TRUE },
            { "---", "", CCMD_NONE, TRUE, FALSE, FALSE },
            { "実身の複製 (Duplicate)", "Ctrl+D", CCMD_FILE_DUPLICATE, FALSE, FALSE, TRUE },
            { "---", "", CCMD_NONE, TRUE, FALSE, FALSE },
            { "閉じる (Close)", "Ctrl+W", CCMD_FILE_CLOSE, FALSE, FALSE, TRUE }
        }
    },
    {
        .title = "編集(E)",
        .rect = { 114, 0, 186, 21 },
        .item_count = 5,
        .items = {
            { "すべて選択 (Select All)", "Ctrl+A", CCMD_EDIT_SELECT_ALL, FALSE, FALSE, TRUE },
            { "選択解除 (Deselect)", "Esc", CCMD_EDIT_DESELECT, FALSE, FALSE, TRUE },
            { "---", "", CCMD_NONE, TRUE, FALSE, FALSE },
            { "名称変更 (Rename)", "F2", CCMD_EDIT_RENAME, FALSE, FALSE, TRUE },
            { "属性 (Properties...)", "Alt+Enter", CCMD_EDIT_PROPERTIES, FALSE, FALSE, TRUE }
        }
    },
    {
        .title = "表示(V)",
        .rect = { 192, 0, 264, 21 },
        .item_count = 8,
        .items = {
            { "一覧表示 (List View)", "Ctrl+1", CCMD_VIEW_LIST, FALSE, FALSE, TRUE },
            { "アイコン表示 (Icon View)", "Ctrl+2", CCMD_VIEW_GRID, FALSE, FALSE, TRUE },
            { "---", "", CCMD_NONE, TRUE, FALSE, FALSE },
            { "名前順で整列 (Sort Name)", "F6", CCMD_VIEW_SORT_NAME, FALSE, FALSE, TRUE },
            { "日付順で整列 (Sort Date)", "F7", CCMD_VIEW_SORT_DATE, FALSE, FALSE, TRUE },
            { "サイズ順で整列 (Sort Size)", "F8", CCMD_VIEW_SORT_SIZE, FALSE, FALSE, TRUE },
            { "種類順で整列 (Sort Type)", "F9", CCMD_VIEW_SORT_TYPE, FALSE, FALSE, TRUE },
            { "再走査・更新 (Rescan)", "F5", CCMD_VIEW_REFRESH, FALSE, FALSE, TRUE }
        }
    },
    {
        .title = "仮身(O)",
        .rect = { 270, 0, 342, 21 },
        .item_count = 3,
        .items = {
            { "仮身リンク作成 (New Link)", "Ctrl+L", CCMD_VOBJ_CREATE_LINK, FALSE, FALSE, TRUE },
            { "キャビネット移動 (Move...)", "", CCMD_VOBJ_MOVE, FALSE, FALSE, TRUE },
            { "総索引 (Global Index)", "Ctrl+I", CCMD_VOBJ_GLOBAL_INDEX, FALSE, FALSE, TRUE }
        }
    },
    {
        .title = "ヘルプ(H)",
        .rect = { 348, 0, 436, 21 },
        .item_count = 2,
        .items = {
            { "キャビネット について (About)", "", CCMD_HELP_ABOUT, FALSE, FALSE, TRUE },
            { "実身・仮身モデル解説 (Guide)", "", CCMD_HELP_GUIDE, FALSE, FALSE, TRUE }
        }
    }
};

static void draw_cab_menu_separator_v(GDEV *dev, H x, H y1, H y2) {
    if (!dev) return;
    RECT s1 = { x, y1, x + 1, y2 };
    RECT s2 = { x + 1, y1, x + 2, y2 };
    fill_rec(dev, &s1, COLOR_DKGRAY);
    fill_rec(dev, &s2, COLOR_WHITE);
}

/* ── Nano About Box for Cabinet ────────────────────────────────────── */
static void paint_vobj_about_window(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_LTGRAY);
    drw_rec(dev, &r);

    RECT card = { 6, 6, dev->width - 6, dev->height - 34 };
    fill_rec(dev, &card, COLOR_WHITE);
    drw_rec(dev, &card);

    drw_tc_string(dev, 16, 12, "Cabinet 3.20 (実身・仮身)", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, 16, 30, "Cleanroom BTRON Object Manager", COLOR_DKGRAY, 0x00000000);
    drw_tc_string(dev, 16, 50, "Brought to B-System by 5HT", COLOR_BLACK, 0x00000000);

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

static void handle_vobj_about_event(WND *wnd, const EVT *evt) {
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
        if (evt->key == BTRON_KEY_ESCAPE || evt->key == 0x0D || evt->key == ' ') {
            cls_wnd(wnd);
        }
    }
}

WND* open_vobj_about_window(void) {
    WND *wnd = opn_wnd("About Cabinet (バージョン情報)", 280, 180, 280, 135,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_vobj_about_window;
        wnd->event_handler = handle_vobj_about_event;
    }
    return wnd;
}

static void paint_vobj_manager(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;

    /* Background */
    RECT r = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &r, COLOR_WHITE);

    /* ── 1. In-Window Application Menu Bar (y = 0..21) ────────────────────────── */
    RECT mbar_r = { 0, 0, dev->width, 21 };
    fill_rec(dev, &mbar_r, COLOR_LTGRAY);
    drw_lin(dev, 0, 21, dev->width, 21);

    for (int h = 0; h < CMENU_HDR_COUNT; h++) {
        const CabMenuHeader *hdr = &s_cab_headers[h];
        RECT hr = hdr->rect;
        BOOL is_active = (g_cabinet.active_menu == h);
        BOOL is_hover = (g_cabinet.hover_menu == h);

        if (is_active) {
            fill_rec(dev, &hr, COLOR_NAVY);
            drw_tc_string(dev, hr.left + 8, hr.top + 3, hdr->title, COLOR_WHITE, 0x00000000);
        } else if (is_hover) {
            fill_rec(dev, &hr, COLOR_WHITE);
            drw_rec(dev, &hr);
            drw_tc_string(dev, hr.left + 8, hr.top + 3, hdr->title, COLOR_NAVY, 0x00000000);
        } else {
            drw_tc_string(dev, hr.left + 8, hr.top + 3, hdr->title, COLOR_BLACK, 0x00000000);
        }

        if (h < CMENU_HDR_COUNT - 1) {
            H sep_x = (hdr->rect.right + s_cab_headers[h + 1].rect.left) / 2;
            draw_cab_menu_separator_v(dev, sep_x, 3, 19);
        }
    }
    draw_cab_menu_separator_v(dev, s_cab_headers[CMENU_HDR_COUNT - 1].rect.right + 3, 3, 19);

    /* Quick View Mode Toggle Button on right */
    if (dev->width >= 560) {
        RECT view_btn = { (H)(s_cab_headers[CMENU_HDR_COUNT - 1].rect.right + 8), 1,
                          (H)(s_cab_headers[CMENU_HDR_COUNT - 1].rect.right + 110), 20 };
        fill_rec(dev, &view_btn, COLOR_WHITE);
        drw_rec(dev, &view_btn);
        const char *v_lbl = (g_cabinet.view_mode == CAB_VIEW_LIST) ? "[一覧表示]" : "[アイコン表示]";
        drw_tc_string(dev, view_btn.left + 8, view_btn.top + 2, v_lbl, COLOR_BLUE, 0x00000000);
    }

    /* Right Margin: Item Count Status */
    char count_buf[32];
    snprintf(count_buf, sizeof(count_buf), "%d 実身", g_cabinet.item_count);
    int cb_w = tc_calc_string_width(count_buf, (int)strlen(count_buf));
    drw_tc_string(dev, dev->width - cb_w - 12, 3, count_buf, COLOR_DKGRAY, 0x00000000);

    /* ── 2. Content Viewport (starts at y = 26) ────────────────────────────── */
    int start_y = 26;
    int visible_rows = (dev->height - 52) / 22;
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

        /* Col 1: Icon Tag */
        drw_tc_string(dev, 14, item_y, it->icon_tag, txt_col, 0x00000000);

        /* Col 2: TOC Hierarchy Path */
        drw_tc_string(dev, 115, item_y, it->category, txt_col, 0x00000000);

        /* Col 3: Real Body ID */
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

    /* ── 3. Scrollbar Indicator (Right Margin) ─────────────────────────────── */
    if (g_cabinet.item_count > visible_rows) {
        int sb_x = dev->width - 16;
        RECT sb_track = { sb_x, 26, dev->width - 4, dev->height - 24 };
        fill_rec(dev, &sb_track, COLOR_LTGRAY);
        drw_rec(dev, &sb_track);

        int track_h = (dev->height - 50);
        int thumb_h = (visible_rows * track_h) / g_cabinet.item_count;
        if (thumb_h < 15) thumb_h = 15;
        int thumb_y = 26 + (g_cabinet.scroll_offset * (track_h - thumb_h)) / max_scroll;

        RECT sb_thumb = { sb_x + 1, thumb_y, dev->width - 5, thumb_y + thumb_h };
        fill_rec(dev, &sb_thumb, COLOR_DKGRAY);
        drw_rec(dev, &sb_thumb);
    }

    /* ── 4. Status Bar Footer (Bottom: height-22 .. height) ────────────────── */
    RECT status_r = { 0, dev->height - 22, dev->width, dev->height };
    fill_rec(dev, &status_r, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 22, dev->width, dev->height - 22);

    char foot_text[128];
    snprintf(foot_text, sizeof(foot_text), "キャビネット: %d 実身 [tad_bin] | #%d: %s (%u B)",
             g_cabinet.item_count,
             g_cabinet.items[g_cabinet.selected_idx].robj_id,
             g_cabinet.items[g_cabinet.selected_idx].name,
             g_cabinet.items[g_cabinet.selected_idx].size_bytes);
    drw_tc_string(dev, 10, dev->height - 17, foot_text, COLOR_BLACK, 0x00000000);

    /* ── 5. Dropdown Menu Overlay ──────────────────────────────────────────── */
    if (g_cabinet.active_menu >= 0 && g_cabinet.active_menu < CMENU_HDR_COUNT) {
        const CabMenuHeader *hdr = &s_cab_headers[g_cabinet.active_menu];
        H menu_x = hdr->rect.left;
        H menu_y = 22;
        H menu_w = CMENU_DROPDOWN_WIDTH;
        H menu_h = hdr->item_count * CMENU_ROW_HEIGHT + 6;

        RECT mr = { menu_x, menu_y, menu_x + menu_w, menu_y + menu_h };
        RECT shadow = { menu_x + 3, menu_y + 3, menu_x + menu_w + 3, menu_y + menu_h + 3 };
        fill_rec(dev, &shadow, COLOR_DKGRAY);

        fill_rec(dev, &mr, COLOR_WHITE);
        drw_rec(dev, &mr);
        drw_lin(dev, mr.left + 1, mr.top + 1, mr.right - 2, mr.top + 1);
        drw_lin(dev, mr.left + 1, mr.top + 1, mr.left + 1, mr.bottom - 2);

        for (int i = 0; i < hdr->item_count; i++) {
            const CabMenuItem *it = &hdr->items[i];
            RECT ir = { menu_x + 3, menu_y + 3 + i * CMENU_ROW_HEIGHT,
                        menu_x + menu_w - 3, menu_y + 3 + (i + 1) * CMENU_ROW_HEIGHT };

            if (it->is_separator) {
                H sep_y = (ir.top + ir.bottom) / 2;
                drw_lin(dev, ir.left + 4, sep_y, ir.right - 4, sep_y);
                continue;
            }

            BOOL is_hov = (g_cabinet.hover_item == i);
            if (is_hov) fill_rec(dev, &ir, COLOR_NAVY);
            COLOR txt_col = is_hov ? COLOR_WHITE : (it->enabled ? COLOR_BLACK : COLOR_GRAY);

            drw_tc_string(dev, ir.left + 8, ir.top + 2, it->label, txt_col, 0x00000000);
            if (it->shortcut[0]) {
                int sc_w = tc_calc_string_width(it->shortcut, (int)strlen(it->shortcut));
                drw_tc_string(dev, ir.right - sc_w - 10, ir.top + 2, it->shortcut, txt_col, 0x00000000);
            }
        }
    }
}

static void handle_vobj_manager_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    H rel_x = evt->pos.x - (wnd->bounds.left + 4);
    H rel_y = evt->pos.y - (wnd->bounds.top + 26);

    if (evt->type == EV_MOUSE_MOVE) {
        /* 1. Header Hover in closed state (y = 0..21) */
        if (g_cabinet.active_menu == -1) {
            g_cabinet.hover_menu = -1;
            if (rel_y >= 0 && rel_y <= 21) {
                for (int h = 0; h < CMENU_HDR_COUNT; h++) {
                    if (rel_x >= s_cab_headers[h].rect.left && rel_x <= s_cab_headers[h].rect.right) {
                        g_cabinet.hover_menu = h;
                        break;
                    }
                }
            }
        }
        /* 2. Hot header tracking and dropdown item tracking when active */
        else {
            if (rel_y >= 0 && rel_y <= 21) {
                for (int h = 0; h < CMENU_HDR_COUNT; h++) {
                    if (rel_x >= s_cab_headers[h].rect.left && rel_x <= s_cab_headers[h].rect.right) {
                        if (g_cabinet.active_menu != h) {
                            g_cabinet.active_menu = h;
                            g_cabinet.hover_menu = h;
                            g_cabinet.hover_item = -1;
                        }
                        return;
                    }
                }
            }

            const CabMenuHeader *hdr = &s_cab_headers[g_cabinet.active_menu];
            H menu_x = hdr->rect.left;
            H menu_y = 22;
            H menu_w = CMENU_DROPDOWN_WIDTH;
            H menu_h = hdr->item_count * CMENU_ROW_HEIGHT + 6;

            if (rel_x >= menu_x && rel_x <= menu_x + menu_w && rel_y >= menu_y && rel_y <= menu_y + menu_h) {
                int item_idx = (rel_y - (menu_y + 3)) / CMENU_ROW_HEIGHT;
                if (item_idx >= 0 && item_idx < hdr->item_count) {
                    if (!hdr->items[item_idx].is_separator && hdr->items[item_idx].enabled) {
                        g_cabinet.hover_item = item_idx;
                    } else {
                        g_cabinet.hover_item = -1;
                    }
                }
            } else {
                g_cabinet.hover_item = -1;
            }
            return;
        }

        /* 3. List Item Hover (starts at y = 26) */
        int start_y = 26;
        int row = (rel_y - start_y) / 22;
        int idx = g_cabinet.scroll_offset + row;
        if (idx >= 0 && idx < g_cabinet.item_count) {
            g_cabinet.hovered_idx = idx;
        } else {
            g_cabinet.hovered_idx = -1;
        }
        return;
    }

    if (evt->type == EV_BUT_DOWN) {
        /* A. Menu Bar Header Clicks (y = 0..21) */
        if (rel_y >= 0 && rel_y <= 21) {
            for (int h = 0; h < CMENU_HDR_COUNT; h++) {
                if (rel_x >= s_cab_headers[h].rect.left && rel_x <= s_cab_headers[h].rect.right) {
                    if (g_cabinet.active_menu == h) {
                        g_cabinet.active_menu = -1;
                        g_cabinet.hover_item = -1;
                    } else {
                        g_cabinet.active_menu = h;
                        g_cabinet.hover_menu = h;
                        g_cabinet.hover_item = -1;
                    }
                    return;
                }
            }

            /* Quick toggle button on right of Menu Bar */
            H dev_w = wnd->dev ? wnd->dev->width : 560;
            if (dev_w >= 560) {
                H btn_l = s_cab_headers[CMENU_HDR_COUNT - 1].rect.right + 8;
                H btn_r = btn_l + 102;
                if (rel_x >= btn_l && rel_x <= btn_r) {
                    g_cabinet.view_mode = (g_cabinet.view_mode == CAB_VIEW_LIST) ? CAB_VIEW_GRID : CAB_VIEW_LIST;
                    return;
                }
            }
        }

        /* B. Dropdown Menu Item Clicks */
        if (g_cabinet.active_menu >= 0) {
            const CabMenuHeader *hdr = &s_cab_headers[g_cabinet.active_menu];
            H menu_x = hdr->rect.left;
            H menu_y = 22;
            H menu_w = CMENU_DROPDOWN_WIDTH;
            H menu_h = hdr->item_count * CMENU_ROW_HEIGHT + 6;

            if (rel_x >= menu_x && rel_x <= menu_x + menu_w && rel_y >= menu_y && rel_y <= menu_y + menu_h) {
                int item_idx = (rel_y - (menu_y + 3)) / CMENU_ROW_HEIGHT;
                if (item_idx >= 0 && item_idx < hdr->item_count) {
                    int cmd = hdr->items[item_idx].cmd_id;
                    g_cabinet.active_menu = -1;
                    g_cabinet.hover_item = -1;

                    switch (cmd) {
                        case CCMD_FILE_OPEN:
                        case CCMD_FILE_VIEW_TAD:
                            if (g_cabinet.selected_idx >= 0 && g_cabinet.selected_idx < g_cabinet.item_count) {
                                open_tad_browser_window(g_cabinet.items[g_cabinet.selected_idx].path,
                                                       g_cabinet.items[g_cabinet.selected_idx].name);
                            }
                            return;
                        case CCMD_FILE_NEW:
                            if (open_t_editor_window) open_t_editor_window();
                            return;
                        case CCMD_FILE_CLOSE:
                            cls_wnd(wnd);
                            return;
                        case CCMD_EDIT_SELECT_ALL:
                            g_cabinet.selected_idx = 0;
                            return;
                        case CCMD_EDIT_DESELECT:
                            g_cabinet.selected_idx = -1;
                            return;
                        case CCMD_VIEW_LIST:
                            g_cabinet.view_mode = CAB_VIEW_LIST;
                            return;
                        case CCMD_VIEW_GRID:
                            g_cabinet.view_mode = CAB_VIEW_GRID;
                            return;
                        case CCMD_VIEW_SORT_NAME:
                            g_cabinet.sort_mode = 0;
                            cabinet_sort_items(&g_cabinet);
                            return;
                        case CCMD_VIEW_SORT_DATE:
                            g_cabinet.sort_mode = 1;
                            cabinet_sort_items(&g_cabinet);
                            return;
                        case CCMD_VIEW_SORT_SIZE:
                            g_cabinet.sort_mode = 2;
                            cabinet_sort_items(&g_cabinet);
                            return;
                        case CCMD_VIEW_SORT_TYPE:
                            g_cabinet.sort_mode = 3;
                            cabinet_sort_items(&g_cabinet);
                            return;
                        case CCMD_VIEW_REFRESH:
                            cabinet_init_defaults(&g_cabinet);
                            return;
                        case CCMD_HELP_ABOUT:
                            open_vobj_about_window();
                            return;
                        default:
                            return;
                    }
                }
            }

            /* Click outside active dropdown closes it */
            g_cabinet.active_menu = -1;
            g_cabinet.hover_item = -1;
            return;
        }

        /* C. Item Selection Click (starts at y = 26) */
        int start_y = 26;
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

    if (evt->type == EV_KEY_DOWN) {
        UW key = evt->key;
        if (key == BTRON_KEY_ESCAPE) {
            if (g_cabinet.active_menu >= 0) {
                g_cabinet.active_menu = -1;
                g_cabinet.hover_item = -1;
                return;
            }
        }

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
                int visible_rows = (wnd->dev ? wnd->dev->height - 52 : 240) / 22;
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
    WND *wnd = opn_wnd("B-System Cabinet Explorer (実身キャビネット)", 80, 60, 560, 360,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_vobj_manager;
        wnd->event_handler = handle_vobj_manager_event;
    }
    return wnd;
}
