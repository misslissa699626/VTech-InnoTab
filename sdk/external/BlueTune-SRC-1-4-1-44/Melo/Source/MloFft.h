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

#ifndef _MLO_FFT_H_
#define _MLO_FFT_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloDefs.h"
#include "MloFloat.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/



enum {   MLO_FFT_BR_PACK         = 4   };
enum {   MLO_FFT_TABLE_LEN_BR_L  = MLO_DEFS_FRAME_LEN_LONG  / MLO_FFT_BR_PACK  };
enum {   MLO_FFT_TABLE_LEN_BR_S  = MLO_DEFS_FRAME_LEN_SHORT / MLO_FFT_BR_PACK  };

/* For tables */
enum {   MLO_FFT_TABLE_LEN_COS   = MLO_DEFS_FRAME_LEN_LONG >> 2   };



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_Fft
{
   MLO_Float      buffer [MLO_DEFS_FRAME_LEN_LONG];
   MLO_Int16      table_br_l [MLO_FFT_TABLE_LEN_BR_L];   /* Bit-reverse table (only multiples of MLO_FFT_BR_PACK) */
   MLO_Int16      table_br_s [MLO_FFT_TABLE_LEN_BR_S];
}  MLO_Fft;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



void  MLO_Fft_Init (MLO_Fft *fft_ptr);
void  MLO_Fft_Process (MLO_Fft *fft_ptr, MLO_Float x_ptr [], const MLO_Float f_ptr [], int len);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_FFT_H_ */
