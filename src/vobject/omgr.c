/*
 * BTRON 3.20 Real and Virtual Object Manager Reference Stubs
 * src/vobject/omgr.c
 *
 * Implements linkable stubs returning E_NOSPT (-17) for Object Manager (§3.8.5 and §3.8.6).
 */

#include <btron/omgr.h>
#include <btron/error.h>

VID oreg_vob(VLINK *vlnk, VP vseg, W wid, UW disp)
{
    (void)vlnk; (void)vseg; (void)wid; (void)disp;
    return E_NOSPT;
}

VID b_odup_vob(W org)
{
    (void)org;
    return E_NOSPT;
}

W odel_vob(W vid, W clr)
{
    (void)vid; (void)clr;
    return E_NOSPT;
}

ERR ocls_wnd(W wid)
{
    (void)wid;
    return E_NOSPT;
}

ERR odsp_vob(W vid, UW disp)
{
    (void)vid; (void)disp;
    return E_NOSPT;
}

ERR odsp_vor(W vid, RECT *r, UW disp)
{
    (void)vid; (void)r; (void)disp;
    return E_NOSPT;
}

ERR odra_vob(W vid, GID gid, UW disp)
{
    (void)vid; (void)gid; (void)disp;
    return E_NOSPT;
}

ERR odra_vor(W vid, RECT *r, GID gid, UW disp)
{
    (void)vid; (void)r; (void)gid; (void)disp;
    return E_NOSPT;
}

VID ofnd_vob(W wid, PNT pos)
{
    (void)wid; (void)pos;
    return E_NOSPT;
}

ERR omov_vob(W vid, PNT pos)
{
    (void)vid; (void)pos;
    return E_NOSPT;
}

ERR orsz_vob(W vid, RECT *rect)
{
    (void)vid; (void)rect;
    return E_NOSPT;
}

ERR ochg_chs(W vid, W chs)
{
    (void)vid; (void)chs;
    return E_NOSPT;
}

ERR ochg_col(W vid, COLOR col)
{
    (void)vid; (void)col;
    return E_NOSPT;
}

ERR ochg_nam(W vid, const TC *name)
{
    (void)vid; (void)name;
    return E_NOSPT;
}

ERR ochg_rel(W vid, const TC *rel)
{
    (void)vid; (void)rel;
    return E_NOSPT;
}

VID onew_obj(const TC *name, W type)
{
    (void)name; (void)type;
    return E_NOSPT;
}

VID ocre_obj(const TC *name, W type, W wid)
{
    (void)name; (void)type; (void)wid;
    return E_NOSPT;
}

ERR odsp_inf(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR odsk_inf(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR oget_vob(W vid, VLINK *vlnk)
{
    (void)vid; (void)vlnk;
    return E_NOSPT;
}

ERR ochg_sts(W vid, UW sts)
{
    (void)vid; (void)sts;
    return E_NOSPT;
}

WERR osta_prc(W vid, W opt)
{
    (void)vid; (void)opt;
    return E_NOSPT;
}

ERR oend_prc(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR oend_req(W vid)
{
    (void)vid;
    return E_NOSPT;
}

WERR oreq_prc(W vid, VP req, W len)
{
    (void)vid; (void)req; (void)len;
    return E_NOSPT;
}

ERR orsp_prc(W vid, VP rsp, W len)
{
    (void)vid; (void)rsp; (void)len;
    return E_NOSPT;
}

WERR oput_dat(W vid, VP data, W len)
{
    (void)vid; (void)data; (void)len;
    return E_NOSPT;
}

ERR oupd_fil(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR oset_tmf(W vid)
{
    (void)vid;
    return E_NOSPT;
}

WERR oget_men(W vid, VP menu)
{
    (void)vid; (void)menu;
    return E_NOSPT;
}

WERR oget_vmn(W vid, VP menu)
{
    (void)vid; (void)menu;
    return E_NOSPT;
}

ERR oexe_vmn(W vid, W item)
{
    (void)vid; (void)item;
    return E_NOSPT;
}

ERR ochg_env(W vid, VP env)
{
    (void)vid; (void)env;
    return E_NOSPT;
}

ERR oget_env(W vid, VP env)
{
    (void)vid; (void)env;
    return E_NOSPT;
}

ERR oreg_apg(const TC *name, W apgid)
{
    (void)name; (void)apgid;
    return E_NOSPT;
}

ERR odel_apg(W apgid)
{
    (void)apgid;
    return E_NOSPT;
}

ERR oset_sea(W sea_id)
{
    (void)sea_id;
    return E_NOSPT;
}

WERR oget_sea(void)
{
    return E_NOSPT;
}

WERR ofnd_apg(const TC *name)
{
    (void)name;
    return E_NOSPT;
}

ERR oexe_apg(W apgid, W vid)
{
    (void)apgid; (void)vid;
    return E_NOSPT;
}

ERR odet_fls(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR ofdt_fls(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR oatt_fls(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR ofat_fls(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR odet_vob(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR oatt_vob(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR oprc_dev(W dev_id)
{
    (void)dev_id;
    return E_NOSPT;
}

ERR ofmt_vob(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR ocnv_vob(W vid, W format)
{
    (void)vid; (void)format;
    return E_NOSPT;
}

VID oopn_obj(W vid)
{
    (void)vid;
    return E_NOSPT;
}

ERR adsp_sel(GID gid, SEL_RGN *selp, W mode)
{
    (void)gid; (void)selp; (void)mode;
    return E_NOSPT;
}

ERR adsp_slt(GID gid, SEL_LIST *selp, W mode, W dh, W dv)
{
    (void)gid; (void)selp; (void)mode; (void)dh; (void)dv;
    return E_NOSPT;
}

W apnl_men(W pnid, W pid, EVT *epv)
{
    (void)pnid; (void)pid; (void)epv;
    return E_NOSPT;
}

W achg_bgc(W *mask, COLOR *color, COLOR bgc)
{
    (void)mask; (void)color; (void)bgc;
    return E_NOSPT;
}
