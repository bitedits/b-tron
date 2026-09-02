/*
 * core_smp.c — B-System x86_64 UEFI SMP bring-up
 *
 * Cleanroom implementation without TianoCore EDK II dependencies.
 *
 * References:
 *   • Haiku src/system/boot/platform/efi/arch/x86/arch_smp.cpp
 *   • Intel 64 and IA-32 SDM Vol.3A §10.6 (INIT-SIPI-SIPI)
 *   • ACPI Spec 6.5 §5.2.12 (MADT structures)
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <btron/smp.h>

volatile uint32_t  g_cpu_ready[BTRON_SMP_MAX_CPUS];
btron_cpu_entry_t  g_cpu_topology[BTRON_SMP_MAX_CPUS];
volatile uint32_t  g_num_cpus   = 1;
volatile uint32_t  g_cpus_online = 1;

static uint8_t g_ap_stacks[BTRON_SMP_MAX_CPUS][BTRON_SMP_AP_STACK_SIZE]
    __attribute__((aligned(16)));

static volatile uint32_t *s_lapic = NULL;

static inline void btron_smp_pause(void) {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile ("pause" ::: "memory");
#endif
}

static inline void btron_smp_mfence(void) {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile ("mfence" ::: "memory");
#endif
}

static inline uint32_t lapic_read(uint32_t off) {
    if (!s_lapic) return 0;
    return s_lapic[off >> 2];
}

static inline void lapic_write(uint32_t off, uint32_t val) {
    if (!s_lapic) return;
    s_lapic[off >> 2] = val;
    (void)s_lapic[LAPIC_TPR >> 2];
}

static inline uint8_t lapic_local_id(void) {
    return (uint8_t)(lapic_read(LAPIC_ID_REG) >> 24);
}

static inline void lapic_clear_errors(void) {
    lapic_write(LAPIC_ERROR_STATUS, 0);
    (void)lapic_read(LAPIC_ERROR_STATUS);
}

static void lapic_wait_icr_idle(void) {
    uint32_t limit = 1000000U;
    while ((lapic_read(LAPIC_ICR_LO) & LAPIC_PENDING) && limit--)
        btron_smp_pause();
}

static int btron_smp_map_lapic(uint64_t phys_base) {
    s_lapic = (volatile uint32_t *)(uintptr_t)phys_base;
    uint32_t svr = lapic_read(LAPIC_SVR);
    lapic_write(LAPIC_SVR, svr | LAPIC_SVR_ENABLE | 0xFF);
    return 0;
}

#pragma pack(push, 1)
typedef struct {
    char     signature[8];
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t  ext_checksum;
    uint8_t  reserved[3];
} btron_rsdp_t;

typedef struct {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} btron_sdt_hdr_t;

typedef struct {
    btron_sdt_hdr_t hdr;
    uint32_t lapic_addr;
    uint32_t flags;
} btron_madt_t;

typedef struct {
    uint8_t  type;
    uint8_t  length;
} btron_madt_entry_hdr_t;

typedef struct {
    btron_madt_entry_hdr_t hdr;
    uint8_t  acpi_processor_id;
    uint8_t  apic_id;
    uint32_t flags;
} btron_madt_local_apic_t;
#pragma pack(pop)

static int acpi_checksum_ok(const void *table, uint32_t length) {
    const uint8_t *p = (const uint8_t *)table;
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) sum += p[i];
    return sum == 0;
}

int btron_smp_parse_madt(const void *rsdp_ptr) {
    if (!rsdp_ptr) {
        g_num_cpus = 1;
        g_cpu_topology[0].apic_id = 0;
        g_cpu_topology[0].is_bsp = 1;
        g_cpu_topology[0].online = 1;
        return 1;
    }

    const btron_rsdp_t *rsdp = (const btron_rsdp_t *)rsdp_ptr;
    if (memcmp(rsdp->signature, "RSD PTR ", 8) != 0) return -1;
    if (!acpi_checksum_ok(rsdp, sizeof(*rsdp))) return -1;

    btron_sdt_hdr_t *madt_hdr = NULL;
    uint64_t lapic_phys_addr = BTRON_LAPIC_DEFAULT_BASE;

    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
        const btron_sdt_hdr_t *xsdt = (const btron_sdt_hdr_t *)(uintptr_t)rsdp->xsdt_address;
        if (acpi_checksum_ok(xsdt, xsdt->length)) {
            uint32_t n_entries = (xsdt->length - sizeof(btron_sdt_hdr_t)) / sizeof(uint64_t);
            const uint64_t *entries = (const uint64_t *)((const uint8_t *)xsdt + sizeof(btron_sdt_hdr_t));
            for (uint32_t i = 0; i < n_entries; i++) {
                const btron_sdt_hdr_t *t = (const btron_sdt_hdr_t *)(uintptr_t)entries[i];
                if (memcmp(t->signature, "APIC", 4) == 0) {
                    madt_hdr = (btron_sdt_hdr_t *)t;
                    break;
                }
            }
        }
    }

    if (!madt_hdr) {
        g_num_cpus = 1;
        g_cpu_topology[0].apic_id = 0;
        g_cpu_topology[0].is_bsp = 1;
        g_cpu_topology[0].online = 1;
        return 1;
    }

    const btron_madt_t *madt = (const btron_madt_t *)madt_hdr;
    lapic_phys_addr = (uint64_t)madt->lapic_addr;

    const uint8_t *p = (const uint8_t *)madt + sizeof(btron_madt_t);
    const uint8_t *end = (const uint8_t *)madt + madt->hdr.length;
    uint32_t ncpu = 0;

    while (p < end && ncpu < BTRON_SMP_MAX_CPUS) {
        const btron_madt_entry_hdr_t *entry = (const btron_madt_entry_hdr_t *)p;
        if (entry->length < 2) break;

        if (entry->type == MADT_TYPE_LOCAL_APIC) {
            const btron_madt_local_apic_t *la = (const btron_madt_local_apic_t *)p;
            if (la->flags & (MADT_LAPIC_ENABLED | MADT_LAPIC_ONLINE_CAPABLE)) {
                g_cpu_topology[ncpu].apic_id = la->apic_id;
                g_cpu_topology[ncpu].apic_version = 0x10;
                g_cpu_topology[ncpu].online = 1;
                g_cpu_topology[ncpu].is_bsp = (ncpu == 0) ? 1 : 0;
                ncpu++;
            }
        }
        p += entry->length;
    }

    g_num_cpus = (ncpu > 0) ? ncpu : 1;
    btron_smp_map_lapic(lapic_phys_addr);
    return (int)g_num_cpus;
}

void btron_smp_spin_us(uint32_t us) {
    volatile uint32_t count = us * 100;
    while (count--) {
        btron_smp_pause();
    }
}

int btron_smp_prepare_aps(void) {
    for (uint32_t i = 0; i < g_num_cpus; i++) {
        g_cpu_topology[i].stack_top = g_ap_stacks[i] + BTRON_SMP_AP_STACK_SIZE;
        g_cpu_ready[i] = (i == 0) ? 1 : 0;
    }
    return 0;
}

int btron_smp_boot_aps(void) {
    if (g_num_cpus < 2) return 0;
    int online = 0;

    for (uint32_t i = 1; i < g_num_cpus; i++) {
        lapic_clear_errors();
        if (s_lapic) {
            lapic_write(LAPIC_ICR_HI, (uint32_t)g_cpu_topology[i].apic_id << 24);
            lapic_wait_icr_idle();
            lapic_write(LAPIC_ICR_LO, LAPIC_DM_INIT | LAPIC_TRIGGER_LEVEL | LAPIC_LEVEL_ASSERT);
            lapic_wait_icr_idle();
            btron_smp_spin_us(200);

            lapic_write(LAPIC_ICR_LO, LAPIC_DM_INIT | LAPIC_TRIGGER_LEVEL | LAPIC_LEVEL_DEASSERT);
            lapic_wait_icr_idle();
            btron_smp_spin_us(10000);

            for (int s = 0; s < 2; s++) {
                lapic_clear_errors();
                lapic_write(LAPIC_ICR_LO, LAPIC_DM_STARTUP | (BTRON_SMP_TRAMPOLINE_PHYS >> 12));
                lapic_wait_icr_idle();
                btron_smp_spin_us(200);
            }
        }
        g_cpu_ready[i] = 1;
        online++;
    }
    g_cpus_online = 1 + online;
    return online;
}

uint32_t btron_cpu_id(void) {
    if (!s_lapic) return 0;
    uint8_t id = lapic_local_id();
    for (uint32_t i = 0; i < g_num_cpus; i++) {
        if (g_cpu_topology[i].apic_id == id) return i;
    }
    return 0;
}

void btron_smp_ap_entry(uint32_t cpu_idx) {
    if (cpu_idx < BTRON_SMP_MAX_CPUS) {
        g_cpu_ready[cpu_idx] = 1;
    }
    btron_smp_mfence();
}
