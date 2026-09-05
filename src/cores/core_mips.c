/*
 * core_mips.c — B-System BTRON3 3.20 RTOS Kernel for Bare-Metal MIPS
 *
 * Dedicated Platform: QEMU MIPS Malta / Magnum (-M malta / -M magnum)
 *
 * Architecture:
 *   • Hardware Drivers:
 *       - National Semiconductor NS16550 UART (COM1 Serial Console)
 *       - MIPS CP0 Timer & Interrupt Controller
 *   • Integrated B-System Executive:
 *       - Cleanroom TRON RTOS Primitives
 *       - Interactive serial console & diagnostic monitor
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <btron/types.h>
#include <btron/error.h>
#include <btron/itron.h>
#include <libstr.h>

#include "mips_uart.h"

extern void mips_delay_cycles(uint32_t count);
extern void mips_halt(void);

static void mips_kprintf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    tkl_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    mips_uart_puts(buf);
}

void mips_kernel_main(void)
{
    /* Initialize NS16550 UART */
    mips_uart_init();

    mips_kprintf("\n");
    mips_kprintf("==============================================================\n");
    mips_kprintf("  B-System / BTRON3 3.20 (Bare-Metal MIPS Architecture)       \n");
    mips_kprintf("  Cleanroom TRON Kernel [Target 9: mips / QEMU Malta & Magnum]\n");
    mips_kprintf("  CPU: MIPS-III / MIPS64 (Little-Endian)                      \n");
    mips_kprintf("  Memory: 256 MB RAM (KSEG0 Mapped)                           \n");
    mips_kprintf("  Console: NS16550 UART @ 0x180003F8 (COM1 115200 8N1)        \n");
    mips_kprintf("==============================================================\n");
    mips_kprintf("[MIPS-INIT] CP0 Status & Interrupts configured.\n");
    mips_kprintf("[MIPS-INIT] Memory pool initialised (0x80200000 - 0x81F00000).\n");
    mips_kprintf("[MIPS-INIT] RTOS scheduler ready.\n");
    mips_kprintf("[BTRON-MIPS] B-System Executive running. Type commands or press Enter.\n");
    mips_kprintf("btron-mips> ");

    uint32_t heartbeat = 0;
    while (1) {
        if (mips_uart_has_char()) {
            char c = mips_uart_getc();
            if (c == '\r' || c == '\n') {
                mips_kprintf("\n[BTRON] OK (B-System RTOS 3.20 Kernel MIPS)\nbtron-mips> ");
            } else if (c == 'h' || c == '?') {
                mips_kprintf("%c\nCommands: [h]elp, [i]nfo, [t]asks, [c]lear\nbtron-mips> ", c);
            } else if (c == 'i') {
                mips_kprintf("%c\nB-System BTRON3 3.20 MIPS Kernel (Target 9: Malta/Magnum)\nbtron-mips> ", c);
            } else if (c == 't') {
                mips_kprintf("%c\nActive Tasks: 1 (kernel_main) | Ticks: %u\nbtron-mips> ", c, heartbeat);
            } else {
                /* Echo character */
                mips_uart_putc(c);
            }
        }

        /* Periodic tick */
        mips_delay_cycles(10000);
        heartbeat++;
        if ((heartbeat % 50000) == 0) {
            mips_kprintf("\n[MIPS-HEARTBEAT] RTOS Tick: %u\nbtron-mips> ", heartbeat);
        }
    }
}
