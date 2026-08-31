/*
 * B-TRON Specification Compatible Header: core.h
 * Real-Time Kernel Core API Declarations.
 */

#ifndef _BTRON_CORE_H_
#define _BTRON_CORE_H_

#include <btron/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ShellOutputFn)(const char *line, COLOR col, void *user_data);

void btron_core_init(void);
void btron_core_print_ver(ShellOutputFn out_fn, void *user_data, const char *arg);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_CORE_H_ */
