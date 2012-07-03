#ifndef _GP_STREAM_H_
#define _GP_STREAM_H_

#include <stdint.h>
#include <mach/typedef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	STREAM_MODE_RDONLY,
	STREAM_MODE_WRONLY,
	STREAM_MODE_RDWR,
	STREAM_MODE_MAX
};

enum {
	STREAM_SEEK_SET,
	STREAM_SEEK_CUR,
	STREAM_SEEK_END
};

enum {
	STREAM_IOCTRL_GET_TIME,
	STREAM_IOCTRL_GET_MODE		
};

typedef struct gpStreamOp_s {
	SINT32 (*close)(HANDLE hStream);
	SINT32 (*read)(HANDLE hStream, void *buf, UINT32 len);
	SINT32 (*write)(HANDLE hStream, void *buf, UINT32 len);
	SINT32 (*seek)(HANDLE hStream, SINT32 offset, UINT32 whence);
	SINT32 (*tell)(HANDLE hStream);
	SINT32 (*ioctl)(HANDLE hStream, SINT32 cmd, void *param);
	SINT32 (*size)(HANDLE hStream);
} gpStreamOp_t;

typedef gpStreamOp_t spStreamOp_t;

typedef struct gpStream_s {
	gpStreamOp_t *op;
	void *pData;
} gpStream_t;

typedef gpStream_t spStream_t;

HANDLE streamOpen(const SP_CHAR *filename, UINT32 mode);
SINT32 streamClose(HANDLE hStream);
SINT32 streamRead(HANDLE hStream, void *pBuf, UINT32 len);
SINT32 streamWrite(HANDLE hStream, void *pBuf, UINT32 len);
SINT32 streamSeek(HANDLE hStream, SINT32 offset, UINT32 whence);
SINT32 streamTell(HANDLE hStream);
SINT32 streamIoctl(HANDLE hStream, SINT32 cmd, void *param);
SINT32 streamGetSize(HANDLE hStream);

SINT32 streamSetUserData(HANDLE hStream, void *pData);
void* streamGetUserData(HANDLE hStream);
SINT32 streamGetErr();
SINT32 streamSetErr(SINT32 errno);

HANDLE fileStreamOpen(SP_CHAR *filename, UINT32 mode);
HANDLE lzoStreamOpen(SP_CHAR *filename);
HANDLE memStreamOpen(void *buf, UINT32 size, UINT32 mode);

#ifdef CONFIG_DLNA
HANDLE httpStreamOpen(SP_CHAR *filename, UINT32 mode);
UINT32 httpSetStreamLen(UINT32 uiLen);
UINT32 httpGetStreamLen(UINT32 *puiLen);
#endif

#ifdef __cplusplus
}
#endif


#endif //_GP_STREAM_H_
