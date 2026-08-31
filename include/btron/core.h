#ifndef _BTRON_CORE_H_
#define _BTRON_CORE_H_

#include <stdint.h>
#include <stddef.h>

#ifndef _BTRON_TYPES_H_
#ifndef _COLOR_DEFINED_
#define _COLOR_DEFINED_
typedef uint32_t COLOR;
#endif
#endif

#ifndef COLOR_CYAN
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_BLACK     0xFF000000
#define COLOR_RED       0xFFFF3B30
#define COLOR_GREEN     0xFF34C759
#define COLOR_YELLOW    0xFFFFCC00
#define COLOR_CYAN      0xFF00C0FF
#define COLOR_LTGRAY    0xFFD0D0D0
#define COLOR_DKGRAY    0xFF808080
#endif

typedef void (*ShellOutputFn)(const char *line, COLOR col, void *user_data);

/* Polymorphic Core Operations */
void btron_core_init(void);
void btron_core_print_ver(ShellOutputFn out_fn, void *user_data, const char *arg);

#endif /* _BTRON_CORE_H_ */
