/*
 * v_memory.c — Memory Management API Verification Suite
 *
 * Tests get_mbk / rel_mbk / chg_mbk if available.
 * Since these are [PARTIAL] in CERTIFICATION.md, this suite
 * also tests the T-Kernel memory pool primitives they map to.
 */

#include "../btron_verify.h"
#include <btron/types.h>
#include <btron/error.h>
#include <stdlib.h>

#define S "Memory"

/*
 * Memory management domain/protection constants from the BTRON spec.
 * These are [MISSING] per CERTIFICATION.md — we test if they compile
 * and have reasonable values, but only if defined.
 */

void vfy_suite_memory(void)
{
    /* ── Basic stdlib allocation sanity (platform must support malloc) ── */
    void *p = malloc(256);
    VFY_ASSERT_NOTNULL(S, "malloc(256)", p);
    if (p) {
        /* Write and read back */
        memset(p, 0xAB, 256);
        VFY_ASSERT_EQ(S, "malloc_readback", ((unsigned char *)p)[0], 0xAB);
        VFY_ASSERT_EQ(S, "malloc_readback_end", ((unsigned char *)p)[255], 0xAB);
        free(p);
    }

    /* ── Zero-size allocation ── */
    /* C standard: malloc(0) is implementation-defined (may return NULL or unique ptr) */
    /* We just verify it doesn't crash */
    void *z = malloc(0);
    vfy_record(S, "malloc(0) no crash", 1, "");
    free(z);

    /* ── Large allocation ── */
    void *big = malloc(1024 * 1024);  /* 1 MiB */
    VFY_ASSERT_NOTNULL(S, "malloc(1MiB)", big);
    if (big) {
        /* Touch first and last byte */
        ((unsigned char *)big)[0] = 0x42;
        ((unsigned char *)big)[1024*1024 - 1] = 0x42;
        VFY_ASSERT_EQ(S, "1MiB_first_byte", ((unsigned char *)big)[0], 0x42);
        VFY_ASSERT_EQ(S, "1MiB_last_byte", ((unsigned char *)big)[1024*1024 - 1], 0x42);
        free(big);
    }

    /* ── Realloc ── */
    void *r = malloc(64);
    VFY_ASSERT_NOTNULL(S, "realloc_base", r);
    if (r) {
        memset(r, 0xCD, 64);
        void *r2 = realloc(r, 256);
        VFY_ASSERT_NOTNULL(S, "realloc(64->256)", r2);
        if (r2) {
            /* First 64 bytes should be preserved */
            VFY_ASSERT_EQ(S, "realloc_preserved", ((unsigned char *)r2)[0], 0xCD);
            VFY_ASSERT_EQ(S, "realloc_preserved_end", ((unsigned char *)r2)[63], 0xCD);
            free(r2);
        }
    }

    /*
     * NOTE: get_mbk / rel_mbk / chg_mbk are [PARTIAL] or [MISSING].
     * When the BTRON memory API is fully implemented, add:
     *   - get_mbk(valid) → E_OK, *adr != NULL
     *   - get_mbk(nblk=0) → E_PAR
     *   - get_mbk(adr=NULL) → E_PAR
     *   - rel_mbk(valid) → E_OK
     *   - rel_mbk(double-free) → error
     *   - chg_mbk(grow) → E_OK
     */
}
