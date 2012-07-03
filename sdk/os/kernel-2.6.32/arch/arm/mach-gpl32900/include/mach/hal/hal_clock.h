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
 * @file    hal_clock.h
 * @brief   Implement of clock HAL API header file.
 * @author  Roger Hsu
 */
 
#ifndef _HAL_CLOCK_H_
#define _HAL_CLOCK_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>
#include <mach/clock_mgr/gp_clock_private.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
#define XTAL_RATE 27000000  //27 MHz
#define RTC_RATE  32768     //32.768KHz
#define USBPHY_RATE 96000000  //96 MHz
#define FCK_48KRATE 96000000  //96 MHz
#define FCK_44K_RATE 96000000  //96 MHz

#define CLK_TREE_SIZE		17

/* arm update bit */
#define APB_RATIO_U		(1<<2)
#define AHB_RATIO_U		(1<<1)
#define ARM_RATIO_U		(1<<0)

#define CEVA_APB_U		(1<<6)
#define CEVA_AHB_U		(1<<5)
#define CEVA_U			(1<<4)
#define SYS_APB_U		(1<<3)
#define SYS_AHB_U		(1<<2)
#define SYS_RT_U		(1<<1) /* do not use */
#define SYS_U			(1<<0)

/* LCD clock define */
#define HAL_LCD_CLK_SEL_OFFSET		(16)
#define HAL_LCD_CLK_XTAL	(1<<16)
#define HAL_LCD_CLK_SPLL	(0<<16)
#define HAL_LCD_CLK_USBPHY	(6<<16)

/* APLL define */
#define AUDIO_APLL_SEL_48K  0x00
#define AUDIO_APLL_SEL_44K  0x02

//clock register bit definition
#define LCD_CLK_SEL		(1<<16)
#define LCD_CLK_EN		(1<<8)
#define CSI_CLK_SEL		(1<<16)
#define CSI_CLK_EN		(1<<8)

/* SYS clock enable define */
#define SYS_APB_EN		(1<<3)
#define SYS_AHB_EN		(1<<2)
#define SYS_EN			(1<<0)
/* CEVA clock enable define */
#define CEVA_APB_EN		(1<<2)
#define CEVA_AHB_EN		(1<<1)
#define CEVA_EN			(1<<0)

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/
//gpClockClass_t;
enum {
	SCU_A,
	SCU_B,
	SCU_C,
	SCU_D,
	SCU_A2,
	SCU_BASE_SYS,	//base clock_sys
	SCU_BASE_CEVA,	//base clock_ceva
	SCN_MAX
};

// SCU A  Peripheral Clock bit
#define		SCU_A_PERI_LCD_CTRL			(1 << 1)
#define		SCU_A_PERI_USB0				(1 << 3)
#define		SCU_A_PERI_USB1				(1 << 4)
#define		SCU_A_PERI_LINEBUFFER		(1 << 5)
#define		SCU_A_PERI_APBDMA_A			(1 << 9)
#define		SCU_A_PERI_CMOS_CTRL		(1 << 10)
#define		SCU_A_PERI_NAND0			(1 << 11)
#define		SCU_A_PERI_BCH				(1 << 13)
#define		SCU_A_PERI_APLL				(1 << 14)
#define		SCU_A_PERI_UART_CON			(1 << 15)
#define		SCU_A_PERI_AAHBM212	    	(1 << 16)
#define		SCU_A_PERI_I2S				(1 << 17)
#define		SCU_A_PERI_I2SRX			(1 << 18)
#define		SCU_A_PERI_SAACC			(1 << 19)
#define		SCU_A_PERI_NAND_ABT			(1 << 20)
#define		SCU_A_PERI_REALTIME_ABT		(1 << 21)
#define		SCU_A_PERI_RTABT212			(1 << 22)
#define		SCU_A_PERI_CAHBM212			(1 << 23)
#define		SCU_A_PERI_SPU  			(1 << 25)
#define		SCU_A_PERI_SCA	    		(1 << 26)
#define		SCU_A_PERI_OVG			    (1 << 27)
#define		SCU_A_PERI_MIPI			    (1 << 28)
#define		SCU_A_PERI_CDSP			    (1 << 29)
#define		SCU_A_PERI_AES			    (1 << 30)

#define     SCU_A_PERI_PPU_SPR			(1 << 0)
#define     SCU_A_PERI_CEVA_L2RAM		(1 << 1)
#define     SCU_A_PERI_PPU  			(1 << 24)
#define     SCU_A_PERI_PPU_REG  		(1 << 25)
#define     SCU_A_PERI_PPU_TFT 			(1 << 26)
#define     SCU_A_PERI_PPU_STN  		(1 << 27)
#define     SCU_A_PERI_PPU_TV 			(1 << 28)
#define     SCU_A_PERI_PPU_FB  			(1 << 29)

// SCU B  Peripheral Clock bit
#define		SCU_B_PERI_AHB2AHB			(1 << 2)
#define		SCU_B_PERI_VIC0				(1 << 4)
#define		SCU_B_PERI_VIC1				(1 << 5)
#define		SCU_B_PERI_TIMER0			(1 << 9)
#define		SCU_B_PERI_TIMER1			(1 << 10)	
#define		SCU_B_PERI_UART 			(1 << 11)															
#define		SCU_B_PERI_I2C				(1 << 12)								
#define		SCU_B_PERI_RAND				(1 << 13)								
#define		SCU_B_PERI_GPIO				(1 << 14)
#define		SCU_B_PERI_RTC				(1 << 15)

// SCU C  Peripheral Clock bit
#define		SCU_C_PERI_DMAC0			(1 << 1)
#define		SCU_C_PERI_DMAC1			(1 << 2)
#define		SCU_C_PERI_DRAM_CTRL		(1 << 4)
#define		SCU_C_PERI_APBDMA			(1 << 7)								
#define		SCU_C_PERI_MS				(1 << 11)	
#define		SCU_C_PERI_INT_MEM			(1 << 12)	
#define		SCU_C_PERI_UART_C0			(1 << 13)	
#define		SCU_C_PERI_UART_C2			(1 << 15)	
#define		SCU_C_PERI_SSP0				(1 << 16)
#define		SCU_C_PERI_SSP1				(1 << 17)	
#define		SCU_C_PERI_SD0				(1 << 18)
#define		SCU_C_PERI_SD1				(1 << 19)
#define		SCU_C_PERI_I2C				(1 << 20)
#define		SCU_C_PERI_SCALING			(1 << 21)
#define 	SCU_C_PERI_2DSCALEABT		(1 << 22)
#define 	SCU_C_PERI_TI2C 			(1 << 23)
#define		SCU_C_PERI_SYS_A			(1 << 24)
#define		SCU_C_PERI_CXMP_SL			(1 << 25)
#define		SCU_C_PERI_CXMD_SL			(1 << 26)
#define		SCU_C_PERI_CIR			    (1 << 27)
#define		SCU_C_PERI_EFUSE			(1 << 29)

enum {
	UPDATE_ARM_CLK=0,
	UPDATE_SYS_CEVA_CLK
};

/* structure of clock tree */
typedef struct gpHalClockTree_s {
	UINT32	armSrc		;
	UINT32	clkSpll		;
	UINT32	clkSpll2	;
	UINT32	clkRefArm	;
	UINT32	clkArm		;
	UINT32	alkArmApb	;
	UINT32	clkArmAhb	;
	UINT32	clkRefCeva	;
	UINT32	clkSys		;
	UINT32	clkSysApb	;
	UINT32	clkSysAhb	;
	UINT32	clkCeva		;
	UINT32	clkCevaApb	;
	UINT32	clkCevaAhb	;
	UINT32	clkUclk		;
	UINT32	clkCsi		;
	UINT32	clkLcd		;
} gpHalClockTree_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/**
* @todo : remove in the furture
* @brief 	LCD clock config hardware function.
* @param 	workFreq[in]: LCD frequency
* @return 	none
*/
SINT32 gpHalLcdScuEnable(UINT32 workFreq);

/**
* @brief 	LCD clock enable hardware function.
* @param 	devinfo[in]: device into
* @param 	enable[in]: enable
* @return 	SUCCESS/ERROR_ID.
*/
SINT32 gpHalLcdClkEnable(UINT32 enable);

/**
* @brief 	LCD clock get hardware function.
* @param 	none
* @return 	lcd clock
*/
SINT32 
gpHalLcdGetFreq(
	void
);

/**
* @brief 	SCU A clock enable function
* @param 	bitMask[in]: enable bits
* @param 	scu[in]: 0 : SCU_A, 1 : SCU_B, 2: SCU_C
* @param 	enable[in]: 1 : enable, 0 : disable
* @return 	SUCCESS/SP_FAIL.
*/
SINT32
gpHalScuClkEnable(
	UINT32 bitMask, UINT8 scu, UINT8 enable
);

/**
* @brief 	Hal lcd clock set function
* @param 	source[in]: 0: SPLL, 1: XTAL(27MHz), 2: USBPHY(96MHz)
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkLcdSetSrc(UINT32 source);

/**
* @brief 	SCU A USB PHY clock enable function
* @param 	enable[in]: 1 : enable, 0 : disable
* @return 	none
*/
void gpHalScuUsbPhyClkEnable ( int enable );

/**
* @brief 	spi clock enable hardware function.
* @param 	devinfo[in]: device into
* @param 	enable[in]: enable
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpSpiClkEnable(void* devinfo, SP_BOOL enable);

/**
* @brief 	SCU A clock enable function, for test
* @param 	clkBuf[in]: clock tree buffer
* @return 	SUCCESS/SP_FAIL.
*/
SINT32 gpHalClockChange(UINT32 *clkBuf);

/**
* @brief 	clock update active function
* @param 	clkSrc[in]: clock source, 0 : arm clock tree, 1 : sys/ceva clock tree
* @param 	value[in]: update value
* @return 	SUCCESS/SP_FAIL.
*/
SINT32 gpHalClockUpdate(UINT32 clkSrc, UINT32 value);

/**************************************************************************
	APLL Function
***************************************************************************/
/**
* @brief 	Hal apll clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalApllSetRate(UINT32 rate);

/**
* @brief 	Hal apll clock enable function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalApllClkEnable(UINT32 enable);

/**************************************************************************
	ARM CLOCK Function
***************************************************************************/
/**
* @brief 	Hal arm clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkArmSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal arm ahb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkArmAhbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal arm apb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkArmApbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**************************************************************************
	CEVA CLOCK Function
***************************************************************************/
/**
* @brief 	Hal ceva clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCevaSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal ceva ahb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCevaAhbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal ceva apb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCevaApbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**************************************************************************
	SYS CLOCK Function
***************************************************************************/
/**
* @brief 	Hal sys ahb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkSysAhbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal sys apb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkSysApbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal spll2 clock set function
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkSpll2SetRate(UINT32 rate, UINT32 *realRate);

/**************************************************************************
	Other CLOCK Function
***************************************************************************/
/**
* @brief 	Hal lcd clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkLcdSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal cmos clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCsiSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal I2S clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkI2sSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate);

/**
* @brief 	Hal clock enable/disable function
* @param 	clk[in]: clock structure
* @param 	enable[in]: 1 : enable , 0 : disable
* @return 	SUCCESS/FAIL.
*/
int gpHalClockEnable(gp_clk_t *clk, UINT32 enable);

/**
* @brief 	Hal keep enable clock check
* @param 	scua_peri_clock[out]: bits keep SCUA clock enable
* @param 	scub_peri_clock[out]: bits keep SCUB clock enable
* @param 	scuc_peri_clock[out]: bits keep SCUC clock enable
* @param 	scua2_peri_clock[out]: bits keep SCUA2 clock enable
* @return 	SUCCESS/FAIL.
*/
int gpHalPeriClokcCheck(unsigned int *scua_peri_clock, unsigned int *scub_peri_clock, unsigned int *scuc_peri_clock, unsigned int *scua2_peri_clock);


/**
* @todo 	jimmy's interface, merge/remove later
*/
SP_BOOL tvout_clk_enable(void* devinfo, SP_BOOL enable);
SP_BOOL lcm_clk_enable(void* devinfo, SP_BOOL enable);
SP_BOOL audioplay_clk_enable(void* devinfo, SP_BOOL enable);
SP_BOOL audiorec_clk_enable(void* devinfo, SP_BOOL enable);



#endif  /* _HAL_SD_H */