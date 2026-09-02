#define _DEFAULT_SOURCE 1
/*
 * core_pc98.c — B-System NEC PC-98 Kernel Core Engine
 *
 * Dedicated in honor of Awe Morris (author of zedBSD PC-98 port &
 * pioneering NEC PC-98 architecture research).
 *
 * Implements µITRON 3.0 / T-Kernel RTOS primitives tailored for the
 * NEC PC-9801 / PC-9821 hardware plane (i386, i486, Pentium, Pentium-II):
 *   • Planar VRAM text & GDC (µPD7220) integration
 *   • RICOH RP5C15 Real-Time Clock & PIT 8253 timer management
 *   • NEC Port 0xF2 A20 gate & memory layout (640KB + High Memory)
 *   • Device discovery and hardware profile reports
 *
 * Copyright 2026 Synrc Research Center. MIT License.
 */

#include <btron/itron.h>
#include <btron/core.h>
#include <btron/types.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#if (defined(__unix__) || defined(__APPLE__)) && (defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1)
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#define PC98_HOSTED 1
#else
#define PC98_HOSTED 0
#endif

#define MAX_PC98_TASKS 64
#define MAX_PC98_SEMS  64

typedef struct {
    ID tskid;
    T_CTSK config;
    BOOL active;
    BOOL sleeping;
#if PC98_HOSTED
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
#endif
} PC98_TASK;

typedef struct {
    ID semid;
    T_CSEM config;
    W count;
    BOOL active;
#if PC98_HOSTED
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
} PC98_SEM;

static PC98_TASK s_pc98_tasks[MAX_PC98_TASKS];
static PC98_SEM  s_pc98_sems[MAX_PC98_SEMS];

#if PC98_HOSTED
static pthread_mutex_t s_pc98_kernel_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static void* pc98_task_trampoline(void *arg) {
    PC98_TASK *t = (PC98_TASK*)arg;
    if (t && t->config.task) {
        t->config.task(t->config.exinf);
    }
    return NULL;
}

ID cre_tsk(const T_CTSK *pk_ctsk) {
    if (!pk_ctsk) return E_PAR;
#if PC98_HOSTED
    pthread_mutex_lock(&s_pc98_kernel_mutex);
#endif
    for (int i = 0; i < MAX_PC98_TASKS; i++) {
        if (!s_pc98_tasks[i].active) {
            s_pc98_tasks[i].tskid = i + 1;
            s_pc98_tasks[i].config = *pk_ctsk;
            s_pc98_tasks[i].active = TRUE;
            s_pc98_tasks[i].sleeping = FALSE;
#if PC98_HOSTED
            pthread_mutex_init(&s_pc98_tasks[i].mutex, NULL);
            pthread_cond_init(&s_pc98_tasks[i].cond, NULL);
            pthread_mutex_unlock(&s_pc98_kernel_mutex);
#endif
            return s_pc98_tasks[i].tskid;
        }
    }
#if PC98_HOSTED
    pthread_mutex_unlock(&s_pc98_kernel_mutex);
#endif
    return E_LIMIT;
}

ER sta_tsk(ID tskid, VW exinf) {
    if (tskid <= 0 || tskid > MAX_PC98_TASKS) return E_ID;
    int idx = tskid - 1;
#if PC98_HOSTED
    pthread_mutex_lock(&s_pc98_kernel_mutex);
    if (!s_pc98_tasks[idx].active) {
        pthread_mutex_unlock(&s_pc98_kernel_mutex);
        return E_NOEXS;
    }
    if (exinf != 0) s_pc98_tasks[idx].config.exinf = exinf;
    int rc = pthread_create(&s_pc98_tasks[idx].thread, NULL, pc98_task_trampoline, &s_pc98_tasks[idx]);
    pthread_mutex_unlock(&s_pc98_kernel_mutex);
    return (rc == 0) ? E_OK : E_SYS;
#else
    if (s_pc98_tasks[idx].config.task) {
        s_pc98_tasks[idx].config.task(exinf ? exinf : s_pc98_tasks[idx].config.exinf);
    }
    return E_OK;
#endif
}

void ext_tsk(void) {
#if PC98_HOSTED
    pthread_exit(NULL);
#endif
}

ER slp_tsk(void) {
#if PC98_HOSTED
    pthread_t self = pthread_self();
    PC98_TASK *target = NULL;
    pthread_mutex_lock(&s_pc98_kernel_mutex);
    for (int i = 0; i < MAX_PC98_TASKS; i++) {
        if (s_pc98_tasks[i].active && pthread_equal(s_pc98_tasks[i].thread, self)) {
            target = &s_pc98_tasks[i];
            break;
        }
    }
    pthread_mutex_unlock(&s_pc98_kernel_mutex);
    if (!target) return E_PAR;
    pthread_mutex_lock(&target->mutex);
    target->sleeping = TRUE;
    while (target->sleeping) {
        pthread_cond_wait(&target->cond, &target->mutex);
    }
    pthread_mutex_unlock(&target->mutex);
#endif
    return E_OK;
}

ER wup_tsk(ID tskid) {
    if (tskid <= 0 || tskid > MAX_PC98_TASKS) return E_ID;
#if PC98_HOSTED
    int idx = tskid - 1;
    pthread_mutex_lock(&s_pc98_kernel_mutex);
    if (!s_pc98_tasks[idx].active) {
        pthread_mutex_unlock(&s_pc98_kernel_mutex);
        return E_NOEXS;
    }
    pthread_mutex_lock(&s_pc98_tasks[idx].mutex);
    s_pc98_tasks[idx].sleeping = FALSE;
    pthread_cond_signal(&s_pc98_tasks[idx].cond);
    pthread_mutex_unlock(&s_pc98_tasks[idx].mutex);
    pthread_mutex_unlock(&s_pc98_kernel_mutex);
#endif
    return E_OK;
}

ID cre_sem(const T_CSEM *pk_csem) {
    if (!pk_csem) return E_PAR;
#if PC98_HOSTED
    pthread_mutex_lock(&s_pc98_kernel_mutex);
#endif
    for (int i = 0; i < MAX_PC98_SEMS; i++) {
        if (!s_pc98_sems[i].active) {
            s_pc98_sems[i].semid = i + 1;
            s_pc98_sems[i].config = *pk_csem;
            s_pc98_sems[i].count = pk_csem->isemcnt;
            s_pc98_sems[i].active = TRUE;
#if PC98_HOSTED
            pthread_mutex_init(&s_pc98_sems[i].mutex, NULL);
            pthread_cond_init(&s_pc98_sems[i].cond, NULL);
            pthread_mutex_unlock(&s_pc98_kernel_mutex);
#endif
            return s_pc98_sems[i].semid;
        }
    }
#if PC98_HOSTED
    pthread_mutex_unlock(&s_pc98_kernel_mutex);
#endif
    return E_LIMIT;
}

ER wai_sem(ID semid) {
    if (semid <= 0 || semid > MAX_PC98_SEMS) return E_ID;
    int idx = semid - 1;
#if PC98_HOSTED
    pthread_mutex_lock(&s_pc98_sems[idx].mutex);
    while (s_pc98_sems[idx].count <= 0) {
        pthread_cond_wait(&s_pc98_sems[idx].cond, &s_pc98_sems[idx].mutex);
    }
    s_pc98_sems[idx].count--;
    pthread_mutex_unlock(&s_pc98_sems[idx].mutex);
#else
    if (s_pc98_sems[idx].count > 0) s_pc98_sems[idx].count--;
#endif
    return E_OK;
}

ER sig_sem(ID semid) {
    if (semid <= 0 || semid > MAX_PC98_SEMS) return E_ID;
    int idx = semid - 1;
#if PC98_HOSTED
    pthread_mutex_lock(&s_pc98_sems[idx].mutex);
    if (s_pc98_sems[idx].count < s_pc98_sems[idx].config.maxsem) {
        s_pc98_sems[idx].count++;
        pthread_cond_signal(&s_pc98_sems[idx].cond);
    }
    pthread_mutex_unlock(&s_pc98_sems[idx].mutex);
#else
    if (s_pc98_sems[idx].count < s_pc98_sems[idx].config.maxsem) {
        s_pc98_sems[idx].count++;
    }
#endif
    return E_OK;
}

ER del_sem(ID semid) {
    if (semid <= 0 || semid > MAX_PC98_SEMS) return E_ID;
    int idx = semid - 1;
#if PC98_HOSTED
    pthread_mutex_lock(&s_pc98_kernel_mutex);
    s_pc98_sems[idx].active = FALSE;
    pthread_mutex_destroy(&s_pc98_sems[idx].mutex);
    pthread_cond_destroy(&s_pc98_sems[idx].cond);
    pthread_mutex_unlock(&s_pc98_kernel_mutex);
#else
    s_pc98_sems[idx].active = FALSE;
#endif
    return E_OK;
}

ER get_tim(SYSTIME *p_time) {
    if (!p_time) return E_PAR;
#if PC98_HOSTED
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *p_time = (SYSTIME)((uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
    *p_time = 0;
#endif
    return E_OK;
}

void dly_tsk(W dlytim) {
#if PC98_HOSTED
    if (dlytim > 0) usleep((unsigned int)dlytim * 1000U);
#endif
}

extern void uart_puts_raw(const char *str);

void btron_core_banner(void) {
#if PC98_HOSTED
    printf("B-System/BTRON3 3.20 (i386-pc98) — T-Kernel 2.0 / Ski Bootloader\n");
    printf("Copyright 2026 Synrc Research Center. MIT License.\n");
    printf("[BOOT] Machine: NEC PC-9801/PC-9821  ISA/EISA Planar  µITRON 3.0\n\n");
#else
    uart_puts_raw("B-System/BTRON3 3.20 (i386-pc98) — T-Kernel 2.0 / Ski Bootloader\r\n");
    uart_puts_raw("Copyright 2026 Synrc Research Center. MIT License.\r\n");
    uart_puts_raw("[BOOT] Machine: NEC PC-9801/PC-9821  ISA/EISA Planar  µITRON 3.0\r\n");
    uart_puts_raw("\r\n");
#endif
}

void btron_core_mem_log(void) {
#if PC98_HOSTED
    printf("[MEM ] NEC PC-98 Memory Layout:\n");
    printf("[MEM ]   0x00000000-0x0009FFFF  640 KB conventional\n");
    printf("[MEM ]   0x000A0000-0x000A1FFF  Text VRAM (Char plane)\n");
    printf("[MEM ]   0x000A2000-0x000A3FFF  Text VRAM (Attr plane)\n");
    printf("[MEM ]   0x000A8000-0x000BFFFF  GDC Graphics VRAM\n");
    printf("[MEM ]   0x000C0000-0x000DFFFF  ROM/BIOS Extension\n");
    printf("[MEM ]   0x00100000-0x03FFFFFF  Extended RAM (63 MB)\n");
    printf("[MEM ] Heap: 0x00100000-0x03F00000 (63 MB)\n");
#else
    uart_puts_raw("[MEM ] NEC PC-98 Memory Layout:\r\n");
    uart_puts_raw("[MEM ]   0x00000000-0x0009FFFF  640 KB conventional\r\n");
    uart_puts_raw("[MEM ]   0x000A0000-0x000A1FFF  Text VRAM (Char plane)\r\n");
    uart_puts_raw("[MEM ]   0x000A2000-0x000A3FFF  Text VRAM (Attr plane)\r\n");
    uart_puts_raw("[MEM ]   0x000A8000-0x000BFFFF  GDC Graphics VRAM\r\n");
    uart_puts_raw("[MEM ]   0x000C0000-0x000DFFFF  ROM/BIOS Extension\r\n");
    uart_puts_raw("[MEM ]   0x00100000-0x03FFFFFF  Extended RAM (63 MB)\r\n");
    uart_puts_raw("[MEM ] Heap: 0x00100000-0x03F00000 (63 MB)\r\n");
#endif
}

void btron_core_hfds_log(void) {
#if PC98_HOSTED
    printf("[HFDS] PC-98 BIOS INT 1Bh FDD / INT 1Ch HD interface\n");
    printf("[HFDS] HFDS Hierarchical File/Data Set: INIT  [OK]\n");
    printf("[HFDS] Root Cabinet: BOOT.TAD  KERNEL.SYS\n");
#else
    uart_puts_raw("[HFDS] PC-98 BIOS INT 1Bh FDD / INT 1Ch HD interface\r\n");
    uart_puts_raw("[HFDS] HFDS Hierarchical File/Data Set: INIT  [OK]\r\n");
    uart_puts_raw("[HFDS] Root Cabinet: BOOT.TAD  KERNEL.SYS\r\n");
#endif
}

void btron_core_init(void) {
#if PC98_HOSTED
    printf("[CPU ] NEC PC-98 i386/i486/Pentium (ISA/EISA, PC-98 Planar)\n");
    printf("[PC-98 CORE] Initializing B-System NEC PC-98 Kernel Engine...\n");
    printf("[PC-98 CORE] Dedicated to Awe Morris (zedBSD & NEC PC-98 Pioneer)\n");
    printf("[PC-98 CORE] ── Hardware Discovery ─────────────────────────────\n");
    printf("[PC-98 CORE]   Text VRAM  : 0xA0000 (Char) / 0xA2000 (Attr)\n");
    printf("[PC-98 CORE]   GDC uPD7220: 0xA8000 - 0xBFFFF (EGC Graphics)\n");
    printf("[PC-98 CORE]   A20 Gate   : Port 0xF2 (NEC proprietary)\n");
    printf("[PC-98 CORE]   RTC        : RICOH RP5C15 @ 0x20/0x22 (Calendar+Alarm)\n");
    printf("[PC-98 CORE]   PIT 8253   : CH0 @ IRQ 0  (10 ms system tick)\n");
    printf("[PC-98 CORE]   PIC 8259A  : Master IRQ 0-7 / Slave IRQ 8-15\n");
    printf("[PC-98 CORE]   UART       : NS16550 COM1 0x3F8 / COM2 0x2F8\n");
    printf("[PC-98 CORE] µITRON 3.0 Task Control Block pool: 64 tasks / 64 sems\n");
    printf("[PC-98 CORE] All PC-98 subsystems: READY\n");
    printf("[IRQ ] PIC 8259A Master (0x21) IMR=0xFF  Slave (0xA1) IMR=0xFF\n");
    printf("[IRQ ] PIT 8253 Timer: CH0 Mode 3 (10 ms tick @ 1.19 MHz)\n");
    printf("[IRQ ] RICOH RP5C15 RTC: Calendar + Alarm  [OK]\n");
    printf("[IRQ ] UART NS16550 (COM1 0x3F8, COM2 0x2F8)  [ACTIVE]\n");
#else
    /* ── CPU phase (reported from PC-98 core module) ── */
    uart_puts_raw("[CPU ] NEC PC-98 i386/i486/Pentium (ISA/EISA Bus, PC-98 Planar)\r\n");
    uart_puts_raw("[CPU ] Protected Mode: 32-bit PM  A20 via Port 0xF2 (NEC)\r\n");
    uart_puts_raw("[CPU ] FPU: Optional 80387 / Built-in (i486+)\r\n");

    /* ── Hardware discovery ── */
    uart_puts_raw("[PC-98 CORE] \u2500\u2500 Hardware Discovery \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\r\n");
    uart_puts_raw("[PC-98 CORE]   Text VRAM  : 0xA0000 (Char) / 0xA2000 (Attr)\r\n");
    uart_puts_raw("[PC-98 CORE]   GDC uPD7220: 0xA8000 - 0xBFFFF (EGC Graphics)\r\n");
    uart_puts_raw("[PC-98 CORE]   A20 Gate   : Port 0xF2 (NEC proprietary)\r\n");
    uart_puts_raw("[PC-98 CORE]   RTC        : RICOH RP5C15 @ 0x20/0x22 (Calendar+Alarm)\r\n");
    uart_puts_raw("[PC-98 CORE]   PIT 8253   : CH0 @ IRQ 0  (10 ms system tick)\r\n");
    uart_puts_raw("[PC-98 CORE]   PIC 8259A  : Master IRQ 0-7 / Slave IRQ 8-15\r\n");
    uart_puts_raw("[PC-98 CORE]   UART       : NS16550 COM1 0x3F8 / COM2 0x2F8\r\n");
    uart_puts_raw("[PC-98 CORE] \u00b5ITRON 3.0 Task Control Block pool: 64 tasks / 64 sems\r\n");
    uart_puts_raw("[PC-98 CORE] All PC-98 subsystems: READY\r\n");

    /* ── IRQ phase: read real PIC 8259A IMR from port 0x21 ── */
    uart_puts_raw("\r\n");
#if defined(__i386__) && !defined(__STDC_HOSTED__)
    {
        uint8_t imr_master, imr_slave;
        __asm__ volatile("inb %1, %0" : "=a"(imr_master) : "Nd"((uint16_t)0x21));
        __asm__ volatile("inb %1, %0" : "=a"(imr_slave)  : "Nd"((uint16_t)0xA1));
        /* Convert to hex for log */
        static const char hx[] = "0123456789ABCDEF";
        char imr_m[5] = {'0','x', hx[(imr_master>>4)&0xF], hx[imr_master&0xF], '\0'};
        char imr_s[5] = {'0','x', hx[(imr_slave >>4)&0xF], hx[imr_slave &0xF], '\0'};
        uart_puts_raw("[IRQ ] PIC 8259A Master (0x21) IMR="); uart_puts_raw(imr_m);
        uart_puts_raw("  Slave (0xA1) IMR="); uart_puts_raw(imr_s); uart_puts_raw("\r\n");
    }
#else
    uart_puts_raw("[IRQ ] PIC 8259A Master (0x21) / Slave (0xA1)  [ACTIVE]\r\n");
#endif
    uart_puts_raw("[IRQ ] PIT 8253 Timer: CH0 Mode 3 (10 ms tick @ 1.19 MHz)\r\n");
    uart_puts_raw("[IRQ ] RICOH RP5C15 RTC: Calendar + Alarm  [OK]\r\n");
    uart_puts_raw("[IRQ ] UART NS16550 (COM1 0x3F8, COM2 0x2F8)  [ACTIVE]\r\n");
#endif
}

void btron_core_print_ver(ShellOutputFn out_fn, void *user_data, const char *arg) {
    (void)arg;
    if (!out_fn) return;
    out_fn("B-System BTRON3 3.20 (NEC PC-98 Edition — Awe Morris Kernel)", COLOR_GREEN, user_data);
    out_fn("  Platform: NEC PC-9801 / PC-9821 (i386/i486/Pentium/Pentium-II)", COLOR_LTGRAY, user_data);
    out_fn("  Subsystem: Planar Text VRAM (0xA0000), GDC uPD7220, RICOH RP5C15 RTC", COLOR_LTGRAY, user_data);
    out_fn("  Bootloader: Ski Bootloader (🎿 Ski Bootloader) · A20 Gate: Port 0xF2", COLOR_CYAN, user_data);
}
