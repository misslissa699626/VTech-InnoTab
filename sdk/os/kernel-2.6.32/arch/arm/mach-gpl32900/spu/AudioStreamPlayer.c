#include "AudioStreamPlayer.h"

/*
#include "SpuDrv.h"
#include "SpuReg.h"

#define buffer_full(read_idx, write_idx) (write_idx == read_idx)
#define buffer_empty(read_idx, write_idx, buffer_count) (write_idx == ((read_idx + 1) % buffer_count))

AudioStreamPlayer* gAudioStreamPlayer[D_SPU_TOTAL_CHANNEL] = {0};

void AudioStreamPlayer_setLoopAddress(int channel_idx, unsigned char* addr) {
	unsigned channel_mode = 0;
	unsigned wl_haddr = 0;

	unsigned addr0 = (addr >> 1) & 0xffff;
	unsigned addr1 = (addr >> 11) & 0x0fc0;
	unsigned addr2 = (addr >> 17) & 0x0fc0;

	channel_mode = Spu_getChannelControlReg(channel_idx, D_SPU_CH_MODE);
	wl_haddr = Spu_getChannelControlReg(channel_idx, D_SPU_CH_WL_HADDR);

	channel_mode &= 0xf0cf;
	channel_mode |= addr1;
	wl_haddr &= 0xf0cf;
	wl_haddr |= addr2;

	Spu_setChannelControlReg(channel_idx, D_SPU_CH_LOOPADDR, addr0);
	Spu_setChannelControlReg(channel_idx, D_SPU_CH_MODE, channel_mode);
	Spu_setChannelControlReg(channel_idx, D_SPU_CH_WL_HADDR, wl_haddr);
}

int AudioStreamPlayer_fillBuffer(int channel_idx) {
	int read_idx;
	int write_idx;
	int buffer_count;
	int result;

	if (gAudioStreamPlayer[idx]) { // no stream player available, should not happen
		// do nothing and return
		return -1;
	}

	read_idx = gAudioStreamPlayer[idx]->mReadBuffer;
	write_idx = gAudioStreamPlayer[idx]->mWriteBuffer;
	buffer_count = gAudioStreamPlayer[idx]->mBufferCount;
	
	if (buffer_full(read_idx, write_idx)) {
		// do nothing
		return 0;
	}
	
	if (gAudioStreamPlayer[idx]->mEndBuffer == -1) { // no more data to read
		// do nothing return
		return 0;
	}
	
	result = gAudioStreamPlayer[idx]->mDecoder->mDecodeFn(
		gAudioStreamPlayer[idx]->mDecoder,
		gAudioStreamPlayer[idx]->mStream,
		gAudioStreamPlayer[idx]->mBuffer[write_idx],
		gAudioStreamPlayer[idx]->mBufferSize - 2
	);
	
	if (result < 0) { // error during stream-in / decode data
		gAudioStreamPlayer[idx]->mBuffer[0] = 0xff;
		gAudioStreamPlayer[idx]->mBuffer[1] = 0xff;
		gAudioStreamPlayer[idx]->mEndBuffer = write_idx;
		return -1;
	}
	
	gAudioStreamPlayer[idx]->mBuffer[result] = 0xff;
	gAudioStreamPlayer[idx]->mBuffer[result + 1] = 0xff;
	
	if (gAudioStreamPlayer[idx]->mStream->mEofFn(gAudioStreamPlayer[idx]->mStream)) { // no more data to read
		gAudioStreamPlayer[idx]->mEndBuffer = write_idx;
	}
	
	write_idx = (write_idx + 1) % buffer_count;
	gAudioStreamPlayer[idx]->mWriteBuffer = write_idx;
}

void AudioStreamPlayer_nextBuffer(int channel_idx) {
	int read_idx;
	int write_idx;
	int buffer_count;
	
	Spu_clearFiqStatus(idx);
	
	if (gAudioStreamPlayer[idx]) { // no stream player available
		Spu_enableFiq(idx, 0); // disable corresponding FIQ
		Spu_enableWaveLoop(idx, 0); // disable loop
		return;
	}
	
	read_idx = gAudioStreamPlayer[idx]->mReadBuffer;
	write_idx = gAudioStreamPlayer[idx]->mWriteBuffer;
	buffer_count = gAudioStreamPlayer[idx]->mBufferCount;
	
	if (buffer_empty(read_idx, write_idx, buffer_count)) { // buffer empty
		// do nothing and let the last audio segment loop
		return;
	}
	
	read_idx = (read_idx + 1) % buffer_count;
	if (read_idx != gAudioStreamPlayer[idx]->mEndBuffer) { // not final audio segment
		// set next audio segment address as loop address
		AudioStreamPlayer_setLoopAddress(idx, gAudioStreamPlayer[idx]->mBuffer[read_idx]);
	} else {
		// disable loop so no more playback will happen afterward
		Spu_enableWaveLoop(idx, 0);
	}
	
	gAudioStreamPlayer[idx]->mReadBuffer = read_idx;
}

void AudioStreamPlayer_init() {
	int i = 0;
	for (i = 0; i < D_SPU_TOTAL_CHANNEL; i++) {
		gAudioStreamPlayer[i] = 0;
	}
}

void AudioStreamPlayer_cleanup() {
	int i = 0;
	for (i = 0; i < D_SPU_TOTAL_CHANNEL; i++) {
		if (gAudioStreamPlayer[i]->mStatus != AUDIOSTREAMPLAYER_CLOSE) {
			AudioStreanPlayer_close(i);
			gAudioStreamPlayer[i] = 0;
		}
	}
}

int AudioStreamPlayer_register(int channel_idx, AudioStreamPlayer* player) {
	if ((channel_idx < 0) || (channel_idx >= D_SPU_TOTAL_CHANNEL)) {
		return -1;
	}
	
	
	if (gAudioStreamPlayer[channel_idx]) { // other player already registered this channel
		return -1;
	}
	
	player->mStatus = AUDIOSTREAMPLAYER_STOP;
	player->mReadBuffer = -1;
	player->mWriteBuffer = 0;
	player->mEndBuffer = -1;
	player->mChannel = channel_idx;
	
	gAudioStreamPlayer[channel_idx] = player;
	
	return 0;
}

void AudioStreamPlayer_unregister(int channel_idx) {
	if ((channel_idx < 0) || (channel_idx >= D_SPU_TOTAL_CHANNEL)) {
		return;
	}

	gAudioStreamPlayer[channel_idx] = 0;
}

int AudioStreamPlayer_open(int channel_idx, const unsigned char* location, unsigned location_size_in_byte) {
	int result;
	
	if (gAudioStreamPlayer[idx]) { // no stream player available
		Spu_enableFiq(idx, 0); // disable corresponding FIQ
		Spu_enableWaveLoop(idx, 0); // disable loop
		return;
	}
	
	result = gAudioStreamPlayer[idx]->mStream->mOpenFn(
		gAudioStreamPlayer[idx]->mStream,
		location,
		location_size
	);
	
	if (result < 0) {
		return result;
	}
	
	result = AudioStreamPlayer_fillBuffer(channel_idx);
	
}

void AudioStreamPlayer_close(int channel_idx) {
}
*/
