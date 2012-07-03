#ifndef __MEDIASTREAM_H__
#define __MEDIASTREAM_H__

#include "mediaSystem_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif


void *OpenMediaFile(const char *filename);

// file function
int mmFile_Free(void *hFile);
int mmFile_GetTotalStreamNumber(void *hFile);
void *mmFile_GetStream(void *hFile, int StreamIdx);

// stream function
int mmStream_Free(void *hStream);
int mmStream_GetStreamInfo(void *hStream, GP_STREAM_INFO *info, int cbSize);
const char *mmStream_GetExtraData(void *hStream);
int mmStream_GetExtraDataSize(void *hStream);	
int mmStream_GetFrameInfo(void *hStream, GP_FRAME_INFO *info);
int mmStream_Read(void *hStream, char *Buffer, int size);
int mmStream_Seek(void *hStream, int *msec, int reserved);


#ifdef __cplusplus
}
#endif

#endif // __MEDIASTREAM_H__
