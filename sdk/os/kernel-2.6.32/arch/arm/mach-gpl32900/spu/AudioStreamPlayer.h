#ifndef AUDIOSTREAMPLAYER_H
#define AUDIOSTREAMPLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "WaveInfo.h"

struct AudioStream;
struct AudioDecoder;

/*!
	\brief function pointer type used to open stream\n
	\brief first parameter is the audio stream itself\n
	\brief second parameter is the location\n
	\brief thrid parameter is the size of location
	\return 0 if operation success or otherwise other error code
*/
typedef int (*AudioStreamOpen)(struct AudioStream* /* this */, const unsigned char* /* location */, int /* location size */);

/*!
	\brief function pointer type used to read block of data from stream\n
	\brief first parameter is the audio stream itself\n
	\brief second parameter is the buffer to hold the result\n
	\brief third parameter is the buffer size
	\return number of bytes actually read (>= 0) or otherwise other error code (< 0)
*/
typedef int (*AudioStreamRead)(struct AudioStream* /* this */, unsigned char* /* buffer */, int /* len */);

/*!
	\brief function pointer type used to move read pointer to specified position\n
	\brief first parameter is the audio stream itself\n
	\brief second parameter is the offset from start of location
	\return 0 if operation success or otherwise other error code
*/
typedef int (*AudioStreamSeek)(struct AudioStream* /* this */, int /* offset from start */);

/*!
	\brief function pointer type used to close an opened stream\n
	\brief first parameter is the audio stream itself
	\return 0 if operation success or otherwise other error code
*/
typedef void (*AudioStreamClose)(struct AudioStream* /* this */);

/*!
	\brief function pointer type used to check if read passed the end of stream\n
	\brief first parameter is the audio stream itself
	\return 1 if read passed EOF or otherwise 0
*/
typedef int (*AudioStreamEof)(struct AudioStream* /* this */);

/*!
	\brief function pointer type used to check if there is any error occurred during the read operation\n
	\brief first parameter is the audio stream itself
	\return error code if error occurred or otherwise 0
*/
typedef int (*AudioStreamError)(struct AudioStream* /* this */);

/*!
	\brief function pointer type used to clear error occurred during the read operation\n
	\brief first parameter is the audio stream itself
*/
typedef void (*AudioStreamClrErr)(struct AudioStream* /* this */);

/*!
	\brief function pointer type used to initialize decoder and give out SPU channel initialization information\n
	\brief first parameter is the audio decoder itself\n
	\brief second parameter is the input audio stream\n
	\brief third parameter is output parameter, stored the SPU channel initialization information
	\return 0 if initialization succeed or otherwise other error code (!= 0)
*/
typedef int (*AudioDecoderInit)(struct AudioDecoder* /* this */, struct AudioStream* /* input stream */, WaveInfo* /* output configuration */);

/*!
	\brief function pointer type used to reset decoder and input stream\n
	\brief first parameter is the audio decoder itself\n
	\brief second parameter is the input audio stream\n
*/
typedef void (*AudioDecoderReset)(struct AudioDecoder* /* this */, struct AudioStream* /* input stream */);

/*!
	\brief function pointer type used to decode audio data\n
	\brief first parameter is the audio decoder itself\n
	\brief second parameter is the input audio stream\n
	\brief third parameter is the buffer used to hold the decode result\n
	\brief forth parameter is the buffer size
	\return number of bytes of decoded data (>= 0) or other error code (< 0)
*/
typedef int (*AudioDecoderDecode)(struct AudioDecoder* /* this */, struct AudioStream* /* input stream */, unsigned char* /* buffer */, unsigned /* buffer size */);

/*!
	\brief function pointer type used to cleanup internal states of audio decoder\n
	\brief first parameter is the audio decoder itself
*/
typedef void (*AudioDecoderCleanup)(struct AudioDecoder* /* this */);



/*!
	\struct AudioStream
	\brief data structure used to handle stream related operations
*/
typedef struct AudioStream {
	AudioStreamOpen mOpenFn;
	AudioStreamRead mReadFn;
	AudioStreamSeek mSeekFn;
	AudioStreamClose mCloseFn;
	AudioStreamEof mEofFn;
	AudioStreamError mErrorFn;
	AudioStreamClrErr mClrErrFn;
	void* mExtra;
} AudioStream;

/*!	
	\struct AudioDecoder
	\brief data structure used to read data from stream and handle decode related operations
*/
typedef struct AudioDecoder {
	AudioDecoderInit mInitFn;
	AudioDecoderReset mResetFn;
	AudioDecoderDecode mDecodeFn;
	AudioDecoderCleanup mCleanupFn;
	void* mExtra;
} AudioDecoder;


typedef enum {
	AUDIOSTREAMPLAYER_CLOSE,
	AUDIOSTREAMPLAYER_OPEN,
	AUDIOSTREAMPLAYER_BUSY,
	AUDIOSTREAMPLAYER_PAUSE,
	AUDIOSTREAMPLAYER_STOP,
	AUDIOSTREAMPLAYER_ERROR
} AudioStreamPlayerStatus;

typedef struct AudioStreamPlayer {
	AudioStream* mStream;
	AudioDecoder* mDecoder;
	int mChannel;
	unsigned char** mBuffer;
	unsigned mBufferCount;
	unsigned mBufferSize;

	WaveInfo mInfo;
	AudioStreamPlayerStatus mStatus;
	int mReadBuffer;
	int mWriteBuffer;
	int mEndBuffer;
} AudioStreamPlayer;

void AudioStreamPlayer_init(void);
void AudioStreamPlayer_cleanup(void);

int AudioStreamPlayer_register(int channel_idx, AudioStreamPlayer* player);
void AudioStreamPlayer_unregister(int channel_idx);
int AudioStreamPlayer_open(int channel_idx, const unsigned char* location, unsigned location_size_in_byte);
void AudioStreamPlayer_close(int channel_idx);
AudioStreamPlayerStatus AudioStreamPlayer_getStatus(int channel_idx);
void AudioStreamPlayer_resume(int channel_idx);
void AudioStreamPlayer_pause(int channel_idx);
void AudioStreamPlayer_reset(int channel_idx);

int AudioStreamPlayer_fillBuffer(int channel_idx);

void AudioStreamPlayer_nextBuffer(int channel_idx);

#ifdef __cplusplus
}
#endif

#endif