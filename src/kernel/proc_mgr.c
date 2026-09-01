/*
 * B-TRON Process Management Engine: proc_mgr.c
 * Cleanroom implementation of BTRON 3.20 Process Control.
 */

#include <btron/proc.h>
#include <btron/types.h>
#include <btron/error.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PROCESSES 32

typedef struct {
    ID      pid;
    W       priority;
    P_USER  user;
    LINK    link;
    BOOL    active;
    W       exit_code;
} BTRON_PROC;

static BTRON_PROC g_procs[MAX_PROCESSES];
static pthread_mutex_t g_proc_lock = PTHREAD_MUTEX_INITIALIZER;
static ID g_next_pid = 1;
static BOOL g_proc_init = FALSE;

static void proc_init_once(void) {
    if (!g_proc_init) {
        memset(g_procs, 0, sizeof(g_procs));
        /* PID 0 is root / system process */
        g_procs[0].pid = 0;
        g_procs[0].priority = 0;
        g_procs[0].active = TRUE;
        g_proc_init = TRUE;
    }
}

ID cre_prc(const LINK *lnk, W pri, const MESSAGE *msg) {
    if (pri < 0 || pri > 255) return ER_PAR;

    pthread_mutex_lock(&g_proc_lock);
    proc_init_once();

    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (!g_procs[i].active) {
            g_procs[i].pid = g_next_pid++;
            g_procs[i].priority = pri;
            if (lnk) g_procs[i].link = *lnk;
            g_procs[i].active = TRUE;
            g_procs[i].exit_code = 0;

            ID pid = g_procs[i].pid;
            pthread_mutex_unlock(&g_proc_lock);

            /* Send launch message if provided */
            if (msg && pid < 32) {
                snd_msg(pid, msg);
            }

            return pid;
        }
    }

    pthread_mutex_unlock(&g_proc_lock);
    return ER_NOSPC;
}

ER ter_prc(W pid, W code, W opt) {
    (void)opt;
    if (pid <= 0 || pid >= MAX_PROCESSES) return ER_ID;

    pthread_mutex_lock(&g_proc_lock);
    proc_init_once();

    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (g_procs[i].active && g_procs[i].pid == pid) {
            g_procs[i].active = FALSE;
            g_procs[i].exit_code = code;
            pthread_mutex_unlock(&g_proc_lock);
            return E_OK;
        }
    }

    pthread_mutex_unlock(&g_proc_lock);
    return ER_NOEXS;
}

ER chg_pri(W id, W pri, W opt) {
    (void)opt;
    if (pri < 0 || pri > 255) return ER_PAR;
    if (id < 0 || id >= MAX_PROCESSES) return ER_ID;

    pthread_mutex_lock(&g_proc_lock);
    proc_init_once();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_procs[i].active && g_procs[i].pid == id) {
            g_procs[i].priority = pri;
            pthread_mutex_unlock(&g_proc_lock);
            return E_OK;
        }
    }

    pthread_mutex_unlock(&g_proc_lock);
    return ER_NOEXS;
}

W get_tid(void) {
    return (W)1;
}

W get_pid(void) {
    return (W)0;
}
