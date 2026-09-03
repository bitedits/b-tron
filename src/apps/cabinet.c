/*
 * B-System (BTRON 3.20) Cabinet Application (src/apps/cabinet.c)
 * Real Body Cabinet & Virtual Body Explorer Window
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#include <btron/vobj.h>
#include <btron/omgr.h>
#include <btron/event.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

typedef enum {
    CABINET_VIEW_GRID = 0,
    CABINET_VIEW_LIST = 1
} CabinetViewMode;

typedef struct {
    UW robj_id;
    char name[64];
    UW size_bytes;
    UW mime_type;
    UW ref_count;
} CabinetEntry;

typedef struct {
    WND *wnd;
    CabinetViewMode view_mode;
    CabinetEntry entries[128];
    int entry_count;
    int selected_idx;
} CabinetApp;

void cabinet_init(CabinetApp *app) {
    if (!app) return;
    memset(app, 0, sizeof(CabinetApp));
    app->view_mode = CABINET_VIEW_GRID;
    app->selected_idx = -1;
}

int cabinet_add_entry(CabinetApp *app, UW robj_id, const char *name, UW size, UW mime) {
    if (!app || app->entry_count >= 128) return -1;
    CabinetEntry *e = &app->entries[app->entry_count++];
    e->robj_id = robj_id;
    strncpy(e->name, name ? name : "Untitled", sizeof(e->name) - 1);
    e->size_bytes = size;
    e->mime_type = mime;
    e->ref_count = 1;
    return app->entry_count - 1;
}

void cabinet_draw(CabinetApp *app) {
    if (!app || !app->wnd) return;
}

void cabinet_open_selected(CabinetApp *app) {
    if (!app || app->selected_idx < 0 || app->selected_idx >= app->entry_count) return;
}
