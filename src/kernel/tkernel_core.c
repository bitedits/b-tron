/*
 * T-Kernel 2.0 / ITRON RTOS Kernel Core
 * Imported & Adapted from ./tkernel_source/ for Bare-Metal & QEMU Targets
 */

#include <btron/itron.h>
#include "virtio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TK_TASKS 64
#define MAX_TK_SEMS  64
#define MAX_TK_FLGS  32

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

void tkernel_init(void) {
    (void)g_current_tskid;
    memset(g_tk_tasks, 0, sizeof(g_tk_tasks));
    memset(g_tk_sems, 0, sizeof(g_tk_sems));
    printf("[T-KERNEL] Real-Time ITRON Core initialized (Raspberry Pi / QEMU target).\n");
    virtio_mmio_init(0x10001000);
}

ID tkernel_cre_tsk(const T_CTSK *pk_ctsk) {
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

ER tkernel_sta_tsk(ID tskid, VW exinf) {
    if (tskid <= 0 || tskid > MAX_TK_TASKS) return E_ID;
    TK_TCB *tcb = &g_tk_tasks[tskid - 1];
    if (tcb->state == TK_TS_NONEXS) return E_NOEXS;

    tcb->config.exinf = exinf;
    tcb->state = TK_TS_READY;
    return E_OK;
}

void tkernel_dispatch(void) {
    /* T-Kernel preemptive priority dispatcher loop */
}
