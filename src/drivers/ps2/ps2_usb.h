/*
 * ps2_usb.h — Cleanroom Sony PlayStation 2 USB Host Controller (OHCI) Driver
 *
 * Direct register layout and USB HID Boot Protocol keyboard/mouse decoder.
 * Zero proprietary Sony SDK dependencies.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#ifndef PS2_USB_H
#define PS2_USB_H

#include <stdint.h>
#include <stddef.h>

/* PS2 OHCI MMIO Base (Uncached KSEG1) */
#define OHCI_BASE_ADDR          0xBF801600UL

/* OHCI Register Offsets */
#define OHCI_REG_REVISION       0x00
#define OHCI_REG_CONTROL        0x04
#define OHCI_REG_CMDSTATUS      0x08
#define OHCI_REG_INTSTATUS      0x0C
#define OHCI_REG_INTENABLE      0x10
#define OHCI_REG_INTDISABLE     0x14
#define OHCI_REG_HCCA           0x18
#define OHCI_REG_FMINTERVAL     0x34
#define OHCI_REG_RHDESCRIPTORA  0x48
#define OHCI_REG_RHSTATUS       0x50
#define OHCI_REG_RHPORT1        0x54
#define OHCI_REG_RHPORT2        0x58

/* OHCI Control Register Bits */
#define OHCI_CTRL_HCFS_RESET    (0x00 << 6)
#define OHCI_CTRL_HCFS_RESUME   (0x01 << 6)
#define OHCI_CTRL_HCFS_OPER     (0x02 << 6)
#define OHCI_CTRL_HCFS_SUSPEND  (0x03 << 6)
#define OHCI_CTRL_CLE           (1 << 4)
#define OHCI_CTRL_BLE           (1 << 5)

/* USB Port Status Bits */
#define OHCI_PORT_CCS           (1 << 0)    /* Current Connect Status */
#define OHCI_PORT_PES           (1 << 1)    /* Port Enable Status */
#define OHCI_PORT_PRS           (1 << 4)    /* Port Reset Status */

/* Public API */
void ps2_usb_init(void);
void ps2_usb_poll(void);
void ps2_usb_inject_keyboard(uint8_t mod, uint8_t keycode);
void ps2_usb_inject_mouse(uint8_t buttons, int8_t dx, int8_t dy);

#endif /* PS2_USB_H */
