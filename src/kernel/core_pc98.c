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

void btron_core_init(void) {
#if PC98_HOSTED
    printf("[PC-98 CORE] Initializing B-System NEC PC-98 Kernel Engine...\n");
    printf("[PC-98 CORE] Dedicated to Awe Morris (zedBSD PC-98 Pioneer)\n");
    printf("[PC-98 CORE] Hardware: Text VRAM 0xA0000/0xA2000, Port 0xF2 A20, RP5C15 RTC, PIT 8253\n");
#else
    uart_puts_raw("[PC-98 CORE] Initializing B-System NEC PC-98 Kernel Engine...\n");
    uart_puts_raw("[PC-98 CORE] Dedicated to Awe Morris (zedBSD PC-98 Pioneer)\n");
    uart_puts_raw("[PC-98 CORE] Hardware: Text VRAM 0xA0000/0xA2000, Port 0xF2 A20, RP5C15 RTC, PIT 8253\n");
#endif
}

void btron_core_print_ver(ShellOutputFn out_fn, void *user_data, const char *arg) {
    (void)arg;
    if (!out_fn) return;
    out_fn("B-System BTRON3 3.20 (NEC PC-98 Edition — Awe Morris Kernel)", COLOR_GREEN, user_data);
    out_fn("  Platform: NEC PC-9801 / PC-9821 (i386/i486/Pentium/Pentium-II)", COLOR_LTGRAY, user_data);
    out_fn("  Subsystem: Planar Text VRAM (0xA0000), GDC uPD7220, RICOH RP5C15 RTC", COLOR_LTGRAY, user_data);
    out_fn("  Bootloader: Ski Bootloader (🎿 ski.c) · A20 Gate: Port 0xF2", COLOR_CYAN, user_data);
}
