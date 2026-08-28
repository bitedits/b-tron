/*
 * B-TRON Retro OS — BCM283x DWC2 USB 2.0 Host Controller & HID Driver
 * Cleanroom implementation for Raspberry Pi 2B (BCM2836) and QEMU.
 */

#include <drivers/bcm283x/dwc2.h>
#include <btron/event.h>
#include <stdint.h>
#include <stdbool.h>

#if (!defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0) && (defined(__arm__) || defined(__aarch64__))

extern void uart_puts(const char *s);
extern void uart_hex32(uint32_t val);

static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t dwc2_read(uint32_t reg) {
    return mmio_read32(DWC2_BASE_ADDR + reg);
}

static inline void dwc2_write(uint32_t reg, uint32_t val) {
    mmio_write32(DWC2_BASE_ADDR + reg, val);
}

static bool g_dwc2_initialized = false;
static bool g_kbd_attached = false;
static bool g_mouse_attached = false;
static uint8_t g_last_kbd_key = 0;
static uint8_t g_last_mouse_buttons = 0;

static usb_kbd_report_t g_kbd_dma_buf __attribute__((aligned(16)));
static usb_mouse_report_t g_mouse_dma_buf __attribute__((aligned(16)));
static bool g_kbd_chan_active = false;
static bool g_mouse_chan_active = false;

static void dwc2_queue_kbd_in(void) {
    dwc2_write(DWC2_HCDMA(1), (uint32_t)(uintptr_t)&g_kbd_dma_buf);
    dwc2_write(DWC2_HCTSIZ(1), (1 << 19) | (8 << 0)); /* 1 pkt, 8 bytes, DATA0 */
    /* Ch 1: Ep 1, IN (bit 15), Interrupt (3 << 18), Dev 1 (1 << 22), EpNum 1 (1 << 11), MaxPk 8, ChEnable (1 << 31) */
    uint32_t hcchar = (1U << 31) | (1 << 15) | (3 << 18) | (1 << 22) | (1 << 11) | 8;
    dwc2_write(DWC2_HCCHAR(1), hcchar);
    g_kbd_chan_active = true;
}

static void dwc2_queue_mouse_in(void) {
    dwc2_write(DWC2_HCDMA(2), (uint32_t)(uintptr_t)&g_mouse_dma_buf);
    dwc2_write(DWC2_HCTSIZ(2), (1 << 19) | (4 << 0)); /* 1 pkt, 4 bytes, DATA0 */
    /* Ch 2: Ep 1, IN (bit 15), Interrupt (3 << 18), Dev 2 (2 << 22), EpNum 1 (1 << 11), MaxPk 4, ChEnable (1 << 31) */
    uint32_t hcchar = (1U << 31) | (1 << 15) | (3 << 18) | (2 << 22) | (1 << 11) | 4;
    dwc2_write(DWC2_HCCHAR(2), hcchar);
    g_mouse_chan_active = true;
}

int dwc2_init(void) {
    uart_puts("[QEMU-ARM] Initializing BCM283x DWC2 USB 2.0 Host Controller...\n");

    /* Read Core ID */
    uint32_t snpsid = dwc2_read(0x040); /* GSNPSID */
    uart_puts("[DRIVER] DWC2 Core Hardware ID: ");
    uart_hex32(snpsid);
    uart_puts("\n");

    /* Core Soft Reset */
    dwc2_write(DWC2_GRSTCTL, (1 << 0)); /* Core Soft Reset */
    for (volatile int i = 0; i < 100000; i++) {
        if ((dwc2_read(DWC2_GRSTCTL) & (1 << 0)) == 0) break;
    }

    /* Wait for AHB master idle */
    for (volatile int i = 0; i < 100000; i++) {
        if (dwc2_read(DWC2_GRSTCTL) & (1 << 31)) break;
    }

    /* Configure GUSBCFG: Force Host Mode, PHY Interface, Timeout */
    uint32_t usbcfg = dwc2_read(DWC2_GUSBCFG);
    usbcfg &= ~((1 << 29) | (1 << 30)); /* Clear force modes */
    usbcfg |= (1 << 29);                /* Force Host Mode */
    usbcfg |= (1 << 6);                 /* PHY Select */
    dwc2_write(DWC2_GUSBCFG, usbcfg);

    /* Configure GAHBCFG: Enable DMA and global interrupts */
    uint32_t ahbcfg = dwc2_read(DWC2_GAHBCFG);
    ahbcfg |= (1 << 0) | (1 << 5);      /* Global Interrupt Mask + DMA Enable */
    dwc2_write(DWC2_GAHBCFG, ahbcfg);

    /* Configure Host Clock & Frame Interval */
    dwc2_write(DWC2_HCFG, 0);           /* 30/60 MHz PHY clock */

    /* Power on Host Port 0 */
    uint32_t hprt = dwc2_read(DWC2_HPRT0);
    hprt &= ~((1 << 1) | (1 << 2) | (1 << 3)); /* Clear write-1-to-clear status bits */
    hprt |= (1 << 12);                         /* Port Power ON */
    dwc2_write(DWC2_HPRT0, hprt);

    /* Assert Port Reset to enumerate attached devices */
    hprt = dwc2_read(DWC2_HPRT0);
    hprt &= ~((1 << 1) | (1 << 2) | (1 << 3));
    hprt |= (1 << 8);                          /* Port Reset */
    dwc2_write(DWC2_HPRT0, hprt);
    for (volatile int i = 0; i < 100000; i++) { __asm__ volatile("nop"); }

    /* De-assert Port Reset */
    hprt &= ~(1 << 8);
    dwc2_write(DWC2_HPRT0, hprt);
    for (volatile int i = 0; i < 100000; i++) { __asm__ volatile("nop"); }

    /* Check Port Connection Status */
    uint32_t port_stat = dwc2_read(DWC2_HPRT0);
    if (port_stat & (1 << 0)) {
        uart_puts("[DRIVER] DWC2: USB Root Port Powered & Device Connected (Speed: Full/High)\n");
    } else {
        uart_puts("[DRIVER] DWC2: USB Root Port Powered (Active)\n");
    }

    g_kbd_attached = true;
    g_mouse_attached = true;

    uart_puts("[DRIVER] DWC2: USB HID Keyboard Driver Ready (Ch 1, EP 0x81)\n");
    uart_puts("[DRIVER] DWC2: USB HID Mouse Driver Ready (Ch 2, EP 0x81)\n");

    g_dwc2_initialized = true;

    /* Prime initial IN requests */
    dwc2_queue_kbd_in();
    dwc2_queue_mouse_in();

    return 0;
}

bool dwc2_has_devices(void) {
    return g_dwc2_initialized && (g_kbd_attached || g_mouse_attached);
}

uint32_t dwc2_usb_to_btron_key(uint8_t scancode, uint8_t modifiers) {
    bool shift = (modifiers & 0x22) != 0;

    if (scancode >= 0x04 && scancode <= 0x1D) {
        /* 'a' - 'z' */
        char base = shift ? 'A' : 'a';
        return (uint32_t)(base + (scancode - 0x04));
    }
    if (scancode >= 0x1E && scancode <= 0x26) {
        /* '1' - '9' */
        static const char shift_num[] = "!@#$%^&*(";
        return shift ? (uint32_t)shift_num[scancode - 0x1E] : (uint32_t)('1' + (scancode - 0x1E));
    }
    if (scancode == 0x27) {
        return shift ? ')' : '0';
    }

    switch (scancode) {
        case 0x28: return BTRON_KEY_RETURN;    /* Enter */
        case 0x29: return 0x1B;                /* ESC */
        case 0x2A: return BTRON_KEY_BACKSPACE; /* Backspace */
        case 0x2B: return BTRON_KEY_TAB;       /* Tab */
        case 0x2C: return ' ';                 /* Space */
        case 0x2D: return shift ? '_' : '-';   /* Minus */
        case 0x2E: return shift ? '+' : '=';   /* Equal */
        case 0x2F: return shift ? '{' : '[';   /* Left Bracket */
        case 0x30: return shift ? '}' : ']';   /* Right Bracket */
        case 0x31: return shift ? '|' : '\\';  /* Backslash */
        case 0x33: return shift ? ':' : ';';   /* Semicolon */
        case 0x34: return shift ? '"' : '\'';  /* Quote */
        case 0x35: return shift ? '~' : '`';   /* Grave Accent */
        case 0x36: return shift ? '<' : ',';   /* Comma */
        case 0x37: return shift ? '>' : '.';   /* Dot */
        case 0x38: return shift ? '?' : '/';   /* Slash */
        case 0x4F: return BTRON_KEY_RIGHT;     /* Right Arrow */
        case 0x50: return BTRON_KEY_LEFT;      /* Left Arrow */
        case 0x51: return BTRON_KEY_DOWN;      /* Down Arrow */
        case 0x52: return BTRON_KEY_UP;        /* Up Arrow */
        case 0x3F: return BTRON_KEY_F6;        /* F6 (Hiragana) */
        case 0x40: return BTRON_KEY_F7;        /* F7 (Katakana) */
        case 0x41: return BTRON_KEY_F8;        /* F8 (Halfwidth Katakana) */
        case 0x42: return BTRON_KEY_F9;        /* F9 (Fullwidth Alphanumeric) */
        case 0x43: return BTRON_KEY_F10;       /* F10 (Kana/Kanji Toggle) */
        default: break;
    }
    return 0;
}

int dwc2_poll_keyboard(usb_kbd_report_t *report) {
    if (!g_dwc2_initialized || !report) return -1;

    if (!g_kbd_chan_active) {
        dwc2_queue_kbd_in();
    }

    uint32_t hcint = dwc2_read(DWC2_HCINT(1));
    if (hcint & (1 << 0)) { /* Transfer Completed (XFRC) */
        dwc2_write(DWC2_HCINT(1), 0x7FF); /* Clear all channel interrupts */
        g_kbd_chan_active = false;
        __asm__ volatile("dsb sy" : : : "memory");

        *report = g_kbd_dma_buf;
        uint8_t current_key = report->keys[0];
        if (current_key != 0 && current_key != g_last_kbd_key) {
            g_last_kbd_key = current_key;
            dwc2_queue_kbd_in();
            return 1;
        }
        g_last_kbd_key = current_key;
        dwc2_queue_kbd_in();
    } else if (hcint & ((1 << 1) | (1 << 4))) { /* Channel Halted or NAK */
        dwc2_write(DWC2_HCINT(1), 0x7FF);
        g_kbd_chan_active = false;
        dwc2_queue_kbd_in();
    }
    return 0;
}

int dwc2_poll_mouse(usb_mouse_report_t *report) {
    if (!g_dwc2_initialized || !report) return -1;

    if (!g_mouse_chan_active) {
        dwc2_queue_mouse_in();
    }

    uint32_t hcint = dwc2_read(DWC2_HCINT(2));
    if (hcint & (1 << 0)) { /* Transfer Completed (XFRC) */
        dwc2_write(DWC2_HCINT(2), 0x7FF); /* Clear all channel interrupts */
        g_mouse_chan_active = false;
        __asm__ volatile("dsb sy" : : : "memory");

        *report = g_mouse_dma_buf;
        if (report->dx != 0 || report->dy != 0 || report->buttons != g_last_mouse_buttons) {
            g_last_mouse_buttons = report->buttons;
            dwc2_queue_mouse_in();
            return 1;
        }
        dwc2_queue_mouse_in();
    } else if (hcint & ((1 << 1) | (1 << 4))) { /* Channel Halted or NAK */
        dwc2_write(DWC2_HCINT(2), 0x7FF);
        g_mouse_chan_active = false;
        dwc2_queue_mouse_in();
    }
    return 0;
}

#endif /* Freestanding ARM */
