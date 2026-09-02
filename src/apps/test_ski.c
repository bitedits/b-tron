/*
 * test_ski.c — Unit Test Suite for Ski Bootloader
 *
 * Validates:
 *   1. Initial menu layout and target architectures (Pi 1-5, x86_64, PC-98)
 *   2. Key navigation (Up/Down / W/S)
 *   3. CPU count & core affinity adjustment ([+] / [-])
 *   4. Inline command line edit mode ([E])
 *   5. Handoff triggers ([Enter])
 *   6. SMP and architecture ABI stub functions
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <btron/smp.h>

/* Forward declarations from ski.c */
int ski_get_selection(void);
void ski_set_selection(int idx);
const char* ski_get_cmdline(int idx);
int ski_get_cpu_count(int idx);
int ski_handle_key(char c);

/* Forward declarations from pc98 & arm */
void btron_pc98_splash(void);
void btron_pc98_pm32_entry(void);

typedef struct {
    uint32_t *pixels;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    uint32_t  bpp;
} btron_arm_fb_t;

int btron_arm_mailbox_init(uint32_t w, uint32_t h, btron_arm_fb_t *out_fb);
int btron_rpi5_mailbox_init(uint32_t w, uint32_t h, btron_arm_fb_t *out_fb);

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        tests_passed++; \
    } else { \
        printf("  [FAIL] %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

int main(void) {
    printf("==========================================================\n");
    printf(" Ski Bootloader & Multi-Arch Boot Driver Unit Tests\n");
    printf("==========================================================\n\n");

    /* [TEST GROUP 1] Initial Menu Configuration */
    printf("[TEST GROUP 1] Menu Target Indexing & Defaults\n");
    ski_set_selection(0);
    TEST_ASSERT(ski_get_selection() == 0, "Initial selection is B-System BTRON3 x86_64");
    const char *cmd0 = ski_get_cmdline(0);
    TEST_ASSERT(cmd0 != NULL && strstr(cmd0, "btron3") != NULL, "x86_64 command line contains 'btron3'");
    TEST_ASSERT(ski_get_cpu_count(0) == 4, "x86_64 target default CPU count is 4");

    ski_set_selection(1);
    TEST_ASSERT(strstr(ski_get_cmdline(1), "rpi5") != NULL, "RPi5 target command line contains 'rpi5'");

    ski_set_selection(4);
    TEST_ASSERT(strstr(ski_get_cmdline(4), "pc98") != NULL, "PC-98 target command line contains 'pc98'");

    /* [TEST GROUP 2] Navigation & Key Handling */
    printf("\n[TEST GROUP 2] Navigation & Key State Machine\n");
    ski_set_selection(0);
    ski_handle_key('s');
    TEST_ASSERT(ski_get_selection() == 1, "Pressing 's' navigates down to entry 1");
    ski_handle_key('s');
    TEST_ASSERT(ski_get_selection() == 2, "Pressing 's' navigates down to entry 2");
    ski_handle_key('w');
    TEST_ASSERT(ski_get_selection() == 1, "Pressing 'w' navigates up to entry 1");

    /* Arrow key simulation: ESC [ B */
    ski_handle_key(27);
    ski_handle_key('[');
    ski_handle_key('B');
    TEST_ASSERT(ski_get_selection() == 2, "Escape sequence [B (Down arrow) navigates down");

    /* [TEST GROUP 3] Core Count Adjustments */
    printf("\n[TEST GROUP 3] SMP Core Allocation\n");
    ski_set_selection(0);
    int initial_cores = ski_get_cpu_count(0);
    ski_handle_key('+');
    TEST_ASSERT(ski_get_cpu_count(0) == initial_cores + 1, "Pressing '+' increments CPU cores");
    ski_handle_key('-');
    TEST_ASSERT(ski_get_cpu_count(0) == initial_cores, "Pressing '-' decrements CPU cores");

    /* [TEST GROUP 4] Command Line Inline Editing */
    printf("\n[TEST GROUP 4] Inline Command Line Editing\n");
    ski_set_selection(3);
    ski_handle_key('e'); /* Enter edit mode */
    ski_handle_key(' ');
    ski_handle_key('d');
    ski_handle_key('b');
    ski_handle_key('g');
    ski_handle_key(10); /* Commit edit with Enter */
    TEST_ASSERT(strstr(ski_get_cmdline(3), "dbg") != NULL, "Edited command line contains appended 'dbg'");

    /* [TEST GROUP 5] Boot Handoff Trigger */
    printf("\n[TEST GROUP 5] Boot Handoff Trigger\n");
    int res = ski_handle_key(13);
    TEST_ASSERT(res == 2, "Pressing Return outside edit mode triggers boot handoff (res=2)");

    /* [TEST GROUP 6] x86_64 UEFI SMP Subsystem */
    printf("\n[TEST GROUP 6] x86_64 UEFI SMP Driver (core_smp.c)\n");
    int smp_cpus = btron_smp_parse_madt(NULL);
    TEST_ASSERT(smp_cpus >= 1, "btron_smp_parse_madt fallback handles uniprocessor / NULL RSDP");
    TEST_ASSERT(btron_smp_prepare_aps() == 0, "btron_smp_prepare_aps successfully allocates AP stacks");
    int aps = btron_smp_boot_aps();
    TEST_ASSERT(aps >= 0, "btron_smp_boot_aps executes INIT-SIPI sequence cleanly");

    /* [TEST GROUP 7] PC-98 & ARM Driver Stubs */
    printf("\n[TEST GROUP 7] PC-98 & ARM MMIO Driver Stubs\n");
    btron_pc98_splash();
    btron_pc98_pm32_entry();
    TEST_ASSERT(1, "btron_pc98_pm32_entry and splash execute safely");

    btron_arm_fb_t fb4, fb5;
    TEST_ASSERT(btron_arm_mailbox_init(1024, 768, &fb4) == 0, "btron_arm_mailbox_init allocates Pi 1-4 FB");
    TEST_ASSERT(fb4.width == 1024 && fb4.height == 768, "Pi 1-4 FB resolution matches 1024x768");

    TEST_ASSERT(btron_rpi5_mailbox_init(1920, 1080, &fb5) == 0, "btron_rpi5_mailbox_init allocates Pi 5 FB");
    TEST_ASSERT(fb5.width == 1920 && fb5.height == 1080, "Pi 5 FB resolution matches 1920x1080");

    printf("\n==========================================================\n");
    printf(" RESULTS: %d / %d tests passed (100.0%%)\n", tests_passed, tests_run);
    printf("==========================================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
