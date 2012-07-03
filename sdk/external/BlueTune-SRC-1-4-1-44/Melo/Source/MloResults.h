/*****************************************************************
|
|    MLO - Result & Error Constants
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
 * Results and Error codes
 */

#ifndef _MLO_ERRORS_H_
#define _MLO_ERRORS_H_

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|    error codes
+---------------------------------------------------------------------*/
#if !defined(MLO_ERROR_BASE)
#define MLO_ERROR_BASE (-22000)
#endif

/** Result code indicating that a call was successful */
#define MLO_SUCCESS      0

/** Error: an unspecififed error has occurred. */
#define MLO_FAILURE      (-1)

#define MLO_FAILED(result)       ((result) != MLO_SUCCESS)
#define MLO_SUCCEEDED(result)    ((result) == MLO_SUCCESS)

/* Generic Errors */
#define MLO_ERROR_BASE_GENERIC            (MLO_ERROR_BASE-0)
#define MLO_ERROR_INVALID_PARAMETERS      (MLO_ERROR_BASE_GENERIC-0)
#define MLO_ERROR_OUT_OF_MEMORY           (MLO_ERROR_BASE_GENERIC-1)
#define MLO_ERROR_OUT_OF_RANGE            (MLO_ERROR_BASE_GENERIC-2)
#define MLO_ERROR_INVALID_DATA            (MLO_ERROR_BASE_GENERIC-3)

/* Decoder errors */
#define MLO_ERROR_BASE_DECODER            (MLO_ERROR_BASE - 100)

/* BitStream errors */
#define MLO_ERROR_BASE_BITSTREAM          (MLO_ERROR_BASE - 200)

/* IcsInfo errors */
#define MLO_ERROR_BASE_ICS_INFO           (MLO_ERROR_BASE - 300)

/* IndivChnPool errors */
#define MLO_ERROR_BASE_INDIV_CHN_POOL     (MLO_ERROR_BASE - 400)

/* IndivChnStream errors */
#define MLO_ERROR_BASE_INDIV_CHN_STREAM   (MLO_ERROR_BASE - 500)

/* Huffman errors */
#define MLO_ERROR_BASE_HUFFMAN            (MLO_ERROR_BASE - 600)

/* ElementFil errors */
#define MLO_ERROR_BASE_ELEMENT_FIL        (MLO_ERROR_BASE - 700)

/* ElementFil errors */
#define MLO_ERROR_BASE_SCALE_FACTOR       (MLO_ERROR_BASE - 800)

/* SyntacticElements errors */
#define MLO_ERROR_BASE_SYNTACTIC_ELEMENTS (MLO_ERROR_BASE - 900)

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/
#if defined(MLO_DEBUG)
#define MLO_CHECK_ARGS(_x) MLO_ASSERT(_x)
#else
#define MLO_CHECK_ARGS(_x) if (!(_x)) return MLO_ERROR_INVALID_PARAMETERS
#endif

#define MLO_CHECK_DATA(_x) if (!(_x)) return MLO_ERROR_INVALID_DATA

#endif /* _MLO_ERRORS_H_ */
