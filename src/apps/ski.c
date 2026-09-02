/*
 * ski.c — Ski Bootloader (Bootman Application for B-System)
 *
 * Implements the BTRON / B-System interactive boot manager interface,
 * inspired by synrc/hv monitor/menu.c, Haiku BootManager, and NetBSD quality.
 *
 * Provides:
 *   • Full ANSI terminal UI with Ski 🎿 branding & box drawing
 *   • Boot target selection (BTRON3, T-Kernel, seL4, NetBSD, PC-98, Pi 1-5)
 *   • Inline command line editing ([E] key)
 *   • Multi-core CPU affinity allocation ([+] / [-] keys)
 *   • Non-blocking / testable execution loop
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <stdint.h>
#include <stddef.h>
#include <basic.h>
#include <libstr.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#define SKI_HAS_TERMIOS 1
#else
#define SKI_HAS_TERMIOS 0
#endif

#define SKI_MAX_ITEMS   14
#define SKI_CMDLINE_MAX 128

/* ANSI Escape Sequences */
#define ANSI_CLEAR        "\033[2J\033[H"
#define ANSI_CURSOR_HIDE  "\033[?25l"
#define ANSI_CURSOR_SHOW  "\033[?25h"
#define ANSI_RESET        "\033[0m"
#define ANSI_CYAN         "\033[36m"
#define ANSI_SELECT       "\033[1;37;46m"  /* White text on cyan background */
#define ANSI_YELLOW       "\033[1;33m"
#define ANSI_GREEN        "\033[1;32m"
#define ANSI_MAGENTA      "\033[1;35m"

/* Key Parsing States */
#define KEY_STATE_NORMAL  0
#define KEY_STATE_ESC     1
#define KEY_STATE_BRACKET 2

typedef struct {
    const char *name;
    const char *cart_type;
    const char *target_arch;
    char cmdline[SKI_CMDLINE_MAX];
    int cpu_count;
    int target_id;
} ski_boot_entry_t;

static ski_boot_entry_t s_ski_items[SKI_MAX_ITEMS] = {
    {"B-System BTRON3 Workstation",     "[BTRON Real Body]",   "x86_64 UEFI",  "boot=btron3 vga=1024x768x32 smp=auto root=hfds:/dev/vda", 4, 1},
    {"B-System BTRON3 (Raspberry Pi 5)","[BTRON Real Body]",   "ARM64 Pi5",    "arch=arm64 machine=rpi5 mbox=0x107c013880 rp1=pcie root=ramfs", 4, 2},
    {"B-System BTRON3 (Raspberry Pi 4)","[BTRON Real Body]",   "ARM64 Pi4",    "arch=arm64 machine=rpi4 mbox=0x3f00b880 console=pl011 root=ramfs", 4, 3},
    {"TRON T-Kernel 2.0 (Sakamura)",    "[Japan RTOS Card]",   "ARM32 Pi2/3",  "arch=arm32 machine=bcm283x console=pl011 tick=1ms max_tasks=64", 1, 4},
    {"NEC PC-98 BTRON3 (i386/Pentium)", "[Japan Retro Card]",  "PC-98 i386",   "arch=i386 machine=pc98 vram=0xa0000 bios=int1a a20=portF2", 1, 5},
    {"NEC PC-98 Linux 7.1 i386",        "[UNIX Card]",         "PC-98 i386",   "arch=i386 machine=pc98 console=ttyS0 root=/dev/hda1 mem=16M", 1, 6},
    {"NEC V30 os8088 Real-Mode",        "[Real Mode Card]",    "NEC V30",      "arch=i8086 machine=nec-v30 bios=rom mem=640K console=gdc", 1, 7},
    {"seL4 Microkernel (VirtIO VM)",    "[Microkernel Card]",  "x86_64/ARM64", "net_backend=sddf smp=true root=viocon0 console=hvc0 mem=128M", 2, 8},
    {"NetBSD 11.0 (smolBSD MICROVM)",   "[UNIX Card]",         "x86_64",       "com0=0x09000000 root=viocon0 crypto=opencrypto smp=2 mem=128M", 2, 9},
    {"NuttX 12.0 RTOS (Flat Model)",    "[RTOS Card]",         "RISC-V/ARM",   "console=hvc0 root=/dev/vda1 virtio_mmio.device=0x0a000000 mem=32M", 1, 10},
    {"FreeRTOS (Symmetric Multi-Task)", "[RTOS Card]",         "ARM32",        "console=hvc0 root=viocon0 platform=virt smp=false mem=64M", 1, 11},
    {"Erlang/OTP 20.0 (seL4 Unikernel)","[Unikernel Card]",    "x86_64",       "node=hv@localhost cookie=monitor sched=priority+edf console=hvc0", 2, 12},
    {"OCaml 5.0 Mirage VM (seL4)",      "[Unikernel Card]",    "ARM64",        "net_backend=sddf console=hvc0 ip=192.168.1.50 mem=64M", 1, 13},
    {"T-Monitor Emergency Shell",       "[Emergency Core]",    "Native",       "mode=tmon baud=115200 fallback=step-debug prompt=TM>", 1, 0}
};

static int s_current_selection = 0;
static int s_edit_mode = 0;
static int s_input_state = KEY_STATE_NORMAL;

#if SKI_HAS_TERMIOS
static struct termios s_orig_termios;
static int s_raw_mode_active = 0;

static void ski_enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) return;
    tcgetattr(STDIN_FILENO, &s_orig_termios);
    struct termios raw = s_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    s_raw_mode_active = 1;
}

static void ski_disable_raw_mode(void) {
    if (s_raw_mode_active && isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &s_orig_termios);
        s_raw_mode_active = 0;
    }
}
#else
static void ski_enable_raw_mode(void) {}
static void ski_disable_raw_mode(void) {}
#endif

static int ski_strlen(const char *str) {
    int len = 0;
    if (!str) return 0;
    while (str[len] != '\0') len++;
    return len;
}

extern void uart_puts_raw(const char *str) __attribute__((weak));

static void ski_print(const char *str) {
    if (!str) return;
#if SKI_HAS_TERMIOS
    write(STDOUT_FILENO, str, ski_strlen(str));
#else
    if (uart_puts_raw) uart_puts_raw(str);
#endif
}

static void ski_print_int(int num) {
    char buf[16];
#if SKI_HAS_TERMIOS
    snprintf(buf, sizeof(buf), "%d", num);
#else
    tkl_snprintf(buf, sizeof(buf), "%d", num);
#endif
    ski_print(buf);
}

void ski_draw_interface(void) {
    ski_print(ANSI_CLEAR);
    ski_print(ANSI_CURSOR_HIDE);
    ski_print("\n");

    /* 1. Ski 🎿 ASCII Header */
    ski_print(ANSI_CYAN);
    ski_print("    OS.1 [ARM64/x86_64/PC-98]  \xF0\x9F\x8F\xBF  SKI BOOTLOADER   ___           \n");
    ski_print("   ___ _   _ _ __  _ __ ___   / / |____   __ \n");
    ski_print("  / __| | | | '_ \\| '__/ __| / /| '_ \\ \\ / / \n");
    ski_print("  \\__ \\ |_| | | | | | | (__ / / | | | \\ V /  \n");
    ski_print("  |___/\\__, |_| |_|_|  \\___/_/  |_| |_|\\_/   \n");
    ski_print("       |___/       (B-System Bootman · b-system/ski) \n\n");
    ski_print(ANSI_RESET);

    /* 2. Boot Selection Frame */
    ski_print(ANSI_CYAN);
    ski_print(" ┌────────────────────────────────────────────────────────┬──────────────────────┬─────────────────┐\n");
    for (int i = 0; i < SKI_MAX_ITEMS; i++) {
        ski_print(" │ ");
        if (i == s_current_selection) {
            ski_print(ANSI_SELECT);
            ski_print("--> ");
            ski_print(s_ski_items[i].name);
            int spaces = 50 - ski_strlen(s_ski_items[i].name);
            for (int j = 0; j < spaces; j++) ski_print(" ");
            ski_print(" │ ");
            ski_print(s_ski_items[i].cart_type);
            spaces = 20 - ski_strlen(s_ski_items[i].cart_type);
            for (int j = 0; j < spaces; j++) ski_print(" ");
            ski_print(" │ ");
            ski_print(s_ski_items[i].target_arch);
            spaces = 15 - ski_strlen(s_ski_items[i].target_arch);
            for (int j = 0; j < spaces; j++) ski_print(" ");
            ski_print(ANSI_RESET);
        } else {
            ski_print("    ");
            ski_print(s_ski_items[i].name);
            int spaces = 50 - ski_strlen(s_ski_items[i].name);
            for (int j = 0; j < spaces; j++) ski_print(" ");
            ski_print(" │ ");
            ski_print(s_ski_items[i].cart_type);
            spaces = 20 - ski_strlen(s_ski_items[i].cart_type);
            for (int j = 0; j < spaces; j++) ski_print(" ");
            ski_print(" │ ");
            ski_print(s_ski_items[i].target_arch);
            spaces = 15 - ski_strlen(s_ski_items[i].target_arch);
            for (int j = 0; j < spaces; j++) ski_print(" ");
        }
        ski_print(ANSI_CYAN);
        ski_print(" │\n");
    }
    ski_print(" └────────────────────────────────────────────────────────┴──────────────────────┴─────────────────┘\n");
    ski_print(ANSI_RESET);

    /* 3. Boot Arguments Section */
    ski_print("\n \xF0\x9F\x8F\xBF Boot Arguments (Press [E] to edit, [Enter] to Boot, [Q] to Exit, [S] for T-Monitor):\n");
    ski_print(" > ");
    if (s_edit_mode) {
        ski_print(ANSI_YELLOW);
    }
    ski_print(s_ski_items[s_current_selection].cmdline);
    ski_print(ANSI_RESET);
    if (s_edit_mode) {
        ski_print("\xE2\x96\x88"); /* Solid cursor block */
    }
    ski_print("\n");

    /* 4. Core Allocation Section */
    ski_print(" Allocated Cores: ");
    ski_print_int(s_ski_items[s_current_selection].cpu_count);
    ski_print(" (Press [+] / [-] to adjust SMP core affinity)\n");
    ski_print("\n Use [↑/↓] arrows or [W/S] keys to navigate. [TERMIOS/UART/GOP].\n");
}

int ski_get_selection(void) { return s_current_selection; }
void ski_set_selection(int idx) {
    if (idx >= 0 && idx < SKI_MAX_ITEMS) s_current_selection = idx;
}
const char* ski_get_cmdline(int idx) {
    if (idx >= 0 && idx < SKI_MAX_ITEMS) return s_ski_items[idx].cmdline;
    return NULL;
}
int ski_get_cpu_count(int idx) {
    if (idx >= 0 && idx < SKI_MAX_ITEMS) return s_ski_items[idx].cpu_count;
    return 1;
}

int ski_handle_key(char c) {
    if (s_edit_mode) {
        int len = ski_strlen(s_ski_items[s_current_selection].cmdline);
        if (c == 13 || c == 10) {
            s_edit_mode = 0;
            return 1;
        } else if (c == 127 || c == 8) {
            if (len > 0) {
                s_ski_items[s_current_selection].cmdline[len - 1] = '\0';
                return 1;
            }
        } else if (len < (SKI_CMDLINE_MAX - 1) && c >= 32 && c <= 126) {
            s_ski_items[s_current_selection].cmdline[len] = c;
            s_ski_items[s_current_selection].cmdline[len + 1] = '\0';
            return 1;
        }
        return 0;
    }

    if (s_input_state == KEY_STATE_ESC) {
        if (c == '[') s_input_state = KEY_STATE_BRACKET;
        else s_input_state = KEY_STATE_NORMAL;
        return 0;
    }
    if (s_input_state == KEY_STATE_BRACKET) {
        s_input_state = KEY_STATE_NORMAL;
        if (c == 'A') {
            if (s_current_selection > 0) { s_current_selection--; return 1; }
        } else if (c == 'B') {
            if (s_current_selection < SKI_MAX_ITEMS - 1) { s_current_selection++; return 1; }
        }
        return 0;
    }

    if (c == 27) {
        s_input_state = KEY_STATE_ESC;
        return 0;
    }
    if (c == 'q' || c == 'Q') return -1;
    if (c == 'e' || c == 'E') {
        s_edit_mode = 1;
        return 1;
    }
    if (c == 'w' || c == 'W') {
        if (s_current_selection > 0) { s_current_selection--; return 1; }
    }
    if (c == 's' || c == 'S') {
        if (s_current_selection < SKI_MAX_ITEMS - 1) { s_current_selection++; return 1; }
    }
    if (c == '+') {
        if (s_ski_items[s_current_selection].cpu_count < 64) {
            s_ski_items[s_current_selection].cpu_count++;
            return 1;
        }
    }
    if (c == '-') {
        if (s_ski_items[s_current_selection].cpu_count > 1) {
            s_ski_items[s_current_selection].cpu_count--;
            return 1;
        }
    }
    if (c == 13 || c == 10) {
        return 2;
    }
    return 0;
}

int ski_run_interactive(void) {
#if SKI_HAS_TERMIOS
    ski_enable_raw_mode();
    ski_draw_interface();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        int res = ski_handle_key(c);
        if (res == -1) break;
        if (res == 1) {
            ski_draw_interface();
        } else if (res == 2) {
            ski_print(ANSI_CLEAR);
            ski_disable_raw_mode();
            ski_print("\n" ANSI_GREEN "[Ski Bootloader Handoff] Booting " ANSI_RESET);
            ski_print(s_ski_items[s_current_selection].name);
            ski_print("...\n");
            ski_print("Kernel parameters transferred to boot environment (Target ID: ");
            ski_print_int(s_ski_items[s_current_selection].target_id);
            ski_print(")\n");
            return s_ski_items[s_current_selection].target_id;
        }
    }

    ski_disable_raw_mode();
    ski_print(ANSI_CLEAR);
    ski_print(ANSI_CURSOR_SHOW);
#endif
    return 0;
}
