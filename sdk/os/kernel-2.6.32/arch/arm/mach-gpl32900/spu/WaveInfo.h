#ifndef WAVEINFO_H
#define WAVEINFO_H

/*!
	\brief configuration information of a channel
*/
typedef struct WaveInfo {
	unsigned char mFormat;
	unsigned char mLoopEnable;
	unsigned char mEnvAutoModeEnable;
	unsigned char mReserved0;
	unsigned short mEnv0;
	unsigned short mEnv1;
	unsigned short mEnvData;
	unsigned short mReserved1;
	unsigned char* mWaveAddr;
	unsigned char* mLoopAddr;
	unsigned char* mEnvAddr;
	unsigned mRampDownOffset;
	unsigned mRampDownClk;
	unsigned mPhase;
} WaveInfo;

#endif