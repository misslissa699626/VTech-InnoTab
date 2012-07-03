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
 * @file    gp_apbdma0.h
 * @brief   Implement of APBDMA0 module API header file.
 * @author  Dunker Chen
 */
 
#ifndef _GP_APBDMA0_H_
#define _GP_APBDMA0_H_

/**************************************************************************
*                         H E A D E R   F I L E S                        *
**************************************************************************/

#include <mach/hal/hal_apbdma0.h>

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/

typedef void (*gp_apbdma0_irq_handle_t)(unsigned int);

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/**
* @brief 	Acquire apbdma0 handle.
* @param 	timeout[in]: Number of system tick, 0 for waitting until acquire channel.
* @return:	Apbdma handle(non-zero and non-negative)/ERROR_ID(-1).
*/
int gp_apbdma0_request(unsigned short timeout);

/**
* @brief 	Release apbdma0 handle.
* @param 	handle [in]: apbdma handle.
* @return:	None.
*/
void gp_apbdma0_release(int handle);

/**
* @brief 	Enable apbdma0.
* @param 	handle [in]: Apbdma handle.
* @param	param [in]: Apbdma parameter. When buffer 1 address is null or length = 0, it will be set as single buffer mode.
* @return: 	None.
*/
void gp_apbdma0_en(unsigned int handle, gpApbdma0Param_t param);

/**
* @brief 	Wait until apbdma0 finish.
* @param 	timeout [in]: timeout (unit: 10ms).
* @param 	handle [in]: apbdma handle.
* @return: 	SUCCESS(0)/ERROR_ID
*/
int gp_apbdma0_wait(unsigned int handle, unsigned short timeout);

/**
* @brief 	Check for apbdma finish or not.
* @param 	handle [in]: apbdma handle.
* @return: 	SUCCESS(0)/ERROR_ID
*/
int gp_apbdma0_trywait(unsigned int handle);

/**
* @brief 	Attach IRQ function to channel.
* @param 	handle [in]: apbdma handle.
* @param	func[in]: IRQ function with which buffer number is finished as input argument.
* @return: 	None.
*/
void gp_apbdma0_irq_attach(unsigned int handle, gp_apbdma0_irq_handle_t func);

/**
* @brief	Set apbdma0 buffer
* @param	handle [in]: Apbdma handle.
* @param	buf_num[in]: Buffer number 0 or 1.
* @param	addr[in]: Buffer start address.
* @param	ln[in]: Buffer size.
* @return: 	None.
*/
void gp_apbdma0_setbuf(unsigned int handle, unsigned int buf_num, unsigned char* addr, unsigned int ln);

/**
* @brief	Force apbdma0 stop
* @param	handle [in]: Apbdma handle.
* @return: 	None.
*/
void gp_apbdma0_stop(unsigned int handle);

#endif /* _GP_APBDMA0_H_ */