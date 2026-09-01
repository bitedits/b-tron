/*
 * v_errors.c — Error Code Value Verification Suite
 *
 * L0 conformance: all error codes must have the exact specified values.
 */

#include "../btron_verify.h"
#include <btron/error.h>

#define S "ErrorCodes"

void vfy_suite_errors(void)
{
    /* §2.1 Defined in error.h — mandatory values from BTRON 3.20 spec */
    VFY_ASSERT_EQ(S, "E_OK",    E_OK,     0);
    VFY_ASSERT_EQ(S, "E_SYS",   E_SYS,   -5);
    VFY_ASSERT_EQ(S, "E_NOMEM", E_NOMEM, -10);
    VFY_ASSERT_EQ(S, "E_NOSPT", E_NOSPT, -17);
    VFY_ASSERT_EQ(S, "E_RSVR",  E_RSVR,  -25);
    VFY_ASSERT_EQ(S, "E_PAR",   E_PAR,   -33);
    VFY_ASSERT_EQ(S, "E_LIMIT", E_LIMIT, -34);
    VFY_ASSERT_EQ(S, "E_ID",    E_ID,    -35);
    VFY_ASSERT_EQ(S, "E_OBJ",   E_OBJ,   -41);
    VFY_ASSERT_EQ(S, "E_NOEXS", E_NOEXS, -52);
    VFY_ASSERT_EQ(S, "E_BUSY",  E_BUSY,  -65);
    VFY_ASSERT_EQ(S, "E_TMOUT", E_TMOUT, -69);

    /* All error codes (except E_OK) must be negative */
    VFY_ASSERT_TRUE(S, "E_SYS<0",   E_SYS   < 0);
    VFY_ASSERT_TRUE(S, "E_NOMEM<0", E_NOMEM < 0);
    VFY_ASSERT_TRUE(S, "E_NOSPT<0", E_NOSPT < 0);
    VFY_ASSERT_TRUE(S, "E_PAR<0",   E_PAR   < 0);
    VFY_ASSERT_TRUE(S, "E_LIMIT<0", E_LIMIT < 0);
    VFY_ASSERT_TRUE(S, "E_ID<0",    E_ID    < 0);
    VFY_ASSERT_TRUE(S, "E_OBJ<0",   E_OBJ   < 0);
    VFY_ASSERT_TRUE(S, "E_NOEXS<0", E_NOEXS < 0);
    VFY_ASSERT_TRUE(S, "E_BUSY<0",  E_BUSY  < 0);
    VFY_ASSERT_TRUE(S, "E_TMOUT<0", E_TMOUT < 0);

    /* Uniqueness: no two error codes share the same value */
    VFY_ASSERT_NEQ(S, "E_SYS!=E_NOMEM",  E_SYS,   E_NOMEM);
    VFY_ASSERT_NEQ(S, "E_PAR!=E_LIMIT",  E_PAR,   E_LIMIT);
    VFY_ASSERT_NEQ(S, "E_PAR!=E_ID",     E_PAR,   E_ID);
    VFY_ASSERT_NEQ(S, "E_OBJ!=E_NOEXS",  E_OBJ,   E_NOEXS);
    VFY_ASSERT_NEQ(S, "E_BUSY!=E_TMOUT",  E_BUSY,  E_TMOUT);
}
