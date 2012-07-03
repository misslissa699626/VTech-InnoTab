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
 * @file    hal_apbdma0.h
 * @brief   Implement of APBDMA0 HAL API header file.
 * @author  Dunker Chen
 */
 
#ifndef _HAL_APBDMA0_H_
#define _HAL_APBDMA0_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
 
#include <mach/typedef.h>

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/
 
/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/

typedef enum gpApbdma0Module_e{
	SD0 	= 1,						/*!< @brief SD card 0 module */
	SD1 	= 2,						/*!< @brief SD card 1 module */
	UART0 	= 3,						/*!< @brief UART 0 module */
	UART1 	= 4,						/*!< @brief UART 1 module */
	SPI0 	= 5,						/*!< @brief SPI 0 module */
	SPI1 	= 6,						/*!< @brief SPI 1 module */
	I2C 	= 7,						/*!< @brief I2C module */
	TI2C 	= 8,						/*!< @brief TI2C module */
	CF 		= 9,						/*!< @brief CF module */
	MS 		= 10						/*!< @brief MS module */
} gpApbdma0Module_t;

typedef struct gpApbdma0Param_s {
	gpApbdma0Module_t	module;			/*!< @brief Peripheral module. */
	UINT8 				*buf0;			/*!< @brief Start address of buffer 0. */
	UINT32				ln0;			/*!< @brief Size of buffer 0 (unit: byte). */
	UINT8				*buf1;			/*!< @brief Start address of buffer 1. */
	UINT32				ln1;			/*!< @brief Size of buffer 1 (unit: byte). */
	SINT32				dir;			/*!< @brief Direction of transfer. 0: read from device, others: write todevice. */
} gpApbdma0Param_t;

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/**
* @brief 	Apbdma0 init function.
* @return	None.
*/
void gpHalApbdmaInit(void);

/**
* @brief 	Apbdma0 un-init function.
* @return	None.
*/
void gpHalApbdmaUninit(void);

/**
* @brief 	Enable apbdma channel.
* @param 	ch[in]: Apbdma channel number.
* @param 	param[in]: Channel parameter.
* @return	None.
*/
void gpHalApbdmaEn(UINT32 ch, gpApbdma0Param_t *param);

/**
* @brief 	Apbdma channel reset.
* @param 	ch[in]: channel number.
* @return	None.
*/
void gpHalApbdmaRst(UINT32 ch);

/**
* @brief	Set apbdma0 buffer.
* @param	ch[in]: Channel number.
* @param	buf_num[in]: Buffer number 0 or 1.
* @param	addr[in]: Buffer start address.
* @param	ln[in]: Buffer size.
* @return: 	None.
*/
void gpHalApbdmaSetBuf(UINT32 ch, UINT32 buf_num, UINT8* addr, UINT32 ln);

/**
* @brief	Check apbdma0 status.
* @param	ch[in]: Channel number.
* @return: 	0 = idle, 1 = busy.
*/
UINT32 gpHalApbdmaChStatus (UINT32 ch);

/**
* @brief	Check which apbdma0 buffer in use.
* @param	ch[in]: Channel number.
* @return: 	0 = buffer 0, 1 = buffer 1.
*/
UINT32 gpHalApbdmaBufStatus (UINT32 ch);

/**
* @brief	Clear channel IRQ status.
* @param	ch[in]: Channel number.
* @return: 	None.
*/
void gpHalApbdmaClearIRQFlag (UINT32 ch);

#endif /* _HAL_APBDMA0_H_ */
