/*
 * B-TRON Real Object / Virtual Object Hyper-Data Model Engine: vobj.c
 */

#include <btron/vobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROBJS 64

static ROBJ g_robj_table[MAX_ROBJS];
static ID g_next_robj_id = 100;
static char g_storage_root[256] = "./btron_store";

ER init_vobj_sys(const char *storage_root) {
    if (storage_root) {
        strncpy(g_storage_root, storage_root, sizeof(g_storage_root) - 1);
    }
    memset(g_robj_table, 0, sizeof(g_robj_table));

    /* Initialize standard BTRON Real Objects */
    cre_robj("Cabinet / Main Directory", VOBJ_TYPE_FOLDER);
    ROBJ *r2 = cre_robj("System README.txt", VOBJ_TYPE_TEXT);
    cre_robj("BTRON Terminal Shell", VOBJ_TYPE_TERMINAL);

    if (r2) {
        const char *sample_text = "Welcome to B-TRON Retro OS!\nTRON Architecture by Dr. Ken Sakamura.\nCleanroom C99 + SDL2 Retro Desktop System.";
        wr_vobj_data(r2, sample_text, strlen(sample_text));
    }

    return E_OK;
}

ROBJ* cre_robj(const char *name, VOBJ_TYPE type) {
    for (int i = 0; i < MAX_ROBJS; i++) {
        if (g_robj_table[i].robj_id == 0) {
            g_robj_table[i].robj_id = g_next_robj_id++;
            g_robj_table[i].type = type;
            strncpy(g_robj_table[i].name, name ? name : "Untitled Object", sizeof(g_robj_table[i].name) - 1);
            snprintf(g_robj_table[i].path, sizeof(g_robj_table[i].path), "%s/robj_%d.dat", g_storage_root, g_robj_table[i].robj_id);
            g_robj_table[i].size = 0;
            return &g_robj_table[i];
        }
    }
    return NULL;
}

ROBJ* opn_robj(ID robj_id) {
    for (int i = 0; i < MAX_ROBJS; i++) {
        if (g_robj_table[i].robj_id == robj_id) {
            return &g_robj_table[i];
        }
    }
    return NULL;
}

ER cls_robj(ROBJ *robj) {
    (void)robj;
    return E_OK;
}

VOBJ_LINK* cre_vobj_link(ID target_robj_id, const char *label, H x, H y) {
    VOBJ_LINK *link = (VOBJ_LINK*)calloc(1, sizeof(VOBJ_LINK));
    if (!link) return NULL;

    link->vobj_id = rand() % 10000 + 1;
    link->target_robj = target_robj_id;
    strncpy(link->label, label ? label : "Virtual Link", sizeof(link->label) - 1);
    link->pos.x = x;
    link->pos.y = y;

    return link;
}

ER rd_vobj_data(ROBJ *robj, void *buf, UW len, UW *read_bytes) {
    if (!robj || !buf) return E_PAR;
    /* Static data simulation for clean memory execution */
    static const char *default_doc = "B-TRON Real Object Storage Record v1.0\nJapanese Lineage Cleanroom Subsystem.\nTRON Hyper-Data Model Engine Active.";
    UW sz = strlen(default_doc);
    if (len < sz) sz = len;
    memcpy(buf, default_doc, sz);
    if (read_bytes) *read_bytes = sz;
    return E_OK;
}

ER wr_vobj_data(ROBJ *robj, const void *buf, UW len) {
    if (!robj || !buf) return E_PAR;
    robj->size = len;
    return E_OK;
}
