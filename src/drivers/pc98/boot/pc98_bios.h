/*
 * pc98_bios.h — NEC PC-98 Hardware Constants & BIOS Interface
 *
 * Covers: NEC PC-9801/PC-9821 series, i386/i486/Pentium/Pentium-II era.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#ifndef _PC98_BIOS_H_
#define _PC98_BIOS_H_

#include <stdint.h>

#define PC98_VRAM_TEXT_BASE      0x000A0000U
#define PC98_VRAM_ATTR_BASE      0x000A2000U
#define PC98_TEXT_COLS           80
#define PC98_TEXT_ROWS           25

#define PC98_IPL_SEGMENT         0x1FE0U
#define PC98_IPL_OFFSET          0x0000U

#define PC98_INT_DISK            0x18
#define PC98_INT_CLOCK           0x1A
#define PC98_INT_TIMER_TICK      0x1C
#define PC98_INT_VIDEO           0x1D
#define PC98_INT_KBD             0x1E
#define PC98_INT_MUX             0x2F

#define PC98_A20_PORT            0x00F2
#define PC98_RTC_INDEX           0x004D
#define PC98_RTC_DATA            0x004F

#define PC98_ATTR_NORMAL         0x00
#define PC98_ATTR_UNDERLINE      0x20
#define PC98_ATTR_REVERSE        0x40
#define PC98_ATTR_COLOR_WHITE    0x07

#ifndef BTRON_TARGET_PC98
#define BTRON_TARGET_PC98        3
#endif

#endif /* _PC98_BIOS_H_ */
