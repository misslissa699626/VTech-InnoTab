#include "MloFloat.h"
#include "MloResults.h"
#include "MloTypes.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>



#if ! defined (MLO_CONFIG_FIXED)
#error Test for fixed point configuration only
#endif



static double   MLO_FloatTest_ToDouble (MLO_Float x)
{
   return ((double)x / (double)MLO_Float_one);
}



/*
static MLO_Float   MLO_FloatTest_ToMloFloat (double x)
{
   return ((MLO_Float) (x * (double) MLO_Float_one));
}
*/



static MLO_Float  MLO_FloatTest_GenerateRand (int min_l2, int max_l2, MLO_Boolean signed_flag)
{
   MLO_Float      n = 1;
   const int      p2 = min_l2 + (rand () % (max_l2 - min_l2)) + MLO_FLOAT_FRAC;
   int            i;

   MLO_ASSERT (min_l2 >= -MLO_FLOAT_FRAC);
   MLO_ASSERT (max_l2 > min_l2);

   for (i = 1; i < p2; ++i)
   {
      n <<= 1;
      n += (rand () >> 12) & 1;
   }

   if (signed_flag && ((rand () >> 12) & 1) != 0)
   {
      n = MLO_Float_Neg (n);
   }

   return (n);
}



static MLO_Result MLO_FloatTest_TestAndPrintAccuracy (const char *name_0, double t, double r, double a, double b, MLO_Boolean print_flag, int toler_bits)
{
   const double err = fabs (r - t);
   const double   err_rel = (double) (err * 2 / fabs (r + t + 1e-100));
   const double   err_rel_l2 = log (err_rel + 1e-30) / log (2);
   const double   min_err = MLO_FloatTest_ToDouble ((MLO_Float) 1);
   const double   min_err_rel = (double) (min_err / fabs (r + 1e-100));
   const double   min_err_rel_l2 = log (min_err_rel + 1e-30) / log (2);
   MLO_Result  result =
      (   err_rel_l2 <= -8
       || err_rel_l2 <= min_err_rel_l2 + 1 + toler_bits
       || fabs (r) >= MLO_FloatTest_ToDouble (MLO_Float_max))
      ? MLO_SUCCESS
      : MLO_FAILURE;

   if (print_flag || MLO_FAILED (result))
   {
      printf (
         "%s: # bits: %4.1f, floating: %24.12f, fixed: %24.12f\n",
         name_0,
         -err_rel_l2,
         r,
         t
      );
      if (MLO_FAILED (result))
      {
         printf ("a = %.12f, b = %.12f\n", a, b);
      }
   }

   return (result);
}

static unsigned long long
MLO_Float_Mul3232To64U(unsigned long a, unsigned long b)
{
    return ((unsigned long long)a)*((unsigned long long)b);
}

typedef unsigned long long MLO_FloatU;
static MLO_Float MLO_Float_Mul2 (MLO_Float a, MLO_Float b)
{
   int change_sign = 0;
//   if (a < 0) {
//        a = -a;
//        change_sign = 1;
//   } 
//   if (b < 0) {
//        b = -b;
//        change_sign = 1-change_sign;
//   }

   {
   const MLO_UInt32   a_f = ((MLO_UInt32) a) & ((MLO_UInt32) MLO_Float_frac_mask);
   const MLO_UInt32   a_i = (MLO_UInt32) (a >> MLO_FLOAT_FRAC);
   const MLO_UInt32   b_f = ((MLO_UInt32) b) & ((MLO_UInt32) MLO_Float_frac_mask);
   const MLO_UInt32   b_i = (MLO_UInt32) (b >> MLO_FLOAT_FRAC);

   MLO_FloatU   ab_ff   = MLO_Float_Mul3232To64U (a_f, b_f);
   MLO_FloatU   ab_if_1 = MLO_Float_Mul3232To64U (a_i, b_f);
   MLO_FloatU   ab_if_2 = MLO_Float_Mul3232To64U (a_f, b_i);
   MLO_FloatU   ab_if   = ab_if_1+ab_if_2;
   MLO_FloatU   ab_ii   = (a_i*b_i)&0xFFFFFFFF;
   MLO_FloatU   result;
   
   ab_ff >>= MLO_FLOAT_FRAC;
   ab_ii <<= MLO_FLOAT_FRAC;
   
   result = (ab_ii + ab_if + ab_ff);
   return change_sign?-result:result;
   }
}

static MLO_Result MLO_FloatTest_TestMul2 (MLO_Float a, MLO_Float b, MLO_Boolean print_flag)
{
   const double a_f = MLO_FloatTest_ToDouble (a);
   const double b_f = MLO_FloatTest_ToDouble (b);

   const MLO_Float   ab = MLO_Float_Mul2 (a, b);
   const double ab_fr = a_f * b_f;

   const double ab_ft = MLO_FloatTest_ToDouble (ab);

   return (MLO_FloatTest_TestAndPrintAccuracy ("Mul ", ab_ft, ab_fr, a_f, b_f, print_flag, 0));
}

static MLO_Result MLO_FloatTest_TestMul (MLO_Float a, MLO_Float b, MLO_Boolean print_flag)
{
   const double a_f = MLO_FloatTest_ToDouble (a);
   const double b_f = MLO_FloatTest_ToDouble (b);

   const MLO_Float   ab = MLO_Float_Mul (a, b);
   const double ab_fr = a_f * b_f;

   const double ab_ft = MLO_FloatTest_ToDouble (ab);

   return (MLO_FloatTest_TestAndPrintAccuracy ("Mul ", ab_ft, ab_fr, a_f, b_f, print_flag, 0));
}



static MLO_Result MLO_FloatTest_TestDiv (MLO_Float a, MLO_Float b, MLO_Boolean print_flag)
{
   const double a_f = MLO_FloatTest_ToDouble (a);
   const double b_f = MLO_FloatTest_ToDouble (b);

   const MLO_Float   ab = MLO_Float_Div (a, b);
   const double ab_fr = a_f / b_f;

   const double ab_ft = MLO_FloatTest_ToDouble (ab);

   return (MLO_FloatTest_TestAndPrintAccuracy ("Div ", ab_ft, ab_fr, a_f, b_f, print_flag, 0));
}



static MLO_Result MLO_FloatTest_TestSqrt (MLO_Float a, MLO_Boolean print_flag)
{
   const double a_f = MLO_FloatTest_ToDouble (a);

   const MLO_Float   as = MLO_Float_Sqrt (a);
   const double as_fr = sqrt (a_f);

   const double as_ft = MLO_FloatTest_ToDouble (as);

   return (MLO_FloatTest_TestAndPrintAccuracy ("Sqrt", as_ft, as_fr, a_f, 0, print_flag, MLO_FLOAT_FRAC/2));
}



static MLO_Result MLO_FloatTest_TestScaleP2 (MLO_Float a, int b, MLO_Boolean print_flag)
{
   const double a_f = MLO_FloatTest_ToDouble (a);

   const MLO_Float   ab = MLO_Float_ScaleP2 (a, b);
   const double ab_fr = a_f * pow (2.0, b);

   const double ab_ft = MLO_FloatTest_ToDouble (ab);

   return (MLO_FloatTest_TestAndPrintAccuracy ("ScP2", ab_ft, ab_fr, a_f, b, print_flag, 0));
}



static MLO_Result MLO_FloatTest_TestMulAll ()
{
   MLO_Result     result = MLO_SUCCESS;
   int            k;

   for (k = 0; k < 100000 && MLO_SUCCEEDED (result); ++k)
   {
      const MLO_Float   a = MLO_FloatTest_GenerateRand (-MLO_FLOAT_FRAC, 30, MLO_TRUE);
      const MLO_Float   b = MLO_FloatTest_GenerateRand (-MLO_FLOAT_FRAC, 30, MLO_TRUE);
      result = MLO_FloatTest_TestMul (a, b, MLO_FALSE);
   }

   if (MLO_FAILED (result))
   {
      printf ("*** Multiplication test failed.\n");
   }

   return (result);
}



static MLO_Result MLO_FloatTest_TestDivAll ()
{
   MLO_Result     result = MLO_SUCCESS;
   int            k;

   for (k = 0; k < 100000 && MLO_SUCCEEDED (result); ++k)
   {
      const MLO_Float   a = MLO_FloatTest_GenerateRand (-MLO_FLOAT_FRAC, 30, MLO_FALSE);
      const MLO_Float   b = MLO_FloatTest_GenerateRand (-MLO_FLOAT_FRAC, 20, MLO_FALSE);
      result = MLO_FloatTest_TestDiv (a, b, MLO_FALSE);
   }

   if (MLO_FAILED (result))
   {
      printf ("*** Division test failed.\n");
   }

   return (result);
}



static MLO_Result MLO_FloatTest_TestSqrtAll ()
{
   MLO_Result     result = MLO_SUCCESS;
   int            k;

   for (k = 0; k < 100000 && MLO_SUCCEEDED (result); ++k)
   {
      const MLO_Float   a = MLO_FloatTest_GenerateRand (-MLO_FLOAT_FRAC, 30, MLO_FALSE);
      result = MLO_FloatTest_TestSqrt (a, MLO_FALSE);
   }

   if (MLO_FAILED (result))
   {
      printf ("*** Square root test failed.\n");
   }

   return (result);
}



static MLO_Result MLO_FloatTest_TestScaleP2All ()
{
   MLO_Result     result = MLO_SUCCESS;
   int            k;

   for (k = 0; k < 100000 && MLO_SUCCEEDED (result); ++k)
   {
      const MLO_Float   a = MLO_FloatTest_GenerateRand (-MLO_FLOAT_FRAC, 30, MLO_TRUE);
      const int      b = (rand() % 61) - 30;
      result = MLO_FloatTest_TestScaleP2 (a, b, MLO_FALSE);
   }

   if (MLO_FAILED (result))
   {
      printf ("*** ScaleP2 test failed.\n");
   }

   return (result);
}

static void MLO_Print_Fix(MLO_Float f) {
#if defined(MLO_CONFIG_FIXED)
    unsigned int hi = (f>>32)&0xFFFFFFFF;
    unsigned int lo = (f    )&0xFFFFFFFF;
    printf("%08x%08x\n", hi, lo);
#endif
}

int main (int argc, char *argv [])
{
   MLO_Result     result = MLO_SUCCESS;

    MLO_Print_Fix(MLO_Float_one);
    MLO_Print_Fix(MLO_Float_frac_mask);
    MLO_Print_Fix(MLO_Float_max);

    MLO_FloatTest_TestMul2 (MLO_FLOAT_C (1.00001), MLO_FLOAT_C (0.99999), MLO_TRUE);
    MLO_FloatTest_TestMul2 (MLO_FLOAT_C (-2147483647.0), MLO_FLOAT_C (-0.000001), MLO_TRUE);
    MLO_FloatTest_TestMul2 (MLO_FLOAT_C (-0.000000005821), MLO_FLOAT_C(-0.562021692283), MLO_TRUE);
    
   if (MLO_SUCCEEDED (result))
   {
      MLO_FloatTest_TestMul (MLO_FLOAT_C (16001.2395), MLO_FLOAT_C (694.65), MLO_TRUE);
      MLO_FloatTest_TestMul (MLO_FLOAT_C (1), MLO_FLOAT_C (1.0 / 65536.0 / 256.0), MLO_TRUE);
      MLO_FloatTest_TestMul (MLO_FLOAT_C (1), MLO_FLOAT_C (65536.0 * 256.0), MLO_TRUE);
      MLO_FloatTest_TestMul (MLO_FLOAT_C (1.00001), MLO_FLOAT_C (1.00001), MLO_TRUE);
      MLO_FloatTest_TestMul2 (MLO_FLOAT_C (1.00001), MLO_FLOAT_C (0.99999), MLO_TRUE);
      MLO_FloatTest_TestMul (MLO_FLOAT_C (-30.0), MLO_FLOAT_C (30.0), MLO_TRUE);
      MLO_FloatTest_TestMul (MLO_FLOAT_C (-2147483647.0), MLO_FLOAT_C (-0.000001), MLO_TRUE);
      printf ("\n");

      result = MLO_FloatTest_TestMulAll ();
      printf ("\n");
   }

   if (MLO_SUCCEEDED (result))
   {
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (16001.2395L), MLO_FLOAT_C (694.65L), MLO_TRUE);
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (1), MLO_FLOAT_C (1.0 / 65536.0 / 256.0), MLO_TRUE);
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (1), MLO_FLOAT_C (65536.0 * 256.0), MLO_TRUE);
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (1.00001), MLO_FLOAT_C (1.00001), MLO_TRUE);
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (1.00001), MLO_FLOAT_C (0.99999), MLO_TRUE);
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (0.99999), MLO_FLOAT_C (1.00001), MLO_TRUE);
      MLO_FloatTest_TestDiv (MLO_FLOAT_C (2000.0*1000.0*1000.0), MLO_FLOAT_C (10.0*1000.0*1000.0+1), MLO_TRUE);
      printf ("\n");

      result = MLO_FloatTest_TestDivAll ();
      printf ("\n");
   }

   if (MLO_SUCCEEDED (result))
   {
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (0.0), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (1.0), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (2.0), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (4.0), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (16.0), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (299352.39254107), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (0.0001535), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (255.99999), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (256.0L), MLO_TRUE);
      MLO_FloatTest_TestSqrt (MLO_FLOAT_C (256.00001), MLO_TRUE);
      printf ("\n");

      result = MLO_FloatTest_TestSqrtAll ();
      printf ("\n");
   }

   if (MLO_SUCCEEDED (result))
   {
      MLO_FloatTest_TestScaleP2 (MLO_FLOAT_C (123.625), 0, MLO_TRUE);
      MLO_FloatTest_TestScaleP2 (MLO_FLOAT_C (123.625), 10, MLO_TRUE);
      MLO_FloatTest_TestScaleP2 (MLO_FLOAT_C (123.625), -10, MLO_TRUE);
      MLO_FloatTest_TestScaleP2 (MLO_FLOAT_C (123.625), 60, MLO_TRUE);
      MLO_FloatTest_TestScaleP2 (MLO_FLOAT_C (123.625), -60, MLO_TRUE);
      printf ("\n");

      result = MLO_FloatTest_TestScaleP2All ();
      printf ("\n");
   }

   return (result);
}
