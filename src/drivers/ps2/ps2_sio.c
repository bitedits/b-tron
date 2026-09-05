/*
 * ps2_sio.c — Sony PlayStation 2 Cleanroom Debug Serial / Console Output
 *
 * Implements SIO / BIOS syscall output to the PCSX2 console log and hardware UART.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "ps2_sio.h"

extern void ps2_sio_putc_raw(char c);

void ps2_sio_init(void)
{
    /* EE BIOS initializes SIO / TTY automatically */
}

void ps2_sio_putc(char c)
{
    if (c == '\n') {
        ps2_sio_putc_raw('\r');
    }
    ps2_sio_putc_raw(c);
}

void ps2_sio_puts(const char *str)
{
    if (!str) return;
    while (*str) {
        ps2_sio_putc(*str++);
    }
}
