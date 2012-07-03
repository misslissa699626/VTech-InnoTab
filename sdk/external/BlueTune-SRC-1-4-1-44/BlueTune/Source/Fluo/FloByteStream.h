/*****************************************************************
|
|   Fluo - Bit Streams
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo - Bit Streams
 */

#ifndef _FLO_BYTE_STREAM_H_
#define _FLO_BYTE_STREAM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"
#include "FloErrors.h"
#include "FloFrame.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

/* smallest power of 2 that can hold any type of frame */
#define FLO_BYTE_STREAM_BUFFER_SIZE  2048

/* flags */
#define FLO_BYTE_STREAM_FLAG_EOS 0x01

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    unsigned char* buffer;
    unsigned int   in;
    unsigned int   out;
    unsigned int   flags;
} FLO_ByteStream;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
FLO_Result   FLO_ByteStream_Construct(FLO_ByteStream* self);
FLO_Result   FLO_ByteStream_Destruct(FLO_ByteStream* self);
FLO_Result   FLO_ByteStream_Reset(FLO_ByteStream* self);
FLO_Result   FLO_ByteStream_Attach(FLO_ByteStream* self, FLO_ByteStream* shadow);
unsigned int FLO_ByteStream_GetContiguousBytesFree(FLO_ByteStream* self);
unsigned int FLO_ByteStream_GetBytesFree(FLO_ByteStream* self);
FLO_Result   FLO_ByteStream_WriteBytes(FLO_ByteStream*       self, 
                                      const unsigned char* bytes, 
                                      unsigned int         byte_count);
unsigned int FLO_ByteStream_GetContiguousBytesAvailable(FLO_ByteStream* self);
unsigned int FLO_ByteStream_GetBytesAvailable(FLO_ByteStream* self);
FLO_Result   FLO_ByteStream_ReadBytes(FLO_ByteStream* self, 
                                      unsigned char* bytes, 
                                      unsigned int   byte_count);
FLO_Result   FLO_ByteStream_SkipBytes(FLO_ByteStream* self, 
                                      unsigned int   byte_count);
FLO_Result   FLO_ByteStream_FindFrame(FLO_ByteStream* self, 
                                      FLO_FrameInfo* frame_info);

#endif /* _FLO_BYTE_STREAM_H_ */
