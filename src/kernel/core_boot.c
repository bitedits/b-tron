/*
 * Multiboot 1 Header & QEMU Direct Kernel Boot Loader Entry
 * Enables QEMU (-kernel) to bypass SeaBIOS network boot stall
 */

#include <stdint.h>
#include <stdio.h>

#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_HEADER_FLAGS 0x00000003 /* Align modules & provide mem info */

struct multiboot_header {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
};

/* Mach-O vs ELF section attribute formatting */
#if defined(__APPLE__) || defined(__MACH__)
__attribute__((section("__TEXT,__multiboot"), used))
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((section(".multiboot"), used))
#endif
const struct multiboot_header g_multiboot_header = {
    MULTIBOOT_HEADER_MAGIC,
    MULTIBOOT_HEADER_FLAGS,
    (uint32_t)(-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))
};

extern void btron_kernel_init(int mode);

void multiboot_main(void) {
    printf("[MULTIBOOT] QEMU direct kernel boot started.\n");
    btron_kernel_init(BTRON_TARGET);
}
