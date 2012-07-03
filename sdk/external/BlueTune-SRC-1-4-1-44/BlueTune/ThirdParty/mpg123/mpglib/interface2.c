#include "mpglib_config.h"
#include <stdlib.h>
#include <string.h>

#include "mpg123.h"
#include "mpglib.h"
#include "layer1.h"
#include "layer2.h"
#include "layer3.h"
#include "common.h"
#include "tabinit.h"
#include "decode_i386.h"

void MPGLIB_Reset(PMPSTR mp) 
{
    memset(mp,0,sizeof(MPSTR));

    mp->framesize = 0;
    mp->fsizeold = -1;

    mp->fr.single = -1;
    mp->bsnum = 0;
    mp->synth_bo = 1;
}

BOOL MPGLIB_Init(PMPSTR mp) 
{
    static int init = 0;

    MPGLIB_Reset(mp);

    if(!init) {
        make_decode_tables(32767);
        init_layer2();
        init_layer3(SBLIMIT);
        init = 1;
    }

    return !0;
}

void MPGLIB_Exit(PMPSTR mp)
{
    (void)mp;
}

int MPGLIB_DecodeFrame(PMPSTR mp, const unsigned char* in, void* out)
{
    int done;
    
    /* First decode header */
    mp->header =
        (((unsigned long)in[0]) << 24) |
        (((unsigned long)in[1]) << 16) |
        (((unsigned long)in[2]) <<  8) |
        (((unsigned long)in[3]));
    decode_header(&mp->fr, mp->header);
    mp->framesize = mp->fr.framesize;

    mp->wordpointer = mp->bsspace[mp->bsnum] + 512;
    mp->bsnum = (mp->bsnum + 1) & 0x1;
    mp->bitindex = 0;

    memcpy(mp->wordpointer, in+4, mp->framesize);

    done = 0;
    if(mp->fr.error_protection)
        getbits(mp, 16);
    switch(mp->fr.lay) {
      case 1:
        do_layer1(mp,(unsigned char *) out, &done);
        break;
      case 2:
        do_layer2(mp,(unsigned char *) out, &done);
        break;
      case 3:
        do_layer3(mp,(unsigned char *) out, &done, synth_1to1_mono, synth_1to1);
        break;
    }
    
    mp->fsizeold = mp->framesize;
    mp->framesize = 0;
    
    if (done) {
        return MP3_OK;
    } else {
        return MP3_INVALID_BITS;
    }
}





