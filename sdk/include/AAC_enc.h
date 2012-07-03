#ifndef __AAC_ENC_H__
#define __AAC_ENC_H__


#define AAC_INPUT_FRAME_LEN			1024	// samples per channel
#define AAC_OUTPUT_FRAME_MAX_LEN	 768	// bytes per channel


int aac_encoder_instance_size(void);


// bit_rate: in "bits per second"
// sample_rate: in "Hz"
// format : 0(ADTS), 1(ADIF) or 2(RAW)
int aac_encoder_init(void *pWorkMem,
					 unsigned int bit_rate, unsigned int sample_rate, unsigned int nchannels,
					 unsigned int format, char *ExtData, int *ExtDataLen);

// PCM: input buffer
// BS: output buffer
// cbBSLen: the length of output buffer (count in byte)
int aac_encoder_run(void *pWorkMem, const short *PCM,
					unsigned char *BS, int cbBSLen);
					
int aac_encoder_uninit(void *pWorkMem);

const char *aac_encoder_version_string(void);




#endif	// __AAC_ENC_H__
