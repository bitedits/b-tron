/*
 * Multiboot 1 Header & QEMU / Xen PVH ELF Note Entry
 * Enables QEMU (-kernel) direct boot for 32-bit and 64-bit kernels.
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

/* QEMU / Xen PVH ELF Note for 64-bit Direct Boot */
#define XEN_ELFNOTE_PHYS32_ENTRY 18

struct pvh_elfnote {
    uint32_t namesz;
    uint32_t descsz;
    uint32_t type;
    char     name[4];
    uint32_t desc;
};

#if defined(__GNUC__) || defined(__clang__)
__attribute__((section(".note.gnu.property"), used, aligned(4)))
#endif
const struct pvh_elfnote g_pvh_elfnote = {
    4,
    4,
    XEN_ELFNOTE_PHYS32_ENTRY,
    "Xen",
    0x00100000
};

extern void btron_kernel_init(int mode);

#ifndef BTRON_TARGET
#define BTRON_TARGET 0
#endif

void multiboot_main(void) {
    printf("[MULTIBOOT/PVH] QEMU direct kernel boot started.\n");
    btron_kernel_init(BTRON_TARGET);
}
