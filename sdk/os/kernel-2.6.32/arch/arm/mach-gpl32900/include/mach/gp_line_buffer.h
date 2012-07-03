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
 * @file    gp_line_buffer.h
 * @brief   Declaration of line buffer.
 * @author  junp.zhang
 * @since   2010/11/10
 * @date    2010/11/10
 */
#ifndef _GP_LINE_BUFFER_H_
#define _GP_LINE_BUFFER_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LINE_BUFFER_MODULE_DISPLAY		0x00
#define LINE_BUFFER_MODULE_SCALER		0x01
#define LINE_BUFFER_MODULE_VSCALER		0x02
#define LINE_BUFFER_MODULE_2D			0x03
#define LINE_BUFFER_MODULE_ROTATOR		0x04

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
* @brief   line buffer request function
* @param   module [in] module id
* @param   length [in] line length in pixels
* @return  SP_OK(0)/ERROR_ID
*/
int gp_line_buffer_request(unsigned int module, unsigned int length);

/**
* @brief   line buffer release function
* @param   module [in] module id
* @return  SP_OK(0)/ERROR_ID
*/
int gp_line_buffer_release(unsigned int module);

#endif /* _GP_LINE_BUFFER_H_ */
