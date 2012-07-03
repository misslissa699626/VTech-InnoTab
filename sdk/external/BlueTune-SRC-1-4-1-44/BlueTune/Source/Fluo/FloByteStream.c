/*****************************************************************
|
|   Fluo - Byte Streams
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   For efficiency reasons, this byte stream library only handles
|   data buffers that are a power of 2 in size
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "FloConfig.h"
#include "FloTypes.h"
#include "FloByteStream.h"
#include "FloFrame.h"
#include "FloDecoder.h"

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define FLO_BYTE_STREAM_POINTER_VAL(offset) \
    ((offset)&(FLO_BYTE_STREAM_BUFFER_SIZE-1))

#define FLO_BYTE_STREAM_POINTER_OFFSET(pointer, offset) \
    (FLO_BYTE_STREAM_POINTER_VAL((pointer)+(offset)))

#define FLO_BYTE_STREAM_POINTER_ADD(pointer, offset) \
    ((pointer) = FLO_BYTE_STREAM_POINTER_OFFSET(pointer, offset))

/* mask the self for header params that should not change */
#define FLO_BYTE_STREAM_HEADER_COMPAT_MASK 0xFFFF0D0F

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define FLO_FHG_VBRI_OFFSET 36

/*----------------------------------------------------------------------
|   FLO_ByteStream_Construct
+---------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_Construct(FLO_ByteStream* self)
{
    self->buffer = 
        (unsigned char*)ATX_AllocateMemory(FLO_BYTE_STREAM_BUFFER_SIZE);
    if (self->buffer == NULL) return FLO_ERROR_OUT_OF_MEMORY;

    FLO_ByteStream_Reset(self);

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_Destruct
+---------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_Destruct(FLO_ByteStream* self)
{
    FLO_ByteStream_Reset(self);
    if (self->buffer != NULL) {
        ATX_FreeMemory(self->buffer);
        self->buffer = 0;
    }

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_Reset
+---------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_Reset(FLO_ByteStream* self)
{
    self->in    = 0;
    self->out   = 0;
    self->flags = 0;

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_Attach
+---------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_Attach(FLO_ByteStream* self, FLO_ByteStream* shadow)
{
    *shadow = *self;
    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_GetContiguousBytesFree
+---------------------------------------------------------------------*/
unsigned int 
FLO_ByteStream_GetContiguousBytesFree(FLO_ByteStream* self)
{
    return 
        (self->in < self->out) ?
        (self->out - self->in - 1) :
        (FLO_BYTE_STREAM_BUFFER_SIZE - self->in);
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_GetBytesFree
+---------------------------------------------------------------------*/
unsigned int 
FLO_ByteStream_GetBytesFree(FLO_ByteStream* self)
{
    return  
        (self->in < self->out) ? 
        (self->out - self->in - 1) : 
        (FLO_BYTE_STREAM_BUFFER_SIZE  + (self->out - self->in) - 1);
}

/*----------------------------------------------------------------------+
|    FLO_ByteStream_WriteBytes
+----------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_WriteBytes(FLO_ByteStream*       self, 
                         const unsigned char* bytes, 
                         unsigned int         byte_count)
{
    if (byte_count == 0) return FLO_SUCCESS;
    if (bytes == NULL) return FLO_ERROR_INVALID_PARAMETERS;

    if (self->in < self->out) {
        ATX_CopyMemory(self->buffer+self->in, bytes, byte_count);
        FLO_BYTE_STREAM_POINTER_ADD(self->in, byte_count);
    } else {
        unsigned int chunk = FLO_BYTE_STREAM_BUFFER_SIZE - self->in;
        if (chunk > byte_count) chunk = byte_count;

        ATX_CopyMemory(self->buffer+self->in, bytes, chunk);
        FLO_BYTE_STREAM_POINTER_ADD(self->in, chunk);

        if (chunk != byte_count) {
            ATX_CopyMemory(self->buffer+self->in, 
                           bytes+chunk, byte_count-chunk);
            FLO_BYTE_STREAM_POINTER_ADD(self->in, byte_count-chunk);
        }
    }

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_GetContiguousBytesAvailable
+---------------------------------------------------------------------*/
unsigned int 
FLO_ByteStream_GetContiguousBytesAvailable(FLO_ByteStream* self)
{
    return 
        (self->out <= self->in) ? 
        (self->in - self->out) :
        (FLO_BYTE_STREAM_BUFFER_SIZE - self->out);
}

/*----------------------------------------------------------------------
|   FLO_ByteStream_GetBytesAvailable
+---------------------------------------------------------------------*/
unsigned int 
FLO_ByteStream_GetBytesAvailable(FLO_ByteStream* self)
{
    return 
        (self->out <= self->in) ? 
        (self->in - self->out) :
        (self->in + (FLO_BYTE_STREAM_BUFFER_SIZE - self->out));
}

/*----------------------------------------------------------------------+
|    FLO_ByteStream_ReadBytes
+----------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_ReadBytes(FLO_ByteStream* self, 
                        unsigned char* bytes, 
                        unsigned int   byte_count)
{
    if (byte_count == 0 || bytes == NULL) {
        return FLO_ERROR_INVALID_PARAMETERS;
    }
    if (self->in > self->out) {
        ATX_CopyMemory(bytes, self->buffer+self->out, byte_count);
        FLO_BYTE_STREAM_POINTER_ADD(self->out, byte_count);
    } else {
        unsigned int chunk = FLO_BYTE_STREAM_BUFFER_SIZE - self->out;
        if (chunk >= byte_count) chunk = byte_count;

        ATX_CopyMemory(bytes, self->buffer+self->out, chunk);
        FLO_BYTE_STREAM_POINTER_ADD(self->out, chunk);

        if (chunk != byte_count) {
            ATX_CopyMemory(bytes+chunk, 
                           self->buffer+self->out, 
                           byte_count-chunk);
            FLO_BYTE_STREAM_POINTER_ADD(self->out, byte_count-chunk);
        }
    }

    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|    FLO_ByteStream_SkipBytes
+----------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_SkipBytes(FLO_ByteStream* self, unsigned int byte_count)
{
    FLO_BYTE_STREAM_POINTER_ADD(self->out, byte_count);
    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|    FLO_ByteStream_AlignToByte
+----------------------------------------------------------------------*/
static FLO_Result
FLO_ByteStream_AlignToByte(FLO_ByteStream* self)
{
    ATX_COMPILER_UNUSED(self);
    return FLO_SUCCESS;
}

/*----------------------------------------------------------------------+
|    FLO_ByteStream_FindHeader
+----------------------------------------------------------------------*/
static FLO_Result
FLO_ByteStream_FindHeader(FLO_ByteStream* self, unsigned long* header)
{
    unsigned long packed;
    int           available = FLO_ByteStream_GetBytesAvailable(self);
    unsigned int  current   = self->out;

    /* read the first 32 self */
    if (available < 4) return FLO_ERROR_NOT_ENOUGH_DATA;
    packed =                 self->buffer[current];
    FLO_BYTE_STREAM_POINTER_ADD(current, 1);
    packed = (packed << 8) | self->buffer[current];
    FLO_BYTE_STREAM_POINTER_ADD(current, 1);
    packed = (packed << 8) | self->buffer[current];
    FLO_BYTE_STREAM_POINTER_ADD(current, 1);
    packed = (packed << 8) | self->buffer[current];
    FLO_BYTE_STREAM_POINTER_ADD(current, 1);
    available -= 4;

    /* read until we find a header or run out of data */
    for (;/* ever */;) {
        /* check if we have a sync word */
        if (((packed >> (32 - FLO_SYNTAX_MPEG_SYNC_WORD_BIT_LENGTH)) & 
             ((1<<FLO_SYNTAX_MPEG_SYNC_WORD_BIT_LENGTH)-1)) == 
            FLO_SYNTAX_MPEG_SYNC_WORD) {
            /* rewind to start of header */
            self->out = FLO_BYTE_STREAM_POINTER_OFFSET(current, -4);

            /* return the header */
            *header = packed;
            return FLO_SUCCESS;
        }

        /* move on to the next byte */
        if (available > 0) {
            packed = (packed << 8) | self->buffer[current];
            FLO_BYTE_STREAM_POINTER_ADD(current, 1);
            available --;
        } else {
            break;
        }
    }

    /* discard all the bytes we've already scanned, except the last 3 */
    self->out = FLO_BYTE_STREAM_POINTER_OFFSET(current, -3);

    return FLO_ERROR_NOT_ENOUGH_DATA;
}

/*----------------------------------------------------------------------+
|    FLO_ByteStream_FindFrame
+----------------------------------------------------------------------*/
FLO_Result
FLO_ByteStream_FindFrame(FLO_ByteStream* self, FLO_FrameInfo* frame_info)
{
    unsigned int    available;
    unsigned long   packed;
    FLO_FrameHeader frame_header;
    FLO_Result      result;

    /* align to the start of the next byte */
    FLO_ByteStream_AlignToByte(self);
    
    /* find a frame header */
    result = FLO_ByteStream_FindHeader(self, &packed);
    if (FLO_FAILED(result)) return result;

    /* unpack the header */
    FLO_FrameHeader_Unpack(packed, &frame_header);

    /* check the header */
    result = FLO_FrameHeader_Check(&frame_header);
    if (FLO_FAILED(result)) {
        /* skip the header and return */
        FLO_BYTE_STREAM_POINTER_ADD(self->out, 1);
        return FLO_ERROR_INVALID_BITSTREAM;
    }

    /* get the frame info */
    FLO_FrameHeader_GetInfo(&frame_header, frame_info);

    /* check that we have enough data */
    available = FLO_ByteStream_GetBytesAvailable(self);
    if (self->flags & FLO_BYTE_STREAM_FLAG_EOS) {
        /* we're at the end of the stream, we only need the entire frame */
        if (available < frame_info->size) {
            return FLO_ERROR_NOT_ENOUGH_DATA;
        } 
    } else {
        /* peek at the header of the next frame */
        unsigned int    peek_offset;
        unsigned long   peek_packed;
        FLO_FrameHeader peek_header;

        if (available < frame_info->size+4) {
            return FLO_ERROR_NOT_ENOUGH_DATA;
        } 
        peek_offset = FLO_BYTE_STREAM_POINTER_OFFSET(self->out, 
                                                   frame_info->size);
        peek_packed =                      self->buffer[peek_offset];
        FLO_BYTE_STREAM_POINTER_ADD(peek_offset, 1);
        peek_packed = (peek_packed << 8) | self->buffer[peek_offset];
        FLO_BYTE_STREAM_POINTER_ADD(peek_offset, 1);
        peek_packed = (peek_packed << 8) | self->buffer[peek_offset];
        FLO_BYTE_STREAM_POINTER_ADD(peek_offset, 1);
        peek_packed = (peek_packed << 8) | self->buffer[peek_offset];
        /* check the following header */
        FLO_FrameHeader_Unpack(peek_packed, &peek_header);
        result = FLO_FrameHeader_Check(&peek_header);
        if (FLO_FAILED(result) || 
            ((peek_packed & FLO_BYTE_STREAM_HEADER_COMPAT_MASK) != 
             (packed      & FLO_BYTE_STREAM_HEADER_COMPAT_MASK))) {
            /* the next header is invalid, or incompatible, so we reject  */
            /* this one, unless it has an FHG VBRI header.                */
            /* it is necessary to check for the VBRI header because the   */
            /* MusicMatch encoder puts the VBRI header in the first frame */
            /* but incorrectly computes the frame size, so there is       */
            /* garbage at the end of the frame, which would cause it to   */
            /* get rejected here...                                       */
            char header[4];
            peek_offset = FLO_BYTE_STREAM_POINTER_OFFSET(self->out,
                                                       FLO_FHG_VBRI_OFFSET);
            header[0] = self->buffer[peek_offset];
            FLO_BYTE_STREAM_POINTER_ADD(peek_offset, 1);
            header[1] = self->buffer[peek_offset];
            FLO_BYTE_STREAM_POINTER_ADD(peek_offset, 1);
            header[2] = self->buffer[peek_offset];
            FLO_BYTE_STREAM_POINTER_ADD(peek_offset, 1);
            header[3] = self->buffer[peek_offset];
            if (header[0] != 'V' || 
                header[1] != 'B' ||
                header[2] != 'R' ||
                header[3] != 'I') {
                FLO_BYTE_STREAM_POINTER_ADD(self->out, 1);
                return FLO_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    return FLO_SUCCESS;
}
