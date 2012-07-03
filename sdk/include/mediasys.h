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

/*!
 * @file mediasys.h
 * @breif This file is for internal usage. Not exported to end-user or customers. 
 * this file is a part of internal implementation.
 */
 
#ifndef _MEDIA_SYSTEM_H_
#define _MEDIA_SYSTEM_H_


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define MEDIA_STATE_FLAG_EOA	0x00000001
#define MEDIA_STATE_FLAG_EOV	0x00000002
#define MEDIA_STATE_FLAG_ERR	0x80000000

enum {
	MEDIA_MSG_EOA = 0x00000001,
	MEDIA_MSG_EOV,
	MEDIA_MSG_ERR = 0x80000000,
};

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/


/*!
 * @breif Create system (default implementation) .
 * @param pbuf pointer of the buffer.
 * @param pmem
 * @param size Size of the buffer.
 * @return The pointer to the structure of the media system.
 * @see mediaSystemDestroy
 */
void *mediaSystem_Create(const char *pname); // , memory_t *pmem);


int mediaSystem_Stop(void *hSys);


/*!
 * @brief Destroy the media system (default implementation).
 * @param psystem The pointer to the media system.
 * @return SP_OK Success.
 * @see mediaSystemCreate, mediaSystemGetParam, mediaSystemSetParam
 */
int mediaSystem_Destroy(void *hMedia );


/*!
 * @breif Start the media system.
 * @param hSys The handle of the system.
 * @return SP_OK
 */
int mediaSystem_Start(void *hSys );

/*!
 * @breif Pause the media system.
 * @param hSys The handle of the system.
 * @return SP_OK if the media system start successfully.
 */
int mediaSystem_Pause(void *hSys );

/*!
 * @breif Resume the media system.
 * @param hSys The handle of the media system.
 * @return SP_OK
 *
 */
int mediaSystem_Resume(void *hSys );


int mediaSystem_Seek(void *hSys, int *ms);

void *mediaSystem_CreateFilter(void *hSys, const void *filterOp, const char *pname);


void mediaSystem_SetCallback(
	void *hMedia,
	int (*callback)(void *param, int msg, int val),
	void *callback_param);

int mediaSystem_SetTime(void *hMedia, int time);
int mediaSystem_GetTime(void *hMedia, int *Err);

void mediaSystem_SetDebugLevel(void *hMedia, int dbgLevel);

int filter_CallFunction(
	void *hFilter,
	const char *Function,
	int val);
int filter_LinkTo(void *hFrom, int pad, void *hTo);

#endif /* END */

