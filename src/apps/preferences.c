/*
 * B-System (BTRON 3.20) System Preferences (src/apps/preferences.c)
 * Desktop, Appearance, TIP/IME, and System Settings Manager
 */

#include <btron/wnd.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

typedef struct {
    char current_theme[32];
    int tip_engine_mode;
    int display_scaling;
} SysPreferences;
