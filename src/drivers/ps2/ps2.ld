/*
 * ps2.ld — Sony PlayStation 2 Emotion Engine Linker Script
 *
 * Cleanroom B-System / BTRON3 3.20 RTOS Kernel Layout
 *
 * Target: Emotion Engine MIPS R5900 (MIPS-III Little-Endian)
 * Base Address: 0x00100000 (1 MB physical RAM mark)
 * Total Memory: 32 MB RDRAM (0x00000000 - 0x02000000)
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

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
