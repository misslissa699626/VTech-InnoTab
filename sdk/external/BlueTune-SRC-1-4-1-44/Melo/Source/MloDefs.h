/*****************************************************************
|
|    Melo - Common Definitions
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
/** @file
 * Header file for common definitions
 */

#ifndef _MLO_DEFS_H_
#define _MLO_DEFS_H_

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#ifndef __cplusplus
#ifndef NULL
#define NULL ((void*)0)
#endif
#endif /* __cplusplus */

/* Fixed frame length */
enum {   MLO_DEFS_FRAME_LEN_LONG    = 1024 };
enum {   MLO_DEFS_FRAME_LEN_SHORT   = MLO_DEFS_FRAME_LEN_LONG / 8 };

/* Maximum number of windows and window groups */
enum {   MLO_DEFS_MAX_NBR_WINDOWS   = 8   };
enum {   MLO_DEFS_MAX_NBR_WIN_GRP   = MLO_DEFS_MAX_NBR_WINDOWS   };

/* Maximum number of Scale Window Bands (related to MLO_IcsInfo data) */
enum {   MLO_DEFS_MAX_NUM_SWB       = 51 };

/* Maximum number of channels that can be supported by the decoder */
enum {   MLO_DEFS_MAX_CHN           = 32 };

/* Numeric constants */
#define  MLO_DEFS_PI                (3.1415926535897932384626433832795)
#define  MLO_DEFS_SQRT2             (1.41421356237309514547462185873883)



#endif /* _MLO_DEFS_H_ */
