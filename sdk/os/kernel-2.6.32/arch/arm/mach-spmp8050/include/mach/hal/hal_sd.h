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
/**
 * @file    hal_sd.h
 * @brief   Implement of SD HAL API header file.
 * @author  Dunker Chen
 */
 
#ifndef _HAL_SD_H_
#define _HAL_SD_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
 
#include <mach/typedef.h>

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
 
#define SDC_RESPTYPE_NONE 	0		/*!< @brief SD response r0. */
#define SDC_RESPTYPE_R1		1		/*!< @brief SD response r1. */
#define SDC_RESPTYPE_R2		2		/*!< @brief SD response r2. */
#define SDC_RESPTYPE_R3		3		/*!< @brief SD response r3. */
#define SDC_RESPTYPE_R4		4		/*!< @brief SD response r4. */
#define SDC_RESPTYPE_R5		5		/*!< @brief SD response r5. */
#define SDC_RESPTYPE_R6		6		/*!< @brief SD response r6. */
#define SDC_RESPTYPE_R7		7		/*!< @brief SD response r7. */
#define SDC_RESPTYPE_R1b	8		/*!< @brief SD response r1b. */

#define SDC_NO_DATA		 	0		/*!< @brief SD command without data. */
#define SDC_SINGLE_DATA		1		/*!< @brief SD command with data. */
#define SDC_MULTI_DATA		3		/*!< @brief SD command with multiple data( multiple block). */

#define SDC_READ			0		/*!< @brief SD data direction: read. */
#define SDC_WRITE			1		/*!< @brief SD data direction: write. */

//Enable Interrupt Type 
#define SD_MMC_INT_CMDCOM       0x0001
#define SD_MMC_INT_DATACOM      0x0002
#define SD_MMC_INT_CMDBUFFULL   0x0004
#define SD_MMC_INT_DATABUFFULL  0x0008
#define SD_MMC_INT_DATABUFEMPTY 0x0010
#define SD_MMC_INT_CARDDETECT   0x0020
#define SD_MMC_INT_SDIODETECT   0x0040
#define SD_MMC_INT_ALLMASK      0x007F
#define SD_MMC_INT_DISABLE      0
#define SD_MMC_INT_ENABLE       1

/*========================================================================
* 	    P_SDCX_CONTROL: SD Card Controller control register
* ========================================================================*/
#define MASK_C_BUSWIDTH_1			0x0000	
#define MASK_C_BUSWIDTH_4			0x0100 
#define MASK_C_DMAMODE				0x0200
#define MASK_C_IOMODE					   0x0400
#define MASK_C_ENSDBUS				   0x0800
#define MASK_C_TXTRI_1         	 0x0000
#define MASK_C_RXTRI_8         	 0xC000
#define MASK_C_RXTRI            	MASK_C_RXTRI_8
#define MASK_C_TXTRI            	MASK_C_TXTRI_1


/*========================================================================
* 	    P_SDCX_STATUS: SD Card Controller status register
* ========================================================================*/
#define MASK_S_ControllerBusy	0x0001
#define MASK_S_CardBusy			0x0002
#define MASK_S_CmdComplete		0x0004
#define MASK_S_DataComplete		0x0008
#define MASK_S_RespIdxError		0x0010
#define MASK_S_RespCRCError		0x0020
#define MASK_S_RespRegFull		0x0040
#define MASK_S_DataBufFull		0x0080
#define MASK_S_DataBufEmpty		0x0100
#define MASK_S_TimeOut			0x0200
#define MASK_S_DataCRCError		0x0400
#define MASK_S_CardWProtect		0x0800
#define MASK_S_CardIsPresent	0x1000
#define MASK_S_SdioInterrupt	0x2000
#define MASK_S_ClrAllBits		0xffff

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/

typedef struct gpHalSDCmd_s {
	UINT8	cmd;					/*!< @brief SD command (0~63). */
	UINT8	resp_type;				/*!< @brief Response type. */
	UINT8	with_data;				/*!< @brief Command with data or not, 0 means no data, others mean with data. */
	UINT8	dir;					/*!< @brief Data direction, 0 for read, others for write. */
} gpHalSDCmd_t;

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/**
* @brief 	SD init function.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
void gpHalSDInit(UINT32 device_id);

/**
* @brief 		SDMA un-init function.
* @return		None.
*/
void gpHalSDUninit(UINT32 device_id);

/**
* @brief 	Set SD transfer 74 clock.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
void gpHalSD74Clk(UINT32 device_id);

/**
* @brief 	Set SD clock.
* @param 	device_id[in]: Index of SD controller.
* @param 	apb_clk [in]:  System apb clock (unit: 100KHz).
* @param 	clk[in]: SD clock(unit:100KHz).
* @return	Actually clock (unit:100KHz).
*/
UINT16 gpHalSDSetClk(UINT32 device_id, UINT16 apb_clk, UINT16 clk);

/**
* @brief 	Set SD block size.
* @param 	device_id[in]: Index of SD controller.
* @param 	len[in]: Block length (0~4095).
* @return	None.
*/
void gpHalSDSetBlk(UINT32 device_id, UINT16 len);

/**
* @brief 	Set SD bus width.
* @param 	device_id[in]: Index of SD controller.
* @param 	bus_width[in]: 0 for 1 bit mode, 1 for 4 bits mode.
* @return	None.
*/
void gpHalSDSetBus (UINT32 device_id, UINT32 bus_width);

/**
* @brief 	Stop SD controller, also stop transaction.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
SP_BOOL gpHalSDStop (UINT32 device_id);

/**
* @brief 	Wait for data complete (only for write).
* @param 	device_id[in]: Index of SD controller.
* @return	SUCCESS/ERROR_ID..
*/
SP_BOOL gpHalSDWaitDataComplete (UINT32 device_id);

/**
* @brief 	Stop SD controller, also stop transaction.
* @param 	device_id[in]: Index of SD controller.
* @param 	param[in]: Command parameter.
* @param	arg[in]: Command arguement.
* @param	resp[in]: Response buffer (4byte align).
* @param	ln[in]: Respones buffer size (unit: byte, must multiple of 4).
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpHalSDSendCmd (UINT32 device_id, gpHalSDCmd_t param, UINT32 arg, UINT32* resp, UINT32 ln);

/**
* @brief 	Tx Rx SD data.
* @param 	device_id[in]: Index of SD controller.
* @param 	buf [in]: Buffer address (4 byte alignment).
* @param 	ln[in]: Buffer length (unit: byte, multiple of 4).
* @param 	dir[in]: Data direction, 0 for read, others for write.
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpHalSDDataTxRx (UINT32 device_id, UINT8* buf, UINT32 ln, UINT32 dir);

/**
* @brief 	Get SD status.
* @param 	device_id[in]: Index of SD controller.
* @return	current status.
*/
UINT32 gpHalSDGetStatus (UINT32 device_id);

/**
* @brief 	Clear SD status.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
void gpHalSDClearStatus (UINT32 device_id);

/**
* @brief 	Enable/disable SD interrupt.
* @param 	device_id[in]: Index of SD controller
* @param 	intInfo[in]: Interrupt enable/disable bits
* @param 	enable[in]: 1 for enable, 0 for disable
* @return	None.
*/
void gpHalSDEnableInterrupt(UINT32 deviceId, UINT32 intInfo, UINT32 enable);

/**
* @brief 	Enable/disable SD DMA mode.
* @param 	device_id[in]: Index of SD controller
* @param 	enable[in]: 1 for enable, 0 for disable
* @return	None.
*/
void gpHalSDEnableDma(UINT32 deviceId, UINT32 enable);

/**
* @brief 	Dump SD registers.
* @param 	device_id[in]: Index of SD controller
* @return	None.
*/
void gpHalSDDump(UINT32 deviceId);

#endif  /* _HAL_SD_H */