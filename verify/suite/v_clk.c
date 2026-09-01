/*
 * v_clk.c — Clock, Calendar & Timer Verification Suite
 *
 * Tests get_tod, set_tod, get_tim_btron, set_tim_btron, DATE_TIM, TIMEZONE, STIME.
 */

#include "../btron_verify.h"
#include <btron/clk.h>
#include <string.h>

#define S "Clock"

void vfy_suite_clk(void)
{
    /* ── Structure Sizes ────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(STIME)>0",    sizeof(STIME) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(TIMEZONE)>0", sizeof(TIMEZONE) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(DATE_TIM)>0", sizeof(DATE_TIM) > 0);

    /* ── get_tod ────────────────────────────────────────────── */
    DATE_TIM dt;
    TIMEZONE tz;
    memset(&dt, 0, sizeof(dt));
    memset(&tz, 0, sizeof(tz));

    ER er = get_tod(&dt, &tz);
    VFY_ASSERT_EQ(S, "get_tod(valid)", er, E_OK);
    VFY_ASSERT_GE(S, "dt.year >= 2020", dt.year, 2020);
    VFY_ASSERT_GE(S, "dt.month >= 1",   dt.month, 1);
    VFY_ASSERT_LE(S, "dt.month <= 12",  dt.month, 12);
    VFY_ASSERT_GE(S, "dt.day >= 1",     dt.day, 1);
    VFY_ASSERT_LE(S, "dt.day <= 31",    dt.day, 31);
    VFY_ASSERT_GE(S, "dt.hour >= 0",    dt.hour, 0);
    VFY_ASSERT_LE(S, "dt.hour <= 23",   dt.hour, 23);

    /* ── set_tod ────────────────────────────────────────────── */
    DATE_TIM new_dt = { 2026, 9, 1, 12, 0, 0, 0 };
    TIMEZONE new_tz = { 540, 0 };
    er = set_tod(&new_dt, &new_tz);
    VFY_ASSERT_EQ(S, "set_tod(valid)", er, E_OK);

    /* Invalid year */
    DATE_TIM bad_dt = { 1970, 1, 1, 0, 0, 0, 0 };
    er = set_tod(&bad_dt, NULL);
    VFY_ASSERT_EQ(S, "set_tod(bad_year)", er, ER_PAR);

    /* ── get_tim_btron (TRON Epoch Seconds) ─────────────────── */
    STIME st = 0;
    er = get_tim_btron(&st);
    VFY_ASSERT_EQ(S, "get_tim_btron(valid)", er, E_OK);
    VFY_ASSERT_TRUE(S, "st > 0", st > 0);

    er = get_tim_btron(NULL);
    VFY_ASSERT_EQ(S, "get_tim_btron(NULL)", er, ER_PAR);
}
