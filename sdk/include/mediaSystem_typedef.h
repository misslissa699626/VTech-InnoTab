#ifndef __TYPEDEF_MULTIMEDIA_H__
#define __TYPEDEF_MULTIMEDIA_H__

typedef struct
{
	int count;			// count of sample(s)
						//   if [count] > 1 then following [count] samples are in the same size
						//   if [count] = 0 that means end of stream
						//   NOTE: ONLY constant-data-rate can set [count] greater than 1
	int time;			// first sample present time in msec
	int duration;		// sample(s) duration in msec
	int size;			// sample(s) size in byte, 	
	int flag;			// I/P/B-frame
} GP_FRAME_INFO;


typedef struct
{
	int SampleRate;
	int Channels;
} GP_AUD_FORMAT;
typedef struct
{
	int Scale;
	int Rate;
	int Width;
	int Height;
} GP_VID_FORMAT;
typedef struct
{
	int cbSize;						// used for version control
	union
	{
		int StreamType;				// 'vids', 'auds', 'text', 'midi'
		char strStreamType[4];		// 'vids', 'auds', 'text', 'midi'
	};
	int CodecID;
	int Duration;					// count in m-sec
	int StartTime;					// count in m-sec
	int reserved[3];
	union
	{
		GP_AUD_FORMAT aud;
		GP_VID_FORMAT vid;
	};
} GP_STREAM_INFO;


typedef struct { 
    short			wFormatTag; 
    short			nChannels; 
    unsigned long	nSamplesPerSec; 
    unsigned long	nAvgBytesPerSec; 
    short			nBlockAlign; 
    short			wBitsPerSample;
} GP_WAVEFORMAT; 

#endif // __TYPEDEF_MULTIMEDIA_H__
