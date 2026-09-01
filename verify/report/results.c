/*
 * results.c — ASCII Table, Dynamic Conformance Matrix and CSV Formatter
 *
 * Table 1 (Subsystem Matrix) is dynamically computed from Table 2 (Entity Clauses),
 * ensuring 100% 1-to-1 consistency between both tables.
 */

#include "../btron_verify.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[32];
    int  total;
    int  passed;
    int  failed;
} DYN_SUBSYS;

#define MAX_DYN_SUBSYS 32

void vfy_print_table(FILE *out)
{
    /* ── Dynamic Aggregation from Recorded Entity Clauses ──────── */
    DYN_SUBSYS subsys[MAX_DYN_SUBSYS];
    int num_subsys = 0;
    memset(subsys, 0, sizeof(subsys));

    for (int i = 0; i < vfy_state.count; i++) {
        const VFY_RESULT *r = &vfy_state.results[i];
        int found = -1;
        for (int s = 0; s < num_subsys; s++) {
            if (strcmp(subsys[s].name, r->suite) == 0) {
                found = s;
                break;
            }
        }
        if (found < 0 && num_subsys < MAX_DYN_SUBSYS) {
            found = num_subsys++;
            strncpy(subsys[found].name, r->suite, sizeof(subsys[found].name) - 1);
        }
        if (found >= 0) {
            subsys[found].total++;
            if (r->passed) {
                subsys[found].passed++;
            } else {
                subsys[found].failed++;
            }
        }
    }

    /* ── Table 1: Subsystem Conformance Matrix ──────────────────── */
    fprintf(out, "\n");
    fprintf(out, "==========================================================================\n");
    fprintf(out, "       BTRON 3.20 + HMI FULL SPECIFICATION CONFORMANCE AUDIT MATRIX       \n");
    fprintf(out, "==========================================================================\n");
    fprintf(out, "%-26s | %5s | %5s | %7s | %6s\n",
            "Subsystem / Suite", "Total", "PASS", "FAILED", "Rate");
    fprintf(out, "---------------------------+-------+-------+---------+---------\n");

    int tot_all = 0, tot_passed = 0, tot_failed = 0;

    for (int i = 0; i < num_subsys; i++) {
        const DYN_SUBSYS *s = &subsys[i];
        tot_all    += s->total;
        tot_passed += s->passed;
        tot_failed += s->failed;
        double pct = (s->total > 0) ? (((double)s->passed / (double)s->total) * 100.0) : 0.0;
        fprintf(out, "%-26s | %5d | %5d | %7d | %5.1f%%\n",
                s->name, s->total, s->passed, s->failed, pct);
    }

    double overall_pct = (tot_all > 0) ? (((double)tot_passed / (double)tot_all) * 100.0) : 0.0;

    fprintf(out, "==========================================================================\n");
    fprintf(out, "%-26s | %5d | %5d | %7d | %5.1f%%\n",
            "OVERALL CONFORMANCE", tot_all, tot_passed, tot_failed, overall_pct);
    fprintf(out, "--------------------------------------------------------------------------\n");
    fprintf(out, "  Passed Clauses  [PASS] : %3d / %3d  (%5.1f%%)\n", tot_passed, tot_all, overall_pct);
    fprintf(out, "  Failed Clauses  [FAIL] : %3d / %3d  (%5.1f%%)\n", tot_failed, tot_all, (tot_all > 0) ? (((double)tot_failed / (double)tot_all) * 100.0) : 0.0);
    fprintf(out, "==========================================================================\n\n");

    /* ── Table 2: Detailed Entity & Clause Verifier ────────────── */
    fprintf(out, "==========================================================================\n");
    fprintf(out, "       BTRON 3.20 + HMI SPECIFICATION ENTITY & CLAUSE VERIFIER            \n");
    fprintf(out, "==========================================================================\n");
    fprintf(out, "%-20s | %-32s | %-6s | %s\n",
            "Subsystem", "Entity / Clause", "Result", "Diagnostic");
    fprintf(out, "---------------------+----------------------------------+--------+-----------\n");

    for (int i = 0; i < vfy_state.count; i++) {
        const VFY_RESULT *r = &vfy_state.results[i];
        fprintf(out, "%-20s | %-32s | %-6s | %s\n",
                r->suite, r->entity,
                r->passed ? "PASS" : "FAIL",
                r->diag);
    }

    fprintf(out, "==========================================================================\n");
    fprintf(out, "TOTAL CLAUSES: %d | PASSED: %d | FAILED: %d\n",
            vfy_state.count, vfy_state.pass_count, vfy_state.fail_count);

    if (vfy_state.fail_count == 0) {
        fprintf(out, "CERTIFICATION STATUS : L0 = PASS | L1 = PASS | L2 = FULLY CERTIFIED (100%%)\n");
    } else {
        fprintf(out, "CERTIFICATION STATUS : L0 = PASS | L1 = CONDITIONAL | L2 = FAILED (%d failed clauses)\n",
                vfy_state.fail_count);
    }
    fprintf(out, "==========================================================================\n\n");
}

void vfy_print_csv(FILE *out)
{
    fprintf(out, "subsystem,entity,result,diagnostic\n");
    for (int i = 0; i < vfy_state.count; i++) {
        const VFY_RESULT *r = &vfy_state.results[i];
        fprintf(out, "%s,%s,%s,\"%s\"\n",
                r->suite, r->entity,
                r->passed ? "PASS" : "FAIL",
                r->diag);
    }
}
