/*
 * boot_pc98.c — NEC PC-98 Real-Mode Boot Stub & 32-bit PM Entry
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <stdint.h>
#include <stddef.h>
#include "pc98_bios.h"

static uint8_t s_dummy_vram[PC98_TEXT_COLS * PC98_TEXT_ROWS * 2];

void btron_pc98_vram_putchar(uint8_t col, uint8_t row, uint8_t ch, uint8_t attr) {
    if (col >= PC98_TEXT_COLS || row >= PC98_TEXT_ROWS) return;
    uint32_t off = (uint32_t)row * PC98_TEXT_COLS * 2 + (uint32_t)col * 2;
    s_dummy_vram[off] = ch;
    s_dummy_vram[off + 1] = attr;
}

void btron_pc98_vram_puts(uint8_t col, uint8_t row, const char *s, uint8_t attr) {
    while (*s && col < PC98_TEXT_COLS) {
        btron_pc98_vram_putchar(col++, row, (uint8_t)*s++, attr);
    }
}

void btron_pc98_splash(void) {
    btron_pc98_vram_puts(2, 2, "B-System Ski Bootloader v1.0 [NEC PC-98]", PC98_ATTR_REVERSE);
    btron_pc98_vram_puts(2, 4, "Architecture: NEC PC-9801 / PC-9821 i386+", PC98_ATTR_NORMAL);
    btron_pc98_vram_puts(2, 6, "A20: Port 0xF2  RTC: RP5C15  INT: 1Ah/2Fh", PC98_ATTR_NORMAL);
}

void btron_pc98_a20_enable(void) {
#if defined(__i386__) && !defined(__STDC_HOSTED__)
    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)0x04), "Nd"((uint16_t)PC98_A20_PORT));
#endif
}

void btron_pc98_pm32_entry(void) {
    btron_pc98_a20_enable();
    btron_pc98_splash();
}
