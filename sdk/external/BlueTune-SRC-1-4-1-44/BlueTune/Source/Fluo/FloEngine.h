/*****************************************************************
|
|   Fluo - Frame Decoding Engine
|
|   (c) 2002-20076 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_ENGINE_H_
#define _FLO_ENGINE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"
#include "FloFrame.h"
#include "FloDecoder.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct FLO_Engine FLO_Engine;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
FLO_Result FLO_Engine_Create(FLO_Engine** engine);
FLO_Result FLO_Engine_Destroy(FLO_Engine* engine);
FLO_Result FLO_Engine_Reset(FLO_Engine* engine);
FLO_Result FLO_Engine_DecodeFrame(FLO_Engine*          engine, 
                                  const FLO_FrameInfo* frame_info,
                                  const unsigned char* frame_data,
                                  FLO_SampleBuffer*    sample_buffer);

#endif /* _FLO_ENGINE_H_ */
