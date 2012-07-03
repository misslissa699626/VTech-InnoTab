/*****************************************************************
|
|   Fluo - Headers (VBR and other Headers)
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo - Headers
 */

#ifndef _FLO_HEADERS_H_
#define _FLO_HEADERS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"
#include "FloSyntax.h"
#include "FloFrame.h"
#include "FloByteStream.h"
#include "FloDecoder.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef unsigned long FLO_VbrTableEntry;

typedef struct {
    FLO_VbrTableEntry* entries;
    FLO_Cardinal       entry_count;
} FLO_VbrToc;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
extern FLO_Result FLO_Headers_Parse(FLO_FrameInfo*     frame_info,
                                    FLO_ByteStream*    bits,
                                    FLO_DecoderStatus* decoder_status,
                                    FLO_VbrToc*        vbr_toc);

#endif /* _FLO_HEADERS_H_ */
