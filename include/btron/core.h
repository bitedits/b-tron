/*
 * B-TRON Specification Compatible Header: core.h
 * Real-Time Kernel Core API Declarations.
 */

#ifndef _BTRON_CORE_H_
#define _BTRON_CORE_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _COLOR_DEFINED_
#define _COLOR_DEFINED_
typedef uint32_t COLOR;
#endif

#ifndef COLOR_BLACK
#define COLOR_BLACK     0xFF000000
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_DKGRAY    0xFF404040
#define COLOR_GRAY      0xFF808080
#define COLOR_LTGRAY    0xFFD4D0C8
#define COLOR_RED       0xFFFF0000
#define COLOR_GREEN     0xFF00C040
#define COLOR_YELLOW    0xFFFFFF00
#define COLOR_CYAN      0xFF00FFFF
#endif

typedef void (*ShellOutputFn)(const char *line, COLOR col, void *user_data);

void btron_core_banner(void);
void btron_core_init(void);
void btron_core_mem_log(void);
void btron_core_hfds_log(void);
void btron_core_print_ver(ShellOutputFn out_fn, void *user_data, const char *arg);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_CORE_H_ */
