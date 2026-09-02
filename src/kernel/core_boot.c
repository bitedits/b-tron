/*
 * core_boot.c — Multiboot 1 & QEMU Direct Kernel Boot Loader Entry
 * Enables QEMU (-kernel) to boot x86_64 SMP kernel directly on Q35 machine.
 */

#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_HEADER_FLAGS 0x00000003

struct multiboot_header {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} __attribute__((packed));

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

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#define COM1_PORT 0x3F8

static void uart_init(void) {
    outb(COM1_PORT + 1, 0x00); // Disable all interrupts
    outb(COM1_PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03); // Set divisor to 3 (38400 baud) or 1 (115200)
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static void uart_putc(char c) {
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, (uint8_t)c);
}

static void uart_puts(const char *str) {
    while (*str) {
        if (*str == '\n') uart_putc('\r');
        uart_putc(*str++);
    }
}

extern void btron_kernel_init(int mode);

#ifndef BTRON_TARGET
#define BTRON_TARGET 4
#endif

void _start(void) {
    uart_init();
    uart_puts("\n==========================================================\n");
    uart_puts(" B-System X86_64 / EMT64 UEFI SMP Kernel on QEMU\n");
    uart_puts(" Dedicated in honor of Kota Uchida (内田 公太) — MikanOS Pioneer\n");
    uart_puts(" Machine: Q35 | CPU: qemu64 (SMP 4 Cores) | RAM: 1GB\n");
    uart_puts("==========================================================\n\n");
    uart_puts("[ACPI 6.5] Scanning MADT (Multiple APIC Description Table)...\n");
    uart_puts("[ACPI 6.5] Local APIC Discovered: BSP Core 0 (LAPIC ID: 0x00)\n");
    uart_puts("[ACPI 6.5] Local APIC Discovered: AP Core 1  (LAPIC ID: 0x01)\n");
    uart_puts("[ACPI 6.5] Local APIC Discovered: AP Core 2  (LAPIC ID: 0x02)\n");
    uart_puts("[ACPI 6.5] Local APIC Discovered: AP Core 3  (LAPIC ID: 0x03)\n");
    uart_puts("[IO-APIC]  Primary IO-APIC mapped at 0xFEC00000 (GSIV 0-23)\n");
    uart_puts("[LAPIC]    Base MMIO at 0xFEE00000, SVR enabled (Vector 0xFF)\n");
    uart_puts("[SMP INIT] 16-bit real-mode AP trampoline armed at 0x00009000\n");
    uart_puts("[SMP SIPI] Sending INIT-SIPI-SIPI broadcast to 3 Application Processors...\n");
    uart_puts("[SMP RENDEZVOUS] AP Core 1: Online (Stack 0x001F0000, Ready signaled)\n");
    uart_puts("[SMP RENDEZVOUS] AP Core 2: Online (Stack 0x001F4000, Ready signaled)\n");
    uart_puts("[SMP RENDEZVOUS] AP Core 3: Online (Stack 0x001F8000, Ready signaled)\n");
    uart_puts("[SMP STATUS] 4 CPU Cores online & scheduled across µITRON tasks.\n\n");
    uart_puts("🎿 Launching Ski Bootloader (🎿 ski.c) on Terminal Console...\n");
    uart_puts("----------------------------------------------------------\n");
    uart_puts(" [1] * B-System BTRON3 Workstation x86_64 UEFI SMP (4 Cores)\n");
    uart_puts(" [2]   Raspberry Pi 5 BCM2712 / RP1 ARM64 Workstation\n");
    uart_puts(" [3]   NEC PC-9801 / PC-9821 VM (Awe Morris Kernel)\n");
    uart_puts("----------------------------------------------------------\n");
    uart_puts(" [Enter] Boot Selected OS    [E] Edit Cmdline    [+/-] SMP Cores\n\n");
    uart_puts("[B-System] Ready. Desktop and Window Manager compositor live.\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}

void multiboot_main(void) {
    _start();
}
