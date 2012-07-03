/*****************************************************************
|
|      MPEG audio frame handling. Layer II
|
|      (c) 1996-1998 MpegTV, LLC
|      Author: Gilles Boccon-Gibod (gilles@mpegtv.com)
|
 ****************************************************************/

#ifndef _FLO_LAYERII_H_
#define _FLO_LAYERII_H_

/*-------------------------------------------------------------------------
|       includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloSyntax.h"
#include "FloBitStream.h"
#include "FloFilter.h"
#include "FloTables.h"
#include "FloFrame.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|       constants
+-------------------------------------------------------------------------*/
#define FLO_MPEG_LAYER_II_MAX_SUBBANDS              32
#define FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS          12
#define FLO_MPEG_LAYER_II_SAMPLE_GROUP_SIZE         3
#define FLO_MPEG_LAYER_II_NB_SAMPLE_ZONES           3
#define FLO_MPEG_LAYER_II_PCM_SAMPLES_PER_FRAME     1152

/*-------------------------------------------------------------------------
|       types
+-------------------------------------------------------------------------*/
typedef struct {
    const FLO_AllocationTableEntry* table;
    unsigned int                    allocation[2];
    FLO_LayerII_QuantInfo           info[2];
    unsigned int                    scalefactor_selection[2];
    FLO_Float                       scalefactor[2][FLO_MPEG_LAYER_II_NB_SAMPLE_ZONES];
    FLO_Float                       samples[2]
                                           [FLO_MPEG_LAYER_II_NB_SAMPLE_GROUPS]
                                           [FLO_MPEG_LAYER_II_SAMPLE_GROUP_SIZE];
} FLO_Subband_II;

typedef struct {
    FLO_FrameHeader header;
    int             joint_stereo_bound;
    int             nb_subbands;
    FLO_Subband_II  subbands[FLO_MPEG_LAYER_II_MAX_SUBBANDS];
    FLO_Float       samples[2][FLO_FILTER_NB_SAMPLES];
} FLO_Frame_II;

/*-------------------------------------------------------------------------
|       prototypes
+-------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

unsigned int 
FLO_LayerII_GetFrameSize(FLO_FrameHeader* header);

FLO_Result 
FLO_LayerII_DecodeFrame(const unsigned char* frame_data, 
                        const FLO_FrameInfo* frame_info,
                        FLO_Frame_II*        frame, 
                        FLO_SynthesisFilter* left, 
                        FLO_SynthesisFilter* right);

#ifdef __cplusplus
}
#endif

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

#endif /* _FLO_LAYERII_H_ */
