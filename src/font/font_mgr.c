/*
 * B-System (BTRON 3.20) Font Manager Implementation: font_mgr.c
 * Spec: doc/os_spec/shell/font_mgr.html
 */

#include <btron/font_mgr.h>
#include <btron/troncode.h>
#include <btron/tibetan_fonts.h>
#include <btron/jis_fonts.h>
#include <btron/error.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdlib.h>
#include <string.h>
#else
#include <libstr.h>
#endif

typedef struct {
    BOOL in_use;
    FSSPEC spec;
    W angle;
} FontSession;

static FontSession g_fonts[16];
static int g_font_id_seq = 1;

WERR fopn_fon(void) {
    for (int i = 0; i < 16; i++) {
        if (!g_fonts[i].in_use) {
            g_fonts[i].in_use = TRUE;
            g_fonts[i].spec.size.width = 16;
            g_fonts[i].spec.size.height = 16;
            g_fonts[i].angle = 0;
            return i + 1;
        }
    }
    return E_LIMIT;
}

ERR fcls_fon(W fdesc) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use) return E_PAR;
    g_fonts[idx].in_use = FALSE;
    return E_OK;
}

WERR fdef_fnt(FLOC loc, W spec) {
    (void)loc; (void)spec;
    return g_font_id_seq++;
}

ERR fdel_fnt(W fid) {
    (void)fid;
    return E_OK;
}

ERR fdel_loc(FLOC loc, W spec) {
    (void)loc; (void)spec;
    return E_OK;
}

ERR fget_def(W fid, FDEF *fdef) {
    if (!fdef) return E_PAR;
    (void)fid;
    memset(fdef, 0, sizeof(FDEF));
    fdef->fclass = FTC_DEFAULT;
    fdef->attr = 0;
    fdef->width = 16;
    fdef->size = 16;
    return E_OK;
}

WERR fget_not(W fid, TC *note, W size) {
    if (!note || size <= 0) return E_PAR;
    (void)fid;
    note[0] = (TC)'B';
    note[1] = (TC)'T';
    note[2] = (TC)'R';
    note[3] = (TC)'O';
    note[4] = (TC)'N';
    note[5] = 0;
    return 5;
}

WERR flst_fon(W target, FLIST *list, W size) {
    (void)target;
    if (!list || size < (W)sizeof(FLIST)) return 0;
    list[0].fid = 1;
    list[0].fclass = FTC_DEFAULT;
    list[0].size = 16;
    list[0].attr = 0;
    return 1;
}

ERR fset_fon(W fdesc, FSSPEC *spec) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use || !spec) return E_PAR;
    g_fonts[idx].spec = *spec;
    return E_OK;
}

ERR fget_fon(W fdesc, FSSPEC *spec) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use || !spec) return E_PAR;
    *spec = g_fonts[idx].spec;
    return E_OK;
}

ERR fset_ang(W fdesc, W ang) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use) return E_PAR;
    g_fonts[idx].angle = ang;
    return E_OK;
}

WERR fget_ang(W fdesc) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use) return E_PAR;
    return g_fonts[idx].angle;
}

WERR fget_fam(W fdesc, W script, FNTINFO *inf) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use || !inf) return E_PAR;
    (void)script;
    memset(inf, 0, sizeof(FNTINFO));
    inf->attr = 0;
    inf->max_width = 16;
    inf->max_height = 16;
    return E_OK;
}

WERR fget_img(W fdesc, FDATA *cimg, W size, W script, TC ch, UW mode) {
    int idx = fdesc - 1;
    if (idx < 0 || idx >= 16 || !g_fonts[idx].in_use || !cimg || size <= 0) return E_PAR;
    (void)script; (void)mode;
    H gw = 8, gh = 16;
    const UB *bmp = get_glyph_bitmap(ch, &gw, &gh);
    if (bmp) {
        int copy_len = (gw <= 8 ? 16 : 32);
        if (copy_len > size) copy_len = size;
        memcpy(cimg, bmp, copy_len);
        return copy_len;
    }
    return 0;
}
