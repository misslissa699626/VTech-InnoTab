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
 * @file hal_spi.h
 * @brief spi HAL Operation API header
 * @author zaimingmeng
 */

#ifndef _HAL_SPI_H_
#define _HAL_SPI_H_
#include <mach/typedef.h>

/**
 * @brief the spi init function
 * @param enable [in] 0: disable; 1: enable
 */
void gpHalSpiClkEnable(void* devinfo, int enable);

/**
 * @brief the spi init function
 * @param freq [in] freqence value, 0 max freqence
 */
void gpHalSpiSetFreq(UINT32  freq);

/**
 * @brief the cs enable function
 * @param id [in] channnel index
 */
void gpHalSpiCsEnable(int id);

/**
 * @brief the cs disable function
 * @param id [in] channnel index
 */
void gpHalSpiCsDisable(int id);

/**
 * @brief the spi DAM control function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalSpiDmaCtrl(int enable);

/**
 * @brief the spi clock polarity state in slck idles set function
 * @param status [in] 0:SCLK idles at low state; 1:SCLK idles at high state
 */
void gpHalSpiClkPol(int status);

/**
 * @brief the spi clock phase state set function
 * @param status [in] 0:data occur at odd edge of SCLK clock ; 1:data occur at even edge of SCLK clock 
 */
void gpHalSpiClkPha(int status);

/**
 * @brief the spi LSB/MSB first set function
 * @param status [in] 0:MSB first ; 1:LSB first
 */
void gpHalSpiSetLsb(int status);

/**
 * @brief set interrup enable register function
 * @param mode [in] TDREE|RDRRE|RFTGE|OURNE
 */
void gpHalSpiSetIER(int mode);

/**
 * @brief get interrup state register function
 * @return TDREE|RDRRE|RFTGE|OURNE
 */
int gpHalSpiGetIIR(void);

/**
 * @brief the spi init function
 * @param buffer [in] write buffer data
 * @param len [in] write data len
 */
void gpHalSpiWrite(char *buffer,int len);

/**
 * @brief the spi init function
 * @param buffer [out] read buffer data
 * @param len [in] read data len
 */
void gpHalSpiRead(char *buffer,int len);



#endif	/*_HAL_SPI_H_*/
