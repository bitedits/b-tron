/*
 *----------------------------------------------------------------------
 *    T-Kernel 2.0 Software Package / B-TRON Extension
 *
 *    Raspberry Pi 2B (BCM2836) VideoCore GPU Screen Driver
 *----------------------------------------------------------------------
 */

#include "screen.h"
#include <device/videomode.h>

#define MBOX_BASE_ARM   0x3F00B880UL
#define MBOX_READ       ((volatile UW*)(MBOX_BASE_ARM + 0x00))
#define MBOX_STATUS     ((volatile UW*)(MBOX_BASE_ARM + 0x18))
#define MBOX_WRITE      ((volatile UW*)(MBOX_BASE_ARM + 0x20))
#define MBOX_FULL       0x80000000UL
#define MBOX_EMPTY      0x40000000UL

static UW s_mbox_buf[36] __attribute__((aligned(16)));

static void mbox_write(UB channel, UW data) {
    while (*MBOX_STATUS & MBOX_FULL) {
        __asm__ volatile("nop");
    }
    __asm__ volatile("dsb sy" ::: "memory");
    *MBOX_WRITE = (data & ~0xFU) | (channel & 0xFU);
}

static UW mbox_read(UB channel) {
    while (1) {
        while (*MBOX_STATUS & MBOX_EMPTY) {
            __asm__ volatile("nop");
        }
        __asm__ volatile("dsb sy" ::: "memory");
        UW val = *MBOX_READ;
        if ((val & 0xFU) == (channel & 0xFU)) {
            return val & ~0xFU;
        }
    }
}

LOCAL void bcm283x_setmode(W flg) {
    (void)flg;
}

LOCAL void bcm283x_suspend(BOOL suspend) {
    (void)suspend;
}

LOCAL void bcm283x_setcmap(COLOR *cmap, W ix, W nent) {
    (void)cmap; (void)ix; (void)nent;
}

EXPORT W getSpecSCRXSPEC(DEV_SPEC *spec, W mode) {
    (void)mode;
    if (spec) {
        spec->hpixels = 1024;
        spec->vpixels = 768;
        spec->pixbits = 32;
        spec->planes = 1;
        spec->attr = DA_HAVEBMP | DA_COLOR_RGB;
    }
    return 1;
}

EXPORT W getSpecSCRLIST(TC *str, W pos) {
    (void)str;
    return pos;
}

IMPORT void *g_pi_fb_ptr;

EXPORT W BCM283xScreenInit(void) {
    UW fb_ptr = 0x3E250000UL;
    if (g_pi_fb_ptr) {
        fb_ptr = (UW)(VW)g_pi_fb_ptr;
    }
    UW fb_size = 1024 * 768 * 4;
    UW pitch = 1024 * 4;

    Vinf.framebuf_addr = (void*)(VW)fb_ptr;
    Vinf.f_addr = (void*)(VW)fb_ptr;
    Vinf.baseaddr = (void*)(VW)fb_ptr;
    Vinf.framebuf_total = fb_size;
    Vinf.framebuf_rowb = pitch;
    Vinf.width = 1024;
    Vinf.height = 768;
    Vinf.act_width = 1024;
    Vinf.act_height = 768;
    Vinf.fb_width = 1024;
    Vinf.fb_height = 768;
    Vinf.pixbits = 32;
    Vinf.pixbyte = 4;
    Vinf.curmode = DMe1024x32;
    Vinf.reqmode = DMe1024x32;
    Vinf.modemap = (1 << DMe1024x32);
    Vinf.attr |= (LINEAR_FRAMEBUF | BPP_24);
    Vinf.fn_setcmap = bcm283x_setcmap;
    Vinf.fn_setmode = bcm283x_setmode;
    Vinf.fn_susres = bcm283x_suspend;

    return 1;
}
