/*
 * v_types.c — Type Size & Alignment Verification Suite
 *
 * L0 conformance: all fundamental types must have the specified widths.
 * These are compile-time/sizeof checks — no runtime kernel needed.
 */

#include "../btron_verify.h"
#include <btron/types.h>

#define S "Types"

void vfy_suite_types(void)
{
    /* §1.1 Primitive Integer Types */
    VFY_ASSERT_EQ(S, "sizeof(B)",  sizeof(B),  1);
    VFY_ASSERT_EQ(S, "sizeof(H)",  sizeof(H),  2);
    VFY_ASSERT_EQ(S, "sizeof(W)",  sizeof(W),  4);
    VFY_ASSERT_EQ(S, "sizeof(D)",  sizeof(D),  8);
    VFY_ASSERT_EQ(S, "sizeof(UB)", sizeof(UB), 1);
    VFY_ASSERT_EQ(S, "sizeof(UH)", sizeof(UH), 2);
    VFY_ASSERT_EQ(S, "sizeof(UW)", sizeof(UW), 4);
    VFY_ASSERT_EQ(S, "sizeof(UD)", sizeof(UD), 8);

    /* §1.2 Pointer and System Types */
    VFY_ASSERT_EQ(S, "sizeof(VP)==sizeof(void*)", sizeof(VP), sizeof(void*));
    VFY_ASSERT_EQ(S, "sizeof(VW)==sizeof(void*)", sizeof(VW), sizeof(void*));
    VFY_ASSERT_EQ(S, "sizeof(ID)",  sizeof(ID),  4);
    VFY_ASSERT_EQ(S, "sizeof(ER)",  sizeof(ER),  4);
    VFY_ASSERT_EQ(S, "sizeof(BOOL)", sizeof(BOOL), 4);
    VFY_ASSERT_EQ(S, "sizeof(TC)",  sizeof(TC),  2);
    VFY_ASSERT_EQ(S, "sizeof(COLOR)", sizeof(COLOR), 4);

    /* §1.3 Geometry Primitives */
    VFY_ASSERT_EQ(S, "sizeof(PNT)",  sizeof(PNT),  4);
    VFY_ASSERT_EQ(S, "sizeof(RECT)", sizeof(RECT), 8);
    VFY_ASSERT_EQ(S, "sizeof(PAT)",  sizeof(PAT),  8);

    /* Signedness checks */
    VFY_ASSERT_TRUE(S, "B is signed",  (B)(-1)  < 0);
    VFY_ASSERT_TRUE(S, "H is signed",  (H)(-1)  < 0);
    VFY_ASSERT_TRUE(S, "W is signed",  (W)(-1)  < 0);
    VFY_ASSERT_TRUE(S, "D is signed",  (D)(-1)  < 0);
    VFY_ASSERT_TRUE(S, "UB is unsigned", (UB)(-1) > 0);
    VFY_ASSERT_TRUE(S, "UH is unsigned", (UH)(-1) > 0);
    VFY_ASSERT_TRUE(S, "UW is unsigned", (UW)(-1) > 0);
    VFY_ASSERT_TRUE(S, "UD is unsigned", (UD)(-1) > 0);

    /* TRUE / FALSE macros */
    VFY_ASSERT_EQ(S, "TRUE==1",  TRUE,  1);
    VFY_ASSERT_EQ(S, "FALSE==0", FALSE, 0);
}
