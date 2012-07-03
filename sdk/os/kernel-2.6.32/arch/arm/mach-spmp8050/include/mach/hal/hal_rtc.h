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
 * @file hal_rtc.h
 * @brief RTC HAL Operation API header
 * @author chao.chen
 */

#ifndef _HAL_RTC_H_
#define _HAL_RTC_H_

/**************************************************************************
 *                              CONSTANTS                               *
 **************************************************************************/
/*RTC Regs bits definition....*/
/*RTC_SIEN*/
#define RTC_MACRO_CLK_ENABLE  0x01
#define RTC_MACRO_CLK_DISABLE 0x00

#define RTC_MARCO_READY       0x01

/*RTC Macro Internal Register address*/
#define RTC_CTL_MARCO            (0x0)  /*RTC control Macro*/
#define RTC_RRP_MARCO            (0x1)  /*reduce PMOS resistor value signal*/
#define RTC_RELIABLECODE_MARCO   (0x2)  /*reliable code*/
#define RTC_FDEN_MARCO           (0x3)  /*Power Fail Detect Enable*/

#define RTC_LOADCNTBIT_7_0_MARCO    (0x10)	/*Starting value of the timer*/
#define RTC_LOADCNTBIT_15_8_MARCO   (0x11)
#define RTC_LOADCNTBIT_23_16_MARCO  (0x12)
#define RTC_LOADCNTBIT_31_24_MARCO  (0x13)
#define RTC_LOADCNTBIT_39_32_MARCO  (0x14)
#define RTC_LOADCNTBIT_47_40_MARCO  (0x15)

#define RTC_ALARM_7_0_MARCO    (0x20)       /*It is for alarm value*/
#define RTC_ALARM_15_8_MARCO   (0x21)
#define RTC_ALARM_23_16_MARCO  (0x22)
#define RTC_ALARM_31_24_MARCO  (0x23)
#define RTC_ALARM_39_32_MARCO  (0x24)
#define RTC_ALARM_47_40_MARCO  (0x25)

#define RTC_TIMERCNT_7_0_MARCO   (0xA0)   /*The counting value of the timer*/
#define RTC_TIMERCNT_15_8_MARCO  (0xA1)
#define RTC_TIMERCNT_23_16_MARCO (0xA2)
#define RTC_TIMERCNT_31_24_MARCO (0xA3)
#define RTC_TIMERCNT_39_32_MARCO (0xA4)
#define RTC_TIMERCNT_47_40_MARCO (0xA5)

#define RTC_LOAD_START_VALUE_MARCO  (0xB0)  
#define RTC_INTR_STATUS_MARCO       (0xC0)  
#define RTC_INTR_ENABLE_MARCO       (0xD0)

/* ctl r/w for RTC_RWREQ */
#define RTC_CTLWRITE   (0x01)
#define RTC_CTLREAD    (0x02)

/* function of RTC_CTL_MACRO */
#define CTL_RTC_CLKEN  (0x01)
#define CTL_RTCRST     (0x02)
#define CTL_COUNT_UP   (0x04)
#define CTL_WRITE_LOAD (0x08)

/* FDEn */
#define FD_ENABLE  0x01
#define FD_DISABLE 0x00
/* Interrupt status of RTC_INTR_STATUS_MACRO */
#define SEC_INTR_STATUS   (0x01)
#define ALARM_INTR_STATUS (0x02)

/* Interrupt status of RTC_INTR_STATUS_MACRO */
#define SEC_INTR_STATUS   (0x01)
#define ALARM_INTR_STATUS (0x02)

/* Interrupt function of RTC_INTR_ENABLE_MARCO */
#define RTC_SEC_INT_ENABLE    (0x01)
#define RTC_ALARM_INT_ENABLE  (0x02)
#define RTC_WAKEUP_INT_ENABLE (0x04)
#define RTC_INT_ENABLE_MASK   (0x07)

/* Other define */
#define RELIABLE_CODE_CHECK_NUMBER (0xA3)

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern void gpHalRtcSetRegBase(UINT32 baseAddr);

extern UINT32 gpHalRtcIntSrcGet(void);

extern void gpHalRtcIntEnable(UINT8 secIntEn, UINT8 alarmIntEn);

extern UINT32 gpHalRtcGetTime(void);

extern UINT32 gpHalRtcSetTime(UINT32 time);

extern void gpHalRtcGetAlarmStatus(UINT8 *enable, UINT8 *pending, UINT32 *time);

extern void gpHalRtcSetAlarmStatus(UINT8 enable, UINT8 pending, UINT32 time);

extern void gpHalRtcEnable(UINT8 enable);

extern void gpHalRtcClkEnable(SP_BOOL enable);

#endif

