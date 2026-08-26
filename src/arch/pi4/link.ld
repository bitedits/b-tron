ENTRY(_start)

SECTIONS
{
    . = 0x70000;
    .stack : {
        . = . + 0x10000; /* 64KB dedicated stack region (0x70000 - 0x80000) */
        __stack_top = .;
    }

    . = 0x80000;
    .text : {
        *(.text._start)
        *(.text*)
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
    _end = .;
}
