#ifndef _DECODE_I386_H_
#define _DECODE_I386_H_

#include "mpglib_config.h"
#include "common.h"

int synth_1to1_mono(PMPSTR mp, real *bandPtr,unsigned char *out,int *pnt);
int synth_1to1(PMPSTR mp, real *bandPtr,int channel,unsigned char *out,int *pnt);

int synth_1to1_mono_unclipped(PMPSTR mp, real *bandPtr,unsigned char *out,int *pnt);
int synth_1to1_unclipped(PMPSTR mp, real *bandPtr,int channel,unsigned char *out,int *pnt);

#endif /* _DECODE_I386_H_ */

