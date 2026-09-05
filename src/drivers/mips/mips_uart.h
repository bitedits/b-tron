/*
 * mips_uart.h — Cleanroom NS16550 UART Driver for QEMU MIPS (Malta & Magnum)
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#ifndef MIPS_UART_H
#define MIPS_UART_H

#include <stdint.h>
#include <stddef.h>

/* QEMU Malta 16550 UART COM1 (KSEG1 Uncached Base) */
#define MALTA_UART_BASE         0xB80003F8UL

/* QEMU Magnum (Jazz) 16550 UART */
#define MAGNUM_UART_BASE        0x90006000UL

void mips_uart_init(void);
void mips_uart_putc(char c);
void mips_uart_puts(const char *str);
int  mips_uart_has_char(void);
char mips_uart_getc(void);

#endif /* MIPS_UART_H */
