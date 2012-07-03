/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#ifndef _GP_AES_H_
#define _GP_AES_H_

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct gpAesPara_s 
{
	unsigned int key0;
	unsigned int key1;
	unsigned int key2;
	unsigned int key3;
	unsigned int input_buf_addr;	//chunck memory address
	unsigned int output_buf_addr;	//chunck memory address
	unsigned int cblen;				//size in byte
} gpAesPara_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/* ioctl for device node definition */
#define AES_IOCTL_ID			'A'
#define AES_IOCTL_S_DECRYPT		_IOW(AES_IOCTL_ID, 0, gpAesPara_t)
#define AES_IOCTL_S_ENCRYPT		_IOW(AES_IOCTL_ID, 1, gpAesPara_t)

#endif /* _GP_AES_H_ */
