/*
 *----------------------------------------------------------------------
 *    T-Kernel 2.0 Software Package
 *
 *    Copyright 2011 by Ken Sakamura.
 *    This software is distributed under the latest version of T-License 2.x.
 *----------------------------------------------------------------------
 *
 *    Released by T-Engine Forum(http://www.t-engine.org/) at 2011/05/17.
 *    Modified by TRON Forum(http://www.tron.org/) at 2015/06/01.
 *
 *----------------------------------------------------------------------
 */

/*
 *	Extended SVC parameter packet
 *
 *	   (generated automatically)
 */

#include <basic.h>
#include <t2ex/datetime.h>
#include <sys/str_align.h>
#include <sys/svc/fndatetime.h>

typedef struct {
	const struct tzinfo* tz;	_align64
} DT_DT_SETSYSTZ_PARA;

typedef struct {
	struct tzinfo* tz;	_align64
} DT_DT_GETSYSTZ_PARA;

