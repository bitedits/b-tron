/*
 * B-TRON Specification Compatible Header: sys_mgmt.h
 * System Management, Configuration & Exception Handling.
 */

#ifndef _BTRON_SYS_MGMT_H_
#define _BTRON_SYS_MGMT_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/itron.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration Keys ────────────────────────────────────────── */
#define CNF_SYS_NAME     1
#define CNF_MAX_PROCS    2
#define CNF_DEF_LANG     3
#define CNF_TIMEZONE     4

/* ── System Management APIs ────────────────────────────────────── */
ER def_exc(W exc_code, FP handler);
void ret_exc(void);
ER get_cnf(W key, VP val, W *sz);
ER set_cnf(W key, const VP val, W sz);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_SYS_MGMT_H_ */
