/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

/**
 * @file hal_common.h
 * @brief Common HAL definition header
 * @author Roger Hsu
 */

#ifndef _HAL_COMMON_H_
#define _HAL_COMMON_H_

#include <mach/typedef.h>
#include <mach/diag.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define HAL_BUSY_WAITING(cond, ms)	\
	({	\
		SINT32 __timeout = ms * 1000;	\
		while (!(cond)){	\
			if (__timeout < 0){	\
				DIAG_ERROR("[%s] HAL_BUSY_WAITING Time Out!!\n", __FUNCTION__);	\
				break;	\
			}	\
				\
			udelay(1);	\
			__timeout--;	\
		}	\
		__timeout; \
	})

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

#endif

