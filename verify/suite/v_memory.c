/*
 * v_memory.c — Memory Management API Verification Suite
 *
 * Full behavioral testing of get_mbk, rel_mbk, chg_mbk, ref_mbk,
 * domain constants, and protection flags.
 */

#include "../btron_verify.h"
#include <btron/memory.h>
#include <btron/types.h>
#include <btron/error.h>
#include <stdlib.h>

#define S "Memory"

void vfy_suite_memory(void)
{
    /* ── Domain & Protection Constants ──────────────────────── */
    VFY_ASSERT_EQ(S, "M_LOCAL",      M_LOCAL,      0x00000000U);
    VFY_ASSERT_EQ(S, "M_COMMON",     M_COMMON,     0x00000001U);
    VFY_ASSERT_EQ(S, "M_SYSTEM",     M_SYSTEM,     0x00000003U);
    VFY_ASSERT_EQ(S, "M_RESIDENT",   M_RESIDENT,   0x00004000U);
    VFY_ASSERT_EQ(S, "DELEXIT",      DELEXIT,      0x00008000U);
    VFY_ASSERT_EQ(S, "M_READ",       M_READ,       0x00010000U);
    VFY_ASSERT_EQ(S, "M_WRITE",      M_WRITE,      0x00020000U);
    VFY_ASSERT_EQ(S, "M_EXEC",       M_EXEC,       0x00040000U);

    /* ── M_STATE Struct Size ────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(M_STATE)>0", sizeof(M_STATE) > 0);

    /* ── get_mbk / rel_mbk Lifecycle ────────────────────────── */
    VP adr = NULL;
    ER er = get_mbk(&adr, 2, M_LOCAL | M_READ | M_WRITE);
    VFY_ASSERT_EQ(S, "get_mbk(valid)", er, E_OK);
    VFY_ASSERT_NOTNULL(S, "get_mbk adr!=NULL", adr);

    if (adr) {
        /* Write to allocated memory blocks (2 blocks = 8192 bytes) */
        unsigned char *bytes = (unsigned char*)adr;
        bytes[0] = 0x5A;
        bytes[8191] = 0xA5;
        VFY_ASSERT_EQ(S, "memory write/read byte 0", bytes[0], 0x5A);
        VFY_ASSERT_EQ(S, "memory write/read byte 8191", bytes[8191], 0xA5);

        /* Query state */
        M_STATE st;
        er = ref_mbk(adr, &st);
        VFY_ASSERT_EQ(S, "ref_mbk(valid)", er, E_OK);
        VFY_ASSERT_EQ(S, "ref_mbk total blocks", st.total, 2);

        /* Change block count */
        er = chg_mbk(adr, 4);
        VFY_ASSERT_EQ(S, "chg_mbk(valid)", er, E_OK);

        /* Release memory */
        er = rel_mbk(adr);
        VFY_ASSERT_EQ(S, "rel_mbk(valid)", er, E_OK);

        /* Double-release should fail */
        er = rel_mbk(adr);
        VFY_ASSERT_EQ(S, "rel_mbk(double)", er, ER_PAR);
    }

    /* ── Error Parameter Handling ───────────────────────────── */
    er = get_mbk(NULL, 1, M_LOCAL);
    VFY_ASSERT_EQ(S, "get_mbk(NULL adr)", er, ER_ADR);

    er = get_mbk(&adr, 0, M_LOCAL);
    VFY_ASSERT_EQ(S, "get_mbk(nblk=0)", er, ER_PAR);

    er = rel_mbk(NULL);
    VFY_ASSERT_EQ(S, "rel_mbk(NULL)", er, ER_ADR);
}
