/*
 * ps2_usb.c — Cleanroom Sony PlayStation 2 USB Host Controller (OHCI) Driver
 *
 * Implements OHCI controller discovery, port reset state machine, and
 * standard USB HID Boot Protocol packet decoding for keyboard and mouse.
 *
 * Cleanroom implementation referencing open specifications in third_party/ps2sdk
 * and ps2tek. Zero proprietary Sony SDK dependencies.
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include "ps2_usb.h"

/* External hooks implemented in core_ps2.c */
extern void ps2_usb_on_key(uint32_t btron_key, int down);
extern void ps2_usb_on_mouse(int dx, int dy, uint8_t buttons);

/* Hardware presence flag */
static int s_usb_available = 0;
static uint8_t s_caps_lock = 0;
static uint8_t s_prev_keys[6] = {0};

#define OHCI_REG(offset) (*(volatile uint32_t *)(OHCI_BASE_ADDR + (offset)))

void ps2_usb_init(void)
{
    /* Read OHCI Revision register */
    volatile uint32_t rev = OHCI_REG(OHCI_REG_REVISION) & 0xFF;
    if (rev == 0x10 || rev == 0x11) {
        s_usb_available = 1;
        /* Put controller in Operational state */
        OHCI_REG(OHCI_REG_CONTROL) = (OHCI_REG(OHCI_REG_CONTROL) & ~0xC0) | OHCI_CTRL_HCFS_OPER;
    } else {
        /* In emulation (PCSX2), OHCI MMIO may not be mapped or disabled by default */
        s_usb_available = 0;
    }
}

void ps2_usb_poll(void)
{
    if (!s_usb_available) return;

    /* Check root hub ports for connection / disconnect events */
    uint32_t p1 = OHCI_REG(OHCI_REG_RHPORT1);
    uint32_t p2 = OHCI_REG(OHCI_REG_RHPORT2);

    /* If port connected and not enabled, issue port reset */
    if ((p1 & OHCI_PORT_CCS) && !(p1 & OHCI_PORT_PES)) {
        OHCI_REG(OHCI_REG_RHPORT1) = OHCI_PORT_PRS;
    }
    if ((p2 & OHCI_PORT_CCS) && !(p2 & OHCI_PORT_PES)) {
        OHCI_REG(OHCI_REG_RHPORT2) = OHCI_PORT_PRS;
    }
}

/* Comprehensive USB HID Keycode to BTRON character mapping */
uint32_t ps2_usb_hid_to_btron_key(uint8_t mod, uint8_t code)
{
    int shift = (mod & 0x22) != 0; /* Left Shift (0x02) or Right Shift (0x20) */

    /* Letters: a - z / A - Z */
    if (code >= 0x04 && code <= 0x1D) {
        int is_upper = shift ^ s_caps_lock;
        char base = is_upper ? 'A' : 'a';
        return (uint32_t)(base + (code - 0x04));
    }

    /* Top Row Numbers: 1 - 9 */
    if (code >= 0x1E && code <= 0x26) {
        const char *num = "123456789";
        const char *sym = "!@#$%^&*(";
        return (uint32_t)(shift ? sym[code - 0x1E] : num[code - 0x1E]);
    }

    /* Top Row 0 */
    if (code == 0x27) return shift ? ')' : '0';

    /* Whitespace and basic editing */
    if (code == 0x28) return 0x0A;  /* Return / Enter */
    if (code == 0x29) return 0x1B;  /* Escape */
    if (code == 0x2A) return 0x08;  /* Backspace */
    if (code == 0x2B) return 0x09;  /* Tab */
    if (code == 0x2C) return ' ';   /* Space */

    /* Punctuation */
    if (code == 0x2D) return shift ? '_' : '-';
    if (code == 0x2E) return shift ? '+' : '=';
    if (code == 0x2F) return shift ? '{' : '[';
    if (code == 0x30) return shift ? '}' : ']';
    if (code == 0x31) return shift ? '|' : '\\';
    if (code == 0x33) return shift ? ':' : ';';
    if (code == 0x34) return shift ? '"' : '\'';
    if (code == 0x35) return shift ? '~' : '`';
    if (code == 0x36) return shift ? '<' : ',';
    if (code == 0x37) return shift ? '>' : '.';
    if (code == 0x38) return shift ? '?' : '/';

    /* Caps Lock Toggle */
    if (code == 0x39) {
        s_caps_lock = !s_caps_lock;
        return 0;
    }

    /* Function Keys F1 - F12 */
    if (code >= 0x3A && code <= 0x45) {
        return BTRON_KEY_F1 + (code - 0x3A);
    }

    /* Navigation & Editing */
    if (code == 0x49) return BTRON_KEY_INSERT;
    if (code == 0x4A) return BTRON_KEY_HOME;
    if (code == 0x4B) return BTRON_KEY_PGUP;
    if (code == 0x4C) return BTRON_KEY_DELETE;
    if (code == 0x4D) return BTRON_KEY_END;
    if (code == 0x4E) return BTRON_KEY_PGDN;

    /* Cursor Arrows */
    if (code == 0x4F) return BTRON_KEY_RIGHT;
    if (code == 0x50) return BTRON_KEY_LEFT;
    if (code == 0x51) return BTRON_KEY_DOWN;
    if (code == 0x52) return BTRON_KEY_UP;

    /* Keypad Operators */
    if (code == 0x54) return '/';
    if (code == 0x55) return '*';
    if (code == 0x56) return '-';
    if (code == 0x57) return '+';
    if (code == 0x58) return 0x0A; /* Keypad Enter */

    /* Keypad Numbers */
    if (code >= 0x59 && code <= 0x61) {
        return (uint32_t)('1' + (code - 0x59));
    }
    if (code == 0x62) return '0';
    if (code == 0x63) return '.';

    /* Japanese Keyboard Specific Keys */
    if (code == 0x88) return BTRON_KEY_HK_TOGGLE; /* Hiragana / Katakana Toggle */
    if (code == 0x8A) return BTRON_KEY_HENKAN;    /* Henkan (Convert) */
    if (code == 0x8B) return BTRON_KEY_MUHENKAN;  /* Muhenkan (Cancel) */

    return 0;
}

/* Process standard 8-byte USB HID keyboard report */
void ps2_usb_process_keyboard_report(const uint8_t report[8])
{
    uint8_t mod = report[0];

    /* 1. Detect newly pressed keys */
    for (int i = 2; i < 8; i++) {
        uint8_t code = report[i];
        if (code == 0) continue;

        int was_down = 0;
        for (int j = 0; j < 6; j++) {
            if (s_prev_keys[j] == code) {
                was_down = 1;
                break;
            }
        }

        if (!was_down) {
            uint32_t key = ps2_usb_hid_to_btron_key(mod, code);
            if (key != 0) {
                ps2_usb_on_key(key, 1);
            }
        }
    }

    /* 2. Detect released keys */
    for (int j = 0; j < 6; j++) {
        uint8_t prev = s_prev_keys[j];
        if (prev == 0) continue;

        int is_down = 0;
        for (int i = 2; i < 8; i++) {
            if (report[i] == prev) {
                is_down = 1;
                break;
            }
        }

        if (!is_down) {
            uint32_t key = ps2_usb_hid_to_btron_key(mod, prev);
            if (key != 0) {
                ps2_usb_on_key(key, 0);
            }
        }
    }

    /* Save current state for next report */
    for (int k = 0; k < 6; k++) {
        s_prev_keys[k] = report[2 + k];
    }
}

/* Process standard 3/4-byte USB HID mouse report */
void ps2_usb_process_mouse_report(const uint8_t report[4])
{
    uint8_t buttons = report[0];
    int8_t dx = (int8_t)report[1];
    int8_t dy = (int8_t)report[2];
    ps2_usb_on_mouse((int)dx, (int)dy, buttons);
}

void ps2_usb_inject_keyboard(uint8_t mod, uint8_t keycode)
{
    uint8_t report[8] = { mod, 0, keycode, 0, 0, 0, 0, 0 };
    ps2_usb_process_keyboard_report(report);

    /* Immediately release to simulate key click */
    uint8_t release_report[8] = { 0 };
    ps2_usb_process_keyboard_report(release_report);
}

void ps2_usb_inject_mouse(uint8_t buttons, int8_t dx, int8_t dy)
{
    ps2_usb_on_mouse((int)dx, (int)dy, buttons);
}
