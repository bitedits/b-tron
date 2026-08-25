/*
 * Unified Kernel Initializer for B-System (POSIX & QEMU targets)
 * Manages runtime target_mode selection and VirtIO 1.4 driver initialization.
 */

#include <btron/itron.h>
#include "virtio.h"
#include <stdio.h>

#ifndef BTRON_QEMU_TARGET
extern void btron_posix_kernel_init(void);
#else
extern void tkernel_init(void);
#endif

void btron_kernel_init(int target_mode) {
    if (target_mode == 0) {
        /* Mode 0: POSIX Microkernel Abstraction Mode */
        #ifndef BTRON_QEMU_TARGET
        btron_posix_kernel_init();
        #else
        printf("[KERNEL] Running POSIX emulation inside QEMU image.\n");
        #endif
    } else {
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
    }
}
