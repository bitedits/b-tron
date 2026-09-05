/*
 * mips_uart.c — Cleanroom NS16550 UART Implementation for QEMU MIPS
 *
 * Implements polled character transmission and reception for serial console.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "mips_uart.h"

/* QEMU Malta / PC ISA UART: KSEG1 0xB80003F8 with stride 1 */
#define ISA_UART_BASE       0xB80003F8UL
#define ISA_REG(r)          (*(volatile uint8_t *)(ISA_UART_BASE + (r)))

#define UART_RBR    0   /* Receiver Buffer Register (read) */
#define UART_THR    0   /* Transmitter Holding Register (write) */
#define UART_IER    1   /* Interrupt Enable Register */
#define UART_FCR    2   /* FIFO Control Register (write) */
#define UART_LCR    3   /* Line Control Register */
#define UART_MCR    4   /* Modem Control Register */
#define UART_LSR    5   /* Line Status Register */

#define LSR_DATA_READY  0x01
#define LSR_TX_EMPTY    0x20

void mips_uart_init(void)
{
    /* Initialize ISA UART COM1 */
    ISA_REG(UART_IER) = 0x00;
    ISA_REG(UART_FCR) = 0x07;
    ISA_REG(UART_LCR) = 0x03;
    ISA_REG(UART_MCR) = 0x03;
}

void mips_uart_putc(char c)
{
    if (c == '\n') {
        mips_uart_putc('\r');
    }

    /* Write directly to Malta ISA COM1 (0xB80003F8) */
    ISA_REG(UART_THR) = (uint8_t)c;
}

void mips_uart_puts(const char *str)
{
    if (!str) return;
    while (*str) {
        mips_uart_putc(*str++);
    }
}

int mips_uart_has_char(void)
{
    return (ISA_REG(UART_LSR) & LSR_DATA_READY) != 0;
}

char mips_uart_getc(void)
{
    while (!mips_uart_has_char()) {
        /* Busy wait */
    }
    return (char)ISA_REG(UART_RBR);
}
