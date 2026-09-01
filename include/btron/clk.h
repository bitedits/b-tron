/*
 * B-TRON Specification Compatible Header: clk.h
 * Real-Time Clock, Calendar & Timer Subsystem.
 */

#ifndef _BTRON_CLK_H_
#define _BTRON_CLK_H_

#include <btron/types.h>
#include <btron/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── TRON Standard Epoch Timestamp (Seconds since 1985-01-01 00:00:00 UTC) ── */
typedef D STIME;

/* ── Timezone & Daylight Savings Descriptor ────────────────────── */
typedef struct {
    H off_min;   /* Local time offset in minutes from UTC (e.g. +540 for JST) */
    H dst_min;   /* Daylight saving offset in minutes */
} TIMEZONE;

/* ── Broken-Down Date and Time Structure ───────────────────────── */
typedef struct {
    H year;      /* 1985..2099 */
    H month;     /* 1..12 */
    H day;       /* 1..31 */
    H hour;      /* 0..23 */
    H min;       /* 0..59 */
    H sec;       /* 0..59 */
    H msec;      /* 0..999 */
} DATE_TIM;

/* ── Clock & Calendar APIs ─────────────────────────────────────── */
ER get_tod(DATE_TIM *dt, TIMEZONE *tz);
ER set_tod(const DATE_TIM *dt, const TIMEZONE *tz);
ER get_tim_btron(STIME *p_time);
ER set_tim_btron(STIME time);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_CLK_H_ */
