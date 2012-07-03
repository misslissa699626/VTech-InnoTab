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
 * @file    gp_sd.h
 * @brief   Declaration of SD base driver.
 * @author  Dunker Chen
 */
 
#ifndef _GP_SD_H_
#define _GP_SD_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/

#include <mach/gp_apbdma0.h>
#include <mach/gp_board.h>

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/

typedef enum gpSDType_e
{
	UNKNOWCARD	= 0,							/*!< @brief SD card type: unknown. */
	SDCARD 		= 1,							/*!< @brief SD card type: SD card. */
	MMCCARD 	= 2,							/*!< @brief SD card type: MMC card. */
	SDIO		= 3								/*!< @brief SD card type: SDIO card. */
}gpSDType_t;

typedef struct gpSDInfo_s{
	unsigned char			device_id;			/*!< @brief Index of SD controller. */
	unsigned char			card_type;			/*!< @brief SD, MMC or SDIO. */
	unsigned char			cap_type;			/*!< @brief Standard or high capacity. */
	unsigned char			vsd;				/*!< @brief Reserved. */
	unsigned int			speed;				/*!< @brief SD speed (unit:100KHz). */
	unsigned int			RCA;				/*!< @brief SD relative card address. */
	unsigned int			capacity;			/*!< @brief Capacity (uint: sector ). */
	unsigned int			CID[4];				/*!< @brief SD card ID. */
	unsigned int			present;			/*!< @brief Card present (initial or not).*/
	spinlock_t				lock;               /*!< @brief For mutual exclusion. */
	spinlock_t				hal_lock;			/*!< @brief For hal dirver lock. */
	struct request_queue 	*queue;    			/*!< @brief The device request queue. */
	struct task_struct		*thread;			/*!< @brief Process thread. */
	struct semaphore		thread_sem;			/*!< @brief Thread semaphore. */
	struct scatterlist		*sg;				/*!< @brief SD scatter list. */
	struct request			*req;				/*!< @brief SD request. */
	struct gendisk 			*gd;             	/*!< @brief The gendisk structure. */
	struct timer_list 		timer;        		/*!< @brief For simulated media changes. */
	struct work_struct		init;				/*!< @brief SD initial work queue. */
	struct work_struct		uninit;				/*!< @brief SD un-initial work queue. */
	gp_board_sd_t			*sd_func;			/*!< @brief Card detect, write protect, power function. */
	int 					pin_handle;			/*!< @brief Pin request handle */
	int 					handle_dma;			/*!< @brief Dma handle. */
	gpApbdma0Param_t		dma_param;			/*!< @brief Dma parameter. */
	short 					users;				/*!< @brief How many users. */
	unsigned short 			cnt;				/*!< @brief For detect io debounce. */		
} gpSDInfo_t;

/*
 * ioctl calls that are permitted to the /dev/uart interface, if
 * any of the SD drivers are enabled.
 */
#define SD_CLK_SET	_IOW('D', 0x01, unsigned int)		/*!< @brief Set SD clock. */
#define SD_CLK_BUS	_IOW('D', 0x02, unsigned int)		/*!< @brief Set SD bus width. */

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

#endif /* _GP_SD_H_ */