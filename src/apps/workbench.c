/*
 * B-System (BTRON 3.20) Workbench Desktop Shell (src/apps/workbench.c)
 * Desktop Manager, Menu Bar, and App Dispatcher
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#include <btron/event.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

void workbench_launch_app(const char *app_name) {
    if (!app_name) return;
}
