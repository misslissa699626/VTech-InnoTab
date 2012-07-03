/*****************************************************************
|
|   Fluo - Frame
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloTypes.h"
#include "FloErrors.h"
#include "FloFrame.h"
#include "FloTables.h"

/*----------------------------------------------------------------------
|   FLO_FrameHeader_GetInfo
+---------------------------------------------------------------------*/
void
FLO_FrameHeader_GetInfo(FLO_FrameHeader* header, FLO_FrameInfo* info)
{
    unsigned int slots;

    info->level       = 2-header->id;
    info->layer       = 4-header->layer;
    info->mode        = header->mode;
    info->bitrate     = 
        FLO_MpegBitrates[header->id][3-header->layer][header->bitrate_index] *
        1000;
    info->sample_rate = 
        FLO_MpegSamplingFrequencies[header->id][header->sampling_frequency];
    if (header->mode == FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
        info->channel_count = 1;
    } else {
        info->channel_count = 2;
    }
    if (header->layer == FLO_SYNTAX_MPEG_LAYER_I) {
        info->sample_count = FLO_SYNTAX_MPEG_LAYER_I_PCM_SAMPLES_PER_FRAME;
    } else if (header->layer == FLO_SYNTAX_MPEG_LAYER_II) {
        info->sample_count = FLO_SYNTAX_MPEG_LAYER_II_PCM_SAMPLES_PER_FRAME;
    } else {
        if (header->id == FLO_SYNTAX_MPEG_ID_MPEG_1) {
            info->sample_count = FLO_SYNTAX_MPEG_LAYER_III_MPEG1_PCM_SAMPLES_PER_FRAME;
        } else {
            info->sample_count = FLO_SYNTAX_MPEG_LAYER_III_MPEG2_PCM_SAMPLES_PER_FRAME;
        }
    }

    switch (header->layer) {
      case FLO_SYNTAX_MPEG_LAYER_I:
        slots = 
            (12 * FLO_MpegBitrates[header->id]
                                  [0]
                                  [header->bitrate_index]*1000)
            / FLO_MpegSamplingFrequencies[header->id]
                                         [header->sampling_frequency];
        if (header->padding_bit) slots++;
        info->size = slots * FLO_SYNTAX_MPEG_LAYER_I_BYTES_PER_SLOT;
        break;

      case FLO_SYNTAX_MPEG_LAYER_II:
        slots = 
            (144 * FLO_MpegBitrates[header->id]
                                   [1]
                                   [header->bitrate_index]*1000)
            / FLO_MpegSamplingFrequencies[header->id]
                                         [header->sampling_frequency];
        if (header->padding_bit) slots++;
        info->size = slots;
        break;

      case FLO_SYNTAX_MPEG_LAYER_III:
        slots = 
            (144 * FLO_MpegBitrates[header->id]
                                   [2]
                                   [header->bitrate_index]*1000)
            / FLO_MpegSamplingFrequencies[header->id]
                                         [header->sampling_frequency];
        if (header->id == FLO_SYNTAX_MPEG_ID_MPEG_2 ||
            header->id == FLO_SYNTAX_MPEG_ID_MPEG_2_5) {
            slots /= 2;
        }
        if (header->padding_bit) slots++;
        info->size = slots;
        break;

      default:
        info->size = 0;
    }
}

/*----------------------------------------------------------------------
|   FLO_FrameHeader_Unpack
+---------------------------------------------------------------------*/
void
FLO_FrameHeader_Unpack(FLO_UInt32 packed, FLO_FrameHeader* header)
{
    header->emphasis            = (unsigned char)(packed & 0x3); packed >>= 2;
    header->original            = (unsigned char)(packed & 0x1); packed >>= 1;
    header->copyright           = (unsigned char)(packed & 0x1); packed >>= 1;
    header->mode_extension      = (unsigned char)(packed & 0x3); packed >>= 2;
    header->mode                = (unsigned char)(packed & 0x3); packed >>= 2;
    header->private_bit         = (unsigned char)(packed & 0x1); packed >>= 1;
    header->padding_bit         = (unsigned char)(packed & 0x1); packed >>= 1;
    header->sampling_frequency  = (unsigned char)(packed & 0x3); packed >>= 2;
    header->bitrate_index       = (unsigned char)(packed & 0xF); packed >>= 4;
    header->protection_bit      = (unsigned char)(packed & 0x1); packed >>= 1;
    header->layer               = (unsigned char)(packed & 0x3); packed >>= 2;
    header->id                  = (unsigned char)(packed & 0x1); packed >>= 1;
    if ((packed & 0x1) == 0) {
        /* this is FHG's MPEG 2.5 extension */
        header->id += 2;
    }
}

/*----------------------------------------------------------------------
|   FLO_FrameHeader_FromBytes
+---------------------------------------------------------------------*/
void
FLO_FrameHeader_FromBytes(const FLO_Byte* bytes, FLO_FrameHeader* header)
{
    FLO_UInt32 packed = 
        (((FLO_UInt32)(bytes[0]))<<24 ) |
        (((FLO_UInt32)(bytes[1]))<<16 ) |
        (((FLO_UInt32)(bytes[2]))<< 8 ) |
        (((FLO_UInt32)(bytes[3]))     );
    FLO_FrameHeader_Unpack(packed, header);
}

/*----------------------------------------------------------------------
|   FLO_FrameHeader_Check
+---------------------------------------------------------------------*/
FLO_Result
FLO_FrameHeader_Check(FLO_FrameHeader* header)
{
    /* check the bitrate */
    if (header->bitrate_index == FLO_SYNTAX_MPEG_BITRATE_ILLEGAL) {
        return FLO_FAILURE;
    }

    /* we cannot handle free format bitrate */
    if (header->bitrate_index == FLO_SYNTAX_MPEG_BITRATE_FREE_FORMAT) {
        return FLO_FAILURE;
    }

    /* check the layer */
    if (header->layer == FLO_SYNTAX_MPEG_LAYER_RESERVED) {
        return FLO_FAILURE;
    }

    /* the padding bit is only used with 44100 and 22050 kHz */
    if (header->padding_bit &&
        (header->sampling_frequency != 
         FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_44100_22050)) {
        return FLO_FAILURE;
    }

    /* check the sampling frequency */
    if (header->sampling_frequency == 
        FLO_SYNTAX_MPEG_SAMPLING_FREQUENCY_RESERVED) {
        return FLO_FAILURE;
    }

    /* for MPEG1 layer II, only certain modes are valid */
    if (header->layer == FLO_SYNTAX_MPEG_LAYER_II && 
        header->id == FLO_SYNTAX_MPEG_ID_MPEG_1) {
        switch (header->bitrate_index) {
          case FLO_SYNTAX_MPEG_BITRATE_II_32:
          case FLO_SYNTAX_MPEG_BITRATE_II_48:
          case FLO_SYNTAX_MPEG_BITRATE_II_56:
          case FLO_SYNTAX_MPEG_BITRATE_II_80:
            if (header->mode != FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
                return FLO_FAILURE;
            }
            break;

          case FLO_SYNTAX_MPEG_BITRATE_II_224:
          case FLO_SYNTAX_MPEG_BITRATE_II_256:
          case FLO_SYNTAX_MPEG_BITRATE_II_320:
          case FLO_SYNTAX_MPEG_BITRATE_II_384:
            if (header->mode == FLO_SYNTAX_MPEG_MODE_SINGLE_CHANNEL) {
                return FLO_FAILURE;
            }
            break;
        }
    }

    /* check the id */
    if (header->id == FLO_SYNTAX_MPEG_ID_ILLEGAL) {
        return FLO_FAILURE;
    }

    /* for MPEG 2.5 extensions, we can only be in layer III */    
    if (header->id == FLO_SYNTAX_MPEG_ID_MPEG_2_5) {
        if (header->layer != FLO_SYNTAX_MPEG_LAYER_III) {
            return FLO_FAILURE;
        }
    }

    return FLO_SUCCESS;
}

