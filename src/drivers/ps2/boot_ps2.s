/*
 * boot_ps2.s — Sony PlayStation 2 Emotion Engine (R5900) Startup Code
 *
 * Cleanroom B-System / BTRON3 3.20 RTOS Kernel Entry Point for PS2
 *
 * Target: Emotion Engine MIPS R5900 (MIPS-III Little-Endian)
 * Execution Environment: PCSX2 / Real Hardware
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

.set noreorder
.set noat

.global _start
.global ps2_delay_cycles
.global ps2_halt
.global ps2_sio_putc_raw

.extern ps2_kernel_main
.extern __bss_start
.extern _end
.extern _gp

/* Top of 32MB EE RDRAM minus 64KB headroom for stack */
#define PS2_STACK_TOP       0x01FF0000

.section .text.startup, "ax", @progbits
.align 4

_start:
    /* Disable interrupts in Coprocessor 0 Status register */
    mfc0    $k0, $12            /* CP0 Status */
    li      $k1, ~0x00010001    /* Clear ERL and IE */
    and     $k0, $k0, $k1
    mtc0    $k0, $12
    nop
    nop

    /* Initialize Global Pointer ($gp) and Stack Pointer ($sp) */
    la      $gp, _gp
    lui     $sp, 0x01FF
    move    $fp, $sp

    /* Clear .bss section */
    la      $t0, __bss_start
    la      $t1, _end
    beq     $t0, $t1, 2f
    nop

1:
    sw      $zero, 0($t0)
    addiu   $t0, $t0, 4
    sltu    $t2, $t0, $t1
    bnez    $t2, 1b
    nop

2:
    /* Call C kernel entry point */
    jal     ps2_kernel_main
    nop

/* Infinite halt loop if kernel ever returns */
ps2_halt:
3:
    wait
    b       3b
    nop

/* ps2_delay_cycles(uint32_t count) */
ps2_delay_cycles:
4:
    bgtz    $a0, 4b
    subu    $a0, $a0, 1
    jr      $ra
    nop

/* ps2_sio_putc_raw(char c) via EE BIOS Syscall 0x75 / 0x3d if supported */
ps2_sio_putc_raw:
    /* Syscall 0x75 (printf/sio_putc in PS2 EE BIOS) */
    li      $v1, 0x75
    syscall
    jr      $ra
    nop
