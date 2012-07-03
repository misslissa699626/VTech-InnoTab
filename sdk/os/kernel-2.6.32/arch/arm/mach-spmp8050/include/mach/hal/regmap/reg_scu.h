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
 * @file    reg_scu.h
 * @brief   Regmap of SPMP8050 SCU
 * @author  qinjian
 * @since   2010-9-29
 * @date    2010-9-29
 */
#ifndef _REG_SCU_H_
#define _REG_SCU_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LOGI_ADDR_SCU_A_REG     IO3_ADDRESS(0x7000)
#define LOGI_ADDR_SCU_B_REG     IO0_ADDRESS(0x5000)
#define LOGI_ADDR_SCU_C_REG     IO2_ADDRESS(0x5000)
#define LOGI_ADDR_SCU_D_REG     IO0_ADDRESS(0x532800)

/* SCU_A Peripheral Clock bit */
#define SCU_A_PERI_SYSA         (1 << 0)
#define SCU_A_PERI_LCD_CTRL     (1 << 1)
#define SCU_A_PERI_DRM          (1 << 2)
#define SCU_A_PERI_USB0         (1 << 3)
#define SCU_A_PERI_USB1         (1 << 4)
#define SCU_A_PERI_LINEBUFFER   (1 << 5)
#define SCU_A_PERI_SCUA         (1 << 6)
#define SCU_A_PERI_TVOUT        (1 << 7)
#define SCU_A_PERI_APBDMA_A     (1 << 9)
#define SCU_A_PERI_CMOS_CTRL    (1 << 10)
#define SCU_A_PERI_NAND0        (1 << 11)
#define SCU_A_PERI_NAND1        (1 << 12)
#define SCU_A_PERI_BCH          (1 << 13)
#define SCU_A_PERI_APLL         (1 << 14)
#define SCU_A_PERI_UART_CNCT    (1 << 15)
#define SCU_A_PERI_AAHBM_SLICE  (1 << 16)
#define SCU_A_PERI_I2S          (1 << 17)
#define SCU_A_PERI_I2SRX        (1 << 18)
#define SCU_A_PERI_SAACC        (1 << 19)
#define SCU_A_PERI_NAND_ABT     (1 << 20)
#define SCU_A_PERI_REALTIME_ABT (1 << 21)
#define SCU_A_PERI_RTABT212     (1 << 22)
#define SCU_A_PERI_CAHBM212     (1 << 23)

/* SCU_B Peripheral Clock bit */
#define SCU_B_PERI_TCM_BIST     (1 << 0)
#define SCU_B_PERI_TCM_CTRL     (1 << 1)
#define SCU_B_PERI_AHB2AHB      (1 << 2)
#define SCU_B_PERI_AHB_SW       (1 << 3)
#define SCU_B_PERI_VIC0         (1 << 4)
#define SCU_B_PERI_VIC1         (1 << 5)
#define SCU_B_PERI_DPM          (1 << 6)
#define SCU_B_PERI_APB_BRG      (1 << 7)
#define SCU_B_PERI_ARM926       (1 << 8)
#define SCU_B_PERI_TIMER0       (1 << 9)
#define SCU_B_PERI_TIMER1       (1 << 10)
#define SCU_B_PERI_UART         (1 << 11)
#define SCU_B_PERI_I2C          (1 << 12)
#define SCU_B_PERI_RAND         (1 << 13)
#define SCU_B_PERI_GPIO         (1 << 14)
#define SCU_B_PERI_RTC          (1 << 15)

/* SCU_C Peripheral Clock bit */
#define SCU_C_PERI_FABRIC_C     (1 << 0)
#define SCU_C_PERI_DMAC0        (1 << 1)
#define SCU_C_PERI_DMAC1        (1 << 2)
#define SCU_C_PERI_MEM_CTRL     (1 << 3)
#define SCU_C_PERI_DRAM_CTRL    (1 << 4)
#define SCU_C_PERI_SCU_C        (1 << 5)
#define SCU_C_PERI_I2C_CFG      (1 << 6)
#define SCU_C_PERI_APBDMA       (1 << 7)
#define SCU_C_PERI_2D_ENGIN     (1 << 8)
#define SCU_C_PERI_NOR          (1 << 9)
#define SCU_C_PERI_CF           (1 << 10)
#define SCU_C_PERI_MS           (1 << 11)
#define SCU_C_PERI_RAM          (1 << 12)
#define SCU_C_PERI_UART_C0      (1 << 13)
#define SCU_C_PERI_UART_C1      (1 << 14)
#define SCU_C_PERI_UART_C2      (1 << 15)
#define SCU_C_PERI_SSP0         (1 << 16)
#define SCU_C_PERI_SSP1         (1 << 17)
#define SCU_C_PERI_SD0          (1 << 18)
#define SCU_C_PERI_SD1          (1 << 19)
#define SCU_C_PERI_I2C          (1 << 20)
#define SCU_C_PERI_SCALING      (1 << 21)
#define SCU_C_PERI_2DSCALEABT   (1 << 22)
#define SCU_C_PERI_TI2C         (1 << 23)
#define SCU_C_PERI_FRBRIC_A     (1 << 24)
#define SCU_C_PERI_CXMP_SL      (1 << 25)
#define SCU_C_PERI_CXMD_SL      (1 << 26)
#define SCU_C_PERI_CIR          (1 << 27)
#define SCU_C_PERI_ROTATOR      (1 << 28)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/* SCU_A Control Unit */
typedef struct scuaReg_s {
	volatile UINT32 scuaPeriRst;        /* 0x0000 ~ 0x0003 Peripheral Reset */
	volatile UINT32 scuaPeriClkEn;      /* 0x0004 ~ 0x0007 Peripheral Clock Enable */
	volatile UINT32 dummy1;             /* 0x0008 ~ 0x000B */
	volatile UINT32 scuaPeriDgClkEn;    /* 0x000C ~ 0x000F Dynamic Clock Gating Enable */

	volatile UINT32 scuaDispType;       /* 0x0010 ~ 0x0013 Display Type */
	volatile UINT32 dummy2[10];         /* 0x0014 ~ 0x003B */
	volatile UINT32 scuaUsbPhyCfg;      /* 0x003C ~ 0x003F USBPHY Configure */

	volatile UINT32 scuaVdacCfg;        /* 0x0040 ~ 0x0043 VDAC Configure */
	volatile UINT32 scuaApllCfg;        /* 0x0044 ~ 0x0047 Audio PLL Configure */
	volatile UINT32 dummy3[2];          /* 0x0048 ~ 0x004F */

	volatile UINT32 scuaCpuDbgCtrlA;    /* 0x0050 ~ 0x0053 For Chip Debug Only */
	volatile UINT32 scuaCpuDbgStatA;    /* 0x0054 ~ 0x0057 For Chip Debug Only */
	volatile UINT32 dummy4[10];         /* 0x0058 ~ 0x007F */

	volatile UINT32 scuaLcdClkCfg;      /* 0x0080 ~ 0x0083 LCD Clock Configure */
	volatile UINT32 scuaCsiClkCfg;      /* 0x0084 ~ 0x0087 CSI Clock Configure */
	volatile UINT32 dummy5[2];          /* 0x0088 ~ 0x008F */

	volatile UINT32 scuaI2sBckCfg;      /* 0x0090 ~ 0x0093 I2S BCK Configure */
	volatile UINT32 scuaUartCfg;        /* 0x0094 ~ 0x0097 Uart Configure */
	volatile UINT32 dummy6[6];          /* 0x0098 ~ 0x00AF */

	volatile UINT32 scuaCodecCfg;       /* 0x00B0 ~ 0x00B3 I2C Codec Configure */
	volatile UINT32 dummy7[11];         /* 0x00B4 ~ 0x00DF */

	volatile UINT32 scuaSarGpioCtrl;    /* 0x00E0 ~ 0x00E3 SAR GPIO Control */
	volatile UINT32 scuaSarGpioOen;     /* 0x00E4 ~ 0x00E7 SAR GPIO Output Enable */
	volatile UINT32 scuaSarGpioO;       /* 0x00E8 ~ 0x00EB SAR GPIO Output */
	volatile UINT32 scuaSarGpioI;       /* 0x00EC ~ 0x00EF SAR GPIO Input */
} scuaReg_t;

/* SCU_B Control Unit */
typedef struct scubReg_s {
	volatile UINT32 scubPeriRst;        /* 0x0000 ~ 0x0003 Peripheral Reset */
	volatile UINT32 scubSpllCfg;        /* 0x0004 ~ 0x0007 SPLL Configure */
	volatile UINT32 scubIntrStatus;     /* 0x0008 ~ 0x000B Interrupt Status */
	volatile UINT32 scubTimerIceEn;     /* 0x000C ~ 0x000F Timer Ice Enable */

	volatile UINT32 scubTimerExtCtrl;   /* 0x0010 ~ 0x0013 Timer External Control */
	volatile UINT32 scubOtp0;           /* 0x0014 ~ 0x0017 OTP0 */
	volatile UINT32 scubRev;            /* 0x0018 ~ 0x001B Chip Version */
	volatile UINT32 scubRand0;          /* 0x001C ~ 0x001F Randomize Number Seed 0 */

	volatile UINT32 scubPeriClkEn;      /* 0x0020 ~ 0x0023 Peripheral Clock Enable */
	volatile UINT32 scubPeriDgClkEn;    /* 0x0024 ~ 0x0027 Dynamic Clock Gating Enable */
	volatile UINT32 scubUpdateRatio;    /* 0x0028 ~ 0x002B ARM Ratio Update */
	volatile UINT32 dummy1[5];          /* 0x002C ~ 0x003F */

	volatile UINT32 scubPwrcCfg;        /* 0x0040 ~ 0x0043 Power Configure */
	volatile UINT32 scubOtp1;           /* 0x0044 ~ 0x0047 OTP1 */
	volatile UINT32 scubRand1;          /* 0x0048 ~ 0x004B Randomize Number Seed 1 */
	volatile UINT32 scubOtp2;           /* 0x004C ~ 0x004F OTP2 */

	volatile UINT32 scubIoIcCtrl;       /* 0x0050 ~ 0x0053 IC Control */
	volatile UINT32 dummy2;             /* 0x0054 ~ 0x0057 */
	volatile UINT32 scubDbgRqCtrl;      /* 0x0058 ~ 0x005B Dynamic Gated Clock Control */
	volatile UINT32 scubSpllCfg2;       /* 0x005C ~ 0x005F SPLL Configure 2 */

	volatile UINT32 dummy3[8];          /* 0x0060 ~ 0x007F */

	volatile UINT32 scubPadGrpSel0;     /* 0x0080 ~ 0x0083 PAD Group Selection 0 */
	volatile UINT32 scubPadGrpSel1;     /* 0x0084 ~ 0x0087 PAD Group Selection 1 */
	volatile UINT32 scubPadGrpSel2;     /* 0x0088 ~ 0x008B PAD Group Selection 2 */
	volatile UINT32 scubPadGrpSel3;     /* 0x008C ~ 0x008F PAD Group Selection 3 */

	volatile UINT32 scubPadGrpCtrl0;    /* 0x0090 ~ 0x0093 PAD Group Control 0 */
	volatile UINT32 scubPadGrpCtrl1;    /* 0x0094 ~ 0x0097 PAD Group Control 1 */
	volatile UINT32 scubPadGrpCtrl2;    /* 0x0098 ~ 0x009B PAD Group Control 2 */
	volatile UINT32 scubPadGrpCtrl3;    /* 0x009C ~ 0x009F PAD Group Control 3 */

	volatile UINT32 dummy4[4];          /* 0x00A0 ~ 0x00AF */

	volatile UINT32 scubCpuDbgCtrl;     /* 0x00B0 ~ 0x00B3 For Chip Debug Only */
	volatile UINT32 scubCpuDbgStat;     /* 0x00B4 ~ 0x00B7 For Chip Debug Only */
	volatile UINT32 dummy5;             /* 0x00B8 ~ 0x00BB */
	volatile UINT32 scubPadCtrl0;       /* 0x00BC ~ 0x00BF PAD Control 0 */

	volatile UINT32 dummy6[4];          /* 0x00C0 ~ 0x00CF */

	volatile UINT32 scubArmRatio;       /* 0x00D0 ~ 0x00D3 ARM Ratio */
	volatile UINT32 scubArmAhbRatio;    /* 0x00D4 ~ 0x00D7 ARM AHB Ratio */
	volatile UINT32 scubArmApbRatio;    /* 0x00D8 ~ 0x00DB ARM APB Ratio */
	volatile UINT32 scubSysCntEn;       /* 0x00DC ~ 0x00DF System Counter Enable */

	volatile UINT32 dummy7[8];          /* 0x00E0 ~ 0x00FF */

	volatile UINT32 scubGpio0PinEn;     /* 0x0100 ~ 0x0103 GPIO0 Input Enable */
	volatile UINT32 scubGpio0PinDs;     /* 0x0104 ~ 0x0107 GPIO0 Driving Strength */
	volatile UINT32 scubGpio0PinPe;     /* 0x0108 ~ 0x010B GPIO0 Pull Enable */
	volatile UINT32 scubGpio0PinPs;     /* 0x010C ~ 0x010F GPIO0 Pull Select */

	volatile UINT32 scubGpio1PinEn;     /* 0x0110 ~ 0x0113 GPIO1 Input Enable */
	volatile UINT32 scubGpio1PinDs;     /* 0x0114 ~ 0x0117 GPIO1 Driving Strength */
	volatile UINT32 scubGpio1PinPe;     /* 0x0118 ~ 0x011B GPIO1 Pull Enable */
	volatile UINT32 scubGpio1PinPs;     /* 0x011C ~ 0x011F GPIO1 Pull Select */

	volatile UINT32 scubGpio2PinEn;     /* 0x0120 ~ 0x0123 GPIO2 Input Enable */
	volatile UINT32 scubGpio2PinDs;     /* 0x0124 ~ 0x0127 GPIO2 Driving Strength */
	volatile UINT32 scubGpio2PinPe;     /* 0x0128 ~ 0x012B GPIO2 Pull Enable */
	volatile UINT32 scubGpio2PinPs;     /* 0x012C ~ 0x012F GPIO2 Pull Select */

	volatile UINT32 scubGpio3PinEn;     /* 0x0130 ~ 0x0133 GPIO3 Input Enable */
	volatile UINT32 scubGpio3PinDs;     /* 0x0134 ~ 0x0137 GPIO3 Driving Strength */
	volatile UINT32 scubGpio3PinPe;     /* 0x0138 ~ 0x013B GPIO3 Pull Enable */
	volatile UINT32 scubGpio3PinPs;     /* 0x013C ~ 0x013F GPIO3 Pull Select */
} scubReg_t;

/* SCU_C Control Unit */
typedef struct scucReg_s {
	volatile UINT32 scucPeriRst;        /* 0x0000 ~ 0x0003 Peripheral Reset */
	volatile UINT32 scucPeriClkEn;      /* 0x0004 ~ 0x0007 Peripheral Clock Enable */
	volatile UINT32 scucPeriDgClkEn;    /* 0x0008 ~ 0x000B Dynamic Gating Clock Enable */
	volatile UINT32 dummy0[1];          /* 0x000C ~ 0x000F */

	volatile UINT32 scucGcCfg0;         /* 0x0010 ~ 0x0013 DDRPHY Configure 0 */
	volatile UINT32 dummy1[5];          /* 0x0014 ~ 0x0027 */
	volatile UINT32 scucSysRatioUpdate; /* 0x0028 ~ 0x002B System Ratio Update */
	volatile UINT32 scucRomAddr0Cyc;    /* 0x002C ~ 0x002F */
	volatile UINT32 scucRomData0Cyc;    /* 0x0030 ~ 0x0033 */
	volatile UINT32 scucRomData1Cyc;    /* 0x0034 ~ 0x0037 */
	volatile UINT32 scucRomData2Cyc;    /* 0x0038 ~ 0x003B */
	volatile UINT32 scucRomData3Cyc;    /* 0x003C ~ 0x003F */

	volatile UINT32 dummy2[2];          /* 0x0040 ~ 0x0047 */
	volatile UINT32 scucDdrPhyCtrl0;    /* 0x0048 ~ 0x004B DDRPHY Control 0 */
	volatile UINT32 scucDdrPhyCtrl1;    /* 0x004C ~ 0x004F DDRPHY Control 1 */

	volatile UINT32 dummy3[22];         /* 0x0050 ~ 0x00A7 */
	volatile UINT32 scucCpuDbgCtrl;     /* 0x00A8 ~ 0x00AB For Chip Debug Only */
	volatile UINT32 scucCpuDbgStat;     /* 0x00AC ~ 0x00AF For Chip Debug Only */

	volatile UINT32 scucNrReg0;         /* 0x00B0 ~ 0x00B3 NO_RST_REG0, this value will keep during reset */
	volatile UINT32 scucNrReg1;         /* 0x00B4 ~ 0x00B7 NO_RST_REG1, this value will keep during reset */
	volatile UINT32 scucNrReg2;         /* 0x00B8 ~ 0x00BB NO_RST_REG2, this value will keep during reset */
	volatile UINT32 scucNrReg3;         /* 0x00BC ~ 0x00BF NO_RST_REG3, this value will keep during reset */

	volatile UINT32 scucTas0;           /* 0x00C0 ~ 0x00C3 TEST_AND_SET[0], the value will be 0 for winner only */
	volatile UINT32 scucTas1;           /* 0x00C4 ~ 0x00C7 TEST_AND_SET[1], the value will be 0 for winner only */
	volatile UINT32 scucTas2;           /* 0x00C8 ~ 0x00CB TEST_AND_SET[2], the value will be 0 for winner only */
	volatile UINT32 scucTas3;           /* 0x00CC ~ 0x00CF TEST_AND_SET[3], the value will be 0 for winner only */
	volatile UINT32 scucTas4;           /* 0x00D0 ~ 0x00D3 TEST_AND_SET[4], the value will be 0 for winner only */
	volatile UINT32 scucTas5;           /* 0x00D4 ~ 0x00D7 TEST_AND_SET[5], the value will be 0 for winner only */
	volatile UINT32 scucTas6;           /* 0x00D8 ~ 0x00DB TEST_AND_SET[6], the value will be 0 for winner only */
	volatile UINT32 scucTas7;           /* 0x00DC ~ 0x00DF TEST_AND_SET[7], the value will be 0 for winner only */

	volatile UINT32 dummy4[8];          /* 0x00E0 ~ 0x00FF */

	volatile UINT32 scucSysRatio;       /* 0x0100 ~ 0x0103 System Clock Ratio */
	volatile UINT32 scucSysRtRatio;     /* 0x0104 ~ 0x0107 System RealTime Clock Ratio */
	volatile UINT32 scucSysAhbRatio;    /* 0x0108 ~ 0x010B System AHB Clock Ratio */
	volatile UINT32 scucSysApbRatio;    /* 0x010C ~ 0x010F System APB Clock Ratio */

	volatile UINT32 scucCevaRatio;      /* 0x0110 ~ 0x0113 CEVA X1620 Clock Ratio */
	volatile UINT32 scucCevaAhbRatio;   /* 0x0114 ~ 0x0117 CEVA AHB Clock Ratio */
	volatile UINT32 scucCevaApbRatio;   /* 0x0118 ~ 0x011B CEVA APB Clock Ratio */
	volatile UINT32 scucCevaCntEn;      /* 0x011C ~ 0x011F CEVA Counter Enable */
} scucReg_t;

/* SCU_D Control Unit */
typedef struct scudReg_s {
	volatile UINT32 scudDramMap;        /* 0x0000 ~ 0x0003 DRAM Map */
	volatile UINT32 scudSb0Rgn;         /* 0x0004 ~ 0x0007 Spare Space Control 0 */
	volatile UINT32 scudSb1Rgn;         /* 0x0008 ~ 0x000B Spare Space Control 1 */
	volatile UINT32 scudPmimSt;         /* 0x000C ~ 0x000F PMIM Status */
	volatile UINT32 scudDmimSt;         /* 0x0010 ~ 0x0013 DMIM Status */
	volatile UINT32 scudCxsCtrl;        /* 0x0014 ~ 0x0017 CEVA System Control */
	volatile UINT32 scudCxpAddr;        /* 0x0018 ~ 0x001B CEVA X1620 Program Address */
	volatile UINT32 scudCxdAddr;        /* 0x001C ~ 0x001F CEVA X1620 Data Address */

	volatile UINT32 dummy1[2];          /* 0x0020 ~ 0x0027 */
	volatile UINT32 scudGClkEn;         /* 0x0028 ~ 0x002B GCLKEN */
	volatile UINT32 scudDgClkEn;        /* 0x002C ~ 0x002F Dynamic Clock Gating Enable */

	volatile UINT32 scudCpuDbgCtrl;     /* 0x0030 ~ 0x0033 For Chip Debug Only */
	volatile UINT32 scudCpuDbgStat;     /* 0x0034 ~ 0x0037 For Chip Debug Only */
#if 0
	volatile UINT32 scudCevaGpio;       /* 0x0038 ~ 0x003B CEVA GPIO */
#endif
} scudReg_t;



#define SCU_A_BASE			IO3_ADDRESS(0x7000)
#define SCU_B_BASE			IO0_ADDRESS(0x5000)
#define SCU_C_BASE			IO2_ADDRESS(0x5000)
#define SCU_D_BASE			IO0_ADDRESS(0x532800)


/*  SCU_A  Control Unit */
#define SCUA_A_PERI_RST			(*(volatile unsigned int*)(SCU_A_BASE+0x00))
#define SCUA_A_PERI_CLKEN		(*(volatile unsigned int*)(SCU_A_BASE+0x04))
#define SCUA_A_PERI_DGCLKEN		(*(volatile unsigned int*)(SCU_A_BASE+0x0C))
#define SCUA_LCD_TYPE_SEL		(*(volatile unsigned int*)(SCU_A_BASE+0x10))
#define SCUA_A_PERI_CLKEN2		(*(volatile unsigned int*)(SCU_A_BASE+0x18))
#define SCUA_USBPHY_CFG			(*(volatile unsigned int*)(SCU_A_BASE+0x3C))
#define SCUA_VDAC_CFG			(*(volatile unsigned int*)(SCU_A_BASE+0x40))
#define SCUA_APLL_CFG			(*(volatile unsigned int*)(SCU_A_BASE+0x44))
#define SCUA_CPU_DBG_CTRL_A		(*(volatile unsigned int*)(SCU_A_BASE+0x50))
#define SCUA_CPU_DBG_STAT_A		(*(volatile unsigned int*)(SCU_A_BASE+0x54))
#define SCUA_LCD_CLK_CFG		(*(volatile unsigned int*)(SCU_A_BASE+0x80))
#define SCUA_CSI_CLK_CFG		(*(volatile unsigned int*)(SCU_A_BASE+0x84))
#define SCUA_DUMMY2				(*(volatile unsigned int*)(SCU_A_BASE+0x88))
#define SCUA_DUMMY6				(*(volatile unsigned int*)(SCU_A_BASE+0x8C))
#define SCUA_I2S_BCK_CFG		(*(volatile unsigned int*)(SCU_A_BASE+0x90))
#define SCUA_UART_CFG			(*(volatile unsigned int*)(SCU_A_BASE+0x94))
#define SCUA_DUMMY0				(*(volatile unsigned int*)(SCU_A_BASE+0xA0))
#define SCUA_DUMMY1				(*(volatile unsigned int*)(SCU_A_BASE+0xA4))
#define SCUA_CODEC_CFG			(*(volatile unsigned int*)(SCU_A_BASE+0xB0))
#define SCUA_DUMMY3				(*(volatile unsigned int*)(SCU_A_BASE+0xC0))
#define SCUA_DUMMY4				(*(volatile unsigned int*)(SCU_A_BASE+0xC4))
#define SCUA_DUMMY5				(*(volatile unsigned int*)(SCU_A_BASE+0xD0))
#define SCUA_SAR_GPIO_CTRL		(*(volatile unsigned int*)(SCU_A_BASE+0xE0))
#define SCUA_SAR_GPIO_OEN		(*(volatile unsigned int*)(SCU_A_BASE+0xE4))
#define SCUA_SAR_GPIO_O			(*(volatile unsigned int*)(SCU_A_BASE+0xE8))
#define SCUA_SAR_GPIO_I			(*(volatile unsigned int*)(SCU_A_BASE+0xEC))

#define SP_SCUB_WFI				(SCU_B_BASE+0x58)

/*  SCU_B  Control Unit */
#define SCUB_B_PERI_RST			(*(volatile unsigned int*)(SCU_B_BASE+0x00))
#define SCUB_SPLL_CFG0			(*(volatile unsigned int*)(SCU_B_BASE+0x04))
#define SCUB_B_INTR_STATUS		(*(volatile unsigned int*)(SCU_B_BASE+0x08))
#define SCUB_TIMER_ICE_EN		(*(volatile unsigned int*)(SCU_B_BASE+0x0C))
#define SCUB_TIMER_EXT_CTRL		(*(volatile unsigned int*)(SCU_B_BASE+0x10))
#define SCUB_OTP0				(*(volatile unsigned int*)(SCU_B_BASE+0x14))
#define SCUB_REV				(*(volatile unsigned int*)(SCU_B_BASE+0x18))
#define SCUB_RAND0				(*(volatile unsigned int*)(SCU_B_BASE+0x1C))
#define SCUB_B_PERI_CLKEN		(*(volatile unsigned int*)(SCU_B_BASE+0x20))
#define SCUB_B_PERI_DBGCLKEN	(*(volatile unsigned int*)(SCU_B_BASE+0x24))
#define SCUB_B_UPDATE_RATIO		(*(volatile unsigned int*)(SCU_B_BASE+0x28))
#define SCUB_CHIP_DBG_CTRL		(*(volatile unsigned int*)(SCU_B_BASE+0x38))
#define SCUB_CHIP_DBG_STAT		(*(volatile unsigned int*)(SCU_B_BASE+0x3C))
#define SCUB_PWRC_CFG			(*(volatile unsigned int*)(SCU_B_BASE+0x40))
#define SCUB_OTP1				(*(volatile unsigned int*)(SCU_B_BASE+0x44))
#define SCUB_RAND1				(*(volatile unsigned int*)(SCU_B_BASE+0x48))
#define SCUB_OTP2				(*(volatile unsigned int*)(SCU_B_BASE+0x4C))
#define SCUB_IO_TRAP			(*(volatile unsigned int*)(SCU_B_BASE+0x50))
#define SCUB_WFI				(*(volatile unsigned int*)(SCU_B_BASE+0x58))
#define SCUB_SPLL_CFG1			(*(volatile unsigned int*)(SCU_B_BASE+0x5C))
#define SCUB_SPLL_CFG2			(*(volatile unsigned int*)(SCU_B_BASE+0x60))
#if 0
#define SCUB_GPIO2_I			(*(volatile unsigned int*)(SCU_B_BASE+0x64))
#define SCUB_GPIO2_O			(*(volatile unsigned int*)(SCU_B_BASE+0x64))
#define SCUB_GPIO2_E			(*(volatile unsigned int*)(SCU_B_BASE+0x68))
#define SCUB_GPIO3_I			(*(volatile unsigned int*)(SCU_B_BASE+0x70))
#define SCUB_GPIO3_O			(*(volatile unsigned int*)(SCU_B_BASE+0x74))
#define SCUB_GPIO3_E			(*(volatile unsigned int*)(SCU_B_BASE+0x78))
#endif
#define SCUB_PGS0				(*(volatile unsigned int*)(SCU_B_BASE+0x80))
#define SCUB_PGS1				(*(volatile unsigned int*)(SCU_B_BASE+0x84))
#define SCUB_PGS2				(*(volatile unsigned int*)(SCU_B_BASE+0x88))
#define SCUB_PGS3				(*(volatile unsigned int*)(SCU_B_BASE+0x8C))
//#define SCUB_PGC0				(*(volatile unsigned int*)(SCU_B_BASE+0x90))
//#define SCUB_PGC1				(*(volatile unsigned int*)(SCU_B_BASE+0x94))
//#define SCUB_PGC2				(*(volatile unsigned int*)(SCU_B_BASE+0x98))
//#define SCUB_PGC3				(*(volatile unsigned int*)(SCU_B_BASE+0x9C))
#define SCUB_DUMMYREG0			(*(volatile unsigned int*)(SCU_B_BASE+0xA8))
#define SCUB_DUMMYREG1			(*(volatile unsigned int*)(SCU_B_BASE+0xAC))
#define SCUB_CPU_DBG_CTRL		(*(volatile unsigned int*)(SCU_B_BASE+0xB0))
#define SCUB_CPU_DBG_STAT		(*(volatile unsigned int*)(SCU_B_BASE+0xB4))
#define SCUB_DUMMYREG4			(*(volatile unsigned int*)(SCU_B_BASE+0xBC))
#define SCUB_ARM_RATIO			(*(volatile unsigned int*)(SCU_B_BASE+0xD0))
#define SCUB_ARM_AHB_RATIO		(*(volatile unsigned int*)(SCU_B_BASE+0xD4))
#define SCUB_ARM_APB_RATIO		(*(volatile unsigned int*)(SCU_B_BASE+0xD8))
#define SCUB_SYS_CNT_EN			(*(volatile unsigned int*)(SCU_B_BASE+0xDC))

#define SCUB_GPIO0_IE   (*(volatile unsigned int*)(SCU_B_BASE+0x100))
#define SCUB_GPIO0_DS   (*(volatile unsigned int*)(SCU_B_BASE+0x104))
#define SCUB_GPIO0_PE   (*(volatile unsigned int*)(SCU_B_BASE+0x108))
#define SCUB_GPIO0_PS   (*(volatile unsigned int*)(SCU_B_BASE+0x10C))

#define SCUB_GPIO1_IE   (*(volatile unsigned int*)(SCU_B_BASE+0x110))
#define SCUB_GPIO1_DS   (*(volatile unsigned int*)(SCU_B_BASE+0x114))
#define SCUB_GPIO1_PE   (*(volatile unsigned int*)(SCU_B_BASE+0x118))
#define SCUB_GPIO1_PS   (*(volatile unsigned int*)(SCU_B_BASE+0x11C))

#define SCUB_GPIO2_IE   (*(volatile unsigned int*)(SCU_B_BASE+0x120))
#define SCUB_GPIO2_DS   (*(volatile unsigned int*)(SCU_B_BASE+0x124))
#define SCUB_GPIO2_PE   (*(volatile unsigned int*)(SCU_B_BASE+0x128))
#define SCUB_GPIO2_PS   (*(volatile unsigned int*)(SCU_B_BASE+0x12C))

#define SCUB_GPIO3_IE   (*(volatile unsigned int*)(SCU_B_BASE+0x130))
#define SCUB_GPIO3_DS   (*(volatile unsigned int*)(SCU_B_BASE+0x134))
#define SCUB_GPIO3_PE   (*(volatile unsigned int*)(SCU_B_BASE+0x138))
#define SCUB_GPIO3_PS   (*(volatile unsigned int*)(SCU_B_BASE+0x13C))
#define SCUB_PIN_MUX    (*(volatile unsigned int*)(SCU_B_BASE+0x144))

/*  SCU_C  Control Unit */
#define SCUC_C_PERI_RST			(*(volatile unsigned int*)(SCU_C_BASE+0x00))
#define SCUC_C_PERI_CLKEN		(*(volatile unsigned int*)(SCU_C_BASE+0x04))
#define SCUC_C_PERI_DGCLKEN		(*(volatile unsigned int*)(SCU_C_BASE+0x08))
#define SCUC_GC_CFG0			(*(volatile unsigned int*)(SCU_C_BASE+0x10))
#define SCUC_DUMMY_C2			(*(volatile unsigned int*)(SCU_C_BASE+0x18))
#define SCUC_SYS_RATIO_UPDATE	(*(volatile unsigned int*)(SCU_C_BASE+0x28))
#define SCUC_ROM_ADDR0_CYC		(*(volatile unsigned int*)(SCU_C_BASE+0x2C))
#define SCUC_ROM_DATA0_CYC		(*(volatile unsigned int*)(SCU_C_BASE+0x30))
#define SCUC_ROM_DATA1_CYC		(*(volatile unsigned int*)(SCU_C_BASE+0x34))
#define SCUC_ROM_DATA2_CYC		(*(volatile unsigned int*)(SCU_C_BASE+0x38))
#define SCUC_ROM_DATA3_CYC		(*(volatile unsigned int*)(SCU_C_BASE+0x3C))
#define SCUC_DDRPHY_CTRL0		(*(volatile unsigned int*)(SCU_C_BASE+0x48))
#define SCUC_DDRPHY_CTRL1		(*(volatile unsigned int*)(SCU_C_BASE+0x4C))
#define SCUC_DUMMY_C0			(*(volatile unsigned int*)(SCU_C_BASE+0xA0))
#define SCUC_DUMMY_C1			(*(volatile unsigned int*)(SCU_C_BASE+0xA4))
#define SCUC_CPU_DBG_CTRL		(*(volatile unsigned int*)(SCU_C_BASE+0xA8))
#define SCUC_CPU_DBG_STAT		(*(volatile unsigned int*)(SCU_C_BASE+0xAC))
#define SCUC_NR_REG0			(*(volatile unsigned int*)(SCU_C_BASE+0xB0))
#define SCUC_NR_REG1			(*(volatile unsigned int*)(SCU_C_BASE+0xB4))
#define SCUC_NR_REG2			(*(volatile unsigned int*)(SCU_C_BASE+0xB8))
#define SCUC_NR_REG3			(*(volatile unsigned int*)(SCU_C_BASE+0xBC))
#define SCUC_TAS0				(*(volatile unsigned int*)(SCU_C_BASE+0xC0))
#define SCUC_TAS1				(*(volatile unsigned int*)(SCU_C_BASE+0xC4))
#define SCUC_TAS2				(*(volatile unsigned int*)(SCU_C_BASE+0xC8))
#define SCUC_TAS3				(*(volatile unsigned int*)(SCU_C_BASE+0xCC))
#define SCUC_TAS4				(*(volatile unsigned int*)(SCU_C_BASE+0xD0))
#define SCUC_TAS5				(*(volatile unsigned int*)(SCU_C_BASE+0xD4))
#define SCUC_TAS6				(*(volatile unsigned int*)(SCU_C_BASE+0xD8))
#define SCUC_TAS7				(*(volatile unsigned int*)(SCU_C_BASE+0xDC))
#define SCUC_SYS_RATIO			(*(volatile unsigned int*)(SCU_C_BASE+0x100))
#define SCUC_SYS_RT_RATIO		(*(volatile unsigned int*)(SCU_C_BASE+0x104))
#define SCUC_SYS_AHB_RATIO		(*(volatile unsigned int*)(SCU_C_BASE+0x108))
#define SCUC_SYS_APB_RATIO		(*(volatile unsigned int*)(SCU_C_BASE+0x10C))
#define SCUC_CEVA_RATIO			(*(volatile unsigned int*)(SCU_C_BASE+0x110))
#define SCUC_CEVA_AHB_RATIO		(*(volatile unsigned int*)(SCU_C_BASE+0x114))
#define SCUC_CEVA_APB_RATIO		(*(volatile unsigned int*)(SCU_C_BASE+0x118))
#define SCUC_CEVA_CNT_EN		(*(volatile unsigned int*)(SCU_C_BASE+0x11C))

/*  SCU_D  Control Unit */
#define SCUD_DRAM_MAP			(*(volatile unsigned int*)(SCU_D_BASE+0x00))
#define SCUD_SB0_RGN			(*(volatile unsigned int*)(SCU_D_BASE+0x04))
#define SCUD_SB1_RGN			(*(volatile unsigned int*)(SCU_D_BASE+0x08))
#define SCUD_PRIM_ST			(*(volatile unsigned int*)(SCU_D_BASE+0x10))
#define SCUD_DMIM_ST			(*(volatile unsigned int*)(SCU_D_BASE+0x18))
#define SCUD_CXS_CTRL			(*(volatile unsigned int*)(SCU_D_BASE+0x28))
#define SCUD_CXP_ADDR			(*(volatile unsigned int*)(SCU_D_BASE+0x2C))
#define SCUD_DUMMY_D0			(*(volatile unsigned int*)(SCU_D_BASE+0x30))
#define SCUD_DUMMY_D1			(*(volatile unsigned int*)(SCU_D_BASE+0x34))
#define SCUD_GCLKEN				(*(volatile unsigned int*)(SCU_D_BASE+0x38))
#define SCUD_DGCLKEN			(*(volatile unsigned int*)(SCU_D_BASE+0x3C))
#define SCUD_CPU_DBG_CTRL		(*(volatile unsigned int*)(SCU_D_BASE+0x48))
#define SCUD_CPU_DBG_STAT		(*(volatile unsigned int*)(SCU_D_BASE+0x4C))
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_SCU_H_ */
