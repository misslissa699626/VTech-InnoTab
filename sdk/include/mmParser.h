#ifndef __MULTIMEDIA_FILE_PARSER_API_H__
#define __MULTIMEDIA_FILE_PARSER_API_H__

#include "mediaSystem_typedef.h"

typedef struct
{
	// file init layer
	int (*Init)(void *hParser, void *fin);
	void (*Release)(void *hParser);
	// file layer
	int (*GetTotalStreamNumber)(void *hParser);
	// stream init layer
	void *(*GetStream)(void *hParser, int StreamIdx);
} MULTIMEDIA_FILE_API;

typedef struct
{
	int (*FreeStream)(void *hStream);
	// stream layer
	int (*GetStreamInfo)(void *hStream, GP_STREAM_INFO *info, int cbSize);
	const char *(*GetExtraData)(void *hStream);
	int (*GetExtraDataSize)(void *hStream);
	int (*GetFrameInfo)(void *hStream, GP_FRAME_INFO *info);
	int (*Read)(void *hStream, char *Buffer, int size);
	int (*Seek)(void *hStream, int *msec, int reversed);
} MULTIMEDIA_STREAM_API;


#ifdef __cplusplus
class CParserRegister
{
public:
	CParserRegister(
		const char *description,
		const char *ext,
		int (*IsTypeOf)(void *fin),
		int (*GetInstSize)(void),
		const MULTIMEDIA_FILE_API	*mmFileFcnTab,
		const MULTIMEDIA_STREAM_API	*mmStreamFcnTab);
	int CheckFilename(const char *filename) const;

public: // member
	const char *description;
	int flag_any_file;
	char ExtList[32];
	int (*IsTypeOf)(void *fin);
	int (*GetInstSize)(void);
	const MULTIMEDIA_FILE_API	*mmFileFcnTab;
	const MULTIMEDIA_STREAM_API	*mmStreamFcnTab;

public:
	static CParserRegister *firstParser;
	static CParserRegister *lastParser;
	CParserRegister *nextParser;
	CParserRegister *preParser;
};
#endif

#endif // __MULTIMEDIA_FILE_PARSER_API_H__
