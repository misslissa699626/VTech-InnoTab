/*****************************************************************
|
|      File: FloVbr.h
|
|      Fluo - Vbr (Variable Bitrate)
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo - Vbrs
 */

#ifndef _FLO_VBR_H_
#define _FLO_VBR_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"
#include "FloSyntax.h"
#include "FloFrame.h"
#include "FloByteStream.h"
#include "FloDecoder.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef unsigned long FLO_VbrTableEntry;

typedef struct {
    FLO_VbrTableEntry* entries;
    FLO_Cardinal       entry_count;
} FLO_VbrToc;

/*----------------------------------------------------------------------
|       prototypes
+---------------------------------------------------------------------*/
extern FLO_Result FLO_Vbr_Parse(FLO_FrameInfo*     frame_info,
                                FLO_ByteStream*    bits,
                                FLO_DecoderStatus* decoder_status,
                                FLO_VbrToc*        vbr_toc);

#endif /* _FLO_VBR_H_ */
