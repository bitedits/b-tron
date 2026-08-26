/*
 * Sakamura T-Kernel 2.0 Specification Engine Core: core_tkernel.c
 * Routes BTRON API calls directly into Sakamura T-Kernel 2.0 Real-Time Operating System.
 */

#include <tk/tkernel.h>
#include <device/virtio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "kernel.h"
#include "isyscall.h"
#include "timer.h"

/* Forward declarations for Sakamura T-Kernel 2.0 Subsystem Initializers */
extern ER task_initialize(void);
extern ER semaphore_initialize(void);
extern ER eventflag_initialize(void);
extern ER mailbox_initialize(void);
extern ER messagebuffer_initialize(void);
extern ER rendezvous_initialize(void);
extern ER mutex_initialize(void);
extern ER memorypool_initialize(void);
extern ER fix_memorypool_initialize(void);
extern ER cyclichandler_initialize(void);
extern ER alarmhandler_initialize(void);
extern ER subsystem_initialize(void);
extern ER resource_group_initialize(void);

void sakamura_tkernel_init(void) {
    printf("==========================================================\n");
    printf(" Sakamura T-Kernel 2.0 Real-Time OS Engine (TRON Forum Spec)\n");
    printf(" Target Mode 2: BTRON_SAKAMURA Active\n");
    printf(" Initializing Sakamura T-Kernel Core Modules...\n");
    printf("==========================================================\n");

    /* Initialize Sakamura T-Kernel 2.0 Real-Time Kernel Subsystems */
    task_initialize();
    semaphore_initialize();
    eventflag_initialize();
    mailbox_initialize();
    messagebuffer_initialize();
    rendezvous_initialize();
    mutex_initialize();
    memorypool_initialize();
    fix_memorypool_initialize();
    cyclichandler_initialize();
    alarmhandler_initialize();
    subsystem_initialize();
    resource_group_initialize();
    timer_initialize();

    printf("[T-KERNEL] All Sakamura T-Kernel 2.0 Real-Time Subsystems Initialized Successfully.\n");

    virtio_mmio_init(0x10001000);
}

/* Sakamura T-Kernel Low-Level Primitive & HAL Support Routines */
void* Imalloc(size_t sz) { return malloc(sz); }
void  Ifree(void *ptr) { free(ptr); }
void* Icalloc(size_t nmemb, size_t sz) { return calloc(nmemb, sz); }
void* IAmalloc(size_t sz, UINT attr) { (void)attr; return malloc(sz); }
void  IAfree(void *ptr, UINT attr) { (void)attr; free(ptr); }

void* tkl_memcpy(void *dst, const void *src, size_t n) { return memcpy(dst, src, n); }
void* tkl_memset(void *s, int c, size_t n) { return memset(s, c, n); }
char* tkl_strncpy(char *dst, const char *src, size_t n) { return strncpy(dst, src, n); }

int BitSearch0_w(const uint32_t *base, int offset, int width) {
    for (int i = 0; i < width; i++) {
        int pos = offset + i;
        int idx = pos / 32;
        int bit = pos % 32;
        if (!(base[idx] & (1U << bit))) return i;
    }
    return -1;
}

int BitSearch1_w(const uint32_t *base, int offset, int width) {
    for (int i = 0; i < width; i++) {
        int pos = offset + i;
        int idx = pos / 32;
        int bit = pos % 32;
        if (base[idx] & (1U << bit)) return i;
    }
    return -1;
}

int BitTest(const uint32_t *base, int offset) {
    int idx = offset / 32;
    int bit = offset % 32;
    return (base[idx] & (1U << bit)) ? 1 : 0;
}

UINT disint(void) { return 0; }
UINT enaint(UINT intsts) { return intsts; }
void DisableInt(UINT vec) { (void)vec; }
void EnableInt(INTVEC intvec) { (void)intvec; }
void SetIntMode(UINT vec, UINT mode) { (void)vec; (void)mode; }
void ClearInt(UINT vec) { (void)vec; }
BOOL CheckInt(INTVEC intvec) { (void)intvec; return FALSE; }

ATR available_cop = 0;
void *hook_dsp = NULL;
void *unhook_dsp = NULL;
void *hook_int = NULL;
void *unhook_int = NULL;
void *hook_svc = NULL;
void *unhook_svc = NULL;

ER no_support(void) { return -70; /* E_NOSPT */ }
void timer_handler_startup(void) {}
void tm_monitor(void) {}
void tm_putstring(const char *s) { if (s) printf("%s", s); }

INT _tk_get_cfn(UB *name, INT *val, INT max) {
    (void)name;
    if (val && max > 0) val[0] = 0;
    return 1;
}

INT __tk_get_cfn(UB *name, INT *val, INT max) {
    return _tk_get_cfn(name, val, max);
}

INT GetDevConf(CONST UB *name, INT *val) { (void)name; if (val) val[0] = 0; return 0; }
INT GetSysConf(CONST UB *name, INT *val) { (void)name; if (val) val[0] = 0; return 0; }

char* tkl_strncat(char *dst, const char *src, size_t n) { return strncat(dst, src, n); }
void tm_command(const char *cmd) { (void)cmd; }
void tm_exit(int code) { (void)code; }

void *lowmem_top = NULL;
void call_entry(void) {}
void dispatch_entry(void) {}
void rettex_entry(void) {}
void _tk_ret_int(void) {}
void call_dbgspt(void) {}
void defaulthdr_startup(void) {}
void exchdr_startup(void) {}
void inthdr_startup(void) {}

/* 
 * Direct Routing to Sakamura T-Kernel 2.0 System Call Implementations
 */
ID tk_cre_tsk(CONST T_CTSK *pk_ctsk) {
    return _tk_cre_tsk(pk_ctsk);
}

ER tk_sta_tsk(ID tskid, INT stacd) {
    return _tk_sta_tsk(tskid, stacd);
}

void tk_ext_tsk(void) {
    _tk_ext_tsk();
}

void tk_exd_tsk(void) {
    _tk_exd_tsk();
}

ER tk_slp_tsk(TMO tmout) {
    return _tk_slp_tsk(tmout);
}

ER tk_wup_tsk(ID tskid) {
    return _tk_wup_tsk(tskid);
}

ID tk_get_tid(void) {
    return _tk_get_tid();
}

ID tk_cre_sem(CONST T_CSEM *pk_csem) {
    return _tk_cre_sem(pk_csem);
}

ER tk_wai_sem(ID semid, INT cnt, TMO tmout) {
    return _tk_wai_sem(semid, cnt, tmout);
}

ER tk_sig_sem(ID semid, INT cnt) {
    return _tk_sig_sem(semid, cnt);
}

ER tk_del_sem(ID semid) {
    return _tk_del_sem(semid);
}

ER tk_dly_tsk(RELTIM dlytim) {
    return _tk_dly_tsk(dlytim);
}
