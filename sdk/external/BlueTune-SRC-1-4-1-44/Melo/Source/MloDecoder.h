/*****************************************************************
|
|    Melo - Decoder API
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
/** @file
 * Melo Decoder API
 */

#ifndef _MLO_DECODER_H_
#define _MLO_DECODER_H_

/** 
 * @defgroup MLO_Decoder Decoder API
 * @{ 
 */

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloFrame.h"
#include "MloBitStream.h"
#include "MloSampleBuffer.h"
#include "MloSamplingFreq.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
/**
 * Decoder object [opaque data structure].
 * An MLO_Decoder object is used to decode AAC audio frames.
 */
typedef struct MLO_Decoder MLO_Decoder;

/**
 * Represents the status of an MLO_Decoder object.
 */
typedef struct {
    MLO_Flags    flags;       /**< status flags                    */
    MLO_Cardinal frame_count; /**< number of frames decoded so far */
} MLO_DecoderStatus;

/**
 * Enumeration of object type identifiers. Object type identifiers
 * identify which of the many possible codecs and codec profiles 
 * an encoded stream carries.
 * This library only supports the AAC Low Complexity profile, but 
 * other object type identifiers are included here for information.
 * These identifiers are defined in: 14496-3 1.5.1.1 
 */
typedef enum {
    MLO_OBJECT_TYPE_AAC_MAIN        = 1,  /**< AAC Main Profile              */
    MLO_OBJECT_TYPE_AAC_LC          = 2,  /**< AAC Low Complexity            */
    MLO_OBJECT_TYPE_AAC_SSR         = 3,  /**< AAC Scalable Sample Rate      */
    MLO_OBJECT_TYPE_AAC_LTP         = 4,  /**< AAC Long Term Prediction           */
    MLO_OBJECT_TYPE_SBR             = 5,  /**< Spectral Band Replication          */
    MLO_OBJECT_TYPE_AAC_SCALABLE    = 6,  /**< AAC Scalable                       */
    MLO_OBJECT_TYPE_TWINVQ          = 7,  /**< Twin VQ                            */
    MLO_OBJECT_TYPE_ER_AAC_LC       = 17, /**< Error Resilient AAC Low Complexity */
    MLO_OBJECT_TYPE_ER_AAC_LTP      = 19, /**< Error Resilient AAC Long Term Prediction */
    MLO_OBJECT_TYPE_ER_AAC_SCALABLE = 20, /**< Error Resilient AAC Scalable */
    MLO_OBJECT_TYPE_ER_TWINVQ       = 21, /**< Error Resilient Twin VQ */
    MLO_OBJECT_TYPE_ER_BSAC         = 22, /**< Error Resilient Bit Sliced Arithmetic Coding */
    MLO_OBJECT_TYPE_ER_AAC_LD       = 23, /**< Error Resilient AAC Low Delay */
    MLO_OBJECT_TYPE_LAYER_1         = 32, /**< MPEG Layer 1 */
    MLO_OBJECT_TYPE_LAYER_2         = 33, /**< MPEG Layer 2 */
    MLO_OBJECT_TYPE_LAYER_3         = 34  /**< MPEG Layer 3 */
} MLO_ObjectTypeIdentifier;

/**
 * Channel configuration for multichannel audio buffers.
 */
typedef enum {
    MLO_CHANNEL_CONFIG_NONE   = 0, /**< No channel (not used)       */
    MLO_CHANNEL_CONFIG_MONO   = 1, /**< Mono (single audio channel) */
    MLO_CHANNEL_CONFIG_STEREO = 2, /**< Stereo (Two audio channels) */
    MLO_CHANNEL_CONFIG_STEREO_PLUS_CENTER = 3, /**< Stereo plus one center channel */
    MLO_CHANNEL_CONFIG_STEREO_PLUS_CENTER_PLUS_REAR_MONO = 4, /**< Stereo plus one center and one read channel */
    MLO_CHANNEL_CONFIG_FIVE = 5,           /**< Five channels */
    MLO_CHANNEL_CONFIG_FIVE_PLUS_ONE = 6,  /**< Five channels plus one low frequency channel */
    MLO_CHANNEL_CONFIG_SEVEN_PLUS_ONE = 7, /**< Seven channels plus one low frequency channel */
    MLO_CHANNEL_CONFIG_UNSUPPORTED
} MLO_ChannelConfiguration;

/**
 * Detailed decoder configuration information.
 * This information is necessary in order to create a decoder object.
 * It is normally obtained from the DecoderSpecificInfo field of the
 * DecoderConfigDescriptor descriptor carried in the sample description
 * for the audio samples. See 14496-3, subpart 1, p 1.6.2.1 for details.
 * To populate the fields of this data structure from an encoded 
 * DecoderSpecificInfo byte array, use the @ref MLO_DecoderConfig_Parse
 * function.
 */
typedef struct {
    MLO_ObjectTypeIdentifier object_type;              /**< Type identifier for the audio data */
    MLO_SamplingFreq_Index   sampling_frequency_index; /**< Index of the sampling frequency in the sampling frequency table */
    MLO_UInt32               sampling_frequency;       /**< Sampling frequency in Hz */
    MLO_ChannelConfiguration channel_configuration;    /**< Channel configuration */
    MLO_Boolean              frame_length_flag;        /**< Frame Length Flag     */
    MLO_Boolean              depends_on_core_coder;    /**< Depends on Core Coder */
    MLO_Boolean              core_coder_delay;         /**< Core Code delay       */
    /** Extension details */
    struct {
        MLO_Boolean              sbr_present;              /**< SBR is present        */
        MLO_ObjectTypeIdentifier object_type;              /**< Extension object type */
        MLO_SamplingFreq_Index   sampling_frequency_index; /**< Sampling frequency index of the extension */
        MLO_UInt32               sampling_frequency;       /**< Sampling frequency of the extension, in Hz */
    } extension;
} MLO_DecoderConfig;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define MLO_DECODER_MAX_FRAME_SIZE 8192

/** Error: the configuration or encoding is not supported by this implementation */
#define MLO_ERROR_DECODER_UNSUPPORTED_CONFIG            (MLO_ERROR_BASE_DECODER-0)

/** Error: an invalid channel configuration was encountered */
#define MLO_ERROR_DECODER_INVALID_CHANNEL_CONFIGURATION (MLO_ERROR_BASE_DECODER-1)

/** Error: the format or encoding is not supported by this implementation */
#define MLO_ERROR_DECODER_UNSUPPORTED_FORMAT            (MLO_ERROR_BASE_DECODER-2)

/** Error: invalid data */
#define MLO_ERROR_DECODER_INVALID_DATA                  (MLO_ERROR_BASE_DECODER-3)

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Parse an array of bytes containing the DecoderSpecificInfo field of a
 * DecoderConfigDescriptor descriptor. 
 * The information contained in this DecoderSpecificInfo field is
 * returned in the fields of a MLO_DecoderConfig structure.
 * See 14496-1, subpart 2, p 2.6.6 for details.
 *
 * @param encoded Pointer to an array of bytes with the encoded 
 * DecoderSpecificInfo field.
 * @param encoded_size Size in bytes of the byte array pointed to 
 * by the @c encoded parameter.
 * @param config Pointer to an MLO_DecoderConfig struct in which the
 * parsed information will be returned.
 *
 * @return MLO_SUCCESS if the encoded bytes could be successfully parsed,
 * or an error code if they could not.
 */
MLO_Result MLO_DecoderConfig_Parse(const unsigned char* encoded, 
                                   MLO_Size             encoded_size,
                                   MLO_DecoderConfig*   config);

/**
 * Returns the value of the sample rate expressed in an MLO_DecoderConfig
 * struct.
 *
 * @param config Pointer to an MLO_DecoderConfig struct of which the sample
 * rate should be returned.
 *
 * @return Sample rate in Hz.
 */
unsigned int MLO_DecoderConfig_GetSampleRate(const MLO_DecoderConfig* config);

/**
 * Returns the number of audio channels expressed in an MLO_DecoderConfig 
 * struct.
 *
 * @param config Pointer to an MLO_DecoderConfig struct of which the number of
 * audio channels should be returned.
 *
 * @return Number of audio channels.
 */
MLO_Cardinal MLO_DecoderConfig_GetChannelCount(const MLO_DecoderConfig* config);

/**
 * Create a new MLO_Decoder object.
 *
 * @param config Pointer to an MLO_DecoderConfig expressing the configuration
 * parameters of the decoder object to create.
 * @param decoder Pointer to a pointer to an MLO_Decoder object where the pointer
 * to the newly created object will be returned.
 *
 * @return MLO_SUCCESS is the decoder object was successfully created, or an 
 * error code if it could not.
 */
MLO_Result MLO_Decoder_Create(const MLO_DecoderConfig* config,
                              MLO_Decoder**            decoder);

/**
 * Destroy an MLO_Decoder object and free the resources associated with it.
 *
 * @param decoder Pointer to the decoder object to destroy.
 */
MLO_Result MLO_Decoder_Destroy(MLO_Decoder* decoder);

/**
 * Reset the internal state of an MLO_Decoder object. 
 * This flushed all internal buffers and returns the decoder to a state
 * equivalent to the state of a newly created object.
 */
MLO_Result MLO_Decoder_Reset(MLO_Decoder* decoder);

/**
 * Decode a single audio frame.
 *
 * @param decoder Pointer to an MLO_Decoder object.
 * @param frame Pointer to an array of bytes containing a frame of
 * AAC compressed audio data. The caller must ensure that this
 * array of bytes contains at least one entire AAC frame.
 * @param frame_size Size in bytes of the AAC frame data.
 * @param sample Pointer to an MLO_SampleBuffer object encapsulating 
 * the information about the decoded audio buffer will be returned.
 * 
 * @return MLO_SUCCESS if the frame could be decoded, or an error code
 * if it could not.
 */
MLO_Result MLO_Decoder_DecodeFrame(MLO_Decoder*       decoder,
                                   const MLO_Byte*    frame,
                                   MLO_Size           frame_size,
                                   MLO_SampleBuffer*  sample);

/**
 * Return the current status of an MLO_Decoder object.
 *
 * @param decoder Pointer to the MLO_Decoder object of which the status
 * will be returned.
 * @param status Pointer to a pointer where a pointer to an 
 * MLO_DecoderStatus struct will be returned. The caller must be aware
 * that the information pointed to by this pointer cannot no longer
 * be used after the next call to any of the MLO_Decoder functions.
 */
MLO_Result MLO_Decoder_GetStatus(MLO_Decoder*        decoder, 
                                 MLO_DecoderStatus** status);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _MLO_DECODER_H_ */
