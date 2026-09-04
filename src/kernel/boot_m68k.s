/*
 * boot_m68k.s — Motorola 68040 Low-Level Startup & Vector Table
 *
 * Target: Macintosh Quadra 800 (QEMU -M q800)
 * Cleanroom B-System / BTRON3 3.20 RTOS Kernel Entry.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

.global _start
.global _vector_table
.global m68k_get_sr
.global m68k_set_sr
.global m68k_disable_irq
.global m68k_enable_irq
.global m68k_delay_cycles
.global m68k_halt

.extern m68k_kernel_main
.extern m68k_timer_tick
.extern __bss_start
.extern __bss_end
.extern __stack_top

/* ═══════════════════════════════════════════════════════════════════
 * Vector Table (Motorola 68040 Architecture, 256 x 4-byte vectors)
 * ═══════════════════════════════════════════════════════════════════ */
.section .vectors, "a"
.align 4
_vector_table:
    .long   __stack_top         /* 00: Initial SSP */
    .long   _start              /* 01: Initial PC */
    .long   exc_bus_error       /* 02: Bus Error */
    .long   exc_addr_error      /* 03: Address Error */
    .long   exc_illegal         /* 04: Illegal Instruction */
    .long   exc_zerodiv         /* 05: Zero Divide */
    .long   exc_chk             /* 06: CHK, CHK2 Instruction */
    .long   exc_trapv           /* 07: TRAPV, FTRAPcc, TRAPcc */
    .long   exc_priv            /* 08: Privilege Violation */
    .long   exc_trace           /* 09: Trace */
    .long   exc_line1010        /* 10: Line 1010 Emulator */
    .long   exc_line1111        /* 11: Line 1111 Emulator */
    .long   exc_reserved        /* 12: Reserved */
    .long   exc_coproc          /* 13: Coprocessor Protocol Violation */
    .long   exc_format          /* 14: Format Error */
    .long   exc_uninit_irq      /* 15: Uninitialized Interrupt */
    .space  (24 - 16) * 4, 0    /* 16-23: Reserved */
    .long   exc_spurious        /* 24: Spurious Interrupt */
    .long   irq_level1          /* 25: Level 1 Autovector (VIA1) */
    .long   irq_level2          /* 26: Level 2 Autovector (VIA2 / NuBus) */
    .long   irq_level3          /* 27: Level 3 Autovector */
    .long   irq_level4          /* 28: Level 4 Autovector (SCC Serial) */
    .long   irq_level5          /* 29: Level 5 Autovector */
    .long   irq_level6          /* 30: Level 6 Autovector */
    .long   irq_level7          /* 31: Level 7 Non-Maskable Interrupt (NMI) */
    /* Traps #0 through #15 (Vectors 32-47) */
    .rept   16
    .long   exc_trap
    .endr
    /* User / Peripheral vectors 48-255 */
    .rept   208
    .long   exc_unhandled
    .endr

/* ═══════════════════════════════════════════════════════════════════
 * Kernel Entry Point
 * ═══════════════════════════════════════════════════════════════════ */
.section .text.boot, "ax"
.align 4
_start:
    /* 1. Mask interrupts (IPL = 7) and enter supervisor mode */
    move.w  #0x2700, %sr

    /* 2. Setup Supervisor Stack Pointer (SSP) */
    lea     __stack_top, %sp

    /* 3. Install Vector Base Register (VBR) on 68040 */
    lea     _vector_table, %a0
    movec   %a0, %vbr

    /* 4. Clear BSS Section */
    lea     __bss_start, %a0
    lea     __bss_end, %a1
    cmpa.l  %a0, %a1
    jls     .bss_done
.bss_loop:
    clr.l   (%a0)+
    cmpa.l  %a0, %a1
    jhi     .bss_loop
.bss_done:

    /* 5. Enable 68040 Instruction & Data Caches via CACR */
    /* Bit 31: Enable Instruction Cache, Bit 15: Enable Data Cache */
    move.l  #0x80008000, %d0
    movec   %d0, %cacr

    /* 6. Jump into C99 BTRON3 Kernel Main */
    jsr     m68k_kernel_main

    /* 7. Fallthrough Halt Loop if kernel_main ever returns */
halt_loop:
    stop    #0x2000
    bra     halt_loop

/* ═══════════════════════════════════════════════════════════════════
 * Low-Level Exception & Interrupt Handlers
 * ═══════════════════════════════════════════════════════════════════ */
.section .text, "ax"
.align 4

irq_level1:
    /* VIA1 Timer / ADB interrupt */
    movem.l %d0-%d1/%a0-%a1, -(%sp)
    jsr     m68k_timer_tick
    movem.l (%sp)+, %d0-%d1/%a0-%a1
    rte

irq_level2:
    /* VIA2 / NuBus slot interrupt */
    rte

irq_level3:
    rte

irq_level4:
    /* SCC Serial Interrupt */
    rte

irq_level5:
irq_level6:
    rte

irq_level7:
    /* NMI / Programmer Key */
    rte

exc_bus_error:
exc_addr_error:
exc_illegal:
exc_zerodiv:
exc_chk:
exc_trapv:
exc_priv:
exc_trace:
exc_line1010:
exc_line1111:
exc_coproc:
exc_format:
exc_uninit_irq:
exc_spurious:
exc_trap:
exc_reserved:
exc_unhandled:
    rte

/* ═══════════════════════════════════════════════════════════════════
 * Assembly Utilities for C Kernel
 * ═══════════════════════════════════════════════════════════════════ */

m68k_get_sr:
    move.w  %sr, %d0
    rts

m68k_set_sr:
    move.w  4(%sp), %sr
    rts

m68k_disable_irq:
    move.w  %sr, %d0
    or.w    #0x0700, %sr
    rts

m68k_enable_irq:
    and.w   #0xf8ff, %sr
    rts

m68k_delay_cycles:
    move.l  4(%sp), %d0
.delay_loop:
    subq.l  #1, %d0
    bne     .delay_loop
    rts

m68k_halt:
    stop    #0x2000
    rts
