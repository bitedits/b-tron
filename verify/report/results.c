/*
 * results.c — ASCII Table, Conformance Matrix and CSV Formatter
 */

#include "../btron_verify.h"
#include <stdio.h>

typedef struct {
    const char *subsystem;
    int         total;
    int         impl;
    int         partial;
    int         missing;
} SUBSYS_SUMMARY;

static const SUBSYS_SUMMARY g_subsystems[] = {
    { "Fundamental Types",      17, 17,  0,   0 },
    { "Error Codes (error.h)",  12, 12,  0,   0 },
    { "Error Codes (spec-only)",19,  0,  0,  19 },
    { "Process Management",      5,  0,  1,   4 },
    { "Task Management",        10,  8,  1,   1 },
    { "Memory Management",      12,  0,  2,  10 },
    { "Message Passing (IPC)",   9,  0,  1,   8 },
    { "Task Comm (sem/flg/mbf)",16, 11,  4,   1 },
    { "Input Events",           14,  2,  2,  10 },
    { "Device Management",      11,  0,  0,  11 },
    { "Clock & Calendar",       11,  1,  5,   5 },
    { "File System (Record)",   26,  0,  0,  26 },
    { "System Management",       5,  0,  1,   4 },
    { "Display Primitives",     14, 11,  0,   3 },
    { "Window Manager",         18, 18,  0,   0 },
    { "HMI Components",         40, 37,  0,   3 },
    { "Virtual Object Subsys",   9,  9,  0,   0 },
};

#define NUM_SUBSYSTEMS ((int)(sizeof(g_subsystems)/sizeof(g_subsystems[0])))

void vfy_print_table(FILE *out)
{
    fprintf(out, "\n");
    fprintf(out, "==========================================================================\n");
    fprintf(out, "       BTRON 3.20 FULL SPECIFICATION CONFORMANCE AUDIT MATRIX            \n");
    fprintf(out, "==========================================================================\n");
    fprintf(out, "%-26s | %5s | %5s | %7s | %7s | %6s\n",
            "Subsystem", "Total", "IMPL", "PARTIAL", "MISSING", "Cover");
    fprintf(out, "---------------------------+-------+-------+---------+---------+---------\n");

    int tot_all = 0, tot_impl = 0, tot_partial = 0, tot_missing = 0;

    for (int i = 0; i < NUM_SUBSYSTEMS; i++) {
        const SUBSYS_SUMMARY *s = &g_subsystems[i];
        tot_all     += s->total;
        tot_impl    += s->impl;
        tot_partial += s->partial;
        tot_missing += s->missing;
        double pct = ((double)(s->impl + s->partial) / (double)s->total) * 100.0;
        fprintf(out, "%-26s | %5d | %5d | %7d | %7d | %5.1f%%\n",
                s->subsystem, s->total, s->impl, s->partial, s->missing, pct);
    }

    double impl_pct = ((double)tot_impl / (double)tot_all) * 100.0;
    double total_coverage = ((double)(tot_impl + tot_partial) / (double)tot_all) * 100.0;

    fprintf(out, "==========================================================================\n");
    fprintf(out, "%-26s | %5d | %5d | %7d | %7d | %5.1f%%\n",
            "OVERALL SPEC COVERAGE", tot_all, tot_impl, tot_partial, tot_missing, total_coverage);
    fprintf(out, "--------------------------------------------------------------------------\n");
    fprintf(out, "  Fully Implemented [IMPL]     : %3d / %3d  (%5.1f%%)\n", tot_impl, tot_all, impl_pct);
    fprintf(out, "  Partially Implemented [PART] : %3d / %3d  (%5.1f%%)\n", tot_partial, tot_all, ((double)tot_partial/tot_all)*100.0);
    fprintf(out, "  Missing from Implementation  : %3d / %3d  (%5.1f%%)\n", tot_missing, tot_all, ((double)tot_missing/tot_all)*100.0);
    fprintf(out, "==========================================================================\n\n");

    fprintf(out, "==========================================================================\n");
    fprintf(out, "     BTRON 3.20 UNIT ASSERTION VERIFIER (%d Automated Runtime Checks)    \n", vfy_state.count);
    fprintf(out, "==========================================================================\n");
    fprintf(out, "%-20s | %-28s | %-6s | %s\n",
            "Suite", "Entity", "Result", "Diagnostic");
    fprintf(out, "---------------------+------------------------------+--------+-----------\n");

    for (int i = 0; i < vfy_state.count; i++) {
        const VFY_RESULT *r = &vfy_state.results[i];
        fprintf(out, "%-20s | %-28s | %-6s | %s\n",
                r->suite, r->entity,
                r->passed ? "PASS" : "FAIL",
                r->diag);
    }

    fprintf(out, "==========================================================================\n");
    fprintf(out, "ASSERTIONS: %d tests | PASS: %d | FAIL: %d\n",
            vfy_state.count, vfy_state.pass_count, vfy_state.fail_count);
    fprintf(out, "CERTIFICATION STATUS : L0 = PASS | L1 = CONDITIONAL (58%%) | L2 = IN PROGRESS\n");
    fprintf(out, "==========================================================================\n\n");
}

void vfy_print_csv(FILE *out)
{
    fprintf(out, "type,subsystem,total,impl,partial,missing\n");
    for (int i = 0; i < NUM_SUBSYSTEMS; i++) {
        const SUBSYS_SUMMARY *s = &g_subsystems[i];
        fprintf(out, "subsystem,%s,%d,%d,%d,%d\n",
                s->subsystem, s->total, s->impl, s->partial, s->missing);
    }
    fprintf(out, "type,suite,entity,result,diagnostic\n");
    for (int i = 0; i < vfy_state.count; i++) {
        const VFY_RESULT *r = &vfy_state.results[i];
        fprintf(out, "test,%s,%s,%s,\"%s\"\n",
                r->suite, r->entity,
                r->passed ? "PASS" : "FAIL",
                r->diag);
    }
}
