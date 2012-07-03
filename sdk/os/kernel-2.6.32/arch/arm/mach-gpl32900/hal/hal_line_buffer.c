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
 * @file    hal_line_buffer.c
 * @brief   Implement of SPMP8050 line buffer HAL API.
 * @author  junp.zhang
 * @since   2010/11/10
 * @date    2010/11/10
 */
#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

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

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static scuaReg_t *scuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

/**
 * @brief   Line buffer hardware mode setting function
 * @param   mode [in] line buffer mode
 * @return  None
 * @see
 */
void
gpHalLineBufferSetMode(
	UINT32 mode
)
{
	UINT32 tmp = 0;

	tmp = scuaReg->scuaDispType;
	tmp &= ~0xFF;
	tmp |= mode;
	scuaReg->scuaDispType = tmp;
}

/**
 * @brief   Line buffer hardware mode getting function
 * @return  line buffer mode
 * @see
 */
UINT32
gpHalLineBufferGetMode(
	void
)
{
	return (scuaReg->scuaDispType) & 0xFF;
}

