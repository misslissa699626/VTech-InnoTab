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

#include "Audio.h"
#include "SpuDrv.h"
#include "SpuReg.h"
#include "AudioPhaseTable.h"

extern unsigned int USER_VA_TO_KERNEL_VA(unsigned int uva);


//#define DEBUG_PRINT		printk
#define DEBUG_PRINT(...)

//#define TRACE_LINE	{DEBUG_PRINT(KERN_WARNING "{TRACE_LINE %s:%d:%s()}\n", __FILE__, __LINE__, __FUNCTION__);}
#define TRACE_LINE	


typedef struct {
	unsigned mPitch;
	unsigned mEnv;
	unsigned mEnvOffset;
	unsigned mDataSize;
	unsigned char mData[1];
} ToneColor;

typedef struct {
	unsigned mToneColorCount;
	unsigned mDataOffset;
	unsigned mToneColorOffset[1];
} Instrument;

// beat count unit time definition
// jackson: from SPU spec.
// jackson: SamplingRate = 288 KHz
// jackson: Frame = 1 / SamplingRate = 3.472us
// jackson: BeatIRQ_Period
// jackson: 	= (BeatBaseCnt * BeatCnt) * 4 * Frame
// jackson: 	= (BeatBaseCnt * BeatCnt) * 13.889us
#define D_BEATBASECNT 0x168
#define D_BEATCNT 0x1
#define D_BEATBASEPERIOD 0xde3 // 13.889us, represents as 8:8 fix point number
#define D_BEATCNTPERIOD ((D_BEATBASEPERIOD * D_BEATBASECNT) >> 8) // in terms of us
// #define D_BEATCNTPERIOD D_BEATBASECNT * D_BEATCNT * D_BEATBASEPERIOD

// definition for default ramp down settings
#define D_RAMPDOWN_OFFSET 0x4000
#define D_RAMPDOWN_CLOCK 0x0002

// total number of logical channel for MIDI playback
#define D_MIDI_TOTAL_LOGICAL_CHANNEL 16
#define D_MIDI_PRECUSSION_CHANNEL 9

#define D_MIDI_CMD_PLAYNOTE 0x0000
#define D_MIDI_CMD_BEATCNT 0x1000
#define D_MIDI_CMD_PITCHBEND 0x2000
#define D_MIDI_CMD_CONTROL 0x3000
#define D_MIDI_CMD_PROGCHANGE 0x4000
#define D_MIDI_CMD_TEMPOCHANGE 0x5000
#define D_MIDI_CMD_ENDSEQ 0x6000
#define D_MIDI_CMD_LYRIC 0x7000
#define D_MIDI_CMD_TEXT 0x8000

#define D_CTRL_VOL 0x07
#define D_CTRL_PAN 0x0A
#define D_CTRL_EXP 0x0B
#define D_CTRL_RPN_LSB 0x64
#define D_CTRL_RPN_MSB 0x65
#define D_CTRL_SUBBANK 0x20
#define D_CTRL_DATA_ENTRY_MSB 0x06 // jackson: not found in spec
#define D_CTRL_DATA_ENTRY_LSB 0x26 // jackson: not found in spec

#define D_MIDI_DEFAULT_PITCH_BEND 0x2000

#define D_MIDI_MAX_INSTRUMENT_COUNT 128

#define MIDI_MAX_TEXT_LEN 255 * sizeof(unsigned short)

unsigned gAudioTotalChannel = D_SPU_TOTAL_CHANNEL;
unsigned gMidiStartChannel = D_SPU_TOTAL_CHANNEL;

unsigned* gMidiMelodyTable[D_MIDI_MAX_INSTRUMENT_COUNT] = {0};
unsigned* gMidiDrumTable[D_MIDI_MAX_INSTRUMENT_COUNT] = {0};

AudioPlayStatus gMidiStatus = AUDIO_PLAY_STOP;
unsigned char* gMidiScoreBase = 0;
unsigned char* gMidiScore = 0;
unsigned gMidiTick = 0;
unsigned gMidiNextTick = 0;
unsigned gMidiScoreSize = 0;
AudioMidiStream* gMidiStream = 0;
int gMidiVolume = 0x7f;
int gMidiPan = 0x40;
int gMidiEnableGlobalPan = 0;
int gMidiEnableLoop = 0;
unsigned short gMidiPlaybackSpeed = AUDIO_X1_MIDI_PLAYBACK_SPEED;
int gMidiFadeVolume = 0x7f;
int gMidiFadeStartTime = 0;
int gMidiFadeEndTime = 0;
int gMidiFadeDirection = 0;

int gMidiDataReady = 0;
int gMidiPauseCommit = 0;
int gMidiResumeCommit = 0;
int gMidiBeatCount = 0;
int gMidiOldNextBeatCount = 0;
int gMidiNextBeatCount = 0;
int gMidiMaxBeatCount = 0;
int gMidiLastSelectChannel = 0;
unsigned gMidiKeyOnMaskLH = 0;

signed char gMidiLogicalChannelInstrument[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned short gMidiLogicalChannelPitchBend[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned char gMidiLogicalChannelVol[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned char gMidiLogicalChannelPan[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned char gMidiLogicalChannelExpression[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned short gMidiLogicalChannelRPN[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned short gMidiLogicalChannelDataEntry[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned char gMidiLogicalChannelSubBand[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned char gMidiLogicalChannelEnable[D_MIDI_TOTAL_LOGICAL_CHANNEL] = {0};
unsigned char gMidiLogicalChannelTempo = 0;

unsigned char gMidiChannel2LogicalChannelMap[D_SPU_TOTAL_CHANNEL] = {0};
int gMidiChannelDuration[D_SPU_TOTAL_CHANNEL] = {0};
unsigned char gMidiChannelVol[D_SPU_TOTAL_CHANNEL] = {0};
unsigned char gMidiChannelPitch[D_SPU_TOTAL_CHANNEL] = {0};
unsigned gMidiChannelPhase[D_SPU_TOTAL_CHANNEL] = {0};
int gMidiChannelRampDownTick[D_SPU_TOTAL_CHANNEL] = {0};

unsigned char gMidiInsVolume[2][D_MIDI_MAX_INSTRUMENT_COUNT] = {{0}, {0}};
unsigned char gMidiInsVolumeChange[2][D_MIDI_MAX_INSTRUMENT_COUNT] = {{0}, {0}};

AudioMidiLyricEventHandler gMidiLyricEventHandler = 0;
unsigned char gMidiTextBuffer[MIDI_MAX_TEXT_LEN];

#define AUDIO_STREAM_WORKING_BUFFER_SIZE (256 * 1024)
typedef struct {
	AudioStreamInputParam mParam;
	int mCounter;
	int mLeftChannel;
	int mRightChannel;
	unsigned short* mIdleLeftBuffer;
	unsigned short* mIdleRightBuffer;
	unsigned short* mLeftBuffer;
	unsigned short* mRightBuffer;
	int mChannelBufferSize; // in terms of word
	int mChannelBufferCount;
	int mCurrentFillBuffer;
	int mCurrentFillPos;
	int mNextPlayBuffer;
	int mCurrentLockWriteBuffer;
	int mCurrentLockReadBuffer;
	int mPreviousLockReadBuffer;
	int mPause;
	int mPlayPos;
	int mAvailable;
} AudioStream;

static AudioStream gAudioStream[MAX_AUDIO_STREAM];
static int gAudioStreamCount = 4;

//
// help functions
//

#define OOPS    {*(int*)0 = 0;}

static unsigned int KERNEL_VA_TO_PA(unsigned int kva)
{
        if (kva) {
                unsigned int pa = gp_chunk_pa((void*)kva);      /* kernel_addr to phy_addr */
                if (pa) {
                        return pa;
                }
                DEBUG_PRINT(KERN_WARNING "%s:%s(0x%08X): failed!\n", __FILE__, __FUNCTION__, kva);
                OOPS
        }
        return 0;
}
static int Audio_getMaxWaveChannel(void) {
	return gMidiStartChannel - gAudioStreamCount * 2;
}

static unsigned short* Audio_allocStreamBuffer(int stream_count) {
	return (unsigned short*)gp_chunk_malloc(current->pid, stream_count * AUDIO_STREAM_WORKING_BUFFER_SIZE + (128 * 1024)); // make sure 128kB aligned
}

static void Audio_startStream(int idx, unsigned short* working_buffer) {
	// 1. set default value to left and right channel
	// 2. initialize stream buffer

	unsigned short* temp = 0;
	unsigned short* left_buffer = 0;
	unsigned short* right_buffer = 0;
	int i = 0;
	int total_buffer_size = 0;
	WaveInfo info;

	gAudioStream[idx].mParam.mSampleSize = 16;
	gAudioStream[idx].mParam.mSamplingFrequency = 24000;
	gAudioStream[idx].mParam.mChannel = 2;
	gAudioStream[idx].mLeftChannel = 15 - idx * 2;
	gAudioStream[idx].mRightChannel = 15 - idx * 2 - 1;

	temp = working_buffer;

	gAudioStream[idx].mIdleLeftBuffer = temp;

	gAudioStream[idx].mChannelBufferSize = gAudioStream[idx].mParam.mSamplingFrequency / 8;

	temp += gAudioStream[idx].mChannelBufferSize;

	gAudioStream[idx].mLeftBuffer = temp;

	total_buffer_size = (AUDIO_STREAM_WORKING_BUFFER_SIZE / sizeof(unsigned short) / 2) - gAudioStream[idx].mChannelBufferSize ;

	gAudioStream[idx].mChannelBufferCount = total_buffer_size / gAudioStream[idx].mChannelBufferSize;

	temp += total_buffer_size;
	gAudioStream[idx].mIdleRightBuffer = temp;
	temp += gAudioStream[idx].mChannelBufferSize;
	gAudioStream[idx].mRightBuffer = temp;

	gAudioStream[idx].mCurrentFillPos = 0;
	gAudioStream[idx].mCurrentFillBuffer = 0;
	gAudioStream[idx].mNextPlayBuffer = 0;
	gAudioStream[idx].mCurrentLockWriteBuffer = 0;
	gAudioStream[idx].mCurrentLockReadBuffer = -1;
	gAudioStream[idx].mPreviousLockReadBuffer = -1;
	
	for (i = 0; i < gAudioStream[idx].mChannelBufferSize - 1; i++) {
		gAudioStream[idx].mIdleLeftBuffer[i] = 0x8000;
		gAudioStream[idx].mIdleRightBuffer[i] = 0x8000;
	}
	gAudioStream[idx].mIdleLeftBuffer[i] = 0xffff;
	gAudioStream[idx].mIdleRightBuffer[i] = 0xffff;

	left_buffer = gAudioStream[idx].mLeftBuffer;
	right_buffer = gAudioStream[idx].mRightBuffer;
	left_buffer += (gAudioStream[idx].mChannelBufferSize - 1);	
	right_buffer += (gAudioStream[idx].mChannelBufferSize - 1);
	for (i = 0; i < gAudioStream[idx].mChannelBufferCount; i++) {
		*left_buffer = 0xffff;
		*right_buffer = 0xffff;
		left_buffer += gAudioStream[idx].mChannelBufferSize;
		right_buffer += gAudioStream[idx].mChannelBufferSize;
	}

	info.mFormat = D_FORMAT_ATTACK_PCM16_LOOP_PCM16;
	info.mLoopEnable = 1;
	info.mEnvAutoModeEnable = 0;
	info.mWaveAddr = (unsigned char*)gAudioStream[idx].mIdleLeftBuffer;
	info.mLoopAddr = (unsigned char*)gAudioStream[idx].mIdleLeftBuffer;
	info.mEnvAddr = 0;
	info.mEnv0 = 0;
	info.mEnv1 = 0;
	info.mEnvData = 0x7f;
	info.mRampDownOffset = D_RAMPDOWN_OFFSET;
	info.mRampDownClk = D_RAMPDOWN_CLOCK;
	info.mPhase = SAMPLERATE_2_PHASE(gAudioStream[idx].mParam.mSamplingFrequency);
	Spu_setWaveEx(gAudioStream[idx].mLeftChannel, &info);

	info.mFormat = D_FORMAT_ATTACK_PCM16_LOOP_PCM16;
	info.mLoopEnable = 1;
	info.mEnvAutoModeEnable = 0;
	info.mWaveAddr = (unsigned char*)gAudioStream[idx].mIdleRightBuffer;
	info.mLoopAddr = (unsigned char*)gAudioStream[idx].mIdleRightBuffer;
	info.mEnvAddr = 0;
	info.mEnv0 = 0;
	info.mEnv1 = 0;
	info.mEnvData = 0x7f;
	info.mRampDownOffset = D_RAMPDOWN_OFFSET;
	info.mRampDownClk = D_RAMPDOWN_CLOCK;
	info.mPhase = SAMPLERATE_2_PHASE(gAudioStream[idx].mParam.mSamplingFrequency);
	Spu_setWaveEx(gAudioStream[idx].mRightChannel, &info);

	Spu_setWavePan(gAudioStream[idx].mLeftChannel, 0x0); // pan left
	Spu_setWavePan(gAudioStream[idx].mRightChannel, 0x7f); // pan right

	Spu_resumeWaves(gAudioStream[idx].mRightChannel, gAudioStream[idx].mLeftChannel);

	Spu_setFiqHandler(gAudioStream[idx].mLeftChannel, Audio_handleFiq);
	Spu_clearFiqStatus(gAudioStream[idx].mLeftChannel);
	Spu_enableFiq(gAudioStream[idx].mLeftChannel, 1);
}

static void Audio_stopStream(int idx) {
	Spu_stopWaves(gAudioStream[idx].mRightChannel, gAudioStream[idx].mLeftChannel);

	Spu_enableFiq(gAudioStream[idx].mLeftChannel, 0);
	Spu_clearFiqStatus(gAudioStream[idx].mLeftChannel);
}


void Audio_initStreams(int n) {
	int i = 0;
	unsigned short* working_buffer = 0;
	unsigned working_buffer_pa = 0;
	unsigned unaligned_bytes = 0;

	if (n > MAX_AUDIO_STREAM) {
		n = MAX_AUDIO_STREAM;
	}

	gAudioStreamCount = n;

	for (i = 0; i < MAX_AUDIO_STREAM; i++) {
		memset(&gAudioStream[i], 0, sizeof(AudioStream));
		if (i < n) {
			gAudioStream[i].mAvailable = 1;
		}
	}

	if (!gAudioStreamCount) return;

	working_buffer = Audio_allocStreamBuffer(gAudioStreamCount);

	if (!working_buffer) {
		return;
	}

	working_buffer_pa = KERNEL_VA_TO_PA((unsigned)working_buffer);
	unaligned_bytes = 0x00020000 - (working_buffer_pa & 0x0001ffff);

	working_buffer = (unsigned short*)((unsigned)working_buffer + unaligned_bytes);

	for (i = 0; i< gAudioStreamCount; i++) {
		Audio_startStream(i, working_buffer);
		working_buffer += AUDIO_STREAM_WORKING_BUFFER_SIZE / sizeof(unsigned short); // 128k words for each stream
	}
}

void Audio_cleanupStreams(void) {
	int i = 0;
	for (i = 0; i < gAudioStreamCount; i++) {
		Audio_stopStream(i);
	}
}

int Audio_findEmptyMidiChannel(void) {
	int i = 0;
	//unsigned temp = 0;
	
	for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
		gMidiLastSelectChannel = gMidiLastSelectChannel + 1;
		if (gMidiLastSelectChannel >= gAudioTotalChannel) {
			gMidiLastSelectChannel = gMidiStartChannel;
		}
		if (gMidiChannelDuration[gMidiLastSelectChannel] <= 0) {
			if (!Spu_isWavePlaying(gMidiLastSelectChannel)) {
				gMidiKeyOnMaskLH |= Spu_getChannelMaskWord(gMidiLastSelectChannel);
				return gMidiLastSelectChannel;
			}
		}
	}
	
	return -1;
}

unsigned short Audio_readMidiScore(int* error_code) {
	if (!gMidiStream) {
		unsigned short temp = *(unsigned short*)gMidiScore;

		if (gMidiScoreSize && (gMidiScore - gMidiScoreBase) >= gMidiScoreSize) {
			if (error_code) {
				*error_code = 1;
			}
		} else {
			gMidiScore += sizeof(unsigned short);
			if (error_code) {
				*error_code = 0;
			}
		}

		return temp;
	} else {
		unsigned short temp = 0;
		int result = (*gMidiStream->mAudioMidiStreamRead)((unsigned char*)&temp, sizeof(unsigned short), gMidiStream);
		if (error_code) {
			if (result <= 0) {
				*error_code = 1;
			} else {
				*error_code = 0;
			}
		}

		return temp;
	}

	return 0; // should never reach
}

void Audio_readMidiScoreN(unsigned char* buffer, unsigned len, int* error_code) {
	if (!gMidiStream) {
		if (gMidiScoreSize && (gMidiScore - gMidiScoreBase) >= gMidiScoreSize) {
			if (error_code) {
				*error_code = 1;
			}
		} else {
			memcpy(buffer, gMidiScore, len);
			if (error_code) {
				*error_code = 0;
			}
			gMidiScore += len;
		}
	} else {
		int result = (*gMidiStream->mAudioMidiStreamRead)(buffer, len, gMidiStream);
		if (error_code) {
			if (result <= 0) {
				*error_code = 1;
			} else {
				*error_code = 0;
			}
		}
	}
}

void Audio_resetMidiScore(void) {
	if (!gMidiStream) {
		gMidiScore = gMidiScoreBase;
	} else {
		(*gMidiStream->mAudioMidiStreamReset)(gMidiStream);
	}
	gMidiTick = 0;
	gMidiNextTick = 0;
}

unsigned Audio_getPitchBendFactor(unsigned logical_channel) {
	int pb_value = 0;
	unsigned pb_table_idx = 0;

	pb_value = ((int)gMidiLogicalChannelPitchBend[logical_channel] - D_MIDI_DEFAULT_PITCH_BEND) >> 7;
	
	pb_table_idx = gMidiLogicalChannelDataEntry[logical_channel] >> 8;
	if (pb_table_idx > 24) {
		pb_table_idx = 0;
	}
	
	return Audio_computePitchBendFactor(pb_table_idx, pb_value);
}

void Audio_setDrumToneColor(int channel_id, ToneColor* tone_color) {

	WaveInfo info;

	unsigned is_adpcm = 0;
	unsigned is_adpcm36 = 0;
	unsigned is_pcm16 = 0;
	unsigned char* temp_pcm_data = tone_color->mData;

	is_adpcm = ((unsigned short*)(tone_color->mData))[16] & 0x0040;
	is_adpcm36 = ((unsigned short*)(tone_color->mData))[16] & 0x0080;
	is_pcm16 = ((unsigned short*)(tone_color->mData))[16] & 0x0010;
	temp_pcm_data += 0x00000028; // jackson: tone color header size, temporary hard code 40 bytes

	if (is_adpcm) {
		if (is_adpcm36) {
			if (is_pcm16) {
				info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16;
			} else {
				info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8;
			}
		} else {
			if (is_pcm16) {
				info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM16;
			} else {
				info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM8;
			}
		}
	} else {
		if (is_pcm16) {
			info.mFormat = D_FORMAT_ATTACK_PCM16_LOOP_PCM16;
		} else {
			info.mFormat = D_FORMAT_ATTACK_PCM8_LOOP_PCM8;
		}
	}

	info.mLoopEnable = 0;
	info.mEnvAutoModeEnable = 1;
	info.mWaveAddr = temp_pcm_data;
	info.mLoopAddr = 0;
	info.mEnvAddr = (unsigned char*)((unsigned)tone_color->mData + tone_color->mEnvOffset);
	info.mEnv0 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[0];
	info.mEnv1 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[1];
	info.mEnvData = 0x7f;
	info.mRampDownOffset = 0x4000;
	info.mRampDownClk = 0x0002;
	info.mPhase = SAMPLERATE_2_PHASE(((unsigned short*)(tone_color->mData))[8]);

	Spu_setWaveEx(channel_id, &info);
}

void Audio_setMelodyToneColor(int channel_id, ToneColor* tone_color, int pitch) {

	WaveInfo info;

	unsigned is_adpcm = 0;
	unsigned is_adpcm36 = 0;
	unsigned is_pcm16 = 0;
	unsigned char* temp_pcm_data = tone_color->mData;
	unsigned short* loop_addr_ptr = 0;
	unsigned loop_addr = 0;
	int note_pitch = 0;
	int base_pitch = 0;
	unsigned pitch_bend_factor = 0;
	unsigned phase_value = 0;

	is_adpcm = ((unsigned short*)(tone_color->mData))[16] & 0x0040;
	is_adpcm36 = ((unsigned short*)(tone_color->mData))[16] & 0x0080;
	is_pcm16 = ((unsigned short*)(tone_color->mData))[16] & 0x0010;
	temp_pcm_data += 0x00000028; // jackson: tone color header size, temporary hard code 40 bytes

	if (is_adpcm) {
		if (is_adpcm36) {
			if (is_pcm16) {
				info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16;
			} else {
				info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8;
			}
		} else {
			if (is_pcm16) {
				info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM16;
			} else {
				info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM8;
			}
		}
	} else {
		if (is_pcm16) {
			info.mFormat = D_FORMAT_ATTACK_PCM16_LOOP_PCM16;
		} else {
			info.mFormat = D_FORMAT_ATTACK_PCM8_LOOP_PCM8;
		}
	}

	loop_addr_ptr = (unsigned short*)(tone_color->mData);
	loop_addr_ptr += 12; // specified the loop addr offset
	loop_addr = (unsigned)(tone_color->mData + (loop_addr_ptr[0] << 1) + (loop_addr_ptr[1] << 17));

	note_pitch = pitch; //gMidiChannelPitch[channel_id];
	base_pitch = (tone_color->mPitch & 0xff);
	phase_value = Audio_computePhase(base_pitch, note_pitch, ((unsigned short*)(tone_color->mData))[8]);
	gMidiChannelPhase[channel_id] = phase_value;

	if (gMidiLogicalChannelPitchBend[gMidiChannel2LogicalChannelMap[channel_id]] != D_MIDI_DEFAULT_PITCH_BEND) {
		pitch_bend_factor = Audio_getPitchBendFactor(gMidiChannel2LogicalChannelMap[channel_id]);
		phase_value = (phase_value * pitch_bend_factor) >> 14;
	}

	info.mLoopEnable = 1;
	info.mEnvAutoModeEnable = 1;
	info.mWaveAddr = temp_pcm_data;
	info.mLoopAddr = (unsigned char*)loop_addr;
	info.mEnvAddr = (unsigned char*)((unsigned)tone_color->mData + tone_color->mEnvOffset);
	info.mEnv0 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[0];
	info.mEnv1 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[1];
	info.mEnvData = 0x0;
	//info.mRampDownOffset = 0x4000;
	//info.mRampDownClk = 0x0002;
	info.mRampDownOffset = (tone_color->mEnv & 0xff00) << 1;
	info.mRampDownClk = (tone_color->mEnv & 0x00ff);
	info.mPhase = phase_value;

	Spu_setWaveEx(channel_id, &info);
}

void Audio_executeBeatCntEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned beat_duration = temp & 0x07ff;
	unsigned long_beat_duration = temp & 0x0800;

	if (long_beat_duration) {
		beat_duration |= (Audio_readMidiScore(0) & 0x07) << 11;
	}
	
	gMidiBeatCount = beat_duration;
}

void Audio_executePlayNoteEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned short channel = temp & 0x000f;
	unsigned short volume = 0; // jackson: should be ignored, use control event setting instead
	unsigned short pitch = 0; // jackson: should be ignored, use control event setting instead
	unsigned short duration = 0;
	int ins_type = 0;
	int idx = -1;
	
	temp = Audio_readMidiScore(0);
	volume = (temp & 0x007f);
	pitch = (temp & 0x7f00) >> 8;
	duration = Audio_readMidiScore(0);

	// check if instrument available
	if (channel == D_MIDI_PRECUSSION_CHANNEL) {
		//if (!gMidiDrumTable) return;
		if (!gMidiDrumTable[pitch]) {
			return;
		}
	} else {
		//if (!gMidiMelodyTable) return;
		if (!gMidiMelodyTable[gMidiLogicalChannelInstrument[channel]]) {
			return;
		}
	}

	if (!gMidiLogicalChannelEnable[channel]) return;

	idx = Audio_findEmptyMidiChannel();

	if (idx >= 0) {
		gMidiChannel2LogicalChannelMap[idx] = channel;
		gMidiChannelDuration[idx] = duration;
		gMidiChannelVol[idx] = volume;
		gMidiChannelPitch[idx] = pitch;

		if (channel == D_MIDI_PRECUSSION_CHANNEL) {
			unsigned drum = gMidiChannelPitch[idx]; // the pitch is the index of instrument
			unsigned char* data = 0;
			Instrument* instrument = 0;
			ToneColor* tone_color = 0;

			instrument = (Instrument*)gMidiDrumTable[drum];
			data = (unsigned char*)(instrument) + instrument->mDataOffset;
			data += instrument->mToneColorOffset[0];
			tone_color = (ToneColor*)data;
			
			gMidiLogicalChannelInstrument[channel] = drum;

			Audio_setDrumToneColor(idx, tone_color);
			
			gMidiChannelRampDownTick[idx] = 0;
			
		} else {
			unsigned melody = 0;
			//unsigned short* ptr = 0;
			//unsigned short* loop_addr_ptr = 0;
			//unsigned short channel_mode = 0;
			//unsigned is_pcm16 = 0;
			//unsigned is_adpcm = 0;
			//unsigned is_adpcm36 = 0;
			//int note_pitch = 0;
			//int base_pitch = 0;
			unsigned ramp_down_clk = 0;
			unsigned ramp_down_offset = 1;

			//unsigned phase = 0;
			//unsigned pitch_bend_factor = 0;

			unsigned char* data = 0;
			Instrument* instrument = 0;
			ToneColor* tone_color = 0;
			
			instrument = (Instrument*)gMidiMelodyTable[gMidiLogicalChannelInstrument[channel]];
			data = (unsigned char*)(instrument) + instrument->mDataOffset;

			for (melody = 0; melody < instrument->mToneColorCount; melody++) {
				tone_color = (ToneColor*)(data + instrument->mToneColorOffset[melody]);
				if (gMidiChannelPitch[idx] <= (tone_color->mPitch >> 8)) {
					break;
				}
			}
			
			Audio_setMelodyToneColor(idx, tone_color, gMidiChannelPitch[idx]);

			ramp_down_clk = (tone_color->mEnv & 0x00ff);
			if (ramp_down_clk <= 5) {
				gMidiChannelRampDownTick[idx] = ((int)4 << (2 * ramp_down_clk)) * 13;
			} else {
				gMidiChannelRampDownTick[idx] = ((int)8192) * 13;
			}

			ramp_down_offset = (tone_color->mEnv & 0xff00) >> 8;
			if (!ramp_down_offset) {
				ramp_down_offset = 1;
			}
			gMidiChannelRampDownTick[idx] *= (0x7f / ramp_down_offset);
			gMidiChannelRampDownTick[idx] /= D_BEATBASECNT;
		}

		if (channel == D_MIDI_PRECUSSION_CHANNEL) {
			ins_type = 1;
		} else {
			ins_type = 0;
		}
		
		Spu_setWaveVolume(idx, (gMidiVolume * gMidiLogicalChannelVol[channel] * gMidiLogicalChannelExpression[channel] * gMidiChannelVol[idx]) / 0x001f417f * gMidiInsVolume[ins_type][gMidiLogicalChannelInstrument[channel]] / 0x7f * gMidiFadeVolume / 0x7f); // 0x001f417f = 0x7f * 0x7f * 0x7f

		if (gMidiEnableGlobalPan) {
			Spu_setWavePan(idx, gMidiPan);
		} else {
			Spu_setWavePan(idx, gMidiLogicalChannelPan[channel]);
		}
	}
}

void Audio_executePitchBendEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned short channel = (temp & 0x0f00) >> 8;
	unsigned short value = (temp & 0x007f) << 7; // upper 7-bits
	unsigned phase = 0;
	unsigned pitch_bend_factor = 0;
	unsigned i = 0;
	value |= (Audio_readMidiScore(0) & 0x007f); // lower 7-bits
	gMidiLogicalChannelPitchBend[channel] = value;
	pitch_bend_factor = Audio_getPitchBendFactor(channel);

	for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
		if (gMidiChannel2LogicalChannelMap[i] == channel) {
			phase = (gMidiChannelPhase[i] * pitch_bend_factor) >> 14;
			if (phase < 0x7ffff) {
				Spu_setWavePhase(i, phase);
			}
		}
	}
}

void Audio_executeControlEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned short channel = (temp & 0x0f00) >> 8;
	unsigned short control = temp & 0x00ff;
	unsigned short value = 0;
	temp = Audio_readMidiScore(0);
	value = temp & 0x00ff;
	
	switch (control) {
	case D_CTRL_VOL:
		gMidiLogicalChannelVol[channel] = value;
		break;
	case D_CTRL_PAN:
		// jackson: disable note pan for differential amp gMidiLogicalChannelPan[channel] = value;
		break;
	case D_CTRL_EXP:
		gMidiLogicalChannelExpression[channel] = value;
		break;
	case D_CTRL_RPN_LSB: // jackson: should be 7 bits instead of 8 bits?
		gMidiLogicalChannelRPN[channel] &= 0xff00;
		gMidiLogicalChannelRPN[channel] |= value;
		break;
	case D_CTRL_RPN_MSB: // jackson: should be 7 bits instead of 8 bits?
		gMidiLogicalChannelRPN[channel] &= 0x00ff;
		gMidiLogicalChannelRPN[channel] |= (value << 8);
		break;
	case D_CTRL_SUBBANK:
		gMidiLogicalChannelSubBand[channel] = value;
		break;
	case D_CTRL_DATA_ENTRY_LSB: // jackson: should be 7 bits instead of 8 bits?
		// used for setting pitch sensitivity
		// MSB value cannot great than 24 (number of pitch blend table)
		// default sensitivity table = 2
		gMidiLogicalChannelDataEntry[channel] &= 0xff00;
		gMidiLogicalChannelDataEntry[channel] |= value;
		break;
	case D_CTRL_DATA_ENTRY_MSB: // jackson: should be 7 bits instead of 8 bits?
		// used for setting pitch sensitivity
		// MSB value cannot great than 24 (number of pitch blend table)
		// default sensitivity table = 2
		gMidiLogicalChannelDataEntry[channel] &= 0x00ff;
		gMidiLogicalChannelDataEntry[channel] |= (value << 8);
		break;
	default:
		//gMidiDataReady = 0; // report for unknown control event

		// do nothing for unknown control event
		break;
	}
	
}

void Audio_executeProgChangeEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned channel = (temp & 0x0f00) >> 8;
	unsigned instrument = temp & 0x00ff;

	gMidiLogicalChannelInstrument[channel] = instrument;
}

void Audio_executeTempoChangeEvent(unsigned short command) {
	// jackson: since Sun Midiar already computed the time, change of tempo means nothing to the playback
	// jackson: just keep the tempo for record
	unsigned short temp = command;
	gMidiLogicalChannelTempo = temp & 0x00ff;
}

void Audio_executeEndSeqEvent(unsigned short command) {
	//unsigned i = 0;
	if (gMidiEnableLoop) {
		if (Audio_isMidiFade()) {
			gMidiFadeStartTime -= gMidiTick;
			gMidiFadeEndTime -= gMidiTick;
		}
		Audio_resetMidiScore();
	} else {
		gMidiDataReady = 0;
	}
}

void Audio_executeLyricEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned short channel = (temp & 0x0f00) >> 8;
	unsigned size = temp & 0x00ff;
	unsigned short duration = Audio_readMidiScore(0);
	//unsigned i = 0;

	Audio_readMidiScoreN(gMidiTextBuffer, size * sizeof(unsigned short), 0);

	if (gMidiLyricEventHandler) {
		(*gMidiLyricEventHandler)(channel, size * sizeof(unsigned short), (const char*)gMidiTextBuffer);
	}
}

void Audio_executeTextEvent(unsigned short command) {
	unsigned short temp = command;
	unsigned size = temp & 0x00ff;
	//unsigned i = 0;

	Audio_readMidiScoreN(gMidiTextBuffer, size * sizeof(unsigned short), 0);
}

void Audio_executeMidiEvent(void) {
	
	while (1) {
		int error_code = 0;
		unsigned short command = Audio_readMidiScore(&error_code);

		if (error_code) {
			gMidiDataReady = 0;
			gMidiStatus = AUDIO_PLAY_ERROR;
			return; // leave parse loop
		}

		switch (command & 0xf000) {
		case D_MIDI_CMD_PLAYNOTE:
			Audio_executePlayNoteEvent(command);
			break;
		case D_MIDI_CMD_BEATCNT:
			Audio_executeBeatCntEvent(command);
			return; // leave parse loop
		case D_MIDI_CMD_PITCHBEND:
			Audio_executePitchBendEvent(command);
			break;
		case D_MIDI_CMD_CONTROL:
			Audio_executeControlEvent(command);
			if (!gMidiDataReady) {
				gMidiStatus = AUDIO_PLAY_ERROR;
				return; // leave parse loop
			}
			break;
		case D_MIDI_CMD_PROGCHANGE:
			Audio_executeProgChangeEvent(command);
			break;
		case D_MIDI_CMD_TEMPOCHANGE:
			Audio_executeTempoChangeEvent(command);
			break;
		case D_MIDI_CMD_ENDSEQ:
			Audio_executeEndSeqEvent(command);
			return; // leave parse loop
		case D_MIDI_CMD_LYRIC:
			Audio_executeLyricEvent(command);
			break;
		case D_MIDI_CMD_TEXT:
			Audio_executeTextEvent(command);
			break;
		default:
			// while (1); // jackson: block the execution for unknown event
			gMidiDataReady = 0;
			gMidiStatus = AUDIO_PLAY_ERROR;
			return; // leave parse loop
		}
	}
}


//
// driver functions
//

void Audio_init(unsigned total_channel_count, unsigned midi_channel_count, int stream_count) {
	DEBUG_PRINT(KERN_WARNING "{TRACE %s(%d, %d, %d)}\n", __FUNCTION__, total_channel_count, midi_channel_count, stream_count);
	Spu_init();
	if (total_channel_count > 32) {
		total_channel_count = 32;
	}

	gAudioTotalChannel = total_channel_count;
	
	if (midi_channel_count <= gAudioTotalChannel) {
		gMidiStartChannel = gAudioTotalChannel - midi_channel_count;
	} else {
		gMidiStartChannel = 0;
	}
	
	//gMidiMelodyTable = 0;
	//gMidiDrumTable = 0;
	
	Spu_setBeatCntHandler(&Audio_handleBeatCounter);

	Audio_initStreams(stream_count);
}
EXPORT_SYMBOL(Audio_init);

void Audio_cleanup() {
	Audio_cleanupStreams();
	Audio_cleanupMidi();
	Spu_cleanup();
}

unsigned Audio_getMasterVolume() {
	return Spu_getMasterVolume();
}

void Audio_setMasterVolume(unsigned vol) {
	Spu_setMasterVolume(vol);	
}

void Audio_setWave(int channel_id, unsigned char* pcm_data) {
DEBUG_PRINT(KERN_WARNING "TRACE line %d: %s(%d, 0x%08X)\n", __LINE__, __FUNCTION__, channel_id, (unsigned int)pcm_data);
	if (channel_id >= Audio_getMaxWaveChannel()) return;
DEBUG_PRINT(KERN_WARNING "TRACE line %d: %s(%d, 0x%08X)\n", __LINE__, __FUNCTION__, channel_id, (unsigned int)pcm_data);
	Spu_setWave(channel_id, pcm_data);
}

void Audio_setDrumNote(int channel_id, int drum_idx) {
	unsigned drum = drum_idx;
	unsigned char* data = 0;
	Instrument* instrument = 0;
	ToneColor* tone_color = 0;
	unsigned is_adpcm = 0;
	unsigned is_adpcm36 = 0;
	unsigned is_pcm16 = 0;
	unsigned char* temp_pcm_data = 0;
	WaveInfo info;

	if (channel_id >= Audio_getMaxWaveChannel()) return;
	//if (!gMidiDrumTable) return;
	if (!gMidiDrumTable[drum_idx]) return;

	instrument = (Instrument*)gMidiDrumTable[drum];
	data = (unsigned char*)(instrument) + instrument->mDataOffset;
	data += instrument->mToneColorOffset[0];
	tone_color = (ToneColor*)data;

	temp_pcm_data = tone_color->mData;

	is_adpcm = ((unsigned short*)(tone_color->mData))[16] & 0x0040;
	is_adpcm36 = ((unsigned short*)(tone_color->mData))[16] & 0x0080;
	is_pcm16 = ((unsigned short*)(tone_color->mData))[16] & 0x0010;
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
	info.mEnvAutoModeEnable = 1;
	info.mWaveAddr = temp_pcm_data;
	info.mLoopAddr = temp_pcm_data;
	info.mEnvAddr = (unsigned char*)((unsigned)tone_color->mData + tone_color->mEnvOffset);
	info.mEnv0 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[0];
	info.mEnv1 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[1];
	info.mEnvData = 0x7f;
	info.mRampDownOffset = 0x4000;
	info.mRampDownClk = 0x0002;
	info.mPhase = SAMPLERATE_2_PHASE(((unsigned short*)(tone_color->mData))[8]);

	Spu_setWaveEx(channel_id, &info);
}

void Audio_setMelodyNote(int channel_id, int melody_idx, int pitch, int keep_on) {

	unsigned melody = 0;
	unsigned char* data = 0;
	Instrument* instrument = 0;
	ToneColor* tone_color = 0;
	unsigned is_pcm16 = 0;
	unsigned is_adpcm = 0;
	unsigned is_adpcm36 = 0;
	unsigned char* temp_pcm_data = 0;
	unsigned short* loop_addr_ptr = 0;
	unsigned loop_addr = 0;
	int note_pitch = 0;
	int base_pitch = 0;
	unsigned phase_value = 0;
	WaveInfo info;

	if (channel_id >= Audio_getMaxWaveChannel()) return;
	//if (!gMidiMelodyTable) return;
	if (!gMidiMelodyTable[melody_idx]) return;

	instrument = (Instrument*)gMidiMelodyTable[melody_idx];
	data = (unsigned char*)(instrument) + instrument->mDataOffset;

	for (melody = 0; melody < instrument->mToneColorCount; melody++) {
		tone_color = (ToneColor*)(data + instrument->mToneColorOffset[melody]);
		if (pitch <= (tone_color->mPitch >> 8)) {
			break;
		}
	}
			
	temp_pcm_data = tone_color->mData;

	is_adpcm = ((unsigned short*)(tone_color->mData))[16] & 0x0040;
	is_adpcm36 = ((unsigned short*)(tone_color->mData))[16] & 0x0080;
	is_pcm16 = ((unsigned short*)(tone_color->mData))[16] & 0x0010;
	temp_pcm_data += 0x00000028; // jackson: tone color header size, temporary hard code 40 bytes

	if (is_adpcm) {
		if (is_adpcm36) {
			if (is_pcm16) {
				info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_PCM16;
			} else {
				info.mFormat = D_FORMAT_ATTACK_ADPCM36_LOOP_PCM8;
			}
		} else {
			if (is_pcm16) {
				info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM16;
			} else {
				info.mFormat = D_FORMAT_ATTACK_ADPCM_LOOP_PCM8;
			}
		}
	} else {
		if (is_pcm16) {
			info.mFormat = D_FORMAT_ATTACK_PCM16_LOOP_PCM16;
		} else {
			info.mFormat = D_FORMAT_ATTACK_PCM8_LOOP_PCM8;
		}
	}

	loop_addr_ptr = (unsigned short*)(tone_color->mData);
	loop_addr_ptr += 12; // specified the loop addr offset
	loop_addr = (unsigned)(tone_color->mData + (loop_addr_ptr[0] << 1) + (loop_addr_ptr[1] << 17));

	note_pitch = pitch;
	base_pitch = (tone_color->mPitch & 0xff);
	phase_value = Audio_computePhase(base_pitch, note_pitch, ((unsigned short*)(tone_color->mData))[8]);

	info.mLoopEnable = 1;
	info.mWaveAddr = temp_pcm_data;
	info.mLoopAddr = (unsigned char*)loop_addr;
	if (keep_on) {
		info.mEnvAutoModeEnable = 0;
		info.mEnvData = 0x7f;
	} else {
		info.mEnvAutoModeEnable = 1;
		info.mEnvData = 0x0;
	}
	info.mEnvAddr = (unsigned char*)((unsigned)tone_color->mData + tone_color->mEnvOffset);
	info.mEnv0 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[0];
	info.mEnv1 = ((unsigned short*)((unsigned)tone_color->mData + tone_color->mEnvOffset))[1];
	info.mRampDownOffset = (tone_color->mEnv & 0xff00) << 1;
	info.mRampDownClk = (tone_color->mEnv & 0x00ff);
	info.mPhase = phase_value;

	Spu_setWaveEx(channel_id, &info);
}

void Audio_releaseMelodyNote(int channel_id, int fast_ramp_down) {

	if (channel_id >= Audio_getMaxWaveChannel()) return;
	if (!Spu_isWavePlaying(channel_id)) return;
	
	if (fast_ramp_down) {
		Spu_setChannelControlReg(channel_id, D_SPU_CH_ENVELOPLoop, 0x4000);
		Spu_setChannelPhaseReg(channel_id, D_SPU_CH_RAMP_DOWN_CLK, 0x0002);
	}

	if (channel_id < 16) {
		Spu_setControlRegL(D_SPU_CH_ENV_MODE_SEL, Spu_getControlRegL(D_SPU_CH_ENV_MODE_SEL) & ~Spu_getChannelMaskWord(channel_id));
		Spu_setControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE, Spu_getControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE) | Spu_getChannelMaskWord(channel_id));
	} else {
		Spu_setControlRegH(D_SPU_CH_ENV_MODE_SEL, Spu_getControlRegH(D_SPU_CH_ENV_MODE_SEL) & ~(Spu_getChannelMaskWord(channel_id) >> 16));
		Spu_setControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE, Spu_getControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE) | (Spu_getChannelMaskWord(channel_id) >> 16));
	}
}

void Audio_resetWave(int channel_id) {
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	Spu_resetWave(channel_id);
}

void Audio_pauseWave(int channel_id) {
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	Spu_pauseWave(channel_id);
}

void Audio_resumeWave(int channel_id) {
DEBUG_PRINT(KERN_WARNING "TRACE %s(%d)\n", __FUNCTION__, channel_id);
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	Spu_resumeWave(channel_id);
}

AudioPlayStatus Audio_getWaveStatus(int channel_id) {
	if (Spu_isWavePlaying(channel_id)) return AUDIO_PLAY_BUSY;
	
	if (Spu_isWavePause(channel_id)) return AUDIO_PLAY_PAUSE;
	
	return AUDIO_PLAY_STOP;
}

int Audio_isWaveLoopEnable(int channel_id) {
	return (Spu_getChannelControlReg(channel_id, D_SPU_CH_MODE) & 0x2000) != 0;
}

void Audio_enableWaveLoop(int channel_id, int enable) {
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	Spu_enableWaveLoop(channel_id, enable);
}

int Audio_getWaveVolume(int channel_id) {
	return Spu_getWaveVolume(channel_id);
}

void Audio_setWaveVolume(int channel_id, int vol) {
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	Spu_setWaveVolume(channel_id, vol);
}

int Audio_getWavePan(int channel_id) {
	return Spu_getWavePan(channel_id);
}

void Audio_setWavePan(int channel_id, int pan) {
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	// jackson: disable wave pan for differential amp Spu_setWavePan(channel_id, pan);
}

unsigned short Audio_getWavePlaybackSpeed(int channel_id) {
	return Spu_getWavePlaybackSpeed(channel_id);
}

void Audio_setWavePlaybackSpeed(int channel_id, unsigned short speed) {
	if (channel_id >= Audio_getMaxWaveChannel()) return;
	
	Spu_setWavePlaybackSpeed(channel_id, speed);
}

unsigned Audio_getWaveCurrentPlayTime(int channel_id) {
	if (channel_id >= Audio_getMaxWaveChannel()) return 0;

	return Spu_getWaveCurrentPlayTime(channel_id);
}

void Audio_initMidi(unsigned char* melody_table, unsigned char* drum_table, unsigned max_update_period_in_ms) {
	//unsigned* temp = 0;
	unsigned i = 0, j = 0;
	unsigned** mt = (unsigned**)melody_table;
	unsigned** dt = (unsigned**)drum_table;

	gMidiStatus = AUDIO_PLAY_STOP;

	//gMidiMelodyTable = 0;
	//gMidiDrumTable = 0;

	gMidiScoreBase = 0;
	gMidiScore = 0;
	gMidiScoreSize = 0;
	gMidiTick = 0;
	gMidiNextTick = 0;
	gMidiStream = 0;
	gMidiVolume = 0x7f;
	gMidiPan = 0x40;
	gMidiEnableGlobalPan = 0;
	gMidiEnableLoop = 0;
	gMidiPlaybackSpeed = AUDIO_X1_MIDI_PLAYBACK_SPEED;
	gMidiFadeVolume = 0x7f;
	gMidiFadeStartTime = 0;
	gMidiFadeEndTime = 0;
	gMidiFadeDirection = 0;

	gMidiDataReady = 0;
	gMidiPauseCommit = 0;
	gMidiResumeCommit = 0;
	gMidiBeatCount = 0;
	gMidiLastSelectChannel = gMidiStartChannel;
	gMidiKeyOnMaskLH = 0;
	
	gMidiLyricEventHandler = 0;

	for (i = 0; i < D_MIDI_TOTAL_LOGICAL_CHANNEL; i++) {
		gMidiLogicalChannelInstrument[i] = 0;
		gMidiLogicalChannelPitchBend[i] = D_MIDI_DEFAULT_PITCH_BEND;
		gMidiLogicalChannelVol[i] = 0x7f;
		gMidiLogicalChannelPan[i] = 0x40;
		gMidiLogicalChannelExpression[i] = 0x7f;
		gMidiLogicalChannelRPN[i] = 0;
		gMidiLogicalChannelSubBand[i] = 0;
		gMidiLogicalChannelDataEntry[i] = 0;
		gMidiLogicalChannelEnable[i] = 1;
	}
	gMidiLogicalChannelTempo = 0;

	for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
		gMidiChannel2LogicalChannelMap[i] = 0;
		gMidiChannelDuration[i] = 0;
		gMidiChannelVol[i] = 0;
		gMidiChannelPitch[i] = 0;
		gMidiChannelPhase[i] = 0;
		gMidiChannelRampDownTick[i] = 0;
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < D_MIDI_MAX_INSTRUMENT_COUNT; j++) {
			gMidiInsVolume[i][j] = 0x7f;
			gMidiInsVolumeChange[i][j] = 0;
		}
	}

	Spu_stopWaves(gMidiStartChannel, gAudioTotalChannel - 1);

	gMidiMaxBeatCount = max_update_period_in_ms / (D_BEATCNTPERIOD / 1000);
	if (gMidiMaxBeatCount == 0) {
		gMidiMaxBeatCount = 1;
	}
	gMidiOldNextBeatCount = gMidiMaxBeatCount;
	gMidiNextBeatCount = gMidiMaxBeatCount;
	
	for (i = 0; i < D_MIDI_MAX_INSTRUMENT_COUNT; i++) {
		if (mt[i]) {
			gMidiMelodyTable[i] = (unsigned*)USER_VA_TO_KERNEL_VA((unsigned)mt[i]);
		} else {
			gMidiMelodyTable[i] = 0;
		}
		if (dt[i]) {
			gMidiDrumTable[i] = (unsigned*)USER_VA_TO_KERNEL_VA((unsigned)dt[i]);
		} else {
			gMidiDrumTable[i] = 0;
		}
	}

	// start SPU beat counter
	Spu_setControlRegL(D_SPU_BEATBASECNT, D_BEATBASECNT * AUDIO_X1_MIDI_PLAYBACK_SPEED / gMidiPlaybackSpeed);
	Spu_setControlRegL(D_SPU_BEATCNT, 0xc000 | gMidiNextBeatCount);
}

void Audio_cleanupMidi() {
	// stop SPU beat counter
	gMidiStatus = AUDIO_PLAY_STOP;
	Spu_stopWaves(gMidiStartChannel, gAudioTotalChannel - 1);

	Spu_setControlRegL(D_SPU_BEATCNT, 0x4000);
}

void Audio_setMidi(unsigned char* midi_data, unsigned midi_data_size) {
	unsigned i;
	
	gMidiStatus = AUDIO_PLAY_STOP;

	Spu_stopWaves(gMidiStartChannel, gAudioTotalChannel - 1);
	
	for (i = 0; i < D_MIDI_TOTAL_LOGICAL_CHANNEL; i++) {
		gMidiLogicalChannelInstrument[i] = 0;
		gMidiLogicalChannelPitchBend[i] = D_MIDI_DEFAULT_PITCH_BEND;
		gMidiLogicalChannelVol[i] = 0x7f;
		gMidiLogicalChannelPan[i] = 0x40;
		gMidiLogicalChannelExpression[i] = 0x7f;
		gMidiLogicalChannelRPN[i] = 0;
		gMidiLogicalChannelSubBand[i] = 0;
		gMidiLogicalChannelDataEntry[i] = 0;
	}
	gMidiLogicalChannelTempo = 0;

	for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
		gMidiChannel2LogicalChannelMap[i] = 0;
		gMidiChannelDuration[i] = 0;
		gMidiChannelVol[i] = 0;
		gMidiChannelPitch[i] = 0;
		gMidiChannelPhase[i] = 0;
		gMidiChannelRampDownTick[i] = 0;
	}
	
	gMidiScoreBase = midi_data;
	gMidiScore = gMidiScoreBase;
	gMidiScoreSize = midi_data_size;
	gMidiTick = 0;
	gMidiNextTick = 0;
	gMidiStream = 0;
	gMidiBeatCount = 0;
	gMidiDataReady = 1;
	gMidiLastSelectChannel = gMidiStartChannel;
	gMidiKeyOnMaskLH = 0;
	gMidiFadeVolume = 0x7f;
	gMidiFadeStartTime = 0;
	gMidiFadeEndTime = 0;
	gMidiFadeDirection = 0;

	gMidiPauseCommit = 1;
	gMidiResumeCommit = 1;
	gMidiStatus = AUDIO_PLAY_PAUSE;
DEBUG_PRINT(KERN_WARNING "%s(): gMidiStatus=%d\n", __FUNCTION__, gMidiStatus);
}

void Audio_setMidiStream(AudioMidiStream* midi_stream) {
	Audio_setMidi(0, 0);

	gMidiStream = midi_stream;
}

void Audio_resetMidi() {
	unsigned i = 0;

	gMidiStatus = AUDIO_PLAY_STOP;

	Spu_stopWaves(gMidiStartChannel, gAudioTotalChannel - 1);

	for (i = 0; i < D_MIDI_TOTAL_LOGICAL_CHANNEL; i++) {
		gMidiLogicalChannelInstrument[i] = 0;
		gMidiLogicalChannelPitchBend[i] = D_MIDI_DEFAULT_PITCH_BEND;
		gMidiLogicalChannelVol[i] = 0x7f;
		gMidiLogicalChannelPan[i] = 0x40;
		gMidiLogicalChannelExpression[i] = 0x7f;
		gMidiLogicalChannelRPN[i] = 0;
		gMidiLogicalChannelSubBand[i] = 0;
		gMidiLogicalChannelDataEntry[i] = 0;
	}
	gMidiLogicalChannelTempo = 0;

	for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
		gMidiChannel2LogicalChannelMap[i] = 0;
		gMidiChannelDuration[i] = 0;
		gMidiChannelVol[i] = 0;
		gMidiChannelPitch[i] = 0;
		gMidiChannelPhase[i] = 0;
		gMidiChannelRampDownTick[i] = 0;
	}

	Audio_resetMidiScore();
	gMidiBeatCount = 0;
	gMidiDataReady = 1;
	gMidiLastSelectChannel = gMidiStartChannel;
	gMidiKeyOnMaskLH = 0;
	gMidiFadeVolume = 0x7f;
	gMidiFadeStartTime = 0;
	gMidiFadeEndTime = 0;
	gMidiFadeDirection = 0;

	gMidiPauseCommit = 1;
	gMidiResumeCommit = 1;
	gMidiStatus = AUDIO_PLAY_PAUSE;
}

void Audio_pauseMidi() {

	if (Audio_getMidiStatus() == AUDIO_PLAY_PAUSE) return;
	
	if (Audio_getMidiStatus() == AUDIO_PLAY_BUSY) {
	gMidiPauseCommit = 0;
	gMidiStatus = AUDIO_PLAY_PAUSE;
	}
}

void Audio_resumeMidi() {

DEBUG_PRINT(KERN_WARNING "%s(): gMidiStatus=%d\n", __FUNCTION__, gMidiStatus);
	if (Audio_getMidiStatus() == AUDIO_PLAY_BUSY) return;

	if (Audio_getMidiStatus() == AUDIO_PLAY_PAUSE) {
		gMidiResumeCommit = 0;
		gMidiStatus = AUDIO_PLAY_BUSY;
	}
	/*
	else {
		gMidiStatus = AUDIO_PLAY_BUSY;
	}
	*/
}

AudioPlayStatus Audio_getMidiStatus() {
	return gMidiStatus;
}

int Audio_isMidiLoopEnable() {
	return gMidiEnableLoop;
}

void Audio_enableMidiLoop(int loop) {
	gMidiEnableLoop = loop;
}

int Audio_getMidiVolume() {
	return gMidiVolume;
}

int Audio_getMidiGlobalPan() {
	return gMidiPan;
}

void Audio_setMidiVolume(int vol) {
	//printk(KERN_WARNING "%s(): vol=%d\n", __FUNCTION__, vol);
	gMidiVolume = vol;
}

void Audio_setMidiGlobalPan(int pan) {
	// jackson: disable MIDI global pan for differential amp gMidiPan = pan;
}

void Audio_enableMidiGlobalPan(int enable) {
	gMidiEnableGlobalPan = enable;
}

int Audio_getMidiInstrumentVolume(int ins_type, int ins_idx) {
	if (ins_idx >= D_MIDI_MAX_INSTRUMENT_COUNT) return 0;
	if (ins_type > 2) return 0;

	return gMidiInsVolume[ins_type][ins_idx];
}

void Audio_setMidiInstrumentVolume(int ins_type, int ins_idx, int vol) {
	if (ins_idx >= D_MIDI_MAX_INSTRUMENT_COUNT) return;
	if (ins_type > 2) return;

	gMidiInsVolume[ins_type][ins_idx] = vol & 0x7f;
	gMidiInsVolumeChange[ins_type][ins_idx] = 1;
}

unsigned Audio_getMidiCurrentPlayTime() {
	return gMidiTick * (D_BEATCNTPERIOD / 1000);
}

unsigned short Audio_getMidiPlaybackSpeed() {
	return gMidiPlaybackSpeed;
}

void Audio_setMidiPlaybackSpeed(unsigned short speed) {
	unsigned beat_cnt_irq = Spu_getControlRegL(D_SPU_BEATCNT) & 0x8000;

	if (beat_cnt_irq) {
		Spu_setControlRegL(D_SPU_BEATCNT, 0x4000); // disable beat count interrupt
	}

	if (speed > AUDIO_MAX_MIDI_PLAYBACK_SPEED) {
		speed = AUDIO_MAX_MIDI_PLAYBACK_SPEED;
	} else if (speed < AUDIO_MIN_MIDI_PLAYBACK_SPEED) {
		speed = AUDIO_MIN_MIDI_PLAYBACK_SPEED; // lowest speed
	}

	gMidiPlaybackSpeed = speed;
	Spu_setControlRegL(D_SPU_BEATBASECNT, D_BEATBASECNT * AUDIO_X1_MIDI_PLAYBACK_SPEED / gMidiPlaybackSpeed);

	if (beat_cnt_irq) {
		Spu_setControlRegL(D_SPU_BEATCNT, 0xc001); // enable beat count interrupt
	}
}

void Audio_fadeMidi(int fade_direction, int fade_duration) {
	gMidiFadeStartTime = gMidiTick;
	gMidiFadeEndTime = gMidiFadeStartTime + fade_duration / (D_BEATCNTPERIOD / 1000);

	if (fade_direction > 0) {
		gMidiFadeDirection = 1;
	} else if (fade_direction < 0) {
		gMidiFadeDirection = -1;
	} else {
		gMidiFadeDirection = 0;
		gMidiFadeVolume = 0x7f;
	}
}

int Audio_isMidiFade() {
	return (gMidiFadeDirection != 0);
}

void Audio_setMidiLyricEventHandler(AudioMidiLyricEventHandler handler) {
	gMidiLyricEventHandler = handler;
}

// jackson: for testing beat counter interrupt handler performance
/*
#define  SFTBASE 0x11200000
#define  P_SFT_GPIO_IOAConfig ((volatile unsigned int *) (SFTBASE + 0x1000))
#define  P_SFT_SOFT_SEL00 ((volatile unsigned int *) (SFTBASE + 0x0000))
#define  P_SFT_SOFT_SEL04 ((volatile unsigned int *) (SFTBASE + 0x0004))
*/

void Audio_handleBeatCounter() {
	unsigned i = 0;
	unsigned keyoff_maskLH = 0;
	//unsigned keyon_maskLH = 0;
	unsigned play_maskLH = 0;
	unsigned midi_maskLH = 0;
	unsigned temp = 0;
	int temp_beat_cnt = 0;
	//unsigned midi_end = 0;

	int midi_fade_time = 0;

TRACE_LINE

	// jackson: for testing beat counter interrupt handler performance
	/*
	*P_SFT_GPIO_IOAConfig= 0x00; //Config IOA4
	*P_SFT_SOFT_SEL00 |=0x10; //Enable output
	*P_SFT_SOFT_SEL04 |= 0x10; //output HIGH
	*/

	// disable beat counter interrupt
	Spu_setControlRegL(D_SPU_BEATCNT, 0x4000);
	
	if (gMidiStatus == AUDIO_PLAY_STOP || gMidiStatus == AUDIO_PLAY_ERROR) {
TRACE_LINE
		gMidiOldNextBeatCount = gMidiMaxBeatCount;
		gMidiNextBeatCount = gMidiMaxBeatCount;
	} else if (gMidiStatus == AUDIO_PLAY_PAUSE) {
TRACE_LINE

		if (!gMidiPauseCommit) {
TRACE_LINE
			Spu_pauseWaves(gMidiStartChannel, gAudioTotalChannel - 1);
			gMidiOldNextBeatCount = gMidiNextBeatCount;
			gMidiNextBeatCount = gMidiMaxBeatCount;
			gMidiPauseCommit = 1;
		}
		
		// clear beat counter interrupt status and enable interrupt
		Spu_setControlRegL(D_SPU_BEATCNT, 0xc000 | gMidiMaxBeatCount);
		
	} else if (gMidiStatus == AUDIO_PLAY_BUSY) {
TRACE_LINE
		if (!gMidiDataReady) {
TRACE_LINE
		
			for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
				if (Spu_isWavePlaying(i)) {
					break;
				}
			}
TRACE_LINE

			if (i == gAudioTotalChannel) {			
				gMidiStatus = AUDIO_PLAY_STOP;
				gMidiNextBeatCount = 1;
			} else {
				gMidiNextBeatCount = gMidiMaxBeatCount;
			}
TRACE_LINE

		} else {

			if (!gMidiResumeCommit) {
				Spu_resumeWaves(gMidiStartChannel, gAudioTotalChannel - 1);
				gMidiResumeCommit = 1;
			}
			
TRACE_LINE
			gMidiTick = gMidiNextTick;
			temp_beat_cnt = gMidiNextBeatCount;
			gMidiNextBeatCount = gMidiMaxBeatCount;
TRACE_LINE

			// update fading effect if fading enabled
			if (Audio_isMidiFade()) {
TRACE_LINE
				midi_fade_time = gMidiTick;
				if (midi_fade_time >= gMidiFadeEndTime) {
TRACE_LINE
					gMidiFadeVolume = 0x7f;
				} else if (midi_fade_time <= gMidiFadeStartTime) {
TRACE_LINE
					gMidiFadeVolume = 0;
				} else {
TRACE_LINE
					gMidiFadeVolume = 0x7f * (midi_fade_time - gMidiFadeStartTime) / (gMidiFadeEndTime - gMidiFadeStartTime);
				}

				if (gMidiFadeDirection == -1) {
TRACE_LINE
					gMidiFadeVolume = 0x7f - gMidiFadeVolume;
				}

				for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
TRACE_LINE
					if (gMidiChannelDuration[i] > 0) {
						unsigned fade_ch = gMidiChannel2LogicalChannelMap[i];
						unsigned fade_ins_type = 0;
						if (fade_ch == D_MIDI_PRECUSSION_CHANNEL) {
							fade_ins_type = 1;
						}
						Spu_setWaveVolume(i, (gMidiVolume * gMidiLogicalChannelVol[fade_ch] * gMidiLogicalChannelExpression[fade_ch] * gMidiChannelVol[i]) / 0x001f417f * gMidiInsVolume[fade_ins_type][gMidiLogicalChannelInstrument[fade_ch]] / 0x7f * gMidiFadeVolume / 0x7f); // 0x001f417f = 0x7f * 0x7f * 0x7f
					}
				}

TRACE_LINE
				if (midi_fade_time >= gMidiFadeEndTime) {
					gMidiFadeDirection = 0;
				}
TRACE_LINE
			} else {
				// check for instrument volume change
				int m = 0, n = 0;
				for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
					if (gMidiChannelDuration[i] > 0) {
						unsigned ch = gMidiChannel2LogicalChannelMap[i];
						unsigned ins_type = 0;
						if (ch == D_MIDI_PRECUSSION_CHANNEL) {
							ins_type = 1;
						}
						if (gMidiInsVolumeChange[ins_type][gMidiLogicalChannelInstrument[ch]]) {
							Spu_setWaveVolume(i, (gMidiVolume * gMidiLogicalChannelVol[ch] * gMidiLogicalChannelExpression[ch] * gMidiChannelVol[i]) / 0x001f417f * gMidiInsVolume[ins_type][gMidiLogicalChannelInstrument[ch]] / 0x7f); // 0x001f417f = 0x7f * 0x7f * 0x7f
						}
					}
				}
				for (m = 0; m < 2; m++) {
					for (n = 0; n < D_MIDI_MAX_INSTRUMENT_COUNT; n++) {
						gMidiInsVolumeChange[m][n] = 0;
					}
				}
			}

			// update MIDI channel status
			keyoff_maskLH = 0;
			for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
TRACE_LINE
				if (gMidiChannel2LogicalChannelMap[i] != D_MIDI_PRECUSSION_CHANNEL) {
					if (gMidiChannelDuration[i] > 0) {
						gMidiChannelDuration[i] -= temp_beat_cnt;
						if (gMidiChannelDuration[i] <= 0) {							

							gMidiChannelDuration[i] = 0;
							
							keyoff_maskLH |= Spu_getChannelMaskWord(i);
						}
					} else if (Spu_isWavePlaying(i)) {
						//keyoff_maskLH |= Spu_getChannelMaskWord(i);

						if (gMidiChannelRampDownTick[i] > 0) {
							gMidiChannelRampDownTick[i] -= temp_beat_cnt;
							if (gMidiChannelRampDownTick[i] <= 0) {
								if (Spu_getWaveVolume(i) > 0) {
									Spu_setWaveVolume(i, Spu_getWaveVolume(i) / 5);
								} else {
									gMidiChannelRampDownTick[i] = 0;
									Spu_resetWave(i);
									Spu_stopWave(i);
								}
							}
						} else {
							if (Spu_getWaveVolume(i) > 0) {
								Spu_setWaveVolume(i, Spu_getWaveVolume(i) / 5);
							} else {
								gMidiChannelRampDownTick[i] = 0;
								Spu_resetWave(i);
								Spu_stopWave(i);
							}
						}
					}
				} else {
					if (!Spu_isWavePlaying(i) && gMidiChannelDuration[i] > 0) {
						gMidiChannelDuration[i] = 0;
					}
				}
			}

TRACE_LINE
			if (keyoff_maskLH & 0x0000ffff) {
				//Spu_setControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE, Spu_getControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE) | (keyoff_maskLH & 0x0000ffff));
				Spu_setControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE, (keyoff_maskLH & 0x0000ffff));
			}
TRACE_LINE
			if (keyoff_maskLH >> 16) {
				//Spu_setControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE, Spu_getControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE) | (keyoff_maskLH >> 16));
				Spu_setControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE, (keyoff_maskLH >> 16));
			}

TRACE_LINE
			if (gMidiBeatCount > 0) {
				gMidiBeatCount -= temp_beat_cnt;
			}
		
TRACE_LINE
			gMidiKeyOnMaskLH = 0;
			if (gMidiBeatCount <= 0) {
TRACE_LINE
				Audio_executeMidiEvent();
			}
			
TRACE_LINE
			if (!gMidiDataReady) {
TRACE_LINE
				play_maskLH = Spu_getControlRegLH(D_SPU_CH_STATUS);
				Spu_getChannelMaskWordByRange(gMidiStartChannel, gAudioTotalChannel - 1, &midi_maskLH);
				if (midi_maskLH & 0xffff) {
					temp = (midi_maskLH & 0xffff) & (play_maskLH & 0xffff);
					//Spu_setControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE, Spu_getControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE) | temp);
					Spu_setControlRegL(D_SPU_CH_ENV_RAMPDOWN_ENABLE, temp);
					Spu_setControlRegL(D_SPU_CH_TONERELEASE_ENABLE, temp);
				}
				if (midi_maskLH >> 16) {
					temp = (midi_maskLH >> 16) & (play_maskLH >> 16);
					//Spu_setControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE, Spu_getControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE) | temp);
					Spu_setControlRegH(D_SPU_CH_ENV_RAMPDOWN_ENABLE, temp);
					Spu_setControlRegH(D_SPU_CH_TONERELEASE_ENABLE, temp);
				}

				for (i = gMidiStartChannel; i <gAudioTotalChannel; i++) {
					temp = play_maskLH & Spu_getChannelMaskWord(i);
					
					if (temp) {
				 		Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP0, 0);
						Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP1, 0);
						Spu_setChannelControlReg(i, D_SPU_CH_ENVELOP_DATA, 0);
						Spu_setChannelControlReg(i, D_SPU_CH_WAVEDATA, 0x8000);
					}
				}

				gMidiNextBeatCount = gMidiMaxBeatCount;
			} else {
TRACE_LINE
				for (i = gMidiStartChannel; i < gAudioTotalChannel; i++) {
					if (!Spu_isWaveStop(i) && (gMidiChannel2LogicalChannelMap[i] != D_MIDI_PRECUSSION_CHANNEL)) {
						if ((gMidiChannelDuration[i] > 0) && (gMidiChannelDuration[i] < gMidiNextBeatCount)) {
							gMidiNextBeatCount = gMidiChannelDuration[i];
						}
					}
				}
				if ((gMidiBeatCount > 0) && (gMidiBeatCount < gMidiNextBeatCount)) {
					gMidiNextBeatCount = gMidiBeatCount;
				}
				gMidiNextTick += gMidiNextBeatCount;

				//play midi for this interval
				if (gMidiKeyOnMaskLH & 0x0000ffff) {
					Spu_setControlRegL(D_SPU_CH_STOPSTATUS, gMidiKeyOnMaskLH & 0x0000ffff);
				}
				if (gMidiKeyOnMaskLH >> 16) {
					Spu_setControlRegH(D_SPU_CH_STOPSTATUS, (gMidiKeyOnMaskLH >> 16));
				}

				play_maskLH = Spu_getControlRegLH(D_SPU_CH_STATUS) | gMidiKeyOnMaskLH;

				Spu_getChannelMaskWordByRange(gMidiStartChannel, gAudioTotalChannel - 1, &midi_maskLH);
				
				if (midi_maskLH & 0xffff) {
					temp = Spu_getControlRegL(D_SPU_CH_ENABLE);
					temp &= ~(midi_maskLH & 0xffff);
					temp |= (midi_maskLH & 0xffff) & (play_maskLH & 0xffff);
					Spu_setControlRegL(D_SPU_CH_ENABLE, temp);
				}

				if (midi_maskLH >> 16) {
					temp = Spu_getControlRegH(D_SPU_CH_ENABLE);
					temp &= ~(midi_maskLH >> 16);
					temp |= (midi_maskLH >> 16) & (play_maskLH >> 16);
					Spu_setControlRegH(D_SPU_CH_ENABLE, temp);
				}
			}
		}
	}

	// clear beat counter interrupt status and enable interrupt
	Spu_setControlRegL(D_SPU_BEATCNT, 0xc000 | gMidiNextBeatCount);

	// jackson: for testing beat counter interrupt handler performance
	/*
	*P_SFT_GPIO_IOAConfig= 0x00; //Config IOA4
	*P_SFT_SOFT_SEL00|=0x10; //Enable output
	*P_SFT_SOFT_SEL04 ^= 0x10; //output HIGH
	*/
}







int Audio_translateStreamHandleToCh(int handle)
{
	int idx = handle & 0xffff;
	int counter = (handle & 0x00ff0000) >> 16;
	if (idx < gAudioStreamCount) {
		if (gAudioStream[idx].mCounter != counter) 
			return -1;
		return 15 - idx * 2 - 1;
	}
	return -1;
}

int Audio_translateChToStreamHandle(int ch)
{
	int idx = (15 - ch) / 2;
	if (idx >= 0 && idx < gAudioStreamCount) {
		if (gAudioStream[idx].mAvailable == 0) {
			return idx | (gAudioStream[idx].mCounter << 16);
		}
	}
	return -1;
}



int Audio_allocStream(void) {
	int i = 0;
	for (i = 0; i < gAudioStreamCount; i++) {
		if (gAudioStream[i].mAvailable) {
			gAudioStream[i].mAvailable = 0;
			return i | (gAudioStream[i].mCounter << 16);
		}
	}

	return -1;
}

void Audio_freeStream(int handle) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return;

	gAudioStream[idx].mCounter++;
	gAudioStream[idx].mCounter %= 256;

	gAudioStream[idx].mAvailable = 1;
}

int Audio_openStream(int handle, AudioStreamInputParam* param) {
	int idx = 0; 
	int counter = 0;
	volatile unsigned wave_addr = 0;
	int vol = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	// wait until playback idle buffer
	gAudioStream[idx].mPause = 1;
	do {
		wave_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
	} while (wave_addr >= (unsigned)gAudioStream[idx].mLeftBuffer);

	gAudioStream[idx].mParam.mSampleSize = param->mSampleSize;
	gAudioStream[idx].mParam.mSamplingFrequency = param->mSamplingFrequency;
	gAudioStream[idx].mParam.mChannel = param->mChannel;

	gAudioStream[idx].mCurrentFillBuffer = 0;
	gAudioStream[idx].mCurrentFillPos = 0;
	gAudioStream[idx].mCurrentLockWriteBuffer = 0;
	gAudioStream[idx].mNextPlayBuffer = 0;
	gAudioStream[idx].mCurrentLockReadBuffer = -1;
	gAudioStream[idx].mPreviousLockReadBuffer = -1;

	gAudioStream[idx].mPlayPos = 0;
	
	vol = Spu_getWaveVolume(gAudioStream[idx].mLeftChannel);

	Spu_setWaveVolume(gAudioStream[idx].mLeftChannel, 0);
	Spu_setWaveVolume(gAudioStream[idx].mRightChannel, 0);
	Spu_setWavePhase(gAudioStream[idx].mLeftChannel, SAMPLERATE_2_PHASE(gAudioStream[idx].mParam.mSamplingFrequency));
	Spu_setWavePhase(gAudioStream[idx].mRightChannel, SAMPLERATE_2_PHASE(gAudioStream[idx].mParam.mSamplingFrequency));
	Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_WAVEADDR, 0);
	Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_WAVEADDR, 0);
	Spu_setWaveVolume(gAudioStream[idx].mLeftChannel, vol);
	Spu_setWaveVolume(gAudioStream[idx].mRightChannel, vol);

	return 0;
}

void Audio_closeStream(int handle) {
	int idx = 0; 
	int counter = 0;
	volatile unsigned wave_addr = 0;
	int vol = 0;

	if (handle < 0) return;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return;

	// wait until playback idle buffer
	gAudioStream[idx].mPause = 1;
	do {
		wave_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
	} while (wave_addr >= (unsigned)gAudioStream[idx].mLeftBuffer);

	gAudioStream[idx].mCurrentFillBuffer = 0;
	gAudioStream[idx].mCurrentFillPos = 0;
	gAudioStream[idx].mCurrentLockWriteBuffer = 0;
	gAudioStream[idx].mNextPlayBuffer = 0;
	gAudioStream[idx].mCurrentLockReadBuffer = -1;
	gAudioStream[idx].mPreviousLockReadBuffer = -1;

	gAudioStream[idx].mPlayPos = 0;

	vol = Spu_getWaveVolume(gAudioStream[idx].mLeftChannel);

	Spu_setWaveVolume(gAudioStream[idx].mLeftChannel, 0);
	Spu_setWaveVolume(gAudioStream[idx].mRightChannel, 0);
	Spu_setWavePhase(gAudioStream[idx].mLeftChannel, SAMPLERATE_2_PHASE(24000));
	Spu_setWavePhase(gAudioStream[idx].mRightChannel, SAMPLERATE_2_PHASE(24000));
	Spu_setWaveVolume(gAudioStream[idx].mLeftChannel, vol);
	Spu_setWaveVolume(gAudioStream[idx].mRightChannel, vol);

	gAudioStream[idx].mPause = 0;
}

int Audio_resetStream(int handle) {
	int idx = 0; 
	int counter = 0;
	int pause = 0;
	volatile unsigned wave_addr = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	// wait until playback idle buffer
	pause = gAudioStream[idx].mPause;
	gAudioStream[idx].mPause = 1;
	do {
		wave_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
	} while (wave_addr >= (unsigned)gAudioStream[idx].mLeftBuffer);

	gAudioStream[idx].mCurrentFillBuffer = 0;
	gAudioStream[idx].mCurrentFillPos = 0;
	gAudioStream[idx].mCurrentLockWriteBuffer = 0;
	gAudioStream[idx].mNextPlayBuffer = 0;
	gAudioStream[idx].mCurrentLockReadBuffer = -1;
	gAudioStream[idx].mPreviousLockReadBuffer = -1;

	gAudioStream[idx].mPlayPos = 0;

	Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_WAVEADDR, 0);
	Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_WAVEADDR, 0);

	gAudioStream[idx].mPause = pause;

	return 0;
}

int Audio_pauseStream(int handle) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	gAudioStream[idx].mPause = 1;

	return 0;
}

int Audio_resumeStream(int handle) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	gAudioStream[idx].mPause = 0;

	return 0;
}

int Audio_getStreamVolume(int handle) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	return Spu_getWaveVolume(gAudioStream[idx].mLeftChannel);
}

int Audio_setStreamVolume(int handle, int vol) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	Spu_setWaveVolume(gAudioStream[idx].mLeftChannel, vol);
	Spu_setWaveVolume(gAudioStream[idx].mRightChannel, vol);

	return 0;
}

AudioPlayStatus Audio_getStreamPlayStatus(int handle) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return AUDIO_PLAY_STOP;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return AUDIO_PLAY_STOP;

	if (gAudioStream[idx].mPause) return AUDIO_PLAY_PAUSE;

	if (gAudioStream[idx].mNextPlayBuffer == gAudioStream[idx].mCurrentFillBuffer) {
		unsigned wave_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
		if (wave_addr < (unsigned)gAudioStream[idx].mLeftBuffer) {
			return AUDIO_PLAY_STOP;
		}
	}

	return AUDIO_PLAY_BUSY;
}

int Audio_getStreamPlayPos(int handle) {
	int idx = 0; 
	int counter = 0;
	int offset = 0;
	int wave_addr1 = 0;
	int wave_addr2 = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	if (gAudioStream[idx].mPreviousLockReadBuffer >= 0) {
		int previous_lock_read_buffer = gAudioStream[idx].mPreviousLockReadBuffer;
		wave_addr1 = (int)(gAudioStream[idx].mLeftBuffer + previous_lock_read_buffer * gAudioStream[idx].mChannelBufferSize);
		wave_addr2 = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
		if (wave_addr2 < wave_addr1) { // fiq happened during obtaining these addresses
			wave_addr1 = (int)(gAudioStream[idx].mLeftBuffer + previous_lock_read_buffer * gAudioStream[idx].mChannelBufferSize);
			wave_addr2 = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
		} else if (((wave_addr2 - wave_addr1) >> 1) > gAudioStream[idx].mChannelBufferSize) { // fiq happened during obtaining these addresses 
			wave_addr1 = (int)(gAudioStream[idx].mLeftBuffer + previous_lock_read_buffer * gAudioStream[idx].mChannelBufferSize);
			wave_addr2 = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
		}
		
		offset = (wave_addr2 - wave_addr1) >> 1;
	}


	return gAudioStream[idx].mPlayPos + offset;
}

int Audio_isStreamFull(int handle) {
	int idx = 0; 
	int counter = 0;
	int fill_buffer = 0;
	int fill_pos = 0;

	if (handle < 0) return 0;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return 0;

	fill_buffer = gAudioStream[idx].mCurrentFillBuffer;
	fill_pos = gAudioStream[idx].mCurrentFillPos;
	if (fill_pos == (gAudioStream[idx].mChannelBufferSize - 1)) {
		fill_buffer++;
		if (fill_buffer >= gAudioStream[idx].mChannelBufferCount) {
			fill_buffer = 0;
		}
		fill_pos = 0;
	}
	if (fill_buffer == gAudioStream[idx].mPreviousLockReadBuffer) { // buffer full
		return 1;
	} else if (fill_buffer == gAudioStream[idx].mCurrentLockReadBuffer) { // buffer full
		return 1;
	} else if (fill_buffer == gAudioStream[idx].mNextPlayBuffer) { // buffer full
		return 1;
	}

	return 0;
}

int Audio_isStreamEmpty(int handle) {
	int idx = 0; 
	int counter = 0;

	if (handle < 0) return 0;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return 0;

	if (gAudioStream[idx].mNextPlayBuffer == gAudioStream[idx].mCurrentFillBuffer) {
		unsigned wave_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
		if (wave_addr < (unsigned)gAudioStream[idx].mLeftBuffer) {
			return 1;
		}
	}

	return 0;
}

int Audio_fillStreamData(int handle, unsigned short* data, int data_size_in_byte) {
	int idx = 0; 
	int counter = 0;
	int i = 0;
	int fill_buffer = 0;
	int fill_pos = 0;
	int x = 0, y = 0;
	unsigned short* left_buffer = 0;
	unsigned short* right_buffer = 0;

	if (handle < 0) return 0;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return 0;

	fill_buffer = gAudioStream[idx].mCurrentFillBuffer;
	fill_pos = gAudioStream[idx].mCurrentFillPos;
	if (fill_pos == (gAudioStream[idx].mChannelBufferSize - 1)) {
		fill_buffer++;
		if (fill_buffer >= gAudioStream[idx].mChannelBufferCount) {
			fill_buffer = 0;
		}
		fill_pos = 0;
		if (fill_buffer == gAudioStream[idx].mPreviousLockReadBuffer) { // buffer full
			return 0;
		} else if (fill_buffer == gAudioStream[idx].mCurrentLockReadBuffer) { // buffer full
			return 0;
		} else if (fill_buffer == gAudioStream[idx].mNextPlayBuffer) { // buffer full
			return 0;
		}
	}

	gAudioStream[idx].mCurrentLockWriteBuffer = fill_buffer;

	if (gAudioStream[idx].mCurrentLockWriteBuffer == gAudioStream[idx].mCurrentLockReadBuffer) {
		return 0;
	}
	if (gAudioStream[idx].mCurrentLockWriteBuffer == gAudioStream[idx].mPreviousLockReadBuffer) {
		return 0;
	}

	left_buffer = gAudioStream[idx].mLeftBuffer + fill_buffer * gAudioStream[idx].mChannelBufferSize;
	right_buffer = gAudioStream[idx].mRightBuffer + fill_buffer * gAudioStream[idx].mChannelBufferSize;
	for (i = 0; i < data_size_in_byte / sizeof(unsigned short); i += gAudioStream[idx].mParam.mChannel) {
		if (fill_pos == (gAudioStream[idx].mChannelBufferSize - 1)) {
			int temp_fill_buffer = fill_buffer;
			temp_fill_buffer++;
			if (temp_fill_buffer >= gAudioStream[idx].mChannelBufferCount) {
				temp_fill_buffer = 0;
			}
			if (temp_fill_buffer == gAudioStream[idx].mPreviousLockReadBuffer) { // buffer full
				break;
			} else if (temp_fill_buffer == gAudioStream[idx].mCurrentLockReadBuffer) { // buffer full
				break;
			} else if (temp_fill_buffer == gAudioStream[idx].mNextPlayBuffer) { // buffer full
				break;
			}
			fill_buffer = temp_fill_buffer;
			fill_pos = 0;
			left_buffer = gAudioStream[idx].mLeftBuffer + fill_buffer * gAudioStream[idx].mChannelBufferSize;
			right_buffer = gAudioStream[idx].mRightBuffer + fill_buffer * gAudioStream[idx].mChannelBufferSize;
			gAudioStream[idx].mCurrentLockWriteBuffer = fill_buffer;
		}

		x = *(short*)data;
		data++;
		x += 0x8000;
		if (x == 0xffff) x = 0xfffe;
		left_buffer[fill_pos] = x;
		if (gAudioStream[idx].mParam.mChannel == 2) {
			y = *(short*)data;
			data++;
			y += 0x8000;
			if (y == 0xffff) y = 0xfffe;
			right_buffer[fill_pos] = y;
		} else {
			right_buffer[fill_pos] = x;
		}

		fill_pos++;
	}

	if (fill_pos != (gAudioStream[idx].mChannelBufferSize - 1)) {
		left_buffer[fill_pos] = 0xffff;
		right_buffer[fill_pos] = 0xffff;
	}

	gAudioStream[idx].mCurrentFillBuffer = fill_buffer;
	gAudioStream[idx].mCurrentFillPos = fill_pos;
	//gAudioStream[idx].mCurrentLockBuffer = -1;

	return i * sizeof(unsigned short);
}

void Audio_enableStreamStereo(int enable) {
	int i = 0;
	for (i = 0; i < gAudioStreamCount; i++) {
		if (enable) {
			Spu_setWavePan(gAudioStream[i].mRightChannel, 0x7f);
		} else {
			Spu_setWavePan(gAudioStream[i].mRightChannel, 0x0);
		}
	}
}
EXPORT_SYMBOL(Audio_enableStreamStereo);

int Audio_getStreamFreeSize(int handle) {
	int idx = 0; 
	int counter = 0;
	int i = 0;
	int fill_buffer = 0;
	int fill_pos = 0;
	int diff = 0;

	if (handle < 0) return -1;

	idx = handle & 0xffff;
	counter = (handle & 0x00ff0000) >> 16;

	if (gAudioStream[idx].mCounter != counter) return -1;

	fill_buffer = gAudioStream[idx].mCurrentFillBuffer;
	fill_pos = gAudioStream[idx].mCurrentFillPos;
	if (fill_pos == (gAudioStream[idx].mChannelBufferSize - 1)) {
		fill_buffer++;
		if (fill_buffer >= gAudioStream[idx].mChannelBufferCount) {
			fill_buffer = 0;
		}
		fill_pos = 0;
		if (fill_buffer == gAudioStream[idx].mPreviousLockReadBuffer) { // buffer full
			return 0;
		} else if (fill_buffer == gAudioStream[idx].mCurrentLockReadBuffer) { // buffer full
			return 0;
		} else if (fill_buffer == gAudioStream[idx].mNextPlayBuffer) { // buffer full
			return 0;
		}
	}

	if (fill_buffer == gAudioStream[idx].mCurrentLockReadBuffer) {
		return 0;
	}
	if (fill_buffer == gAudioStream[idx].mPreviousLockReadBuffer) {
		return 0;
	}

	while (i < gAudioStream[idx].mChannelBufferCount * gAudioStream[idx].mChannelBufferSize) {
		if (fill_pos == (gAudioStream[idx].mChannelBufferSize - 1)) {
			int temp_fill_buffer = fill_buffer;
			temp_fill_buffer++;
			if (temp_fill_buffer >= gAudioStream[idx].mChannelBufferCount) {
				temp_fill_buffer = 0;
			}
			if (temp_fill_buffer == gAudioStream[idx].mPreviousLockReadBuffer) { // buffer full
				break;
			} else if (temp_fill_buffer == gAudioStream[idx].mCurrentLockReadBuffer) { // buffer full
				break;
			} else if (temp_fill_buffer == gAudioStream[idx].mNextPlayBuffer) { // buffer full
				break;
			}
			fill_buffer = temp_fill_buffer;
			fill_pos = 0;
		}

		diff = (gAudioStream[idx].mChannelBufferSize - 1) - fill_pos;
		fill_pos += diff;
		i += diff * gAudioStream[idx].mParam.mChannel;
	}

	return i * sizeof(unsigned short);
}

void Audio_handleFiq(int channel) {
	int idx = (15 - channel) / 2;
	unsigned short* left_buffer = 0;
	unsigned short* right_buffer = 0;
	int current_play_buffer = 0;
	volatile unsigned wave_addr = 0;
	unsigned loop_addr = 0;

	Spu_enableFiq(gAudioStream[idx].mLeftChannel, 0);
	Spu_clearFiqStatus(gAudioStream[idx].mLeftChannel);

/*
	loop_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
	do {
		wave_addr = Spu_getWaveAddr(gAudioStream[idx].mLeftChannel);
	} while (wave_addr == loop_addr);
*/

	if (gAudioStream[idx].mAvailable) { // channel was freed
		//Spu_setLoopAddress(gAudioStream[idx].mLeftChannel, (unsigned char*)gAudioStream[idx].mIdleLeftBuffer);
		//Spu_setLoopAddress(gAudioStream[idx].mRightChannel, (unsigned char*)gAudioStream[idx].mIdleRightBuffer);
		//Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, ((unsigned)gAudioStream[idx].mIdleLeftBuffer >> 1) & 0xffff);
		//Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_LOOPADDR, ((unsigned)gAudioStream[idx].mIdleRightBuffer >> 1) & 0xffff);
		Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, 0);
		Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_LOOPADDR, 0);
		Spu_enableFiq(gAudioStream[idx].mLeftChannel, 1);
		gAudioStream[idx].mCurrentLockReadBuffer = -1;
		return;
	}

	if (gAudioStream[idx].mPause) {
		//Spu_setLoopAddress(gAudioStream[idx].mLeftChannel, (unsigned char*)gAudioStream[idx].mIdleLeftBuffer);
		//Spu_setLoopAddress(gAudioStream[idx].mRightChannel, (unsigned char*)gAudioStream[idx].mIdleRightBuffer);
		//Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, ((unsigned)gAudioStream[idx].mIdleLeftBuffer >> 1) & 0xffff);
		//Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_LOOPADDR, ((unsigned)gAudioStream[idx].mIdleRightBuffer >> 1) & 0xffff);
		Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, 0);
		Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_LOOPADDR, 0);
		Spu_enableFiq(gAudioStream[idx].mLeftChannel, 1);
		gAudioStream[idx].mPreviousLockReadBuffer = gAudioStream[idx].mCurrentLockReadBuffer;
		gAudioStream[idx].mCurrentLockReadBuffer = -1;
		return;
	}

	current_play_buffer = gAudioStream[idx].mNextPlayBuffer;

	if (current_play_buffer == gAudioStream[idx].mCurrentFillBuffer) { // last buffer to play?
		if (gAudioStream[idx].mCurrentLockWriteBuffer == current_play_buffer) { // buffer is filling data, cannot play at this moment
			//Spu_setLoopAddress(gAudioStream[idx].mLeftChannel, (unsigned char*)gAudioStream[idx].mIdleLeftBuffer);
			//Spu_setLoopAddress(gAudioStream[idx].mRightChannel, (unsigned char*)gAudioStream[idx].mIdleRightBuffer);

			//Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, ((unsigned)gAudioStream[idx].mIdleLeftBuffer >> 1) & 0xffff);
			//Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_LOOPADDR, ((unsigned)gAudioStream[idx].mIdleRightBuffer >> 1) & 0xffff);
			Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, 0);
			Spu_setChannelControlReg(gAudioStream[idx].mRightChannel,D_SPU_CH_LOOPADDR, 0);
			Spu_enableFiq(gAudioStream[idx].mLeftChannel, 1);
			gAudioStream[idx].mPreviousLockReadBuffer = gAudioStream[idx].mCurrentLockReadBuffer;
			gAudioStream[idx].mCurrentLockReadBuffer = -1;
			return;
		}
	}

	left_buffer = gAudioStream[idx].mLeftBuffer + current_play_buffer * gAudioStream[idx].mChannelBufferSize;
	right_buffer = gAudioStream[idx].mRightBuffer + current_play_buffer * gAudioStream[idx].mChannelBufferSize;

	Spu_setLoopAddress(gAudioStream[idx].mLeftChannel, (unsigned char*)left_buffer);
	Spu_setLoopAddress(gAudioStream[idx].mRightChannel, (unsigned char*)right_buffer);
	//Spu_setChannelControlReg(gAudioStream[idx].mLeftChannel, D_SPU_CH_LOOPADDR, ((unsigned)left_buffer >> 1) & 0xffff);
	//Spu_setChannelControlReg(gAudioStream[idx].mRightChannel, D_SPU_CH_LOOPADDR, ((unsigned)right_buffer >> 1) & 0xffff);

	if (gAudioStream[idx].mPreviousLockReadBuffer != -1) {
		gAudioStream[idx].mPlayPos += gAudioStream[idx].mChannelBufferSize;
	}

	gAudioStream[idx].mPreviousLockReadBuffer = gAudioStream[idx].mCurrentLockReadBuffer;
	gAudioStream[idx].mCurrentLockReadBuffer = current_play_buffer;

	current_play_buffer++;
	if (current_play_buffer >= gAudioStream[idx].mChannelBufferCount) {
		current_play_buffer = 0;
	}
	gAudioStream[idx].mNextPlayBuffer = current_play_buffer;

	Spu_enableFiq(gAudioStream[idx].mLeftChannel, 1);
}

