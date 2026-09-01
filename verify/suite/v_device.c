/*
 * v_device.c — BTRON 3.20 Device Management Verification Suite
 *
 * Tests opn_dev_mgr, cls_dev_mgr, rea_dev, wri_dev, ctl_dev, ref_dev, wai_dev.
 */

#include "../btron_verify.h"
#include <btron/device.h>
#include <string.h>

#define S "Device"

void vfy_suite_device(void)
{
    /* ── Structure Sizes ────────────────────────────────────── */
    VFY_ASSERT_TRUE(S, "sizeof(DEV_STAT)>0", sizeof(DEV_STAT) > 0);
    VFY_ASSERT_TRUE(S, "sizeof(DEV_REQ)>0",  sizeof(DEV_REQ) > 0);

    /* ── Device open / write / read / close lifecycle ────────── */
    ID did = opn_dev_mgr("serial0", DEV_READ | DEV_WRITE);
    VFY_ASSERT_TRUE(S, "opn_dev_mgr(valid)>0", did > 0);

    if (did > 0) {
        /* Write data */
        const char msg[] = "TRON_DEV_TEST";
        W wrote = 0;
        ER er = wri_dev(did, msg, (W)strlen(msg), &wrote);
        VFY_ASSERT_EQ(S, "wri_dev(valid)", er, E_OK);
        VFY_ASSERT_EQ(S, "wri_dev wrote bytes", wrote, (W)strlen(msg));

        /* Query device status */
        DEV_STAT stat;
        er = ref_dev(did, &stat);
        VFY_ASSERT_EQ(S, "ref_dev(valid)", er, E_OK);
        VFY_ASSERT_EQ(S, "stat.mode", stat.mode, DEV_READ | DEV_WRITE);

        /* Read back data */
        char buf[64];
        W read_bytes = 0;
        memset(buf, 0, sizeof(buf));
        er = rea_dev(did, buf, (W)sizeof(buf), &read_bytes);
        VFY_ASSERT_EQ(S, "rea_dev(valid)", er, E_OK);
        VFY_ASSERT_EQ(S, "rea_dev count", read_bytes, (W)strlen(msg));
        VFY_ASSERT_TRUE(S, "rea_dev round-trip", strcmp(buf, msg) == 0);

        /* Device control */
        er = ctl_dev(did, DEV_CMD_FLUSH, NULL);
        VFY_ASSERT_EQ(S, "ctl_dev(FLUSH)", er, E_OK);

        /* Asynchronous request */
        DEV_REQ req;
        memset(&req, 0, sizeof(req));
        er = wai_dev(did, &req, 100);
        VFY_ASSERT_EQ(S, "wai_dev(valid)", er, E_OK);

        /* Close device */
        er = cls_dev_mgr(did);
        VFY_ASSERT_EQ(S, "cls_dev_mgr(valid)", er, E_OK);

        /* Double close should return error */
        er = cls_dev_mgr(did);
        VFY_ASSERT_EQ(S, "cls_dev_mgr(double)", er, ER_DID);
    }

    /* Error parameter checks */
    ID bad_did = opn_dev_mgr(NULL, DEV_READ);
    VFY_ASSERT_EQ(S, "opn_dev_mgr(NULL)", bad_did, ER_PAR);

    ER bad_cls = cls_dev_mgr(-1);
    VFY_ASSERT_EQ(S, "cls_dev_mgr(bad_did)", bad_cls, ER_DID);
}
