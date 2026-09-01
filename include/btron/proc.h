/*
 * B-TRON Specification Compatible Header: proc.h
 * Process Management & Privilege Engine.
 */

#ifndef _BTRON_PROC_H_
#define _BTRON_PROC_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Process User & Group Privilege Information ────────────────── */
typedef struct {
    TC  usr_name[14];   /* User name (12 chars + 2 reserved) */
    TC  grp_name1[14];  /* Group 1 name */
    TC  grp_name2[14];  /* Group 2 name */
    TC  grp_name3[14];  /* Group 3 name */
    TC  grp_name4[14];  /* Group 4 name */
    W   level;          /* Privilege level 0..15 (0 = highest) */
    W   net_level;      /* Network privilege 1..15 */
} P_USER;

/* ── File System Link Reference for Executables ────────────────── */
typedef struct {
    ID    vol_id;       /* Volume ID */
    ID    rec_id;       /* Record ID */
    TC    path[32];     /* Path / file name */
    UW    flags;        /* Execution flags */
} LINK;

/* ── Process Lifecycle Functions ───────────────────────────────── */
ID   cre_prc(const LINK *lnk, W pri, const MESSAGE *msg);
ER   ter_prc(W pid, W code, W opt);
ER   chg_pri(W id, W pri, W opt);
W    get_tid(void);
W    get_pid(void);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_PROC_H_ */
