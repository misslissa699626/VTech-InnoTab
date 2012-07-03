/*****************************************************************
|
|      MPEG audio frame handling. Layer III
|
|      (c) 1996-1998 MpegTV, LLC
|      Author: Gilles Boccon-Gibod (gilles@mpegtv.com)
|
 ****************************************************************/

#ifndef _FLO_LAYERIII_H_
#define _FLO_LAYERIII_H_

/*-------------------------------------------------------------------------
|       includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloSyntax.h"
#include "FloBitStream.h"
#include "FloFilter.h"
#include "FloTables.h"
#include "FloFrame.h"
#include "FloEngine.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|       constants
+-------------------------------------------------------------------------*/
#define FLO_MPEG_LAYER_III_MPEG_1_PCM_SAMPLES_PER_FRAME     1152
#define FLO_MPEG_LAYER_III_MPEG_2_PCM_SAMPLES_PER_FRAME     576
#define FLO_MPEG_LAYER_III_NB_SCFSI_BANDS                   4
#define FLO_MPEG_LAYER_III_NB_GRANULES                      2
#define FLO_MPEG_LAYER_III_NB_WINDOWS                       3
#define FLO_MPEG_LAYER_III_MPEG_1_SIDE_INFO_SIZE_MONO       17
#define FLO_MPEG_LAYER_III_MPEG_1_SIDE_INFO_SIZE_STEREO     32
#define FLO_MPEG_LAYER_III_MPEG_2_SIDE_INFO_SIZE_MONO       9
#define FLO_MPEG_LAYER_III_MPEG_2_SIDE_INFO_SIZE_STEREO     17
    
#define FLO_MAX_BIT_RESERVOIR_SIZE 512

/*-------------------------------------------------------------------------
|       types
+-------------------------------------------------------------------------*/
typedef struct {
    int s[13][3];
    int l[22];
} FLO_ScalefactorValues;

typedef struct {
    unsigned int          part_2_3_length;
    unsigned int          part_2_length; /* not in bitstream */
    unsigned int          big_values;
    unsigned int          global_gain;
    unsigned int          scalefactor_compression;
    unsigned int          window_switching_flag;

    unsigned int          block_type;
    unsigned int          mixed_block_flag;
    unsigned int          table_selection[3];
    unsigned int          subblock_gain[FLO_MPEG_LAYER_III_NB_WINDOWS];

    unsigned int          region0_count;
    unsigned int          region1_count;
    unsigned int          region1_start; /* not in bitstream */
    unsigned int          region2_start; /* not in bitstream */

    unsigned int          preflag;
    unsigned int          scalefactor_scale;
    unsigned int          count1_table_selection;

    FLO_ScalefactorValues scalefactors;
} FLO_Granule;

typedef struct {
    unsigned int   main_data_begin;
    unsigned int   private_bits;
    unsigned int   scalefactor_selection[2][FLO_MPEG_LAYER_III_NB_SCFSI_BANDS];
    FLO_Granule    granules[FLO_MPEG_LAYER_III_NB_GRANULES][2];
} FLO_SideInfo;

typedef struct {
    unsigned char data[FLO_FRAME_BUFFER_SIZE+FLO_MAX_BIT_RESERVOIR_SIZE];
    unsigned int  available;
} FLO_MainDataBuffer;

typedef struct {
    FLO_FrameHeader  header;
    FLO_SideInfo     side_info;
    int              nb_granules;
    int              ms_stereo;
    int              intensity_stereo;
    FLO_HybridFilter hybrid[2];
    int              subsampling;
} FLO_Frame_III;

/*-------------------------------------------------------------------------
|       prototypes
+-------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void 
FLO_LayerIII_ResetFrame(FLO_Frame_III* frame);

unsigned int 
FLO_LayerIII_GetFrameSize(FLO_FrameHeader *header);

unsigned int 
FLO_LayerIII_GetSideInfoSize(FLO_FrameHeader *header);

FLO_Result   
FLO_LayerIII_DecodeFrame(const unsigned char* frame_data, 
                         const FLO_FrameInfo* frame_info,
                         FLO_Frame_III*       frame, 
                         FLO_MainDataBuffer*  main_data_buffer,
                         FLO_SynthesisFilter* left, 
                         FLO_SynthesisFilter* right);

#ifdef __cplusplus
}
#endif

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

#endif /* _FLO_LAYERIII_H_ */
