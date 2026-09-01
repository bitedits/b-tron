/*
 * B-TRON Specification Compatible Header: memory.h
 * BTRON 3.20 Memory Management Subsystem.
 */

#ifndef _BTRON_MEMORY_H_
#define _BTRON_MEMORY_H_

#include <btron/types.h>
#include <btron/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Domain Attributes ─────────────────────────────────────────── */
#define M_LOCAL      0x00000000U  /* Local process memory */
#define M_COMMON     0x00000001U  /* Shared / common memory */
#define M_SYSTEM     0x00000003U  /* Kernel system memory */

/* ── Memory Block Attributes ───────────────────────────────────── */
#define M_RESIDENT   0x00004000U  /* Non-pageable / pinned */
#define DELEXIT      0x00008000U  /* Auto-free on process termination */
#define M_READ       0x00010000U  /* Read permission */
#define M_WRITE      0x00020000U  /* Write permission */
#define M_EXEC       0x00040000U  /* Execute permission */

/* ── Memory Status Structure ───────────────────────────────────── */
typedef struct m_state {
    W blksz;   /* Block unit size in bytes (e.g. 4096) */
    W total;   /* Total blocks managed */
    W free;    /* Free blocks available */
} M_STATE;

/* ── Memory Management APIs ────────────────────────────────────── */
ER get_mbk(VP *adr, W nblk, UW atr);
ER rel_mbk(VP adr);
ER chg_mbk(VP adr, W nblk);
ER ref_mbk(VP adr, M_STATE *pk_state);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_MEMORY_H_ */
