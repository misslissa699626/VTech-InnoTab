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
 * @file hal_uart.h
 * @brief uart driver HAL layer
 * @author zh.l
 */
 
#ifndef __HAL_UART_H__
#define __HAL_UART_H__
#include <mach/common.h>
#include <mach/hal/regmap/reg_uart.h>

enum {
	PARITY_NONE,
	PARITY_ZERO,
	PARITY_ONE,
	PARITY_EVEN,
	PARITY_ODD
};

enum {
	ONE_STOP_BIT,
	ONEHALF_STOP_BITS,
	TWO_STOP_BITS
};

typedef struct uart_cfg_s {
	UINT8	bits_per_word;	/*bits of word,surport 5,6,7,8bits*/
	UINT8	parity;
	UINT8	stop_bits;
	UINT8	auto_flow_ctrl;	/*1-enable/0-disable auto flow control*/
	UINT32	baudrate;
}uart_cfg_t;
/**
* @brief UART init function.
* @param device_id[in] : Index of uart device
* @param baud[in] : baudrate value
*/
void gpHalUartInit(UINT32 device_id, uart_cfg_t* cfg);

/**
* @brief UART write data function.
* @param device_id[in] : Index of uart device
* @param buffer[in] : write data buffer
* @param len[in] : write data len
*/
void gpHalUartWrite(UINT32 device_id,char *buffer, UINT32 len);

/**
* @brief UART read data function.
* @param device_id[in] : Index of uart device
* @param buffer[in] : read data buffer
* @param len[in] : read buffer len
*/
int gpHalUartRead(UINT32 device_id,char *buffer, UINT32 len);

/**
* @brief UART set freq function.
* @param device_id[in] : Index of uart device
* @param freq[in] : baudrate value
*/
void gpHalUartSetBaud(UINT32 device_id,UINT32 baud);

/**
* @brief UART set word length function.
* @param device_id[in] : Index of uart device
* @param len[in] : word length value
*/
void gpHalUartSetWordLen(UINT32 device_id,UINT32 len);

/**
* @brief UART set parity function.
* @param device_id[in] : Index of uart device
* @param parity [in] : parity value
*/
void gpHalUartSetParity (UINT32 device_id,UINT32 parity);

/**
* @brief UART set stop bit function.
* @param device_id[in] : Index of uart device
* @param stop_bit [in] : stop_bit value
*/
void gpHalUartSetStopBit(UINT32 device_id,UINT32 stop_bit);

/**
* @brief UART set auto flow control function.
* @param device_id[in] : Index of uart device
* @param mode[in] : auto hardware flow control mode value
*/
void gpHalUartSetAutoFlowCtrl(UINT32 device_id,UINT32 mode);

/**
* @brief UART set modem function.
* @param device_id[in] : Index of uart device
* @param modem [in] : modem value
*/
void gpHalUartSetModemCtrl(UINT32 device_id,UINT32 modem);

/**
* @brief UART get modem status register
* @param device_id[in] : Index of uart device
* @param modem [out] : modem status
*/
UINT32 gpHalUartGetModemStatus(UINT32 device_id);


/**
* @brief UART set interrupt enable register
* @param device_id[in] : Index of uart device
* @param int_state[out] : interrupt enable state
*/
void gpHalUartSetIntEn(unsigned device_id, unsigned int_state);

/**
* @brief UART get interrupt enable register
* @param device_id[in] : Index of uart device
* @return interrupt enable state
*/
unsigned gpHalUartGetIntEn(unsigned device_id);

/**
* @brief UART get interrupt idenfication register
* @param device_id[in] : Index of uart device
* @return interrupt identification
*/
unsigned gpHalUartGetIntFlags(unsigned device_id);

/**
* @brief UART get received data byte
* @param device_id[in] : Index of uart device
* @return received data byte
*/
int gpHalUartGetChar(unsigned device_id);

/**
* @brief UART send data byte
* @param device_id[in] : Index of uart device
* @return none
*/
void gpHalUartPutChar(unsigned device_id, unsigned ch);

/**
* @brief UART get line status
* @param device_id[in] : Index of uart device
* @return line status
*/
unsigned gpHalUartGetLineStatus( unsigned device_id);

/**
* @brief UART set line control
* @param device_id[in] : Index of uart device
* @param ctrl[in] : line control
* @return none
*/
void gpHalUartSetLineCtrl( unsigned device_id, unsigned ctrl);

/**
* @brief UART get FIFO status
* @param device_id[in] : Index of uart device
* @return FIFO status
*/
unsigned gpHalUartGetFifoStatus(unsigned device_id);

/**
* @brief UART set FIFO control
* @param device_id[in] : Index of uart device
* @param ctrl[in] : FIFO control
* @return none
*/
void gpHalUartSetFifoCtrl(unsigned device_id, unsigned ctrl);

/**
* @brief UART get line control
* @param device_id[in] : Index of uart device
* @return line control
*/
unsigned gpHalUartGetLineCtrl(unsigned device_id);

/**
* @brief UART clear all interrupt flags
* @param device_id[in] : Index of uart device
* @return none
*/
void gpHalUartClrIntFlags(unsigned device_id);

/**
* @brief UART set clock enable/disable
* @param device_id[in] : Index of uart device
* @param enable[in] : 0-disable/none zero-enbale
* @return none
*/
void gpHalUartClkEnable(unsigned device_id, unsigned enable);

/**
* @brief UART get clock rate
* @param device_id[in] : Index of uart device
* @return clock rate of specified uart port
*/
unsigned int gpHalUartGetClkRate(unsigned device_id);

#endif

