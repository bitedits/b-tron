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
 *
 *    Modified by T.Yokobayashi since 2015/11/25.
 *
 *----------------------------------------------------------------------
 */

/*
 *	@(#)segment_common.h (sys) 2019/05/31
 *
 *	Segment management
 */

#ifndef __SYS_SEGMENT_COMMON_H__
#define __SYS_SEGMENT_COMMON_H__

#include <basic.h>

/*
 * System-dependent definition
 */
#ifdef STD_SH7727
#  include <sys/sysdepend/std_sh7727/segment_depend.h>
#endif
#ifdef STD_SH7751R
#  include <sys/sysdepend/std_sh7751r/segment_depend.h>
#endif
#ifdef MIC_M32104
#  include <sys/sysdepend/mic_m32104/segment_depend.h>
#endif
#ifdef STD_S1C38K
#  include <sys/sysdepend/std_s1c38k/segment_depend.h>
#endif
#ifdef STD_MC9328
#  include <sys/sysdepend/std_mc9328/segment_depend.h>
#endif
#ifdef MIC_VR4131
#  include <sys/sysdepend/mic_vr4131/segment_depend.h>
#endif
#ifdef STD_VR5500
#  include <sys/sysdepend/std_vr5500/segment_depend.h>
#endif
#ifdef STD_MB87Q1100
#  include <sys/sysdepend/std_mb87q1100/segment_depend.h>
#endif
#ifdef STD_SH7760
#  include <sys/sysdepend/std_sh7760/segment_depend.h>
#endif
#ifdef TEF_EM1D
#  include <sys/sysdepend/tef_em1d/segment_depend.h>
#endif

#if defined(RPI_BCM283x) || defined(_RPI_BCM283x_)
#  include <arch/bcm283x/segment_depend.h>
#endif
#ifdef APP_SH7760
#  include <sys/sysdepend/app_sh7760/segment_depend.h>
#endif
#ifdef APP_RZT1
#  include <sys/sysdepend/app_rzt1/segment_depend.h>
#endif

#endif /* __SYS_SEGMENT_COMMON_H__ */


/*----------------------------------------------------------------------*/
#if 0
#|History of "segment_common.h"
#|-----------------------------
#|* 2015/11/25	[app_sh7760]用に、APP_SH7760 処理の追加
#|* 2016/03/01	[rpi_bcm283x]用に、APP_BCM283x 処理の追加
#|* 2016/03/14	[app_rzt1]用に、APP_RZT1 処理の追加
#|
#endif
