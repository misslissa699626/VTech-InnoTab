#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/regs-scu.h>
#include <mach/regs-saradc.h>
#include <mach/audio/audio_util.h>

#define AUDIO_PLL_ENABLE  0x01
#define AUDIO_PLL_DISABLE 0x00

#define AUDIO_APLL_SEL_48K  0x00
#define AUDIO_APLL_SEL_44K  0x02

#define AUDIO_I2S_SCLK_FROM_I2S   0x00
#define AUDIO_I2S_SCLK_FROM_APLL  0x04

#define AUDIO_APLL_AS_MASK  (0x3F << 4)
#define AUDIO_APLL_AS_OFST  4

#define AUDIO_CODEC_SEL_INTERAL 0x00
#define AUDIO_CODEC_SEL_EXTERAL (1<<10)
#define AUDIO_APLL_RESET  (1<<11)

#define AUDIO_DARATIO_DIV_MASK 0x7
#define AUDIO_DARATIO_DIV_OFST 16
#define AUDIO_DA_STOP (1<<19)

#define AUDIO_ADRATIO_DIV_MASK 0x7
#define AUDIO_ADRATIO_DIV_OFST 24
#define AUDIO_AD_STOP (1<<27)

/* Local Function */
static int scu_set_freqclk(unsigned int a_AudSampleRate, int play)
{
    unsigned int reg;
	int sample_flag = 0;
	SCUA_APLL_CFG  = AUDIO_PLL_ENABLE | AUDIO_I2S_SCLK_FROM_APLL;

	SCUA_APLL_CFG |= AUDIO_CODEC_SEL_EXTERAL;
	
#if 1
	if ((264600 % a_AudSampleRate) == 0)
	{
		/* 11025, 14700, 22050, 29400, 44100, 88200, 132300 Hz*/
		SCUA_APLL_CFG |= AUDIO_APLL_SEL_44K;
		sample_flag = 264600 / a_AudSampleRate - 1;
	}
	else if ((288000 % a_AudSampleRate) == 0)
	{
		/* 8000, 12000, 16000, 24000, 32000, 48000, 96000, 144000 Hz*/
		SCUA_APLL_CFG &= (~AUDIO_APLL_SEL_44K);
		sample_flag = 288000 / a_AudSampleRate - 1;
	}
	else
	{
		// not support
		return -1;
	}

	if (sample_flag <= 0 || sample_flag > 255)
	{
		// not support
		return -1;
	}
#else
	if (a_AudSampleRate >= 96000)
		a_AudSampleRate = 96000;
	else if (a_AudSampleRate >=88200 )
		a_AudSampleRate =88200 ;
	else if (a_AudSampleRate >=48000 )
		a_AudSampleRate =48000 ;
	else if (a_AudSampleRate >= 44100)
		a_AudSampleRate = 44100;
	else if (a_AudSampleRate >= 32000)
		a_AudSampleRate = 32000;
	else if (a_AudSampleRate >= 29400)
		a_AudSampleRate = 29400;
	else if (a_AudSampleRate >= 24000)
		a_AudSampleRate = 24000;
	else if (a_AudSampleRate >= 22050)
		a_AudSampleRate = 22050;
	else if (a_AudSampleRate >= 16000)
		a_AudSampleRate = 16000;
	else if (a_AudSampleRate >= 14700)
		a_AudSampleRate = 14700;
	else if (a_AudSampleRate >= 12000)
		a_AudSampleRate = 12000;
	else if (a_AudSampleRate >= 11025)
		a_AudSampleRate = 11025;
	else
		a_AudSampleRate = 8000;


	/* Select the clock divisor */
	switch (a_AudSampleRate) {
	case 11025:
	case 14700:
	case 22050:
	case 29400:
	case 44100:
	case 88200:
		SCUA_APLL_CFG |= AUDIO_APLL_SEL_44K;
		if(a_AudSampleRate == 88200)
		{
			sample_flag = 2;  // div 3
		}
		else if(a_AudSampleRate == 44100)
		{
			sample_flag = 5;  // div 6
		}
		else if(a_AudSampleRate == 29400)
		{
			sample_flag = 8;  // div 9
		}
		else if(a_AudSampleRate == 22050)
		{
			sample_flag = 11;  // div 12
		}
		else if(a_AudSampleRate == 14700)
		{
			sample_flag = 17;  // div 18
		}
		else if(a_AudSampleRate == 11025)
		{
			sample_flag = 23;  // div 24
		}
		break;
	case 8000:
	case 12000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 96000:
		SCUA_APLL_CFG &= (~AUDIO_APLL_SEL_44K);
		if(a_AudSampleRate == 96000)
		{
			sample_flag = 2;  	// div 3
		}
		else if(a_AudSampleRate == 48000)
		{
			sample_flag = 5;  	// div 6
		}
		else if(a_AudSampleRate == 32000)
		{
			sample_flag = 8;	// div 9
		}
		else if(a_AudSampleRate == 24000)
		{
			sample_flag = 11;	// div 12
		}
		else if(a_AudSampleRate == 16000)
		{
			sample_flag = 17;	// div 18
		}
		else if(a_AudSampleRate == 12000)
		{
			sample_flag = 23;	// div 24
		}
		else if(a_AudSampleRate == 8000)
		{
			sample_flag = 35;	// div 36 
		}
		break;
	}
#endif


	reg = SCUA_APLL_CFG;
	reg = reg & (~(AUDIO_APLL_AS_MASK));
	reg = reg | 0x120; //divider counter number must set 10010 B

	reg = reg & (~AUDIO_CODEC_SEL_EXTERAL); //internal codec (SAR_ADC)
	reg = reg | AUDIO_APLL_RESET;   //reset pin


    if(!play)
    {
		reg = ((reg&(~0xff0000))|(sample_flag<<16));
    }
	else
	{
		reg = (reg&(~0xff000000))|(sample_flag<<24);
	}
	SCUA_APLL_CFG = reg;

	reg = reg&(~AUDIO_APLL_RESET);  //reset pin pull down

	SCUA_APLL_CFG = reg;
	reg = SCUA_CODEC_CFG;
	SCUA_CODEC_CFG = reg&(~0x200);  //I2S config	  this is necessary

	return 0;
}

static unsigned int sar_set_samplerate(unsigned int a_AudSmpleRate)
{
	unsigned int reg = 0;
	int ret = 0;
	
//	reg = SAACC_DACCTRL;
	reg = SAACC_DACCTRL;
	switch(a_AudSmpleRate)
	{
		case 96000:
		case 88200:
		case 48000:
		case 44100:
		case 32000:
			reg = (reg&(~SAR_SEL_FS_MASK))|SAR_SEL_FS_32_48;
			break;

		case 24000:
		case 22050:
		case 16000:
			reg = (reg&(~SAR_SEL_FS_MASK))|SAR_SEL_FS_16_24;
			break;

		case 12000:
		case 11025:
		case 8000:
			reg = (reg&(~SAR_SEL_FS_MASK))|SAR_SEL_FS_08_12;
			break;

		default:
			reg = (reg&(~SAR_SEL_FS_MASK))|SAR_SEL_FS_32_48;
			ret = -1;
			break;
	}
	if (ret != -1)
	{
		SAACC_DACCTRL = reg;
	}
	return ret;
}

static void sar_vref_ctrl(int isVrefOn)
{
   	unsigned int reg = 0;

	if(isVrefOn)
	{
		reg = SAACC_PWCTRL;
		SAACC_PWCTRL = reg | SAR_ENVREF | SAR_ENZCD;

		do
		{

		}while(!(SAACC_PWCTRL && SAR_ENVREF));
	}
	else
	{
		reg = SAACC_PWCTRL;
		SAACC_PWCTRL = ((reg&(~SAR_ENVREF))&(~SAR_ENZCD));
	}
}

/* extern function */
void audio_Power_ctrl(int On)
{
   	unsigned int reg = 0;
       reg = SAACC_TADDA;   //??
       reg |= 0x20;
       SAACC_TADDA = reg;
   sar_vref_ctrl(On);
}

unsigned int audio_Adins_get(void)
{
    unsigned int reg;

	reg = SAACC_LININ;
	reg = (reg & SAR_ADINS) >> 1;

	return reg;
}
EXPORT_SYMBOL(audio_Adins_get);

unsigned int audio_HPINS_get(void)
{
	unsigned int reg;

	reg = SAACC_DACCTRL;
	reg = (reg & SAR_HPINS_MASK) >> 6;

	return reg;
}
EXPORT_SYMBOL(audio_HPINS_get);

void audio_HPINS_set(unsigned int hpins)
{
	unsigned int reg;

	reg = SAACC_DACCTRL;
	reg &= ~SAR_HPINS_MASK;
	reg |= hpins << 6;
	SAACC_DACCTRL = reg;
}
EXPORT_SYMBOL(audio_HPINS_set);

/**********************************************************
*  WAVE Section
***********************************************************/

void audio_WAVE_ctrl(unsigned int enable)
{
    unsigned int reg;

	if(enable == 1)
	{
	   reg = SAACC_DACCTRL & ~(0x700);   //prevent bit 8~10 as 1 , why it ?? i don't konw
	   reg = (reg & (~SAR_HPINS_MASK)) |SAR_HPINS_DAC;   // force to DAC
     	reg = reg | SAR_ENDAL | SAR_ENDAR|SAR_DAC_POWER;
	   SAACC_DACCTRL = reg;
	}
	else
	{
	   reg = SAACC_DACCTRL;
     	reg = reg & (~SAR_DAC_POWER);
	   SAACC_DACCTRL = reg;
	}
}
EXPORT_SYMBOL(audio_WAVE_ctrl);

void audio_WAVE_volset(long l_vol,long r_vol)
{
	unsigned char vol_l, vol_r;
	unsigned int vol_ctrl;
	vol_l = 0x1f - (l_vol & 0x1f);
	vol_r = 0x1f - (r_vol & 0x1f);
	vol_ctrl = SAACC_HDPHN;
    vol_ctrl = (vol_ctrl & (~SAR_HLG_MASK)) | (vol_l<<2);
    vol_ctrl = (vol_ctrl & (~SAR_HRG_MASK)) | (vol_r<<7);
	vol_ctrl |= (SAR_ENHPL | SAR_ENHPR);
    SAACC_HDPHN = vol_ctrl;
}
EXPORT_SYMBOL(audio_WAVE_volset);

void audio_WAVE_volget(long *l_vol,long *r_vol)
{
	*l_vol = 0x1f - ((SAACC_HDPHN & SAR_HLG_MASK) >> 2);
	*r_vol = 0x1f - ((SAACC_HDPHN & SAR_HRG_MASK) >> 7);
}
EXPORT_SYMBOL(audio_WAVE_volget);

long audio_WAVE_volmax(void)
{
   return 0x1f;
}

void audio_WAVE_muteget(long *l_mute,long *r_mute)
{
	unsigned int hdphn_ctrl;
	hdphn_ctrl = SAACC_HDPHN;
	*l_mute = !!(hdphn_ctrl & SAR_ENHPL);
	*r_mute = !!(hdphn_ctrl & SAR_ENHPR);
}
EXPORT_SYMBOL(audio_WAVE_muteget);

void audio_WAVE_muteset(long l_mute,long r_mute)
{
	unsigned int hdphn_ctrl;

	hdphn_ctrl = SAACC_HDPHN;
	hdphn_ctrl |= (SAR_ENHPL | SAR_ENHPR);
	if (!l_mute)
		hdphn_ctrl &= (~SAR_ENHPL);
	if (!r_mute)
		hdphn_ctrl &= (~SAR_ENHPR);
	SAACC_HDPHN = hdphn_ctrl;
}
EXPORT_SYMBOL(audio_WAVE_muteset);

/**********************************************************
*  LINEOUT Section
***********************************************************/

void audio_LINEOUT_volset(long l_vol,long r_vol)
{
	unsigned char vol_l, vol_r;
	unsigned int vol_ctrl;
	vol_l = 0x1f - (l_vol & 0x1f);
	vol_r = 0x1f - (r_vol & 0x1f);
	vol_ctrl = SAACC_LINOUT;
    vol_ctrl = (vol_ctrl & (~SAR_LNLG_MASK)) | (vol_l<<2);
    vol_ctrl = (vol_ctrl & (~SAR_LNRG_MASK)) | (vol_r<<7);
	vol_ctrl |= (SAR_ENLINOUTL | SAR_ENLINOUTR);
    SAACC_LINOUT = vol_ctrl;
}
EXPORT_SYMBOL(audio_LINEOUT_volset);

void audio_LINEOUT_volget(long *l_vol,long *r_vol)
{
	*l_vol = 0x1f - ((SAACC_LINOUT & SAR_LNLG_MASK) >> 2);
	*r_vol = 0x1f - ((SAACC_LINOUT & SAR_LNRG_MASK) >> 7);
}
EXPORT_SYMBOL(audio_LINEOUT_volget);

long audio_LINEOUT_volmax(void)
{
   return 0x1f;
}

void audio_LINEOUT_muteget(long *l_mute,long *r_mute)
{
	unsigned int linout_ctrl;
	linout_ctrl = SAACC_LINOUT;
	*l_mute = !!(linout_ctrl & SAR_ENLINOUTL);
	*r_mute = !!(linout_ctrl & SAR_ENLINOUTR);
}
EXPORT_SYMBOL(audio_LINEOUT_muteget);

void audio_LINEOUT_muteset(long l_mute,long r_mute)
{
	unsigned int linout_ctrl;

	linout_ctrl = SAACC_LINOUT;
	linout_ctrl |= (SAR_ENLINOUTL | SAR_ENLINOUTR);
	if (!l_mute)
		linout_ctrl &= (~SAR_ENLINOUTL);
	if (!r_mute)
		linout_ctrl &= (~SAR_ENLINOUTR);
	SAACC_LINOUT = linout_ctrl;
}
EXPORT_SYMBOL(audio_LINEOUT_muteset);

/**********************************************************
*  LINEIN Section
***********************************************************/

void audio_LINEIN_ctrl(unsigned int enable)
{
    unsigned int 	reg;

	if(enable == 1)
	{
      reg = SAACC_LININ;
	  reg = reg | SAR_ENLNIN | SAR_ADINS;
      SAACC_LININ = reg;

      reg = SAACC_ADCCTRL;
      SAACC_ADCCTRL = reg | SAR_ADHP | SAR_ENADR | SAR_ENADL;
	}
	else
	{
		reg = SAACC_LININ;
		reg = ((reg&(~SAR_ENLNIN))&(~SAR_ADINS));
		SAACC_LININ = reg;

		reg = SAACC_ADCCTRL;
		SAACC_ADCCTRL = (((reg&(~SAR_ADHP))&(~SAR_ENADR))&(~SAR_ENADL));
	}
}
EXPORT_SYMBOL(audio_LINEIN_ctrl);

void audio_LINEIN_volset(long l_vol,long r_vol)
{
	unsigned char vol_l, vol_r;
	unsigned int vol_ctrl;

	vol_l = 0x1f - l_vol;
	vol_r = 0x1f - r_vol;

 	vol_ctrl = SAACC_LININ;
    vol_ctrl = (vol_ctrl&(~SAR_LNINLG_MASK))|((vol_l&(0x1f))<<2);
    vol_ctrl = (vol_ctrl&(~SAR_LNINRG_MASK))|((vol_r&(0x1f))<<7);
	SAACC_LININ = vol_ctrl;
}
EXPORT_SYMBOL(audio_LINEIN_volset);

void audio_LINEIN_volget(long *l_vol,long *r_vol)
{
	unsigned char vol_l, vol_r;

    vol_l = ((SAACC_LININ & SAR_LNINLG_MASK) >> 2);
	vol_r = ((SAACC_LININ & SAR_LNINRG_MASK) >> 7);
	
	*l_vol = 0x1f - vol_l;
	*r_vol = 0x1f - vol_r;

}
EXPORT_SYMBOL(audio_LINEIN_volget);

long audio_LINEIN_volmax(void)
{
   return 0x1f;
}

/**********************************************************
*  MIC Section
***********************************************************/

void audio_MIC_ctrl(unsigned int enable)
{
    unsigned int 	reg;

	if(enable == 1)
	{
		reg = SAACC_PWCTRL;
		SAACC_PWCTRL = reg  | SAR_ENMIC | SAR_BOOST  | SAR_ENMICBIAS;
		SAACC_LININ &= 0xffc;  //clean all line-in setting for MIC
		reg = SAACC_ADCCTRL;
		SAACC_ADCCTRL= ((reg|SAR_ENADL)&(~(SAR_ENADR|SAR_ADHP)));
	}
	else
	{
		reg = SAACC_PWCTRL;
		SAACC_PWCTRL = (((reg&(~SAR_ENMICBIAS))&(~SAR_ENMIC))&(~SAR_BOOST));

		reg = SAACC_LININ;
		SAACC_LININ = reg | SAR_ADINS;

		reg = SAACC_ADCCTRL;
		SAACC_ADCCTRL = (((reg&(~SAR_ADHP))&(~SAR_ENADR))&(~SAR_ENADL));
	}
}
EXPORT_SYMBOL(audio_MIC_ctrl);

void audio_MIC_volset(long l_vol)
{
	unsigned char vol;
	unsigned int vol_ctrl;
	vol = 0x1f - l_vol;

	vol_ctrl = SAACC_PWCTRL;
    vol_ctrl = (vol_ctrl&(~SAR_PGAG_MASK))|((vol&(0x1f))<<6);
	SAACC_PWCTRL = vol_ctrl;
}
EXPORT_SYMBOL(audio_MIC_volset);

void audio_MIC_volget(long *l_vol)
{
	unsigned char vol_l;

	vol_l = ((SAACC_PWCTRL & SAR_PGAG_MASK) >> 6);

	*l_vol = 0x1f - vol_l;
}
EXPORT_SYMBOL(audio_MIC_volget);

long audio_MIC_volmax(void)
{
   return 0x1f;
}


int audio_FREQ_set(unsigned int a_AudSampleRate, int play)
{
	int ret;
    ret = scu_set_freqclk(a_AudSampleRate,play);
	if (ret < 0)
	{
		return ret;
	}
	if(play)
	{
		ret = sar_set_samplerate(a_AudSampleRate);
	}
	return ret;
}

/**********************************************************
*  Headphone Section
***********************************************************/

void audio_HDPHN_volset(long l_vol,long r_vol)
{
	unsigned char vol_l, vol_r;
	unsigned int vol_ctrl;

	vol_l = 0x1f - l_vol;
	vol_r = 0x1f - r_vol;

 	vol_ctrl = SAACC_HDPHN;
    vol_ctrl = (vol_ctrl&(~SAR_HLG_MASK))|((vol_l&(0x1f))<<2);
    vol_ctrl = (vol_ctrl&(~SAR_HRG_MASK))|((vol_r&(0x1f))<<7);
	SAACC_HDPHN = vol_ctrl;
}
EXPORT_SYMBOL(audio_HDPHN_volset);

void audio_HDPHN_volget(long *l_vol,long *r_vol)
{
	unsigned char vol_l, vol_r;

    vol_l = ((SAACC_HDPHN & SAR_HLG_MASK) >> 2);
	vol_r = ((SAACC_HDPHN & SAR_HRG_MASK) >> 7);

	*l_vol = 0x1f - vol_l;
	*r_vol = 0x1f - vol_r;
}
EXPORT_SYMBOL(audio_HDPHN_volget);
