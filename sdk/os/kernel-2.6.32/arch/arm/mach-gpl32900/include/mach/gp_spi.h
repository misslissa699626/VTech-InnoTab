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
 * @file    gp_spi.h
 * @brief   Declaration of spi driver API.
 * @author  zaimingmeng
 */

#ifndef _GP_SPI_H_
#define _GP_SPI_H_

#include <mach/typedef.h>
/******************************************************************
*		CONSTANTS		*
******************************************************************/

/*Ioctl for device node define */
#define SPMP_TC_MAGIC		'S'

#define SPI_IOCTL_CS_ENABLE		_IO(SPMP_TC_MAGIC,0x01)
#define SPI_IOCTL_CS_DISABLE		_IO(SPMP_TC_MAGIC,0x02)

#define SPI_IOCTL_SET_FREQ		_IOW(SPMP_TC_MAGIC,0x03,unsigned int)
#define SPI_IOCTL_SET_CHANNEL		_IOW(SPMP_TC_MAGIC,0x04,unsigned int)

#define SPI_IOCTL_SET_DMA		_IOW(SPMP_TC_MAGIC,0x05,unsigned int)
#define SPI_IOCTL_SET_CLK_POL		_IOW(SPMP_TC_MAGIC,0x06,unsigned int)
#define SPI_IOCTL_SET_CLK_PHA		_IOW(SPMP_TC_MAGIC,0x07,unsigned int)
#define SPI_IOCTL_SET_LSBF		_IOW(SPMP_TC_MAGIC,0x08,unsigned int)


/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/
/**
 * @brief spi request function
 * @param id [in] spi channel index
 * @return  handle/NULL
 */
int gp_spi_request(int id);

/**
 * @brief spi release function
 * @param handle [in] spi handle
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_release(int handle);

/*
 *@brief spi cs enable function
 *@param handle[in]:spi handle
 *@return :SP_OK(0)/ERROR
 */
int gp_spi_cs_enable(int handle);

/*
 *@brief spi cs disable function
 *@param handle[in]:spi handle
 *@return :SP_OK(0)/ERROR
 */
int gp_spi_cs_disable(int handle);

/**
 * @brief set spi dma mode function
 * @param handle [in] spi handle
 * @param mode [in] 0:disable dma; 1:enable dma
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_spi_set_dma_mode(int handle,int mode);

/**
 * @brief set spi clock polarity function
 * @param handle [in] spi handle
 * @param mode [in] 0:SCLK idles at low state; 1:SLCK idles at higt state
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_clk_pol(int handle,int mode);

/**
 * @brief set spi clock phase function
 * @param handle [in] spi handle
 * @param mode [in] 0:odd edge of SCLK clok; 1:even edge of SLCK clok
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_clk_pha(int handle,int mode);

/**
 * @brief set spi LSB/MSB first set function
 * @param handle [in] spi handle
 * @param mode [in] 0:MSB first; 1:LSB first
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_lsb(int handle,int mode);

/**
 * @brief set spi freq function
 * @param handle [in] spi handle
 * @param freq [in] freq value(Hz)
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_freq(int handle,int freq);

/**
 * @brief set spi interrupt enable mode function
 * @param handle [in] spi handle
 * @param mode [in] interrupt mode
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_enable_int(int handle,int mode);

/**
 * @brief get spi interrupt state function
 * @param handle [in] spi handle
 * @param state [out] interrupt mode
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_get_int_state(int handle,int *state);

/**
 * @brief spi write function
 * @param handle [in] spi handle
 * @param buffer [in] write data buffer
 * @param len [in] size of buffer
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_write(int handle,char *buffer,int len);

/**
 * @brief spi read function
 * @param handle [in] spi handle
 * @param buffer [out] read data buffer
 * @param len [in] size of buffer
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_read(int handle,char *buffer,int len);

/*******************************************************************************

*               F U N C T I O N    D E C L A R A T I O N S

*******************************************************************************/
/**
 * @brief spi request function
 * @param id [in] spi channel index
 * @return  handle/NULL
 */
int gp_spi1_request(int id);

/**
 * @brief spi release function
 * @param handle [in] spi handle
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_release(int handle);

/*
 *@brief spi cs enable function
 *@param handle[in]:spi handle
 *@return :SP_OK(0)/ERROR
 */
int gp_spi1_cs_enable(int handle);

/*
 *@brief spi cs disable function
 *@param handle[in]:spi handle
 *@return :SP_OK(0)/ERROR
 */
int gp_spi1_cs_disable(int handle);

/**
 * @brief set spi dma mode function
 * @param handle [in] spi handle
 * @param mode [in] 0:disable dma; 1:enable dma
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_spi1_set_dma_mode(int handle,int mode);

/**
 * @brief set spi clock polarity function
 * @param handle [in] spi handle
 * @param mode [in] 0:SCLK idles at low state; 1:SLCK idles at higt state
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_set_clk_pol(int handle,int mode);

/**
 * @brief set spi clock phase function
 * @param handle [in] spi handle
 * @param mode [in] 0:odd edge of SCLK clok; 1:even edge of SLCK clok
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_set_clk_pha(int handle,int mode);

/**
 * @brief set spi LSB/MSB first set function
 * @param handle [in] spi handle
 * @param mode [in] 0:MSB first; 1:LSB first
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_set_lsb(int handle,int mode);

/**
 * @brief set spi freq function
 * @param handle [in] spi handle
 * @param freq [in] freq value(Hz)
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_set_freq(int handle,int freq);

/**
 * @brief set spi interrupt enable mode function
 * @param handle [in] spi handle
 * @param mode [in] interrupt mode
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_set_enable_int(int handle,int mode);

/**
 * @brief get spi interrupt state function
 * @param handle [in] spi handle
 * @param state [out] interrupt mode
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_get_int_state(int handle,int *state);

/**
 * @brief spi write function
 * @param handle [in] spi handle
 * @param buffer [in] write data buffer
 * @param len [in] size of buffer
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_write(int handle,char *buffer,int len);

/**
 * @brief spi read function
 * @param handle [in] spi handle
 * @param buffer [out] read data buffer
 * @param len [in] size of buffer
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi1_read(int handle,char *buffer,int len);


#endif	/*_GP_SPI_H_*/
