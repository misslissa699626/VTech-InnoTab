/*****************************************************************
|
|   Fluo - Layer I Decoding
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*-------------------------------------------------------------------------
|   includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloLayerI.h"
#include "FloFrame.h"
#include "FloTables.h"
#include "FloFilter.h"
#include "FloSyntax.h"
#include "FloMath.h"
#include "FloErrors.h"
#include "FloDecoder.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|   FLO_LayerI_InitFrame
+-------------------------------------------------------------------------*/
static FLO_Result 
FLO_LayerI_InitFrame(FLO_Frame_I* frame)
{
    if (frame->header.mode == FLO_MPEG_MODE_JOINT_STEREO) {
        /* joint stereo bound */
        /* 00 -> 4, 01 -> 8, 10 -> 12, 11 -> 16 */
        frame->joint_stereo_bound = 4*(1+frame->header.mode_extension);
    } else {
        frame->joint_stereo_bound = 0;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|   FLO_LayerI_GetFrameSize
+-------------------------------------------------------------------------*/
unsigned int
FLO_LayerI_GetFrameSize(FLO_FrameHeader *header)
{
    int slots;

    slots = (12 * FLO_MpegBitrates[header->id][3-header->layer][header->bitrate_index]*1000)
        / FLO_MpegSamplingFrequencies[header->id][header->sampling_frequency];
    if (header->sampling_frequency == FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_44100_22050
        && header->padding_bit) slots++;
    return slots * FLO_MPEG_LAYER_I_BYTES_PER_SLOT;
}

/*-------------------------------------------------------------------------
|   FLO_LayerI_ReadBitAllocations
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerI_ReadBitAllocations(FLO_BitStream* bits, FLO_Frame_I* frame)
{
    FLO_Subband_I* subband = frame->subbands;
    int            i;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* non joint stereo, read 2 allocations */
        for (i = 0; i < FLO_MPEG_LAYER_I_SUBBANDS; i++, subband++) {
            subband->allocation[0] = FLO_BitStream_ReadBits(bits, 4);
            subband->allocation[1] = FLO_BitStream_ReadBits(bits, 4);
            
            if (subband->allocation[0] == FLO_SYNTAX_MPEG_LAYER_I_ALLOCATION_INVALID) {
                return FLO_ERROR_INVALID_BITSTREAM;
            }

            if (subband->allocation[1] == FLO_SYNTAX_MPEG_LAYER_I_ALLOCATION_INVALID) {
                return FLO_ERROR_INVALID_BITSTREAM;
            }
        }
        break;
        
      case FLO_MPEG_MODE_JOINT_STEREO:
        /* joint stereo, before bound, read 2 allocations */
        for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
            subband->allocation[0] = FLO_BitStream_ReadBits(bits, 4);
            subband->allocation[1] = FLO_BitStream_ReadBits(bits, 4);
            
            if (subband->allocation[0] == FLO_SYNTAX_MPEG_LAYER_I_ALLOCATION_INVALID) {
                return FLO_ERROR_INVALID_BITSTREAM;
            }

            if (subband->allocation[1] == FLO_SYNTAX_MPEG_LAYER_I_ALLOCATION_INVALID){
                return FLO_ERROR_INVALID_BITSTREAM;
            }
        }
        /* FALLTHROUGH */

      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono, or joint stereo after bound, read 1 allocation */
        for (i = frame->joint_stereo_bound; 
             i < FLO_MPEG_LAYER_I_SUBBANDS; 
             i++, subband++) {
            subband->allocation[0] = FLO_BitStream_ReadBits(bits, 4);

            if (subband->allocation[0] == FLO_SYNTAX_MPEG_LAYER_I_ALLOCATION_INVALID) {
                return FLO_ERROR_INVALID_BITSTREAM;
            }
        }
        break;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|   FLO_LayerI_ReadScalefactors
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerI_ReadScalefactors(FLO_BitStream* bits, FLO_Frame_I* frame)
{
    FLO_Subband_I* subband = frame->subbands;
    int            i;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* non joint stereo, 2 scalefactors */
        for (i = 0; i < FLO_MPEG_LAYER_I_SUBBANDS; i++, subband++) {
            if (subband->allocation[0]) {
                int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                    return FLO_ERROR_INVALID_BITSTREAM;
                }
                subband->scalefactor[0] = FLO_Scalefactors[scalefactor];
            }
            if (subband->allocation[1]) {
                int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                    return FLO_ERROR_INVALID_BITSTREAM;
                }
                subband->scalefactor[1] = FLO_Scalefactors[scalefactor];
            }
        }
        break;

      case FLO_MPEG_MODE_JOINT_STEREO:
        /* joint stereo before bound, 2 scalefactors */
        for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
            if (subband->allocation[0]) {
                int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                    return FLO_ERROR_INVALID_BITSTREAM;
                }
                subband->scalefactor[0] = FLO_Scalefactors[scalefactor];
            }
            if (subband->allocation[1]) {
                int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                    return FLO_ERROR_INVALID_BITSTREAM;
                }
                subband->scalefactor[1] = FLO_Scalefactors[scalefactor];
            }
        }
        /* joint stereo after bound, 2 scalefactors but one allocation */
        for (i = frame->joint_stereo_bound; 
             i < FLO_MPEG_LAYER_I_SUBBANDS; 
             i++, subband++) {
            if (subband->allocation[0]) {
                {
                    int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                    if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                        return FLO_ERROR_INVALID_BITSTREAM;
                    }
                    subband->scalefactor[0] = FLO_Scalefactors[scalefactor];
                }
                {
                    int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                    if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                        return FLO_ERROR_INVALID_BITSTREAM;
                    }
                    subband->scalefactor[1] = FLO_Scalefactors[scalefactor];
                }
            }
        }
        break;

      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono, 1 scalefactor */
        for (i = 0; i < FLO_MPEG_LAYER_I_SUBBANDS; i++, subband++) {
            if (subband->allocation[0]) {
                int scalefactor = FLO_BitStream_ReadBits(bits, 6);
                if (scalefactor == FLO_SYNTAX_MPEG_LAYER_I_II_SCALEFACTOR_INVALID) {
                    return FLO_ERROR_INVALID_BITSTREAM;
                }
                subband->scalefactor[0] = FLO_Scalefactors[scalefactor];
            }
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|   inverse quantization macros
+-------------------------------------------------------------------------*/
#ifdef FLO_CONFIG_INTEGER_DECODE
#define FLO_FQUANT(quantized, allocation, scalefactor)                       \
FLO_FC1_MUL(scalefactor, ((quantized)+FLO_LayerI_QuantOffsets[allocation]) * \
  FLO_LayerI_QuantFactors[allocation])
#else
#define FLO_FQUANT(quantized, allocation, scalefactor) \
(((quantized)+FLO_LayerI_QuantOffsets[allocation]) *   \
  FLO_LayerI_QuantFactors[allocation]*(scalefactor))
#endif /* FLO_CONFIG_INTEGER_DECODE */

/*-------------------------------------------------------------------------
|   FLO_LayerI_ReadSamples
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerI_ReadSamples(FLO_BitStream* bits, FLO_Frame_I* frame)
{
    int i;
    int s;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* dual channel or stereo, read 2 samples */
        for (s = 0; s < FLO_MPEG_LAYER_I_NB_SAMPLES; s++) { 
            FLO_Subband_I *subband = frame->subbands;
            
            for (i = 0; i < FLO_MPEG_LAYER_I_SUBBANDS; i++, subband++) {
                FLO_Float quantized;

                if (subband->allocation[0]) {
                    quantized = (FLO_Float)FLO_BitStream_ReadBits(bits, 
                                                                  subband->allocation[0]+1);
                    subband->samples[0][s] = FLO_FQUANT(quantized, 
                                                        subband->allocation[0],
                                                        subband->scalefactor[0]);
                } else {
                    subband->samples[0][s] = FLO_ZERO;
                }
                if (subband->allocation[1]) {
                    quantized = (FLO_Float)FLO_BitStream_ReadBits(bits, 
                                                                  subband->allocation[1]+1);
                    subband->samples[1][s] = FLO_FQUANT(quantized,
                                                        subband->allocation[1],
                                                        subband->scalefactor[1]);
                } else {
                    subband->samples[1][s] = FLO_ZERO;
                }
            } 
        }
        break;

      case FLO_MPEG_MODE_JOINT_STEREO:
        for (s = 0; s < FLO_MPEG_LAYER_I_NB_SAMPLES; s++) { 
            FLO_Subband_I *subband = frame->subbands;
        
            /* joint stereo before bound */
            for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
                if (subband->allocation[0]) {
                    FLO_Float quantized;

                    quantized = (FLO_Float)FLO_BitStream_ReadBits(bits, 
                                                                  subband->allocation[0]+1);
                    subband->samples[0][s] = FLO_FQUANT(quantized,
                                                        subband->allocation[0],
                                                        subband->scalefactor[0]);
                } else {
                    subband->samples[0][s] = FLO_ZERO;
                }
                if (subband->allocation[1]) {
                    FLO_Float quantized;

                    quantized = (FLO_Float)FLO_BitStream_ReadBits(bits, 
                                                                  subband->allocation[1]+1);
                    subband->samples[1][s] = FLO_FQUANT(quantized,
                                                        subband->allocation[1],
                                                        subband->scalefactor[1]);
                } else {
                    subband->samples[1][s] = FLO_ZERO;
                }
            } 

            /* joint stereo after bound */
            for (i = frame->joint_stereo_bound; 
                 i < FLO_MPEG_LAYER_I_SUBBANDS; 
                 i++, subband++) {
                if (subband->allocation[0]) {
                    FLO_Float quantized;

                    quantized = (FLO_Float)FLO_BitStream_ReadBits(bits, 
                                                                  subband->allocation[0]+1);
                    subband->samples[0][s] = FLO_FQUANT(quantized,
                                                        subband->allocation[0],
                                                        subband->scalefactor[0]);
                    subband->samples[1][s] = FLO_FQUANT(quantized,
                                                        subband->allocation[0],
                                                        subband->scalefactor[1]);
                } else {
                    subband->samples[0][s] = subband->samples[1][s] = FLO_ZERO;
                }
            } 
        }
        break;
        
      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono or joint stereo, read 1 sample */
        for (s = 0; s < FLO_MPEG_LAYER_I_NB_SAMPLES; s++) { 
            FLO_Subband_I *subband = frame->subbands;
            for (i = 0; i < FLO_MPEG_LAYER_I_SUBBANDS; i++, subband++) {
                if (subband->allocation[0]) {
                    FLO_Float quantized;

                    quantized = (FLO_Float)FLO_BitStream_ReadBits(bits, 
                                                                  subband->allocation[0]+1);
                    subband->samples[0][s] = FLO_FQUANT(quantized,
                                                        subband->allocation[0],
                                                        subband->scalefactor[0]);
                } else {
                    subband->samples[0][s] = FLO_ZERO;
                }
            }
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|   FLO_LayerI_ComputePcm
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerI_ComputePcm(FLO_Frame_I*         frame, 
                      FLO_SynthesisFilter* filter_left,
                      FLO_SynthesisFilter* filter_right)
{
    int s;
    int nb_subbands = FLO_MPEG_LAYER_I_SUBBANDS;

    /* prepare for subsampling */
    {
        int subsampling;
        subsampling = filter_left ? 
            filter_left->subsampling : filter_right->subsampling;
        if (subsampling) {
            nb_subbands = FLO_MPEG_LAYER_I_SUBBANDS >> subsampling;
        }
    }

    if (filter_left == filter_right) {  
        /* mix left + right */
        filter_left->input = frame->samples;
        for (s=0; s<FLO_MPEG_LAYER_I_NB_SAMPLES; s++) {
            int i;
            FLO_Subband_I* subband = frame->subbands; 
            FLO_Float* samples = filter_left->input;
            for (i=0; i<nb_subbands; i++, subband++) {
                *samples++ = 
                    FLO_FDIV2((subband->samples[0][s] + subband->samples[1][s]));
            }
            FLO_SynthesisFilter_ComputePcm(filter_left);
        }    
    } else {
        if (filter_left) {
            filter_left->input = frame->samples;
            for (s=0; s<FLO_MPEG_LAYER_I_NB_SAMPLES; s++) {
                int i;
                FLO_Subband_I* subband = frame->subbands; 
                FLO_Float*     samples = filter_left->input;
                for (i=0; i<nb_subbands; i++) {
                    *samples++ = subband++->samples[0][s];
                }
                FLO_SynthesisFilter_ComputePcm(filter_left);
            }    
        }
        if (filter_right) {
            filter_right->input = frame->samples;
            for (s=0; s<FLO_MPEG_LAYER_I_NB_SAMPLES; s++) {
                int i;
                FLO_Subband_I* subband = frame->subbands;
                FLO_Float*     samples = filter_right->input;
                for (i=0; i<nb_subbands; i++) {
                    *samples++ = subband++->samples[1][s];
                }
                FLO_SynthesisFilter_ComputePcm(filter_right);
            }
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|   FLO_LayerI_DecodeFrame
+-------------------------------------------------------------------------*/
FLO_Result
FLO_LayerI_DecodeFrame(const unsigned char* frame_data,
                       const FLO_FrameInfo* frame_info,
                       FLO_Frame_I*         frame,
                       FLO_SynthesisFilter* filter_left, 
                       FLO_SynthesisFilter* filter_right)
{
    /* setup the bitstream */
    FLO_BitStream bits;
    FLO_BitStream_SetData(&bits, frame_data, frame_info->size-4);

    /* decode the frame */
    FLO_CHECK(FLO_LayerI_InitFrame(frame));
    FLO_CHECK(FLO_LayerI_ReadBitAllocations(&bits, frame));
    FLO_CHECK(FLO_LayerI_ReadScalefactors(&bits, frame));
    FLO_CHECK(FLO_LayerI_ReadSamples(&bits, frame));
    FLO_CHECK(FLO_LayerI_ComputePcm(frame, filter_left, filter_right));

    return FLO_SUCCESS;
}

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */


