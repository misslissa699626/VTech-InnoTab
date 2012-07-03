#ifndef _COMMON_H_
#define _COMMON_H_

#include "mpglib_config.h"
#include "mpg123.h"
#include "mpglib.h"

extern const int  tabsel_123[2][3][16];
extern const long freqs[9];

#if defined( USE_LAYER_1 ) || defined ( USE_LAYER_2 )
extern real muls[27][64];
#endif

int head_check(unsigned long head,int check_layer);
int decode_header(struct frame *fr,unsigned long newhead);
unsigned int getbits(PMPSTR mp, int number_of_bits);
unsigned int getbits_fast(PMPSTR mp, int number_of_bits);
int set_pointer(PMPSTR mp, long backstep);

#endif /* _COMMON_H_ */

