/*
 * Unified Kernel Initializer for B-System (POSIX, QEMU & Sakamura T-Kernel targets)
 * Manages runtime target_mode selection and VirtIO driver initialization.
 *
 * Target Modes:
 *   0: BTRON_POSIX    (B-Kernel POSIX Microkernel Abstraction)
 *   1: BTRON_QEMU     (B-Kernel VirtIO Emulation Mode)
 *   2: BTRON_SAKAMURA (QEMU Raspberry Pi 4 Sakamura T-Kernel 2.0 Engine)
 */

#include <btron/itron.h>
#include <device/virtio.h>
#include <stdio.h>

#if !defined(BTRON_QEMU_TARGET) && !defined(BTRON_SAKAMURA_TARGET)
extern void btron_posix_kernel_init(void);
#endif

#ifdef BTRON_QEMU_TARGET
extern void tkernel_init(void);
#endif

#ifdef BTRON_SAKAMURA_TARGET
extern void sakamura_tkernel_init(void);
#endif

void btron_kernel_init(int target_mode) {
    if (target_mode == 0) {
        /* Mode 0: POSIX Microkernel Abstraction Mode */
        #if !defined(BTRON_QEMU_TARGET) && !defined(BTRON_SAKAMURA_TARGET)
        btron_posix_kernel_init();
        #else
        printf("[KERNEL] Running POSIX emulation inside QEMU image.\n");
        #endif
    } else if (target_mode == 1) {
        /* Mode 1: T-Kernel Bare-Metal / QEMU VirtIO Mode */
        #ifdef BTRON_QEMU_TARGET
        tkernel_init();
        virtio_mmio_init(0x10001000);
        virtio_driver_init_all();
        #else
        printf("[KERNEL] Initializing T-Kernel VirtIO drivers in POSIX simulation.\n");
        virtio_mmio_init(0x10001000);
        virtio_driver_init_all();
        #endif
    } else if (target_mode == 2) {
        /* Mode 2: Unmodified Sakamura T-Kernel 2.0 Real-Time Engine Mode */
        #ifdef BTRON_SAKAMURA_TARGET
        sakamura_tkernel_init();
        virtio_driver_init_all();
        #else
        printf("[KERNEL] Running Sakamura T-Kernel 2.0 Engine Target.\n");
        #endif
    }
}
