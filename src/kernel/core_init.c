/*
 * Unified Kernel Initializer for B-System
 * Dispatches polymorphically to active kernel core (btron_core_init & btron_core_print_ver)
 */

#include <btron/itron.h>
#include <btron/apps.h>
#include <device/virtio.h>
#include <libstr.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#endif

void btron_kernel_init(int target_mode) {
    (void)target_mode;
    btron_core_init();
}

void btron_unified_init(void) {
    btron_core_init();
}

/* ═══════════════════════════════════════════════════════════════════
 * Kernel Info & Hardware Query Interfaces
 * ═══════════════════════════════════════════════════════════════════ */

#if (defined(__arm__) || defined(__aarch64__)) && (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0)
extern uint32_t heap_ptr;
#define K_HEAP_BASE  0x00200000
#define K_HEAP_LIMIT 0x01000000
extern void get_baremetal_mouse_pos(H *x, H *y);
extern void set_baremetal_mouse_pos(H x, H y);
extern void handle_baremetal_mouse_click(GDEV *screen, H x, H y, BOOL press);
#endif

void sys_get_devconf(char *buf, size_t bufsz) {
    if (!buf || bufsz == 0) return;
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    snprintf(buf, bufsz,
        "Registered Device Drivers:\n"
        "  [0] ScreenDrv : VideoCore IV GPU 1024x768 32-bpp (Active, OK)\n"
        "  [1] SerialDrv : PL011 UART0 115200 8N1 (Active, Console)\n"
        "  [2] KBPD      : Keyboard & Pointing Device (Mouse Cursor Active)\n"
        "  [3] TKernel   : 14 Sakamura T-Kernel 2.0 Subsystems (Active)\n"
        "  [4] VObjStore : HyperData HFDS Real Body Storage (Active)");
#else
    tkl_strncpy(buf,
        "Registered Device Drivers:\n"
        "  [0] ScreenDrv : VideoCore IV GPU 1024x768 32-bpp (Active, OK)\n"
        "  [1] SerialDrv : PL011 UART0 115200 8N1 (Active, Console)\n"
        "  [2] KBPD      : Keyboard & Pointing Device (Mouse Cursor Active)\n"
        "  [3] TKernel   : 14 Sakamura T-Kernel 2.0 Subsystems (Active)\n"
        "  [4] VObjStore : HyperData HFDS Real Body Storage (Active)", bufsz - 1);
    buf[bufsz - 1] = '\0';
#endif
}

void sys_get_mem_stats(uint32_t *base, uint32_t *limit, uint32_t *used) {
#if (defined(__arm__) || defined(__aarch64__)) && (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0)
    if (base) *base = K_HEAP_BASE;
    if (limit) *limit = K_HEAP_LIMIT;
    if (used) *used = (heap_ptr >= K_HEAP_BASE) ? (heap_ptr - K_HEAP_BASE) : 0;
#else
    if (base) *base = 0x01000000;
    if (limit) *limit = 0x08000000;
    if (used) *used = 0x00400000;
#endif
}

void sys_mouse_get_pos(H *x, H *y) {
#if (defined(__arm__) || defined(__aarch64__)) && (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0)
    get_baremetal_mouse_pos(x, y);
#else
    if (x) *x = 640;
    if (y) *y = 400;
#endif
}

void sys_mouse_set_pos(H x, H y) {
#if (defined(__arm__) || defined(__aarch64__)) && (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0)
    set_baremetal_mouse_pos(x, y);
#else
    (void)x; (void)y;
#endif
}

void sys_mouse_click(H x, H y) {
#if (defined(__arm__) || defined(__aarch64__)) && (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0)
    set_baremetal_mouse_pos(x, y);
    handle_baremetal_mouse_click(NULL, x, y, TRUE);
    handle_baremetal_mouse_click(NULL, x, y, FALSE);
#else
    (void)x; (void)y;
#endif
}
