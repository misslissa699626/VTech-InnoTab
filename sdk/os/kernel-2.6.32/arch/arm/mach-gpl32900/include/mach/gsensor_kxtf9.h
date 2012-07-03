
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
#ifndef __GSENSOR_KXTF9_H__
#define __GSENSOR_KXTF9_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define GSENSOR_IOCTL_ID			'G'
#define GSENSOR_GET_DATA			_IOR(GSENSOR_IOCTL_ID, 0x01, int)
#define GSENSOR_GET_FILTERED_DATA	_IOR(GSENSOR_IOCTL_ID, 0x02, int)
#define GSENSOR_SET_CAL_DATA		_IOW(GSENSOR_IOCTL_ID, 0x03, int)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
	int x;
	int y;
	int z;
	int counts_per_g;
	int range_of_count;
	int tilt;
}gsensor_data_t;

typedef struct
{
	int deltaX;
	int deltaY;
	int deltaZ;
}gsensor_cal_data_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

#endif
