/*
 * v_errors.c — Error Code Value Verification Suite
 *
 * L0 conformance: all 31 error codes must have the exact specified values.
 */

#include "../btron_verify.h"
#include <btron/error.h>

#define S "ErrorCodes"

void vfy_suite_errors(void)
{
    /* ── Base Error Codes ───────────────────────────────────── */
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

    /* ── Extended BTRON 3.20 Error Codes ─────────────────────── */
    VFY_ASSERT_EQ(S, "ER_ADR",     ER_ADR,     -1);
    VFY_ASSERT_EQ(S, "ER_NOSPC",   ER_NOSPC,   -11);
    VFY_ASSERT_EQ(S, "ER_ACCES",   ER_ACCES,   -12);
    VFY_ASSERT_EQ(S, "ER_IO",      ER_IO,      -13);
    VFY_ASSERT_EQ(S, "ER_DID",     ER_DID,     -14);
    VFY_ASSERT_EQ(S, "ER_ROVR",    ER_ROVR,    -15);
    VFY_ASSERT_EQ(S, "ER_DLT",     ER_DLT,     -16);
    VFY_ASSERT_EQ(S, "ER_CTX",     ER_CTX,     -18);
    VFY_ASSERT_EQ(S, "ER_FD",      ER_FD,      -19);
    VFY_ASSERT_EQ(S, "ER_NOFS",    ER_NOFS,    -20);
    VFY_ASSERT_EQ(S, "ER_NODSK",   ER_NODSK,   -21);
    VFY_ASSERT_EQ(S, "ER_RONLY",   ER_RONLY,   -22);
    VFY_ASSERT_EQ(S, "ER_FNAME",   ER_FNAME,   -23);
    VFY_ASSERT_EQ(S, "ER_EXS",     ER_EXS,     -24);
    VFY_ASSERT_EQ(S, "ER_PWD",     ER_PWD,     -26);
    VFY_ASSERT_EQ(S, "ER_PERM",    ER_PERM,    -27);
    VFY_ASSERT_EQ(S, "ER_OVRW",    ER_OVRW,    -28);
    VFY_ASSERT_EQ(S, "ER_OVVR",    ER_OVVR,    -29);
    VFY_ASSERT_EQ(S, "ER_TIMEOUT", ER_TIMEOUT, -69);

    /* ── Alias Equality ─────────────────────────────────────── */
    VFY_ASSERT_EQ(S, "ER_OK==E_OK",       ER_OK,    E_OK);
    VFY_ASSERT_EQ(S, "ER_PAR==E_PAR",     ER_PAR,   E_PAR);
    VFY_ASSERT_EQ(S, "ER_TMOUT==E_TMOUT", ER_TMOUT, E_TMOUT);
}
