/*****************************************************************
|
|   Fluo - Math Support
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _FLO_MATH_H_
#define _FLO_MATH_H_

/*-------------------------------------------------------------------------
|       includes
+-------------------------------------------------------------------------*/
#include "FloConfig.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*-------------------------------------------------------------------------
|       macros
+-------------------------------------------------------------------------*/
#ifdef FLO_CONFIG_INTEGER_DECODE
#define FLO_ZERO  0
typedef long FLO_Float;
#define FLO_FDIV2(x) ((x)/2)

#ifdef FLO_CONFIG_INTEGER_32
#define FLO_FIX_MUL(x, y, bits) ((FLO_Float)(((x)*(y))>>(bits)))
#define FLO_FIX_BITS 16
#define FLO_FC0_DSCL  0
#define FLO_FC0_BITS 14                           
#define FLO_FC1_BITS 14
#define FLO_FC3_BITS 30
#define FLO_FC4_BITS 14
#define FLO_FC5_BITS 14
#define FLO_FC6_BITS 12
#define FLO_FC7_BITS 10
#define FLO_FC8_BITS 23
#define FLO_FC9_BITS 5
#else /* FLO_CONFIG_INTEGER_32 */
#if defined(__GNUC__) && defined(__arm__) && !defined(BLT_DEBUG)

/* define the following for  multiple inline functions */
/* (one per bit shift constant)                        */
/*#define FLO_FIX_MUL_MULTI_DECLARE 1 */
#if defined(FLO_FIX_MUL_MULTI_DECLARE)
#define FLO_DECLARE_FIX_MUL(_name, _bits)                      \
static inline long FLO_##_name##_MUL(FLO_Float x, FLO_Float y) \
{                                                              \
    long result;                                               \
    asm("smull r8, r9, %1, %2\n\t"                             \
        "mov r9, r9, lsl %3\n\t"                               \
        "orr %0, r9, r8, lsr %4"                               \
        : "=r" (result)                                        \
        : "r" (x), "r" (y), "I" (32-(_bits)), "I" (_bits)      \
        : "r8", "r9"); /* r8 and r9 are clobbered */           \
    return result;                                             \
}
#else
static inline long FLO_FIX_MUL(FLO_Float x, FLO_Float y, int bits)
{                                                                          
    long result;                                                            
    asm("smull r8, r9, %1, %2\n\t"                                          
        "mov r9, r9, lsl %3\n\t"                                            
        "orr %0, r9, r8, lsr %4"                                            
        : "=r" (result)                                                     
        : "r" (x), "r" (y), "i" (32-bits), "i" (bits)                       
        : "r8", "r9"); /* r8 and r9 are clobbered */         
    return result;                                                                 
}
#endif

#else
#define FLO_FIX_MUL(x, y, bits) ((FLO_Float)(((long long)(x)*(long long)(y))>>(bits)))
#endif

#define FLO_FIX_BITS 24
#define FLO_FC0_DSCL 16
#define FLO_FC0_BITS 16                           
#define FLO_FC1_BITS 24
#define FLO_FC3_BITS 30
#define FLO_FC4_BITS 24
#define FLO_FC5_BITS 24
#define FLO_FC6_BITS 22
#define FLO_FC7_BITS 20
#define FLO_FC8_BITS 31
#define FLO_FC9_BITS 13
#endif /* FLO_CONFIG_INTEGER_32 */

#define FLO_FIX_CONV(x) ((FLO_Float)((x)<0.0?((x)-0.5):((x)+0.5)))
#define FLO_FIX_TO_SHORT(sample) ((int)(sample)>>(FLO_FIX_BITS-16+FLO_FC0_BITS-FLO_FC0_DSCL))
#if FLO_FC0_BITS > 15
#define FLO_FC0(x) ((FLO_Float)((x)*(1<<(FLO_FC0_BITS-15))))
#else
#define FLO_FC0(x) ((FLO_Float)((x)/(1<<(15-FLO_FC0_BITS))))
#endif
#define FLO_FC1(x) FLO_FIX_CONV((x)*(1<<FLO_FC1_BITS))
#define FLO_FC2(x) FLO_FIX_CONV((x)*(1<<(FLO_FIX_BITS-1)))
#define FLO_FC3(x,y) FLO_FIX_CONV((x)*((1<<FLO_FC3_BITS)/(y)))
#define FLO_FC4(x) FLO_FIX_CONV((x)*(1<<FLO_FC4_BITS))
#define FLO_FC5(x) FLO_FIX_CONV((x)*(1<<FLO_FC5_BITS))
#define FLO_FC6(x) FLO_FIX_CONV((x)*(1<<FLO_FC6_BITS))
#define FLO_FC7(x) FLO_FIX_CONV((x)*(1<<FLO_FC7_BITS))
#define FLO_FC8(x) FLO_FIX_CONV((x)*(1UL<<FLO_FC8_BITS))
#define FLO_FC8_SCL(x) ((x)>>(FLO_FC8_BITS-(FLO_FIX_BITS-1)))
#define FLO_FC9(x) FLO_FIX_CONV((x)*(1<<FLO_FC9_BITS))

#if defined(FLO_FIX_MUL_MULTI_DECLARE)
#if FLO_FC0_DSCL > 0
FLO_DECLARE_FIX_MUL(FC0, FLO_FC0_DSCL)
#else
#define FLO_FC0_MUL(x, y) ((x)*(y))
#endif
FLO_DECLARE_FIX_MUL(FC1, FLO_FC1_BITS)
FLO_DECLARE_FIX_MUL(FC3, (FLO_FC3_BITS+1-FLO_FIX_BITS))
FLO_DECLARE_FIX_MUL(FC4, FLO_FC4_BITS)
FLO_DECLARE_FIX_MUL(FC5, FLO_FC5_BITS)
FLO_DECLARE_FIX_MUL(FC6, FLO_FC6_BITS)
FLO_DECLARE_FIX_MUL(FC7, FLO_FC7_BITS) 
FLO_DECLARE_FIX_MUL(FC8, (FLO_FC9_BITS+FLO_FC8_BITS-(FLO_FIX_BITS-1)))
#else /* FLO_FIX_MUL_MULTI_DECLARE */
#if FLO_FC0_DSCL > 0
#define FLO_FC0_MUL(x, y) FLO_FIX_MUL(x, y, FLO_FC0_DSCL)
#else
#define FLO_FC0_MUL(x, y) ((x)*(y))
#endif
#define FLO_FC1_MUL(x, y) FLO_FIX_MUL(x, y, FLO_FC1_BITS)
#define FLO_FC3_MUL(x, y) FLO_FIX_MUL(x, y, (FLO_FC3_BITS+1-FLO_FIX_BITS))
#define FLO_FC4_MUL(x, y) FLO_FIX_MUL(x, y, FLO_FC4_BITS)
#define FLO_FC5_MUL(x, y) FLO_FIX_MUL(x, y, FLO_FC5_BITS)
#define FLO_FC6_MUL(x, y) FLO_FIX_MUL(x, y, FLO_FC6_BITS)
#define FLO_FC7_MUL(x, y) FLO_FIX_MUL(x, y, FLO_FC7_BITS) 
#define FLO_FC8_MUL(x, y) FLO_FIX_MUL(x, y, (FLO_FC9_BITS+FLO_FC8_BITS-(FLO_FIX_BITS-1)))
#endif /* FLO_FIX_MUL_MULTI_DECLARE */

#else

#define FLO_ZERO  0.0f
typedef float FLO_Float;
#define FLO_FDIV2(x) (0.5f*(x))
#define FLO_FIX_TO_SHORT(sample) ((int)(sample))
#define FLO_FC0(x) x##f
#define FLO_FC1(x) ((FLO_Float)(x))
#define FLO_FC2(x) ((FLO_Float)(x))
#define FLO_FC3(x, y) ((FLO_Float)(x/y))
#define FLO_FC4(x) ((FLO_Float)(x))
#define FLO_FC5(x) ((FLO_Float)(x))
#define FLO_FC6(x) x##f
#define FLO_FC7(x) ((FLO_Float)(x))
#define FLO_FC8(x) x##f
#define FLO_FC9(x) x##f
#define FLO_FC0_MUL(x, y) ((x) * (y))
#define FLO_FC1_MUL(x, y) ((x) * (y))
#define FLO_FC4_MUL(x, y) ((x) * (y))
#define FLO_FC5_MUL(x, y) ((x) * (y))
#define FLO_FC6_MUL(x, y) ((x) * (y))
#define FLO_FC7_MUL(x, y) ((x) * (y))
#define FLO_FC8_MUL(x, y) ((x) * (y))
#define FLO_FC8_SCL(x)     (x)
#define FLO_FC9_MUL(x, y) ((x) * (y))
#endif /* FLO_CONFIG_INTEGER_DECODE */

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

#endif /* _FLO_MATH_H_ */






