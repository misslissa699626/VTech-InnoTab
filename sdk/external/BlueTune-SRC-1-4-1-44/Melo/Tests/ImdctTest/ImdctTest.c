/*****************************************************************
|
|      File: ImdctTest.c
|
|      Melo - Test of the IMDCT functions
|
|      (c) 2005 Ohm Force
|      Author: Laurent de Soras (laurent.de.soras@ohmforce.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "MloDebug.h"
#include "MloDefs.h"
#include "MloFft.h"
#include "MloImdct.h"
#include "MloResults.h"
#include "MloUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>



/*----------------------------------------------------------------------
|       Constants
+---------------------------------------------------------------------*/


enum {   IMDCT_TEST_NBR_TESTS = 100 };


/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/



static void ImdctTest_ComputeRefImdct (float dest_ptr [], const float src_ptr [], int len);
static void ImdctTest_ComputeRefDct2 (float dest_ptr [], const float src_ptr [], int len);
static void ImdctTest_ComputeRefDct4 (float dest_ptr [], const float src_ptr [], int len);
static void ImdctTest_ComputeRefDctCommon (float dest_ptr [], const float src_ptr [], int len, double a);
static void ImdctTest_ComputeRefFft (float dest_ptr [], const float src_ptr [], int len);

static int  ImdctTest_TestFft (int len);
static void ImdctTest_TestFftSpeed (int len);
static int  ImdctTest_TestImdct (int len);
static void ImdctTest_TestImdctSpeed (int len);
static int  ImdctTest_TestDct2 (int len);
static int  ImdctTest_TestDct4 (int len);
static void ImdctTest_GenerateRandVect (float dest_ptr [], int len);
static int  ImdctTest_Compare (const float set_1_ptr [], const float set_2_ptr [], int len);

static void ImdctTest_PrintSpeed (const clock_t s [3], long nbr_tests, long nbr_tests_ref, long len, const char *txt_0);



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



int   main (int argc, char** argv)
{
   int            ret_val = 0;

   ImdctTest_TestFftSpeed (MLO_DEFS_FRAME_LEN_SHORT);
   ImdctTest_TestFftSpeed (MLO_DEFS_FRAME_LEN_LONG);
   ImdctTest_TestImdctSpeed (MLO_DEFS_FRAME_LEN_SHORT);
   ImdctTest_TestImdctSpeed (MLO_DEFS_FRAME_LEN_LONG);

   /* First, tests FFT used in IMDCT calculation */
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestFft (MLO_DEFS_FRAME_LEN_SHORT);
   }
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestFft (MLO_DEFS_FRAME_LEN_LONG);
   }

   /* Then, tests the intermediate DCT calculations */
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestDct2 (MLO_DEFS_FRAME_LEN_SHORT);
   }
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestDct2 (MLO_DEFS_FRAME_LEN_LONG);
   }
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestDct4 (MLO_DEFS_FRAME_LEN_SHORT);
   }
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestDct4 (MLO_DEFS_FRAME_LEN_LONG);
   }

   /* Finally, tests IMDCT */
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestImdct (MLO_DEFS_FRAME_LEN_SHORT * 2);
   }
   if (ret_val == 0)
   {
      ret_val = ImdctTest_TestImdct (MLO_DEFS_FRAME_LEN_LONG * 2);
   }

   return (ret_val);
}



/*
==============================================================================
Name: ImdctTest_ComputeRefImdct
Description:
   Ref: 4.6.11.3.1
Input parameters:
	- src_ptr: IMDCT input, length len/2
	- len: Length of the IMDCT, > 0
Output parameters:
	- dest_ptr: IMDCT output, length len
==============================================================================
*/

static void ImdctTest_ComputeRefImdct (float dest_ptr [], const float src_ptr [], int len)
{
   int            half_len;
   int            n;
   float          n_0;
   float          scale;

   MLO_ASSERT (dest_ptr != 0);
   MLO_ASSERT (src_ptr != 0);
   MLO_ASSERT (len > 0);

   half_len = len / 2;
   n_0 = (len * 0.5f + 1) * 0.5f;
   scale = 2.0f / len;

   for (n = 0; n < len; ++n)
   {
      int            k;
      double         mult_phase = (2 * MLO_DEFS_PI) * (n + n_0) / len;
      double         sum = 0;

      for (k = 0; k < half_len; ++k)
      {
         double         phase = (k + 0.5) * mult_phase;
         sum += src_ptr [k] * cos (phase);
      }

      dest_ptr [n] = (float) (sum * scale);
   }
}



static void ImdctTest_ComputeRefDct2 (float dest_ptr [], const float src_ptr [], int len)
{
   ImdctTest_ComputeRefDctCommon (dest_ptr, src_ptr, len, 0.0);
}



static void ImdctTest_ComputeRefDct4 (float dest_ptr [], const float src_ptr [], int len)
{
   ImdctTest_ComputeRefDctCommon (dest_ptr, src_ptr, len, 0.5);
}



static void ImdctTest_ComputeRefDctCommon (float dest_ptr [], const float src_ptr [], int len, double a)
{
   int            n;

   MLO_ASSERT (dest_ptr != 0);
   MLO_ASSERT (src_ptr != 0);
   MLO_ASSERT (len > 0);
   MLO_ASSERT (a == 0 || a == 0.5);

   for (n = 0; n < len; ++n)
   {
      int            k;
      double         mult_phase = MLO_DEFS_PI * (n + a) / len;
      double         sum = 0;

      for (k = 0; k < len; ++k)
      {
         double         phase = (k + 0.5) * mult_phase;
         sum += src_ptr [k] * cos (phase);
      }

      dest_ptr [n] = (float) sum;
   }
}



static void ImdctTest_ComputeRefFft (float dest_ptr [], const float src_ptr [], int len)
{
	int            nbr_bins;
   int            bin;
   int            pos;
	double			dc = 0;
	double			ny = 0;

   MLO_ASSERT (dest_ptr != 0);
   MLO_ASSERT (src_ptr != 0);
   MLO_ASSERT (len > 2);

	nbr_bins = len >> 1;

	// DC and Nyquist
	for (pos = 0; pos < len; pos += 2)
	{
		double         even = src_ptr [pos    ];
		double         odd  = src_ptr [pos + 1];
		dc += even + odd;
		ny += even - odd;
	}
	dest_ptr [0       ] = (float) dc;
	dest_ptr [nbr_bins] = (float) ny;

	// Regular bins
	for (bin = 1; bin < nbr_bins; ++ bin)
	{
		double         sum_r = 0;
		double         sum_i = 0;

		double	      m = bin * (double) (2 * MLO_DEFS_PI) / len;

		for (pos = 0; pos < len; ++pos)
		{
			double         phase = pos * m;
			double         e_r = cos (phase);
			double         e_i = sin (phase);

			sum_r += src_ptr [pos] * e_r;
			sum_i += src_ptr [pos] * e_i;
		}

		dest_ptr [           bin] = (float) sum_r;
		dest_ptr [nbr_bins + bin] = (float) sum_i;
	}
}



static int  ImdctTest_TestFft (int len)
{
   int            ret_val = 0;
   int            t;

   MLO_Fft        fft;
   float          src [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_ref [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_test [MLO_DEFS_FRAME_LEN_LONG];

   printf ("testing FFT, length = %4d samples.\n", len);

   MLO_Fft_Init (&fft);

   for (t = 0; t < IMDCT_TEST_NBR_TESTS && ret_val == 0; ++t)
   {
      ImdctTest_GenerateRandVect (&src [0], len);

      ImdctTest_ComputeRefFft (&dest_ref [0], &src [0], len);
      MLO_Fft_Process (&fft, &dest_test [0], &src [0], len);

      ret_val = ImdctTest_Compare (&dest_ref [0], &dest_test [0], len);
   }

   return (ret_val);
}



static void ImdctTest_TestFftSpeed (int len)
{
   int            k;
   MLO_Fft        fft;
   float          src [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_test [MLO_DEFS_FRAME_LEN_LONG];
   int            nbr_tests = 10000;
   int            nbr_tests_ref = 10;
   clock_t        s [3];

   MLO_Fft_Init (&fft);
   ImdctTest_GenerateRandVect (&src [0], len);

   s [0] = clock ();
   for (k = 0; k < nbr_tests; ++k)
   {
      MLO_Fft_Process (&fft, &dest_test [0], &src [0], len);
   }
   s [1] = clock ();
   for (k = 0; k < nbr_tests_ref; ++k)
   {
      ImdctTest_ComputeRefFft (&dest_test [0], &src [0], len);
   }
   s [2] = clock ();

   ImdctTest_PrintSpeed (s, nbr_tests, nbr_tests_ref, len, "FFT");
}



static int  ImdctTest_TestImdct (int len)
{
   int            ret_val = 0;
   int            t;

   MLO_Imdct      imdct;
   float          src [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_ref [MLO_DEFS_FRAME_LEN_LONG * 2];
   float          dest_test [MLO_DEFS_FRAME_LEN_LONG * 2];

   printf ("testing IMDCT, length = %4d samples.\n", len);

   ret_val = MLO_SUCCEEDED (MLO_Imdct_Init (&imdct)) ? 0 : -1;

   for (t = 0; t < IMDCT_TEST_NBR_TESTS && ret_val == 0; ++t)
   {
      ImdctTest_GenerateRandVect (&src [0], len / 2);

      ImdctTest_ComputeRefImdct (&dest_ref [0], &src [0], len);
      MLO_Imdct_Process (&imdct, &dest_test [0], &src [0], len);

      ret_val = ImdctTest_Compare (&dest_ref [0], &dest_test [0], len);
   }

   return (ret_val);
}



static void ImdctTest_TestImdctSpeed (int len)
{
   int            k;
   MLO_Imdct      imdct;
   float          src [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_test [MLO_DEFS_FRAME_LEN_LONG * 2];
   int            nbr_tests = 10000;
   int            nbr_tests_ref = 10;
   clock_t        s [3];

   MLO_Imdct_Init (&imdct);
   ImdctTest_GenerateRandVect (&src [0], len);

   s [0] = clock ();
   for (k = 0; k < nbr_tests; ++k)
   {
      MLO_Imdct_Process (&imdct, &dest_test [0], &src [0], len * 2);
   }
   s [1] = clock ();
   for (k = 0; k < nbr_tests_ref; ++k)
   {
      ImdctTest_ComputeRefImdct ( &dest_test [0], &src [0], len * 2);
   }
   s [2] = clock ();

   ImdctTest_PrintSpeed (s, nbr_tests, nbr_tests_ref, len, "IMDCT");

   MLO_Imdct_Restore (&imdct);
}



static int  ImdctTest_TestDct2 (int len)
{
   int            ret_val = 0;
   int            t;

   MLO_Imdct      imdct;
   float          src [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_ref [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_test [MLO_DEFS_FRAME_LEN_LONG];

   printf ("testing DCT-II, length = %4d samples.\n", len);

   ret_val = MLO_SUCCEEDED (MLO_Imdct_Init (&imdct)) ? 0 : -1;

   for (t = 0; t < IMDCT_TEST_NBR_TESTS && ret_val == 0; ++t)
   {
      ImdctTest_GenerateRandVect (&src [0], len);

      ImdctTest_ComputeRefDct2 (&dest_ref [0], &src [0], len);
      MLO_Imdct_ComputeDct2 (&imdct, &dest_test [0], &src [0], len);

      ret_val = ImdctTest_Compare (&dest_ref [0], &dest_test [0], len);
   }

   return (ret_val);
}



static int  ImdctTest_TestDct4 (int len)
{
   int            ret_val = 0;
   int            t;

   MLO_Imdct      imdct;
   float          src [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_ref [MLO_DEFS_FRAME_LEN_LONG];
   float          dest_test [MLO_DEFS_FRAME_LEN_LONG];

   printf ("testing DCT-IV, length = %4d samples.\n", len);

   ret_val = MLO_SUCCEEDED (MLO_Imdct_Init (&imdct)) ? 0 : -1;

   for (t = 0; t < IMDCT_TEST_NBR_TESTS && ret_val == 0; ++t)
   {
      ImdctTest_GenerateRandVect (&src [0], len);

      ImdctTest_ComputeRefDct4 (&dest_ref [0], &src [0], len);
      MLO_Imdct_ComputeDct4 (&imdct, &dest_test [0], &src [0], len);

      ret_val = ImdctTest_Compare (&dest_ref [0], &dest_test [0], len);
   }

   return (ret_val);
}



static void ImdctTest_GenerateRandVect (float dest_ptr [], int len)
{
   int            pos;

   for (pos = 0; pos < len; ++pos)
   {
      dest_ptr [pos] = (float) rand () * 2 / RAND_MAX - 1;
/*************************************************************************************************************/
/*** Debug ***/
//      dest_ptr [pos] = (float) (((pos + 1) / 2) * (1 - (pos & 1) * 2) + (pos & 1) * len);
/*************************************************************************************************************/
   }
}



static int  ImdctTest_Compare (const float set_1_ptr [], const float set_2_ptr [], int len)
{
   int            ret_val = 0;
   int            pos = 0;

   do
   {
      float       x_1 = set_1_ptr [pos];
      float       x_2 = set_2_ptr [pos];
      float       dif = x_2 - x_1;
      float       err_abs = MLO_ABS (dif);
      float       mag = (float) sqrt (MLO_ABS (x_1 * x_2));
      if (x_1 == 0 || x_2 == 0)
      {
         mag = 1;
      }

      /* if (err_abs > 1e-1 * mag) */
      if (err_abs > 1e-3)
      {
         printf ("Comparison failed at %4d: %12.8f, should be %12.8f\n", pos, x_2, x_1);
         ret_val = -1;
      }
/*************************************************************************************************************/
/*** Debug ***/
      else
      {
//         printf ("                           %12.8f            %12.8f\n", set_1_ptr [pos], set_2_ptr [pos]);
      }
/*************************************************************************************************************/

      ++ pos;
   }
   while (pos < len);

   return (ret_val);
}



static void ImdctTest_PrintSpeed (const clock_t s [3], long nbr_tests, long nbr_tests_ref, long len, const char *txt_0)
{
   const double   t_mult = 1e9;
   const double   t     =   (double) (s [1] - s [0])
                          * t_mult
                          / (double) CLOCKS_PER_SEC
                          / nbr_tests
                          / len;
   const double   t_ref =   (double) (s [2] - s [1])
                          * t_mult
                          / (double) CLOCKS_PER_SEC
                          / nbr_tests_ref
                          / len;

   printf (
      "%s speed (length = %4d): %7.3f ns/sample (x %6.1f)\n",
      txt_0,
      len,
      t,
      t_ref / t
   );
}
