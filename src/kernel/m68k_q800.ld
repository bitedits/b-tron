/*
 * m68k_q800.ld — Linker Script for BTRON3 Motorola 68040 (Macintosh Quadra 800)
 *
 * Designed for QEMU -M q800 with direct ELF loading (-kernel).
 * RAM Map:
 *   0x00000000 - 0x00000FFF : Low Memory / Hardware Reset Table
 *   0x00010000              : Kernel Entry Point (_start) & Vector Table
 *   0x00010000 - 0x07FFFFFF : Kernel Code, Data, BSS, Stack & RTOS Heap (up to 128MB)
 */

ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x00010000, LENGTH = 127M
}

SECTIONS
{
    . = 0x00010000;

    .text : ALIGN(4) {
        KEEP(*(.vectors))
        *(.text.boot)
        *(.text*)
        *(.rodata*)
    } > RAM

    .data : ALIGN(4) {
        *(.data*)
    } > RAM

    .bss : ALIGN(4) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    .stack : ALIGN(16) {
        __stack_bottom = .;
        . += 0x10000; /* 64 KB Kernel Stack */
        __stack_top = .;
    } > RAM

    /* BTRON RTOS Heap Pool */
    . = ALIGN(4096);
    __heap_start = .;
    __heap_end   = ORIGIN(RAM) + LENGTH(RAM);

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
