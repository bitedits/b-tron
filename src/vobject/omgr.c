/*
 * B-System (BTRON 3.20) Real/Virtual Object Manager Implementation: omgr.c
 * Spec: doc/os_spec/shell/omgr.html
 */

#include <btron/omgr.h>
#include <btron/error.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <string.h>
#else
#include <libstr.h>
#endif

typedef struct {
    BOOL in_use;
    VLINK link;
    UW view_mode;
    PNT pos;
    RECT rect;
} VObjSlot;

static VObjSlot g_vobjs[64];
static int g_vid_seq = 1;

VID oreg_vob(VLINK *vlnk, VP vseg, W wid, UW disp) {
    (void)vseg; (void)wid;
    if (!vlnk) return E_PAR;
    for (int i = 0; i < 64; i++) {
        if (!g_vobjs[i].in_use) {
            g_vobjs[i].in_use = TRUE;
            g_vobjs[i].link = *vlnk;
            g_vobjs[i].view_mode = disp;
            g_vobjs[i].pos.x = 10;
            g_vobjs[i].pos.y = 10;
            g_vobjs[i].rect.left = 0;
            g_vobjs[i].rect.top = 0;
            g_vobjs[i].rect.right = 100;
            g_vobjs[i].rect.bottom = 100;
            return i + 1;
        }
    }
    return g_vid_seq++;
}

VID b_odup_vob(W org) {
    (void)org;
    return g_vid_seq++;
}

ERR odsp_vob(W vid, UW disp) {
    int idx = vid - 1;
    if (idx >= 0 && idx < 64 && g_vobjs[idx].in_use) {
        g_vobjs[idx].view_mode = disp;
    }
    return E_OK;
}

VID ofnd_vob(W wid, PNT pos) {
    (void)wid; (void)pos;
    return 1;
}

ERR omov_vob(W vid, PNT pos) {
    int idx = vid - 1;
    if (idx >= 0 && idx < 64 && g_vobjs[idx].in_use) {
        g_vobjs[idx].pos = pos;
    }
    return E_OK;
}

ERR orsz_vob(W vid, RECT *rect) {
    if (!rect) return E_PAR;
    int idx = vid - 1;
    if (idx >= 0 && idx < 64 && g_vobjs[idx].in_use) {
        g_vobjs[idx].rect = *rect;
    }
    return E_OK;
}

VID onew_obj(const TC *name, W type) {
    (void)name; (void)type;
    return g_vid_seq++;
}

VID ocre_obj(const TC *name, W type, W wid) {
    (void)name; (void)type; (void)wid;
    return g_vid_seq++;
}

WERR osta_prc(W vid, W opt) {
    (void)vid; (void)opt;
    return 1;
}

ERR oend_prc(W vid) {
    (void)vid;
    return E_OK;
}

WERR oput_dat(W vid, VP data, W len) {
    (void)vid; (void)data;
    return len;
}

ERR oupd_fil(W vid) {
    (void)vid;
    return E_OK;
}

W odel_vob(W vid, W clr) {
    int idx = vid - 1;
    if (idx >= 0 && idx < 64) {
        g_vobjs[idx].in_use = FALSE;
    }
    (void)clr;
    return 1;
}

ERR adsp_sel(GID gid, SEL_RGN *selp, W mode) {
    (void)gid; (void)selp; (void)mode;
    return E_OK;
}

W achg_bgc(W *mask, COLOR *color, COLOR bgc) {
    (void)mask; (void)color; (void)bgc;
    return 0;
}
