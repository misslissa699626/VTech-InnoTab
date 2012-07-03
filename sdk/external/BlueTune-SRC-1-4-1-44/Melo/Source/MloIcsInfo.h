/*
Individual Channel Stream information

Ref:
4.4.2.1, Table 4.6
4.5.2.3
*/

#ifndef _MLO_ICS_INFO_H_
#define _MLO_ICS_INFO_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloDefs.h"
#include "MloFloat.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



#define  MLO_ERROR_LTP_IN_LC        (MLO_ERROR_BASE_ICS_INFO-0)
#define  MLO_ERROR_UNSUPPORTED_SFI  (MLO_ERROR_BASE_ICS_INFO-1)



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



/* 4.5.4, Table 4.109 */
typedef enum MLO_IcsInfo_WinSeq
{
   MLO_ICS_INFO_WIN_ONLY_LONG_SEQUENCE = 0,
   MLO_ICS_INFO_WIN_LONG_START_SEQUENCE,
   MLO_ICS_INFO_WIN_EIGHT_SHORT_SEQUENCE,
   MLO_ICS_INFO_WIN_LONG_STOP_SEQUENCE
}  MLO_IcsInfo_WinSeq;



/* 4.6.11.3.2 */
typedef enum MLO_IcsInfo_WindowShape
{
   MLO_ICS_INFO_WINDOW_SHAPE_SINE = 0,
   MLO_ICS_INFO_WINDOW_SHAPE_KAISER,

   MLO_ICS_INFO_WINDOW_SHAPE_NBR_ELT
}  MLO_IcsInfo_WindowShape;

/* Shape information for previous and current frame */
typedef enum MLO_IcsInfo_WSIndex
{
   MLO_IcsInfo_WSIndex_PREV = 0,
   MLO_IcsInfo_WSIndex_CUR,

   MLO_IcsInfo_WSIndex_NBR_ELT
}  MLO_IcsInfo_WSIndex;



/* 4.4.2.1, Table 4.6 */
typedef struct MLO_IcsInfo
{
   MLO_SamplingFreq_Index
                  fs_index;

   /* Read data */
   MLO_IcsInfo_WinSeq
                  window_sequence;
   MLO_IcsInfo_WindowShape
                  window_shape [MLO_IcsInfo_WSIndex_NBR_ELT];
   int            max_sfb;
   int            scale_factor_grouping;

   /* Calculated data: window grouping info */
   int            num_windows;
   int            num_window_groups;
   int            num_swb;             /* Max: 51 */
   MLO_UInt8      window_group_length [MLO_DEFS_MAX_NBR_WIN_GRP];
   MLO_UInt16     swb_offset [MLO_DEFS_MAX_NUM_SWB + 1]; /* Array size: maximum size from the swb_offset tables */
   MLO_UInt16     sect_sfb_offset [MLO_DEFS_MAX_NBR_WIN_GRP] [MLO_DEFS_MAX_NUM_SWB + 1];

}  MLO_IcsInfo;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



void  MLO_IcsInfo_ClearBuffers (MLO_IcsInfo *ics_ptr);

MLO_Result  MLO_IcsInfo_Decode (MLO_IcsInfo *ics_ptr, MLO_BitStream *bit_ptr, MLO_SamplingFreq_Index fs_index);

void  MLO_IcsInfo_DeinterleaveCoefficients (const MLO_IcsInfo *ics_ptr, MLO_Float coef_ptr []);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_ICS_INFO_H_ */
