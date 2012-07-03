#ifndef SPUREG_H
#define SPUREG_H

////////////////////////////////////////////////////////
//
// Jackson:
// SPU LOW LEVEL OPERATIONS
// USE WITH EXTREME CARE
//
////////////////////////////////////////////////////////

typedef enum {
	D_SPU_CH_ENABLE = 0x0000,
	D_SPU_VOLUME = 0x0001,
	D_SPU_CH_FIQ_ENABLE = 0x0002,
	D_SPU_CH_FIQ_STATUS = 0x0003,
	D_SPU_BEATBASECNT = 0x0004,
	D_SPU_BEATCNT = 0x0005,
	D_SPU_ENV_CLOCK0 = 0x0006,
	D_SPU_ENV_CLOCK1 = 0x0007,
	D_SPU_ENV_CLOCK2 = 0x0008,
	D_SPU_ENV_CLOCK3 = 0x0009,
	D_SPU_CH_ENV_RAMPDOWN_ENABLE = 0x000A,
	D_SPU_CH_STOPSTATUS = 0x000B,
	//D_SPU_CH_ZEROCROSS_ENABLE = 0x000C,
	D_SPU_CONTROL = 0x000D,
	D_SPU_THRESHOLD = 0x000E,
	D_SPU_CH_STATUS = 0x000F,
	D_SPU_WAVE_INPUTL = 0x0010,
	D_SPU_WAVE_INPUTR = 0x0011,
	D_SPU_WAVE_OUTPUTL = 0x0012,
	D_SPU_WAVE_OUTPUTR = 0x0013,
	D_SPU_CH_REPEAT_ENABLE = 0x0014,
	D_SPU_CH_ENV_MODE_SEL = 0x0015,
	D_SPU_CH_TONERELEASE_ENABLE = 0x0016,
	D_SPU_CH_ENV_IRQ_STATUS = 0x0017,
	D_SPU_CH_PITCHBEND_ENABLE = 0x0018,
	//D_SPU_SOFTCH_PHASE = 0x0019,
	D_SPU_ATTACK_RELEASE_TIME = 0x001A,
	//D_SPU_EQ_CUTFREQ10 = 0x001B,
	//D_SPU_EQ_CUTFREQ32 = 0x001C,
	//D_SPU_EQ_GAIN10 = 0x001D,
	//D_SPU_EQ_GAIN32 = 0x001E,
	
	/* vicsiu 2011-03-24: NOT USE!
		D_SPU_BANKADDR = 0x001F,*/
		
	//D_SPU_SOFTCH_BASEL = 0x0020,
	//D_SPU_SOFTCH_BASEH = 0x0021,
	//D_SPU_SOFTCH_CONTROL = 0x0022,
	D_SPU_PWEN = 0x0025,
	D_SPU_ENV_CLOCK4 = 0x0026,
	D_SPU_ENV_CLOCK5 = 0x0027,
	D_SPU_ENV_CLOCK6 = 0x0028,	
	D_SPU_ENV_CLOCK7 = 0x0029,
	D_SPU_PWAVE_OUTPUTL = 0x0032,	
	D_SPU_PWAVE_OUTPUTR = 0x0033,	
	//D_SPU_ENV_CLOCK8 = 0x0806,
	//D_SPU_ENV_CLOCK9 = 0x0807,
	//D_SPU_ENV_CLOCK10 = 0x0808,
	//D_SPU_ENV_CLOCK11 = 0x0809,
} SpuControlReg;

typedef enum {
	D_SPU_CH_WAVEADDR = 0x0000,			//vicsiu: contain address
	D_SPU_CH_MODE = 0x0001,				//vicsiu: contain address
	D_SPU_CH_LOOPADDR = 0x0002,			//vicsiu: contain address
	D_SPU_CH_PAN = 0x0003,
	D_SPU_CH_ENVELOP0 = 0x0004,
	D_SPU_CH_ENVELOP_DATA = 0x0005,
	D_SPU_CH_ENVELOP1 = 0x0006,
	D_SPU_CH_ENVELOP_SEGMENT = 0x0007,	//vicsiu: contain address
	D_SPU_CH_ENVELOP_OFFSET = 0x0008,	//vicsiu: contain address
	D_SPU_CH_WAVEDATA0 = 0x0009,
	D_SPU_CH_ENVELOPLoop = 0x000A,
	D_SPU_CH_WAVEDATA = 0x000B,
	D_SPU_CH_ADPCMSEL = 0x000D,
	D_SPU_CH_WL_HADDR = 0x000E,			//vicsiu: contain address
	D_SPU_CH_E_HADDR = 0x000F,			//vicsiu: contain address
} SpuChannelControlReg;

typedef enum {
	D_SPU_CH_HPHASE = 0x0000,
	D_SPU_CH_HPHASE_ACCUMULATOR = 0x0001,
	D_SPU_CH_HTARGETPHASE = 0x0002,
	D_SPU_CH_RAMP_DOWN_CLK = 0x0003,
	D_SPU_CH_LPHASE = 0x0004,
	D_SPU_CH_LPHASE_ACCUMULATOR = 0x0005,
	D_SPU_CH_LTARGETPHASE = 0x0006,
	D_SPU_CH_PHASE_CHANGE_CONTROL = 0x0007,
} SpuChannelPhaseReg;

#ifdef __cplusplus
extern "C" {
#endif

unsigned Spu_getChannelMaskWord(unsigned channel);
void Spu_getChannelMaskWordByRange(unsigned start_idx, unsigned end_idx, unsigned* channel_maskLH);
unsigned Spu_getControlRegL(SpuControlReg reg);
unsigned Spu_getControlRegH(SpuControlReg reg);
unsigned Spu_getControlRegLH(SpuControlReg reg);
unsigned Spu_getChannelControlReg(unsigned channel, SpuChannelControlReg reg);
unsigned Spu_getChannelPhaseReg(unsigned channel, SpuChannelPhaseReg reg);
void Spu_setControlRegL(SpuControlReg reg,  unsigned value);
void Spu_setControlRegH(SpuControlReg reg,  unsigned value);
void Spu_setControlRegLH(SpuControlReg reg, unsigned value);
void Spu_setChannelControlReg(unsigned channel, SpuChannelControlReg reg, unsigned value);
void Spu_setChannelPhaseReg(unsigned channel, SpuChannelPhaseReg reg, unsigned value);

void Spu_syncDacOutput(void);

#ifdef __cplusplus
}
#endif

#endif
