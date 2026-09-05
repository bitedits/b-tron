/*
 * boot_mips.s — Bare-Metal MIPS Startup & Reset Entry
 *
 * Target: QEMU MIPS Malta / Magnum (MIPS32 / MIPS64 Little-Endian)
 * Cleanroom B-System / BTRON3 3.20 RTOS Kernel Entry Point
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

.set noreorder
.set noat

.global _start
.global mips_delay_cycles
.global mips_halt

.extern mips_kernel_main
.extern __bss_start
.extern _end
.extern _gp

/* KSEG0 Stack Top at 32MB mark */
#define MIPS_STACK_TOP      0x81F00000

.section .text.startup, "ax", @progbits
.align 4

_start:
    /* Disable interrupts in CP0 Status */
    mfc0    $k0, $12
    li      $k1, ~0x00000001
    and     $k0, $k0, $k1
    mtc0    $k0, $12
    nop
    nop

    /* Initialize Stack Pointer and Frame Pointer in KSEG0 */
    la      $gp, _gp
    lui     $sp, 0x81F0
    move    $fp, $sp

    /* Clear .bss */
    la      $t0, __bss_start
    la      $t1, _end
    beq     $t0, $t1, 2f
    nop

1:
    sw      $zero, 0($t0)
    sw      $zero, 4($t0)
    sw      $zero, 8($t0)
    sw      $zero, 12($t0)
    addiu   $t0, $t0, 16
    sltu    $t2, $t0, $t1
    bnez    $t2, 1b
    nop

2:
    /* Jump to C kernel entry point */
    jal     mips_kernel_main
    nop

mips_halt:
3:
    wait
    b       3b
    nop

mips_delay_cycles:
4:
    bgtz    $a0, 4b
    subu    $a0, $a0, 1
    jr      $ra
    nop
