/*
 * mips_qemu.ld — QEMU MIPS Malta / Magnum Bare-Metal Linker Script
 *
 * Cleanroom B-System / BTRON3 3.20 RTOS Kernel Layout
 *
 * Target: QEMU MIPS Malta / Magnum
 * Base: 0x80100000 (KSEG0 Cached, 1 MB into physical RAM)
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x80100000;

    .text : {
        *(.text.startup)
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
    }

    . = ALIGN(128);
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    }

    . = ALIGN(128);
    .data : {
        _gp = ALIGN(16) + 0x7FF0;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.sdata)
        *(.sdata.*)
    }

    . = ALIGN(128);
    __bss_start = .;
    .sbss : {
        *(.sbss)
        *(.sbss.*)
        *(.scommon)
    }
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    . = ALIGN(128);
    _end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
