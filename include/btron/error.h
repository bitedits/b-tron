/*
 * B-TRON Specification Compatible Header: error.h
 * Standard TRON Error Codes (BTRON 3.20 Complete Normative Set).
 */

#ifndef _BTRON_ERROR_H_
#define _BTRON_ERROR_H_

#include <btron/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Standard Error Codes (Defined in Base Specification) ────────── */
#define E_OK        (0)   /* Normal completion */
#define E_SYS      (-5)   /* System error */
#define E_NOMEM    (-10)  /* Out of memory */
#define E_NOSPT    (-17)  /* Feature not supported */
#define E_RSVR     (-25)  /* Reserved error */
#define E_PAR      (-33)  /* Parameter error */
#define E_LIMIT    (-34)  /* Limit exceeded */
#define E_ID       (-35)  /* Invalid ID */
#define E_OBJ      (-41)  /* Invalid object state */
#define E_NOEXS    (-52)  /* Object does not exist */
#define E_BUSY     (-65)  /* Resource busy */
#define E_TMOUT    (-69)  /* Timeout */

/* ── Extended BTRON 3.20 Subsystem Error Codes ──────────────────── */
#define ER_ADR     (-1)   /* Invalid pointer address / memory address */
#define ER_NOSPC   (-11)  /* Insufficient memory / storage space */
#define ER_ACCES   (-12)  /* Insufficient privilege / access denied */
#define ER_IO      (-13)  /* Hardware I/O device error */
#define ER_DID     (-14)  /* Invalid device descriptor / device ID */
#define ER_ROVR    (-15)  /* Read-only violation */
#define ER_DLT     (-16)  /* Object deleted during waiting state */
#define ER_CTX     (-18)  /* Invalid calling context / task context */
#define ER_FD      (-19)  /* Invalid file descriptor */
#define ER_NOFS    (-20)  /* File system not mounted */
#define ER_NODSK   (-21)  /* No disk space available */
#define ER_RONLY   (-22)  /* Volume or medium is read-only */
#define ER_FNAME   (-23)  /* Invalid file or path name */
#define ER_EXS     (-24)  /* File or directory already exists */
#define ER_PWD     (-26)  /* Wrong password / authentication failed */
#define ER_PERM    (-27)  /* Permission denied */
#define ER_OVRW    (-28)  /* Write-protected configuration / overwrite prevented */
#define ER_OVVR    (-29)  /* Resource overflow (semaphore/queue max exceeded) */
#define ER_TIMEOUT (-69)  /* Timeout (BTRON specification alias) */

/* ── Standard BTRON Alias Macros ─────────────────────────────────── */
#define ER_OK      E_OK
#define ER_SYS     E_SYS
#define ER_NOMEM   E_NOMEM
#define ER_NOSPT   E_NOSPT
#define ER_RSVR    E_RSVR
#define ER_PAR     E_PAR
#define ER_LIMIT   E_LIMIT
#define ER_ID      E_ID
#define ER_OBJ     E_OBJ
#define ER_NOEXS   E_NOEXS
#define ER_BUSY    E_BUSY
#define ER_TMOUT   E_TMOUT

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_ERROR_H_ */
