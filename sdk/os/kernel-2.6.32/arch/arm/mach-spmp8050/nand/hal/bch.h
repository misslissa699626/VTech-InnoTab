/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2007 by Sunplus Technology Co., Ltd.             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
 
/**************************************************************************
 *  System:         Multimedia platform                                   *
 *  Subsystem:      helper library                                        *
 *                                                                        *
 *  Filename:   	bch.h                                                 *
 *  Author:     	alexchang                                             *
 *  Description:	header file                                           * 
 *  Reference:      Reference documents or websites                       * 
 *                                                                        * 
 *  Version history:                                                      *
 *------------------------------------------------------------------------*
 *	Version   Date          Modified By    Description                    *
 *	0.0.1     2007/11/20    alexchang         Create this file            *  
 *                                                                        *
 **************************************************************************/
#ifndef _BCH_H_
#define _BCH_H_

#include "nf_s330.h"
#include "hal_base.h"

//define BCH error code, add by alexchang 11/20/2007
#define OK_DEV_BCH				0 
#define ERR_DEV_BCH_DECODE		-1
#define ERR_DEV_BCH_ENCODE		-2		
#define ERR_DEV_BCH_OVER_8BITS  -3
#define ERR_BCH_ERROR			-4

//define BCH bit mask, add by alexchang 11/20/2007
#define BCH_FINISH_MASK(X)				(X)
#define BCH_DECODE_FAIL_MASK(X)			(X<<1)
#define BCH_ERR_8BIT_MASK(X)			(X<<2)
#define BCH_PARITY_COMPARE_MASK(X)		(X<<3)

//define CFG register, add by alexchang 11/20/2007
#define BCH_START						1
#define BCH_MODE(X)						(X<<1)	//0:encode, 1:decode
#define BCH_8BIT_REDO_MODE(X)			(X<<2)	//0:close,  1:open
#define BCH_SET_BLOCK_NUM(X)			(X<<4)
#define BCH_DIV512(X)					(X>>9)  //define div 512


//define BCH Status register, add by alexchang 11/20/2007
#define BCH_STATUS_DEFAULT		0
#define BCH_FINISH 						1
#define BCH_DECODE_FAIL 			0x2
#define BCH_ERROR_8BITS				0x4
#define BCH_ERROR_BLK_NO(X) 	(X&0x3F00)>>16

	
//BCH function declare add by alexchang 11/20/2007
int bchProcess(unsigned long* a_PyldBuffer,unsigned long* a_ReduntBuffer,int a_BufLen,int a_OP);
#endif

