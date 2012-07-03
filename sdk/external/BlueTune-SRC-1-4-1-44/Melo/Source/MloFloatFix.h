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

#ifndef _MLO_FLOAT_FIX_H_
#define _MLO_FLOAT_FIX_H_



#if ! defined (_MLO_FLOAT_H_)
#error This header can be included only by MloFloat.h
#endif



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloConfig.h"
#include "MloDebug.h"



/*----------------------------------------------------------------------
|       Definitions
+---------------------------------------------------------------------*/



#if ! defined (MLO_CONFIG_HAVE_INT64)
#error This implementation requires 64-bit integer type
#endif



typedef  MLO_CONFIG_INT64_TYPE   MLO_Float;



/* Resolution in bits */
#define MLO_FLOAT_BD 64

/* Number of bits of the fractional part. Must be even */
#define MLO_FLOAT_FRAC 32

#define  MLO_FLOAT_C(x) ((MLO_Float) ((x) * (65536.0 * (double) (1L << (MLO_FLOAT_FRAC - 16)))))

static const MLO_Float  MLO_Float_one = ((MLO_Float) 1) << MLO_FLOAT_FRAC;
static const MLO_Float  MLO_Float_frac_mask = (((MLO_Float) 1) << MLO_FLOAT_FRAC) - 1;
static const MLO_Float  MLO_Float_max = ~((MLO_Float) 1) ^ (((MLO_Float)1) << (MLO_FLOAT_BD - 1));

#if ! defined (NDEBUG)
#define  MLO_FLOAT_IS_SAME_SIGN(a, b)  (((a) <= 0 && (b) <= 0) || ((a) >= 0 && (b) >= 0))
#endif



/*----------------------------------------------------------------------
|       Private functions
+---------------------------------------------------------------------*/
static inline MLO_CONFIG_INT64_TYPE MLO_Float_Mul3232To64 (MLO_Int32 a, MLO_Int32 b)
{
#if defined (_MSC_VER) && defined(_X86_)

   __asm
   {
      mov         eax, a
      mov         edx, b
      imul        edx
   }
   /* No return */

#elif defined (__GNUC__) && defined (__i386__)

   MLO_CONFIG_INT64_TYPE   ret_val;
	__asm__ (
      "imul %2"
      : "=A" (ret_val)
      : "a" (a), "r" (b)
   );
   return (ret_val);

#else

   return (((MLO_CONFIG_INT64_TYPE) a) * ((MLO_CONFIG_INT64_TYPE) b));

#endif
}

/*----------------------------------------------------------------------
|       Public functions
+---------------------------------------------------------------------*/
static inline MLO_Float MLO_Float_ConvIntToFloat (int a)
{
   return (((MLO_Float) a) << MLO_FLOAT_FRAC);
}

static inline int MLO_Float_RoundInt (MLO_Float a)
{
   return ((int) ((a + (((MLO_Float) 1) << (MLO_FLOAT_FRAC - 1))) >> MLO_FLOAT_FRAC));
}

static inline MLO_Float MLO_Float_Add (MLO_Float a, MLO_Float b)
{
   return (a + b);
}

static inline MLO_Float MLO_Float_Sub (MLO_Float a, MLO_Float b)
{
   return (a - b);
}

static inline MLO_Float MLO_Float_Neg (MLO_Float a)
{
   return (-a);
}

static inline unsigned long long
MLO_Float_Mul3232To64U(unsigned long a, unsigned long b)
{
    return ((unsigned long long)a)*((unsigned long long)b);
}

static inline MLO_Float MLO_Float_Mul (MLO_Float a, MLO_Float b)
{
   int change_sign = 0;
   if (a < 0) {
        a = -a;
        change_sign = 1;
   } 
   if (b < 0) {
        b = -b;
        change_sign = 1-change_sign;
   }

   {
       const MLO_UInt32   a_f = ((MLO_UInt32) a) & ((MLO_UInt32) MLO_Float_frac_mask);
       const MLO_UInt32   a_i = (MLO_UInt32) (a >> MLO_FLOAT_FRAC);
       const MLO_UInt32   b_f = ((MLO_UInt32) b) & ((MLO_UInt32) MLO_Float_frac_mask);
       const MLO_UInt32   b_i = (MLO_UInt32) (b >> MLO_FLOAT_FRAC);

#if 1
   const MLO_Float   ab_ff = (MLO_Float_Mul3232To64U(a_f, b_f)) >> MLO_FLOAT_FRAC;
   const MLO_Float   ab_if = MLO_Float_Mul3232To64U (a_i, b_f) + MLO_Float_Mul3232To64U (a_f, b_i);
   const MLO_Float   ab_ii = MLO_Float_Mul3232To64U (a_i, b_i) << MLO_FLOAT_FRAC;

   MLO_Float result = (ab_ii + ab_if + ab_ff);
   if (change_sign) {
      return -result;
   } else {
      return result;
   }

#else

   const MLO_Float   ab_ff = ((unsigned long long)MLO_Float_Mul3232To64 (a_f, b_f)) >> MLO_FLOAT_FRAC;
   const MLO_Float   ab_if = MLO_Float_Mul3232To64 (a_i, b_f) + MLO_Float_Mul3232To64 (a_f, b_i);
   const MLO_Float   ab_ii = MLO_Float_Mul3232To64 (a_i, b_i) << MLO_FLOAT_FRAC;

   return (ab_ii + ab_if + ab_ff);

#endif
    }
}

static inline MLO_Float MLO_Float_MulInt (MLO_Float a, int b)
{
   return (a * b);
}

/* Only for positive operands */
static inline MLO_Float MLO_Float_Div (MLO_Float a, MLO_Float b)
{
   MLO_CONFIG_INT64_TYPE   d;
   MLO_Float      sum;

   MLO_ASSERT (a >= 0);
   MLO_ASSERT (b > 0);
   MLO_ASSERT (b < (((MLO_CONFIG_INT64_TYPE)1) << (64-1 - 8)));

   d = a / b;  /* Integer part */
   sum = d << MLO_FLOAT_FRAC;
   a -= d * b; /* Remainder */

   a <<= MLO_FLOAT_FRAC - 24;
   d = a / b;
   sum += d << 24;
   a -= d * b;

   a <<= 24 - 16;
   d = a / b;
   sum += d << 16;
   a -= d * b;

   a <<= 16 - 8;
   d = a / b;
   sum += d << 8;
   a -= d * b;

   a <<= 8 - 0;
   d = a / b;
   sum += d;
   a -= d * b;

   return (sum);
}

static inline MLO_Float MLO_Float_DivInt (MLO_Float a, int b)
{
   MLO_ASSERT (b > 0);

   return (a / b);
}

static inline MLO_Float MLO_Float_ScaleP2 (MLO_Float a, int b)
{
   if (b < 0)
   {
      if (b > -64)
      {
         a >>= -b;
      }
      else
      {
         a = 0;
      }
   }
   else
   {
      MLO_ASSERT (b < 64);

      a <<= b;
   }

   return (a);
}

/*

Algorithm based on the binary decomposition of a square:
Input: r
Output: a = r*r

for (i = 0; i < n; ++i)
{
   if ((r & (1 << i)) != 0)
   {
      r -= 1 << i;
      a += (1 << (2*i)) + (r << (i+1))
   }
}

Inverted to obtain the square root:
Input: a = r*r
Output: r

for (i = n-1; i >= 0; --i)
{
   an = a - (1 << (2*i)) - (r << (i+1))
   if (an >= 0)
   {
      a = an;
      r += 1 << i;
   }
}

*/
static inline MLO_Float MLO_Float_Sqrt (MLO_Float a)
{
   MLO_Float      r = 0;

   MLO_CHECK_CST (Even_precision_requested, (MLO_FLOAT_FRAC & 1) == 0);

#if 1 /* Unrolled loop */

#define  MLO_FLOAT_SQRT_ITERATE(a, r, i)  do \
   {  \
      const MLO_Float   tmp = a - (((MLO_Float)1) << (2*i)) - (r << (i+1)); \
      if (tmp >= 0)   \
      {  \
         a = tmp;  \
         r += ((MLO_Float)1) << i;  \
      }  \
   } while (0)

   if (a >= ((MLO_Float) 1) << (30+8))
   {
      MLO_FLOAT_SQRT_ITERATE (a, r, 30);
      MLO_FLOAT_SQRT_ITERATE (a, r, 29);
      MLO_FLOAT_SQRT_ITERATE (a, r, 28);
      MLO_FLOAT_SQRT_ITERATE (a, r, 27);
      MLO_FLOAT_SQRT_ITERATE (a, r, 26);
      MLO_FLOAT_SQRT_ITERATE (a, r, 25);
      MLO_FLOAT_SQRT_ITERATE (a, r, 24);
      MLO_FLOAT_SQRT_ITERATE (a, r, 23);
      MLO_FLOAT_SQRT_ITERATE (a, r, 22);
      MLO_FLOAT_SQRT_ITERATE (a, r, 21);
      MLO_FLOAT_SQRT_ITERATE (a, r, 20);
      MLO_FLOAT_SQRT_ITERATE (a, r, 19);
   }
   MLO_FLOAT_SQRT_ITERATE (a, r, 18);
   MLO_FLOAT_SQRT_ITERATE (a, r, 17);
   MLO_FLOAT_SQRT_ITERATE (a, r, 16);
   MLO_FLOAT_SQRT_ITERATE (a, r, 15);
   MLO_FLOAT_SQRT_ITERATE (a, r, 14);
   MLO_FLOAT_SQRT_ITERATE (a, r, 13);
   MLO_FLOAT_SQRT_ITERATE (a, r, 12);
   MLO_FLOAT_SQRT_ITERATE (a, r, 11);
   MLO_FLOAT_SQRT_ITERATE (a, r, 10);
   MLO_FLOAT_SQRT_ITERATE (a, r,  9);
   MLO_FLOAT_SQRT_ITERATE (a, r,  8);
   MLO_FLOAT_SQRT_ITERATE (a, r,  7);
   MLO_FLOAT_SQRT_ITERATE (a, r,  6);
   MLO_FLOAT_SQRT_ITERATE (a, r,  5);
   MLO_FLOAT_SQRT_ITERATE (a, r,  4);
   MLO_FLOAT_SQRT_ITERATE (a, r,  3);
   MLO_FLOAT_SQRT_ITERATE (a, r,  2);
   MLO_FLOAT_SQRT_ITERATE (a, r,  1);
   if (r < a)
   {
      ++r;
   }

   return (r << (MLO_FLOAT_FRAC / 2));

#undef   MLO_FLOAT_SQRT_ITERATE

#else /* Naive implementation */

   MLO_Float      m = ((MLO_Float)1) << 60;

   MLO_ASSERT (a >= 0);

   do
   {
      const MLO_Float   a_new = a - m - r;
      r >>= 1;
      if (a_new >= 0)
      {
         a = a_new;
         r += m;
      }
      m >>= 2;
   }
   while (m > 0);

   return (r << (MLO_FLOAT_FRAC / 2));

#endif
}

static inline MLO_Float MLO_Float_Lerp (MLO_Float a, MLO_Float b, int k, int bits)
{
   const MLO_Float   diff = MLO_Float_Sub (b, a);

   MLO_ASSERT (k >= 0);
   MLO_ASSERT (k < (1L << bits));
   
   return (MLO_Float_Add (a, MLO_Float_MulInt (diff, k) >> bits));
}



#endif   /* _MLO_FLOAT_FIX_H_ */
