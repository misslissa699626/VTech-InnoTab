#ifndef __MEDIASYSTEM_FILE_H__
#define __MEDIASYSTEM_FILE_H__

#include "fcntl.h"

#ifdef __cplusplus
extern "C" {
#endif

void *mediaSystem_FileOpen(const char *filename, unsigned int mode);
int mediaSystem_FileClose(void *hFile);
int mediaSystem_FileRead(void *hFile, void *pBuf, unsigned int len);
int mediaSystem_FileWrite(void *hFile, void *pBuf, unsigned int len);
int mediaSystem_FileSeek(void *hFile, int offset, unsigned int whence);
int mediaSystem_FileTell(void *hFile);
int mediaSystem_FileGetSize(void *hFile);

void *mediaSystem_EncryptFile_Open(const char *filename, unsigned int mode);
int mediaSystem_EncryptFile_Close(void *hFile);
int mediaSystem_EncryptFile_Read(void *hFile, void *pBuf, unsigned int len);
int mediaSystem_EncryptFile_Write(void *hFile, void *pBuf, unsigned int len);
int mediaSystem_EncryptFile_Seek(void *hFile, int offset, unsigned int whence);
int mediaSystem_EncryptFile_Tell(void *hFile);
int mediaSystem_EncryptFile_Size(void *hFile);

#ifdef __cplusplus
}
#endif


static __inline void *streamOpen(const char *filename, unsigned int  mode)
{
	return mediaSystem_FileOpen(filename, mode);
}
static __inline int streamClose(void *hFile)
{
	return mediaSystem_FileClose(hFile);
}
static __inline int streamRead(void *hFile, void *pBuf, unsigned int  len)
{
	return mediaSystem_FileRead(hFile, pBuf, len);
}
static __inline int streamWrite(void *hFile, void *pBuf, unsigned int  len)
{
	return mediaSystem_FileWrite(hFile, pBuf, len);
}
static __inline int streamSeek(void *hFile, int offset, unsigned int whence)
{
	return mediaSystem_FileSeek(hFile, offset, whence);
}
static __inline int streamTell(void *hFile)
{
	return mediaSystem_FileTell(hFile);
}
static __inline int streamGetSize(void *hFile)
{
	return mediaSystem_FileGetSize(hFile);
}


#endif // __MEDIASYSTEM_FILE_H__
