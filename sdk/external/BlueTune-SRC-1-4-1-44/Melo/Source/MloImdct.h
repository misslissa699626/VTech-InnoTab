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

#ifndef _MLO_IMDCT_H_
#define _MLO_IMDCT_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/

#include "MloFft.h"
#include "MloFloat.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_Imdct
{
   MLO_Float      buffer [2] [MLO_DEFS_FRAME_LEN_LONG];
   MLO_Fft        fft;
}  MLO_Imdct;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_Imdct_Init (MLO_Imdct *imdct_ptr);
void  MLO_Imdct_Restore (MLO_Imdct *imdct_ptr);

void  MLO_Imdct_Process (MLO_Imdct *imdct_ptr, MLO_Float x_ptr [], const MLO_Float f_ptr [], int len);

/* The following functions are public only for testing & debugging purpose. */
void  MLO_Imdct_ComputeDct4 (MLO_Imdct *imdct_ptr, MLO_Float dest_ptr [], const MLO_Float src_ptr [], int len);
void  MLO_Imdct_ComputeDct2 (MLO_Imdct *imdct_ptr, MLO_Float dest_ptr [], const MLO_Float src_ptr [], int len);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_IMDCT_H_ */
