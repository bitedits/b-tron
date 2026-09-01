/*
 * BTRON 3.20 Conformance Verifier — Framework Header
 * btron_verify.h
 *
 * Pure C99, zero external dependencies.
 * Provides VFY_RESULT type, assertion macros, suite registration,
 * and reporting infrastructure.
 */

#ifndef BTRON_VERIFY_H
#define BTRON_VERIFY_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ─────────────────────────────────────────────── */

#define VFY_MAX_RESULTS  512
#define VFY_MAX_SUITES    32
#define VFY_DIAG_LEN     128

/* ── Result Record ─────────────────────────────────────────────── */

typedef struct {
    const char *suite;
    const char *entity;
    int         passed;
    char        diag[VFY_DIAG_LEN];   /* empty on pass */
} VFY_RESULT;

/* ── Global State ──────────────────────────────────────────────── */

typedef struct {
    VFY_RESULT results[VFY_MAX_RESULTS];
    int        count;
    int        pass_count;
    int        fail_count;
} VFY_STATE;

extern VFY_STATE vfy_state;

/* ── Suite Registration ────────────────────────────────────────── */

typedef void (*VFY_SUITE_FN)(void);

typedef struct {
    const char  *name;
    VFY_SUITE_FN fn;
} VFY_SUITE;

extern VFY_SUITE vfy_suites[VFY_MAX_SUITES];
extern int        vfy_suite_count;

void vfy_register_suite(const char *name, VFY_SUITE_FN fn);

/* ── Recording ─────────────────────────────────────────────────── */

void vfy_record(const char *suite, const char *entity,
                int passed, const char *fmt, ...);

/* ── Assertion Macros ──────────────────────────────────────────── */

#define VFY_ASSERT_EQ(suite, entity, got, expect) \
    do { \
        long long _got = (long long)(got); \
        long long _exp = (long long)(expect); \
        if (_got == _exp) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, \
                       "expected %lld, got %lld", _exp, _got); \
        } \
    } while (0)

#define VFY_ASSERT_NEQ(suite, entity, got, notexpect) \
    do { \
        long long _got = (long long)(got); \
        long long _nex = (long long)(notexpect); \
        if (_got != _nex) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, \
                       "expected != %lld, got %lld", _nex, _got); \
        } \
    } while (0)

#define VFY_ASSERT_NULL(suite, entity, ptr) \
    do { \
        if ((ptr) == NULL) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, "expected NULL, got non-NULL"); \
        } \
    } while (0)

#define VFY_ASSERT_NOTNULL(suite, entity, ptr) \
    do { \
        if ((ptr) != NULL) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, "expected non-NULL, got NULL"); \
        } \
    } while (0)

#define VFY_ASSERT_TRUE(suite, entity, cond) \
    do { \
        if ((cond)) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, "condition is false"); \
        } \
    } while (0)

#define VFY_ASSERT_GE(suite, entity, got, lo) \
    do { \
        long long _got = (long long)(got); \
        long long _lo  = (long long)(lo); \
        if (_got >= _lo) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, \
                       "expected >= %lld, got %lld", _lo, _got); \
        } \
    } while (0)

#define VFY_ASSERT_LE(suite, entity, got, hi) \
    do { \
        long long _got = (long long)(got); \
        long long _hi  = (long long)(hi); \
        if (_got <= _hi) { \
            vfy_record((suite), (entity), 1, ""); \
        } else { \
            vfy_record((suite), (entity), 0, \
                       "expected <= %lld, got %lld", _hi, _got); \
        } \
    } while (0)

/* ── Reporting ─────────────────────────────────────────────────── */

void vfy_print_table(FILE *out);
void vfy_print_csv(FILE *out);

/* ── Suite Declarations ────────────────────────────────────────── */

void vfy_suite_types(void);
void vfy_suite_errors(void);
void vfy_suite_itron(void);
void vfy_suite_memory(void);
void vfy_suite_message(void);
void vfy_suite_vobj(void);
void vfy_suite_dp(void);
void vfy_suite_wnd(void);
void vfy_suite_hmi(void);

#ifdef __cplusplus
}
#endif

#endif /* BTRON_VERIFY_H */
