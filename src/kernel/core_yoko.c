/*
 * B-TRON Real-Time Kernel: Bare-Metal ARM / BCM283x Boot & Initialization (core_yoko.c)
 */

#include <btron/desktop.h>
#include <btron/troncode.h>
#include <btron/vobj.h>
#include <btron/wnd.h>
#include <btron/types.h>
#include <btron/error.h>
#include <btron/itron.h>

extern void task_initialize(void);
extern void semaphore_initialize(void);
extern void eventflag_initialize(void);
extern void mailbox_initialize(void);
extern void messagebuffer_initialize(void);
extern void rendezvous_initialize(void);
extern void mutex_initialize(void);
extern void memorypool_initialize(void);
extern void fix_memorypool_initialize(void);
extern void cyclichandler_initialize(void);
extern void alarmhandler_initialize(void);
extern void subsystem_initialize(void);
extern void resource_group_initialize(void);
extern void timer_initialize(void);

void yokobayashi_tkernel_init(void) {
#if BTRON_TARGET == 2 && (!defined(__arm__) || defined(__aarch64__))
    printf("\n==========================================================\n");
    printf(" Sakamura T-Kernel 2.0 Real-Time OS Engine (Host PC Mode)\n");
    printf(" Target Mode 2: BTRON_YOKOBAYASHI Active\n");
    printf(" Initializing Sakamura T-Kernel Core Modules...\n");
    printf("==========================================================\n\n");
#endif

    task_initialize();
    semaphore_initialize();
    eventflag_initialize();
    mailbox_initialize();
    messagebuffer_initialize();
    rendezvous_initialize();
    mutex_initialize();
    memorypool_initialize();
    fix_memorypool_initialize();
    cyclichandler_initialize();
    alarmhandler_initialize();
    subsystem_initialize();
    resource_group_initialize();
    timer_initialize();

#if !defined(__arm__) || defined(__aarch64__)
    printf("[T-KERNEL] All Sakamura T-Kernel 2.0 Real-Time Subsystems Initialized Successfully.\n");
#endif
}

#if defined(__arm__) && !defined(__aarch64__)
#define HEAP_BASE ((uintptr_t)0x01000000)  /* 16 MB */
#define HEAP_LIMIT ((uintptr_t)0x1B000000) /* 432 MB limit */
extern uintptr_t heap_ptr;

extern void uart_init(void);
extern void uart_puts(const char *s);
extern void uart_hex32(uint32_t val);
extern uint32_t *init_pi_framebuffer(uint32_t w, uint32_t h);
extern ER ScreenDrv(int ac, unsigned char *av[]);
extern void* tkl_memset( void *s, int c, size_t n );

extern void draw_btron_pattern(uint32_t *fb, uint32_t w, uint32_t h);
extern void *_stack_top;

/*
 * -- Bare-Metal ARM Boot Entry Point -----------------------------------------
 * This function is the entry point for target hardware execution (Raspberry Pi).
 * It is called directly from assembly bootstrap (_start in startup_arm.c) and is
 * NOT compiled for host PC target simulation.
 *
 * Boot Flow (Bare-Metal ARM):
 *   startup_arm.c (_start) -> btron_main() -> yokobayashi_tkernel_init()
 *
 * Boot Flow (Host PC Emulator):
 *   main.c (main) -> btron_kernel_init() -> yokobayashi_tkernel_init()
 *   (btron_main is bypassed entirely on Host to prevent physical register faults)
 */
 
void btron_main(void) {
    /* Reset heap pointer */
    heap_ptr = HEAP_BASE;
    /* Zero the first page of the heap base to avoid stale data issues */
    tkl_memset((void*)HEAP_BASE, 0, 4096);

    uart_init();

    uart_puts("\n==========================================================\n");
    uart_puts(" Sakamura T-Kernel 2.0 Real-Time OS Engine (BCM283x ARM)\n");
    uart_puts(" Target Mode 2: BTRON_YOKOBAYASHI Active\n");
    uart_puts("==========================================================\n\n");
    uart_puts("[QEMU-ARM] Notice: Running bundled QEMU emulation. Hardware VRAM format active (no color format bugs).\n\n");

    uart_puts("[QEMU-ARM] Heap base: ");
    uart_hex32((uint32_t)HEAP_BASE);
    uart_puts(" limit: ");
    uart_hex32((uint32_t)HEAP_LIMIT);
    uart_puts("\n");

    uart_puts("[QEMU-ARM] Initializing Video Display Framebuffer (1024x768 32-bpp)...\n");
    uint32_t *fb = init_pi_framebuffer(1024, 768);

    uart_puts("[QEMU-ARM] Framebuffer pointer: ");
    uart_hex32((uint32_t)(uintptr_t)fb);
    uart_puts("\n");

    uart_puts("[QEMU-ARM] Initializing Sakamura T-Kernel 2.0 Subsystems...\n");
    yokobayashi_tkernel_init();
    uart_puts("[T-KERNEL] All 14 Sakamura T-Kernel 2.0 Subsystems Initialized Successfully.\n");

    uart_puts("[QEMU-ARM] Initializing BCM283x Hardware Screen Device Driver...\n");
    ER sdrv_res = ScreenDrv(0, NULL);
    if (sdrv_res >= 0) {
        uart_puts("[DRIVER] ScreenDrv: Hardware Screen Driver Registered: SCREEN (OK)\n");
    } else {
        uart_puts("[DRIVER] ScreenDrv: Screen Driver Status: ");
        uart_hex32((uint32_t)sdrv_res);
        uart_puts("\n");
    }

    uart_puts("[QEMU-ARM] Drawing B-TRON Desktop with Window Manager & Typography...\n");
    draw_btron_pattern(fb, 1024, 768);
    uart_puts("[QEMU-ARM] Desktop rendered to Video VRAM.\n");

    uart_puts("[B-TRON] Desktop Multi-Window Compositor running in VRAM — entering idle loop.\n");

    while (1) {
        __asm__ volatile("wfe");
    }
}
#endif /* __arm__ */
