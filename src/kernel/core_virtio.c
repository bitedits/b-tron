/*
 * B-Kernel / ITRON T-Kernel RTOS Kernel Core: core_virtio.c
 * Bare-Metal & QEMU Target Implementation (ARM / VirtIO MMIO)
 */

#include <btron/itron.h>
#include <device/virtio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TK_TASKS 64
#define MAX_TK_SEMS  64

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

static TK_TCB  g_tk_tasks[MAX_TK_TASKS];
static TK_SEMB g_tk_sems[MAX_TK_SEMS];
static ID      g_current_tskid = 1;
static SYSTIME g_tk_system_ticks = 0;

void tkernel_init(void) {
    (void)g_current_tskid;
    memset(g_tk_tasks, 0, sizeof(g_tk_tasks));
    memset(g_tk_sems, 0, sizeof(g_tk_sems));
    printf("\n==========================================================\n");
    printf(" B-Kernel / ITRON RTOS Core (QEMU VirtIO Mode)\n");
    printf(" Target Mode 1: BTRON_QEMU Active\n");
    printf("==========================================================\n\n");
    virtio_mmio_init(0x10001000);
}

void btron_posix_kernel_init(void) {
    tkernel_init();
}

ID cre_tsk(const T_CTSK *pk_ctsk) {
    if (!pk_ctsk || !pk_ctsk->task) return E_PAR;

    for (int i = 0; i < MAX_TK_TASKS; i++) {
        if (g_tk_tasks[i].state == TK_TS_NONEXS) {
            g_tk_tasks[i].tskid = i + 1;
            g_tk_tasks[i].config = *pk_ctsk;
            g_tk_tasks[i].state = TK_TS_DORM;
            g_tk_tasks[i].current_pri = pk_ctsk->itskpri;
            return g_tk_tasks[i].tskid;
        }
    }
    return E_NOMEM;
}

ER sta_tsk(ID tskid, VW exinf) {
    if (tskid <= 0 || tskid > MAX_TK_TASKS) return E_ID;
    TK_TCB *tcb = &g_tk_tasks[tskid - 1];
    if (tcb->state == TK_TS_NONEXS) return E_NOEXS;

    tcb->config.exinf = exinf;
    tcb->state = TK_TS_READY;
    return E_OK;
}

void ext_tsk(void) {
    if (g_current_tskid > 0 && g_current_tskid <= MAX_TK_TASKS) {
        g_tk_tasks[g_current_tskid - 1].state = TK_TS_DORM;
    }
}

ER slp_tsk(void) {
    if (g_current_tskid > 0 && g_current_tskid <= MAX_TK_TASKS) {
        g_tk_tasks[g_current_tskid - 1].state = TK_TS_WAIT;
    }
    return E_OK;
}

ER wup_tsk(ID tskid) {
    if (tskid <= 0 || tskid > MAX_TK_TASKS) return E_ID;
    TK_TCB *tcb = &g_tk_tasks[tskid - 1];
    if (tcb->state == TK_TS_WAIT) {
        tcb->state = TK_TS_READY;
    }
    return E_OK;
}

ID cre_sem(const T_CSEM *pk_csem) {
    if (!pk_csem) return E_PAR;
    for (int i = 0; i < MAX_TK_SEMS; i++) {
        if (!g_tk_sems[i].active) {
            g_tk_sems[i].semid = i + 1;
            g_tk_sems[i].config = *pk_csem;
            g_tk_sems[i].count = pk_csem->isemcnt;
            g_tk_sems[i].active = TRUE;
            return g_tk_sems[i].semid;
        }
    }
    return E_NOMEM;
}

ER wai_sem(ID semid) {
    if (semid <= 0 || semid > MAX_TK_SEMS) return E_ID;
    TK_SEMB *sem = &g_tk_sems[semid - 1];
    if (!sem->active) return E_NOEXS;
    if (sem->count > 0) {
        sem->count--;
        return E_OK;
    }
    return E_TMOUT;
}

ER sig_sem(ID semid) {
    if (semid <= 0 || semid > MAX_TK_SEMS) return E_ID;
    TK_SEMB *sem = &g_tk_sems[semid - 1];
    if (!sem->active) return E_NOEXS;
    if (sem->count < sem->config.maxsem) {
        sem->count++;
    }
    return E_OK;
}

ER del_sem(ID semid) {
    if (semid <= 0 || semid > MAX_TK_SEMS) return E_ID;
    g_tk_sems[semid - 1].active = FALSE;
    return E_OK;
}

ER get_tim(SYSTIME *p_time) {
    if (!p_time) return E_PAR;
    *p_time = ++g_tk_system_ticks;
    return E_OK;
}

void dly_tsk(W dlytim) {
    (void)dlytim;
}

ID tkernel_cre_tsk(const T_CTSK *pk_ctsk) {
    return cre_tsk(pk_ctsk);
}

ER tkernel_sta_tsk(ID tskid, VW exinf) {
    return sta_tsk(tskid, exinf);
}

void tkernel_dispatch(void) {
    /* Preemptive priority dispatcher loop */
}
