/*
 * Unified Kernel Initializer for B-System (POSIX, QEMU & Sakamura T-Kernel targets)
 * Manages runtime target_mode selection and VirtIO driver initialization based on BTRON_TARGET.
 *
 * Target Modes (BTRON_TARGET):
 *   0: BTRON_POSIX       (B-Kernel POSIX Microkernel Abstraction)
 *   1: BTRON_QEMU        (B-Kernel VirtIO Emulation Mode)
 *   2: BTRON_YOKOBAYASHI (QEMU Raspberry Pi 4 Yokobayashi T-Kernel 2.0 Engine)
 *   3: BTRON_SAKAMURA    (UART and VirtIO only kernels)
 */

#include <btron/itron.h>
#include <device/virtio.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#endif

#ifndef BTRON_TARGET
#define BTRON_TARGET 0
#endif

#if BTRON_TARGET == 0
extern void btron_posix_kernel_init(void);
#elif BTRON_TARGET == 1
extern void tkernel_init(void);
#elif BTRON_TARGET == 2
extern void yokobayashi_tkernel_init(void);
#elif BTRON_TARGET == 3
extern void sakamura_tkernel_init(void);
#endif

#if (BTRON_TARGET == 2 || BTRON_TARGET == 3) || ((defined(__arm__) || defined(__aarch64__)) && (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0))
/* Forward declarations for Sakamura T-Kernel 2.0 Subsystem Initializers */
extern ER task_initialize(void);
extern ER semaphore_initialize(void);
extern ER eventflag_initialize(void);
extern ER mailbox_initialize(void);
extern ER messagebuffer_initialize(void);
extern ER rendezvous_initialize(void);
extern ER mutex_initialize(void);
extern ER memorypool_initialize(void);
extern ER fix_memorypool_initialize(void);
extern ER cyclichandler_initialize(void);
extern ER alarmhandler_initialize(void);
extern ER subsystem_initialize(void);
extern ER resource_group_initialize(void);
extern ER timer_initialize(void);

void tkernel_init_subsystems(int full_suite) {
    task_initialize();
    semaphore_initialize();
    eventflag_initialize();
    mailbox_initialize();
    messagebuffer_initialize();
    rendezvous_initialize();
    mutex_initialize();
    memorypool_initialize();
    fix_memorypool_initialize();
    if (full_suite) {
        cyclichandler_initialize();
        alarmhandler_initialize();
        subsystem_initialize();
        resource_group_initialize();
        timer_initialize();
    } else {
        subsystem_initialize();
    }
}
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
    /* Mode 2: Yokobayashi T-Kernel 2.0 Real-Time Engine Mode */
    yokobayashi_tkernel_init();
#elif BTRON_TARGET == 3
    /* Mode 3: Sakamura T-Kernel 2.0 Real-Time Engine Mode (UART & VirtIO only) */
    sakamura_tkernel_init();
    virtio_driver_init_all();
#endif
}
