/*****************************************************************
|
|      MPEG audio frame handling. Layer II
|
|      (c) 1996-1998 MpegTV, LLC
|      Author: Gilles Boccon-Gibod (gilles@mpegtv.com)
|
 ****************************************************************/


/*-------------------------------------------------------------------------
|       includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloLayerII.h"
#include "FloFrame.h"
#include "FloTables.h"
#include "FloFilter.h"
#include "FloMath.h"
#include "FloDecoder.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|       FLO_LayerII_InitFrame
+-------------------------------------------------------------------------*/
static FLO_Result 
FLO_LayerII_InitFrame(FLO_Frame_II* frame)
{
    int bitrate_per_channel = frame->header.bitrate_index;
          
    if (frame->header.mode != FLO_MPEG_MODE_SINGLE_CHANNEL) {
        /* the bitrate per channel is half of the total bitrate */
        if (frame->header.bitrate_index == FLO_SYNTAX_MPEG_BITRATE_II_64) {
            /* 64 is a special case */
            bitrate_per_channel = FLO_SYNTAX_MPEG_BITRATE_II_32;
        } else {
            /* for other values, the index for the half bitrate */
            /* is just the index - 4                            */
            bitrate_per_channel -= 4;
        }
    }
          
    {
        int             i;
        FLO_Subband_II* subband;

        subband = frame->subbands;

        if (frame->header.id == FLO_SYNTAX_MPEG_ID_MPEG_2) {
            frame->nb_subbands = 30;
            for (i = 0; i < frame->nb_subbands; i++) {
                subband++->table = FLO_LayerII_Mpeg2_AllocationTable[i];
            }
        } else {
            FLO_AllocationTableEntry **tables;

            frame->nb_subbands = 
                FLO_LayerII_SubbandLimits[frame->header.sampling_frequency]
                [bitrate_per_channel];
            tables = 
                (FLO_AllocationTableEntry **)(FLO_LayerII_AllocationTables
                                             [frame->header.sampling_frequency]
                                             [bitrate_per_channel]);
            for (i = 0; i < frame->nb_subbands; i++) {
                subband++->table = *tables++;
            }
        }
    }

    if (frame->header.mode == FLO_MPEG_MODE_JOINT_STEREO) {
        /* joint stereo bound */
        /* 00 -> 4, 01 -> 8, 10 -> 12, 11 -> 16 */
        frame->joint_stereo_bound = 4*(1+frame->header.mode_extension);
        if (frame->joint_stereo_bound > frame->nb_subbands) {
            return FLO_ERROR_INVALID_BITSTREAM;
        }
    } else {
        frame->joint_stereo_bound = 0;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerII_GetFrameSize
+-------------------------------------------------------------------------*/
unsigned int
FLO_LayerII_GetFrameSize(FLO_FrameHeader* header)
{
    int slots;

    slots = (144 * FLO_MpegBitrates[header->id][3-header->layer][header->bitrate_index]*1000)
        / FLO_MpegSamplingFrequencies[header->id][header->sampling_frequency];
    if (header->sampling_frequency == FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_44100_22050
        && header->padding_bit) slots++;

    return slots;
}

/*-------------------------------------------------------------------------
|       FLO_LayerII_ReadBitAllocations
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerII_ReadBitAllocations(FLO_BitStream* bits, 
                               FLO_Frame_II*  frame)
{
    FLO_Subband_II* subband = frame->subbands;
    int             i;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* non joint stereo, read 2 allocations */
        for (i = 0; i < frame->nb_subbands; i++, subband++) {
            int allocation_length = subband->table->allocation_length;
            subband->allocation[0] = FLO_BitStream_ReadBits(bits, allocation_length);
            subband->allocation[1] = FLO_BitStream_ReadBits(bits, allocation_length);
        }
        break;
        
      case FLO_MPEG_MODE_JOINT_STEREO:
        /* joint stereo, before bound, read 2 allocations */
        for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
            int allocation_length = subband->table->allocation_length;
            subband->allocation[0] = FLO_BitStream_ReadBits(bits, allocation_length);
            subband->allocation[1] = FLO_BitStream_ReadBits(bits, allocation_length);
        }
        /* FALLTHROUGH */

      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono, or joint stereo after bound, read 1 allocation */
        for (i = frame->joint_stereo_bound; 
             i < frame->nb_subbands; 
             i++, subband++) {
            int allocation_length = subband->table->allocation_length;
            subband->allocation[0] = FLO_BitStream_ReadBits(bits, allocation_length);
            subband->allocation[1] = subband->allocation[0];
        }
        break;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerII_ReadScalefactorSelections
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerII_ReadScalefactorSelections(FLO_BitStream* bits, 
                                      FLO_Frame_II* frame)
{
    FLO_Subband_II* subband = frame->subbands;
    int             i;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* non joint stereo, read 2 scfsi */
        for (i = 0; i < frame->nb_subbands; i++, subband++) {
            if (subband->allocation[0]) {
                subband->scalefactor_selection[0] = FLO_BitStream_ReadBits(bits, 2);
            }
            if (subband->allocation[1]) {
                subband->scalefactor_selection[1] = FLO_BitStream_ReadBits(bits, 2);
            }
        }
        break;
        
      case FLO_MPEG_MODE_JOINT_STEREO:
        /* joint stereo, before bound, read 2 scfsi */
        for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
            if (subband->allocation[0]) {
                subband->scalefactor_selection[0] = FLO_BitStream_ReadBits(bits, 2);
            }
            if (subband->allocation[1]) {
                subband->scalefactor_selection[1] = FLO_BitStream_ReadBits(bits, 2);
            }
        }
        /* joint stereo, after bound, read 2 scfsi, but one allocation */
        for (i = frame->joint_stereo_bound; 
             i < frame->nb_subbands;
             i++, subband++) {
            if (subband->allocation[0]) {
                subband->scalefactor_selection[0] = FLO_BitStream_ReadBits(bits, 2);
                subband->scalefactor_selection[1] = FLO_BitStream_ReadBits(bits, 2);
            }
        }
        break;

      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono read 1 scsfi */
        for (i = 0; i < frame->nb_subbands; i++, subband++) {
            if (subband->allocation[0]) {
                subband->scalefactor_selection[0] = FLO_BitStream_ReadBits(bits, 2);
            }
        }
        break;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       READ_SCALEFACTORS macro
+-------------------------------------------------------------------------*/
#define READ_SCALEFACTORS(subband, i)                                                \
switch ((subband)->scalefactor_selection[i]) {                                       \
  case 0:                                                                            \
    (subband)->scalefactor[i][0] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    (subband)->scalefactor[i][1] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    (subband)->scalefactor[i][2] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    break;                                                                           \
  case 1:                                                                            \
    (subband)->scalefactor[i][0] =                                                   \
    (subband)->scalefactor[i][1] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    (subband)->scalefactor[i][2] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    break;                                                                           \
  case 2:                                                                            \
    (subband)->scalefactor[i][0] =                                                   \
    (subband)->scalefactor[i][1] =                                                   \
    (subband)->scalefactor[i][2] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    break;                                                                           \
  case 3:                                                                            \
    (subband)->scalefactor[i][0] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
    (subband)->scalefactor[i][1] =                                                   \
    (subband)->scalefactor[i][2] = FLO_Scalefactors[FLO_BitStream_ReadBits(bits, 6)];\
}

/*-------------------------------------------------------------------------
|       FLO_LayerII_ReadScalefactors
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerII_ReadScalefactors(FLO_BitStream* bits, FLO_Frame_II* frame)
{
    FLO_Subband_II* subband = frame->subbands;
    int             i;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* non joint stereo, read 2 scale factors */
        for (i = 0; i < frame->nb_subbands; i++, subband++) {
            if (subband->allocation[0]) {
                READ_SCALEFACTORS(subband, 0);
                subband->info[0]=subband->table->info[subband->allocation[0]];
            }
            if (subband->allocation[1]) {
                READ_SCALEFACTORS(subband, 1);
                subband->info[1]=subband->table->info[subband->allocation[1]];
            }
        }
        break;
        
      case FLO_MPEG_MODE_JOINT_STEREO:
        /* joint stereo, before bound, read 2 scale factors */
        for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
            if (subband->allocation[0]) {
                READ_SCALEFACTORS(subband, 0);
                subband->info[0]=subband->table->info[subband->allocation[0]];
            }
            if (subband->allocation[1]) {
                READ_SCALEFACTORS(subband, 1);
                subband->info[1]=subband->table->info[subband->allocation[1]];
            }
        }
        /* joint stereo, after bound, read 2 scale factors (one allocation) */
        for (i = frame->joint_stereo_bound; 
             i < frame->nb_subbands;
             i++, subband++) {
            if (subband->allocation[0]) {
                READ_SCALEFACTORS(subband, 0);
                subband->info[0]=subband->table->info[subband->allocation[0]];
                READ_SCALEFACTORS(subband, 1);
                subband->info[1]=subband->table->info[subband->allocation[0]];
            }
        }
        break;

      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono read 1 scale factor */
        for (i = 0; i < frame->nb_subbands; i++, subband++) {
            if (subband->allocation[0]) {
                READ_SCALEFACTORS(subband, 0);
                subband->info[0]=subband->table->info[subband->allocation[0]];
            }
        }
        break;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       READ_SAMPLE_N
+-------------------------------------------------------------------------*/
#define READ_SAMPLE_N(bits, info)                                         \
((int)(FLO_BitStream_ReadBits((bits), info.code_length) - info.offset))

#ifdef XA_CONFIG_INTEGER_DECODE
/*-------------------------------------------------------------------------
|       READ_3_SAMPLES_N
+-------------------------------------------------------------------------*/
#define READ_3_SAMPLES_N(subband, c, s, z, bits)                           \
do {                                                                       \
    (subband)->samples[c][s][0] = FLO_FC1_MUL((subband)->scalefactor[c][z],\
        (FLO_Float)FLO_FC3_MUL((subband)->info[c].factor,                  \
                       READ_SAMPLE_N(bits, (subband)->info[c])));          \
    (subband)->samples[c][s][1] = FLO_FC1_MUL((subband)->scalefactor[c][z],\
        (FLO_Float)FLO_FC3_MUL((subband)->info[c].factor,                  \
                       READ_SAMPLE_N(bits, (subband)->info[c])));          \
    (subband)->samples[c][s][2] = FLO_FC1_MUL((subband)->scalefactor[c][z],\
        (FLO_Float)FLO_FC3_MUL((subband)->info[c].factor,                  \
                       READ_SAMPLE_N(bits, (subband)->info[c])));          \
} while (0)
#else /* not XA_CONFIG_INTEGER_DECODE */
/*-------------------------------------------------------------------------
|       READ_3_SAMPLES_N
+-------------------------------------------------------------------------*/
#define READ_3_SAMPLES_N(subband, c, s, z, bits)                           \
do {                                                                       \
    FLO_Float f = (subband)->scalefactor[c][z] * (subband)->info[c].factor;\
    (subband)->samples[c][s][0] = f *                                      \
        (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[c]));              \
    (subband)->samples[c][s][1] = f *                                      \
        (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[c]));              \
    (subband)->samples[c][s][2] = f *                                      \
        (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[c]));              \
} while (0)
#endif /* XA_CONFIG_INTEGER_DECODE */

/*-------------------------------------------------------------------------
|       READ_3_SAMPLES_G
+-------------------------------------------------------------------------*/
#define READ_3_SAMPLES_G(subband, c, s, z, bits)                            \
do {                                                                        \
    int       code = FLO_BitStream_ReadBits((bits),                         \
                                            (subband)->info[c].code_length);\
    FLO_Float f    = (subband)->scalefactor[c][z];                          \
    (subband)->samples[c][s][0] = FLO_FC1_MUL(f,                            \
        (*(subband)->info[c].grouping)[code][0]);                           \
    (subband)->samples[c][s][1] = FLO_FC1_MUL(f,                            \
        (*(subband)->info[c].grouping)[code][1]);                           \
    (subband)->samples[c][s][2] = FLO_FC1_MUL(f,                            \
        (*(subband)->info[c].grouping)[code][2]);                           \
} while (0)

#ifdef XA_CONFIG_INTEGER_DECODE
/*-------------------------------------------------------------------------
|       FLO_READ_3_SAMPLES_N_JOINT
+-------------------------------------------------------------------------*/
#define FLO_READ_3_SAMPLES_N_JOINT(subband, s, z, bits)                        \
do {                                                                           \
    FLO_Float sample;                                                          \
    sample = (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[0]));             \
    (subband)->samples[0][s][0] = FLO_FC1_MUL((subband)->scalefactor[0][z],    \
        (FLO_Float)(FC3_MUL((subband)->info[0].factor, sample)));              \
    (subband)->samples[1][s][0] = FLO_FC1_MUL((subband)->scalefactor[1][z],    \
        (FLO_Float)(FLO_FC3_MUL((subband)->info[0].factor, sample)));          \
    sample = (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[0]));             \
    (subband)->samples[0][s][1] = FLO_FC1_MUL((subband)->scalefactor[0][z],    \
        (FLO_Float)(FLO_FC3_MUL((subband)->info[0].factor, sample)));          \
    (subband)->samples[1][s][1] = FLO_FC1_MUL((subband)->scalefactor[1][z],    \
        (FLO_Float)(FLO_FC3_MUL((subband)->info[0].factor, sample)));          \
    sample = (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[0]));             \
    (subband)->samples[0][s][2] = FLO_FC1_MUL((subband)->scalefactor[0][z],    \
        (FLO_Float)(FLO_FC3_MUL((subband)->info[0].factor, sample)));          \
    (subband)->samples[1][s][2] = FLO_FC1_MUL((subband)->scalefactor[1][z],    \
        (FLO_Float)(FLO_FC3_MUL((subband)->info[0].factor, sample)));          \
} while (0)
#else /* not XA_CONFIG_INTEGER_DECODE */
/*-------------------------------------------------------------------------
|       READ_3_SAMPLES_N_JOINT
+-------------------------------------------------------------------------*/
#define READ_3_SAMPLES_N_JOINT(subband, s, z, bits)                         \
do {                                                                        \
    FLO_Float f0 = (subband)->scalefactor[0][z] * (subband)->info[0].factor;\
    FLO_Float f1 = (subband)->scalefactor[1][z] * (subband)->info[0].factor;\
    FLO_Float sample;                                                       \
    sample = (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[0]));          \
    (subband)->samples[0][s][0] = f0 * sample;                              \
    (subband)->samples[1][s][0] = f1 * sample;                              \
    sample = (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[0]));          \
    (subband)->samples[0][s][1] = f0 * sample;                              \
    (subband)->samples[1][s][1] = f1 * sample;                              \
    sample = (FLO_Float)(READ_SAMPLE_N(bits, (subband)->info[0]));          \
    (subband)->samples[0][s][2] = f0 * sample;                              \
    (subband)->samples[1][s][2] = f1 * sample;                              \
} while (0)
#endif

/*-------------------------------------------------------------------------
|       READ_3_SAMPLES_G_JOINT
+-------------------------------------------------------------------------*/
#define READ_3_SAMPLES_G_JOINT(subband, s, z, bits)                              \
do {                                                                             \
    int code    = FLO_BitStream_ReadBits((bits), (subband)->info[0].code_length);\
    FLO_Float f0    = (subband)->scalefactor[0][z];                              \
    FLO_Float f1    = (subband)->scalefactor[1][z];                              \
    (subband)->samples[0][s][0] = FLO_FC1_MUL(f0,                                \
        (*(subband)->info[0].grouping)[code][0]);                                \
    (subband)->samples[1][s][0] = FLO_FC1_MUL(f1,                                \
        (*(subband)->info[0].grouping)[code][0]);                                \
    (subband)->samples[0][s][1] = FLO_FC1_MUL(f0,                                \
        (*(subband)->info[0].grouping)[code][1]);                                \
    (subband)->samples[1][s][1] = FLO_FC1_MUL(f1,                                \
        (*(subband)->info[0].grouping)[code][1]);                                \
    (subband)->samples[0][s][2] = FLO_FC1_MUL(f0,                                \
        (*(subband)->info[0].grouping)[code][2]);                                \
    (subband)->samples[1][s][2] = FLO_FC1_MUL(f1,                                \
        (*(subband)->info[0].grouping)[code][2]);                                \
} while (0)

/*-------------------------------------------------------------------------
|       FLO_LayerII_ReadSamples
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerII_ReadSamples(FLO_BitStream* bits, FLO_Frame_II* frame)
{
    int i;
    int s;
    int z = 0;

    switch (frame->header.mode) {
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
        /* dual channel or stereo, read 2 sample groups */
        for (s = 0; s < FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS; s++) { 
            FLO_Subband_II* subband = frame->subbands;
            for (i = 0; i < frame->nb_subbands; i++, subband++) {
                if (subband->allocation[0]) {
                    if (subband->info[0].grouping) {
                        READ_3_SAMPLES_G(subband, 0, s, z, bits);
                    } else {
                        READ_3_SAMPLES_N(subband, 0, s, z, bits);
                    }
                } else {
                    subband->samples[0][s][0] =
                    subband->samples[0][s][1] =
                    subband->samples[0][s][2] = FLO_ZERO;
                }
                if (subband->allocation[1]) {
                    if (subband->info[1].grouping) {
                        READ_3_SAMPLES_G(subband, 1, s, z, bits);
                    } else {
                        READ_3_SAMPLES_N(subband, 1, s, z, bits);
                    }
                } else {
                    subband->samples[1][s][0] =
                    subband->samples[1][s][1] =
                    subband->samples[1][s][2] = FLO_ZERO;
                }
            } 
            if ((s & 3) == 3) z++;
        }
        break;

      case FLO_MPEG_MODE_JOINT_STEREO:
        for (s = 0; s < FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS; s++) { 
            FLO_Subband_II* subband = frame->subbands;
            /* joint stereo before bound */
            for (i = 0; i < frame->joint_stereo_bound; i++, subband++) {
                if (subband->allocation[0]) {
                    if (subband->info[0].grouping) {
                        READ_3_SAMPLES_G(subband, 0, s, z, bits);
                    } else {
                        READ_3_SAMPLES_N(subband, 0, s, z, bits);
                    }
                } else {
                    subband->samples[0][s][0] =
                    subband->samples[0][s][1] =
                    subband->samples[0][s][2] = FLO_ZERO;
                }
                if (subband->allocation[1]) {
                    if (subband->info[1].grouping) {
                        READ_3_SAMPLES_G(subband, 1, s, z, bits);
                    } else {
                        READ_3_SAMPLES_N(subband, 1, s, z, bits);
                    }
                } else {
                    subband->samples[1][s][0] =
                    subband->samples[1][s][1] =
                    subband->samples[1][s][2] = FLO_ZERO;
                }
            } 

            /* joint stereo after bound */
            for (i = frame->joint_stereo_bound; 
                 i < frame->nb_subbands; 
                 i++, subband++) {
                if (subband->allocation[0]) {
                    if (subband->info[0].grouping) {
                        READ_3_SAMPLES_G_JOINT(subband, s, z, bits);
                    } else {
                        READ_3_SAMPLES_N_JOINT(subband, s, z, bits);
                    }
                } else {
                    subband->samples[0][s][0] =
                    subband->samples[0][s][1] =
                    subband->samples[0][s][2] =
                    subband->samples[1][s][0] =
                    subband->samples[1][s][1] =
                    subband->samples[1][s][2] = FLO_ZERO;
                }
            } 
            if ((s & 3) == 3) z++;
        }
        break;
        
      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        /* mono or joint stereo, read 1 sample */
        for (s = 0; s < FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS; s++) { 
            FLO_Subband_II* subband = frame->subbands;
            for (i = 0; i < frame->nb_subbands; i++, subband++) {
                if (subband->allocation[0]) {
                    if (subband->info[0].grouping) {
                        READ_3_SAMPLES_G(subband, 0, s, z, bits);
                    } else {
                        READ_3_SAMPLES_N(subband, 0, s, z, bits);
                    }
                } else {
                    subband->samples[0][s][0] =
                    subband->samples[0][s][1] =
                    subband->samples[0][s][2] = FLO_ZERO;
                }
            }
            if ((s & 3) == 3) z++;
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerII_ComputePcm
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerII_ComputePcm(FLO_Frame_II*        frame, 
                       FLO_SynthesisFilter* filter_left,
                       FLO_SynthesisFilter* filter_right)
{
    int i, g;
    
    /* prepare for subsampling */
    {
        int subsampling;
        subsampling = filter_left ? 
            filter_left->subsampling : filter_right->subsampling;
        if (subsampling) {
            int max_subband = FLO_MPEG_LAYER_II_MAX_SUBBANDS >> subsampling;
            if (frame->nb_subbands > max_subband) {
                frame->nb_subbands = max_subband;
            }
        }
    }

    if (filter_left && filter_right) {
        if (filter_left == filter_right) {
            /* mix left + right */
            filter_left->input = frame->samples[0];

            for (i=frame->nb_subbands; i< FLO_FILTER_NB_SAMPLES; i++) {
                filter_left->input[i] = FLO_ZERO;
            }
            for (g=0; g<FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS; g++) {
                int j;
                for (j=0; j<FLO_MPEG_LAYER_II_SAMPLE_GROUP_SIZE; j++) {
                    int        s;
                    FLO_Subband_II* subband = frame->subbands; 
                    FLO_Float*      samples = filter_left->input;
                    for (s=0; s<frame->nb_subbands; s++, subband++) {
                        FLO_Float sample;
                        if (subband->allocation[0]) {
                            sample = subband->samples[0][g][j];
                        } else {
                            sample = FLO_ZERO;
                        }
                        if (subband->allocation[1]) {
                            sample += subband->samples[1][g][j];
                        } 
                        *samples++ = FLO_FDIV2(sample);
                    }
                    FLO_SynthesisFilter_ComputePcm(filter_left);
                }
            }
        } else {
            /* stereo */
            filter_left->input  = frame->samples[0];
            filter_right->input = frame->samples[1];

            for (i=frame->nb_subbands; i< FLO_FILTER_NB_SAMPLES; i++) {
                filter_left->input[i] = filter_right->input[i] = FLO_ZERO;
            }
            for (g=0; g<FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS; g++) {
                int j;
                for (j=0; j<FLO_MPEG_LAYER_II_SAMPLE_GROUP_SIZE; j++) {
                    int             s;
                    FLO_Subband_II* subband = frame->subbands; 
                    FLO_Float*      samples_l = filter_left->input;
                    FLO_Float*      samples_r = filter_right->input;
                    for (s=0; s<frame->nb_subbands; s++) {
                        if (subband->allocation[0]) {
                            *samples_l++ = subband->samples[0][g][j];
                        } else {
                            *samples_l++ = FLO_ZERO;
                        }
                        if (subband->allocation[1]) {
                            *samples_r++ = subband->samples[1][g][j];
                        } else {
                            *samples_r++ = FLO_ZERO;
                        }
                        subband++;
                    }
                    FLO_SynthesisFilter_ComputePcm(filter_left);
                    FLO_SynthesisFilter_ComputePcm(filter_right);
                }
            }
        }
    } else {
        /* mono */
        int                  channel;
        FLO_SynthesisFilter* filter;

        if (filter_left) {
            filter = filter_left;
            channel = 0;
        } else {
            filter = filter_right;
            channel = 1;
        }

        filter->input = frame->samples[0];

        for (i=frame->nb_subbands; i< FLO_FILTER_NB_SAMPLES; i++) {
            filter->input[i] = FLO_ZERO;
        }

        for (g=0; g<FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS; g++) {
            int j;
            for (j=0; j<FLO_MPEG_LAYER_II_SAMPLE_GROUP_SIZE; j++) {
                int             s;
                FLO_Subband_II* subband = frame->subbands; 
                FLO_Float*      samples_l = filter->input;

                for (s=0; s<frame->nb_subbands; s++) {
                    if (subband->allocation[channel]) {
                        *samples_l++ = subband->samples[channel][g][j];
                    } else {
                        *samples_l++ = FLO_ZERO;
                    }
                    subband++;
                }
                FLO_SynthesisFilter_ComputePcm(filter);
            }
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerII_DecodeFrame
+-------------------------------------------------------------------------*/
FLO_Result
FLO_LayerII_DecodeFrame(const unsigned char* frame_data,
                        const FLO_FrameInfo* frame_info,
                        FLO_Frame_II*        frame,
                        FLO_SynthesisFilter* filter_left, 
                        FLO_SynthesisFilter* filter_right)
{
    /* setup the bitstream */
    FLO_BitStream bits;
    FLO_BitStream_SetData(&bits, frame_data, frame_info->size-4);

    /* decode the frame */
    FLO_CHECK(FLO_LayerII_InitFrame(frame));
    FLO_CHECK(FLO_LayerII_ReadBitAllocations(&bits, frame));
    FLO_CHECK(FLO_LayerII_ReadScalefactorSelections(&bits, frame));
    FLO_CHECK(FLO_LayerII_ReadScalefactors(&bits, frame));
    FLO_CHECK(FLO_LayerII_ReadSamples(&bits, frame));
    FLO_CHECK(FLO_LayerII_ComputePcm(frame, filter_left, filter_right));

    return FLO_SUCCESS;
}

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */


