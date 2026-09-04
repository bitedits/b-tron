/*
 * BTRON 3.20 Font Manager (Менеджер шрифтів) Reference Stubs
 * src/font/font_mgr.c
 *
 * Implements linkable stubs returning E_NOSPT (-17) for unimplemeted APIs.
 */

#include <btron/font_mgr.h>
#include <btron/error.h>

WERR fdef_fnt(FLOC loc, W spec)
{
    (void)loc; (void)spec;
    return E_NOSPT;
}

ERR fdel_fnt(W fid)
{
    (void)fid;
    return E_NOSPT;
}

ERR fdel_loc(FLOC loc, W spec)
{
    (void)loc; (void)spec;
    return E_NOSPT;
}

ERR fget_def(W fid, FDEF *fdef)
{
    (void)fid; (void)fdef;
    return E_NOSPT;
}

WERR fget_not(W fid, TC *note, W size)
{
    (void)fid; (void)note; (void)size;
    return E_NOSPT;
}

WERR flst_fon(W target, FLIST *list, W size)
{
    (void)target; (void)list; (void)size;
    return E_NOSPT;
}

WERR fopn_fon(void)
{
    return E_NOSPT;
}

ERR fcls_fon(W fdesc)
{
    (void)fdesc;
    return E_NOSPT;
}

ERR fset_fon(W fdesc, FSSPEC *spec)
{
    (void)fdesc; (void)spec;
    return E_NOSPT;
}

ERR fget_fon(W fdesc, FSSPEC *spec)
{
    (void)fdesc; (void)spec;
    return E_NOSPT;
}

ERR fset_ang(W fdesc, W ang)
{
    (void)fdesc; (void)ang;
    return E_NOSPT;
}

WERR fget_ang(W fdesc)
{
    (void)fdesc;
    return E_NOSPT;
}

WERR fget_fam(W fdesc, W script, FNTINFO *inf)
{
    (void)fdesc; (void)script; (void)inf;
    return E_NOSPT;
}

WERR fget_img(W fdesc, FDATA *cimg, W size, W script, TC ch, UW mode)
{
    (void)fdesc; (void)cimg; (void)size; (void)script; (void)ch; (void)mode;
    return E_NOSPT;
}
