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

#ifndef _MLO_SECTION_DATA_H_
#define _MLO_SECTION_DATA_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloBitStream.h"
#include "MloConfig.h"
#include "MloIcsInfo.h"



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_SectionData
{
   MLO_UInt8      sect_cb [MLO_DEFS_MAX_NBR_WIN_GRP] [15*8];  /* [g] [i], g bounded by num_window_groups. Actual type is MLO_Hcb */
   MLO_UInt8      sect_start [MLO_DEFS_MAX_NBR_WIN_GRP] [15*8];
   MLO_UInt8      sect_end [MLO_DEFS_MAX_NBR_WIN_GRP] [15*8];
   MLO_UInt8      sfb_cb [MLO_DEFS_MAX_NBR_WIN_GRP] [15*8];   /* Actual type is MLO_Hcb */
   MLO_UInt8      num_sec [MLO_DEFS_MAX_NBR_WIN_GRP];
}  MLO_SectionData;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



void  MLO_SectionData_Decode (MLO_SectionData *sec_ptr, const MLO_IcsInfo *ics_ptr, MLO_BitStream *bit_ptr);
int   MLO_SectionData_IsIntensity (const MLO_SectionData *sec_ptr, int group, int sfb);
MLO_Boolean MLO_SectionData_IsNoise (const MLO_SectionData *sec_ptr, int group, int sfb);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_SECTION_DATA_H_ */
