/*
 * B-TRON Real-Time Kernel: POSIX Microkernel Abstraction Engine (core_posix.c)
 * Pure POSIX pthread-backed implementation of µITRON & T-Kernel specification APIs.
 */

#include <btron/itron.h>
#include <device/virtio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_TASKS 64
#define MAX_SEMS  64

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

void btron_posix_kernel_init(void) {
    pthread_mutex_lock(&g_kernel_mutex);
    memset(g_tasks, 0, sizeof(g_tasks));
    memset(g_sems, 0, sizeof(g_sems));
    pthread_mutex_unlock(&g_kernel_mutex);
    printf("\n==========================================================\n");
    printf(" POSIX Microkernel Abstraction Engine (Host PC Mode)\n");
    printf(" Target Mode 0: BTRON_POSIX Active\n");
    printf("==========================================================\n\n");
}

void tkernel_init(void) {
    btron_posix_kernel_init();
}

ID cre_tsk(const T_CTSK *pk_ctsk) {
    if (!pk_ctsk || !pk_ctsk->task) return E_PAR;
    
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
}

static void* task_wrapper(void *arg) {
    ITRON_TASK *tsk = (ITRON_TASK*)arg;
    if (tsk && tsk->config.task) {
        tsk->config.task(tsk->config.exinf);
    }
    tsk->active = FALSE;
    return NULL;
}

ER sta_tsk(ID tskid, VW exinf) {
    if (tskid <= 0 || tskid > MAX_TASKS) return E_ID;
    
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
}

void ext_tsk(void) {
    pthread_exit(NULL);
}

ER slp_tsk(void) {
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
}

ER wup_tsk(ID tskid) {
    if (tskid <= 0 || tskid > MAX_TASKS) return E_ID;

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
}

ID cre_sem(const T_CSEM *pk_csem) {
    if (!pk_csem) return E_PAR;

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
}

ER wai_sem(ID semid) {
    if (semid <= 0 || semid > MAX_SEMS) return E_ID;
    ITRON_SEM *sem = &g_sems[semid - 1];
    if (!sem->active) return E_NOEXS;

    pthread_mutex_lock(&sem->mutex);
    while (sem->count <= 0) {
        pthread_cond_wait(&sem->cond, &sem->mutex);
    }
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);

    return E_OK;
}

ER sig_sem(ID semid) {
    if (semid <= 0 || semid > MAX_SEMS) return E_ID;
    ITRON_SEM *sem = &g_sems[semid - 1];
    if (!sem->active) return E_NOEXS;

    pthread_mutex_lock(&sem->mutex);
    if (sem->count < sem->config.maxsem) {
        sem->count++;
        pthread_cond_signal(&sem->cond);
    }
    pthread_mutex_unlock(&sem->mutex);

    return E_OK;
}

ER del_sem(ID semid) {
    if (semid <= 0 || semid > MAX_SEMS) return E_ID;
    ITRON_SEM *sem = &g_sems[semid - 1];
    if (!sem->active) return E_NOEXS;

    pthread_mutex_lock(&sem->mutex);
    sem->active = FALSE;
    pthread_cond_broadcast(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);

    pthread_mutex_destroy(&sem->mutex);
    pthread_cond_destroy(&sem->cond);
    return E_OK;
}

ER get_tim(SYSTIME *p_time) {
    if (!p_time) return E_PAR;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *p_time = (SYSTIME)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    return E_OK;
}

void dly_tsk(W dlytim) {
    if (dlytim > 0) {
        usleep(dlytim * 1000);
    }
}

/* T-Kernel function aliases */
ID tkernel_cre_tsk(const T_CTSK *pk_ctsk) {
    return cre_tsk(pk_ctsk);
}

ER tkernel_sta_tsk(ID tskid, VW exinf) {
    return sta_tsk(tskid, exinf);
}

void tkernel_dispatch(void) {
    /* POSIX scheduling managed by OS pthreads */
}
