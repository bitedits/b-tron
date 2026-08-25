/*
 * B-TRON Real-Time Kernel Core: Merged Single-File Core (merged_core.c)
 * Function-level #ifdef BTRON_QEMU_TARGET branching model.
 *
 * NOTE: not reaaly used, just to prove that merged code is
 *       ugly way of managing things, they are just TRON implementations.
 */

#include <btron/itron.h>
#include "virtio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BTRON_QEMU_TARGET
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#endif

/* -------------------------------------------------------------------------
 * Shared Data Structures & Configuration
 * ------------------------------------------------------------------------- */
#define MAX_TASKS 64
#define MAX_SEMS  64

#ifndef BTRON_QEMU_TARGET

/* POSIX Backend State */
typedef struct {
    ID tskid;
    T_CTSK config;
    pthread_t thread;
    BOOL active;
    BOOL sleeping;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} ITRON_TASK;

typedef struct {
    ID semid;
    T_CSEM config;
    W count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    BOOL active;
} ITRON_SEM;

static ITRON_TASK g_tasks[MAX_TASKS];
static ITRON_SEM  g_sems[MAX_SEMS];
static pthread_mutex_t g_kernel_mutex = PTHREAD_MUTEX_INITIALIZER;

#else

/* Bare-Metal QEMU State */
typedef enum {
    TK_TS_NONEXS = 0,
    TK_TS_READY  = 1,
    TK_TS_WAIT   = 2,
    TK_TS_SUSP   = 4,
    TK_TS_DORM   = 8
} TK_TASK_STATE;

typedef struct {
    ID tskid;
    T_CTSK config;
    TK_TASK_STATE state;
    PRI current_pri;
    VP stack_ptr;
} TK_TCB;

typedef struct {
    ID semid;
    T_CSEM config;
    W count;
    BOOL active;
} TK_SEMB;

static TK_TCB  g_tk_tasks[MAX_TASKS];
static TK_SEMB g_tk_sems[MAX_SEMS];
static ID      g_current_tskid = 1;
static SYSTIME g_tk_system_ticks = 0;

#endif

/* -------------------------------------------------------------------------
 * Function-Level Dual Implementations
 * ------------------------------------------------------------------------- */

void tkernel_init(void) {
#ifndef BTRON_QEMU_TARGET
    pthread_mutex_lock(&g_kernel_mutex);
    memset(g_tasks, 0, sizeof(g_tasks));
    memset(g_sems, 0, sizeof(g_sems));
    pthread_mutex_unlock(&g_kernel_mutex);
    printf("[KERNEL] POSIX microkernel abstraction initialized.\n");
#else
    (void)g_current_tskid;
    memset(g_tk_tasks, 0, sizeof(g_tk_tasks));
    memset(g_tk_sems, 0, sizeof(g_tk_sems));
    printf("[T-KERNEL] Real-Time ITRON Core initialized (Raspberry Pi / QEMU target).\n");
    virtio_mmio_init(0x10001000);
#endif
}

void btron_posix_kernel_init(void) {
    tkernel_init();
}

ID cre_tsk(const T_CTSK *pk_ctsk) {
    if (!pk_ctsk || !pk_ctsk->task) return E_PAR;

#ifndef BTRON_QEMU_TARGET
    pthread_mutex_lock(&g_kernel_mutex);
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!g_tasks[i].active) {
            g_tasks[i].tskid = i + 1;
            g_tasks[i].config = *pk_ctsk;
            g_tasks[i].active = TRUE;
            g_tasks[i].sleeping = FALSE;
            pthread_mutex_init(&g_tasks[i].mutex, NULL);
            pthread_cond_init(&g_tasks[i].cond, NULL);

            ID id = g_tasks[i].tskid;
            pthread_mutex_unlock(&g_kernel_mutex);
            return id;
        }
    }
    pthread_mutex_unlock(&g_kernel_mutex);
    return E_NOMEM;
#else
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tk_tasks[i].state == TK_TS_NONEXS) {
            g_tk_tasks[i].tskid = i + 1;
            g_tk_tasks[i].config = *pk_ctsk;
            g_tk_tasks[i].state = TK_TS_DORM;
            g_tk_tasks[i].current_pri = pk_ctsk->itskpri;
            return g_tk_tasks[i].tskid;
        }
    }
    return E_NOMEM;
#endif
}

#ifndef BTRON_QEMU_TARGET
static void* task_wrapper(void *arg) {
    ITRON_TASK *tsk = (ITRON_TASK*)arg;
    if (tsk && tsk->config.task) {
        tsk->config.task(tsk->config.exinf);
    }
    tsk->active = FALSE;
    return NULL;
}
#endif

ER sta_tsk(ID tskid, VW exinf) {
    if (tskid <= 0 || tskid > MAX_TASKS) return E_ID;

#ifndef BTRON_QEMU_TARGET
    pthread_mutex_lock(&g_kernel_mutex);
    ITRON_TASK *tsk = &g_tasks[tskid - 1];
    if (!tsk->active) {
        pthread_mutex_unlock(&g_kernel_mutex);
        return E_NOEXS;
    }
    tsk->config.exinf = exinf;
    if (pthread_create(&tsk->thread, NULL, task_wrapper, tsk) != 0) {
        pthread_mutex_unlock(&g_kernel_mutex);
        return E_SYS;
    }
    pthread_mutex_unlock(&g_kernel_mutex);
    return E_OK;
#else
    TK_TCB *tcb = &g_tk_tasks[tskid - 1];
    if (tcb->state == TK_TS_NONEXS) return E_NOEXS;

    tcb->config.exinf = exinf;
    tcb->state = TK_TS_READY;
    return E_OK;
#endif
}

void ext_tsk(void) {
#ifndef BTRON_QEMU_TARGET
    pthread_exit(NULL);
#else
    if (g_current_tskid > 0 && g_current_tskid <= MAX_TASKS) {
        g_tk_tasks[g_current_tskid - 1].state = TK_TS_DORM;
    }
#endif
}

ER slp_tsk(void) {
#ifndef BTRON_QEMU_TARGET
    pthread_t self = pthread_self();
    ITRON_TASK *tsk = NULL;

    pthread_mutex_lock(&g_kernel_mutex);
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].active && pthread_equal(g_tasks[i].thread, self)) {
            tsk = &g_tasks[i];
            break;
        }
    }
    pthread_mutex_unlock(&g_kernel_mutex);

    if (!tsk) return E_OBJ;

    pthread_mutex_lock(&tsk->mutex);
    tsk->sleeping = TRUE;
    while (tsk->sleeping) {
        pthread_cond_wait(&tsk->cond, &tsk->mutex);
    }
    pthread_mutex_unlock(&tsk->mutex);

    return E_OK;
#else
    if (g_current_tskid > 0 && g_current_tskid <= MAX_TASKS) {
        g_tk_tasks[g_current_tskid - 1].state = TK_TS_WAIT;
    }
    return E_OK;
#endif
}

ER wup_tsk(ID tskid) {
    if (tskid <= 0 || tskid > MAX_TASKS) return E_ID;

#ifndef BTRON_QEMU_TARGET
    pthread_mutex_lock(&g_kernel_mutex);
    ITRON_TASK *tsk = &g_tasks[tskid - 1];
    if (!tsk->active) {
        pthread_mutex_unlock(&g_kernel_mutex);
        return E_NOEXS;
    }
    pthread_mutex_unlock(&g_kernel_mutex);

    pthread_mutex_lock(&tsk->mutex);
    if (tsk->sleeping) {
        tsk->sleeping = FALSE;
        pthread_cond_signal(&tsk->cond);
    }
    pthread_mutex_unlock(&tsk->mutex);

    return E_OK;
#else
    TK_TCB *tcb = &g_tk_tasks[tskid - 1];
    if (tcb->state == TK_TS_WAIT) {
        tcb->state = TK_TS_READY;
    }
    return E_OK;
#endif
}

ID cre_sem(const T_CSEM *pk_csem) {
    if (!pk_csem) return E_PAR;

#ifndef BTRON_QEMU_TARGET
    pthread_mutex_lock(&g_kernel_mutex);
    for (int i = 0; i < MAX_SEMS; i++) {
        if (!g_sems[i].active) {
            g_sems[i].semid = i + 1;
            g_sems[i].config = *pk_csem;
            g_sems[i].count = pk_csem->isemcnt;
            g_sems[i].active = TRUE;
            pthread_mutex_init(&g_sems[i].mutex, NULL);
            pthread_cond_init(&g_sems[i].cond, NULL);

            ID id = g_sems[i].semid;
            pthread_mutex_unlock(&g_kernel_mutex);
            return id;
        }
    }
    pthread_mutex_unlock(&g_kernel_mutex);
    return E_NOMEM;
#else
    for (int i = 0; i < MAX_SEMS; i++) {
        if (!g_tk_sems[i].active) {
            g_tk_sems[i].semid = i + 1;
            g_tk_sems[i].config = *pk_csem;
            g_tk_sems[i].count = pk_csem->isemcnt;
            g_tk_sems[i].active = TRUE;
            return g_tk_sems[i].semid;
        }
    }
    return E_NOMEM;
#endif
}

ER wai_sem(ID semid) {
    if (semid <= 0 || semid > MAX_SEMS) return E_ID;

#ifndef BTRON_QEMU_TARGET
    ITRON_SEM *sem = &g_sems[semid - 1];
    if (!sem->active) return E_NOEXS;

    pthread_mutex_lock(&sem->mutex);
    while (sem->count <= 0) {
        pthread_cond_wait(&sem->cond, &sem->mutex);
    }
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);

    return E_OK;
#else
    TK_SEMB *sem = &g_tk_sems[semid - 1];
    if (!sem->active) return E_NOEXS;
    if (sem->count > 0) {
        sem->count--;
        return E_OK;
    }
    return E_TMOUT;
#endif
}

ER sig_sem(ID semid) {
    if (semid <= 0 || semid > MAX_SEMS) return E_ID;

#ifndef BTRON_QEMU_TARGET
    ITRON_SEM *sem = &g_sems[semid - 1];
    if (!sem->active) return E_NOEXS;

    pthread_mutex_lock(&sem->mutex);
    if (sem->count < sem->config.maxsem) {
        sem->count++;
        pthread_cond_signal(&sem->cond);
    }
    pthread_mutex_unlock(&sem->mutex);

    return E_OK;
#else
    TK_SEMB *sem = &g_tk_sems[semid - 1];
    if (!sem->active) return E_NOEXS;
    if (sem->count < sem->config.maxsem) {
        sem->count++;
    }
    return E_OK;
#endif
}

ER del_sem(ID semid) {
    if (semid <= 0 || semid > MAX_SEMS) return E_ID;

#ifndef BTRON_QEMU_TARGET
    ITRON_SEM *sem = &g_sems[semid - 1];
    if (!sem->active) return E_NOEXS;

    pthread_mutex_lock(&sem->mutex);
    sem->active = FALSE;
    pthread_cond_broadcast(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);

    pthread_mutex_destroy(&sem->mutex);
    pthread_cond_destroy(&sem->cond);
    return E_OK;
#else
    g_tk_sems[semid - 1].active = FALSE;
    return E_OK;
#endif
}

ER get_tim(SYSTIME *p_time) {
    if (!p_time) return E_PAR;

#ifndef BTRON_QEMU_TARGET
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *p_time = (SYSTIME)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    return E_OK;
#else
    *p_time = ++g_tk_system_ticks;
    return E_OK;
#endif
}

void dly_tsk(W dlytim) {
#ifndef BTRON_QEMU_TARGET
    if (dlytim > 0) {
        usleep(dlytim * 1000);
    }
#else
    (void)dlytim;
#endif
}

ID tkernel_cre_tsk(const T_CTSK *pk_ctsk) {
    return cre_tsk(pk_ctsk);
}

ER tkernel_sta_tsk(ID tskid, VW exinf) {
    return sta_tsk(tskid, exinf);
}

void tkernel_dispatch(void) {
}
