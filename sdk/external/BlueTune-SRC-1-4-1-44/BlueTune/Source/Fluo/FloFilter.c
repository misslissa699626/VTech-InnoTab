/*****************************************************************
|
|   Fluo - Synthesis Filter
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "FloConfig.h"
#include "FloMath.h"
#include "FloFilter.h"
#include "FloTables.h"
#include "FloUtils.h"
#include "FloErrors.h"
#include "FloLayerIII.h"

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define COS_01_64  FLO_FC6(0.500602998235) /* 1/2*cos(1*pi/64)  */
#define COS_03_64  FLO_FC6(0.505470959898) /* 1/2*cos(3*pi/64)  */
#define COS_05_64  FLO_FC6(0.515447309923) /* 1/2*cos(5*pi/64)  */
#define COS_07_64  FLO_FC6(0.531042591090) /* 1/2*cos(7*pi/64)  */
#define COS_09_64  FLO_FC6(0.553103896034) /* 1/2*cos(9*pi/64)  */
#define COS_11_64  FLO_FC6(0.582934968206) /* 1/2*cos(11*pi/64) */
#define COS_13_64  FLO_FC6(0.622504123036) /* 1/2*cos(13*pi/64) */
#define COS_15_64  FLO_FC6(0.674808341455) /* 1/2*cos(15*pi/64) */
#define COS_17_64  FLO_FC6(0.744536271002) /* 1/2*cos(17*pi/64) */
#define COS_19_64  FLO_FC6(0.839349645416) /* 1/2*cos(19*pi/64) */
#define COS_21_64  FLO_FC6(0.972568237862) /* 1/2*cos(21*pi/64) */
#define COS_23_64  FLO_FC6(1.169439933430) /* 1/2*cos(23*pi/64) */
#define COS_25_64  FLO_FC6(1.484164616310) /* 1/2*cos(25*pi/64) */
#define COS_27_64  FLO_FC6(2.057781009950) /* 1/2*cos(27*pi/64) */
#define COS_29_64  FLO_FC6(3.407608418470) /* 1/2*cos(29*pi/64) */
#define COS_31_64  FLO_FC6(10.19000812350) /* 1/2*cos(31*pi/64) */

#define COS_01_36  FLO_FC6(0.501909918772) /* 1/2*cos(1*pi/36)  */
#define COS_03_36  FLO_FC6(0.517638090205) /* 1/2*cos(3*pi/36)  */
#define COS_05_36  FLO_FC6(0.551688959481) /* 1/2*cos(5*pi/36)  */
#define COS_07_36  FLO_FC6(0.610387294381) /* 1/2*cos(7*pi/36)  */
#define COS_09_36  FLO_FC6(0.707106781187) /* 1/2*cos(9*pi/36)  */
#define COS_11_36  FLO_FC6(0.871723397811) /* 1/2*cos(11*pi/36) */ 
#define COS_13_36  FLO_FC6(1.183100791580) /* 1/2*cos(13*pi/36) */
#define COS_15_36  FLO_FC6(1.931851652580) /* 1/2*cos(15*pi/36) */
#define COS_17_36  FLO_FC6(5.736856622830) /* 1/2*cos(17*pi/36) */

#define COS_01_32  FLO_FC6(0.502419286188) /* 1/2*cos(1*pi/32)  */
#define COS_03_32  FLO_FC6(0.522498614940) /* 1/2*cos(3*pi/32)  */
#define COS_05_32  FLO_FC6(0.566944034816) /* 1/2*cos(5*pi/32)  */
#define COS_07_32  FLO_FC6(0.646821783360) /* 1/2*cos(7*pi/32)  */
#define COS_09_32  FLO_FC6(0.788154623451) /* 1/2*cos(9*pi/32)  */
#define COS_11_32  FLO_FC6(1.060677685990) /* 1/2*cos(11*pi/32) */
#define COS_13_32  FLO_FC6(1.722447098240) /* 1/2*cos(13*pi/32) */
#define COS_15_32  FLO_FC6(5.101148618690) /* 1/2*cos(15*pi/32) */

#define COS_01_16  FLO_FC6(0.509795579104) /* 1/2*cos(1*pi/16)  */
#define COS_03_16  FLO_FC6(0.601344886935) /* 1/2*cos(3*pi/16)  */
#define COS_05_16  FLO_FC6(0.899976223136) /* 1/2*cos(5*pi/16)  */
#define COS_07_16  FLO_FC6(2.562915447740) /* 1/2*cos(7*pi/16)  */

#define COS_01_12  FLO_FC6(0.517638090205) /* 1/2*cos(1*pi/12)  */
#define COS_03_12  FLO_FC6(0.707106781187) /* 1/2*cos(3*pi/12)  */
#define COS_05_12  FLO_FC6(1.931851652580) /* 1/2*cos(5*pi/12)  */

#define COS_01_08  FLO_FC6(0.541196100146) /* 1/2*cos(1*pi/8)   */ 
#define COS_03_08  FLO_FC6(1.306562964880) /* 1/2*cos(3*pi/8)   */


#define COS_01_09  FLO_FC6(0.939692620786) /* cos(1*pi/9)       */
#define COS_03_09  FLO_FC6(0.500000000000) /* cos(3*pi/9)       */
#define COS_05_09 -FLO_FC6(0.173648177667) /* cos(5*pi/9)       */
#define COS_07_09 -FLO_FC6(0.766044443119) /* cos(7*pi/9)       */

#define COS_01_06  FLO_FC6(0.866025403784) /* cos(1*pi/6)       */
#define COS_02_06  FLO_FC6(0.500000000000) /* cos(2*pi/6)       */

#define COS_01_04  FLO_FC6(0.707106781187) /* cos(1*pi/4)       */

#define COS_00_18  FLO_FC6(1.000000000000) /* cos(0*pi/18)      */
#define COS_01_18  FLO_FC6(0.984807753012) /* cos(1*pi/18)      */
#define COS_02_18  FLO_FC6(0.939692620786) /* cos(2*pi/18)      */
#define COS_03_18  FLO_FC6(0.866025403784) /* cos(3*pi/18)      */
#define COS_04_18  FLO_FC6(0.766044443119) /* cos(4*pi/18)      */
#define COS_05_18  FLO_FC6(0.642787609687) /* cos(5*pi/18)      */
#define COS_06_18  FLO_FC6(0.500000000000) /* cos(6*pi/18)      */
#define COS_07_18  FLO_FC6(0.342020143326) /* cos(7*pi/18)      */
#define COS_08_18  FLO_FC6(0.173648177667) /* cos(8*pi/18)      */

#define COS_11_18 -FLO_FC6(0.342020143326) /* cos(11*pi/18)     */
#define COS_13_18 -FLO_FC6(0.642787609687) /* cos(13*pi/18)     */

#define FLO_FILTER_STORE_SIZE      (16*17)

/*-------------------------------------------------------------------------
|   FLO_SynthesisFilter_Create
+-------------------------------------------------------------------------*/
FLO_Result
FLO_SynthesisFilter_Create(FLO_SynthesisFilter** filter)
{
    /* allocate memory for the filter data structure */
    (*filter) = (FLO_SynthesisFilter*)FLO_AllocateMemory(sizeof(FLO_SynthesisFilter));
    if (*filter == NULL) return FLO_ERROR_OUT_OF_MEMORY;

    /* reset the fields */
    (*filter)->input  = NULL;

    /* allocate the 2 filter sample banks for the windowing */
    (*filter)->v0     = (FLO_Float*)FLO_AllocateMemory(FLO_FILTER_STORE_SIZE*sizeof(FLO_Float));
    if ((*filter)->v0 == NULL) {
        FLO_FreeMemory((*filter));
        return FLO_ERROR_OUT_OF_MEMORY;
    }
    (*filter)->v1     = (FLO_Float*)FLO_AllocateMemory(FLO_FILTER_STORE_SIZE*sizeof(FLO_Float));
    if ((*filter)->v1 == NULL) {
        FLO_FreeMemory((*filter)->v0);
        FLO_FreeMemory((*filter));
        return FLO_ERROR_OUT_OF_MEMORY;
    }

    /* no equalizer */
    (*filter)->equalizer = NULL;

    /* no sumbsampling */
    (*filter)->subsampling = 0;

    /* reset the values */
    FLO_SynthesisFilter_Reset(*filter);

    return FLO_SUCCESS;
}

/*-------------------------------------------------------------------------
|   FLO_SynthesisFilter_Destroy
+-------------------------------------------------------------------------*/
void
FLO_SynthesisFilter_Destroy(FLO_SynthesisFilter *filter)
{
    if (filter->v0) {
        FLO_FreeMemory(filter->v0);
    }
    if (filter->v1) {
        FLO_FreeMemory(filter->v1);
    }

    FLO_FreeMemory(filter);
}

/*-------------------------------------------------------------------------
|   FLO_SynthesisFilter_Reset
+-------------------------------------------------------------------------*/
void
FLO_SynthesisFilter_Reset(FLO_SynthesisFilter* filter) 
{
    int i;
    if (filter->v0) {
        for (i=0 ; i<FLO_FILTER_STORE_SIZE; i++) {
            filter->v0[i] = FLO_ZERO;
        }
    }
    if (filter->v1) {
        for (i=0 ; i<FLO_FILTER_STORE_SIZE; i++) {
            filter->v1[i] = FLO_ZERO;
        }
    }

    filter->v = filter->v0;
    filter->v_offset = 0;    
}

/*-------------------------------------------------------------------------
|       FLO_SynthesisFilter_Idct
+-------------------------------------------------------------------------*/
static void 
FLO_SynthesisFilter_Idct(FLO_SynthesisFilter* filter)
{
    FLO_Float p00,  p01,  p02,  p03,  p04,  p05,  p06,  p07;
    FLO_Float p08,  p09,  p10,  p11,  p12,  p13,  p14,  p15;
    FLO_Float pp00, pp01, pp02, pp03, pp04, pp05, pp06, pp07;
    FLO_Float pp08, pp09, pp10, pp11, pp12, pp13, pp14, pp15;
    register FLO_Float *s0 =  filter->v + filter->v_offset;
    register FLO_Float *s1 = (filter->v == filter->v0 ?
                              filter->v1 : 
                              filter->v0) + filter->v_offset;
    /*FLO_Float v[FLO_FILTER_NB_SAMPLES];*/

#ifndef FLO_OPTIMIZATION_NO_FAST_INDEXED_LOAD
    {
        FLO_Float* s = filter->input;

        p00 = s[ 0] + s[31];
        p01 = s[ 1] + s[30];
        p02 = s[ 2] + s[29];
        p03 = s[ 3] + s[28];
        p04 = s[ 4] + s[27];
        p05 = s[ 5] + s[26];
        p06 = s[ 6] + s[25];
        p07 = s[ 7] + s[24];
        p08 = s[ 8] + s[23];
        p09 = s[ 9] + s[22];
        p10 = s[10] + s[21];
        p11 = s[11] + s[20];
        p12 = s[12] + s[19];
        p13 = s[13] + s[18];
        p14 = s[14] + s[17];
        p15 = s[15] + s[16];
    }
#else
    {
        FLO_Float *s0, *s1;

        s0 = filter->input;
        s1 = filter->input + 31;

        p00 = *s0++ + *s1--;   
        p01 = *s0++ + *s1--; 
        p02 = *s0++ + *s1--; 
        p03 = *s0++ + *s1--;
        p04 = *s0++ + *s1--; 
        p05 = *s0++ + *s1--; 
        p06 = *s0++ + *s1--; 
        p07 = *s0++ + *s1--;
        p08 = *s0++ + *s1--; 
        p09 = *s0++ + *s1--; 
        p10 = *s0++ + *s1--; 
        p11 = *s0++ + *s1--;
        p12 = *s0++ + *s1--; 
        p13 = *s0++ + *s1--; 
        p14 = *s0++ + *s1--; 
        p15 = *s0   + *s1;
    }
#endif

    pp00 = p00 + p15; 
    pp01 = p01 + p14; 
    pp02 = p02 + p13; 
    pp03 = p03 + p12;
    pp04 = p04 + p11; 
    pp05 = p05 + p10; 
    pp06 = p06 + p09; 
    pp07 = p07 + p08;
    pp08 = FLO_FC6_MUL(COS_01_32, (p00 - p15));
    pp09 = FLO_FC6_MUL(COS_03_32, (p01 - p14));
    pp10 = FLO_FC6_MUL(COS_05_32, (p02 - p13));
    pp11 = FLO_FC6_MUL(COS_07_32, (p03 - p12));
    pp12 = FLO_FC6_MUL(COS_09_32, (p04 - p11));
    pp13 = FLO_FC6_MUL(COS_11_32, (p05 - p10));
    pp14 = FLO_FC6_MUL(COS_13_32, (p06 - p09));
    pp15 = FLO_FC6_MUL(COS_15_32, (p07 - p08));

    p00 = pp00 + pp07; 
    p01 = pp01 + pp06; 
    p02 = pp02 + pp05; 
    p03 = pp03 + pp04;
    p04 = FLO_FC6_MUL(COS_01_16, (pp00 - pp07));
    p05 = FLO_FC6_MUL(COS_03_16, (pp01 - pp06));
    p06 = FLO_FC6_MUL(COS_05_16, (pp02 - pp05));
    p07 = FLO_FC6_MUL(COS_07_16, (pp03 - pp04));
    p08 = pp08 + pp15; 
    p09 = pp09 + pp14; 
    p10 = pp10 + pp13; 
    p11 = pp11 + pp12;
    p12 = FLO_FC6_MUL(COS_01_16, (pp08 - pp15));
    p13 = FLO_FC6_MUL(COS_03_16, (pp09 - pp14));
    p14 = FLO_FC6_MUL(COS_05_16, (pp10 - pp13));
    p15 = FLO_FC6_MUL(COS_07_16, (pp11 - pp12));

    pp00 = p00 + p03; 
    pp01 = p01 + p02;
    pp02 = FLO_FC6_MUL(COS_01_08, (p00 - p03));
    pp03 = FLO_FC6_MUL(COS_03_08, (p01 - p02));
    pp04 = p04 + p07; 
    pp05 = p05 + p06;
    pp06 = FLO_FC6_MUL(COS_01_08, (p04 - p07));
    pp07 = FLO_FC6_MUL(COS_03_08, (p05 - p06));
    pp08 = p08 + p11; 
    pp09 = p09 + p10;
    pp10 = FLO_FC6_MUL(COS_01_08, (p08 - p11));
    pp11 = FLO_FC6_MUL(COS_03_08, (p09 - p10));
    pp12 = p12 + p15; 
    pp13 = p13 + p14;
    pp14 = FLO_FC6_MUL(COS_01_08, (p12 - p15));
    pp15 = FLO_FC6_MUL(COS_03_08, (p13 - p14));

    p00 = pp00 + pp01;
    p01 = FLO_FC6_MUL(COS_01_04, (pp00 - pp01));
    p02 = pp02 + pp03;
    p03 = FLO_FC6_MUL(COS_01_04, (pp02 - pp03));
    p04 = pp04 + pp05;
    p05 = FLO_FC6_MUL(COS_01_04, (pp04 - pp05));
    p06 = pp06 + pp07;
    p07 = FLO_FC6_MUL(COS_01_04, (pp06 - pp07));
    p08 = pp08  + pp09;
    p09 = FLO_FC6_MUL(COS_01_04, (pp08 - pp09));
    p10 = pp10 + pp11;
    p11 = FLO_FC6_MUL(COS_01_04, (pp10 - pp11));
    p12 = pp12 + pp13;
    p13 = FLO_FC6_MUL(COS_01_04, (pp12 - pp13));
    p14 = pp14 + pp15;
    p15 = FLO_FC6_MUL(COS_01_04, (pp14 - pp15));

    { 
        register FLO_Float tmp;

        tmp   = p06 + p07;
        /*v[19]*/ s1[0x040] = -(p05 + tmp);
        /*v[27]*/ s1[0x0C0] = -(p04 + tmp);
        tmp   = p11 + p15;
        /*v[10]*/ s0[0x0A0] = tmp;
        /*v[ 6]*/ s0[0x060] = p13 + tmp;
        tmp   = p14 + p15;
        /*v[29]*/ s1[0x0E0] = -(p08 + p12 + tmp);
        /*v[17]*/ s1[0x020] = -(p09 + p13 + tmp);
        tmp  += p10 + p11;
        /*v[21]*/ s1[0x060] = -(p13 + tmp);
        /*v[25]*/ s1[0x0A0] = -(p12 + tmp);
        /*v[ 2]*/ s0[0x020] = p09 + p13 + p15;
        /*v[ 4]*/ s0[0x040] = p05 + p07;
        /*v[31]*/ s1[0x100] = -p00;
        /*v[ 0]*/ s0[0x000] = p01;
        /*v[ 8]*/ s0[0x080] = p03;
        /*v[12]*/ s0[0x0C0] = p07;
        /*v[14]*/ s0[0x0E0] = p15;
        /*v[23]*/ s1[0x080] = -(p02 + p03);
    }

#ifndef FLO_OPTIMIZATION_NO_FAST_INDEXED_LOAD
    {
        FLO_Float* s = filter->input;

        p00 = FLO_FC6_MUL(COS_01_64, (s[ 0] - s[31]));
        p01 = FLO_FC6_MUL(COS_03_64, (s[ 1] - s[30]));
        p02 = FLO_FC6_MUL(COS_05_64, (s[ 2] - s[29]));
        p03 = FLO_FC6_MUL(COS_07_64, (s[ 3] - s[28]));
        p04 = FLO_FC6_MUL(COS_09_64, (s[ 4] - s[27]));
        p05 = FLO_FC6_MUL(COS_11_64, (s[ 5] - s[26]));
        p06 = FLO_FC6_MUL(COS_13_64, (s[ 6] - s[25]));
        p07 = FLO_FC6_MUL(COS_15_64, (s[ 7] - s[24]));
        p08 = FLO_FC6_MUL(COS_17_64, (s[ 8] - s[23]));
        p09 = FLO_FC6_MUL(COS_19_64, (s[ 9] - s[22]));
        p10 = FLO_FC6_MUL(COS_21_64, (s[10] - s[21]));
        p11 = FLO_FC6_MUL(COS_23_64, (s[11] - s[20]));
        p12 = FLO_FC6_MUL(COS_25_64, (s[12] - s[19]));
        p13 = FLO_FC6_MUL(COS_27_64, (s[13] - s[18]));
        p14 = FLO_FC6_MUL(COS_29_64, (s[14] - s[17]));
        p15 = FLO_FC6_MUL(COS_31_64, (s[15] - s[16]));
    }
#else
    { 
        FLO_Float *s0, *s1;
        
        s0 = filter->input;
        s1 = filter->input + 31;

        p00 = FLO_FC6_MUL(COS_01_64, (*s0++ - *s1--));   
        p01 = FLO_FC6_MUL(COS_03_64, (*s0++ - *s1--));
        p02 = FLO_FC6_MUL(COS_05_64, (*s0++ - *s1--)); 
        p03 = FLO_FC6_MUL(COS_07_64, (*s0++ - *s1--));
        p04 = FLO_FC6_MUL(COS_09_64, (*s0++ - *s1--)); 
        p05 = FLO_FC6_MUL(COS_11_64, (*s0++ - *s1--));
        p06 = FLO_FC6_MUL(COS_13_64, (*s0++ - *s1--)); 
        p07 = FLO_FC6_MUL(COS_15_64, (*s0++ - *s1--));
        p08 = FLO_FC6_MUL(COS_17_64, (*s0++ - *s1--)); 
        p09 = FLO_FC6_MUL(COS_19_64, (*s0++ - *s1--));
        p10 = FLO_FC6_MUL(COS_21_64, (*s0++ - *s1--)); 
        p11 = FLO_FC6_MUL(COS_23_64, (*s0++ - *s1--));
        p12 = FLO_FC6_MUL(COS_25_64, (*s0++ - *s1--)); 
        p13 = FLO_FC6_MUL(COS_27_64, (*s0++ - *s1--));
        p14 = FLO_FC6_MUL(COS_29_64, (*s0++ - *s1--)); 
        p15 = FLO_FC6_MUL(COS_31_64, (*s0   - *s1  ));
    }
#endif

    pp00 = p00 + p15; 
    pp01 = p01 + p14; 
    pp02 = p02 + p13; 
    pp03 = p03 + p12;
    pp04 = p04 + p11; 
    pp05 = p05 + p10; 
    pp06 = p06 + p09;  
    pp07 = p07 + p08;
    pp08 = FLO_FC6_MUL(COS_01_32, (p00 - p15));
    pp09 = FLO_FC6_MUL(COS_03_32, (p01 - p14));
    pp10 = FLO_FC6_MUL(COS_05_32, (p02 - p13));
    pp11 = FLO_FC6_MUL(COS_07_32, (p03 - p12));
    pp12 = FLO_FC6_MUL(COS_09_32, (p04 - p11));
    pp13 = FLO_FC6_MUL(COS_11_32, (p05 - p10));
    pp14 = FLO_FC6_MUL(COS_13_32, (p06 - p09));
    pp15 = FLO_FC6_MUL(COS_15_32, (p07 - p08));

    p00 = pp00 + pp07; 
    p01 = pp01 + pp06; 
    p02 = pp02 + pp05; 
    p03 = pp03 + pp04;
    p04 = FLO_FC6_MUL(COS_01_16, (pp00 - pp07));
    p05 = FLO_FC6_MUL(COS_03_16, (pp01 - pp06));
    p06 = FLO_FC6_MUL(COS_05_16, (pp02 - pp05));
    p07 = FLO_FC6_MUL(COS_07_16, (pp03 - pp04));
    p08 = pp08 + pp15; 
    p09 = pp09 + pp14; 
    p10 = pp10 + pp13; 
    p11 = pp11 + pp12;
    p12 = FLO_FC6_MUL(COS_01_16, (pp08 - pp15));
    p13 = FLO_FC6_MUL(COS_03_16, (pp09 - pp14));
    p14 = FLO_FC6_MUL(COS_05_16, (pp10 - pp13));
    p15 = FLO_FC6_MUL(COS_07_16, (pp11 - pp12));

    pp00 = p00 + p03; 
    pp01 = p01 + p02;
    pp02 = FLO_FC6_MUL(COS_01_08, (p00 - p03));
    pp03 = FLO_FC6_MUL(COS_03_08, (p01 - p02));
    pp04 = p04 + p07; 
    pp05 = p05 + p06;
    pp06 = FLO_FC6_MUL(COS_01_08, (p04 - p07));
    pp07 = FLO_FC6_MUL(COS_03_08, (p05 - p06));
    pp08 = p08 + p11; 
    pp09 = p09 + p10;
    pp10 = FLO_FC6_MUL(COS_01_08, (p08 - p11));
    pp11 = FLO_FC6_MUL(COS_03_08, (p09 - p10));
    pp12 = p12 + p15; 
    pp13 = p13 + p14;
    pp14 = FLO_FC6_MUL(COS_01_08, (p12 - p15));
    pp15 = FLO_FC6_MUL(COS_03_08, (p13 - p14));

    p00 = pp00 + pp01;
    p01 = FLO_FC6_MUL(COS_01_04, (pp00 - pp01));
    p02 = pp02 + pp03;
    p03 = FLO_FC6_MUL(COS_01_04, (pp02 - pp03));
    p04 = pp04 + pp05;
    p05 = FLO_FC6_MUL(COS_01_04, (pp04 - pp05));
    p06 = pp06 + pp07;
    p07 = FLO_FC6_MUL(COS_01_04, (pp06 - pp07));
    p08 = pp08  + pp09;
    p09 = FLO_FC6_MUL(COS_01_04, (pp08 - pp09));
    p10 = pp10 + pp11;
    p11 = FLO_FC6_MUL(COS_01_04, (pp10 - pp11));
    p12 = pp12 + pp13;
    p13 = FLO_FC6_MUL(COS_01_04, (pp12 - pp13));
    p14 = pp14 + pp15;
    p15 = FLO_FC6_MUL(COS_01_04, (pp14 - pp15));

    {
        register FLO_Float tmp;

        tmp   = p13 + p15;
        /*v[ 1]*/ s0[0x010] = p01 + p09 + tmp;
        /*v[ 5]*/ s0[0x050] = p05 + p07 + p11 + tmp;
        tmp  += p09;
        /*v[16]*/ s1[0x010] = -(p01 + p14 + tmp);
        tmp  += p05 + p07;
        /*v[ 3]*/ s0[0x030] = tmp;
        /*v[18]*/ s1[0x030] = -(p06 + p14 + tmp);
        tmp   = p10 + p11 + p12 + p13 + p14 + p15;
        /*v[22]*/ s1[0x070] = -(p02 + p03 + tmp - p12);
        /*v[26]*/ s1[0x0B0] = -(p04 + p06 + p07 + tmp - p13);
        /*v[20]*/ s1[0x050] = -(p05 + p06 + p07 + tmp - p12);
        /*v[24]*/ s1[0x090] = -(p02 + p03 + tmp - p13);
        tmp   = p08 + p12 + p14 + p15;
        /*v[30]*/ s1[0x0F0] = -(p00 + tmp);
        /*v[28]*/ s1[0x0D0] = -(p04 + p06 + p07 + tmp);
        tmp   = p11 + p15;
        /*v[11]*/ s0[0x0B0] = p07  + tmp;
        tmp  += p03;
        /*v[ 9]*/ s0[0x090] = tmp;
        /*v[ 7]*/ s0[0x070] = p13 + tmp;
        /*v[13]*/ s0[0x0D0] = p07 + p15; 
        /*v[15]*/ s0[0x0F0] = p15;       
    }

    /* NOTE: we only keep 17 values per store (instead of 32) because of  */
    /*       the symetry in the value. The symetry will be used also when */
    /*       reading these arrays                                         */
    {

        /*s0[0x000] = v[ 0];  */
        /*s0[0x010] = v[ 1];  */
        /*s0[0x020] = v[ 2];  */
        /*s0[0x030] = v[ 3];  */ 
        /*s0[0x040] = v[ 4];  */
        /*s0[0x050] = v[ 5];  */
        /*s0[0x060] = v[ 6];  */
        /*s0[0x070] = v[ 7];  */
        /*s0[0x080] = v[ 8];  */
        /*s0[0x090] = v[ 9];  */
        /*s0[0x0A0] = v[10];  */
        /*s0[0x0B0] = v[11];  */
        /*s0[0x0C0] = v[12];  */
        /*s0[0x0D0] = v[13];  */
        /*s0[0x0E0] = v[14];  */
        /*s0[0x0F0] = v[15];  */
        s0[0x100] = FLO_ZERO; 
        s1[0x000] = -s0[0x000]; /*-v[0];*/
        /*s1[0x010] = v[16];  */
        /*s1[0x020] = v[17];  */
        /*s1[0x030] = v[18];  */
        /*s1[0x040] = v[19];  */
        /*s1[0x050] = v[20];  */
        /*s1[0x060] = v[21];  */
        /*s1[0x070] = v[22];  */
        /*s1[0x080] = v[23];  */
        /*s1[0x090] = v[24];  */
        /*s1[0x0A0] = v[25];  */
        /*s1[0x0B0] = v[26];  */
        /*s1[0x0C0] = v[27];  */
        /*s1[0x0D0] = v[28];  */
        /*s1[0x0E0] = v[29];  */
        /*s1[0x0F0] = v[30];  */
        /*s1[0x100] = v[31];  */
    }
}

/*----------------------------------------------------------------------
|   FLO_STORE_SAMPLE
|   clip and store a sample in the output buffer 
+---------------------------------------------------------------------*/
#define FLO_STORE_SAMPLE(buffer, sample)                \
{                                                       \
    int out = FLO_FIX_TO_SHORT(sample);                 \
    if (out < -32768) {                                 \
        *buffer = -32768;                               \
    } else if (out > 32767) {                           \
        *buffer = 32767;                                \
    } else {                                            \
        *buffer = (short)out;                           \
    }                                                   \
}

/*----------------------------------------------------------------------
|   FLO_SynthesisFilter_ComputeAndStorePcm
+---------------------------------------------------------------------*/
static void 
FLO_SynthesisFilter_ComputeAndStorePcm(FLO_SynthesisFilter* filter)
{
    register       FLO_Float* v = filter->v;
    register const FLO_Float* d = FLO_SynthesisFilter_D + (16-filter->v_offset);
    short*                    buffer = filter->buffer;   
    int                       i;

    /* compute the first 16 samples */
    for (i = 0; i < 16; i++, d += 32, v += 16) {
        FLO_STORE_SAMPLE(buffer,
              FLO_FC0_MUL(v[ 0], d[ 0]) + 
              FLO_FC0_MUL(v[ 1], d[ 1]) + 
              FLO_FC0_MUL(v[ 2], d[ 2]) + 
              FLO_FC0_MUL(v[ 3], d[ 3]) +
              FLO_FC0_MUL(v[ 4], d[ 4]) + 
              FLO_FC0_MUL(v[ 5], d[ 5]) + 
              FLO_FC0_MUL(v[ 6], d[ 6]) + 
              FLO_FC0_MUL(v[ 7], d[ 7]) +
              FLO_FC0_MUL(v[ 8], d[ 8]) + 
              FLO_FC0_MUL(v[ 9], d[ 9]) + 
              FLO_FC0_MUL(v[10], d[10]) + 
              FLO_FC0_MUL(v[11], d[11]) +
              FLO_FC0_MUL(v[12], d[12]) + 
              FLO_FC0_MUL(v[13], d[13]) + 
              FLO_FC0_MUL(v[14], d[14]) + 
              FLO_FC0_MUL(v[15], d[15]));
        buffer += filter->buffer_increment;
    }

    /* for the second half, there is a phase inversion, so there is a sign */
    /* difference for odd and even runs                                    */
    if (filter->v == filter->v0) {
        /* 17th sample, use the fact that some of the v[] values are FLO_ZERO */
        FLO_STORE_SAMPLE(buffer,
              FLO_FC0_MUL(v[ 1], d[ 1]) + 
              FLO_FC0_MUL(v[ 3], d[ 3]) + 
              FLO_FC0_MUL(v[ 5], d[ 5]) + 
              FLO_FC0_MUL(v[ 7], d[ 7]) +
              FLO_FC0_MUL(v[ 9], d[ 9]) + 
              FLO_FC0_MUL(v[11], d[11]) + 
              FLO_FC0_MUL(v[13], d[13]) + 
              FLO_FC0_MUL(v[15], d[15]));
        buffer += filter->buffer_increment;

        /* do the last 15 samples */
        d += (filter->v_offset<<1) - 48;
        v -= 16;

        for (i = 1; i < 16; i++, d -= 32, v -= 16) {
            FLO_STORE_SAMPLE(buffer,
                  FLO_FC0_MUL(v[ 0], d[15]) - 
                  FLO_FC0_MUL(v[ 1], d[14]) + 
                  FLO_FC0_MUL(v[ 2], d[13]) - 
                  FLO_FC0_MUL(v[ 3], d[12]) +
                  FLO_FC0_MUL(v[ 4], d[11]) - 
                  FLO_FC0_MUL(v[ 5], d[10]) +
                  FLO_FC0_MUL(v[ 6], d[ 9]) - 
                  FLO_FC0_MUL(v[ 7], d[ 8]) +
                  FLO_FC0_MUL(v[ 8], d[ 7]) - 
                  FLO_FC0_MUL(v[ 9], d[ 6]) +
                  FLO_FC0_MUL(v[10], d[ 5]) - 
                  FLO_FC0_MUL(v[11], d[ 4]) +
                  FLO_FC0_MUL(v[12], d[ 3]) - 
                  FLO_FC0_MUL(v[13], d[ 2]) +
                  FLO_FC0_MUL(v[14], d[ 1]) - 
                  FLO_FC0_MUL(v[15], d[ 0]));
            buffer += filter->buffer_increment;
        }
    } else {
        /* 17th sample, use the fact that some of the v[] values are FLO_ZERO */
        FLO_STORE_SAMPLE(buffer,
              FLO_FC0_MUL(v[ 0], d[ 0]) + 
              FLO_FC0_MUL(v[ 2], d[ 2]) + 
              FLO_FC0_MUL(v[ 4], d[ 4]) + 
              FLO_FC0_MUL(v[ 6], d[ 6]) +
              FLO_FC0_MUL(v[ 8], d[ 8]) + 
              FLO_FC0_MUL(v[10], d[10]) + 
              FLO_FC0_MUL(v[12], d[12]) + 
              FLO_FC0_MUL(v[14], d[14]));
        buffer += filter->buffer_increment;
        
        /* do the last 15 samples */
        d += (filter->v_offset<<1) - 48;
        v -= 16;

        for (i = 1; i < 16; i++, d -=32, v -= 16) {
            FLO_STORE_SAMPLE(buffer,
                  FLO_FC0_MUL(v[15], d[ 0]) - 
                  FLO_FC0_MUL(v[14], d[ 1]) +
                  FLO_FC0_MUL(v[13], d[ 2]) - 
                  FLO_FC0_MUL(v[12], d[ 3]) +
                  FLO_FC0_MUL(v[11], d[ 4]) - 
                  FLO_FC0_MUL(v[10], d[ 5]) +
                  FLO_FC0_MUL(v[ 9], d[ 6]) - 
                  FLO_FC0_MUL(v[ 8], d[ 7]) +
                  FLO_FC0_MUL(v[ 7], d[ 8]) - 
                  FLO_FC0_MUL(v[ 6], d[ 9]) +
                  FLO_FC0_MUL(v[ 5], d[10]) - 
                  FLO_FC0_MUL(v[ 4], d[11]) +
                  FLO_FC0_MUL(v[ 3], d[12]) - 
                  FLO_FC0_MUL(v[ 2], d[13]) +
                  FLO_FC0_MUL(v[ 1], d[14]) - 
                  FLO_FC0_MUL(v[ 0], d[15]));
            buffer += filter->buffer_increment;
        }
    }
    filter->buffer = buffer;
}

/*----------------------------------------------------------------------
|   FLO_SynthesisFilter_ComputeAndStorePcm_Subsampled
+---------------------------------------------------------------------*/
static void 
FLO_SynthesisFilter_ComputeAndStorePcm_Subsampled(FLO_SynthesisFilter* filter)
{
    register       FLO_Float* v = filter->v;
    register const FLO_Float* d = FLO_SynthesisFilter_D + (16-filter->v_offset);
    short*                    buffer = filter->buffer;
    int                       mask;
    int                       i;

    /* compute the subsampling mask */
    mask = (1<<filter->subsampling)-1;

    /* compute the first half of the samples */
    for (i = 0; i < 16; i++, d += 32, v += 16) {
        if (i & mask) continue;
        FLO_STORE_SAMPLE(buffer,
              FLO_FC0_MUL(v[ 0], d[ 0]) + 
              FLO_FC0_MUL(v[ 1], d[ 1]) + 
              FLO_FC0_MUL(v[ 2], d[ 2]) + 
              FLO_FC0_MUL(v[ 3], d[ 3]) +
              FLO_FC0_MUL(v[ 4], d[ 4]) + 
              FLO_FC0_MUL(v[ 5], d[ 5]) + 
              FLO_FC0_MUL(v[ 6], d[ 6]) + 
              FLO_FC0_MUL(v[ 7], d[ 7]) +
              FLO_FC0_MUL(v[ 8], d[ 8]) + 
              FLO_FC0_MUL(v[ 9], d[ 9]) + 
              FLO_FC0_MUL(v[10], d[10]) + 
              FLO_FC0_MUL(v[11], d[11]) +
              FLO_FC0_MUL(v[12], d[12]) + 
              FLO_FC0_MUL(v[13], d[13]) + 
              FLO_FC0_MUL(v[14], d[14]) + 
              FLO_FC0_MUL(v[15], d[15]));
        buffer += filter->buffer_increment;
    }

    /* for the second half, there is a phase inversion, so there is a sign */
    /* difference for odd and even runs                                    */
    if (filter->v == filter->v0) {
        /* middle sample, use the fact that some of the v[] values are FLO_ZERO */
        FLO_STORE_SAMPLE(buffer,
              FLO_FC0_MUL(v[ 1], d[ 1]) + 
              FLO_FC0_MUL(v[ 3], d[ 3]) + 
              FLO_FC0_MUL(v[ 5], d[ 5]) + 
              FLO_FC0_MUL(v[ 7], d[ 7]) +
              FLO_FC0_MUL(v[ 9], d[ 9]) + 
              FLO_FC0_MUL(v[11], d[11]) + 
              FLO_FC0_MUL(v[13], d[13]) + 
              FLO_FC0_MUL(v[15], d[15]));
        buffer += filter->buffer_increment;

        /* do the last half of the samples */
        d += (filter->v_offset<<1) - 48;
        v -= 16;

        for (i = 1; i < 16; i++, d -= 32, v -= 16) {
            if (i & mask) continue;
            FLO_STORE_SAMPLE(buffer,
                  FLO_FC0_MUL(v[ 0], d[15]) - 
                  FLO_FC0_MUL(v[ 1], d[14]) + 
                  FLO_FC0_MUL(v[ 2], d[13]) - 
                  FLO_FC0_MUL(v[ 3], d[12]) +
                  FLO_FC0_MUL(v[ 4], d[11]) - 
                  FLO_FC0_MUL(v[ 5], d[10]) +
                  FLO_FC0_MUL(v[ 6], d[ 9]) - 
                  FLO_FC0_MUL(v[ 7], d[ 8]) +
                  FLO_FC0_MUL(v[ 8], d[ 7]) - 
                  FLO_FC0_MUL(v[ 9], d[ 6]) +
                  FLO_FC0_MUL(v[10], d[ 5]) - 
                  FLO_FC0_MUL(v[11], d[ 4]) +
                  FLO_FC0_MUL(v[12], d[ 3]) - 
                  FLO_FC0_MUL(v[13], d[ 2]) +
                  FLO_FC0_MUL(v[14], d[ 1]) - 
                  FLO_FC0_MUL(v[15], d[ 0]));
            buffer += filter->buffer_increment;
        }
    } else {
        /* middle sample, use the fact that some of the v[] values are FLO_ZERO */
        FLO_STORE_SAMPLE(buffer,
              FLO_FC0_MUL(v[ 0], d[ 0]) + 
              FLO_FC0_MUL(v[ 2], d[ 2]) + 
              FLO_FC0_MUL(v[ 4], d[ 4]) + 
              FLO_FC0_MUL(v[ 6], d[ 6]) +
              FLO_FC0_MUL(v[ 8], d[ 8]) + 
              FLO_FC0_MUL(v[10], d[10]) + 
              FLO_FC0_MUL(v[12], d[12]) + 
              FLO_FC0_MUL(v[14], d[14]));
        buffer += filter->buffer_increment;
        
        /* do the last half of the samples */
        d += (filter->v_offset<<1) - 48;
        v -= 16;

        for (i = 1; i < 16; i++, d -=32, v -= 16) {
            if (i & mask) continue;
            FLO_STORE_SAMPLE(buffer,
                  FLO_FC0_MUL(v[15], d[ 0]) - 
                  FLO_FC0_MUL(v[14], d[ 1]) +
                  FLO_FC0_MUL(v[13], d[ 2]) - 
                  FLO_FC0_MUL(v[12], d[ 3]) +
                  FLO_FC0_MUL(v[11], d[ 4]) - 
                  FLO_FC0_MUL(v[10], d[ 5]) +
                  FLO_FC0_MUL(v[ 9], d[ 6]) - 
                  FLO_FC0_MUL(v[ 8], d[ 7]) +
                  FLO_FC0_MUL(v[ 7], d[ 8]) - 
                  FLO_FC0_MUL(v[ 6], d[ 9]) +
                  FLO_FC0_MUL(v[ 5], d[10]) - 
                  FLO_FC0_MUL(v[ 4], d[11]) +
                  FLO_FC0_MUL(v[ 3], d[12]) - 
                  FLO_FC0_MUL(v[ 2], d[13]) +
                  FLO_FC0_MUL(v[ 1], d[14]) - 
                  FLO_FC0_MUL(v[ 0], d[15]));
            buffer += filter->buffer_increment;
        }
    }
    filter->buffer = buffer;
}

/*----------------------------------------------------------------------
|       FLO_SynthesisFilter_NullPcm
+---------------------------------------------------------------------*/
void
FLO_SynthesisFilter_NullPcm(FLO_SynthesisFilter* filter)
{
    if (filter) {
        int mask = 0;
        int i;
        
        if (filter->subsampling) {
            mask = (1<<filter->subsampling)-1;
        } 

        /* fill the samples buffer with silence */
        for (i=0; i<FLO_FILTER_NB_SAMPLES; i++) {
            if (i & mask) continue;
            *filter->buffer = 0;
            filter->buffer += filter->buffer_increment;
        }
    }
}
    
/*----------------------------------------------------------------------
|   FLO_SynthesisFilter_Equalize
+---------------------------------------------------------------------*/
static void
FLO_SynthesisFilter_Equalize(FLO_SynthesisFilter* filter)
{
    int i;

    /* apply the equalizer values to the bands */
    for (i = 0; i < FLO_FILTER_BAND_WIDTH; i++) {
        filter->input[i] = FLO_FC7_MUL(filter->equalizer[i], filter->input[i]);
    }
}

/*----------------------------------------------------------------------
|   FLO_SynthesisFilter_ComputePcm
+---------------------------------------------------------------------*/
void 
FLO_SynthesisFilter_ComputePcm(FLO_SynthesisFilter* filter)
{
    /* if we has set an equalizer, equalize now */
    if (filter->equalizer) FLO_SynthesisFilter_Equalize(filter);

    /* compute the DCT values */
    FLO_SynthesisFilter_Idct(filter);

    /* do the windowing to compute the output samples */
    if (filter->subsampling) {
        FLO_SynthesisFilter_ComputeAndStorePcm_Subsampled(filter);
    } else {
        FLO_SynthesisFilter_ComputeAndStorePcm(filter);
    }

    /* decrement and wrap-around the store offset counter */
    filter->v_offset = (filter->v_offset-1)&0x0F;

    /* flip-flop the current store (alternate between v0 and v1 */
    filter->v = (filter->v == filter->v0 ? filter->v1 : filter->v0);
}

/*-------------------------------------------------------------------------
|   FLO_HybridFilter_Reset
+-------------------------------------------------------------------------*/
void
FLO_HybridFilter_Reset(FLO_HybridFilter *filter)
{
    int i, j;

    for (i=0; i<FLO_HYBRID_NB_BANDS; i++) {
        for (j=0; j<FLO_HYBRID_BAND_WIDTH; j++) {
            filter->in[i][j]    = FLO_ZERO;
            filter->out[j][i]   = FLO_ZERO;
            filter->store[i][j] = FLO_ZERO;
        }
    }
}

/*----------------------------------------------------------------------
|   FLO_DCT_12_PROLOGUE
+---------------------------------------------------------------------*/
#define FLO_DCT_12_PROLOGUE(v, o, t0, t1, t2, t3, t4, t5)           \
     t0  = v[o+ 0];                                                 \
     t1  = v[o+ 0] + v[o+ 3];                                       \
     t2  = v[o+ 3] + v[o+ 6];                                       \
     t3  = v[o+ 6] + v[o+ 9];                                       \
     t4  = v[o+ 9] + v[o+12];                                       \
     t5  = v[o+12] + v[o+15];                                       \
     t5 += t3;    /* t5 = v[o+ 6] + v[o+ 9] + v[o+12] + v[o+15] */  \
     t3 += t1;    /* t3 = v[o+ 0] + v[o+ 3] + v[o+ 6] + v[o+ 9] */  \
     t2  = FLO_FC6_MUL(COS_01_06, t2);                                  \
     t3  = FLO_FC6_MUL(COS_01_06, t3);

/*----------------------------------------------------------------------
|   FLO_DCT_12_EPILOGUE
+---------------------------------------------------------------------*/
#define FLO_DCT_12_EPILOGUE(t0, t1, t2, t3, t4, t5)                 \
     t0 += FLO_FC6_MUL(COS_02_06, t4);                                  \
     t4  = t0 + t2;                                                 \
     t0 -= t2;                                                      \
     t1 += FLO_FC6_MUL(COS_02_06, t5);                                  \
     t5  = FLO_FC6_MUL(COS_01_12, (t1 + t3));                           \
     t1  = FLO_FC6_MUL(COS_05_12, (t1 - t3));                           \
     t3  = t4 + t5;                                                 \
     t4 -= t5;                                                      \
     t2  = t0 + t1;                                                 \
     t0 -= t1;

/*----------------------------------------------------------------------
|   FLO_HybridFilter_Imdct_12
+---------------------------------------------------------------------*/
void
FLO_HybridFilter_Imdct_12(FLO_HybridFilter* filter, int subband)
{
    FLO_Float*       out   = &filter->out[0][subband];
    FLO_Float*       in    = filter->in[subband];
    FLO_Float*       store = filter->store[subband];
    const FLO_Float* window;

    if (subband & 1) {
       window = FLO_LayerIII_ImdctWindows_Odd[2];
    } else {
       window = FLO_LayerIII_ImdctWindows_Even[2];
    }

    {
        FLO_Float t0, t1, t2, t3, t4, t5;

        out[0 * 32] = store[0]; 
        out[1 * 32] = store[1]; 
        out[2 * 32] = store[2];
        out[3 * 32] = store[3]; 
        out[4 * 32] = store[4]; 
        out[5 * 32] = store[5];
 
        FLO_DCT_12_PROLOGUE(in, 0, t0, t1, t2, t3, t4, t5);

        {
            FLO_Float tmp0;
            FLO_Float tmp1 = (t0 - t4);
            {
                FLO_Float tmp2 = FLO_FC6_MUL(COS_03_12, (t1 - t5));
                tmp0 = tmp1 + tmp2;
                tmp1 -= tmp2;
            }
            out[16 * 32] = store[16] + FLO_FC5_MUL(window[10], tmp0);
            out[13 * 32] = store[13] + FLO_FC5_MUL(window[ 7], tmp0);
            out[ 7 * 32] = store[ 7] + FLO_FC5_MUL(window[ 1], tmp1);
            out[10 * 32] = store[10] + FLO_FC5_MUL(window[ 4], tmp1);
        }

        FLO_DCT_12_EPILOGUE(t0, t1, t2, t3, t4, t5);

        out[17 * 32] = store[17] + FLO_FC5_MUL(window[11], t2);
        out[12 * 32] = store[12] + FLO_FC5_MUL(window[ 6], t2);
        out[14 * 32] = store[14] + FLO_FC5_MUL(window[ 8], t3);
        out[15 * 32] = store[15] + FLO_FC5_MUL(window[ 9], t3);
        out[ 6 * 32] = store[ 6] + FLO_FC5_MUL(window[ 0], t0);
        out[11 * 32] = store[11] + FLO_FC5_MUL(window[ 5], t0);
        out[ 8 * 32] = store[ 8] + FLO_FC5_MUL(window[ 2], t4);
        out[ 9 * 32] = store[ 9] + FLO_FC5_MUL(window[ 3], t4);
    }
    {
        FLO_Float t0, t1, t2, t3, t4, t5;
 
        FLO_DCT_12_PROLOGUE(in, 1, t0, t1, t2, t3, t4, t5);

        {
            FLO_Float tmp0;
            FLO_Float tmp1 = (t0 - t4);
            {
                FLO_Float tmp2 = FLO_FC6_MUL(COS_03_12, (t1 - t5));
                tmp0 = tmp1 + tmp2;
                tmp1 -= tmp2;
            }

            store[ 4]     = FLO_FC5_MUL(window[10], tmp0);
            store[ 1]     = FLO_FC5_MUL(window[ 7], tmp0);

            out[13 * 32] += FLO_FC5_MUL(window[1], tmp1);
            out[16 * 32] += FLO_FC5_MUL(window[4], tmp1);
        }

        FLO_DCT_12_EPILOGUE(t0, t1, t2, t3, t4, t5);

        store[ 5]     = FLO_FC5_MUL(window[11], t2);
        store[ 0]     = FLO_FC5_MUL(window[ 6], t2);
        store[ 2]     = FLO_FC5_MUL(window[ 8], t3);
        store[ 3]     = FLO_FC5_MUL(window[ 9], t3);

        out[12 * 32] += FLO_FC5_MUL(window[0], t0);
        out[17 * 32] += FLO_FC5_MUL(window[5], t0);
        out[14 * 32] += FLO_FC5_MUL(window[2], t4);
        out[15 * 32] += FLO_FC5_MUL(window[3], t4);

        /* ------------------------------------------------ */

        store[12] =
        store[13] =
        store[14] = 
        store[15] =
        store[16] = 
        store[17] = FLO_ZERO;

        FLO_DCT_12_PROLOGUE(in, 2, t0, t1, t2, t3, t4, t5);

        {
            FLO_Float tmp0;
            FLO_Float tmp1 = (t0 - t4);
            {
                FLO_Float tmp2 = FLO_FC6_MUL(COS_03_12, (t1 - t5));
                tmp0 = tmp1 + tmp2;
                tmp1 -= tmp2;
            }
            store[10]  = FLO_FC5_MUL(window[10], tmp0);
            store[ 7]  = FLO_FC5_MUL(window[ 7], tmp0);
            store[ 1] += FLO_FC5_MUL(window[ 1], tmp1);
            store[ 4] += FLO_FC5_MUL(window[ 4], tmp1);
        }

        FLO_DCT_12_EPILOGUE(t0, t1, t2, t3, t4, t5);

        store[11]  = FLO_FC5_MUL(window[11], t2);
        store[ 6]  = FLO_FC5_MUL(window[ 6], t2);
        store[ 8]  = FLO_FC5_MUL(window[ 8], t3);
        store[ 9]  = FLO_FC5_MUL(window[ 9], t3);
        store[ 0] += FLO_FC5_MUL(window[ 0], t0);
        store[ 5] += FLO_FC5_MUL(window[ 5], t0);
        store[ 2] += FLO_FC5_MUL(window[ 2], t4);
        store[ 3] += FLO_FC5_MUL(window[ 3], t4);
    }
}

/*-------------------------------------------------------------------------
|   FLO_HybridFilter_Imdct_36
+-------------------------------------------------------------------------*/
void 
FLO_HybridFilter_Imdct_36(FLO_HybridFilter *filter, int subband, int window_type)
{
    FLO_Float *in = filter->in[subband];
    FLO_Float tmp00, tmp01, tmp02, tmp03, tmp04, tmp05, tmp06, tmp07, tmp08;
    FLO_Float tmp09, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17;

    in[17] += in[16]; 
    in[16] += in[15]; 
    in[15] += in[14]; 
    in[14] += in[13];
    in[13] += in[12]; 
    in[12] += in[11]; 
    in[11] += in[10]; 
    in[10] += in[ 9];
    in[ 9] += in[ 8];  
    in[ 8] += in[ 7];  
    in[ 7] += in[ 6];  
    in[ 6] += in[ 5];
    in[ 5] += in[ 4];  
    in[ 4] += in[ 3];  
    in[ 3] += in[ 2];  
    in[ 2] += in[ 1];
    in[ 1] += in[ 0];

    in[17] += in[15]; 
    in[15] += in[13]; 
    in[13] += in[11]; 
    in[11] += in[ 9];
    in[ 9] += in[ 7];  
    in[ 7] += in[ 5];
    in[ 5] += in[ 3];
    in[ 3] += in[ 1];

    /* 9 points IDCT, even indices */    
    {
        FLO_Float t0, t1, t2, t3, t4, t5, t6, t7;

        t1     = FLO_FC6_MUL(COS_02_06, in[12]);
        t2     = FLO_FC6_MUL(COS_02_06, (in[8] + in[16] - in[4]));
        t3     = in[0] + t1;
        t4     = in[0] - t1 - t1;
        t5     = t4 - t2;
        t0     = FLO_FC6_MUL(COS_01_09, (in[4] + in[8]));
        t1     = FLO_FC6_MUL(COS_05_09, (in[8] - in[16]));
        tmp04  = t4 + t2 + t2;
        t2     = FLO_FC6_MUL(COS_07_09, (in[4] + in[16]));
        t6     = t3 - t0 - t2;
        t0    += t3 + t1;
        t3    += t2 - t1;
        t2     = FLO_FC6_MUL(COS_01_18, (in[2]  + in[10]));
        t4     = FLO_FC6_MUL(COS_11_18, (in[10] - in[14]));
        t7     = FLO_FC6_MUL(COS_01_06, in[6]);
        t1     = t2 + t4 + t7;
        tmp00  = t0 + t1;
        tmp08  = t0 - t1;
        t1     = FLO_FC6_MUL(COS_13_18, (in[2] + in[14]));
        t2    += t1 - t7;
        tmp03  = t3 + t2;
        t0     = FLO_FC6_MUL(COS_01_06, (in[10] + in[14] - in[2]));
        tmp05  = t3 - t2;
        t4    -= t1 + t7;
        tmp01  = t5 - t0;
        tmp07  = t5 + t0;
        tmp02  = t6 + t4;
        tmp06  = t6 - t4;
    }

    /* 9 points IDCT, odd indices */    
    {
        FLO_Float t0, t1, t2, t3, t4, t5, t6, t7;

        t1     = FLO_FC6_MUL(COS_02_06, in[13]);
        t2     = FLO_FC6_MUL(COS_02_06, (in[9] + in[17] - in[5]));
        t3     = in[1] + t1;
        t4     = in[1] - t1 - t1;
        t5     = t4 - t2;
        t0     = FLO_FC6_MUL(COS_01_09, (in[5] + in[9]));
        t1     = FLO_FC6_MUL(COS_05_09, (in[9] - in[17]));
        tmp13  = FLO_FC6_MUL(COS_09_36, (t4 + t2 + t2));
        t2     = FLO_FC6_MUL(COS_07_09, (in[5] + in[17]));
        t6     = t3 - t0 - t2;
        t0    += t3 + t1;
        t3    += t2 - t1;
        t2     = FLO_FC6_MUL(COS_01_18, (in[3]  + in[11]));
        t4     = FLO_FC6_MUL(COS_11_18, (in[11] - in[15]));
        t7     = FLO_FC6_MUL(COS_01_06, in[7]);
        t1     = t2 + t4 + t7;
        tmp17  = FLO_FC6_MUL(COS_01_36, (t0 + t1));
        tmp09  = FLO_FC6_MUL(COS_17_36, (t0 - t1));
        t1     = FLO_FC6_MUL(COS_13_18, (in[3] + in[15]));
        t2    += t1 - t7;
        tmp14  = FLO_FC6_MUL(COS_07_36, (t3 + t2));
        t0     = FLO_FC6_MUL(COS_01_06, (in[11] + in[15] - in[3]));
        tmp12  = FLO_FC6_MUL(COS_11_36, (t3 - t2));
        t4    -= t1 + t7;
        tmp16  = FLO_FC6_MUL(COS_03_36, (t5 - t0));
        tmp10  = FLO_FC6_MUL(COS_15_36, (t5 + t0));
        tmp15  = FLO_FC6_MUL(COS_05_36, (t6 + t4));
        tmp11  = FLO_FC6_MUL(COS_13_36, (t6 - t4));
    }
#define FLO_BUTTERFLY(n, a, b)                                       \
    {                                                                \
        register FLO_Float sum, diff;                                \
                                                                     \
        diff = a - b;                                                \
        out[(8-n) * 32] = FLO_FC5_MUL(win[8-n], diff) + store[8-n];  \
        out[(9+n) * 32] = FLO_FC5_MUL(win[9+n], diff) + store[9+n];  \
        sum  = a + b;                                                \
        store[8-n] = FLO_FC5_MUL(win[26-n], sum);                    \
        store[9+n] = FLO_FC5_MUL(win[27+n], sum);                    \
    }

    {
        register const FLO_Float *win;
        register FLO_Float       *out   = &filter->out[0][subband];
        register FLO_Float       *store = filter->store[subband];
        if (subband & 1) {
            win = FLO_LayerIII_ImdctWindows_Odd[window_type];
        } else {
            win = FLO_LayerIII_ImdctWindows_Even[window_type];
        }
        FLO_BUTTERFLY(0, tmp00, tmp17);
        FLO_BUTTERFLY(1, tmp01, tmp16);
        FLO_BUTTERFLY(2, tmp02, tmp15);
        FLO_BUTTERFLY(3, tmp03, tmp14);
        FLO_BUTTERFLY(4, tmp04, tmp13);
        FLO_BUTTERFLY(5, tmp05, tmp12);
        FLO_BUTTERFLY(6, tmp06, tmp11);
        FLO_BUTTERFLY(7, tmp07, tmp10);
        FLO_BUTTERFLY(8, tmp08, tmp09);
    }
}

/*-------------------------------------------------------------------------
|       FLO_ZERO_BAND
+-------------------------------------------------------------------------*/
#define FLO_ZERO_BAND(filter, band, index)                      \
    filter->out[index][band] = filter->store[band][index];      \
    filter->store[band][index] = FLO_ZERO;                      \

/*-------------------------------------------------------------------------
|   FLO_HybridFilter_Imdct_Null
+-------------------------------------------------------------------------*/
void 
FLO_HybridFilter_Imdct_Null(FLO_HybridFilter *filter, int subband)
{
    FLO_ZERO_BAND(filter, subband,  0);
    FLO_ZERO_BAND(filter, subband,  1);
    FLO_ZERO_BAND(filter, subband,  2);
    FLO_ZERO_BAND(filter, subband,  3);
    FLO_ZERO_BAND(filter, subband,  4);
    FLO_ZERO_BAND(filter, subband,  5);
    FLO_ZERO_BAND(filter, subband,  6);
    FLO_ZERO_BAND(filter, subband,  7);
    FLO_ZERO_BAND(filter, subband,  8);
    FLO_ZERO_BAND(filter, subband,  9);
    FLO_ZERO_BAND(filter, subband, 10);
    FLO_ZERO_BAND(filter, subband, 11);
    FLO_ZERO_BAND(filter, subband, 12);
    FLO_ZERO_BAND(filter, subband, 13);
    FLO_ZERO_BAND(filter, subband, 14);
    FLO_ZERO_BAND(filter, subband, 15);
    FLO_ZERO_BAND(filter, subband, 16);
    FLO_ZERO_BAND(filter, subband, 17);
}

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */




