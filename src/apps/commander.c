/*
 * B-System (BTRON 3.20) 2-Pane Commander File Manager (src/apps/commander.c)
 * Dual-Panel High-Speed File Manipulator
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

typedef struct {
    char left_path[256];
    char right_path[256];
    int active_panel;
} CommanderState;
