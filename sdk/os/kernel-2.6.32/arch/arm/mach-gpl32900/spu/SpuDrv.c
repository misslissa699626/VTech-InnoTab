#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>

#include "SpuDrv.h"
#include "SpuReg.h"

//#define DEBUG_PRINT		printk
#define DEBUG_PRINT(...)

#define ERROR_PRINT		printk
//#define ERROR_PRINT(...)

#define OOPS	{*(int*)0 = 0;}


#if 0
#define R_CHA_CTRL *(volatile unsigned*)(0xC00D0000)
#define R_CHB_CTRL *(volatile unsigned*)(0xC00D0020)
#define R_CHA_FIFO *(volatile unsigned*)(0xC00D0008)
#define R_CHB_FIFO *(volatile unsigned*)(0xC00D0028)
#define R_IISEN *(volatile unsigned*)(0xC00D003C)
#endif

// initial DAC volume
#define D_SPU_DAC_VOLUME 0x7F

// definition of envelope clock
#define D_SPU_ENV_CLK 0x3333

// volume scale of each hardware channel
#define D_SPU_VOLUMEUME_SEL_0 0 // 1/32
#define D_SPU_VOLUMEUME_SEL_1 (0x1<<6) // 1/8
#define D_SPU_VOLUMEUME_SEL_2 (0x2<<6) // 1/2
#define D_SPU_VOLUMEUME_SEL_3 (0x3<<6) // 1

// definition for default ramp down settings
#define D_RAMPDOWN_OFFSET 0x4000
#define D_RAMPDOWN_CLOCK 0x0002

// definition of loop mode
#define D_LOOP_MODE0 0x1000 // no loop
#define D_LOOP_MODE1 0x2000 // pcm->pcm, adpcm->pcm, adpcm36->adpcm36
#define D_LOOP_MODE2 0x3000 // adpcm36->pcm


WaveInfo gSpuChannelWaveInfo[D_SPU_TOTAL_CHANNEL];

unsigned short gSpuChannelSpeed[D_SPU_TOTAL_CHANNEL] = {0};
unsigned char gSpuChannelDataReady[D_SPU_TOTAL_CHANNEL] = {0};
SpuBeatCntHandler gSpuBeatCntHandler = 0;
SpuFiqHandler gSpuFiqHandler[D_SPU_TOTAL_CHANNEL] = {0};

//
// helper functions
//

static const signed char gBitLut[] = {
	-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,	5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,	6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,	7, 7, 7, 7, 7, 7, 7, 7
};

#define __lsb1(i) ((i) & -(int)(i))
#define __lsb2(x) (((x) & 0xffff0000)? (((x) & 0xff000000)? 24 : 16) : (((x) & 0x0000ff00)? 8 : 0))
#define lsb(i) ((int)gBitLut[__lsb1(i) >> __lsb2(__lsb1(i))] + __lsb2(__lsb1(i)))

static unsigned int PA_TO_KERNEL_VA(unsigned int pa)
{
	if (pa) {
		unsigned int ka = (unsigned int)gp_chunk_va(pa);	/* phy_addr to kernel_addr */
		if (ka) {
			return ka;
		}
		ERROR_PRINT(KERN_WARNING "%s:%s(0x%08X): failed!\n", __FILE__, __FUNCTION__, pa);
		OOPS
	}
	return 0;
}

static unsigned int KERNEL_VA_TO_PA(unsigned int kva)
{
	if (kva) {
		unsigned int pa = gp_chunk_pa((void*)kva);	/* kernel_addr to phy_addr */
		if (pa) {
			return pa;
		}
		ERROR_PRINT(KERN_WARNING "%s:%s(0x%08X): failed!\n", __FILE__, __FUNCTION__, kva);
		OOPS
	}
	return 0;
}

void Spu_enableDac(int enable) {
#if 0
	if (enable) {
		R_CHA_CTRL = 0;
		R_CHB_CTRL = 0;  
	
		R_CHA_CTRL |= 0x8000; /* clear FIFO empty flag */
		R_CHB_CTRL |= 0x8000;
	
		R_CHA_CTRL &= ~0x4000; /* disabe FIFO empty interrupt */
		R_CHB_CTRL &= ~0x4000; /* disabe FIFO empty interrupt */ 
	
		R_CHA_CTRL |= 0x3800; // (DAC_CH_EN|DAC_SIGNED|DAC_VREFEN); /* enable CHA and use signed data type */
		R_CHB_CTRL |= 0x3000; // (DAC_CH_EN|DAC_CHB_SSF);
	
		R_CHA_FIFO |= 0x0100; /* reset FIFO */
		R_CHB_FIFO |= 0x0100; 
	
		R_IISEN |= 0x0001; /* enable IIS */
	} else {
		R_IISEN &= ~0x0001; /* disable IIS */
	}
#endif	
}

unsigned Spu_getWaveAddr(unsigned channel_id) {
	unsigned addr = 0;
	unsigned reg = 0;
	
	reg = Spu_getChannelControlReg(channel_id, D_SPU_CH_WAVEADDR);
	addr |= (reg << 1);
	
	reg = Spu_getChannelControlReg(channel_id, D_SPU_CH_MODE);
	reg &= 0x003f;
	addr |= (reg << 17);

	reg = Spu_getChannelControlReg(channel_id, D_SPU_CH_WL_HADDR);
	reg &= 0x003f;
	addr |= (reg << 23);
	
	//vicsiu
	//#error TODO: change pa to kernel va
	addr = PA_TO_KERNEL_VA(addr);
	
	return addr;
}

unsigned Spu_getLoopAddr(unsigned channel_id) {
	unsigned addr = 0;
	unsigned reg0 = 0;
	unsigned reg1 = 0;
	unsigned reg2 = 0;
	
	reg0 = Spu_getChannelControlReg(channel_id, D_SPU_CH_LOOPADDR);
	reg1 = Spu_getChannelControlReg(channel_id, D_SPU_CH_MODE);
	reg2 = Spu_getChannelControlReg(channel_id, D_SPU_CH_WL_HADDR);

	addr |= (reg0 << 1);
	
	reg1 &= 0x0fc0;
	addr |= (reg1 << 11);

	reg2 &= 0x0fc0;
	addr |= (reg2 << 17);

	//vicsiu
	//#error TODO: change pa to kernel va
	addr = PA_TO_KERNEL_VA(addr);
	
	return addr;
}

unsigned Spu_getEnvAddr(unsigned channel_id) {
	unsigned addr = 0;
	unsigned reg0 = 0;
	unsigned reg1 = 0;
	unsigned reg2 = 0;
	
	reg0 = Spu_getChannelControlReg(channel_id, D_SPU_CH_ENVELOP_OFFSET);
	reg1 = Spu_getChannelControlReg(channel_id, D_SPU_CH_ENVELOP_SEGMENT);
	reg2 = Spu_getChannelControlReg(channel_id, D_SPU_CH_E_HADDR);

	addr |= (reg0 << 1);
	
	reg1 &= 0x003f;
	addr |= (reg1 << 17);

	reg2 &= 0x003f;
	addr |= (reg2 << 23);

	//vicsiu
	//#error TODO: change pa to kernel va
	addr = PA_TO_KERNEL_VA(addr);
	
	return addr;
}

unsigned Spu_getWaveCurrentPlayTime(unsigned channel_id) {
	int current_addr = (int)Spu_getWaveAddr(channel_id);
	int base_addr = (int)gSpuChannelWaveInfo[channel_id].mWaveAddr;
	int phase = gSpuChannelWaveInfo[channel_id].mPhase;
	int diff = current_addr - base_addr;
	int bps = 8;

	if (diff < 0) return 0;

	switch (gSpuChannelWaveInfo[channel_id].mFormat) {
	case D_FORMAT_ATTACK_PCM8_LOOP_PCM8:
		bps = 8;
		break;
	case D_FORMAT_ATTACK_PCM16_LOOP_PCM16:
		bps = 16;
		break;
	case D_FORMAT_ATTACK_ADPCM36_LOOP_ADPCM36:
		bps = 4;
		break;
	case D_FORMAT_ATTACK_ADPCM_LOOP_PCM8:
	case D_FORMAT_ATTACK_ADPCM_LOOP_PCM16:
	case D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8:
	case D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16:
		bps = 4; // approx as 4bits per sample
		break;
	default:
		return 0; // undefined format
	}

	return (unsigned)(diff * 8 * 1000 / bps / PHASE_2_SAMPLERATE(phase));
}

void Spu_enableEnvAutoMode(int channel_id, int enable) {
	if (enable) {
		if( channel_id < 16 ) {
			unsigned temp = Spu_getControlRegL(D_SPU_CH_ENV_MODE_SEL);
			temp &= ~Spu_getChannelMaskWord(channel_id);
			Spu_setControlRegL(D_SPU_CH_ENV_MODE_SEL, temp);
		} else {
			unsigned temp = Spu_getControlRegH(D_SPU_CH_ENV_MODE_SEL);
			temp &= ~(Spu_getChannelMaskWord(channel_id) >> 16);
			Spu_setControlRegH(D_SPU_CH_ENV_MODE_SEL, temp);
		}
	} else {
		if( channel_id < 16 ) {
			unsigned temp = Spu_getControlRegL(D_SPU_CH_ENV_MODE_SEL);
			temp |= Spu_getChannelMaskWord(channel_id);
			Spu_setControlRegL(D_SPU_CH_ENV_MODE_SEL, temp);
		} else {
			unsigned temp = Spu_getControlRegH(D_SPU_CH_ENV_MODE_SEL);
			temp |= (Spu_getChannelMaskWord(channel_id) >> 16);
			Spu_setControlRegH(D_SPU_CH_ENV_MODE_SEL, temp);
		}
	}
}

void Spu_setAddresses(int channel_id,
	unsigned wave_addr, unsigned loop_addr, unsigned env_addr,
	int adpcm_mode, int pcm16_mode, int loop_mode) {
	
	unsigned channel_mode = 0;
	unsigned extend_addr = 0;

DEBUG_PRINT(KERN_WARNING "TRACE %s(%d, 0x%08X, 0x%08X, 0x%08X...)\n", __FUNCTION__, channel_id, wave_addr, loop_addr, env_addr);

	//vicsiu
	//#error TODO: change kernel va to pa for wave_addr, loop_addr and env_addr
	wave_addr = KERNEL_VA_TO_PA(wave_addr);
	loop_addr = KERNEL_VA_TO_PA(loop_addr);
	env_addr = KERNEL_VA_TO_PA(env_addr);

DEBUG_PRINT(KERN_WARNING "TRACE %s(): wave_addr=0x%08X, loop_addr=0x%08X, env_addr=0x%08X\n", __FUNCTION__, wave_addr, loop_addr, env_addr);
	
	extend_addr = 0;

	extend_addr |= (wave_addr >> 23) & 0x003f;
	extend_addr |= (loop_addr >> 17) & 0x0fc0;
	Spu_setChannelControlReg(channel_id, D_SPU_CH_WL_HADDR, extend_addr);

	extend_addr = (env_addr >> 23) & 0x003f;
	Spu_setChannelControlReg(channel_id, D_SPU_CH_E_HADDR, extend_addr);

	channel_mode = 0;
	
	if (adpcm_mode) {
		channel_mode |= 0x8000;
	}
	
	if (pcm16_mode) {
		channel_mode |= 0x4000;
	}
	
	channel_mode |= loop_mode;
	
	channel_mode |= (wave_addr >> 17) & 0x003f;
	channel_mode |= (loop_addr >> 11) & 0x0fc0;
	
	Spu_setChannelControlReg(channel_id, D_SPU_CH_MODE, channel_mode);
	
	Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOP_SEGMENT, (env_addr >> 17) & 0x3f);
	
	Spu_setChannelControlReg(channel_id, D_SPU_CH_WAVEADDR, (wave_addr >> 1) & 0xffff);
	Spu_setChannelControlReg(channel_id, D_SPU_CH_LOOPADDR, (loop_addr >> 1) & 0xffff);
	Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOP_OFFSET, (env_addr >> 1) & 0xffff);
}

void Spu_setupEnvelope(int channel_id,
	unsigned short env0, unsigned short env1, unsigned short env_data,
	unsigned ramp_down_offset, unsigned ramp_down_clk) {

	Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOP0, env0);
	Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOP1, env1);

	Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOP_DATA, env_data);

	Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOPLoop, ramp_down_offset);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_RAMP_DOWN_CLK, ramp_down_clk);
}

void Spu_setLoopAddress(int channel_id, unsigned char* addr) {
	unsigned channel_mode = 0;
	unsigned wl_haddr = 0;
	unsigned addr0, addr1, addr2;

	//vicsiu
	//#error TODO: change kernel va to pa
	addr = (unsigned char*)KERNEL_VA_TO_PA((unsigned int)addr);

	addr0 = ((unsigned)(addr) >> 1) & 0xffff;
	addr1 = ((unsigned)(addr) >> 11) & 0x0fc0;
	addr2 = ((unsigned)(addr) >> 17) & 0x0fc0;

	channel_mode = Spu_getChannelControlReg(channel_id, D_SPU_CH_MODE);
	wl_haddr = Spu_getChannelControlReg(channel_id, D_SPU_CH_WL_HADDR);

	channel_mode &= 0xf03f;
	channel_mode |= addr1;
	wl_haddr &= 0xf03f;
	wl_haddr |= addr2;

	Spu_setChannelControlReg(channel_id, D_SPU_CH_LOOPADDR, addr0);
	Spu_setChannelControlReg(channel_id, D_SPU_CH_MODE, channel_mode);
	Spu_setChannelControlReg(channel_id, D_SPU_CH_WL_HADDR, wl_haddr);
}


//
// driver functions
//

void Spu_init() {
	unsigned i = 0;
	
	Spu_enableDac(1);

	Spu_syncDacOutput();

	for(i = 0; i < D_SPU_TOTAL_CHANNEL; i++)
	{
		Spu_setChannelControlReg(i, D_SPU_CH_WAVEADDR, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_MODE, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_LOOPADDR, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_PAN, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP0, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP_DATA, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP1, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP_SEGMENT, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP_OFFSET, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_WAVEDATA0, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOPLoop, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_WAVEDATA, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_ADPCMSEL, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_WL_HADDR, 0);
		Spu_setChannelControlReg(i, D_SPU_CH_E_HADDR, 0);
	}

	for(i = 0; i < D_SPU_TOTAL_CHANNEL; i++)
	{
		Spu_setChannelPhaseReg(i, D_SPU_CH_HPHASE, 0);
		Spu_setChannelPhaseReg(i, D_SPU_CH_HPHASE_ACCUMULATOR, 0);
		Spu_setChannelPhaseReg(i, D_SPU_CH_HTARGETPHASE, 0);
		Spu_setChannelPhaseReg(i, D_SPU_CH_LPHASE, 0);
		Spu_setChannelPhaseReg(i, D_SPU_CH_LPHASE_ACCUMULATOR, 0);
		Spu_setChannelPhaseReg(i, D_SPU_CH_LTARGETPHASE, 0);
		Spu_setChannelPhaseReg(i, D_SPU_CH_PHASE_CHANGE_CONTROL, 0);
	}

	Spu_setControlRegLH(D_SPU_CH_ENABLE, 0);
	Spu_setControlRegLH(D_SPU_CH_FIQ_ENABLE, 0);
	Spu_setControlRegLH(D_SPU_CH_REPEAT_ENABLE, 0);
	Spu_setControlRegLH(D_SPU_CH_ENV_MODE_SEL, 0);
	Spu_setControlRegLH(D_SPU_CH_STOPSTATUS, 0xFFFFFFFF);
	
	Spu_setControlRegL(D_SPU_VOLUME, D_SPU_DAC_VOLUME);
	Spu_setControlRegL(D_SPU_BEATBASECNT, 0);
	Spu_setControlRegL(D_SPU_BEATCNT, 0);
	Spu_setControlRegL(D_SPU_ENV_CLOCK0, D_SPU_ENV_CLK);
	Spu_setControlRegL(D_SPU_ENV_CLOCK1, D_SPU_ENV_CLK);
	Spu_setControlRegL(D_SPU_ENV_CLOCK2, D_SPU_ENV_CLK);
	Spu_setControlRegL(D_SPU_ENV_CLOCK3, D_SPU_ENV_CLK);
	Spu_setControlRegL(D_SPU_ENV_CLOCK4, D_SPU_ENV_CLK);
	Spu_setControlRegL(D_SPU_ENV_CLOCK5, D_SPU_ENV_CLK);
	Spu_setControlRegL(D_SPU_ENV_CLOCK6, D_SPU_ENV_CLK);			
	Spu_setControlRegL(D_SPU_ENV_CLOCK7, D_SPU_ENV_CLK);			
	Spu_setControlRegL(D_SPU_CONTROL, 0x0008 | D_SPU_VOLUMEUME_SEL_2);
	Spu_setControlRegL(D_SPU_THRESHOLD, 0x8207);
	
	Spu_setMasterVolume(0x7f);
	
	for(i = 0; i < D_SPU_TOTAL_CHANNEL; i++) {
		Spu_enableWaveLoop(i, 0);
		Spu_setWaveVolume(i, 0x7f);
		Spu_setWavePan(i, 0x40);
	}
	
	for(i = 0; i < D_SPU_TOTAL_CHANNEL; i++) {
		memset((unsigned char*)&gSpuChannelWaveInfo[i], 0x0, sizeof(WaveInfo));
		gSpuChannelWaveInfo[i].mFormat = D_FORMAT_ATTACK_PCM8_LOOP_PCM8;
		gSpuChannelSpeed[i] = D_SPEED_FACTOR_ONE;
		gSpuChannelDataReady[i] = 0;
	}

	for(i = 0; i < D_SPU_TOTAL_CHANNEL; i++) {
		gSpuFiqHandler[i] = 0;
	}
	
	gSpuBeatCntHandler = 0;
}

void Spu_cleanup() {
	Spu_stopWaves(0, D_SPU_TOTAL_CHANNEL - 1);
	
	Spu_enableDac(0);
}

unsigned Spu_getMasterVolume() {
	return Spu_getControlRegL(D_SPU_VOLUME) & 0x0000007f;
}

void Spu_setMasterVolume(unsigned vol) {
	Spu_setControlRegL(D_SPU_VOLUME, vol & 0x0000007f);	
}

void Spu_setWaveEx(int channel_id, const WaveInfo* info) {
	unsigned char* temp_pcm_data = info->mWaveAddr;
	unsigned is_adpcm = 0;
	unsigned is_adpcm36 = 0;
	unsigned is_pcm16 = 0;
	unsigned loop_mode = 0;

DEBUG_PRINT(KERN_WARNING "TRACE %s(%d, 0x%08X)\n", __FUNCTION__, channel_id, (unsigned int)info);
	gSpuChannelWaveInfo[channel_id] = *info;

	is_adpcm =
		(info->mFormat == D_FORMAT_ATTACK_ADPCM_LOOP_PCM8) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM_LOOP_PCM16) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_ADPCM36);
	is_adpcm36 = 
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_ADPCM36);
	is_pcm16 =
		(info->mFormat == D_FORMAT_ATTACK_PCM16_LOOP_PCM16) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM_LOOP_PCM16) ||
		(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16);

	if (info->mLoopEnable) {
		if ((info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8) ||
			(info->mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16)) {
			loop_mode = D_LOOP_MODE2;
		} else {
			loop_mode = D_LOOP_MODE1;
		}
	} else {
		loop_mode = D_LOOP_MODE0;
	}

	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HPHASE, 0);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HPHASE_ACCUMULATOR, 0);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HTARGETPHASE, 0x0000);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_RAMP_DOWN_CLK, 0);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LPHASE, 0);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LPHASE_ACCUMULATOR, 0);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LTARGETPHASE, 0x0000);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_PHASE_CHANGE_CONTROL, 0);

	Spu_stopWave(channel_id);

	Spu_enableEnvAutoMode(channel_id, info->mEnvAutoModeEnable);
	
	if(is_adpcm) {
		// ADPCM mode setting.
		if (is_adpcm36) {
			// ADPCM36 mode
			Spu_setChannelControlReg(channel_id, D_SPU_CH_WAVEDATA0, 0x8000);
			Spu_setChannelControlReg(channel_id, D_SPU_CH_ADPCMSEL, 0xBE00);
			Spu_setChannelControlReg(channel_id, D_SPU_CH_WAVEDATA, 0x8000);
		} else {
			Spu_setChannelControlReg(channel_id, D_SPU_CH_ADPCMSEL, 0);
			Spu_setChannelControlReg(channel_id, D_SPU_CH_WAVEDATA, *(unsigned short*)temp_pcm_data);
			temp_pcm_data += sizeof(unsigned short);
		}
	} else {
		if (is_pcm16) {
			// 16 bit pcm mode
			Spu_setChannelControlReg(channel_id, D_SPU_CH_ADPCMSEL, 0);
			Spu_setChannelControlReg(channel_id, D_SPU_CH_WAVEDATA, 0x8000);
		} else {
			// 8 bit pcm mode
			Spu_setChannelControlReg(channel_id, D_SPU_CH_ADPCMSEL, 0);
			Spu_setChannelControlReg(channel_id, D_SPU_CH_WAVEDATA, 0x8080);
		}
	}

	Spu_setAddresses(channel_id, (unsigned)temp_pcm_data, (unsigned)info->mLoopAddr, (unsigned)info->mEnvAddr, is_adpcm, is_pcm16, loop_mode);

	//Spu_setupEnvelope(channel_id, info->mEnv0, info->mEnv1, info->mEnvData, info->mRampDownOffset & 0xFE00, info->mRampDownClk & 0xF0000);
	Spu_setupEnvelope(channel_id, info->mEnv0, info->mEnv1, info->mEnvData, info->mRampDownOffset, info->mRampDownClk);

	// set sampling rate
	Spu_setWavePhase(channel_id, info->mPhase);

	gSpuChannelDataReady[channel_id] = 1;
}

void Spu_setWave(int channel_id, unsigned char* pcm_data) {
	WaveInfo info;

	unsigned is_adpcm = 0;
	unsigned is_adpcm36 = 0;
	unsigned is_pcm16 = 0;
	unsigned char* temp_pcm_data = pcm_data;

DEBUG_PRINT(KERN_WARNING "TRACE %s(%d, 0x%08X)\n", __FUNCTION__, channel_id, (unsigned int)pcm_data);

	is_adpcm = ((unsigned short*)(pcm_data))[16] & 0x0040;
	is_adpcm36 = ((unsigned short*)(pcm_data))[16] & 0x0080;
	is_pcm16 = ((unsigned short*)(pcm_data))[16] & 0x0010;
	temp_pcm_data += 0x00000028; // jackson: tone color header size, temporary hard code 40 bytes

	if (is_adpcm) {
		if (is_adpcm36) {
			info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_ADPCM36;
		} else {
			info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM8;
		}
	} else {
		if (is_pcm16) {
			info.mFormat = D_FORMAT_ATTACK_PCM16_LOOP_PCM16;
		} else {
			info.mFormat = D_FORMAT_ATTACK_PCM8_LOOP_PCM8;
		}
	}

	if (is_adpcm && !is_adpcm36) {
		info.mLoopEnable = 0; // jackson: adpcm32 cannot loop by this function
	} else {
		info.mLoopEnable = Spu_isWaveLoopEnable(channel_id);
	}
	info.mEnvAutoModeEnable = 0;
	info.mWaveAddr = temp_pcm_data;
	info.mLoopAddr = temp_pcm_data;
	info.mEnvAddr = 0;
	info.mEnv0 = 0;
	info.mEnv1 = 0;
	info.mEnvData = 0x7f;
	info.mRampDownOffset = D_RAMPDOWN_OFFSET;
	info.mRampDownClk = D_RAMPDOWN_CLOCK;
	info.mPhase = SAMPLERATE_2_PHASE(((unsigned short*)(pcm_data))[8]);

	Spu_setWaveEx(channel_id, &info);
}

void Spu_resetWave(int channel_id) {
	Spu_setWaveEx(channel_id, &gSpuChannelWaveInfo[channel_id]);
}

void Spu_pauseWave(int channel_id) {
	unsigned temp = 0;
	//unsigned envelop_loop = 0;
	//unsigned phase_control = 0;

	if (Spu_isWavePause(channel_id)) return;

	if (channel_id < 16) {
		temp = Spu_getControlRegL(D_SPU_CH_STATUS) & Spu_getChannelMaskWord(channel_id);
	} else {
		temp = Spu_getControlRegH(D_SPU_CH_STATUS) & (Spu_getChannelMaskWord(channel_id) >> 16);
	}

	if (temp) {
		gSpuChannelDataReady[channel_id] = 1;
	} else {
		gSpuChannelDataReady[channel_id] = 0;
	}

	if (channel_id < 16) {
		Spu_setControlRegL(D_SPU_CH_ENABLE, Spu_getControlRegL(D_SPU_CH_ENABLE) & (~Spu_getChannelMaskWord(channel_id)));
	} else {
		Spu_setControlRegH(D_SPU_CH_ENABLE, Spu_getControlRegH(D_SPU_CH_ENABLE) & (~(Spu_getChannelMaskWord(channel_id) >> 16)));
	}
}

void Spu_pauseWaves(unsigned start_idx, unsigned end_idx) {
	unsigned i = 0;
	unsigned channel_maskL = 0;
	unsigned channel_maskH = 0;
	unsigned tempL = 0;
	unsigned tempH = 0;
	
	if (start_idx >= D_SPU_TOTAL_CHANNEL) start_idx = D_SPU_TOTAL_CHANNEL;
	if (end_idx >= D_SPU_TOTAL_CHANNEL) end_idx = D_SPU_TOTAL_CHANNEL - 1;

	Spu_getChannelMaskWordByRange(start_idx, end_idx, &channel_maskL);
	
	channel_maskH = (channel_maskL >> 16) & 0x0000ffff;
	channel_maskL &= 0x0000ffff;

	if (channel_maskL) {
		tempL = Spu_getControlRegL(D_SPU_CH_STATUS) & channel_maskL;
	}
	
	if (channel_maskH) {
		tempH = Spu_getControlRegH(D_SPU_CH_STATUS) & channel_maskH;
	}
	
	for (i = start_idx; i <= end_idx; i++) {
		if (i < 16) {
			if (tempL & Spu_getChannelMaskWord(i)) {
				gSpuChannelDataReady[i] = 1;
			} else {
				gSpuChannelDataReady[i] = 0;
			}
		} else {
			if (tempH & (Spu_getChannelMaskWord(i) >> 16)) {
				gSpuChannelDataReady[i] = 1;
			} else {
				gSpuChannelDataReady[i] = 0;
			}
		}
	}

	if (channel_maskL) {
		Spu_setControlRegL(D_SPU_CH_ENABLE, Spu_getControlRegL(D_SPU_CH_ENABLE) & (~channel_maskL));
	}

	if (channel_maskH) {
		Spu_setControlRegH(D_SPU_CH_ENABLE, Spu_getControlRegH(D_SPU_CH_ENABLE) & (~channel_maskH));
	}
}

void Spu_resumeWave(int channel_id) {
	if (!Spu_isWavePause(channel_id)) return;
	
	if (channel_id < 16) {
		if (gSpuChannelDataReady[channel_id]) { // if the channel is busy before pause
			Spu_setControlRegL(D_SPU_CH_STOPSTATUS, Spu_getChannelMaskWord(channel_id));
		}
		Spu_setControlRegL(D_SPU_CH_ENABLE, Spu_getControlRegL(D_SPU_CH_ENABLE) | Spu_getChannelMaskWord(channel_id));
	} else {
		if (gSpuChannelDataReady[channel_id]) { // if the channel is busy before pause
			Spu_setControlRegH(D_SPU_CH_STOPSTATUS, (Spu_getChannelMaskWord(channel_id) >> 16));
		}
		Spu_setControlRegH(D_SPU_CH_ENABLE, Spu_getControlRegH(D_SPU_CH_ENABLE) | (Spu_getChannelMaskWord(channel_id) >> 16));
	}
}

void Spu_resumeWaves(unsigned start_idx, unsigned end_idx) {
	unsigned i = 0;
	unsigned channel_maskL = 0;
	unsigned channel_maskH = 0;
	
	if (start_idx >= D_SPU_TOTAL_CHANNEL) start_idx = D_SPU_TOTAL_CHANNEL;
	if (end_idx >= D_SPU_TOTAL_CHANNEL) end_idx = D_SPU_TOTAL_CHANNEL - 1;

	Spu_getChannelMaskWordByRange(start_idx, end_idx, &channel_maskL);

	channel_maskH = (channel_maskL >> 16) & 0x0000ffff;
	channel_maskL &= 0x0000ffff;

	for (i = start_idx; i <= end_idx; i++) {
		if (i < 16) {
			if (gSpuChannelDataReady[i]) { // if the channel is busy before pause
				Spu_setControlRegL(D_SPU_CH_STOPSTATUS, Spu_getChannelMaskWord(i));
			}
		} else {
			if (gSpuChannelDataReady[i]) { // if the channel is busy before pause
				Spu_setControlRegH(D_SPU_CH_STOPSTATUS, (Spu_getChannelMaskWord(i) >> 16));
			}
		}
	}
	
	if (channel_maskL) {
		Spu_setControlRegL(D_SPU_CH_ENABLE, Spu_getControlRegL(D_SPU_CH_ENABLE) | channel_maskL);
	}

	if (channel_maskH) {
		Spu_setControlRegH(D_SPU_CH_ENABLE, Spu_getControlRegH(D_SPU_CH_ENABLE) | channel_maskH);
	}
}

void Spu_stopWave(int channel_id) {

	if (channel_id < 16) {
		Spu_setControlRegL(D_SPU_CH_ENABLE, Spu_getControlRegL(D_SPU_CH_ENABLE) & (~Spu_getChannelMaskWord(channel_id)));
	} else {
		Spu_setControlRegH(D_SPU_CH_ENABLE, Spu_getControlRegH(D_SPU_CH_ENABLE) & (~(Spu_getChannelMaskWord(channel_id) >> 16)));
	}
	
	gSpuChannelDataReady[channel_id] = 0;
}

void Spu_stopWaves(unsigned start_idx, unsigned end_idx) {
	unsigned i = 0;
	unsigned channel_maskL = 0;
	unsigned channel_maskH = 0;
	
	if (start_idx >= D_SPU_TOTAL_CHANNEL) start_idx = D_SPU_TOTAL_CHANNEL;
	if (end_idx >= D_SPU_TOTAL_CHANNEL) end_idx = D_SPU_TOTAL_CHANNEL - 1;

	Spu_getChannelMaskWordByRange(start_idx, end_idx, &channel_maskL);	

	channel_maskH = (channel_maskL >> 16) & 0x0000ffff;
	channel_maskL &= 0x0000ffff;
	
	if (channel_maskL) {
		Spu_setControlRegL(D_SPU_CH_ENABLE, Spu_getControlRegL(D_SPU_CH_ENABLE) & (~channel_maskL));
	}

	if (channel_maskH) {
		Spu_setControlRegH(D_SPU_CH_ENABLE, Spu_getControlRegH(D_SPU_CH_ENABLE) & (~channel_maskH));
	}
	
	for (i = start_idx; i <= end_idx; i++) {
		gSpuChannelDataReady[i] = 0;
	}	
}

int Spu_isWaveLoopEnable(int channel_id) {
	return gSpuChannelWaveInfo[channel_id].mLoopEnable;
	//return Spu_getChannelControlReg(channel_id, D_SPU_CH_MODE) & 0x2000;
}

void Spu_enableWaveLoop(int channel_id, int enable) {
	unsigned temp = Spu_getChannelControlReg(channel_id, D_SPU_CH_MODE);
	unsigned loop_mode = 0;

	gSpuChannelWaveInfo[channel_id].mLoopEnable = enable;

	if (!Spu_isWaveStop(channel_id)) {
		if (enable) {
			if ((gSpuChannelWaveInfo[channel_id].mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8) ||
				(gSpuChannelWaveInfo[channel_id].mFormat == D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16)) {
				loop_mode = D_LOOP_MODE2;
			} else {
				loop_mode = D_LOOP_MODE1;
			}
		} else {
			loop_mode = D_LOOP_MODE0;
		}

		temp &= 0xcfff;
		temp |= loop_mode;
		Spu_setChannelControlReg(channel_id, D_SPU_CH_MODE, temp);
	}
}

int Spu_getWaveVolume(int channel_id) {
	return (Spu_getChannelControlReg(channel_id, D_SPU_CH_PAN) & 0x00ff);
}

void Spu_setWaveVolume(int channel_id, int vol) {
	unsigned temp = Spu_getChannelControlReg(channel_id, D_SPU_CH_PAN);
	temp &= 0xff00;
	temp += (vol & 0xff);
	temp &= 0x0000ffff;
	Spu_setChannelControlReg(channel_id, D_SPU_CH_PAN, temp);
}

int Spu_getWavePan(int channel_id) {
	return (Spu_getChannelControlReg(channel_id, D_SPU_CH_PAN) & 0xff00) >> 8;
}

void Spu_setWavePan(int channel_id, int pan) {
	unsigned temp = Spu_getChannelControlReg(channel_id, D_SPU_CH_PAN);
	temp &= 0x00ff;
	temp += (pan << 8);
	temp &= 0x0000ffff;
	Spu_setChannelControlReg(channel_id, D_SPU_CH_PAN, temp);
}

unsigned short Spu_getWavePlaybackSpeed(int channel_id) {
	return gSpuChannelSpeed[channel_id];
}

void Spu_setWavePlaybackSpeed(int channel_id, unsigned short speed) {
	unsigned phase = 0;
	gSpuChannelSpeed[channel_id] = speed & 0x03ff;

	if (gSpuChannelSpeed[channel_id] > D_SPEED_MAX_FACTOR) {
		gSpuChannelSpeed[channel_id] = D_SPEED_MAX_FACTOR;
	} else if (gSpuChannelSpeed[channel_id] < D_SPEED_MIN_FACTOR) {
		gSpuChannelSpeed[channel_id] = D_SPEED_MIN_FACTOR;
	}

	phase = gSpuChannelWaveInfo[channel_id].mPhase * gSpuChannelSpeed[channel_id] / D_SPEED_FACTOR_ONE;
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HPHASE, (phase & 0xffff0000) >> 16);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LPHASE, phase & 0x0000ffff);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HPHASE_ACCUMULATOR, 0x0000);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LPHASE_ACCUMULATOR, 0x0000);
}

unsigned Spu_getWavePhase(int channel_id) {
	return gSpuChannelWaveInfo[channel_id].mPhase;
}

void Spu_setWavePhase(int channel_id, unsigned phase) {
	gSpuChannelWaveInfo[channel_id].mPhase = phase;
	phase = gSpuChannelWaveInfo[channel_id].mPhase * gSpuChannelSpeed[channel_id] / D_SPEED_FACTOR_ONE;
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HPHASE, (phase & 0xffff0000) >> 16);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LPHASE, phase & 0x0000ffff);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_HPHASE_ACCUMULATOR, 0x0000);
	Spu_setChannelPhaseReg(channel_id, D_SPU_CH_LPHASE_ACCUMULATOR, 0x0000);
}

void Spu_enableFiq(int channel_id, int enable) {
	unsigned mask = Spu_getChannelMaskWord(channel_id);
	unsigned enable_status = 0;
	
	if (channel_id < 16) {
		enable_status = Spu_getControlRegL(D_SPU_CH_FIQ_ENABLE);
		if (enable) {
			enable_status |= mask;
		} else {
			enable_status &= ~mask;
		}
		Spu_setControlRegL(D_SPU_CH_FIQ_ENABLE, enable_status);
	} else {
		enable_status = Spu_getControlRegH(D_SPU_CH_FIQ_ENABLE);
		if (enable) {
			enable_status |= (mask >> 16);
		} else {
			enable_status &= ~(mask >> 16);
		}
		Spu_setControlRegH(D_SPU_CH_FIQ_ENABLE, enable_status);
	}

}

int Spu_isFiqEnable(int channel_id) {
	unsigned mask = Spu_getChannelMaskWord(channel_id);
	
	if (channel_id < 16) {
		return ((Spu_getControlRegL(D_SPU_CH_FIQ_ENABLE) & mask) != 0);
	}
	
	return ((Spu_getControlRegH(D_SPU_CH_FIQ_ENABLE) & (mask >> 16)) != 0);
}

int Spu_getFiqStatus(int channel_id) {
	unsigned mask = Spu_getChannelMaskWord(channel_id);
	
	if (channel_id < 16) {
		return ((Spu_getControlRegL(D_SPU_CH_FIQ_STATUS) & mask) != 0);
	}
	
	return ((Spu_getControlRegH(D_SPU_CH_FIQ_STATUS) & (mask >> 16)) != 0);
}

void Spu_clearFiqStatus(int channel_id) {
	unsigned mask = Spu_getChannelMaskWord(channel_id);
	
	if (channel_id < 16) {
		Spu_setControlRegL(D_SPU_CH_FIQ_STATUS, mask);
	} else {
		Spu_setControlRegH(D_SPU_CH_FIQ_STATUS, mask >> 16);
	}
}

void Spu_setFiqHandler(int channel_id, SpuFiqHandler handler) {
	gSpuFiqHandler[channel_id] = handler;
}

void Spu_setBeatCntHandler(SpuBeatCntHandler handler) {
	gSpuBeatCntHandler = handler;
}

int Spu_isWavePlaying(int channel_id) {
	unsigned temp = 0;

	if (channel_id < 16) {
		temp = Spu_getControlRegL(D_SPU_CH_STATUS) & Spu_getChannelMaskWord(channel_id);
	} else {
		temp = Spu_getControlRegH(D_SPU_CH_STATUS) & (Spu_getChannelMaskWord(channel_id) >> 16);
	}
	
	return (temp != 0);
}

int Spu_isWavePause(int channel_id) {
	unsigned temp = 0;
	if (channel_id < 16) {
		temp = Spu_getControlRegL(D_SPU_CH_ENABLE) & Spu_getChannelMaskWord(channel_id);
	} else {
		temp = Spu_getControlRegH(D_SPU_CH_ENABLE) & (Spu_getChannelMaskWord(channel_id) >> 16);
	}
	
	return (temp == 0) && (gSpuChannelDataReady[channel_id]);	
}

int Spu_isWaveStop(int channel_id) {
	return (!Spu_isWavePlaying(channel_id) && !Spu_isWavePause(channel_id));
}

void Spu_FiqIsr() {
	unsigned fiq_status = Spu_getControlRegLH(D_SPU_CH_FIQ_STATUS);
	unsigned idx = lsb(fiq_status);

	if (gSpuFiqHandler[idx]) {
		(*gSpuFiqHandler[idx])(idx);
	}
}

void Spu_BeatCntIsr() {
	if (gSpuBeatCntHandler) {
		(*gSpuBeatCntHandler)();
	}
}
