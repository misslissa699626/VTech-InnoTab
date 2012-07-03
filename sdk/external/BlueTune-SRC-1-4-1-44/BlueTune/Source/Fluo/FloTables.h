/*****************************************************************
|
|   Fluo - Constant Tables
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Fluo - Constant Tables
 */

#ifndef _FLO_TABLES_H_
#define _FLO_TABLES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "FloTypes.h"
#include "FloMath.h"

/*-------------------------------------------------------------------------
|       types
+-------------------------------------------------------------------------*/
#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

typedef FLO_Float FLO_GroupingTable[][3];

typedef struct {
    int                     code_length;
    const FLO_GroupingTable *grouping;
    FLO_Float               factor;
    int                     offset;   
} FLO_LayerII_QuantInfo;

typedef struct {
    unsigned char               allocation_length;
    FLO_LayerII_QuantInfo const info[16];
} FLO_AllocationTableEntry;

typedef struct {
    int start;
    int width;
    int sample_index;
} FLO_LayerIII_BandInfo;

typedef struct {
    unsigned char length[4];
    unsigned char preflag;
    unsigned char table;
} FLO_LayerIII_ScalefactorLengthInfo;

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

/*-------------------------------------------------------------------------
|       exported tables
+-------------------------------------------------------------------------*/
extern const unsigned short FLO_MpegBitrates[3][3][16];
extern const unsigned       FLO_MpegSamplingFrequencies[3][4];

#if (FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN)

extern const unsigned char FLO_LayerII_SubbandLimits[4][11];
extern const FLO_AllocationTableEntry * const * const FLO_LayerII_AllocationTables[3][11];
extern const FLO_AllocationTableEntry * const FLO_LayerII_Mpeg2_AllocationTable[30];
extern const FLO_Float FLO_LayerI_QuantFactors[15];
extern const FLO_Float FLO_LayerI_QuantOffsets[15];
extern const FLO_Float FLO_Scalefactors[64];
extern const FLO_Float FLO_SynthesisFilter_D[32*17];
extern const int FLO_ScalefactorLengths[16][2];
extern const FLO_LayerIII_BandInfo FLO_SubbandInfo_Short[3][3][13*3+1];
extern const FLO_LayerIII_BandInfo FLO_SubbandInfo_Long[3][3][23];
extern const int FLO_LayerIII_Pretab[22];
extern const int FLO_LayerIII_NullPretab[22];
extern const FLO_Float FLO_Power_4_3[8192 + 15];
extern const FLO_Float * const FLO_GainTable;
extern const FLO_Float FLO_CsTable[8];
extern const FLO_Float FLO_CaTable[8];
extern const FLO_Float * const FLO_LayerIII_ImdctWindows_Even[4];
extern const FLO_Float * const FLO_LayerIII_ImdctWindows_Odd[4];
extern const FLO_LayerIII_ScalefactorLengthInfo FLO_LayerIII_ScalefactorInfo_0[512];
extern const FLO_LayerIII_ScalefactorLengthInfo FLO_LayerIII_ScalefactorInfo_1[256];
extern const int FLO_LayerIII_ScalefactorPartitions[2][3][3][4];

#endif /* FLO_DECODER_ENGINE == FLO_DECODER_ENGINE_BUILTIN */

#endif /* _FLO_TABLES_H_ */
