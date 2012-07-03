/*
 * arch/arm/mach-spmp8000/include/mach/regs-rtc.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * RTC - System peripherals regsters.
 *
 */



#define SAACC_BASE			IO3_ADDRESS(0x1F000)

/* SAR ADC controller */
#define SAACC_SARCTRL	(*(volatile unsigned *) (SAACC_BASE + 0x00))
#define SAACC_CONDLY	(*(volatile unsigned *) (SAACC_BASE + 0x04))
#define SAACC_AUTODLY	(*(volatile unsigned *) (SAACC_BASE + 0x08))
#define SAACC_DEBTIME	(*(volatile unsigned *) (SAACC_BASE + 0x0C))
#define SAACC_PNL	    (*(volatile unsigned *) (SAACC_BASE + 0x10))
#define SAACC_AUX	    (*(volatile unsigned *) (SAACC_BASE + 0x14))
#define SAACC_INTEN	    (*(volatile unsigned *) (SAACC_BASE + 0x18))
#define SAACC_INTF   	(*(volatile unsigned *) (SAACC_BASE + 0x1C))

/* audio ADC/DAC controller */
#define SAACC_PWCTRL	(*(volatile unsigned *) (SAACC_BASE + 0x20))
#define SAACC_LININ	    (*(volatile unsigned *) (SAACC_BASE + 0x24))
#define SAACC_ADCCTRL	(*(volatile unsigned *) (SAACC_BASE + 0x28))
#define SAACC_DACCTRL	(*(volatile unsigned *) (SAACC_BASE + 0x2C))
#define SAACC_AUTOSLP	(*(volatile unsigned *) (SAACC_BASE + 0x30))
#define SAACC_LINOUT	(*(volatile unsigned *) (SAACC_BASE + 0x34))
#define SAACC_HDPHN	    (*(volatile unsigned *) (SAACC_BASE + 0x38))
#define SAACC_TADDA	    (*(volatile unsigned *) (SAACC_BASE + 0x3C))
/* dolphin IIS macro SFR Setting and control */
#define SAACC_ICDC      (*(volatile unsigned *) (SAACC_BASE + 0x40))
#define SAACC_IRDY      (*(volatile unsigned *) (SAACC_BASE + 0x44))
#define SAACC_MC_CMD    (*(volatile unsigned *) (SAACC_BASE + 0x48))
#define SAACC_MC_RDATA  (*(volatile unsigned *) (SAACC_BASE + 0x4C))

#define SAACC_SARCTRL_OFST 0x00
#define SAACC_CONDLY_OFST  0x04
#define SAACC_AUTODLY_OFST 0x08
#define SAACC_DEBTIME_OFST 0x0C
#define SAACC_PNL_OFST	   0x10
#define SAACC_AUX_OFST	   0x14
#define SAACC_INTEN_OFST   0x18
#define SAACC_INTF_OFST    0x1C


//SAACC_SARCTRL

#define SARCTL_SAR_AUTO_CON_ON  (0x01)
#define SARCTL_SAR_MAN_CON_ON  (0x02)

#define SAACC_SAR_BUSY_OFST     21
#define SAACC_SAR_MODE_OFST     18
#define SAACC_SAR_REF_OFST      17
#define SAACC_SAR_TOGEN_OFST    16
#define SAACC_SAR_DIVNUM_OFST   8
#define SAACC_SAR_BACK2INT_OFST 7
#define SAACC_SAR_TPS_OFST      5
#define SAACC_SAR_SARS_OFST     2
#define SAACC_SAR_MANCON_OFST   1
#define SAACC_SAR_AUTOCON_OFST  0

#define SAACC_SAR_BUSY_MASK     0x1
#define SAACC_SAR_MODE_MASK     0x7
#define SAACC_SAR_REF_MASK      0x1
#define SAACC_SAR_TOGEN_MASK    0x1
#define SAACC_SAR_DIVNUM_MASK   0xFF
#define SAACC_SAR_BACK2INT_MASK 0x1
#define SAACC_SAR_TPS_MASK      0x3
#define SAACC_SAR_SARS_MASK     0x7
#define SAACC_SAR_MANCON_MASK   0x1
#define SAACC_SAR_AUTOCON_MASK  0x1

//SAACC_CONDLY
#define SAACC_SAR_CONDLY_OFST   0
#define SAACC_SAR_CONDLY_MASK   0xFF

//SAACC_AUTODLY
#define SAACC_SAR_INTDLY_OFST   0
#define SAACC_SAR_INTDLY_MASK   0x1F
#define SAACC_SAR_X2YDLY_OFST   5
#define SAACC_SAR_X2YDLY_MASK   0x3F

//SAACC_DEBTIME
#define SAACC_SAR_CHKDLY_OFST   0
#define SAACC_SAR_CHKDLY_MASK   0xFFFF
#define SAACC_SAR_DEBDLY_OFST   16
#define SAACC_SAR_DEBDLY_MASK   0xFF

//SAACC_PNL
#define SAACC_SAR_PNLY_OFST   0
#define SAACC_SAR_PNLY_MASK   0xFFFF
#define SAACC_SAR_PNLX_OFST   16
#define SAACC_SAR_PNLX_MASK   0xFFFF

//SAACC_AUX
#define SAACC_SAR_PNLY_OFST   0
#define SAACC_SAR_PNLY_MASK   0xFFFF

//SAACC_INTEN/INTF
#define SAACC_SAR_IENAUX     0x08
#define SAACC_SAR_IENPNL     0x04
#define SAACC_SAR_IENPENUP   0x02
#define SAACC_SAR_IENPENDN   0x01


//------PWCTRL register----------------------------
#define SAR_ENZCD					(1<<0)     	//gain is updated while signal crossing zero
#define SAR_ENVREF					(1<<1)     	//VREF power control
#define SAR_VREFSM					(1<<2)     	//VREF fast setup mode
#define SAR_ENMICBIAS				(1<<3)     	//Microphone bias-voltage output power control
#define SAR_ENMIC					(1<<4)     	//MIC power control
#define SAR_BOOST					(1<<5)     	//boost amplifier gain control
#define SAR_PGAG_MASK				(0x1F<<6)  	//mute mask

//------ADCCTRL register read only---------------------------
#define SAR_ENADL               	(1<<0)     	//Left channel ADC power control
#define SAR_ENADR               	(1<<1)     	//Right channel ADC power control
#define SAR_ADHP  	             	(1<<2)     	//Audio ADC high pass filter control
#define SAR_ADOVRS_LMT84            (0x0<<3)   	//ADC input limit range 0.84*fullrange
#define SAR_ADOVRS_LMT71            (0x1<<3)   	//ADC input limit range 0.71*fullrange
#define SAR_ADOVRS_LMT60            (0x2<<3)   	//ADC input limit range 0.60*fullrange
#define SAR_ADOVRS_LMT50            (0x3<<3)   	//ADC input limit range 0.50*fullrange
#define SAR_ADOVRS_MASK            	(0x3<<3)    //ADC input limit range 0.50*fullrange
#define SAR_ADLOVP               	(1<<5)  	//when high,it means left ADC input is over ADC'positive input ranges
#define SAR_ADLOVN               	(1<<6)  	//when high,it means left ADC input is over ADC's negative input range
#define SAR_ADROVP               	(1<<7)   	//when high,it means right ADC input is over ADC's positive input range
#define SAR_ADROVN               	(1<<8)		//when high,it means right ADC input is over ADC's negative input range
#define	SAR_VOLCTL					(1<<9)		//Add by Simon

//------LININ register-------------------------------
#define SAR_ENLNIN					(1<<0)		//Line in power control
#define SAR_ADINS					(1<<1)		//Left channel ADC input select(0:mic,1:lineLeft)
#define SAR_LNINLG_MASK				(0x1F<<2)	//Left channel line-in gain
#define SAR_LNINRG_MASK				(0x1F<<7)   //Right channel line-in gain

//------DACCTRL register------------------------
#define SAR_ENDAL					(1<<0)		//audio left channel DAC power control
#define SAR_ENDAR					(1<<1)		//audio right channel DAC power control
#define SAR_FORMAT_IIS				(1<<2)		//format selection 0:normal format,1:IIS format
#define SAR_SEL_FS_32_48			(0x0<<3)	//DAC sample rate select:32~48K
#define SAR_SEL_FS_16_24			(0x1<<3)	//DAC sample rate select:16~24K
#define SAR_SEL_FS_08_12			(0x2<<3)	//DAC sample rate select:08~12K
#define SAR_SEL_FS_MASK				(0x3<<3)	//DAC sample rate select:mask
#define SAR_BPFIR					(1<<5)		//bypass DAC digital filter
#define SAR_HPINS_DAC				(0x0<<6)	//audio driver input 00:DAC
#define SAR_HPINS_MIC				(0x1<<6)	//audio driver input 01:MIC
#define SAR_HPINS_LININ				(0x2<<6)	//audio driver input 10:line-in
#define SAR_HPINS_MASK				(0x3<<6)	//audio driver input mask
#define	SAR_DITHER					(0x1<<8)
#define	SAR_EQ_GND					(0x2<<9)
#define	SAR_EQ_VREF					(0x1<<9)
#define SAR_DAC_POWER				(1<<11)		//

//------AUTOSLP register-----------------------
#define SAR_AUTOSLEEP_S				(1<<0)		//select auto sleep function input source 0:IIS input,1:digital filter input
#define SAR_AS_RANGE_MASK			(0x3<<1)	//auto sleep variation range,00:0,01:-1~1,11:-2~2
#define SAR_AS_CYCLE_MASK			(0x3<<3)	//auto sleep start-up time control,00:8192*DALRC,01:16384,10:32768,11:65536

//------LINOUT register-----------------------
#define SAR_ENLINOUTL				(1<<0)		//left channel lineout amplifier
#define SAR_ENLINOUTR				(1<<1)		//right channel lineout amplifier
#define SAR_LNLG_MASK				(0x1F<<2)	//left channel lineout volume control
#define SAR_LNRG_MASK				(0x1F<<7)	//right channel lineout volume control

//------HDPHN register------------------------
#define SAR_ENHPL					(1<<0)		//left channel headphone amplifier power control
#define SAR_ENHPR					(1<<1)		//right channel headphone amplifier power control
#define SAR_HLG_MASK				(0x1F<<2)	//left channel headphone volume control
#define SAR_HRG_MASK				(0x1F<<7)	//right channel headphone volume control

//------TADDA register-----------------------
#define SAR_TADDA_NORMAL			(0x0)
#define SAR_TADDA_MIC_BOOST			(0x1)
#define SAR_TADDA_ADC_PGA			(0x2)
#define SAR_TADDA_LININ_VOL			(0x3)
#define SAR_TADDA_ANTI_ALIAS		(0x4)
#define SAR_TADDA_AUTOSLP			(0x8)
#define SAR_TADDA_DIGFLTR			(0x9)
#define SAR_TADDA_DAC_SCF			(0xA)
#define SAR_TADDA_LCH_SCF			(0xB)
#define SAR_TADDA_RCH_SCF			(0xC)
#define SAR_TADDA_LCH_LPFLTR		(0xD)
#define SAR_TADDA_RCH_LPFLTR		(0xE)
