/*
 * B-TRON Specification Compatible Header: message.h
 * BTRON 3.20 Inter-Process Communication (IPC) Message Engine.
 */

#ifndef _BTRON_MESSAGE_H_
#define _BTRON_MESSAGE_H_

#include <btron/types.h>
#include <btron/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Standard System Message Types (1..31) ─────────────────────── */
#define MS_ABORT       1   /* Abort process request */
#define MS_EXIT        2   /* Process exit notification */
#define MS_TERM        3   /* Process terminate request */
#define MS_TMOUT       4   /* Timer timeout notification */
#define MS_SYSEVT      5   /* System event notification */

/* User defined message types */
#define MS_TYPE1      25
#define MS_TYPE2      26
#define MS_TYPE3      27
#define MS_TYPE4      28
#define MS_TYPE5      29
#define MS_TYPE6      30
#define MS_TYPE7      31

#define MSGMASK(t)    (1U << ((t) - 1))
#define MSGMASK_ALL   0xFFFFFFFFU

/* ── Message Payload Union ─────────────────────────────────────── */
typedef union {
    struct { W pid; W code; } ABORT;
    struct { W pid; W code; } EXIT;
    struct { W pid; W code; } TERM;
    struct { W code; }        TMOUT;
    struct { W code; }        SYSEVT;
    struct { UB msg_str[32]; } ANYMSG;
    struct { W  param[8]; }   RAW;
} MSGBODY;

/* ── Standard Message Envelope ─────────────────────────────────── */
typedef struct message {
    W       msg_type;   /* Message type (1..31) */
    W       msg_size;   /* Size of payload bytes in msg_body */
    MSGBODY msg_body;   /* Payload data */
} MESSAGE;

/* ── IPC Message Passing APIs ──────────────────────────────────── */
ER snd_msg(W pid, const MESSAGE *msg);
ER rcv_msg(W pid, MESSAGE *msg, UW mask, W tmo);
ER chk_msg(W pid, MESSAGE *msg, UW mask);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_MESSAGE_H_ */
