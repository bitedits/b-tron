/*
 * Unified Kernel Initializer for B-System (POSIX, QEMU & Sakamura T-Kernel targets)
 * Manages runtime target_mode selection and VirtIO driver initialization based on BTRON_TARGET.
 *
 * Target Modes (BTRON_TARGET):
 *   0: BTRON_POSIX    (B-Kernel POSIX Microkernel Abstraction)
 *   1: BTRON_QEMU     (B-Kernel VirtIO Emulation Mode)
 *   2: BTRON_SAKAMURA (QEMU Raspberry Pi 4 Sakamura T-Kernel 2.0 Engine)
 */

#include <btron/itron.h>
#include <device/virtio.h>
#include <stdio.h>

#ifndef BTRON_TARGET
#define BTRON_TARGET 0
#endif

#if BTRON_TARGET == 0
extern void btron_posix_kernel_init(void);
#elif BTRON_TARGET == 1
extern void tkernel_init(void);
#elif BTRON_TARGET == 2
extern void sakamura_tkernel_init(void);
#endif

void btron_kernel_init(int target_mode) {
    (void)target_mode;

#if BTRON_TARGET == 0
    /* Mode 0: POSIX Microkernel Abstraction Mode */
    btron_posix_kernel_init();
#elif BTRON_TARGET == 1
    /* Mode 1: T-Kernel Bare-Metal / QEMU VirtIO Mode */
    tkernel_init();
    virtio_mmio_init(0x10001000);
    virtio_driver_init_all();
#elif BTRON_TARGET == 2
    /* Mode 2: Unmodified Sakamura T-Kernel 2.0 Real-Time Engine Mode */
    sakamura_tkernel_init();
    virtio_driver_init_all();
#endif
}
