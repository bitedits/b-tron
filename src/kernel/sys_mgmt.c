/*
 * B-TRON System Management & Configuration: sys_mgmt.c
 * Cleanroom implementation of BTRON 3.20 System Calls.
 */

#include <btron/sys_mgmt.h>
#include <btron/types.h>
#include <btron/error.h>
#include <string.h>

static FP g_exc_handlers[32];
static char g_sys_name[64] = "BTRON3 Sakamura T-Kernel 2.0 (Cleanroom C99)";
static int g_max_procs = 32;

ER def_exc(W exc_code, FP handler) {
    if (exc_code < 0 || exc_code >= 32) return ER_PAR;
    g_exc_handlers[exc_code] = handler;
    return E_OK;
}

void ret_exc(void) {
    /* Return from exception / trap context */
}

ER get_cnf(W key, VP val, W *sz) {
    if (!val) return ER_PAR;

    if (key == CNF_SYS_NAME) {
        int len = (int)strlen(g_sys_name) + 1;
        memcpy(val, g_sys_name, len);
        if (sz) *sz = len;
        return E_OK;
    } else if (key == CNF_MAX_PROCS) {
        *(int*)val = g_max_procs;
        if (sz) *sz = sizeof(int);
        return E_OK;
    }

    return ER_PAR;
}

ER set_cnf(W key, const VP val, W sz) {
    (void)sz;
    if (!val) return ER_PAR;

    if (key == CNF_SYS_NAME) {
        strncpy(g_sys_name, (const char*)val, sizeof(g_sys_name) - 1);
        return E_OK;
    } else if (key == CNF_MAX_PROCS) {
        g_max_procs = *(const int*)val;
        return E_OK;
    }

    return ER_PAR;
}
