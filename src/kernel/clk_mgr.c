/*
 * B-TRON Clock & Calendar Subsystem: clk_mgr.c
 * Cleanroom implementation of BTRON 3.20 Real-Time Clock Engine.
 */

#include <btron/clk.h>
#include <btron/types.h>
#include <btron/error.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define TRON_EPOCH_OFFSET 473385600LL /* Seconds between Unix 1970 and TRON 1985-01-01 */

static TIMEZONE g_system_tz = { 540, 0 }; /* Default JST (+9h) */

ER get_tod(DATE_TIM *dt, TIMEZONE *tz) {
    if (!dt) return ER_PAR;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t unix_sec = tv.tv_sec;
    struct tm tm_info;
    gmtime_r(&unix_sec, &tm_info);

    dt->year  = (H)(tm_info.tm_year + 1900);
    dt->month = (H)(tm_info.tm_mon + 1);
    dt->day   = (H)(tm_info.tm_mday);
    dt->hour  = (H)(tm_info.tm_hour);
    dt->min   = (H)(tm_info.tm_min);
    dt->sec   = (H)(tm_info.tm_sec);
    dt->msec  = (H)(tv.tv_usec / 1000);

    if (tz) {
        *tz = g_system_tz;
    }

    return E_OK;
}

ER set_tod(const DATE_TIM *dt, const TIMEZONE *tz) {
    if (!dt) return ER_PAR;
    if (dt->year < 1985 || dt->month < 1 || dt->month > 12 || dt->day < 1 || dt->day > 31) {
        return ER_PAR;
    }

    if (tz) {
        g_system_tz = *tz;
    }

    return E_OK;
}

ER get_tim_btron(STIME *p_time) {
    if (!p_time) return ER_PAR;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    *p_time = (STIME)(tv.tv_sec - TRON_EPOCH_OFFSET);
    return E_OK;
}

ER set_tim_btron(STIME time) {
    (void)time;
    return E_OK;
}
