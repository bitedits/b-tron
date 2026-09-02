/*
 * boot_arm_stub.c — ARM MMIO Boot ABI Stubs for B-System Ski Bootloader
 *
 * Covers:
 *   • Raspberry Pi 1/2/3/4 — BCM283x Mailbox / VideoCore IV/VI
 *   • Raspberry Pi 5       — BCM2712 Mailbox / RP1 south-bridge
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t *pixels;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    uint32_t  bpp;
} btron_arm_fb_t;

int btron_arm_mailbox_init(uint32_t w, uint32_t h, btron_arm_fb_t *out_fb) {
    if (!out_fb) return -1;
    out_fb->pixels = (uint32_t *)0x3C000000U;
    out_fb->width  = (w > 0) ? w : 1024;
    out_fb->height = (h > 0) ? h : 768;
    out_fb->pitch  = out_fb->width * 4;
    out_fb->bpp    = 32;
    return 0;
}

void btron_arm_uart_init(void) {}
void btron_arm_uart_puts(const char *s) { (void)s; }
void btron_arm_timer_us(uint32_t us) { (void)us; }

int btron_rpi5_mailbox_init(uint32_t w, uint32_t h, btron_arm_fb_t *out_fb) {
    if (!out_fb) return -1;
    out_fb->pixels = (uint32_t *)0x40000000U;
    out_fb->width  = (w > 0) ? w : 1920;
    out_fb->height = (h > 0) ? h : 1080;
    out_fb->pitch  = out_fb->width * 4;
    out_fb->bpp    = 32;
    return 0;
}

void btron_rpi5_rp1_uart_init(void) {}
