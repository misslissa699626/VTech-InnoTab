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
Does filter bank processing and block switching

Ref:
4.6.11
*/

#ifndef _MLO_FILTER_BANK_H_
#define _MLO_FILTER_BANK_H_



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloImdct.h"
#include "MloIndivChnStream.h"
#include "MloDefs.h"
#include "MloTypes.h"



/*----------------------------------------------------------------------
|       Types
+---------------------------------------------------------------------*/



typedef struct MLO_FilterBank
{
   MLO_Imdct      imdct;
   MLO_Float      tmp_buf [MLO_DEFS_FRAME_LEN_LONG * 2]; /* For IMDCT result and overlapped data */
   MLO_Float      tmp_win [MLO_DEFS_FRAME_LEN_LONG * 2]; /* Windowed data */
}  MLO_FilterBank;



/*----------------------------------------------------------------------
|       Function prototypes
+---------------------------------------------------------------------*/



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



MLO_Result  MLO_FilterBank_Init (MLO_FilterBank *fb_ptr);
void  MLO_FilterBank_Restore (MLO_FilterBank *fb_ptr);

void  MLO_FilterBank_ConvertSpectralToTime (MLO_FilterBank *fb_ptr, MLO_IndivChnStream *ics_ptr);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _MLO_FILTER_BANK_H_ */
