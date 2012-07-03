
#include <linux/kernel.h>       /* printk() */
#include <linux/math64.h>
#include <mach/audio2/agc.h>
#include <mach/hal/hal_dac.h>

#define Q2_BITS 24
#define Q2_PRECISION (1 << Q2_BITS)
#define Q2_CONST(A) (((A) >= 0) ? ((int)((A)*(Q2_PRECISION)+0.5)) : ((int)((A)*(Q2_PRECISION)-0.5)))


#define FRAC_BITS 31
#define FRAC_PRECISION ((unsigned int)(1 << FRAC_BITS))
#define FRAC_MAX 0x7FFFFFFF
#define FRAC_CONST(A) (((A) == 1.00) ? ((int)FRAC_MAX) : (((A) >= 0) ? ((int)((A)*(FRAC_PRECISION)+0.5)) : ((int)((A)*(FRAC_PRECISION)-0.5))))

#define CONST_ENV    FRAC_CONST(0.02)
#define CONST_SMAX   FRAC_CONST(0.0004)
#define ATTACK_RATE  FRAC_CONST(0.2)
#define RELEASE_RATE FRAC_CONST(0.0001)

#define	MIC_PLATFORM_GAIN					10
#define AUDIO_MIC_REC_DRC_GAIN		(MIC_PLATFORM_GAIN)

__inline int MUL_Q2S8(int x,int y)  { return ((long long)x*y)>>(Q2_BITS-8);};
__inline int MUL_FAC(int x,int y)   { return ((long long)x*y)>>FRAC_BITS;  };
__inline int MUL_Q2IS8(int x,int y) { return ((long long)x*y)>>(Q2_BITS+8);};

static int max_level;
static int dc_r=0;
static int envS;
static int Signal;
static int Gain;
static int MicSettingGain;
static int TargetGain;

static int DelayBuffer[96];
static int *pDelay;

int PGA_Gain[46]={ // Normalize PGA gain to 0dB
Q2_CONST(0.022387211),Q2_CONST(0.026607251),Q2_CONST(0.031622777),Q2_CONST(0.03758374 ),
Q2_CONST(0.044668359),Q2_CONST(0.053088444),Q2_CONST(0.063095734),Q2_CONST(0.074989421),
Q2_CONST(0.089125094),Q2_CONST(0.105925373),Q2_CONST(0.125892541),Q2_CONST(0.149623566),
Q2_CONST(0.177827941),Q2_CONST(0.211348904),Q2_CONST(0.251188643),Q2_CONST(0.298538262),
Q2_CONST(0.354813389),Q2_CONST(0.421696503),Q2_CONST(0.501187234),Q2_CONST(0.595662144),
Q2_CONST(0.707945784),Q2_CONST(0.841395142),Q2_CONST(1          ),Q2_CONST(1.188502227),
Q2_CONST(1.412537545),Q2_CONST(1.678804018),Q2_CONST(1.995262315),Q2_CONST(2.371373706),
Q2_CONST(2.818382931),Q2_CONST(3.349654392),Q2_CONST(3.981071706),Q2_CONST(4.73151259 ),
Q2_CONST(5.623413252),Q2_CONST(6.683439176),Q2_CONST(7.943282347),Q2_CONST(9.440608763),
Q2_CONST(11.22018454),Q2_CONST(13.33521432),Q2_CONST(15.84893192),Q2_CONST(18.83649089),
Q2_CONST(22.38721139),Q2_CONST(26.6072506 ),Q2_CONST(31.6227766 ),Q2_CONST(37.58374043),
Q2_CONST(44.66835922),Q2_CONST(0          )
};

void I2SRx_Mic_VolGet(unsigned int* a_pVolume_value)
{
//	unsigned int Vol;
	long Vol;

	audio2_MIC_volget(&Vol);

//	if(Vol!= 0)
//	{
//		printk("Error:vol get addr failed...%d\n", (int)Vol);
//	}
//	else
//	{
//		*a_pVolume_value = ((~Vol) & 0x1f);
		*a_pVolume_value = Vol;//((~Vol) & 0x1f);
//	}
}

int I2SRx_Mic_VolSet(int a_volume_value)
{
//	unsigned int 	volume;
	long volume;
	int	volume_set = 0;

	volume_set = a_volume_value;
	if(a_volume_value > 0x1f)
	{
		volume_set = 0x1f;

	}
	else if(a_volume_value < 0)
	{
		volume_set = 0;
	}
	
//	volume = ((~volume_set) & 0x1f);  //volume scale => 5 bits	
	volume = volume_set;//((~volume_set) & 0x1f);
//	printk("vol=%d\n", volume);
	audio2_MIC_volset(volume);

	return volume_set;
}

static int abs_mono(int x)
{
    if( x < 0 ) x = -x;
    
    if( x > max_level ) max_level = x;

    return x;
}

static int envelope(int x)
{
	if( x >= envS ) {
		envS = x;
	} else {
		x = x - envS;
		envS = envS + MUL_FAC(x,CONST_ENV);
	}

    x = envS;
	if( x >= Signal ) {
		Signal = x;
		return Signal;
	} else {
		x = x - Signal;
		Signal = Signal + MUL_FAC(x,CONST_SMAX);
		return 0;
	}    
}

void  I2SRx_init_mic_agc(void)
{
	int i;

	MicSettingGain    = Q2_CONST(AUDIO_MIC_REC_DRC_GAIN); // (1 - 40)<=>(0dB ~32 dB)
	I2SRx_Mic_VolSet(9);        // Setting PGA to minimum value when initial ( -9 dB )
//	I2SRx_Mic_VolSet(0x1f);        // Setting PGA to minimum value when initial ( 0 dB )
//	I2SRx_Mic_VolSet(9);
//	sar_audio_SET_MIC_boost(false); // Enable/disable 20dB boost        

	TargetGain = MicSettingGain;
	Gain = 0;
	pDelay = DelayBuffer;             // initialize delay buffer
	for(i=0;i<96;i++) DelayBuffer[i] = 0;
}

int I2SRx_Mic_AGC(unsigned int AGCbufferAddr, int AGCbufferSize)
{
	int	x,z;//,y
	int	out;
	int	i;
	int	attack;
	int amp=0;
	short	*pBuffPtrRead; 
	short	*pBuffPtrWrite;
	int	InvPGA;
	long long	tmp;
	int mean=0;
	
	x=0;
	
	pBuffPtrRead = pBuffPtrWrite = ((short*)AGCbufferAddr);
	I2SRx_Mic_VolGet(&x);
	InvPGA = PGA_Gain[31-x];
	AGCbufferSize >>= 1; // Convert AGCbufferSize from Bytes to Samples ( divide by 4 )
	max_level = 0;
	for(i=0;i<AGCbufferSize;i++){
		
		x = *pBuffPtrRead++; 
//		y = *pBuffPtrRead++;
		// x = x + y + 0x8000;   // SPMP8000 use this way
		x = ((x + x)>>1)>>2;	 // joseph.hsieh@100225 (x+y)>>1 = average of stereo channels,  ">>2" is a factor, you can adjust it.
		x = x - dc_r;
		mean += x;


		z = abs_mono(x);
		z = MUL_Q2S8(z,InvPGA);
	
		out = MUL_Q2S8(x,InvPGA);
	
		attack = envelope(z);
			
		if( attack ) {
			amp = MUL_Q2IS8(Signal,Gain);
			if(amp!=0) {
	//			tmp = ((long long)30000*Gain)/amp;
				tmp = div_s64((long long)30000*Gain, amp);
				if( tmp < MicSettingGain ) TargetGain = tmp;
				else  TargetGain = MicSettingGain;
			} else {
				TargetGain = MicSettingGain;
			}
		}
		
		z = TargetGain - Gain;
		if( z > 0 ) {  // Rampup gain
		  Gain = Gain + MUL_FAC(z,RELEASE_RATE);
		} else {	   // Rampdown gain
		  Gain = Gain + MUL_FAC(z,ATTACK_RATE);
		}
	
		// prepare output data
		x = pDelay[0];	   // Get delayed sample

		x = MUL_Q2IS8(x,Gain);
			
		*pBuffPtrWrite++ = x;
	//	*pBuffPtrWrite++ = x; // write as mono
			
		*pDelay++ = out;

		if( pDelay >= &DelayBuffer[96] ) pDelay = DelayBuffer;		
	}
//	printk("max=%d\n", max_level);
	if( (max_level) > 30000 ) {
		I2SRx_Mic_VolGet(&x);
		x-=3;
		if(x < 3) x = 3; // lower bound of PGA
		I2SRx_Mic_VolSet(x);
	//	printk("+++++++++++++++++++++++++++++++++++++++\n");
	} else {
		if( max_level < 8000 ) { 
            I2SRx_Mic_VolGet(&x); 
            x++; 
            if(x > 9) x = 9; // upper bound of PGA 
            I2SRx_Mic_VolSet(x);
   //         printk("----------------------------------------\n");
        } 
	    mean = mean / AGCbufferSize;
	    dc_r += mean;
	}
	return 0;
}
