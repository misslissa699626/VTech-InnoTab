#ifndef _MPGLIB_H_
#define _MPGLIB_H_

#include "mpglib_config.h"

#ifndef NOANALYSIS
extern plotting_data *mpg123_pinfo;
#endif


typedef struct mpstr_tag {
	int framesize;
        int fsizeold;
	struct frame fr;
        unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	real hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	unsigned long header;
	int bsnum;
	real synth_buffs[2][2][0x110];
        int  synth_bo;

    int bitindex;
    unsigned char* wordpointer;	
} MPSTR, *PMPSTR;


#if ( defined(_MSC_VER) || defined(__BORLANDC__) )
	typedef int BOOL; /* windef.h contains the same definition */
#else
	#define BOOL int
#endif

#define MP3_ERR -1
#define MP3_INVALID_BITS  -2
#define MP3_OK  0
#define MP3_NEED_MORE 1


#ifdef __cplusplus
extern "C" {
#endif

BOOL MPGLIB_Init(PMPSTR mp);
void MPGLIB_Reset(PMPSTR mp);
int MPGLIB_DecodeFrame(PMPSTR               mp, 
                       const unsigned char* frame, 
                       void*                samples);
void MPGLIB_Exit(PMPSTR mp);

#ifdef __cplusplus
}
#endif

#endif /* _MPGLIB_H_ */
