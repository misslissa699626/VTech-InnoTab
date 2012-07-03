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

#ifndef _MLO_FLOAT_FPU_H_
#define _MLO_FLOAT_FPU_H_



#if ! defined (_MLO_FLOAT_H_)
#error This header can be included only by MloFloat.h
#endif



/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloConfig.h"
#include "MloDebug.h"

#if defined(MLO_CONFIG_HAVE_MATH_H)
#include <math.h>
#endif



/*----------------------------------------------------------------------
|       Definitions
+---------------------------------------------------------------------*/



typedef  float MLO_Float;

#define  MLO_FLOAT_C(x) ((float) (x))



enum
{
   MLO_FLOAT_TABLE_P2_MIN  = -126,
   MLO_FLOAT_TABLE_P2_MAX  = +126,
   MLO_FLOAT_TABLE_P2_LEN  = MLO_FLOAT_TABLE_P2_MAX - MLO_FLOAT_TABLE_P2_MIN + 1
};

static const MLO_Float MLO_Float_table_p2 [MLO_FLOAT_TABLE_P2_LEN] =
{
   MLO_FLOAT_C (1.17549435e-038),
   MLO_FLOAT_C (2.3509887e-038),
   MLO_FLOAT_C (4.7019774e-038),
   MLO_FLOAT_C (9.40395481e-038),
   MLO_FLOAT_C (1.88079096e-037),
   MLO_FLOAT_C (3.76158192e-037),
   MLO_FLOAT_C (7.52316385e-037),
   MLO_FLOAT_C (1.50463277e-036),
   MLO_FLOAT_C (3.00926554e-036),
   MLO_FLOAT_C (6.01853108e-036),
   MLO_FLOAT_C (1.20370622e-035),
   MLO_FLOAT_C (2.40741243e-035),
   MLO_FLOAT_C (4.81482486e-035),
   MLO_FLOAT_C (9.62964972e-035),
   MLO_FLOAT_C (1.92592994e-034),
   MLO_FLOAT_C (3.85185989e-034),
   MLO_FLOAT_C (7.70371978e-034),
   MLO_FLOAT_C (1.54074396e-033),
   MLO_FLOAT_C (3.08148791e-033),
   MLO_FLOAT_C (6.16297582e-033),
   MLO_FLOAT_C (1.23259516e-032),
   MLO_FLOAT_C (2.46519033e-032),
   MLO_FLOAT_C (4.93038066e-032),
   MLO_FLOAT_C (9.86076132e-032),
   MLO_FLOAT_C (1.97215226e-031),
   MLO_FLOAT_C (3.94430453e-031),
   MLO_FLOAT_C (7.88860905e-031),
   MLO_FLOAT_C (1.57772181e-030),
   MLO_FLOAT_C (3.15544362e-030),
   MLO_FLOAT_C (6.31088724e-030),
   MLO_FLOAT_C (1.26217745e-029),
   MLO_FLOAT_C (2.5243549e-029),
   MLO_FLOAT_C (5.04870979e-029),
   MLO_FLOAT_C (1.00974196e-028),
   MLO_FLOAT_C (2.01948392e-028),
   MLO_FLOAT_C (4.03896783e-028),
   MLO_FLOAT_C (8.07793567e-028),
   MLO_FLOAT_C (1.61558713e-027),
   MLO_FLOAT_C (3.23117427e-027),
   MLO_FLOAT_C (6.46234854e-027),
   MLO_FLOAT_C (1.29246971e-026),
   MLO_FLOAT_C (2.58493941e-026),
   MLO_FLOAT_C (5.16987883e-026),
   MLO_FLOAT_C (1.03397577e-025),
   MLO_FLOAT_C (2.06795153e-025),
   MLO_FLOAT_C (4.13590306e-025),
   MLO_FLOAT_C (8.27180613e-025),
   MLO_FLOAT_C (1.65436123e-024),
   MLO_FLOAT_C (3.30872245e-024),
   MLO_FLOAT_C (6.6174449e-024),
   MLO_FLOAT_C (1.32348898e-023),
   MLO_FLOAT_C (2.64697796e-023),
   MLO_FLOAT_C (5.29395592e-023),
   MLO_FLOAT_C (1.05879118e-022),
   MLO_FLOAT_C (2.11758237e-022),
   MLO_FLOAT_C (4.23516474e-022),
   MLO_FLOAT_C (8.47032947e-022),
   MLO_FLOAT_C (1.69406589e-021),
   MLO_FLOAT_C (3.38813179e-021),
   MLO_FLOAT_C (6.77626358e-021),
   MLO_FLOAT_C (1.35525272e-020),
   MLO_FLOAT_C (2.71050543e-020),
   MLO_FLOAT_C (5.42101086e-020),
   MLO_FLOAT_C (1.08420217e-019),
   MLO_FLOAT_C (2.16840434e-019),
   MLO_FLOAT_C (4.33680869e-019),
   MLO_FLOAT_C (8.67361738e-019),
   MLO_FLOAT_C (1.73472348e-018),
   MLO_FLOAT_C (3.46944695e-018),
   MLO_FLOAT_C (6.9388939e-018),
   MLO_FLOAT_C (1.38777878e-017),
   MLO_FLOAT_C (2.77555756e-017),
   MLO_FLOAT_C (5.55111512e-017),
   MLO_FLOAT_C (1.11022302e-016),
   MLO_FLOAT_C (2.22044605e-016),
   MLO_FLOAT_C (4.4408921e-016),
   MLO_FLOAT_C (8.8817842e-016),
   MLO_FLOAT_C (1.77635684e-015),
   MLO_FLOAT_C (3.55271368e-015),
   MLO_FLOAT_C (7.10542736e-015),
   MLO_FLOAT_C (1.42108547e-014),
   MLO_FLOAT_C (2.84217094e-014),
   MLO_FLOAT_C (5.68434189e-014),
   MLO_FLOAT_C (1.13686838e-013),
   MLO_FLOAT_C (2.27373675e-013),
   MLO_FLOAT_C (4.54747351e-013),
   MLO_FLOAT_C (9.09494702e-013),
   MLO_FLOAT_C (1.8189894e-012),
   MLO_FLOAT_C (3.63797881e-012),
   MLO_FLOAT_C (7.27595761e-012),
   MLO_FLOAT_C (1.45519152e-011),
   MLO_FLOAT_C (2.91038305e-011),
   MLO_FLOAT_C (5.82076609e-011),
   MLO_FLOAT_C (1.16415322e-010),
   MLO_FLOAT_C (2.32830644e-010),
   MLO_FLOAT_C (4.65661287e-010),
   MLO_FLOAT_C (9.31322575e-010),
   MLO_FLOAT_C (1.86264515e-009),
   MLO_FLOAT_C (3.7252903e-009),
   MLO_FLOAT_C (7.4505806e-009),
   MLO_FLOAT_C (1.49011612e-008),
   MLO_FLOAT_C (2.98023224e-008),
   MLO_FLOAT_C (5.96046448e-008),
   MLO_FLOAT_C (1.1920929e-007),
   MLO_FLOAT_C (2.38418579e-007),
   MLO_FLOAT_C (4.76837158e-007),
   MLO_FLOAT_C (9.53674316e-007),
   MLO_FLOAT_C (1.90734863e-006),
   MLO_FLOAT_C (3.81469727e-006),
   MLO_FLOAT_C (7.62939453e-006),
   MLO_FLOAT_C (1.52587891e-005),
   MLO_FLOAT_C (3.05175781e-005),
   MLO_FLOAT_C (6.10351563e-005),
   MLO_FLOAT_C (0.000122070313),
   MLO_FLOAT_C (0.000244140625),
   MLO_FLOAT_C (0.00048828125),
   MLO_FLOAT_C (0.0009765625),
   MLO_FLOAT_C (0.001953125),
   MLO_FLOAT_C (0.00390625),
   MLO_FLOAT_C (0.0078125),
   MLO_FLOAT_C (0.015625),
   MLO_FLOAT_C (0.03125),
   MLO_FLOAT_C (0.0625),
   MLO_FLOAT_C (0.125),
   MLO_FLOAT_C (0.25),
   MLO_FLOAT_C (0.5),
   MLO_FLOAT_C (1),
   MLO_FLOAT_C (2),
   MLO_FLOAT_C (4),
   MLO_FLOAT_C (8),
   MLO_FLOAT_C (16),
   MLO_FLOAT_C (32),
   MLO_FLOAT_C (64),
   MLO_FLOAT_C (128),
   MLO_FLOAT_C (256),
   MLO_FLOAT_C (512),
   MLO_FLOAT_C (1024),
   MLO_FLOAT_C (2048),
   MLO_FLOAT_C (4096),
   MLO_FLOAT_C (8192),
   MLO_FLOAT_C (16384),
   MLO_FLOAT_C (32768),
   MLO_FLOAT_C (65536),
   MLO_FLOAT_C (131072),
   MLO_FLOAT_C (262144),
   MLO_FLOAT_C (524288),
   MLO_FLOAT_C (1048576),
   MLO_FLOAT_C (2097152),
   MLO_FLOAT_C (4194304),
   MLO_FLOAT_C (8388608),
   MLO_FLOAT_C (16777216),
   MLO_FLOAT_C (33554432),
   MLO_FLOAT_C (67108864),
   MLO_FLOAT_C (134217728),
   MLO_FLOAT_C (268435456),
   MLO_FLOAT_C (536870912),
   MLO_FLOAT_C (1.07374182e+009),
   MLO_FLOAT_C (2.14748365e+009),
   MLO_FLOAT_C (4.2949673e+009),
   MLO_FLOAT_C (8.58993459e+009),
   MLO_FLOAT_C (1.71798692e+010),
   MLO_FLOAT_C (3.43597384e+010),
   MLO_FLOAT_C (6.87194767e+010),
   MLO_FLOAT_C (1.37438953e+011),
   MLO_FLOAT_C (2.74877907e+011),
   MLO_FLOAT_C (5.49755814e+011),
   MLO_FLOAT_C (1.09951163e+012),
   MLO_FLOAT_C (2.19902326e+012),
   MLO_FLOAT_C (4.39804651e+012),
   MLO_FLOAT_C (8.79609302e+012),
   MLO_FLOAT_C (1.7592186e+013),
   MLO_FLOAT_C (3.51843721e+013),
   MLO_FLOAT_C (7.03687442e+013),
   MLO_FLOAT_C (1.40737488e+014),
   MLO_FLOAT_C (2.81474977e+014),
   MLO_FLOAT_C (5.62949953e+014),
   MLO_FLOAT_C (1.12589991e+015),
   MLO_FLOAT_C (2.25179981e+015),
   MLO_FLOAT_C (4.50359963e+015),
   MLO_FLOAT_C (9.00719925e+015),
   MLO_FLOAT_C (1.80143985e+016),
   MLO_FLOAT_C (3.6028797e+016),
   MLO_FLOAT_C (7.2057594e+016),
   MLO_FLOAT_C (1.44115188e+017),
   MLO_FLOAT_C (2.88230376e+017),
   MLO_FLOAT_C (5.76460752e+017),
   MLO_FLOAT_C (1.1529215e+018),
   MLO_FLOAT_C (2.30584301e+018),
   MLO_FLOAT_C (4.61168602e+018),
   MLO_FLOAT_C (9.22337204e+018),
   MLO_FLOAT_C (1.84467441e+019),
   MLO_FLOAT_C (3.68934881e+019),
   MLO_FLOAT_C (7.37869763e+019),
   MLO_FLOAT_C (1.47573953e+020),
   MLO_FLOAT_C (2.95147905e+020),
   MLO_FLOAT_C (5.9029581e+020),
   MLO_FLOAT_C (1.18059162e+021),
   MLO_FLOAT_C (2.36118324e+021),
   MLO_FLOAT_C (4.72236648e+021),
   MLO_FLOAT_C (9.44473297e+021),
   MLO_FLOAT_C (1.88894659e+022),
   MLO_FLOAT_C (3.77789319e+022),
   MLO_FLOAT_C (7.55578637e+022),
   MLO_FLOAT_C (1.51115727e+023),
   MLO_FLOAT_C (3.02231455e+023),
   MLO_FLOAT_C (6.0446291e+023),
   MLO_FLOAT_C (1.20892582e+024),
   MLO_FLOAT_C (2.41785164e+024),
   MLO_FLOAT_C (4.83570328e+024),
   MLO_FLOAT_C (9.67140656e+024),
   MLO_FLOAT_C (1.93428131e+025),
   MLO_FLOAT_C (3.86856262e+025),
   MLO_FLOAT_C (7.73712525e+025),
   MLO_FLOAT_C (1.54742505e+026),
   MLO_FLOAT_C (3.0948501e+026),
   MLO_FLOAT_C (6.1897002e+026),
   MLO_FLOAT_C (1.23794004e+027),
   MLO_FLOAT_C (2.47588008e+027),
   MLO_FLOAT_C (4.95176016e+027),
   MLO_FLOAT_C (9.90352031e+027),
   MLO_FLOAT_C (1.98070406e+028),
   MLO_FLOAT_C (3.96140813e+028),
   MLO_FLOAT_C (7.92281625e+028),
   MLO_FLOAT_C (1.58456325e+029),
   MLO_FLOAT_C (3.1691265e+029),
   MLO_FLOAT_C (6.338253e+029),
   MLO_FLOAT_C (1.2676506e+030),
   MLO_FLOAT_C (2.5353012e+030),
   MLO_FLOAT_C (5.0706024e+030),
   MLO_FLOAT_C (1.01412048e+031),
   MLO_FLOAT_C (2.02824096e+031),
   MLO_FLOAT_C (4.05648192e+031),
   MLO_FLOAT_C (8.11296384e+031),
   MLO_FLOAT_C (1.62259277e+032),
   MLO_FLOAT_C (3.24518554e+032),
   MLO_FLOAT_C (6.49037107e+032),
   MLO_FLOAT_C (1.29807421e+033),
   MLO_FLOAT_C (2.59614843e+033),
   MLO_FLOAT_C (5.19229686e+033),
   MLO_FLOAT_C (1.03845937e+034),
   MLO_FLOAT_C (2.07691874e+034),
   MLO_FLOAT_C (4.15383749e+034),
   MLO_FLOAT_C (8.30767497e+034),
   MLO_FLOAT_C (1.66153499e+035),
   MLO_FLOAT_C (3.32306999e+035),
   MLO_FLOAT_C (6.64613998e+035),
   MLO_FLOAT_C (1.329228e+036),
   MLO_FLOAT_C (2.65845599e+036),
   MLO_FLOAT_C (5.31691198e+036),
   MLO_FLOAT_C (1.0633824e+037),
   MLO_FLOAT_C (2.12676479e+037),
   MLO_FLOAT_C (4.25352959e+037),
   MLO_FLOAT_C (8.50705917e+037)
};



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



static inline MLO_Float MLO_Float_ConvIntToFloat (int a)
{
   return ((MLO_Float) a);
}

static inline int MLO_Float_RoundInt (MLO_Float a)
{
   return ((int) floor (a + 0.5f));
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

static inline MLO_Float MLO_Float_Mul (MLO_Float a, MLO_Float b)
{
   return (a * b);
}

static inline MLO_Float MLO_Float_MulInt (MLO_Float a, int b)
{
   return (a * b);
}

static inline MLO_Float MLO_Float_Div (MLO_Float a, MLO_Float b)
{
   MLO_ASSERT (a >= 0);
   MLO_ASSERT (b > 0);

   return (a / b);
}

static inline MLO_Float MLO_Float_DivInt (MLO_Float a, int b)
{
   MLO_ASSERT (a >= 0);
   MLO_ASSERT (b > 0);

   return (a / b);
}

static inline MLO_Float MLO_Float_ScaleP2 (MLO_Float a, int b)
{
   MLO_ASSERT (b >= MLO_FLOAT_TABLE_P2_MIN);
   MLO_ASSERT (b <= MLO_FLOAT_TABLE_P2_MAX);

   return (a * MLO_Float_table_p2 [b - MLO_FLOAT_TABLE_P2_MIN]);
}

static inline MLO_Float MLO_Float_Sqrt (MLO_Float a)
{
   MLO_ASSERT (a >= 0);

#if defined (MLO_CONFIG_HAVE_SQRT)
   return ((MLO_Float) sqrt (a));
#else
#error sqrt() not defined.
#endif
}



#endif   /* _MLO_FLOAT_FPU_H_ */
