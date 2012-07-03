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
Single Channel Element & LFE channel Element

Tow of the Syntactic Elements contained in the raw data blocks.

Ref: 4.4.2.1, Table 4.4
*/



#ifndef _MLO_ELEMENT_SCE_LFE_H_
#define _MLO_ELEMENT_SCE_LFE_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloSamplingFreq.h"
#include "MloBitStream.h"
#include "MloIndivChnPool.h"
#include "MloIndivChnStream.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



/* Ref: 4.4.2.1, Table 4.4 */
typedef struct MLO_ElementSceLfe
{
   int            element_instance_tag;
   MLO_IndivChnStream *
                  ics_ptr;  /* Allocated by the channel pool */
}  MLO_ElementSceLfe;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_ElementSceLfe_Decode (MLO_ElementSceLfe *sce_ptr, MLO_BitStream *bit_ptr, MLO_IndivChnPool *chn_pool_ptr, MLO_SamplingFreq_Index fs_index);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_ELEMENT_SCE_LFE_H_ */
