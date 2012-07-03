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
Pool of Individual Channel Stream
*/

#ifndef _MLO_INDIV_CHN_POOL_H_
#define _MLO_INDIV_CHN_POOL_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloDefs.h"
#include "MloIndivChnStream.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



#define MLO_ERROR_NO_CHN_AVAILABLE  (MLO_ERROR_BASE_INDIV_CHN_POOL-0)



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_IndivChnPool
{
   int            nbr_alloc_chn;
   int            nbr_chn;
   MLO_IndivChnStream *
                  chn_ptr_arr [MLO_DEFS_MAX_CHN];
}  MLO_IndivChnPool;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_IndivChnPool_Create (MLO_IndivChnPool *pool_ptr, int nbr_chn);
void  MLO_IndivChnPool_Destroy (MLO_IndivChnPool *pool_ptr);

MLO_Result  MLO_IndivChnPool_Allocate (MLO_IndivChnPool *pool_ptr, int nbr_chn);

void  MLO_IndivChnPool_Clear (MLO_IndivChnPool *pool_ptr);
MLO_Result  MLO_IndivChnPool_AddChn (MLO_IndivChnPool *pool_ptr, int *index_ptr);
int   MLO_IndivChnPool_GetNbrChnFree (const MLO_IndivChnPool *pool_ptr);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_INDIV_CHN_POOL_H_ */
