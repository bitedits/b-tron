/*
 * B-TRON Specification Compatible Header: core.h
 * Real-Time Kernel Core API Declarations.
 */

#ifndef _BTRON_CORE_H_
#define _BTRON_CORE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _COLOR_DEFINED_
#define _COLOR_DEFINED_
typedef uint32_t COLOR;
#endif

#ifndef COLOR_BLACK
#define COLOR_BLACK     0xFF000000
#endif
#ifndef COLOR_WHITE
#define COLOR_WHITE     0xFFFFFFFF
#endif
#ifndef COLOR_DKGRAY
#define COLOR_DKGRAY    0xFF404040
#endif
#ifndef COLOR_GRAY
#define COLOR_GRAY      0xFF808080
#endif
#ifndef COLOR_LTGRAY
#define COLOR_LTGRAY    0xFFD4D0C8
#endif
#ifndef COLOR_TEAL
#define COLOR_TEAL      0xFF008080
#endif
#ifndef COLOR_NAVY
#define COLOR_NAVY      0xFF000080
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE      0xFF0000FF
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW    0xFFFFFF00
#endif
#ifndef COLOR_RED
#define COLOR_RED       0xFFFF0000
#endif
#ifndef COLOR_GREEN
#define COLOR_GREEN     0xFF00C040
#endif
#ifndef COLOR_CYAN
#define COLOR_CYAN      0xFF00FFFF
#endif
#ifndef COLOR_GOLD
#define COLOR_GOLD      0xFFFFCC00
#endif

typedef void (*ShellOutputFn)(const char *line, COLOR col, void *user_data);

void btron_core_init(void);
void btron_core_print_ver(ShellOutputFn out_fn, void *user_data, const char *arg);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_CORE_H_ */
