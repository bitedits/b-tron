/*
 * BTRON 3.20 Conformance Verifier — Main Runner
 * btron_verify.c
 *
 * Registers all suites, runs them, and prints the result table.
 * Exit code 0 = all tests passed, non-zero = failure count.
 */

#include "btron_verify.h"
#include <stdlib.h>

/* ── Global State ──────────────────────────────────────────────── */

VFY_STATE vfy_state;
VFY_SUITE vfy_suites[VFY_MAX_SUITES];
int       vfy_suite_count = 0;

/* ── Suite Registration ────────────────────────────────────────── */

void vfy_register_suite(const char *name, VFY_SUITE_FN fn)
{
    if (vfy_suite_count < VFY_MAX_SUITES) {
        vfy_suites[vfy_suite_count].name = name;
        vfy_suites[vfy_suite_count].fn   = fn;
        vfy_suite_count++;
    }
}

/* ── Recording ─────────────────────────────────────────────────── */

void vfy_record(const char *suite, const char *entity,
                int passed, const char *fmt, ...)
{
    if (vfy_state.count >= VFY_MAX_RESULTS) return;

    VFY_RESULT *r = &vfy_state.results[vfy_state.count++];
    r->suite  = suite;
    r->entity = entity;
    r->passed = passed;

    if (passed) {
        r->diag[0] = '\0';
        vfy_state.pass_count++;
    } else {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(r->diag, VFY_DIAG_LEN, fmt, ap);
        va_end(ap);
        vfy_state.fail_count++;
    }
}

/* ── Main ──────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    int csv_mode = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) csv_mode = 1;
    }

    /* Clear state */
    memset(&vfy_state, 0, sizeof(vfy_state));
    vfy_suite_count = 0;

    /* Register all suites */
    vfy_register_suite("Types",        vfy_suite_types);
    vfy_register_suite("ErrorCodes",   vfy_suite_errors);
    vfy_register_suite("Process",      vfy_suite_proc);
    vfy_register_suite("uITRON",       vfy_suite_itron);
    vfy_register_suite("Memory",       vfy_suite_memory);
    vfy_register_suite("Clock",        vfy_suite_clk);
    vfy_register_suite("Device",       vfy_suite_device);
    vfy_register_suite("FileSystem",   vfy_suite_fs);
    vfy_register_suite("SysMgmt",       vfy_suite_sys);
    vfy_register_suite("Message",      vfy_suite_message);
    vfy_register_suite("VirtualObject",vfy_suite_vobj);
    vfy_register_suite("DisplayPrim",  vfy_suite_dp);
    vfy_register_suite("WindowMgr",    vfy_suite_wnd);
    vfy_register_suite("HMI",          vfy_suite_hmi);

    /* Run all suites */
    for (int i = 0; i < vfy_suite_count; i++) {
        vfy_suites[i].fn();
    }

    /* Output */
    if (csv_mode) {
        vfy_print_csv(stdout);
    } else {
        vfy_print_table(stdout);
    }

    return vfy_state.fail_count;
}
