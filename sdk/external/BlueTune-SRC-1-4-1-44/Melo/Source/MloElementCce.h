/*****************************************************************
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*
Ref:
4.4.2.1, Table 4.8 (p 17)
4.6.8.3 (p 153)
*/

#ifndef _MLO_ELEMENT_CCE_H_
#define _MLO_ELEMENT_CCE_H_

/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/
#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloDefs.h"
#include "MloIndivChnPool.h"
#include "MloIndivChnStream.h"
#include "MloTypes.h"

/*----------------------------------------------------------------------
|       Data
+---------------------------------------------------------------------*/
enum {   MLO_ELEMENT_CCE_MAX_NBR_TARGETS  = (1<<3) + 1   };
enum {   MLO_ELEMENT_CCE_MAX_NBR_G_E_L    = MLO_ELEMENT_CCE_MAX_NBR_TARGETS * 2  };

/* For implementation */
enum {   MLO_ELEMENT_CCE_POW2_RES_L2   = 3   }; /* Bits */
enum {   MLO_ELEMENT_CCE_POW2_RES      = 1 << MLO_ELEMENT_CCE_POW2_RES_L2   };

/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/
/* Forward declarations */
struct MLO_SyntacticElements;



typedef enum MLO_ElementCce_Stage
{
   MLO_ELEMENT_CCE_STAGE_DEP_BEFORE_TNS = 0,
   MLO_ELEMENT_CCE_STAGE_DEP_AFTER_TNS,
   MLO_ELEMENT_CCE_STAGE_INDEP,

   MLO_ELEMENT_CCE_STAGE_NBR_ELT
}  MLO_ElementCce_Stage;

typedef struct MLO_ElementCce_CcTarget
{
   MLO_UInt8      is_cpe;     /* Boolean */
   MLO_UInt8      tag_select;
   MLO_UInt8      cc_l;       /* Boolean */
   MLO_UInt8      cc_r;       /* Boolean */
}  MLO_ElementCce_CcTarget;

typedef struct MLO_ElementCce_GainElementList
{
   MLO_UInt8      cge_flag;   /* Boolean, true = common, false = dpcm */
   MLO_UInt8      common_gain_element;
   MLO_UInt8      dpcm_gain_element [MLO_DEFS_MAX_NBR_WIN_GRP] [MLO_DEFS_MAX_NUM_SWB + 1];  /* [g] [sfb] */
}  MLO_ElementCce_GainElementList;

typedef struct MLO_ElementCce
{
   int            element_instance_tag;
   MLO_Boolean    ind_sw_cce_flag;
   int            num_coupled_elements;
   MLO_ElementCce_CcTarget
                  cc_target_arr [MLO_ELEMENT_CCE_MAX_NBR_TARGETS];
   MLO_Boolean    cc_domain;  /* True = apply after TNS */
   MLO_Boolean    gain_element_sign;
   int            gain_element_scale;
   int            num_gain_element_lists;
   MLO_ElementCce_GainElementList
                  gel_arr [MLO_ELEMENT_CCE_MAX_NBR_G_E_L];  /* Index 0 is not used */
   MLO_IndivChnStream *
                  ics_ptr;    /* Allocated by the channel pool */
}  MLO_ElementCce;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_ElementCce_Decode (MLO_ElementCce *cce_ptr, MLO_BitStream *bit_ptr, MLO_IndivChnPool *chn_pool_ptr, MLO_SamplingFreq_Index fs_index);
MLO_Result  MLO_ElementCce_Process (const MLO_ElementCce *cce_ptr, struct MLO_SyntacticElements *se_ptr, MLO_ElementCce_Stage stage);




#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_ELEMENT_CCE_H_ */
