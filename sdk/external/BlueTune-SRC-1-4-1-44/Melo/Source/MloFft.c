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

/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/
#include "MloDebug.h"
#include "MloFft.h"
#include "MloFloat.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Data
+---------------------------------------------------------------------*/

/* [0 ; Pi/2[ */
static const MLO_Float MLO_Fft_table_cos [MLO_FFT_TABLE_LEN_COS] =
{
   MLO_FLOAT_C (1),
   MLO_FLOAT_C (0.999981175),
   MLO_FLOAT_C (0.999924702),
   MLO_FLOAT_C (0.999830582),
   MLO_FLOAT_C (0.999698819),
   MLO_FLOAT_C (0.999529418),
   MLO_FLOAT_C (0.999322385),
   MLO_FLOAT_C (0.999077728),
   MLO_FLOAT_C (0.998795456),
   MLO_FLOAT_C (0.998475581),
   MLO_FLOAT_C (0.998118113),
   MLO_FLOAT_C (0.997723067),
   MLO_FLOAT_C (0.997290457),
   MLO_FLOAT_C (0.996820299),
   MLO_FLOAT_C (0.996312612),
   MLO_FLOAT_C (0.995767414),
   MLO_FLOAT_C (0.995184727),
   MLO_FLOAT_C (0.994564571),
   MLO_FLOAT_C (0.99390697),
   MLO_FLOAT_C (0.993211949),
   MLO_FLOAT_C (0.992479535),
   MLO_FLOAT_C (0.991709754),
   MLO_FLOAT_C (0.990902635),
   MLO_FLOAT_C (0.99005821),
   MLO_FLOAT_C (0.98917651),
   MLO_FLOAT_C (0.988257568),
   MLO_FLOAT_C (0.987301418),
   MLO_FLOAT_C (0.986308097),
   MLO_FLOAT_C (0.985277642),
   MLO_FLOAT_C (0.984210092),
   MLO_FLOAT_C (0.983105487),
   MLO_FLOAT_C (0.981963869),
   MLO_FLOAT_C (0.98078528),
   MLO_FLOAT_C (0.979569766),
   MLO_FLOAT_C (0.978317371),
   MLO_FLOAT_C (0.977028143),
   MLO_FLOAT_C (0.97570213),
   MLO_FLOAT_C (0.974339383),
   MLO_FLOAT_C (0.972939952),
   MLO_FLOAT_C (0.971503891),
   MLO_FLOAT_C (0.970031253),
   MLO_FLOAT_C (0.968522094),
   MLO_FLOAT_C (0.966976471),
   MLO_FLOAT_C (0.965394442),
   MLO_FLOAT_C (0.963776066),
   MLO_FLOAT_C (0.962121404),
   MLO_FLOAT_C (0.960430519),
   MLO_FLOAT_C (0.958703475),
   MLO_FLOAT_C (0.956940336),
   MLO_FLOAT_C (0.955141168),
   MLO_FLOAT_C (0.95330604),
   MLO_FLOAT_C (0.951435021),
   MLO_FLOAT_C (0.949528181),
   MLO_FLOAT_C (0.947585591),
   MLO_FLOAT_C (0.945607325),
   MLO_FLOAT_C (0.943593458),
   MLO_FLOAT_C (0.941544065),
   MLO_FLOAT_C (0.939459224),
   MLO_FLOAT_C (0.937339012),
   MLO_FLOAT_C (0.93518351),
   MLO_FLOAT_C (0.932992799),
   MLO_FLOAT_C (0.930766961),
   MLO_FLOAT_C (0.92850608),
   MLO_FLOAT_C (0.926210242),
   MLO_FLOAT_C (0.923879533),
   MLO_FLOAT_C (0.921514039),
   MLO_FLOAT_C (0.919113852),
   MLO_FLOAT_C (0.91667906),
   MLO_FLOAT_C (0.914209756),
   MLO_FLOAT_C (0.911706032),
   MLO_FLOAT_C (0.909167983),
   MLO_FLOAT_C (0.906595705),
   MLO_FLOAT_C (0.903989293),
   MLO_FLOAT_C (0.901348847),
   MLO_FLOAT_C (0.898674466),
   MLO_FLOAT_C (0.89596625),
   MLO_FLOAT_C (0.893224301),
   MLO_FLOAT_C (0.890448723),
   MLO_FLOAT_C (0.88763962),
   MLO_FLOAT_C (0.884797098),
   MLO_FLOAT_C (0.881921264),
   MLO_FLOAT_C (0.879012226),
   MLO_FLOAT_C (0.876070094),
   MLO_FLOAT_C (0.873094978),
   MLO_FLOAT_C (0.870086991),
   MLO_FLOAT_C (0.867046246),
   MLO_FLOAT_C (0.863972856),
   MLO_FLOAT_C (0.860866939),
   MLO_FLOAT_C (0.85772861),
   MLO_FLOAT_C (0.854557988),
   MLO_FLOAT_C (0.851355193),
   MLO_FLOAT_C (0.848120345),
   MLO_FLOAT_C (0.844853565),
   MLO_FLOAT_C (0.841554977),
   MLO_FLOAT_C (0.838224706),
   MLO_FLOAT_C (0.834862875),
   MLO_FLOAT_C (0.831469612),
   MLO_FLOAT_C (0.828045045),
   MLO_FLOAT_C (0.824589303),
   MLO_FLOAT_C (0.821102515),
   MLO_FLOAT_C (0.817584813),
   MLO_FLOAT_C (0.81403633),
   MLO_FLOAT_C (0.810457198),
   MLO_FLOAT_C (0.806847554),
   MLO_FLOAT_C (0.803207531),
   MLO_FLOAT_C (0.799537269),
   MLO_FLOAT_C (0.795836905),
   MLO_FLOAT_C (0.792106577),
   MLO_FLOAT_C (0.788346428),
   MLO_FLOAT_C (0.784556597),
   MLO_FLOAT_C (0.780737229),
   MLO_FLOAT_C (0.776888466),
   MLO_FLOAT_C (0.773010453),
   MLO_FLOAT_C (0.769103338),
   MLO_FLOAT_C (0.765167266),
   MLO_FLOAT_C (0.761202385),
   MLO_FLOAT_C (0.757208847),
   MLO_FLOAT_C (0.753186799),
   MLO_FLOAT_C (0.749136395),
   MLO_FLOAT_C (0.745057785),
   MLO_FLOAT_C (0.740951125),
   MLO_FLOAT_C (0.736816569),
   MLO_FLOAT_C (0.732654272),
   MLO_FLOAT_C (0.72846439),
   MLO_FLOAT_C (0.724247083),
   MLO_FLOAT_C (0.720002508),
   MLO_FLOAT_C (0.715730825),
   MLO_FLOAT_C (0.711432196),
   MLO_FLOAT_C (0.707106781),
   MLO_FLOAT_C (0.702754744),
   MLO_FLOAT_C (0.698376249),
   MLO_FLOAT_C (0.693971461),
   MLO_FLOAT_C (0.689540545),
   MLO_FLOAT_C (0.685083668),
   MLO_FLOAT_C (0.680600998),
   MLO_FLOAT_C (0.676092704),
   MLO_FLOAT_C (0.671558955),
   MLO_FLOAT_C (0.666999922),
   MLO_FLOAT_C (0.662415778),
   MLO_FLOAT_C (0.657806693),
   MLO_FLOAT_C (0.653172843),
   MLO_FLOAT_C (0.648514401),
   MLO_FLOAT_C (0.643831543),
   MLO_FLOAT_C (0.639124445),
   MLO_FLOAT_C (0.634393284),
   MLO_FLOAT_C (0.629638239),
   MLO_FLOAT_C (0.624859488),
   MLO_FLOAT_C (0.620057212),
   MLO_FLOAT_C (0.615231591),
   MLO_FLOAT_C (0.610382806),
   MLO_FLOAT_C (0.605511041),
   MLO_FLOAT_C (0.600616479),
   MLO_FLOAT_C (0.595699304),
   MLO_FLOAT_C (0.590759702),
   MLO_FLOAT_C (0.585797857),
   MLO_FLOAT_C (0.580813958),
   MLO_FLOAT_C (0.575808191),
   MLO_FLOAT_C (0.570780746),
   MLO_FLOAT_C (0.565731811),
   MLO_FLOAT_C (0.560661576),
   MLO_FLOAT_C (0.555570233),
   MLO_FLOAT_C (0.550457973),
   MLO_FLOAT_C (0.545324988),
   MLO_FLOAT_C (0.540171473),
   MLO_FLOAT_C (0.53499762),
   MLO_FLOAT_C (0.529803625),
   MLO_FLOAT_C (0.524589683),
   MLO_FLOAT_C (0.51935599),
   MLO_FLOAT_C (0.514102744),
   MLO_FLOAT_C (0.508830143),
   MLO_FLOAT_C (0.503538384),
   MLO_FLOAT_C (0.498227667),
   MLO_FLOAT_C (0.492898192),
   MLO_FLOAT_C (0.48755016),
   MLO_FLOAT_C (0.482183772),
   MLO_FLOAT_C (0.47679923),
   MLO_FLOAT_C (0.471396737),
   MLO_FLOAT_C (0.465976496),
   MLO_FLOAT_C (0.460538711),
   MLO_FLOAT_C (0.455083587),
   MLO_FLOAT_C (0.44961133),
   MLO_FLOAT_C (0.444122145),
   MLO_FLOAT_C (0.438616239),
   MLO_FLOAT_C (0.433093819),
   MLO_FLOAT_C (0.427555093),
   MLO_FLOAT_C (0.422000271),
   MLO_FLOAT_C (0.41642956),
   MLO_FLOAT_C (0.410843171),
   MLO_FLOAT_C (0.405241314),
   MLO_FLOAT_C (0.3996242),
   MLO_FLOAT_C (0.39399204),
   MLO_FLOAT_C (0.388345047),
   MLO_FLOAT_C (0.382683432),
   MLO_FLOAT_C (0.37700741),
   MLO_FLOAT_C (0.371317194),
   MLO_FLOAT_C (0.365612998),
   MLO_FLOAT_C (0.359895037),
   MLO_FLOAT_C (0.354163525),
   MLO_FLOAT_C (0.34841868),
   MLO_FLOAT_C (0.342660717),
   MLO_FLOAT_C (0.336889853),
   MLO_FLOAT_C (0.331106306),
   MLO_FLOAT_C (0.325310292),
   MLO_FLOAT_C (0.319502031),
   MLO_FLOAT_C (0.31368174),
   MLO_FLOAT_C (0.30784964),
   MLO_FLOAT_C (0.302005949),
   MLO_FLOAT_C (0.296150888),
   MLO_FLOAT_C (0.290284677),
   MLO_FLOAT_C (0.284407537),
   MLO_FLOAT_C (0.278519689),
   MLO_FLOAT_C (0.272621355),
   MLO_FLOAT_C (0.266712757),
   MLO_FLOAT_C (0.260794118),
   MLO_FLOAT_C (0.25486566),
   MLO_FLOAT_C (0.248927606),
   MLO_FLOAT_C (0.24298018),
   MLO_FLOAT_C (0.237023606),
   MLO_FLOAT_C (0.231058108),
   MLO_FLOAT_C (0.225083911),
   MLO_FLOAT_C (0.21910124),
   MLO_FLOAT_C (0.21311032),
   MLO_FLOAT_C (0.207111376),
   MLO_FLOAT_C (0.201104635),
   MLO_FLOAT_C (0.195090322),
   MLO_FLOAT_C (0.189068664),
   MLO_FLOAT_C (0.183039888),
   MLO_FLOAT_C (0.17700422),
   MLO_FLOAT_C (0.170961889),
   MLO_FLOAT_C (0.16491312),
   MLO_FLOAT_C (0.158858143),
   MLO_FLOAT_C (0.152797185),
   MLO_FLOAT_C (0.146730474),
   MLO_FLOAT_C (0.140658239),
   MLO_FLOAT_C (0.134580709),
   MLO_FLOAT_C (0.128498111),
   MLO_FLOAT_C (0.122410675),
   MLO_FLOAT_C (0.116318631),
   MLO_FLOAT_C (0.110222207),
   MLO_FLOAT_C (0.104121634),
   MLO_FLOAT_C (0.0980171403),
   MLO_FLOAT_C (0.0919089565),
   MLO_FLOAT_C (0.0857973123),
   MLO_FLOAT_C (0.079682438),
   MLO_FLOAT_C (0.0735645636),
   MLO_FLOAT_C (0.0674439196),
   MLO_FLOAT_C (0.0613207363),
   MLO_FLOAT_C (0.0551952443),
   MLO_FLOAT_C (0.0490676743),
   MLO_FLOAT_C (0.0429382569),
   MLO_FLOAT_C (0.0368072229),
   MLO_FLOAT_C (0.0306748032),
   MLO_FLOAT_C (0.0245412285),
   MLO_FLOAT_C (0.0184067299),
   MLO_FLOAT_C (0.0122715383),
   MLO_FLOAT_C (0.00613588465)
};



/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/



static void MLO_Fft_BuildBrTable (MLO_Int16 br_ptr [], int bit_depth);

static inline void   MLO_Fft_DoPass1 (MLO_Float dest_ptr [], const MLO_Float x_ptr [], int len, const MLO_Int16 br_ptr []);
static inline void   MLO_Fft_DoPass3 (MLO_Float dest_ptr [], const MLO_Float src_ptr [], int len);
static inline void   MLO_Fft_DoPassN (MLO_Float dest_ptr [], const MLO_Float src_ptr [], int len, int pass);



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



/*
==============================================================================
Name: MLO_Fft_Init
Description:
   Initialise the FFT object, fill tables. Call this function at least first
   before using this object.
Output parameters:
	- fft_ptr: FFT object
==============================================================================
*/

void  MLO_Fft_Init (MLO_Fft *fft_ptr)
{
   MLO_ASSERT(fft_ptr != NULL);

   MLO_Fft_BuildBrTable (&fft_ptr->table_br_l [0], 10);
   MLO_Fft_BuildBrTable (&fft_ptr->table_br_s [0], 7);
}



/*
==============================================================================
Name: MLO_Fft_Process
Description:
   Does a Discrete Fourier Transform on the input data, implemented as Fast
   Fourier Transform (radix 1 and 2 depending on the stage).

            len-1
             ___
   bin (k) = \   x [p] * exp (-j*2*pi*k*p)
             /__
            p = 0

   Input is real data.
   Output is complex conjugate data, arranged like this:

   y_ptr [0]       = real (bin 0) = DC
   y_ptr [1]       = real (bin 1)
   ...
   y_ptr [len/2-1] = real (bin len/2-1)
   y_ptr [len/2  ] = real (bin len/2) = Nyquist
   y_ptr [len/2+1] = -imag (bin 1)
   y_ptr [len/2+2] = -imag (bin 2)
   ...
   y_ptr [len - 1] = -imag (bin len/2-1)

   Can work in-place.

   Optimisations are based on compiler ability to precompute constants.
   Therefore numerical parameters of MLO_Fft_DoPass*() functions should be
   constants, in order to allow compiler to generate specific code for each
   function call, hence simplifying pointer handling and indexing.

Input parameters:
	- x_ptr: Array of source data (len elements).
	- len: Length of the FFT. Only 1024 or 128.
Output parameters:
	- y_ptr: Array of transformed data (len elements).
Input/output parameters:
	- fft_ptr: FFT processing object. Not modified here.
Throws: Nothing
==============================================================================
*/

void  MLO_Fft_Process (MLO_Fft *fft_ptr, MLO_Float y_ptr [], const MLO_Float x_ptr [], int len)
{
   MLO_Float *    buf_ptr;

   MLO_ASSERT(fft_ptr != NULL);
   MLO_ASSERT(x_ptr != NULL);
   MLO_ASSERT(y_ptr != NULL);
   MLO_ASSERT(   len == MLO_DEFS_FRAME_LEN_SHORT
/*************************************************************************************************************/
/*** Debug ***/
               || len == 8 /*** Debug ***/
/*************************************************************************************************************/
               || len == MLO_DEFS_FRAME_LEN_LONG);

   buf_ptr = &fft_ptr->buffer [0];
   MLO_ASSERT (x_ptr != buf_ptr);
   MLO_ASSERT (y_ptr != buf_ptr);

   if (len == MLO_DEFS_FRAME_LEN_SHORT)
   {
      MLO_Fft_DoPass1 (buf_ptr, x_ptr, MLO_DEFS_FRAME_LEN_SHORT, &fft_ptr->table_br_s [0]);
      MLO_Fft_DoPass3 (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_SHORT);
      MLO_Fft_DoPassN (buf_ptr, y_ptr, MLO_DEFS_FRAME_LEN_SHORT, 4);
      MLO_Fft_DoPassN (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_SHORT, 5);
      MLO_Fft_DoPassN (buf_ptr, y_ptr, MLO_DEFS_FRAME_LEN_SHORT, 6);
      MLO_Fft_DoPassN (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_SHORT, 7);
   }

/*************************************************************************************************************/
/*** Debug ***/
   else if (len == 8)
   {
      const MLO_Int16   br_ptr [8 >> 2] = { 0, 1 };
      MLO_Fft_DoPass1 (buf_ptr, x_ptr, 8, &br_ptr [0]);
      MLO_Fft_DoPass3 (y_ptr, buf_ptr, 8);
   }
/*************************************************************************************************************/

   else  /* MLO_DEFS_FRAME_LEN_LONG */
   {
      MLO_Fft_DoPass1 (y_ptr, x_ptr, MLO_DEFS_FRAME_LEN_LONG, &fft_ptr->table_br_l [0]);
      MLO_Fft_DoPass3 (buf_ptr, y_ptr, MLO_DEFS_FRAME_LEN_LONG);
      MLO_Fft_DoPassN (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_LONG, 4);
      MLO_Fft_DoPassN (buf_ptr, y_ptr, MLO_DEFS_FRAME_LEN_LONG, 5);
      MLO_Fft_DoPassN (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_LONG, 6);
      MLO_Fft_DoPassN (buf_ptr, y_ptr, MLO_DEFS_FRAME_LEN_LONG, 7);
      MLO_Fft_DoPassN (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_LONG, 8);
      MLO_Fft_DoPassN (buf_ptr, y_ptr, MLO_DEFS_FRAME_LEN_LONG, 9);
      MLO_Fft_DoPassN (y_ptr, buf_ptr, MLO_DEFS_FRAME_LEN_LONG, 10);
   }
}



static void MLO_Fft_BuildBrTable (MLO_Int16 br_ptr [], int bit_depth)
{
   const int      len = (1 << bit_depth) / MLO_FFT_BR_PACK;
   int            cnt;

   MLO_ASSERT (br_ptr != 0);
   MLO_ASSERT (   len == MLO_FFT_TABLE_LEN_BR_S
               || len == MLO_FFT_TABLE_LEN_BR_L);

   br_ptr [0] = 0;
	for (cnt = 1; cnt < len; ++cnt)
	{
		long           index = cnt * MLO_FFT_BR_PACK;
		long           br_index = 0;

		int				bit_cnt = bit_depth;
		do
		{
			br_index <<= 1;
			br_index += (index & 1);
			index >>= 1;

			-- bit_cnt;
		}
		while (bit_cnt > 0);

		br_ptr [cnt] = (MLO_Int16) br_index;
	}
}



/* First and second pass at once */
static inline void   MLO_Fft_DoPass1 (MLO_Float dest_ptr [], const MLO_Float x_ptr [], int len, const MLO_Int16 br_ptr [])
{
   const int      qlen = len >> 2;
   int            coef_index = 0;

   do
   {
      const int      ri_0 = br_ptr [coef_index >> 2];
      const int      ri_1 = ri_0 + 2 * qlen;	/* br_ptr [coef_index + 1] */
      const int      ri_2 = ri_0 + 1 * qlen;	/* br_ptr [coef_index + 2] */
      const int      ri_3 = ri_0 + 3 * qlen;	/* br_ptr [coef_index + 3] */

      MLO_Float *    df2 = dest_ptr + coef_index;
      MLO_Float      sf_0;
      MLO_Float      sf_2;
      df2 [1] = MLO_Float_Sub (x_ptr [ri_0], x_ptr [ri_1]);
      df2 [3] = MLO_Float_Sub (x_ptr [ri_2], x_ptr [ri_3]);

      sf_0 = MLO_Float_Add (x_ptr [ri_0], x_ptr [ri_1]);
      sf_2 = MLO_Float_Add (x_ptr [ri_2], x_ptr [ri_3]);

      df2 [0] = MLO_Float_Add (sf_0, sf_2);
      df2 [2] = MLO_Float_Sub (sf_0, sf_2);

      coef_index += 4;
   }
   while (coef_index < len);
}



/* Third pass */
static inline void   MLO_Fft_DoPass3 (MLO_Float dest_ptr [], const MLO_Float src_ptr [], int len)
{
   const MLO_Float   sqrt2_2 = MLO_FLOAT_C (MLO_DEFS_SQRT2 * 0.5);

   int            coef_index = 0;
   do
   {
      MLO_Float      v;

      dest_ptr [coef_index    ] = MLO_Float_Add (src_ptr [coef_index], src_ptr [coef_index + 4]);
      dest_ptr [coef_index + 4] = src_ptr [coef_index] - src_ptr [coef_index + 4];
      dest_ptr [coef_index + 2] = src_ptr [coef_index + 2];
      dest_ptr [coef_index + 6] = src_ptr [coef_index + 6];

      v = MLO_Float_Sub (src_ptr [coef_index + 5], src_ptr [coef_index + 7]);
      v = MLO_Float_Mul (v, sqrt2_2);
      dest_ptr [coef_index + 1] = MLO_Float_Add (src_ptr [coef_index + 1], v);
      dest_ptr [coef_index + 3] = MLO_Float_Sub (src_ptr [coef_index + 1], v);

      v = MLO_Float_Add (src_ptr [coef_index + 5], src_ptr [coef_index + 7]);
      v = MLO_Float_Mul (v, sqrt2_2);
      dest_ptr [coef_index + 5] = MLO_Float_Add (v, src_ptr [coef_index + 3]);
      dest_ptr [coef_index + 7] = MLO_Float_Sub (v, src_ptr [coef_index + 3]);

      coef_index += 8;
   }
   while (coef_index < len);
}



/* Other passes */
static inline void   MLO_Fft_DoPassN (MLO_Float dest_ptr [], const MLO_Float src_ptr [], int len, int pass)
{
   const int      dist = 1L << (pass - 2);
   const int      c1_r = 0;
   const int      c1_i = dist;
   const int      c2_r = dist * 2;
   const int      c2_i = dist * 3;
   const int      cend = dist * 4;
   const int      table_step = MLO_FFT_TABLE_LEN_COS >> (pass - 2);

   long				coef_index = 0;
   do
   {
      const MLO_Float * const sf = src_ptr + coef_index;
      MLO_Float * const df = dest_ptr + coef_index;
      int            i = 1;

      /* Extreme coefficients are always real */
      df [c1_r] = MLO_Float_Add (sf [c1_r], sf [c2_r]);
      df [c2_r] = sf [c1_r] - sf [c2_r];
      df [c1_i] = sf [c1_i];
      df [c2_i] = sf [c2_i];

      /* Others are conjugate complex numbers */
      do
      {
         const MLO_Float   c = MLO_Fft_table_cos [        i  * table_step];
         const MLO_Float   s = MLO_Fft_table_cos [(dist - i) * table_step];

         const MLO_Float   sf_r_i = sf [c1_r + i];
         const MLO_Float   sf_i_i = sf [c1_i + i];

         const MLO_Float   v1 = MLO_Float_Sub (
            MLO_Float_Mul (sf [c2_r + i], c),
            MLO_Float_Mul (sf [c2_i + i], s)
         );
         MLO_Float      v2;
         df [c1_r + i] = MLO_Float_Add (sf_r_i, v1);
         df [c2_r - i] = MLO_Float_Sub (sf_r_i, v1);

         v2 = MLO_Float_Add (
            MLO_Float_Mul (sf [c2_r + i], s),
            MLO_Float_Mul (sf [c2_i + i], c)
         );
         df [c2_r + i] = MLO_Float_Add (v2, sf_i_i);
         df [cend - i] = MLO_Float_Sub (v2, sf_i_i);

         ++ i;
      }
      while (i < dist);

      coef_index += cend;
   }
   while (coef_index < len);
}
