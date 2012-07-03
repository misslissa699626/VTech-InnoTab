/*****************************************************************
|
|   Fluo - Layer I Decoding
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_LAYERI_H_
#define _FLO_LAYERI_H_

/*-------------------------------------------------------------------------
|   includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloSyntax.h"
#include "FloBitStream.h"
#include "FloTables.h"
#include "FloFilter.h"
#include "FloFrame.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|   constants
+-------------------------------------------------------------------------*/
#define FLO_MPEG_LAYER_I_SUBBANDS                   32
#define FLO_MPEG_LAYER_I_NB_SAMPLES                 12
#define FLO_MPEG_LAYER_I_BYTES_PER_SLOT             4
#define FLO_MPEG_LAYER_I_PCM_SAMPLES_PER_FRAME      384

/*-------------------------------------------------------------------------
|   types
+-------------------------------------------------------------------------*/
typedef struct {
    unsigned int allocation[2];
    FLO_Float    scalefactor[2];
    FLO_Float    samples[2][FLO_MPEG_LAYER_I_NB_SAMPLES];
} FLO_Subband_I;
    
typedef struct {
    FLO_FrameHeader header;
    int             joint_stereo_bound;        
    FLO_Subband_I   subbands[FLO_MPEG_LAYER_I_SUBBANDS];
    FLO_Float       samples[FLO_FILTER_NB_SAMPLES];
} FLO_Frame_I;

/*-------------------------------------------------------------------------
|   prototypes
+-------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

unsigned int 
FLO_LayerI_GetFrameSize(FLO_FrameHeader* header);

FLO_Result 
FLO_LayerI_DecodeFrame(const unsigned char* frame_data, 
                       const FLO_FrameInfo* frame_info,
                       FLO_Frame_I*         frame, 
                       FLO_SynthesisFilter* left, 
                       FLO_SynthesisFilter* right);

#ifdef __cplusplus
}
#endif

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

#endif /* _FLO_LAYERI_H_ */
