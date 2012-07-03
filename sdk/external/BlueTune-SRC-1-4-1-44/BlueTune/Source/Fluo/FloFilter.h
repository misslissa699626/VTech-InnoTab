/*****************************************************************
|
|   Fluo - Synthesis Filter
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_FILTER_H_
#define _FLO_FILTER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloMath.h"
#include "FloTypes.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define FLO_FILTER_NB_SAMPLES                       32
#define FLO_FILTER_BAND_WIDTH                       32
#define FLO_HYBRID_NB_BANDS                         32
#define FLO_HYBRID_BAND_WIDTH                       18
#define FLO_FILTER_FEEDBACK_NB_FREQUENCY_SAMPLES    32

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    FLO_Float* v0;
    FLO_Float* v1;
    FLO_Float* v;
    FLO_Float* input;
    FLO_Float* equalizer;
    int        subsampling;
    int        v_offset;
    short*     buffer;
    int        buffer_increment;
} FLO_SynthesisFilter;

typedef struct {
    FLO_Float in   [FLO_HYBRID_NB_BANDS][FLO_HYBRID_BAND_WIDTH];
    FLO_Float out  [FLO_HYBRID_BAND_WIDTH][FLO_HYBRID_NB_BANDS];
    FLO_Float store[FLO_HYBRID_NB_BANDS][FLO_HYBRID_BAND_WIDTH];
    int       nb_zero_bands;
} FLO_HybridFilter;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

FLO_Result FLO_SynthesisFilter_Create(FLO_SynthesisFilter** filter);
void       FLO_SynthesisFilter_Destroy(FLO_SynthesisFilter* filter);
void       FLO_SynthesisFilter_Reset(FLO_SynthesisFilter* filter);
void       FLO_SynthesisFilter_ComputePcm(FLO_SynthesisFilter* filter);
void       FLO_SynthesisFilter_NullPcm(FLO_SynthesisFilter* filter);
void       FLO_HybridFilter_Reset(FLO_HybridFilter* filter);
void       FLO_HybridFilter_Imdct_36(FLO_HybridFilter* filter, int group, int window_type);
void       FLO_HybridFilter_Imdct_12(FLO_HybridFilter* filter, int group);
void       FLO_HybridFilter_Imdct_Null(FLO_HybridFilter* filter, int group);

#ifdef __cplusplus
}
#endif

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

#endif /* _FLO_FILTER_H_ */

