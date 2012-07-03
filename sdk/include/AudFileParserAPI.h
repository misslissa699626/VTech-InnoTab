#ifndef __AUD_FILE_PARSER_API_H__
#define __AUD_FILE_PARSER_API_H__

typedef struct
{
	int count;			// count of sample(s)
						//   if [count] > 1 then following [count] samples are in the same size
						//   if [count] = 0 that means end of stream
						//   NOTE: ONLY constant-data-rate can set [count] greater than 1
	int time;			// first sample present time in msec
	int duration;		// sample(s) duration in msec
	int size;			// sample(s) size in byte, 	
} SAMPLEINFO;

typedef struct
{
	int (*GetInstSize)(void);
	int (*Init)(void *hParser, const char *filename);
	void (*Release)(void *hParser);
	int (*Read)(void *hParser, char *Buffer, int size);
	int (*Seek)(void *hParser, int *msec);
	const char *(*GetExtraData)(const void *hParser);
	int (*GetExtraDataSize)(const void *hParser);
	int (*GetCodecID)(const void *hParser);
	int (*GetSampleInfo)(const void *hParser, SAMPLEINFO *info);
} AUD_FILE_PARSER_API;

#endif // __AUD_FILE_PARSER_API_H__

