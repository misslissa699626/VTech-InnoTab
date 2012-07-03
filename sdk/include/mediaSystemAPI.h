#ifndef __MEDIA_SYSTEM_API_H__
#define __MEDIA_SYSTEM_API_H__

#include "mediaSystem_typedef.h"

enum {
	MEDIA_MSG_EOA = 0x00000001,
	MEDIA_MSG_ERR = 0x80000000,
};

#ifdef __cplusplus
extern "C" {
#endif

// Create and Destroy
void *audPlayer_Create(int (*Callback)(void *param, int msg, int val), void *param, int dbgLevel);
int audPlayer_Destroy(void *hMedia);

// Device Setup, used ONLY in STOP state
int audPlayer_SetAudioDevice(void *hMedia, const char *WaveOutDevice);

// Playback Setup, used ONLY in STOP state
int audPlayer_EnableStreamLevelLooping(void *hMedia, int enable);

// Play and Stop
int audPlayer_Play(void *hMedia, const char *file_name, int *ms);
int audPlayer_Stop(void *hMedia);

// Playback Setup, used both in PLAY and STOP state
int audPlayer_Seek(void *hMedia, int *ms);
int audPlayer_Pause(void *hMedia);
int audPlayer_Resume(void *hMedia);
int audPlayer_SetVolume(void *hMedia, int vol);
int audPlayer_GetPlayTime(void *hMedia, int *err);
int audPlayer_SetSpeed(void *hMedia, int Speed);

// Current Playing Stream Information, used ONLY in PLAY state
int audPlayer_GetStreamInfo(void *hMedia, GP_STREAM_INFO *Info);


#ifdef __cplusplus
}
#endif

#endif // __MEDIA_SYSTEM_API_H__
