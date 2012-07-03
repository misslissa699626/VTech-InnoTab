/*****************************************************************
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/
/**
 * @file 
 * Sample Buffer objects.
 */

#ifndef _MLO_SAMPLE_BUFFER_H_
#define _MLO_SAMPLE_BUFFER_H_
/**
 * @defgroup MLO_SampleBuffer PCM Sample Buffers
 * @{ 
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "MloTypes.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
/**
 * MLO_SampleBuffer object.
 * An MLO_SampleBuffer object represents a buffer of PCM audio samples.
 */
typedef struct MLO_SampleBuffer MLO_SampleBuffer;

/**
 * Sample type indicating the way PCM sample values are encoded and 
 * layed out in memory.
 */
typedef enum
{
    /**
     * Sample values are encoded as signed integers and the value for
     * multiple audio channels are interleaved in memory.
     * For example, samples for stereo audio (two channels) are layed out
     * in memory as: LRLRLRLRL... where L represents the value of a sample
     * from the left channel, and R the value of a sample from the right
     * channel.
     */
    MLO_SAMPLE_TYPE_INTERLACED_SIGNED = 0 
} MLO_SampleType;

/**
 * Information about the encoding, memory layout, sample rate and number of 
 * channels of the PCM audio data represented by an MLO_SampleBuffer object.
 */
typedef struct
{
   MLO_SampleType type;            /**< Encoding and memory layout of samples.    */
   MLO_Cardinal   sample_rate;     /**< Sample rate in Hz.                        */
   MLO_Cardinal   channel_count;   /**< Number of channels.                       */
   MLO_Cardinal   bits_per_sample; /**< Number of bits per sample (multiple of 8) */
} MLO_SampleFormat;

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Create a new instance of an MLO_SampleBuffer object.
 * NOTE: this method is not intended to be called by clients of the 
 * MLO_Decoder API, since this API never takes MLO_SampleBuffer objects
 * as input (always as output).
 *
 * @param size Size in bytes of the buffer that this object should
 * encapsulate.
 * @param buffer Pointer to a pointer where a pointer to an MLO_SampleBuffer
 * object will be returned.
 * 
 * @return MLO_SUCCESS if the object could be created, or an error code
 * if it could not.
 */
MLO_Result 
MLO_SampleBuffer_Create(MLO_Size size, MLO_SampleBuffer** buffer);

/**
 * Destroy an MLO_SampleBuffer object and free the resources associated 
 * with it.
 *
 * @param self Pointer to the MLO_SampleBuffer object to destroy.
 *
 * @return MLO_SUCCESS if the call succeeded, or an error code if it failed.
 */
MLO_Result 
MLO_SampleBuffer_Destroy(MLO_SampleBuffer* self);

/**
 * Return a (const) pointer to the memory buffer containing the PCM audio samples
 * encapsulated by an MLO_SampleBuffer object.
 * The returned pointer is maked 'const' to indicate that the caller may read
 * but not write to the memory buffer.
 * 
 * @param self Pointer to the MLO_SampleBuffer object of which the samples
 * should be retuned.
 *
 * @return Const pointer to a memory buffer. The encoding and layout of the 
 * PCM sample values in the buffer are determined by the MLO_SampleFormat
 * format of this object.
 */
const void* 
MLO_SampleBuffer_GetSamples(const MLO_SampleBuffer* self);

/**
 * Return a pointer to the memory buffer containing the PCM audio samples
 * encapsulated by an MLO_SampleBuffer object.
 * This method is similar to the MLO_SampleBuffer_GetSamples except that the
 * returned pointer is not marked 'const', indicating that the caller may
 * write to the memory buffer.
 * 
 * @param self Pointer to the MLO_SampleBuffer object of which the samples
 * should be retuned.
 *
 * @return Const pointer to a memory buffer. The encoding and layout of the 
 * PCM sample values in the buffer are determined by the MLO_SampleFormat
 * format of this object.
 */
void* 
MLO_SampleBuffer_UseSamples(MLO_SampleBuffer* self);

/**
 * Return the number of audio samples encapsulated in a MLO_SampleBuffer object.
 * 
 * @param self Pointer to the MLO_SampleBuffer object of which the sample count
 * should be retuned.
 *
 * @return The number of audio samples encapsulated by the MLO_SampleBuffer object.
 * Each 'sample' contains the data for all the channels at any given sampling point.
 * Thus, each sample is <channel_count>*<bits_per_sample> bits. For example, a
 * buffer containing 1 second of audio at 44.1kHz contains 44100 samples, regardless
 * of the numbe of channels.
 */
MLO_Cardinal   
MLO_SampleBuffer_GetSampleCount(const MLO_SampleBuffer* self);

/**
 * Set the number of audio samples encapsulated in a MLO_SampleBuffer object.
 * This method is typically only used by the internal implementation of the decoder,
 * not by clients of the decoder API, since the decoder API never takes MLO_SampleBuffer
 * objects as input.
 *
 * @param self Pointer to the MLO_SampleBuffer object of which the sample count
 * should be set.
 *
 * @return MLO_SUCCESS if the call succeeded, or an error code if it failed.
 */
MLO_Result 
MLO_SampleBuffer_SetSampleCount(MLO_SampleBuffer* self, MLO_Cardinal sample_count);

/**
 * Return the size of the buffer encapsulated in a MLO_SampleBuffer object.
 * 
 * @param self Pointer to the MLO_SampleBuffer object of which the buffer size
 * should be retuned.
 *
 * @return The number of bytes in the buffer encapsulated by the MLO_SampleBuffer object.
 */
MLO_Size       
MLO_SampleBuffer_GetSize(const MLO_SampleBuffer* self);

/**
 * Return the format of the PCM samples encapsulated by an MLO_SampleBuffer object.
 * 
 * @param self Pointer to the MLO_SampleBuffer object of which the format
 * should be retuned.
 *
 * @return Pointer to an MLO_SampleFormat struct containing the details of the 
 * format of the PCM samples encapsulated by this MLO_SampleBuffer object.
 */
const MLO_SampleFormat* 
MLO_SampleBuffer_GetFormat(const MLO_SampleBuffer* self);

/**
 * Sets the format of the PCM samples encapsulated by an MLO_SampleBuffer object.
 * NOTE: this method only sets the format information, without doing any conversion
 * on any existing data in the buffer.
 * This method is typically only used by the internal implementation of the decoder,
 * not by clients of the decoder API, since the decoder API never takes MLO_SampleBuffer
 * objects as input.
 * 
 * @param self Pointer to the MLO_SampleBuffer object of which the format
 * should be set.
 * @param format Pointer to an MLO_SampleFormat struct containing the details of the 
 * format of the PCM samples encapsulated by this MLO_SampleBuffer object.
 *
 */
void 
MLO_SampleBuffer_SetFormat(MLO_SampleBuffer* self, const MLO_SampleFormat* format);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _MLO_SAMPLE_BUFFER_H_ */
