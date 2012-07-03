#ifndef _MACH_SPU_H_
#define _MACH_SPU_H_

typedef struct ppu_register_region_s {
	unsigned int start;
	unsigned int end;
} ppu_register_region_t; 

/* ioctl command */
//#define SPU_IOC_Audio_init						0x1000
//#define SPU_IOC_Audio_cleanup					0x1001
#define SPU_IOC_Audio_getMasterVolume			0x1002
#define SPU_IOC_Audio_setMasterVolume			0x1003
#define SPU_IOC_Audio_setWave					0x1004
#define SPU_IOC_Audio_setDrumNote				0x1005
#define SPU_IOC_Audio_setMelodyNote				0x1006
#define SPU_IOC_Audio_releaseMelodyNote			0x1007
#define SPU_IOC_Audio_resetWave					0x1008
#define SPU_IOC_Audio_pauseWave					0x1009
#define SPU_IOC_Audio_resumeWave				0x100A
#define SPU_IOC_Audio_getWaveStatus				0x100B
#define SPU_IOC_Audio_isWaveLoopEnable			0x100C
#define SPU_IOC_Audio_enableWaveLoop			0x100D
#define SPU_IOC_Audio_getWaveVolume				0x100E
#define SPU_IOC_Audio_setWaveVolume				0x100F
#define SPU_IOC_Audio_getWavePan				0x1010
#define SPU_IOC_Audio_setWavePan				0x1011
#define SPU_IOC_Audio_getWavePlaybackSpeed		0x1012
#define SPU_IOC_Audio_setWavePlaybackSpeed		0x1013
#define SPU_IOC_Audio_initMidi					0x1014
#define SPU_IOC_Audio_cleanupMidi				0x1015
#define SPU_IOC_Audio_setMidi					0x1016
#define SPU_IOC_Audio_setMidiStream				0x1017
#define SPU_IOC_Audio_resetMidi					0x1018
#define SPU_IOC_Audio_pauseMidi					0x1019
#define SPU_IOC_Audio_resumeMidi				0x101A
#define SPU_IOC_Audio_getMidiStatus				0x101B
#define SPU_IOC_Audio_isMidiLoopEnable			0x101C
#define SPU_IOC_Audio_enableMidiLoop			0x101D
#define SPU_IOC_Audio_getMidiVolume				0x101E
#define SPU_IOC_Audio_setMidiVolume				0x101F
#define SPU_IOC_Audio_getMidiGlobalPan			0x1020
#define SPU_IOC_Audio_setMidiGlobalPan			0x1021
#define SPU_IOC_Audio_enableMidiGlobalPan		0x1022
#define SPU_IOC_Audio_getMidiInstrumentVolume	0x1023
#define SPU_IOC_Audio_setMidiInstrumentVolume	0x1024
#define SPU_IOC_Audio_getMidiCurrentPlayTime	0x1025
#define SPU_IOC_Audio_getMidiPlaybackSpeed		0x1026
#define SPU_IOC_Audio_setMidiPlaybackSpeed		0x1027
#define SPU_IOC_Audio_fadeMidi					0x1028
#define SPU_IOC_Audio_isMidiFade				0x1029
#define SPU_IOC_Audio_setMidiLyricEventHandler	0x102A
#define SPU_IOC_Audio_handleBeatCounter			0x102B

#define SPU_IOC_Audio_allocStream 				0x102C
#define SPU_IOC_Audio_freeStream 				0x102D
#define SPU_IOC_Audio_openStream 				0x102E
#define SPU_IOC_Audio_closeStream 				0x102F
#define SPU_IOC_Audio_resetStream 				0x1030
#define SPU_IOC_Audio_pauseStream 				0x1031
#define SPU_IOC_Audio_resumeStream 				0x1032
#define SPU_IOC_Audio_getStreamVolume 			0x1033
#define SPU_IOC_Audio_setStreamVolume 			0x1034
#define SPU_IOC_Audio_getStreamPlayStatus 		0x1035
#define SPU_IOC_Audio_getStreamPlayPos 			0x1036
#define SPU_IOC_Audio_isStreamFull 				0x1037
#define SPU_IOC_Audio_isStreamEmpty 			0x1038
#define SPU_IOC_Audio_fillStreamData 			0x1039
#define SPU_IOC_Audio_handleFiq 				0x103A
#define SPU_IOC_Audio_enableStreamStereo 		0x103B

#define SPU_IOC_Audio_acquireWaveChannels		0x103C
#define SPU_IOC_Audio_acquireMidiChannels		0x103D

#define SPU_IOC_Audio_getStreamFreeSize			0x103E

#define SPU_IOC_Debug_getChannelAllocation		0x2000

#endif /* _MACH_SPU_H_ */
