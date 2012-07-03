/*****************************************************************
|
|      MPEG audio frame handling. Layer III
|
|      (c) 1996-1998 MpegTV, LLC
|      Author: Gilles Boccon-Gibod (gilles@mpegtv.com)
|
 ****************************************************************/

/*-------------------------------------------------------------------------
|       includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloLayerIII.h"
#include "FloFrame.h"
#include "FloTables.h"
#include "FloFilter.h"
#include "FloHuffman.h"
#include "FloUtils.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|       FLO_LayerIII_InitFrame
+-------------------------------------------------------------------------*/
static FLO_Result 
FLO_LayerIII_InitFrame(FLO_Frame_III* frame)
{
    if (frame->header.id == FLO_SYNTAX_MPEG_ID_MPEG_1) {
        frame->nb_granules = 2;
    } else {
        frame->nb_granules = 1;
    }

    if (frame->header.mode == FLO_SYNTAX_MPEG_MODE_JOINT_STEREO) {
        frame->intensity_stereo =  (frame->header.mode_extension & 1);
        frame->ms_stereo = (frame->header.mode_extension & 2);
    } else {    
        frame->intensity_stereo = frame->ms_stereo = FLO_FALSE;
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ResetFrame
+-------------------------------------------------------------------------*/
void
FLO_LayerIII_ResetFrame(FLO_Frame_III* frame)
{
    FLO_HybridFilter_Reset(&frame->hybrid[0]);
    FLO_HybridFilter_Reset(&frame->hybrid[1]);
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_GetFrameSize
+-------------------------------------------------------------------------*/
unsigned int
FLO_LayerIII_GetFrameSize(FLO_FrameHeader* header)
{
    unsigned int slots;

    slots = (144 * FLO_MpegBitrates[header->id][3-header->layer][header->bitrate_index]*1000)
        / FLO_MpegSamplingFrequencies[header->id][header->sampling_frequency];
    if (header->id == FLO_SYNTAX_MPEG_ID_MPEG_2 ||
        header->id == FLO_SYNTAX_MPEG_ID_MPEG_2_5) slots /= 2;
    if (header->sampling_frequency == FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_44100_22050
        && header->padding_bit) slots++;

    return slots;
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_GetSideInfoSize
+-------------------------------------------------------------------------*/
unsigned int
FLO_LayerIII_GetSideInfoSize(FLO_FrameHeader* header)
{
    if (header->id == FLO_SYNTAX_MPEG_ID_MPEG_1) {
        if (header->mode == FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
            return FLO_MPEG_LAYER_III_MPEG_1_SIDE_INFO_SIZE_MONO;
        } else {
            return FLO_MPEG_LAYER_III_MPEG_1_SIDE_INFO_SIZE_STEREO;
        }
    } else {
        if (header->mode == FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
            return FLO_MPEG_LAYER_III_MPEG_2_SIDE_INFO_SIZE_MONO;
        } else {
            return FLO_MPEG_LAYER_III_MPEG_2_SIDE_INFO_SIZE_STEREO;
        }
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadSideInfo_Mpeg1
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerIII_ReadSideInfo_Mpeg1(FLO_BitStream* bits, FLO_Frame_III* frame)
{
    int granule;
    int channel;
    int nb_channels;

    FLO_SideInfo* info = &frame->side_info;

    info->main_data_begin = FLO_BitStream_ReadBits(bits, 9);
    switch (frame->header.mode) {
      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        info->private_bits = FLO_BitStream_ReadBits(bits, 5);
        nb_channels = 1;
        break;
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
      case FLO_MPEG_MODE_JOINT_STEREO:
        info->private_bits = FLO_BitStream_ReadBits(bits, 3);
        nb_channels = 2;
        break;
      default:
        nb_channels = 0; /* to avoid compiler complaints */
        break;
    }

    for (channel = 0; channel < nb_channels; channel++) {
        info->scalefactor_selection[channel][0] = FLO_BitStream_ReadBits(bits, 1); 
        info->scalefactor_selection[channel][1] = FLO_BitStream_ReadBits(bits, 1);
        info->scalefactor_selection[channel][2] = FLO_BitStream_ReadBits(bits, 1);
        info->scalefactor_selection[channel][3] = FLO_BitStream_ReadBits(bits, 1);
    }
    for (granule = 0; granule < frame->nb_granules; granule++) {
        for (channel = 0; channel < nb_channels; channel++) {
            FLO_Granule *gp = &(info->granules[granule][channel]);

            gp->part_2_3_length   = FLO_BitStream_ReadBits(bits, 12);
            gp->big_values        = FLO_BitStream_ReadBits(bits, 9);
            if (gp->big_values > 
                FLO_MPEG_LAYER_III_MPEG_2_PCM_SAMPLES_PER_FRAME/2) {
                return FLO_ERROR_INVALID_BITSTREAM;
            }
            gp->global_gain             = FLO_BitStream_ReadBits(bits, 8);
            gp->scalefactor_compression = FLO_BitStream_ReadBits(bits, 4);
            gp->window_switching_flag   = FLO_BitStream_ReadBits(bits, 1);
            if (gp->window_switching_flag) {
                gp->block_type          = FLO_BitStream_ReadBits(bits, 2);
                if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_RESERVED) {
                    return FLO_ERROR_INVALID_BITSTREAM;
                }
                gp->mixed_block_flag    = FLO_BitStream_ReadBits(bits, 1);
                gp->table_selection[0]  = FLO_BitStream_ReadBits(bits, 5);
                gp->table_selection[1]  = FLO_BitStream_ReadBits(bits, 5);
                gp->table_selection[2]  = 0;
                gp->subblock_gain[0]    = FLO_BitStream_ReadBits(bits, 3);
                gp->subblock_gain[1]    = FLO_BitStream_ReadBits(bits, 3);
                gp->subblock_gain[2]    = FLO_BitStream_ReadBits(bits, 3);

                /* these are set by default */
                gp->region1_start = 36; /* no need to lookup in tables, the value */
                                        /* is always the same.                    */
                gp->region2_start = 576; /* that means no region 2 */
            } else {
                gp->block_type          = 0;
                gp->mixed_block_flag    = 0;
                gp->table_selection[0]  = FLO_BitStream_ReadBits(bits, 5);
                gp->table_selection[1]  = FLO_BitStream_ReadBits(bits, 5);
                gp->table_selection[2]  = FLO_BitStream_ReadBits(bits, 5);
                gp-> region0_count      = FLO_BitStream_ReadBits(bits, 4);
                gp->region1_start       = 
                    FLO_SubbandInfo_Long[1]
                                        [frame->header.sampling_frequency]
                                        [1+gp->region0_count].start;
                gp->region1_count       = FLO_BitStream_ReadBits(bits, 3);
                if (1+gp->region0_count + 1+gp->region1_count >= 22) {
                    gp->region2_start = 576;
                } else {
                    gp->region2_start = 
                        FLO_SubbandInfo_Long[1]
                                            [frame->header.sampling_frequency]
                                            [1+gp->region0_count + 1+gp->region1_count].start;
                }
            }
            gp->preflag                 = FLO_BitStream_ReadBits(bits, 1);
            gp->scalefactor_scale       = FLO_BitStream_ReadBits(bits, 1);
            gp->count1_table_selection  = FLO_BitStream_ReadBits(bits, 1);
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadSideInfo_Mpeg2
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerIII_ReadSideInfo_Mpeg2(FLO_BitStream* bits, FLO_Frame_III* frame)
{
    int channel;
    int nb_channels;

    FLO_SideInfo *info = &frame->side_info;

    info->main_data_begin = FLO_BitStream_ReadBits(bits, 8);
    switch (frame->header.mode) {
      case FLO_MPEG_MODE_SINGLE_CHANNEL:
        info->private_bits = FLO_BitStream_ReadBits(bits, 1);
        nb_channels = 1;
        break;
      case FLO_MPEG_MODE_STEREO:
      case FLO_MPEG_MODE_DUAL_CHANNEL:
      case FLO_MPEG_MODE_JOINT_STEREO:
        info->private_bits = FLO_BitStream_ReadBits(bits, 2);
        nb_channels = 2;
        break;
      default:
        nb_channels = 0; /* to avoid compiler complaints */
        break;
    }

    for (channel = 0; channel < nb_channels; channel++) {
        FLO_Granule* gp = &(info->granules[0][channel]);

        gp->part_2_3_length                 = FLO_BitStream_ReadBits(bits, 12);
        gp->big_values                      = FLO_BitStream_ReadBits(bits, 9);
        if (gp->big_values > FLO_MPEG_LAYER_III_MPEG_2_PCM_SAMPLES_PER_FRAME/2) {
            return FLO_ERROR_INVALID_BITSTREAM;
        }
        gp->global_gain                     = FLO_BitStream_ReadBits(bits, 8);
        gp->scalefactor_compression         = FLO_BitStream_ReadBits(bits, 9);
        gp->window_switching_flag           = FLO_BitStream_ReadBits(bits, 1);
        if (gp->window_switching_flag) {
            gp->block_type = FLO_BitStream_ReadBits(bits, 2);
            if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_RESERVED) {
                return FLO_ERROR_INVALID_BITSTREAM;
            }
            gp->mixed_block_flag    = FLO_BitStream_ReadBits(bits, 1);
            gp->table_selection[0]  = FLO_BitStream_ReadBits(bits, 5);
            gp->table_selection[1]  = FLO_BitStream_ReadBits(bits, 5);
            gp->table_selection[2]  = 0;
            gp->subblock_gain[0]    = FLO_BitStream_ReadBits(bits, 3);
            gp->subblock_gain[1]    = FLO_BitStream_ReadBits(bits, 3);
            gp->subblock_gain[2]    = FLO_BitStream_ReadBits(bits, 3);

            /* these are set by default */
            if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS &&
                !gp->mixed_block_flag) {
                gp->region0_count = 8;
                gp->region1_start = FLO_SubbandInfo_Short[frame->header.id]
                                                         [frame->header.sampling_frequency]
                                                         [9].start*3; /* 9th subband */
            } else {
                gp->region0_count = 7;
                gp->region1_start = FLO_SubbandInfo_Long[frame->header.id]
                                                        [frame->header.sampling_frequency]
                                                        [8].start; /* 8th subband */
            }
            gp->region1_count = 36;
            gp->region2_start = 576; /* that means no region 2 */
        } else {
            gp->block_type          = 0;
            gp->table_selection[0]  = FLO_BitStream_ReadBits(bits, 5);
            gp->table_selection[1]  = FLO_BitStream_ReadBits(bits, 5);
            gp->table_selection[2]  = FLO_BitStream_ReadBits(bits, 5);
            gp->region0_count       = FLO_BitStream_ReadBits(bits, 4);
            gp->region1_start       = FLO_SubbandInfo_Long[frame->header.id]
                                                          [frame->header.sampling_frequency]
                                                          [1+gp->region0_count].start;
            gp->region1_count       = FLO_BitStream_ReadBits(bits, 3);
            if (1+gp->region0_count + 1+gp->region1_count >= 22) {
                gp->region2_start = 576;
            } else {
                gp->region2_start = FLO_SubbandInfo_Long[frame->header.id]
                                                        [frame->header.sampling_frequency]
                                                        [1+gp->region0_count + 1+gp->region1_count].start;
            }
        }
        gp->scalefactor_scale       = FLO_BitStream_ReadBits(bits, 1);
        gp->count1_table_selection  = FLO_BitStream_ReadBits(bits, 1);
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadSideInfo
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerIII_ReadSideInfo(FLO_BitStream* bits, FLO_Frame_III* frame)
{
    if (frame->header.id == FLO_SYNTAX_MPEG_ID_MPEG_1) {
        return FLO_LayerIII_ReadSideInfo_Mpeg1(bits, frame);
    } else {
        return FLO_LayerIII_ReadSideInfo_Mpeg2(bits, frame);
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadScalefactors_Mpeg1
+-------------------------------------------------------------------------*/
static void
FLO_LayerIII_ReadScalefactors_Mpeg1(FLO_BitStream* bits, 
                                    FLO_Frame_III* frame, 
                                    int            granule, 
                                    int            channel)
{
    FLO_Granule* gp = &frame->side_info.granules[granule][channel];
    int sf_length0  = FLO_ScalefactorLengths[gp->scalefactor_compression][0];
    int sf_length1  = FLO_ScalefactorLengths[gp->scalefactor_compression][1];

    if (gp->window_switching_flag && 
        gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS) {
        if (gp->mixed_block_flag) {
            gp->scalefactors.l[0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[3] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[4] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[5] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[6] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[7] = FLO_BitStream_ReadBits(bits, sf_length0);
            
            gp->scalefactors.s[ 3][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 3][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 3][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 4][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 4][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 4][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 5][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 5][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 5][2] = FLO_BitStream_ReadBits(bits, sf_length0);

            gp->scalefactors.s[ 6][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 6][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 6][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 7][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 7][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 7][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 8][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 8][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 8][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 9][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 9][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 9][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[10][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[10][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[10][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[11][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[11][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[11][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[12][0] = 
            gp->scalefactors.s[12][1] = 
            gp->scalefactors.s[12][2] = 0;

            gp->part_2_length         = 17*sf_length0 + 18*sf_length1;
        } else {
            gp->scalefactors.s[ 0][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 0][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 0][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 1][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 1][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 1][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 2][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 2][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 2][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 3][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 3][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 3][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 4][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 4][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 4][2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 5][0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 5][1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.s[ 5][2] = FLO_BitStream_ReadBits(bits, sf_length0);

            gp->scalefactors.s[ 6][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 6][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 6][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 7][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 7][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 7][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 8][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 8][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 8][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 9][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 9][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[ 9][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[10][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[10][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[10][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[11][0] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[11][1] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[11][2] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.s[12][0] =
            gp->scalefactors.s[12][1] =
            gp->scalefactors.s[12][2] = 0;

            gp->part_2_length         = 18*(sf_length0 + sf_length1);
        }
    } else {
        if (granule == 0) {
            gp->scalefactors.l[ 0] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 1] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 2] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 3] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 4] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 5] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 6] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 7] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 8] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[ 9] = FLO_BitStream_ReadBits(bits, sf_length0);
            gp->scalefactors.l[10] = FLO_BitStream_ReadBits(bits, sf_length0);

            gp->scalefactors.l[11] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[12] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[13] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[14] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[15] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[16] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[17] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[18] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[19] = FLO_BitStream_ReadBits(bits, sf_length1);
            gp->scalefactors.l[20] = FLO_BitStream_ReadBits(bits, sf_length1);

            gp->part_2_length      = 11*sf_length0 + 10*sf_length1;
        } else {
            FLO_Granule* g0p = &frame->side_info.granules[0][channel];
            if (frame->side_info.scalefactor_selection[channel][0]) {
                gp->scalefactors.l[0] = g0p->scalefactors.l[0];
                gp->scalefactors.l[1] = g0p->scalefactors.l[1];
                gp->scalefactors.l[2] = g0p->scalefactors.l[2];
                gp->scalefactors.l[3] = g0p->scalefactors.l[3];
                gp->scalefactors.l[4] = g0p->scalefactors.l[4];
                gp->scalefactors.l[5] = g0p->scalefactors.l[5];

                gp->part_2_length     = 0;
            } else {
                gp->scalefactors.l[0] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[1] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[2] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[3] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[4] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[5] = FLO_BitStream_ReadBits(bits, sf_length0);

                gp->part_2_length     = 6*sf_length0;
            }
            if (frame->side_info.scalefactor_selection[channel][1]) {
                gp->scalefactors.l[ 6] = g0p->scalefactors.l[ 6];
                gp->scalefactors.l[ 7] = g0p->scalefactors.l[ 7];
                gp->scalefactors.l[ 8] = g0p->scalefactors.l[ 8]; 
                gp->scalefactors.l[ 9] = g0p->scalefactors.l[ 9];
                gp->scalefactors.l[10] = g0p->scalefactors.l[10];
            } else {
                gp->scalefactors.l[ 6] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[ 7] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[ 8] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[ 9] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->scalefactors.l[10] = FLO_BitStream_ReadBits(bits, sf_length0);
                gp->part_2_length     += 5*sf_length0;
            }
            if (frame->side_info.scalefactor_selection[channel][2]) {
                gp->scalefactors.l[11] = g0p->scalefactors.l[11];
                gp->scalefactors.l[12] = g0p->scalefactors.l[12];
                gp->scalefactors.l[13] = g0p->scalefactors.l[13];
                gp->scalefactors.l[14] = g0p->scalefactors.l[14];
                gp->scalefactors.l[15] = g0p->scalefactors.l[15];
            } else {
                gp->scalefactors.l[11] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[12] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[13] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[14] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[15] = FLO_BitStream_ReadBits(bits, sf_length1);
                
                gp->part_2_length     += 5*sf_length1;
            }
            if (frame->side_info.scalefactor_selection[channel][3]) {
                gp->scalefactors.l[16] = g0p->scalefactors.l[16];
                gp->scalefactors.l[17] = g0p->scalefactors.l[17]; 
                gp->scalefactors.l[18] = g0p->scalefactors.l[18];
                gp->scalefactors.l[19] = g0p->scalefactors.l[19];
                gp->scalefactors.l[20] = g0p->scalefactors.l[20];
            } else {
                gp->scalefactors.l[16] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[17] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[18] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[19] = FLO_BitStream_ReadBits(bits, sf_length1);
                gp->scalefactors.l[20] = FLO_BitStream_ReadBits(bits, sf_length1);

                gp->part_2_length     += 5*sf_length1;
            }
        }
        gp->scalefactors.l[21] = 0;
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadScalefactors_Mpeg2
+-------------------------------------------------------------------------*/
static void
FLO_LayerIII_ReadScalefactors_Mpeg2(FLO_BitStream* bits, 
                                    FLO_Frame_III* frame, 
                                    int            channel)
{
    FLO_Granule* gp = &frame->side_info.granules[0][channel];
    const FLO_LayerIII_ScalefactorLengthInfo* info;
    int index = 0;
    int partition;
    int table;

    if (channel == 1 && frame->intensity_stereo) {
        info = &FLO_LayerIII_ScalefactorInfo_1[gp->scalefactor_compression>>1];
        table = 1;
    } else {
        info = &FLO_LayerIII_ScalefactorInfo_0[gp->scalefactor_compression];
        table = 0;
    }

    gp->part_2_length = 0;
    gp->preflag = info->preflag;

    if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS) {
        if (gp->mixed_block_flag) {
            const int* nb_subbands = 
                FLO_LayerIII_ScalefactorPartitions[table][2][info->table];
            
            for (partition = 0; partition < 4; partition++) {
                int length = info->length[partition];
                int number_of_bands = nb_subbands[partition];
                int band;
                int in_long = FLO_TRUE;

                if (length) {
                    for (band = 0; band < number_of_bands; band += (in_long?1:3)) {
                        if (in_long) {
                            gp->scalefactors.l[index]    = FLO_BitStream_ReadBits(bits, length);
                            gp->scalefactors.s[index][0] = 0;
                            gp->scalefactors.s[index][1] = 0;
                            gp->scalefactors.s[index][2] = 0;
                            gp->part_2_length += length;
                        } else {
                            gp->scalefactors.s[index][0] = FLO_BitStream_ReadBits(bits, length);
                            gp->scalefactors.s[index][1] = FLO_BitStream_ReadBits(bits, length);
                            gp->scalefactors.s[index][2] = FLO_BitStream_ReadBits(bits, length);
                            gp->scalefactors.l[index]    = 0;
                            gp->part_2_length += 3*length;
                        }
                        index++;
                    }
                } else {
                    for (band = 0; band < number_of_bands; band += (in_long?1:3)) {
                        gp->scalefactors.s[index][0] =
                        gp->scalefactors.s[index][1] =
                        gp->scalefactors.s[index][2] =
                        gp->scalefactors.l[index]    = 0;
                        index++;
                    }
                }
                if (in_long && index == 6) {
                    /* switch to short */
                    in_long = FLO_FALSE;
                    index = 3;
                }
            }
            gp->scalefactors.s[12][0] =
            gp->scalefactors.s[12][1] =
            gp->scalefactors.s[12][2] =
            gp->scalefactors.l[21]    = 0;
        } else {
            const int* nb_subbands = 
                FLO_LayerIII_ScalefactorPartitions[table][1][info->table];

            for (partition = 0; partition < 4; partition++) {
                int length = info->length[partition];
                int number_of_bands = nb_subbands[partition];
                int band;

                if (length) {
                    for (band = 0; band < number_of_bands; band += 3) {
                        gp->scalefactors.s[index][0] = FLO_BitStream_ReadBits(bits, length);
                        gp->scalefactors.s[index][1] = FLO_BitStream_ReadBits(bits, length);
                        gp->scalefactors.s[index][2] = FLO_BitStream_ReadBits(bits, length);
                        index++;
                    }
                    gp->part_2_length += length * number_of_bands;
                } else {
                    for (band = 0; band < number_of_bands; band += 3) {
                        gp->scalefactors.s[index][0] =
                        gp->scalefactors.s[index][1] =
                        gp->scalefactors.s[index][2] = 0;
                        index++;
                    }
                }
            }
            gp->scalefactors.s[12][0] =
            gp->scalefactors.s[12][1] =
            gp->scalefactors.s[12][2] = 0;
        }
    } else {
        const int* nb_subbands = 
            FLO_LayerIII_ScalefactorPartitions[table][0][info->table];

        for (partition = 0; partition < 4; partition++) {
            int length = info->length[partition];
            int number_of_bands = nb_subbands[partition];
            int band;

            if (length) {
                for (band = 0; band < number_of_bands; band++) {
                    gp->scalefactors.l[index++] = FLO_BitStream_ReadBits(bits, length);
                }
                gp->part_2_length += length * number_of_bands;
            } else {
                for (band = 0; band < number_of_bands; band++) {
                    gp->scalefactors.l[index++] = 0;
                }
            }
        }
        gp->scalefactors.l[21] = 0;
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadScalefactors
+-------------------------------------------------------------------------*/
static void
FLO_LayerIII_ReadScalefactors(FLO_BitStream* bits, 
                              FLO_Frame_III* frame, 
                              int            granule,
                              int            channel)
{
    if (frame->header.id == FLO_SYNTAX_MPEG_ID_MPEG_1) {
        FLO_LayerIII_ReadScalefactors_Mpeg1(bits, frame, granule, channel);
    } else {
        FLO_LayerIII_ReadScalefactors_Mpeg2(bits, frame, channel);
    }
}

/*-------------------------------------------------------------------------
|       STORE_QUAD_1
+-------------------------------------------------------------------------*/
#define STORE_QUAD_1(samples, value, quad, n, bits, bits_left) \
    if ((quad) & (8 >> (n))) {                                 \
        if (FLO_BitStream_ReadBit(bits)) {                     \
            *samples++ = -FLO_FC8_SCL(value);                  \
        } else {                                               \
            *samples++ =  FLO_FC8_SCL(value);                  \
        }                                                      \
        (bits_left)--;                                         \
    } else {                                                   \
        *samples++ = FLO_ZERO;                                 \
    }

/*-------------------------------------------------------------------------
|       STORE_QUAD_N
+-------------------------------------------------------------------------*/
#define STORE_QUAD_N(samples, value, quad, n, bits, bits_left, inc) \
    if ((quad) & (8 >> (n))) {                                      \
        if (FLO_BitStream_ReadBit(bits)) {                          \
            *samples = -FLO_FC8_SCL(value);                         \
        } else {                                                    \
            *samples =  FLO_FC8_SCL(value);                         \
        }                                                           \
        (bits_left)--;                                              \
    } else {                                                        \
        *samples = FLO_ZERO;                                        \
    }                                                               \
    samples += inc;

/*-------------------------------------------------------------------------
|       BOUNDARY_CHECKPOINT_SHORT
+-------------------------------------------------------------------------*/
#define BOUNDARY_CHECKPOINT_SHORT                                       \
if (subband_count-- == 0) {                                             \
    if (!switched && subband == switch_point) {                         \
        switched = FLO_TRUE;                                            \
        subband_info = &FLO_SubbandInfo_Short                           \
            [frame->header.id][frame->header.sampling_frequency][3*3];  \
        scalefactor = gp->scalefactors.s[3];                            \
    }                                                                   \
    if (switched) {                                                     \
        factor =                                                        \
            short_gain[window][(*scalefactor++) << gain_shift];         \
        if (++window == 3) window = 0;                                  \
    } else {                                                            \
        factor = long_gain[(*scalefactor++) << gain_shift];             \
    }                                                                   \
    subband_count = subband_info->width/2 - 1;                          \
    subband++;                                                          \
    samples = &samples_start[subband_info++->sample_index];             \
}

/*-------------------------------------------------------------------------
|       BOUNDARY_CHECKPOINT_SHORT_QUICK_BREAK
+-------------------------------------------------------------------------*/
#define BOUNDARY_CHECKPOINT_SHORT_QUICK_BREAK                           \
if (subband_count-- == 0) {                                             \
    if (!switched && subband == switch_point) {                         \
        switched = FLO_TRUE;                                            \
        subband_info = &FLO_SubbandInfo_Short                           \
            [frame->header.id][frame->header.sampling_frequency][3*3];  \
    }                                                                   \
    if (subband_info->start < 0) break;                                 \
    subband_count = subband_info->width/2 - 1;                          \
    subband++;                                                          \
    samples = &samples_start[subband_info++->sample_index];             \
}

/*-------------------------------------------------------------------------
|       BOUNDARY_CHECKPOINT_LONG
+-------------------------------------------------------------------------*/
#define BOUNDARY_CHECKPOINT_LONG                                        \
if (subband_count-- == 0) {                                             \
    subband_count = subband_info++->width/2 - 1;                        \
    factor = gain[(*scalefactor++ + *pretab++) << gain_shift];          \
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadHuffmanSamples
+-------------------------------------------------------------------------*/
static void
FLO_LayerIII_ReadHuffmanSamples(FLO_BitStream* bits, 
                                FLO_Frame_III* frame,
                                int            granule, 
                                int            channel)
{
    FLO_Granule*                 gp = &frame->side_info.granules[granule][channel];
    FLO_Float*                   samples = (FLO_Float*)frame->hybrid[channel].in;
    int                          region_size[4];
    int                          bits_left = gp->part_2_3_length-gp->part_2_length;
    int                          subband_count = 0;
    const FLO_LayerIII_BandInfo* subband_info;
    int                          gain_shift = 1 + gp->scalefactor_scale;
    FLO_Float                    factor = FLO_ZERO;

    /* compute the size of the regions */
    {
        unsigned int big_values = gp->big_values*2;
        if (big_values > gp->region1_start) {
            region_size[0] = gp->region1_start;
            if (big_values > gp->region2_start) {
                region_size[1] = gp->region2_start - region_size[0];
                region_size[2] = big_values - gp->region2_start;
            } else {
                region_size[1] = big_values - region_size[0];
                region_size[2] = 0;
            }
        } else {
            region_size[0] = big_values;
            region_size[1] = 
            region_size[2] = 0;
        }
        region_size[3] = FLO_SYNTAX_MPEG_LAYER_III_NB_FREQUENCY_LINES - big_values;
    }

    /* reset the null band counter */
    frame->hybrid[channel].nb_zero_bands = 0;

    /* now read the data */
    if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS) {
        /* short blocks, or mixed long/short */
        int              i;
        int              region;
        int              switch_point;
        FLO_Boolean      switched;
        const FLO_Float* short_gain[3];
        const FLO_Float* long_gain = FLO_GainTable - gp->global_gain;
        int              window = 0;
    	int              subband = 0;
        int*             scalefactor;
        int              sample_spacing;
        FLO_Float*       samples_start = samples;

        if (gp->mixed_block_flag) {
            switch_point = 8; /* start with long, and then switch to short */
            subband_info = 
                &FLO_SubbandInfo_Long
                [frame->header.id]
                [frame->header.sampling_frequency][0];
            scalefactor = gp->scalefactors.l;
            sample_spacing = 1;
            switched = FLO_FALSE;
        } else {
            switch_point = 0; /* all short */
            subband_info = 
                &FLO_SubbandInfo_Short
                [frame->header.id]
                [frame->header.sampling_frequency][0];
            scalefactor = (int*)gp->scalefactors.s;
            sample_spacing = 3;
            switched = FLO_TRUE;
        }

        /* get the gain tables for the short windows */
        if (frame->ms_stereo) long_gain += 2;
        short_gain[0] = long_gain + (gp->subblock_gain[0]<<3);
        short_gain[1] = long_gain + (gp->subblock_gain[1]<<3);
        short_gain[2] = long_gain + (gp->subblock_gain[2]<<3);

        /* regions 0,1: pairs */
        for (region = 0; region < 2; region++) {
            const FLO_HuffmanTable* table = 
                &FLO_HuffmanTables_Pair[gp->table_selection[region]];

            for (i = region_size[region]/2; i; i--) {
                BOUNDARY_CHECKPOINT_SHORT;
                FLO_Huffman_DecodePair(bits, table, factor, 
                                       samples, bits_left, sample_spacing);
            }
        }

        /* region 3: quads */
        {
            const FLO_HuffmanTable* table = 
                &FLO_HuffmanTables_Quad[gp->count1_table_selection];
            int quad;
            for (i = region_size[3]/4; i && bits_left > 0; i--) {
                FLO_Huffman_DecodeQuad(bits, table, quad, bits_left);

                /* store first 2 samples of the quad */
                BOUNDARY_CHECKPOINT_SHORT;
                STORE_QUAD_N(samples, factor, quad, 0, bits, 
                             bits_left, sample_spacing);
                STORE_QUAD_N(samples, factor, quad, 1, bits, 
                             bits_left, sample_spacing);

                /* store last 2 samples of the quad */
                BOUNDARY_CHECKPOINT_SHORT;
                STORE_QUAD_N(samples, factor, quad, 2, bits, 
                             bits_left, sample_spacing);
                STORE_QUAD_N(samples, factor, quad, 3, bits, 
                             bits_left, sample_spacing);
            }
        }

        /* if we end up with a negative bit count, discard last quad value */
        if (bits_left < 0) {
            samples[-1*sample_spacing] = FLO_ZERO;
            samples[-2*sample_spacing] = FLO_ZERO;
            samples[-3*sample_spacing] = FLO_ZERO;
            samples[-4*sample_spacing] = FLO_ZERO;
        }

        /* zero out the rest */
        while (1) {
            BOUNDARY_CHECKPOINT_SHORT_QUICK_BREAK;
            *samples = FLO_ZERO;
            samples += sample_spacing;
            *samples = FLO_ZERO;
            samples += sample_spacing;
        }
    } else {
        /* long block */
        int region, i;
        const int *pretab = 
            gp->preflag ? FLO_LayerIII_Pretab : FLO_LayerIII_NullPretab;
        int *scalefactor = gp->scalefactors.l;
        const FLO_Float* gain = FLO_GainTable - gp->global_gain;

        if (frame->ms_stereo) gain += 2;
        subband_info = 
            &FLO_SubbandInfo_Long
            [frame->header.id]
            [frame->header.sampling_frequency][0];

        /* regions 0,1,2: pairs */
        for (region = 0; region < 3; region++) {
            const FLO_HuffmanTable* table = 
                &FLO_HuffmanTables_Pair[gp->table_selection[region]]; 

            for (i = region_size[region]/2; i; i--) {
                BOUNDARY_CHECKPOINT_LONG;
                FLO_Huffman_DecodePair(bits, table, factor, samples, bits_left,1);
            }
        }       

        /* region 3: quads */
        {
            const FLO_HuffmanTable* table = 
                &FLO_HuffmanTables_Quad[gp->count1_table_selection];
            int quad;

            for (i = region_size[3]/4; i && bits_left > 0; i--) {
                FLO_Huffman_DecodeQuad(bits, table, quad, bits_left);

                /* store first 2 samples of the quad */
                BOUNDARY_CHECKPOINT_LONG;
                STORE_QUAD_1(samples, factor, quad, 0, bits, bits_left);
                STORE_QUAD_1(samples, factor, quad, 1, bits, bits_left);

                /* store last 2 samples of the quad */
                BOUNDARY_CHECKPOINT_LONG;
                STORE_QUAD_1(samples, factor, quad, 2, bits, bits_left);
                STORE_QUAD_1(samples, factor, quad, 3, bits, bits_left);
            }
        }

        /* if we end up with a negative bit count, discard last quad value */
        if (bits_left < 0) {
            samples -= 4;
        }

        /* zero out the rest and count the bands with all zeros */
        { 
            int nb_zeros = 
                (int)(&frame->hybrid[channel].in[FLO_HYBRID_NB_BANDS][0] - samples);
            frame->hybrid[channel].nb_zero_bands = nb_zeros/FLO_HYBRID_BAND_WIDTH;
            for (i=0; i<nb_zeros/2; i++) {
                *samples++ = FLO_ZERO;
                *samples++ = FLO_ZERO;
            }
        }
    }

    /* flush the end of the granule */
    if (bits_left) {
        if (bits_left > 0) {
            while (bits_left >= 32) {
                FLO_BitStream_ReadBits(bits, 32);
                bits_left -= 32;
            }
            if (bits_left) FLO_BitStream_ReadBits(bits, bits_left);
        } else {
            while (bits_left <= -32) {
                FLO_BitStream_Rewind(bits, 32);
                bits_left += 32;
            }
            if (bits_left) FLO_BitStream_Rewind(bits, -bits_left);
        }
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_MsStereo
+-------------------------------------------------------------------------*/
static void
FLO_LayerIII_MsStereo(FLO_Frame_III* frame)
{
    FLO_Float* samples_left  = (FLO_Float*)frame->hybrid[0].in;
    FLO_Float* samples_right = (FLO_Float*)frame->hybrid[1].in;
    int i;

    for (i = 0; i < FLO_SYNTAX_MPEG_LAYER_III_NB_FREQUENCY_LINES; i++) {
        FLO_Float left = *samples_left;
        *samples_left++  += *samples_right;
        *samples_right = left - *samples_right;
        samples_right++;
    }

    /* we modified the samples, so some bands might not be null anymore */
    if (frame->hybrid[0].nb_zero_bands < frame->hybrid[1].nb_zero_bands) {
        frame->hybrid[1].nb_zero_bands = frame->hybrid[0].nb_zero_bands;
    } else {
        frame->hybrid[0].nb_zero_bands = frame->hybrid[1].nb_zero_bands;
    }
}

/*-------------------------------------------------------------------------
|       BUTTERFLY
+-------------------------------------------------------------------------*/
#define BUTTERFLY(samples, u, d, n)                                     \
{                                                                       \
    FLO_Float save = samples[u];                                        \
    samples[u] = FLO_FC4_MUL(samples[u], FLO_CsTable[n]) -              \
                 FLO_FC4_MUL(samples[d], FLO_CaTable[n]);               \
    samples[d] = FLO_FC4_MUL(samples[d], FLO_CsTable[n]) +              \
                 FLO_FC4_MUL(save      , FLO_CaTable[n]);               \
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_Antialias
+-------------------------------------------------------------------------*/
static void 
FLO_LayerIII_Antialias(FLO_Frame_III* frame, int granule, int channel)
{
    FLO_Granule* gp = &frame->side_info.granules[granule][channel];
    int          subband_max;
    int          subband;
    FLO_Float*   samples = (FLO_Float *) frame->hybrid[channel].in;
    
    if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS) {
        if (gp->mixed_block_flag) {
            subband_max = 2; /* first 2 subbands are long blocks */
        } else {
            return; /* no antialias for small blocks */
        }
    } else {
        subband_max = FLO_HYBRID_NB_BANDS;
    }

    for(subband = subband_max-1; subband; subband--) {
        BUTTERFLY(samples, 17, 18, 0)
        BUTTERFLY(samples, 16, 19, 1)
        BUTTERFLY(samples, 15, 20, 2)
        BUTTERFLY(samples, 14, 21, 3)
        BUTTERFLY(samples, 13, 22, 4)
        BUTTERFLY(samples, 12, 23, 5)
        BUTTERFLY(samples, 11, 24, 6)
        BUTTERFLY(samples, 10, 25, 7)

        samples += FLO_HYBRID_BAND_WIDTH;
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_Hybrid
+-------------------------------------------------------------------------*/
static void 
FLO_LayerIII_Hybrid(FLO_Frame_III* frame, int granule, int channel)
{
    FLO_Granule*      gp = &frame->side_info.granules[granule][channel];
    FLO_HybridFilter* filter = &frame->hybrid[channel];
    int               subband;
    int               non_zero = FLO_HYBRID_NB_BANDS - filter->nb_zero_bands;
        
    /* due to the anti-aliasing, there is a one-band cross-over
    **   so we cannot assume the last band will remain null after
    **   anti-alising */
    if (non_zero != FLO_HYBRID_NB_BANDS) non_zero++;

    /* if we subsample, force more bands to zero */
    if (frame->subsampling) {
        int max_subband = FLO_HYBRID_NB_BANDS >> frame->subsampling;
        if (non_zero > max_subband) non_zero = max_subband;
    }

    if (gp->block_type == FLO_SYNTAX_MPEG_LAYER_III_BLOCK_TYPE_SHORT_WINDOWS) {
        /* do all the bands */
        for (subband = 0; subband < FLO_HYBRID_NB_BANDS; subband++) {
            FLO_HybridFilter_Imdct_12(filter, subband);
        }
    } else {
        /* do the non null bands */
        for (subband = 0; subband < non_zero; subband++) {
            FLO_HybridFilter_Imdct_36(filter, subband, gp->block_type);
        }
    }

    /* do the null (full of zeros) bands */
    for (subband = non_zero; subband < FLO_HYBRID_NB_BANDS; subband++) {
        FLO_HybridFilter_Imdct_Null(filter, subband);
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_ReadAndProcessMainData
+-------------------------------------------------------------------------*/
static FLO_Result
FLO_LayerIII_ReadAndProcessMainData(FLO_BitStream*       bits, 
                                    FLO_Frame_III*       frame,
                                    FLO_SynthesisFilter* filter_left,
                                    FLO_SynthesisFilter* filter_right)
{
    int granule;

    /* prepare for subsampling */
    frame->subsampling = 
        filter_left ? filter_left->subsampling : filter_right->subsampling;

    if (frame->header.mode == FLO_MPEG_MODE_SINGLE_CHANNEL) {
        /* mono */
        int                  subband;
        FLO_SynthesisFilter* filter = filter_left ? filter_left : filter_right;

        for (granule = 0; granule < frame->nb_granules; granule++) {
            FLO_LayerIII_ReadScalefactors(bits, frame, granule, 0);
            FLO_LayerIII_ReadHuffmanSamples(bits, frame, granule, 0);
            FLO_LayerIII_Antialias(frame, granule, 0);
            FLO_LayerIII_Hybrid(frame, granule, 0);

            for (subband = 0; subband< FLO_HYBRID_BAND_WIDTH; subband++) {
                filter->input  = frame->hybrid[0].out[subband];
                FLO_SynthesisFilter_ComputePcm(filter);
            }   
        }
    } else {
        if (filter_left == filter_right) {
            /* mix left + right */
            for (granule = 0; granule < frame->nb_granules; granule++) {
                int group;

                FLO_LayerIII_ReadScalefactors   (bits, frame, granule, 0);
                FLO_LayerIII_ReadHuffmanSamples(bits, frame, granule, 0);
                FLO_LayerIII_ReadScalefactors   (bits, frame, granule, 1);
                FLO_LayerIII_ReadHuffmanSamples(bits, frame, granule, 1);

                if (frame->ms_stereo) FLO_LayerIII_MsStereo(frame);
            
                FLO_LayerIII_Antialias(frame, granule, 0);
                FLO_LayerIII_Antialias(frame, granule, 1);

                FLO_LayerIII_Hybrid(frame, granule, 0);
                FLO_LayerIII_Hybrid(frame, granule, 1);
                
                for (group = 0; group< FLO_HYBRID_BAND_WIDTH; group++) {
                    int subband;
                    for (subband = 0; subband < FLO_HYBRID_NB_BANDS; subband++) {
                        frame->hybrid[0].out[group][subband] =
                            FLO_FDIV2((frame->hybrid[0].out[group][subband] +
                                       frame->hybrid[1].out[group][subband]));
                    }
                    filter_left->input = frame->hybrid[0].out[group];
                    FLO_SynthesisFilter_ComputePcm(filter_left);
                }
            }
        } else {
            /* stereo */
            for (granule = 0; granule < frame->nb_granules; granule++) {
                int group;

                FLO_LayerIII_ReadScalefactors  (bits, frame, granule, 0);
                FLO_LayerIII_ReadHuffmanSamples(bits, frame, granule, 0);
                FLO_LayerIII_ReadScalefactors  (bits, frame, granule, 1);
                FLO_LayerIII_ReadHuffmanSamples(bits, frame, granule, 1);

                if (frame->ms_stereo) FLO_LayerIII_MsStereo(frame);
            
                FLO_LayerIII_Antialias(frame, granule, 0);
                FLO_LayerIII_Antialias(frame, granule, 1);

                if (filter_left && filter_right) {
                    FLO_LayerIII_Hybrid(frame, granule, 0);
                    FLO_LayerIII_Hybrid(frame, granule, 1);
                
                    for (group = 0; group< FLO_HYBRID_BAND_WIDTH; group++) {
                        filter_left->input  = frame->hybrid[0].out[group];
                        FLO_SynthesisFilter_ComputePcm(filter_left);
                        filter_right->input = frame->hybrid[1].out[group];
                        FLO_SynthesisFilter_ComputePcm(filter_right);
                    }
                } else {
                    if (filter_left) {
                        FLO_LayerIII_Hybrid(frame, granule, 0);
                        for (group = 0; 
                             group< FLO_HYBRID_BAND_WIDTH; 
                             group++) {
                            filter_left->input = 
                                frame->hybrid[0].out[group];
                            FLO_SynthesisFilter_ComputePcm(filter_left);
                        }
                    } else {
                        FLO_LayerIII_Hybrid(frame, granule, 1);
                        for (group = 0; 
                             group< FLO_HYBRID_BAND_WIDTH; 
                             group++) {
                            filter_right->input = 
                                frame->hybrid[1].out[group];
                            FLO_SynthesisFilter_ComputePcm(filter_right);
                        }
                    }
                }
            }
        }
    }

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_NullFrame
+-------------------------------------------------------------------------*/
static void
FLO_LayerIII_NullFrame(FLO_Frame_III*       frame,  
                       FLO_SynthesisFilter* filter_left,
                       FLO_SynthesisFilter* filter_right)
{
    int granule;
    int subband;
    
    for (granule = 0; granule < frame->nb_granules; granule++) {
        if (filter_left) {
            for (subband = 0; subband < FLO_HYBRID_BAND_WIDTH; subband++) {
                FLO_SynthesisFilter_NullPcm(filter_left);
            }
        }
        if (filter_right) {
            for (subband = 0; subband < FLO_HYBRID_BAND_WIDTH; subband++) {
                FLO_SynthesisFilter_NullPcm(filter_right);
            }
        }
    }
}

/*-------------------------------------------------------------------------
|       FLO_LayerIII_DecodeFrame
+-------------------------------------------------------------------------*/
FLO_Result
FLO_LayerIII_DecodeFrame(const unsigned char* frame_data, 
                         const FLO_FrameInfo* frame_info,
                         FLO_Frame_III*       frame,
                         FLO_MainDataBuffer*  main_data_buffer,
                         FLO_SynthesisFilter* filter_left, 
                         FLO_SynthesisFilter* filter_right)
{
    FLO_BitStream        bits;
    unsigned int         side_info_size;
    const unsigned char* main_data_start;
    FLO_Size             main_data_size;
    FLO_Size             frame_payload_size = frame_info->size-4;
    FLO_Result           result = FLO_SUCCESS;

    /* if the frame has a CRC, update the frame size */
    if (frame->header.protection_bit == 0) {
        frame_payload_size -= 2;
    }
    
    /* the frame has two parts: the side info and the main data */
    side_info_size = FLO_LayerIII_GetSideInfoSize(&frame->header);

    /* check that we have enough data */
    if (frame_payload_size < side_info_size) {
        /* something is wrong */
        result = FLO_ERROR_INVALID_BITSTREAM;
        goto err;
    }
    
    /* setup the bitstream */
    FLO_BitStream_SetData(&bits, frame_data, frame_payload_size);

    /* setup the frame */
    FLO_CHECK(FLO_LayerIII_InitFrame(frame));
    FLO_CHECK(FLO_LayerIII_ReadSideInfo(&bits, frame));

    /* switch to the main data bits */
    main_data_start = frame_data+side_info_size;
    main_data_size  = frame_payload_size - side_info_size;
    
    /* check if we have enough in the buffer */
    if (main_data_buffer->available < frame->side_info.main_data_begin) {
        /* not enough main data: we can't decode, but we still need to buffer */
        /* the main data from this frame.                                     */
		unsigned int can_fit = sizeof(main_data_buffer->data)-main_data_buffer->available;
		if (can_fit < main_data_size) {
			/* we can't fit this frame's entire main data, shift to make some room */
			unsigned int shrink = main_data_size-can_fit;
			FLO_MoveMemory(main_data_buffer->data, 
				           main_data_buffer->data+shrink,
				           main_data_size);
			main_data_buffer->available -= shrink;
		}
        if (main_data_size) {
            FLO_CopyMemory(main_data_buffer->data+main_data_buffer->available,
                           main_data_start, 
                           main_data_size);
        }
        main_data_buffer->available += main_data_size;
        result = FLO_ERROR_INVALID_BITSTREAM;
        goto err;
    }

    if (frame->side_info.main_data_begin) {
        /* align the start of this frame's main data at the start of the buffer */
        FLO_MoveMemory(main_data_buffer->data, 
                       main_data_buffer->data+main_data_buffer->available-frame->side_info.main_data_begin,
                       frame->side_info.main_data_begin);
    }
    if (main_data_size) {
        /* add this frame's main data payload to the buffer */
        FLO_CopyMemory(main_data_buffer->data+frame->side_info.main_data_begin,
                       main_data_start, 
                       main_data_size);
    }
    main_data_buffer->available = frame->side_info.main_data_begin+main_data_size;
    FLO_BitStream_SetData(&bits, main_data_buffer->data, main_data_buffer->available);

    /* decode the main data */
    FLO_CHECK(FLO_LayerIII_ReadAndProcessMainData(&bits, 
                                                  frame, 
                                                  filter_left, 
                                                  filter_right));

    return FLO_SUCCESS;

err:
    FLO_LayerIII_NullFrame(frame, filter_left, filter_right);
    FLO_LayerIII_ResetFrame(frame);

    return result;
}

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */



