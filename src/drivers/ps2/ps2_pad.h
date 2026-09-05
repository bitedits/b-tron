/*
 * ps2_pad.h — Cleanroom Sony PlayStation 2 DualShock 2 Controller Driver
 *
 * Button bitmasks, analog stick tracking, and B-TRON event translation.
 * Cleanroom implementation with zero proprietary Sony SDK headers.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#ifndef PS2_PAD_H
#define PS2_PAD_H

#include <stdint.h>
#include <stddef.h>

/* DualShock 2 Button Bitmasks (Active-Low: 0 = Pressed, 1 = Released) */
#define PAD_SELECT      0x0001
#define PAD_L3          0x0002
#define PAD_R3          0x0004
#define PAD_START       0x0008
#define PAD_UP          0x0010
#define PAD_RIGHT       0x0020
#define PAD_DOWN        0x0040
#define PAD_LEFT        0x0080
#define PAD_L2          0x0100
#define PAD_R2          0x0200
#define PAD_L1          0x0400
#define PAD_R1          0x0800
#define PAD_TRIANGLE    0x1000
#define PAD_CIRCLE      0x2000
#define PAD_CROSS       0x4000
#define PAD_SQUARE      0x8000

/* Controller State Structure */
typedef struct {
    uint16_t btns;      /* Active-low button word (0xFFFF = none pressed) */
    uint8_t  ljoy_h;    /* Left analog horizontal (0=left, 128=center, 255=right) */
    uint8_t  ljoy_v;    /* Left analog vertical   (0=up,   128=center, 255=down)  */
    uint8_t  rjoy_h;    /* Right analog horizontal */
    uint8_t  rjoy_v;    /* Right analog vertical   */
} ps2_pad_status_t;

/* Public API */
void ps2_pad_init(void);
int  ps2_pad_read(ps2_pad_status_t *status);
void ps2_pad_set_state(uint16_t btns, uint8_t lx, uint8_t ly, uint8_t rx, uint8_t ry);
void ps2_pad_poll(void);

#endif /* PS2_PAD_H */
