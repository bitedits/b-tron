/*
 * B-TRON IPC Message Passing Engine: ipc_msg.c
 * Cleanroom implementation of BTRON 3.20 Message Queueing System.
 */

#include <btron/message.h>
#include <btron/types.h>
#include <btron/error.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_QUEUED_MSGS 64
#define MAX_IPC_PIDS    32

typedef struct {
    MESSAGE messages[MAX_QUEUED_MSGS];
    int     head;
    int     tail;
    int     count;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    BOOL    active;
} IPC_MAILBOX;

static IPC_MAILBOX g_mailboxes[MAX_IPC_PIDS];
static pthread_mutex_t g_ipc_init_lock = PTHREAD_MUTEX_INITIALIZER;
static BOOL g_ipc_initialized = FALSE;

static void ipc_ensure_init(void) {
    pthread_mutex_lock(&g_ipc_init_lock);
    if (!g_ipc_initialized) {
        for (int i = 0; i < MAX_IPC_PIDS; i++) {
            memset(&g_mailboxes[i], 0, sizeof(IPC_MAILBOX));
            pthread_mutex_init(&g_mailboxes[i].lock, NULL);
            pthread_cond_init(&g_mailboxes[i].cond, NULL);
            g_mailboxes[i].active = TRUE;
        }
        g_ipc_initialized = TRUE;
    }
    pthread_mutex_unlock(&g_ipc_init_lock);
}

ER snd_msg(W pid, const MESSAGE *msg) {
    if (!msg) return E_PAR;
    if (pid < 0 || pid >= MAX_IPC_PIDS) return ER_ID;
    if (msg->msg_type < 1 || msg->msg_type > 31) return E_PAR;

    ipc_ensure_init();

    IPC_MAILBOX *mb = &g_mailboxes[pid];
    pthread_mutex_lock(&mb->lock);

    if (mb->count >= MAX_QUEUED_MSGS) {
        pthread_mutex_unlock(&mb->lock);
        return ER_NOSPC;
    }

    mb->messages[mb->tail] = *msg;
    mb->tail = (mb->tail + 1) % MAX_QUEUED_MSGS;
    mb->count++;

    pthread_cond_signal(&mb->cond);
    pthread_mutex_unlock(&mb->lock);

    return E_OK;
}

ER rcv_msg(W pid, MESSAGE *msg, UW mask, W tmo) {
    if (!msg) return E_PAR;
    if (pid < 0 || pid >= MAX_IPC_PIDS) return ER_ID;

    ipc_ensure_init();

    IPC_MAILBOX *mb = &g_mailboxes[pid];
    pthread_mutex_lock(&mb->lock);

    struct timespec ts;
    if (tmo > 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        ts.tv_sec = now.tv_sec + (tmo / 1000);
        ts.tv_nsec = (now.tv_usec + (tmo % 1000) * 1000) * 1000;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
    }

    while (1) {
        /* Search for matching message in queue */
        for (int i = 0; i < mb->count; i++) {
            int idx = (mb->head + i) % MAX_QUEUED_MSGS;
            UW mtype = mb->messages[idx].msg_type;
            if ((mask == 0) || (mask & MSGMASK(mtype))) {
                *msg = mb->messages[idx];

                /* Remove from queue by shifting forward */
                for (int j = i; j < mb->count - 1; j++) {
                    int from = (mb->head + j + 1) % MAX_QUEUED_MSGS;
                    int to   = (mb->head + j) % MAX_QUEUED_MSGS;
                    mb->messages[to] = mb->messages[from];
                }
                mb->count--;
                mb->tail = (mb->head + mb->count) % MAX_QUEUED_MSGS;

                pthread_mutex_unlock(&mb->lock);
                return E_OK;
            }
        }

        /* If no match and timeout is 0 (polling), return timeout */
        if (tmo == 0) {
            pthread_mutex_unlock(&mb->lock);
            return E_TMOUT;
        }

        /* Wait for new message */
        if (tmo > 0) {
            int res = pthread_cond_timedwait(&mb->cond, &mb->lock, &ts);
            if (res == ETIMEDOUT) {
                pthread_mutex_unlock(&mb->lock);
                return E_TMOUT;
            }
        } else {
            pthread_cond_wait(&mb->cond, &mb->lock);
        }
    }
}

ER chk_msg(W pid, MESSAGE *msg, UW mask) {
    return rcv_msg(pid, msg, mask, 0);
}
