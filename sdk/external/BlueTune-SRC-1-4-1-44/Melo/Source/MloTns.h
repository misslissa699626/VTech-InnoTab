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
Temporal Noise Shaping (TNS)

Ref:
4.4.2.7, Table 4.48
4.6.9
*/

#ifndef _MLO_TNS_H_
#define _MLO_TNS_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloBitStream.h"
#include "MloDefs.h"
#include "MloIcsInfo.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



/* 4.6.9.4, Table 4.137 */
enum {   MLO_TNS_MAX_ORDER = 12   };

/* 4.4.2.7, Table 4.48 */
enum {   MLO_TNS_MAX_COEF_RES = 4   };



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



/* Forward declaration */
struct MLO_IndivChnStream;



typedef struct MLO_Tns_Filter
{
   MLO_UInt8      length;
   MLO_UInt8      order;
   MLO_UInt8      direction;
   MLO_UInt8      compress;
   MLO_UInt8      coef [MLO_TNS_MAX_ORDER];
} MLO_Tns_Filter;



/* For a single window */
typedef struct MLO_Tns_Window
{
   int            n_filt;
   int            coef_res;
   MLO_Tns_Filter filter [4];
}  MLO_Tns_Window;



typedef struct MLO_Tns
{
   MLO_Tns_Window win_arr [MLO_DEFS_MAX_NBR_WINDOWS];   /* [win] */
}  MLO_Tns;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result MLO_Tns_Decode (MLO_Tns *tns_ptr, const MLO_IcsInfo *ics_info_ptr, MLO_BitStream *bit_ptr);
MLO_Result MLO_Tns_Process (struct MLO_IndivChnStream *ics_ptr);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_TNS_H_ */
