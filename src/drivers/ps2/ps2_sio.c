/*
 * ps2_sio.c — Sony PlayStation 2 Cleanroom Debug Serial / Console Output
 *
 * Implements SIO / BIOS syscall output to the PCSX2 console log and hardware UART.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "ps2_sio.h"

/* Emotion Engine SIO0 Hardware Registers */
#define SIO_LCR     (*(volatile uint32_t *)0x1000F100)
#define SIO_LSR     (*(volatile uint32_t *)0x1000F110)
#define SIO_IER     (*(volatile uint32_t *)0x1000F120)
#define SIO_ISR     (*(volatile uint32_t *)0x1000F130)
#define SIO_FCR     (*(volatile uint32_t *)0x1000F140)
#define SIO_BGR     (*(volatile uint32_t *)0x1000F150)
#define SIO_TXFIFO  (*(volatile uint8_t  *)0x1000F180)
#define SIO_RXFIFO  (*(volatile uint8_t  *)0x1000F1C0)

void ps2_sio_init(void)
{
    /* Configure SIO0 for 115200 8N1 (CPU clock = 294.912 MHz) */
    SIO_LCR = (1 << 5); /* LCR_SCS_VAL */
    SIO_IER = 0;        /* Disable SIO interrupts */
    SIO_FCR = 0x07;     /* Reset TX/RX FIFOs */
    SIO_FCR = 0;        /* Enable FIFOs */
    SIO_BGR = 9;        /* Baud rate divisor for 115200 */
}

void ps2_sio_putc(char c)
{
    if (c == '\n') {
        while ((SIO_ISR & 0xF000) == 0x8000)
            ;
        SIO_TXFIFO = '\r';
    }

    while ((SIO_ISR & 0xF000) == 0x8000)
        ;

    SIO_TXFIFO = (uint8_t)c;
}

void ps2_sio_puts(const char *str)
{
    if (!str) return;
    while (*str) {
        ps2_sio_putc(*str++);
    }
}

int ps2_sio_has_char(void)
{
    return (SIO_ISR & 0x0F00) ? 1 : 0;
}

int ps2_sio_getc(void)
{
    if (SIO_ISR & 0x0F00) {
        uint8_t b = SIO_RXFIFO;
        SIO_ISR = 7;
        return (int)b;
    }
    return -1;
}
