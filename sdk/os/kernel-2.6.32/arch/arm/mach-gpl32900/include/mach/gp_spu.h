/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file    gp_spu.h
 * @brief   Declaration of SPU base driver.
 * @author  George Hsieh
 * @since   2010-11-01
 * @date    2010-11-01
 */
 
#ifndef _GP_SPU_H_
#define _GP_SPU_H_
/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>
//#include "spu_midi_driver.h"
//#include <linux/io.h>
//#include <mach/typedef.h>
#include <mach/gp_chunkmem.h>
/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
#define SPU_ChannelNumber		32
#define MIDI_ChannelNumber	16
#define C_DefaultBeatBase		352
#define C_DefaultTempo			120

#define	STS_MIDI_PLAY				0x0001		// SPU Status
#define STS_MIDI_CALLBACK		0x0100		// MIDI stop call back routine has been initiated
#define STS_MIDI_REPEAT_ON	0x0200		// MIDI play back continuously
#define STS_MIDI_PAUSE_ON		0x0400		// MIDI Pause flag
#define STS_ADPCM_MODE_ON		0x0800		// SPU is at ADPCM mode
#define STS_NORMAL_MODE_ON	0x0000  	// SPU is at PCM mode
#define C_MIDI_Delay				0x00010000	// wait beat count equal to 0x0000

#define C_NoteEvent						0x0000
#define C_BeatCountEvent			0x0001
#define C_PitchBendEvent			0x0002
#define C_ControlEvent				0x0003
#define C_ProgramChangeEvent	0x0004
#define C_TempoEvent					0x0005
#define C_MIDI_EndEvent				0x0006
#define C_LyricEvent					0x0007
#define C_TextEvent						0x0008

// the following definition is for control event
#define C_DataEntryEvent		0x0006
#define C_VolumeEvent				0x0007
#define C_PanEvent					0x000A
#define C_ExpressionEvent		0x000B
#define C_RPN_LSB_Event			0x0064
#define C_RPN_MSB_Event			0x0065

#define MIDI_RING_BUFFER_SIZE	512*8//4096 byte

#define MIXER_SPU_PCM_BUF_NUM	5
#define MIXER_SPU_PCM_ONE_FRAME_LENGTH	1024*16
//---------------------------------------------------------------------

// SPU Hardware Module 
#define GP_SPU_DISABLE                        0
#define GP_SPU_ENABLE                         1
#define GP_SPU_HARDWARE_MODULE                GP_SPU_ENABLE  

#define GP_SPU_LOOP_SIZE						(2*8*1024)//bytes
#define GP_SPU_QUEU_SIZE						6


/*ggg*/
#define C_SPU_PW_FIQ		1
#define C_SPU_BEAT_FIQ	2
#define C_SPU_ENV_FIQ		3
#define C_SPU_FIQ				4
#define C_MAX_FIQ				5				// Don't forget to modify this when new fast interrupt source is added

#define B_SPU_PW_FIQ		0x01
#define B_SPU_BEAT_FIQ	0x02
#define B_SPU_ENV_FIQ		0x04
#define B_SPU_FIQ				0x08

#define	PB_8bit				0
#define PB_16bit			1

#define MIXER_SPU_CH0		26
#define MIXER_SPU_CH1		27
#define MIXER_SPU_CH2		28
#define MIXER_SPU_CH3		29
#define MIXER_SPU_CH4		30
#define MIXER_SPU_CH5		31


//#define	T_InstrumentStartSection_size	128
//#define T_InstrumentPitchTable_size 	128
//#define	T_InstrumentSectionAddr_size	128
//#define T_DrumAddr_size								128
//#define T_Midi_size										1



const UINT32 T_BitEnable[]={
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

const UINT32 T_BitDisable[]={
	~0x00000001, ~0x00000002, ~0x00000004, ~0x00000008,
	~0x00000010, ~0x00000020, ~0x00000040, ~0x00000080,
	~0x00000100, ~0x00000200, ~0x00000400, ~0x00000800,
	~0x00001000, ~0x00002000, ~0x00004000, ~0x00008000,
	~0x00010000, ~0x00020000, ~0x00040000, ~0x00080000,
	~0x00100000, ~0x00200000, ~0x00400000, ~0x00800000,
	~0x01000000, ~0x02000000, ~0x04000000, ~0x08000000,
	~0x10000000, ~0x20000000, ~0x40000000, ~0x80000000
};

#if PB_8bit
const UINT32 T_VarPhaseTableBottom[]={
	15, 16, 17, 18, 19, 21, 22, 23,
	25, 26, 28, 29, 31, 33, 35, 37,
	39, 42, 44, 47, 50, 53, 56, 59,
	63, 66, 70, 75, 79, 84, 89, 94,
	100, 106, 112, 119, 126, 133, 141, 150,
	159, 168, 178, 189, 200, 212, 225, 238,
	252, 267, 283, 300, 318, 337, 357, 378,
	401, 425, 450, 477, 505, 535, 567, 601,
	637, 674, 715, 757, 802, 850, 900, 954,
	1011, 1071, 1135, 1202, 1274, 1349, 1430, 1515,
	1605, 1700, 1801, 1908, 2022, 2142, 2270, 2405,
	2548, 2699, 2860, 3030, 3210, 3401, 3603, 3817,
	4044, 4285, 4540, 4810, 5096, 5399, 5720, 6060,
	6420, 6802, 7206,
//};

//const INT32U T_VarPhaseTable[]={
	D_BASEPHASE, 505, 535, 567, 601, 637, 674, 715,
	757, 802, 850, 900, 954, 1011, 1071, 1135,
	1202, 1274, 1349, 1430, 1515, 1605, 1700, 1801,
	1908, 2022, 2142, 2270, 2405, 2548, 2699, 2860,
	3030, 3210, 3401, 3603, 3817, 4044, 4285, 4540,
	4810, 5096, 5399, 5720, 6060, 6420, 6802, 7207,
	7635, 8089, 8570, 9080, 9620, 10192, 10798, 11440,
	12121, 12841, 13605, 14414, 15271, 16179, 17141, 18161,
	19240, 20385, 21597, 22881, 24242, 25683, 27210, 28828,
	30543, 32359, 34283, 36322, 38481, 40770, 43194, 45762,
	48484, 51367, 54421, 57657, 61086, 64718
};
#define T_VarPhaseTable T_VarPhaseTableBottom + 107
#endif

#if PB_16bit
const UINT32 T_VarPhaseTableBottom[]={		//for step 65536
	//2^12 * 2^19 / (281250)
	//2^12 * 2^19 / SERVICE_RATE
	252, 267, 283, 300, 318, 337, 357, 378 ,
	401, 425, 450, 477, 505, 535, 567, 601,
	637, 674, 715, 757, 802, 850, 900, 954,
	1011, 1071, 1135, 1202, 1274, 1349, 1430, 1515,
	1605, 1700, 1801, 1908, 2022, 2142, 2270, 2405,
	2548, 2699, 2860, 3030, 3210, 3401, 3603, 3817,
	4044, 4285, 4540, 4810, 5096, 5399, 5720, 6060,
	6420, 6802, 7206, 7635, 8089, 8570, 9080, 9620,
	10192, 10798, 11440, 12120, 12841, 13604, 14413, 15270,
	16179, 17141, 18160, 19240, 20384, 21596, 22880, 24241,
	25682, 27209, 28827, 30541, 32358, 34282, 36320, 38480,
	40768, 43192, 45761, 48482, 51365, 54419, 57655, 61083,
	64716, 68564, 72641, 76960, 81537, 86385, 91522, 96964,
	102730, 108839, 115311,
//};

//const FP32 T_VarPhaseTable[]={
//	65536*2^19/281250*2^(0/12),
//	65536*2^19/281250*2^(1/12),
	122167, 129432, 137128, 145283, 153921, 163074, 172771, 183045,
	193929, 205461, 217678, 230622, 244335, 258864, 274257, 290566,
	307843, 326149, 345543, 366090, 387859, 410922, 435357, 461244,
	488671, 517729, 548515, 581132, 615687, 652298, 691086, 732180,
	775718, 821844, 870714, 922489, 977343, 1035459, 1097031, 1162264,
	1231375, 1304597, 1382172, 1464360, 1551436, 1643689, 1741428, 1844979,
	1954687, 2070919, 2194062, 2324528, 2462751, 2609194, 2764345, 2928721,
	3102872, 3287379, 3482856, 3689958, 3909374, 4141838, 4388124, 4649056,
	4925503, 5218389, 5528690, 5857443, 6205745, 6574758, 6965713, 7379916,
	7818749, 8283676, 8776249, 9298112, 9851006, 10436778, 11057381, 11714887,
	12411490, 13149516, 13931427, 14759833, 15637498, 16567352
};
/*
const INT32U T_VarPhaseTableBottom[]={		//for step 65536
	//2^12 * 2^19 / (281250)
	//2^12 * 2^19 / SERVICE_RATE
	252.80, 267.83, 283.76, 300.63, 318.51, 337.44, 357.51, 378.77 ,
	401.29, 425.15, 450.43, 477.22, 505.60, 535.66, 567.51, 601.26,
	637.01, 674.89, 715.02, 757.54, 802.58, 850.31, 900.87, 954.44,
	1011.19, 1071.32, 1135.02, 1202.52, 1274.02, 1349.78, 1430.04, 1515.07,
	1605.17, 1700.61, 1801.74, 1908.87, 2022.38, 2142.64, 2270.05, 2405.03,
	2548.04, 2699.56, 2860.08, 3030.15, 3210.33, 3401.23, 3603.47, 3817.75,
	4044.76, 4285.28, 4540.09, 4810.06, 5096.08, 5399.11, 5720.16, 6060.30,
	6420.66, 6802.45, 7206.95, 7635.50, 8089.53, 8570.56, 9080.19, 9620.12,
	10192.17, 10798.22, 11440.32, 12120.60, 12841.32, 13604.91, 14413.90, 15270.99,
	16179.06, 17141.11, 18160.38, 19240.25, 20384.33, 21596.45, 22880.64, 24241.19,
	25682.65, 27209.82, 28827.80, 30541.99, 32358.11, 34282.22, 36320.75, 38480.50,
	40768.67, 43192.90, 45761.28, 48482.39, 51365.30, 54419.64, 57655.60, 61083.98,
	64716.22, 68564.45, 72641.50, 76960.99, 81537.33, 86385.79, 91522.56, 96964.77,
	102730.60, 108839.28, 115311.20,
//};

//const FP32 T_VarPhaseTable[]={
//	65536*2^19/281250*2^(0/12),
//	65536*2^19/281250*2^(1/12),
	122167.9586, 129432.4435, 137128.8971, 145283.0056, 153921.9827, 163074.6601, 172771.584, 183045.117,
	193929.5461, 205461.197, 217678.5555, 230622.396, 244335.9173, 258864.887, 274257.7942, 290566.0113,
	307843.9654, 326149.3202, 345543.168, 366090.234, 387859.0921, 410922.3939, 435357.111, 461244.792,
	488671.8346, 517729.774, 548515.5884, 581132.0226, 615687.9309, 652298.6404, 691086.336, 732180.468,
	775718.1843, 821844.7878, 870714.222, 922489.584, 977343.6691, 1035459.548, 1097031.177, 1162264.045,
	1231375.862, 1304597.281, 1382172.672, 1464360.936, 1551436.369, 1643689.576, 1741428.444, 1844979.168,
	1954687.338, 2070919.096, 2194062.354, 2324528.09, 2462751.723, 2609194.562, 2764345.344, 2928721.872,
	3102872.737, 3287379.151, 3482856.888, 3689958.336, 3909374.677, 4141838.192, 4388124.707, 4649056.181,
	4925503.447, 5218389.123, 5528690.688, 5857443.744, 6205745.474, 6574758.303, 6965713.776, 7379916.672,
	7818749.353, 8283676.384, 8776249.414, 9298112.361, 9851006.894, 10436778.25, 11057381.38, 11714887.49,
	12411490.95, 13149516.61, 13931427.55, 14759833.34, 15637498.71, 16567352.77
};
*/
#define T_VarPhaseTable T_VarPhaseTableBottom + 107
#endif

const UINT32 T_TempoRefTable[]={
	36045, 35166, 34328, 33530, 32768, 32040, 31343, 30676,
	30037, 29424, 28836, 28270, 27727, 27204, 26700, 26214,
	25746, 25295, 24858, 24437, 24030, 23636, 23255, 22886,
	22528, 22181, 21845, 21519, 21203, 20896, 20597, 20307,
	20025, 19751, 19484, 19224, 18971, 18725, 18485, 18251,
	18022, 17800, 17583, 17371, 17164, 16962, 16765, 16572,
	16384, 16200, 16020, 15844, 15672, 15503, 15338, 15177,
	15019, 14864, 14712, 14564, 14418, 14275, 14135, 13998,
	13863, 13731, 13602, 13475, 13350, 13227, 13107, 12989,
	12873, 12759, 12647, 12537, 12429, 12323, 12219, 12116,
	12015, 11916, 11818, 11722, 11627, 11534, 11443, 11353,
	11264, 11177, 11091, 11006, 10923, 10841, 10760, 10680,
	10601, 10524, 10448, 10373, 10299, 10225, 10153, 10082,
	10012,  9943,  9875,  9808,  9742,  9676,  9612,  9548,
	 9485,  9423,  9362,  9302,  9242,  9183,  9125,  9068,
	 9011,  8955,  8900,  8845,  8791,  8738,  8685,  8633,
	 8582,  8531,  8481,  8432,  8383,  8334,  8286,  8239,
	 8192,  8146,  8100,  8055,  8010,  7966,  7922,  7879,
	 7836,  7793,  7752,  7710,  7669,  7629,  7588,  7549,
	 7509,  7470,  7432,  7394,  7356,  7319,  7282,  7245,
	 7209,  7173,  7138,  7102,  7068,  7033,  6999,  6965,
	 6932,  6899,  6866,  6833,  6801,  6769,  6737,  6706,
	 6675,  6644,  6614,  6584,  6554,  6524,  6495,  6465,
	 6437,  6408,  6380,  6352,  6324,  6296,  6269,  6242,
	 6215,  6188,  6162,  6135,  6109,  6084,  6058,  6033,
	 6007
};

const UINT32 T_TempoDivide[]={
	102, 100, 98, 95, 93, 91, 89, 87,
	85, 84, 82, 80, 79, 77, 76, 74,
	73, 72, 71, 69, 68, 67, 66, 65,
	64, 63, 62, 61, 60, 59, 59, 58,
	57, 56, 55, 55, 54, 53, 53, 52,
	51, 51, 50, 49, 49, 48, 48, 47,
	47, 46, 46, 45, 45, 44, 44, 43,
	43, 42, 42, 41, 41, 41, 40, 40,
	39, 39, 39, 38, 38, 38, 37, 37,
	37, 36, 36, 36, 35, 35, 35, 34,
	34, 34, 34, 33, 33, 33, 33, 32,
	32, 32, 32, 31, 31, 31, 31, 30,
	30, 30, 30, 29, 29, 29, 29, 29,
	28, 28, 28, 28, 28, 27, 27, 27,
	27, 27, 27, 26, 26, 26, 26, 26,
	26, 25, 25, 25, 25, 25, 25, 25,
	24, 24, 24, 24, 24, 24, 24, 23,
	23, 23, 23, 23, 23, 23, 23, 22,
	22, 22, 22, 22, 22, 22, 22, 21,
	21, 21, 21, 21, 21, 21, 21, 21,
	20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 17, 17, 17, 17, 17,
	17
};
    
//*****************************************************************************
//							Pitch Bend Table
//*****************************************************************************
#define D_PITCH_BEND_TABLE_TOTAL	 0x000D


const UINT32 T_PicthWheelTable[]={
	15464, 15478, 15492, 15506, 15520, 15534, 15548, 15562,
	15576, 15590, 15604, 15618, 15632, 15646, 15661, 15675,
	15689, 15703, 15717, 15731, 15746, 15760, 15774, 15788,
	15803, 15817, 15831, 15845, 15860, 15874, 15888, 15903,
	15917, 15931, 15946, 15960, 15975, 15989, 16004, 16018,
	16032, 16047, 16061, 16076, 16090, 16105, 16119, 16134,
	16149, 16163, 16178, 16192, 16207, 16222, 16236, 16251,
	16266, 16280, 16295, 16310, 16324, 16339, 16354, 16369,
	16384, 16398, 16413, 16428, 16443, 16458, 16472, 16487,
	16502, 16517, 16532, 16547, 16562, 16577, 16592, 16607,
	16622, 16637, 16652, 16667, 16682, 16697, 16712, 16727,
	16742, 16757, 16773, 16788, 16803, 16818, 16833, 16848,
	16864, 16879, 16894, 16909, 16925, 16940, 16955, 16970,
	16986, 17001, 17016, 17032, 17047, 17063, 17078, 17093,
	17109, 17124, 17140, 17155, 17171, 17186, 17202, 17217,
	17233, 17248, 17264, 17280, 17295, 17311, 17326, 17342,
	17358
};
//==================================================================

const UINT32 T_PicthWheelTable_TWO[]={
	14596, 14623, 14649, 14676, 14702, 14729, 14755, 14782,
	14809, 14836, 14862, 14889, 14916, 14943, 14970, 14997,
	15024, 15051, 15079, 15106, 15133, 15160, 15188, 15215,
	15243, 15270, 15298, 15325, 15353, 15381, 15409, 15437,
	15464, 15492, 15520, 15548, 15576, 15605, 15633, 15661,
	15689, 15718, 15746, 15775, 15803, 15832, 15860, 15889,
	15918, 15946, 15975, 16004, 16033, 16062, 16091, 16120,
	16149, 16178, 16208, 16237, 16266, 16296, 16325, 16354,
	16384, 16414, 16443, 16473, 16503, 16533, 16562, 16592,
	16622, 16652, 16682, 16713, 16743, 16773, 16803, 16834,
	16864, 16895, 16925, 16956, 16986, 17017, 17048, 17079,
	17109, 17140, 17171, 17202, 17233, 17264, 17296, 17327,
	17358, 17390, 17421, 17452, 17484, 17516, 17547, 17579,
	17611, 17643, 17674, 17706, 17738, 17770, 17802, 17835,
	17867, 17899, 17931, 17964, 17996, 18029, 18061, 18094,
	18127, 18160, 18192, 18225, 18258, 18291, 18324, 18357,
	18390
};
//==================================================================
const UINT32 T_PicthWheelTable_THREE[]={
	13777, 13815, 13852, 13890, 13927, 13965, 14003, 14041,
	14079, 14117, 14155, 14194, 14232, 14271, 14310, 14348,
	14387, 14426, 14465, 14505, 14544, 14583, 14623, 14663,
	14702, 14742, 14782, 14822, 14862, 14903, 14943, 14984,
	15024, 15065, 15106, 15147, 15188, 15229, 15270, 15312,
	15353, 15395, 15437, 15478, 15520, 15562, 15605, 15647,
	15689, 15732, 15775, 15817, 15860, 15903, 15946, 15990,
	16033, 16076, 16120, 16164, 16208, 16251, 16296, 16340,
	16384, 16428, 16473, 16518, 16562, 16607, 16652, 16697,
	16743, 16788, 16834, 16879, 16925, 16971, 17017, 17063,
	17109, 17156, 17202, 17249, 17296, 17343, 17390, 17437,
	17484, 17531, 17579, 17627, 17674, 17722, 17770, 17819,
	17867, 17915, 17964, 18013, 18061, 18110, 18160, 18209,
	18258, 18308, 18357, 18407, 18457, 18507, 18557, 18607,
	18658, 18708, 18759, 18810, 18861, 18912, 18963, 19015,
	19066, 19118, 19170, 19222, 19274, 19326, 19379, 19431,
	19484
};
//==================================================================

const UINT32 T_PicthWheelTable_FOUR[]={
	13004, 13051, 13098, 13146, 13193, 13241, 13289, 13337,
	13385, 13433, 13482, 13531, 13580, 13629, 13678, 13728,
	13777, 13827, 13877, 13927, 13978, 14028, 14079, 14130,
	14181, 14232, 14284, 14335, 14387, 14439, 14491, 14544,
	14596, 14649, 14702, 14755, 14809, 14862, 14916, 14970,
	15024, 15079, 15133, 15188, 15243, 15298, 15353, 15409,
	15464, 15520, 15576, 15633, 15689, 15746, 15803, 15860,
	15918, 15975, 16033, 16091, 16149, 16208, 16266, 16325,
	16384, 16443, 16503, 16562, 16622, 16682, 16743, 16803,
	16864, 16925, 16986, 17048, 17109, 17171, 17233, 17296,
	17358, 17421, 17484, 17547, 17611, 17674, 17738, 17802,
	17867, 17931, 17996, 18061, 18127, 18192, 18258, 18324,
	18390, 18457, 18524, 18591, 18658, 18725, 18793, 18861,
	18929, 18998, 19066, 19135, 19205, 19274, 19344, 19414,
	19484, 19554, 19625, 19696, 19767, 19839, 19911, 19983,
	20055, 20127, 20200, 20273, 20347, 20420, 20494, 20568,
	20643
};
//==================================================================

const UINT32 T_PicthWheelTable_FIVE[]={
	12274, 12330, 12385, 12441, 12498, 12554, 12611, 12668,
	12725, 12783, 12841, 12899, 12957, 13016, 13075, 13134,
	13193, 13253, 13313, 13373, 13433, 13494, 13555, 13617,
	13678, 13740, 13802, 13865, 13927, 13990, 14054, 14117,
	14181, 14245, 14310, 14374, 14439, 14505, 14570, 14636,
	14702, 14769, 14836, 14903, 14970, 15038, 15106, 15174,
	15243, 15312, 15381, 15450, 15520, 15591, 15661, 15732,
	15803, 15875, 15946, 16018, 16091, 16164, 16237, 16310,
	16384, 16458, 16533, 16607, 16682, 16758, 16834, 16910,
	16986, 17063, 17140, 17218, 17296, 17374, 17452, 17531,
	17611, 17690, 17770, 17851, 17931, 18013, 18094, 18176,
	18258, 18341, 18424, 18507, 18591, 18675, 18759, 18844,
	18929, 19015, 19101, 19187, 19274, 19361, 19449, 19537,
	19625, 19714, 19803, 19893, 19983, 20073, 20164, 20255,
	20347, 20439, 20531, 20624, 20717, 20811, 20905, 21000,
	21095, 21190, 21286, 21382, 21479, 21576, 21674, 21772,
	21870
};
//==================================================================

const UINT32 T_PicthWheelTable_SIX[]={
	11585, 11648, 11711, 11775, 11839, 11903, 11968, 12033,
	12098, 12164, 12230, 12296, 12363, 12430, 12498, 12566,
	12634, 12702, 12771, 12841, 12910, 12981, 13051, 13122,
	13193, 13265, 13337, 13409, 13482, 13555, 13629, 13703,
	13777, 13852, 13927, 14003, 14079, 14155, 14232, 14310,
	14387, 14465, 14544, 14623, 14702, 14782, 14862, 14943,
	15024, 15106, 15188, 15270, 15353, 15437, 15520, 15605,
	15689, 15775, 15860, 15946, 16033, 16120, 16208, 16296,
	16384, 16473, 16562, 16652, 16743, 16834, 16925, 17017,
	17109, 17202, 17296, 17390, 17484, 17579, 17674, 17770,
	17867, 17964, 18061, 18160, 18258, 18357, 18457, 18557,
	18658, 18759, 18861, 18963, 19066, 19170, 19274, 19379,
	19484, 19590, 19696, 19803, 19911, 20019, 20127, 20237,
	20347, 20457, 20568, 20680, 20792, 20905, 21019, 21133,
	21247, 21363, 21479, 21595, 21713, 21831, 21949, 22068,
	22188, 22309, 22430, 22552, 22674, 22797, 22921, 23045,
	23170
};
//==================================================================

const UINT32 T_PicthWheelTable_SEVEN[]={
	10935, 11004, 11074, 11144, 11215, 11286, 11357, 11429,
	11502, 11575, 11648, 11722, 11796, 11871, 11946, 12022,
	12098, 12175, 12252, 12330, 12408, 12486, 12566, 12645,
	12725, 12806, 12887, 12969, 13051, 13134, 13217, 13301,
	13385, 13470, 13555, 13641, 13728, 13815, 13902, 13990,
	14079, 14168, 14258, 14348, 14439, 14531, 14623, 14716,
	14809, 14903, 14997, 15092, 15188, 15284, 15381, 15478,
	15576, 15675, 15775, 15875, 15975, 16076, 16178, 16281,
	16384, 16488, 16592, 16697, 16803, 16910, 17017, 17125,
	17233, 17343, 17452, 17563, 17674, 17786, 17899, 18013,
	18127, 18242, 18357, 18474, 18591, 18708, 18827, 18946,
	19066, 19187, 19309, 19431, 19554, 19678, 19803, 19929,
	20055, 20182, 20310, 20439, 20568, 20699, 20830, 20962,
	21095, 21228, 21363, 21498, 21634, 21772, 21910, 22048,
	22188, 22329, 22470, 22613, 22756, 22900, 23045, 23191,
	23338, 23486, 23635, 23785, 23936, 24087, 24240, 24394,
	24548
};
//==================================================================

const UINT32 T_PicthWheelTable_EIGHT[]={
	10321, 10396, 10471, 10547, 10624, 10701, 10778, 10856,
	10935, 11014, 11094, 11174, 11255, 11337, 11419, 11502,
	11585, 11669, 11754, 11839, 11925, 12011, 12098, 12186,
	12274, 12363, 12453, 12543, 12634, 12725, 12818, 12910,
	13004, 13098, 13193, 13289, 13385, 13482, 13580, 13678,
	13777, 13877, 13978, 14079, 14181, 14284, 14387, 14491,
	14596, 14702, 14809, 14916, 15024, 15133, 15243, 15353,
	15464, 15576, 15689, 15803, 15918, 16033, 16149, 16266,
	16384, 16503, 16622, 16743, 16864, 16986, 17109, 17233,
	17358, 17484, 17611, 17738, 17867, 17996, 18127, 18258,
	18390, 18524, 18658, 18793, 18929, 19066, 19205, 19344,
	19484, 19625, 19767, 19911, 20055, 20200, 20347, 20494,
	20643, 20792, 20943, 21095, 21247, 21401, 21556, 21713,
	21870, 22028, 22188, 22349, 22511, 22674, 22838, 23004,
	23170, 23338, 23507, 23678, 23849, 24022, 24196, 24372,
	24548, 24726, 24905, 25086, 25268, 25451, 25635, 25821,
	26008
};
//==================================================================
const UINT32 T_PicthWheelTable_NINE[]={
	9742, 9821, 9902, 9982, 10064, 10146, 10229, 10312,
	10396, 10481, 10566, 10653, 10739, 10827, 10915, 11004,
	11094, 11185, 11276, 11368, 11460, 11554, 11648, 11743,
	11839, 11935, 12033, 12131, 12230, 12330, 12430, 12532,
	12634, 12737, 12841, 12945, 13051, 13157, 13265, 13373,
	13482, 13592, 13703, 13815, 13927, 14041, 14155, 14271,
	14387, 14505, 14623, 14742, 14862, 14984, 15106, 15229,
	15353, 15478, 15605, 15732, 15860, 15990, 16120, 16251,
	16384, 16518, 16652, 16788, 16925, 17063, 17202, 17343,
	17484, 17627, 17770, 17915, 18061, 18209, 18357, 18507,
	18658, 18810, 18963, 19118, 19274, 19431, 19590, 19750,
	19911, 20073, 20237, 20402, 20568, 20736, 20905, 21076,
	21247, 21421, 21595, 21772, 21949, 22128, 22309, 22491,
	22674, 22859, 23045, 23233, 23423, 23614, 23806, 24001,
	24196, 24394, 24593, 24793, 24995, 25199, 25405, 25612,
	25821, 26031, 26244, 26458, 26674, 26891, 27110, 27332,
	27554
};
//==================================================================

const UINT32 T_PicthWheelTable_TEN[]={
	9195, 9279, 9363, 9448, 9533, 9620, 9707, 9795,
	9884, 9973, 10064, 10155, 10247, 10340, 10434, 10528,
	10624, 10720, 10817, 10915, 11014, 11114, 11215, 11317,
	11419, 11523, 11627, 11733, 11839, 11946, 12055, 12164,
	12274, 12385, 12498, 12611, 12725, 12841, 12957, 13075,
	13193, 13313, 13433, 13555, 13678, 13802, 13927, 14054,
	14181, 14310, 14439, 14570, 14702, 14836, 14970, 15106,
	15243, 15381, 15520, 15661, 15803, 15946, 16091, 16237,
	16384, 16533, 16682, 16834, 16986, 17140, 17296, 17452,
	17611, 17770, 17931, 18094, 18258, 18424, 18591, 18759,
	18929, 19101, 19274, 19449, 19625, 19803, 19983, 20164,
	20347, 20531, 20717, 20905, 21095, 21286, 21479, 21674,
	21870, 22068, 22268, 22470, 22674, 22880, 23087, 23296,
	23507, 23721, 23936, 24153, 24372, 24593, 24816, 25041,
	25268, 25497, 25728, 25961, 26196, 26434, 26674, 26915,
	27159, 27406, 27654, 27905, 28158, 28413, 28671, 28931,
	29193
};
//==================================================================

const UINT32 T_PicthWheelTable_ELEVEN[]={
	8679, 8766, 8853, 8942, 9031, 9121, 9212, 9304,
	9397, 9490, 9585, 9681, 9777, 9875, 9973, 10073,
	10173, 10275, 10377, 10481, 10585, 10691, 10798, 10905,
	11014, 11124, 11235, 11347, 11460, 11575, 11690, 11807,
	11925, 12044, 12164, 12285, 12408, 12532, 12657, 12783,
	12910, 13039, 13169, 13301, 13433, 13567, 13703, 13840,
	13978, 14117, 14258, 14400, 14544, 14689, 14836, 14984,
	15133, 15284, 15437, 15591, 15746, 15903, 16062, 16222,
	16384, 16547, 16713, 16879, 17048, 17218, 17390, 17563,
	17738, 17915, 18094, 18275, 18457, 18641, 18827, 19015,
	19205, 19396, 19590, 19785, 19983, 20182, 20383, 20587,
	20792, 21000, 21209, 21421, 21634, 21850, 22068, 22288,
	22511, 22735, 22962, 23191, 23423, 23656, 23893, 24131,
	24372, 24615, 24860, 25108, 25359, 25612, 25868, 26126,
	26386, 26650, 26915, 27184, 27455, 27729, 28006, 28285,
	28567, 28852, 29140, 29431, 29725, 30021, 30321, 30623,
	30929
};
//==================================================================

const UINT32 T_PicthWheelTable_TWELVE[]={
	8192, 8281, 8371, 8463, 8555, 8648, 8742, 8837,
	8933, 9031, 9129, 9228, 9329, 9431, 9533, 9637,
	9742, 9848, 9955, 10064, 10173, 10284, 10396, 10509,
	10624, 10739, 10856, 10975, 11094, 11215, 11337, 11460,
	11585, 11711, 11839, 11968, 12098, 12230, 12363, 12498,
	12634, 12771, 12910, 13051, 13193, 13337, 13482, 13629,
	13777, 13927, 14079, 14232, 14387, 14544, 14702, 14862,
	15024, 15188, 15353, 15520, 15689, 15860, 16033, 16208,
	16384, 16562, 16743, 16925, 17109, 17296, 17484, 17674,
	17867, 18061, 18258, 18457, 18658, 18861, 19066, 19274,
	19484, 19696, 19911, 20127, 20347, 20568, 20792, 21019,
	21247, 21479, 21713, 21949, 22188, 22430, 22674, 22921,
	23170, 23423, 23678, 23936, 24196, 24460, 24726, 24995,
	25268, 25543, 25821, 26102, 26386, 26674, 26964, 27258,
	27554, 27855, 28158, 28464, 28774, 29088, 29405, 29725,
	30048, 30376, 30706, 31041, 31379, 31720, 32066, 32415,
	32768
};

const UINT32 T_PicthWheelTable_THIRTEEN[]={
	7732, 7823, 7916, 8009, 8104, 8199, 8296, 8394,
	8493, 8593, 8695, 8797, 8901, 9006, 9113, 9220,
	9329, 9439, 9550, 9663, 9777, 9893, 10009, 10127,
	10247, 10368, 10490, 10614, 10739, 10866, 10994, 11124,
	11255, 11388, 11523, 11659, 11796, 11935, 12076, 12219,
	12363, 12509, 12657, 12806, 12957, 13110, 13265, 13421,
	13580, 13740, 13902, 14066, 14232, 14400, 14570, 14742,
	14916, 15092, 15270, 15450, 15633, 15817, 16004, 16193,
	16384, 16577, 16773, 16971, 17171, 17374, 17579, 17786,
	17996, 18209, 18424, 18641, 18861, 19084, 19309, 19537,
	19767, 20001, 20237, 20476, 20717, 20962, 21209, 21459,
	21713, 21969, 22228, 22491, 22756, 23025, 23296, 23571,
	23849, 24131, 24416, 24704, 24995, 25290, 25589, 25891,
	26196, 26506, 26818, 27135, 27455, 27779, 28107, 28439,
	28774, 29114, 29458, 29805, 30157, 30513, 30873, 31237,
	31606, 31979, 32357, 32738, 33125, 33516, 33911, 34312,
	34716
};


/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/
typedef struct{
	unsigned int ALP_ID[4];
	unsigned int SampleRate;
	unsigned int SampleLength;
	unsigned int LoopStartAddr;
	unsigned int EnvelopStartAddr;
	unsigned int WaveType;
	unsigned int BasePitch;
	unsigned int MaxPitch;
	unsigned int MinPitch;
	unsigned int RampDownClock;
	unsigned int RampDownStep;
	unsigned int Pan;
	unsigned int Velocity;
}ALP_Header_Struct;

typedef struct {
	unsigned char	SPU_pitch;                                
	unsigned int*	SPU_pAddr;    
	unsigned int* SPU_vAddr;                      
	unsigned char	SPU_pan;
	unsigned char	SPU_velocity;                                
	unsigned char SPU_channel;
	unsigned char SPU_drumIdx;
	
	unsigned int SampleRate;
	unsigned int SampleLength;
	unsigned int LoopStartAddr;
	unsigned int EnvelopStartAddr;
	unsigned int WaveType;
	unsigned int BasePitch;
	unsigned int MaxPitch;
	unsigned int MinPitch;
	unsigned int RampDownClock;
	unsigned int RampDownStep;
	unsigned int Pan;
	unsigned int Velocity;
} SPU_MOUDLE_STRUCT, *SPU_MOUDLE_STRUCT_PTR;

typedef struct{
	UINT32 R_MIDI_CH_PAN;
	UINT32 R_MIDI_CH_VOLUME;
	UINT32 R_MIDI_CH_EXPRESSION;
	UINT32 R_MIDI_CH_PitchBend;
	UINT32 R_CH_SMFTrack;			// Record 16 MIDI channel's instrument
	UINT32 R_ChannelInst;			// Instrument Index mapping of Logical Channel in MIDI file
	UINT32 *R_PB_TABLE_Addr;			//PitchBend Table Address
	UINT32 R_RPN_ReceiveFlag;		//RPN Receive Flag
	UINT32 R_RPN_DATA;				//MIDI CH RPN Value
	
}MIDI_ChannelInfoStruct;

typedef struct{
	UINT32 R_NOTE_PITCH;
	UINT32 R_NOTE_VELOCITY;
	UINT32 R_MIDI_ToneLib_PAN;
	UINT32 R_MIDI_ToneLib_Volume;
//	UINT32 R_DUR_Tone;
	UINT32 R_MIDI_CH_MAP;
	UINT32 R_NoteOnHist;			// Log the NoteOn mapping channel to a Circular Queue
	UINT32 R_PB_PhaseRecord;		//Original Channel Phase Value
}SPU_ChannelInfoStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 Pitch;
	UINT32 Velocity;
	UINT32 Duration;
}NoteEventStruct;

typedef struct{
	UINT32 BeatCountValue;
}BeatCountEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 PitchBendValue;
}PitchBendEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 ControlNumber;
	UINT32 ControlValue;
}ControlEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 InstrumentIndex;
}ProgramChangeEventStruct;

typedef struct{
	UINT32 TempoValue;
}TempoEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 LyricWordCount;
	UINT32 Duration;
}LyricEventStruct;

typedef struct{
	UINT32 TextType;
	UINT32 LyricWordCount;
}TextEventStruct;

typedef union{
	NoteEventStruct NoteEvent;
	BeatCountEventStruct BeatCountEvent;
	PitchBendEventStruct PitchBendEvent;
	ControlEventStruct ControlEvent;
	ProgramChangeEventStruct ProgramChangeEvent;
	TempoEventStruct TempoEvent;
	LyricEventStruct LyricEvent;
	TextEventStruct TextEvent;
}MIDI_EventStruct;


typedef struct{//20110412
	UINT32 MIDI_PlayFlag;
	UINT32 MIDI_Tempo;
	UINT8 R_MIDI_Volume;	//0409
	UINT8 R_MIDI_Pan;		//0409
	ALP_Header_Struct ALP_Header;
	MIDI_ChannelInfoStruct MIDI_ChannelInfo[MIDI_ChannelNumber];
	SPU_ChannelInfoStruct SPU_ChannelInfo[SPU_ChannelNumber];
	UINT32 R_DUR_Tone[SPU_ChannelNumber + 1];
	
	UINT32 uiAddr;
	UINT32 uiData;
	UINT32 uiPhaseLow;
	UINT32 uiPhaseMiddle1;
	UINT32 uiPhaseMiddle2;
	UINT32 uiPhaseHigh;
	UINT32 *pTableAddr;
	UINT32 R_channel_original_phase[SPU_ChannelNumber];//0409
	//UINT32 R_Total_Voice;
	//UINT8 *pMIDI_StartAddr;
	UINT8 *pMIDI_DataPointer;
	MIDI_ChannelInfoStruct *pMIDI_ChInfo;
	SPU_ChannelInfoStruct *pSPU_ChInfo;
	//union MIDI_EventStruct MIDI_Event;
	MIDI_EventStruct MIDI_Event;
	UINT32 R_MIDI_EndFlag;
	UINT32 R_CH_NoteOff;
	UINT32 EventIndex;
	//UINT32 R_PlayChannel;
	UINT32 R_CH_OneBeat;
	UINT32 R_MIDI_CH_MASK;
	UINT32 R_SourceMIDIMask;
	//UINT32 R_NoteOnHistPtr;
	//UINT32 R_Avail_Voice;
	//void* (*user_memory_malloc)(INT32U size);
	//void (*user_memory_free)(void* buffer_addr);
	void (*MIDI_StopCallBack)(void);
	void (*MIDI_DataEntryEventCallBack)(void);		//20081028 Roy
	void (*MIDI_PlayDtStopCallBack)(void);			//20081028 Roy
	UINT8	MIDI_Control_Event[4];					
	UINT32	MIDI_Current_Dt;						
	UINT32  MIDI_Stop_Dt;							
	UINT8	MIDI_Skip_Flag;							
	
	UINT8 User_FIQ_Flag;
	void (*SPU_User_FIQ_ISR[C_MAX_FIQ])(void);

	UINT8  SPU_load_data_mode;
	UINT32 total_inst;
	UINT32 total_drum;
	UINT32 idi_offset_addr;
	UINT32 currt_midi_data_offset;
	UINT32 static_midi_offset;
	UINT32 static_midi_length;
	UINT32 remain_midi_length;
	UINT32 midi_ring_buffer_addr;
	UINT32 midi_ring_buffer_ri;
	UINT32 midi_ring_buffer_wi;
	UINT32 static_midi_ring_buffer_ri;
	UINT32 static_midi_ring_buffer_wi;
	UINT32 adpcm_comb_offset_addr;
	UINT32 adpcm_ram_buffer_addr;
	UINT32 flag_malloc_adpcm_ram_buffer;
	UINT32 adpcm_data_temp_buffer_addr;
	UINT32 inst_start_addr;
	UINT32 lib_start_addr;
	UINT32 midi_start_addr;

	UINT32 T_InstrumentStartSection[129];
	UINT32 T_InstrumentPitchTable[500];
	UINT32 *T_InstrumentSectionAddr[500];
	UINT32 T_InstrumentSectionPhyAddr[500];
	UINT32 *T_DrumAddr[128];
	UINT32 *T_DrumPhyAddr[128];

	UINT32 T_InstrumentStartSection_1[129];//
	UINT32 T_InstrumentPitchTable_1[500];//
	UINT32 *T_InstrumentSectionAddr_1[500];//
	
	UINT8 T_channel[SPU_ChannelNumber];
	SINT32 static_fd_idi;
}STRUCT_MIDI_SPU;


typedef struct{//20110503
	UINT32 *pAddr;
	UINT32 phyAddr;
	UINT8 uiPan;
	UINT8 uiVelocity;
	UINT8 uiSPUChannel;
}STRUCT_DRM_SPU;


typedef struct{
	UINT8  mixer_channel;//0/1/2
	UINT8  vol_L, vol_R;
	UINT8  pan_L, pan_R;
	UINT32 pcm_buf_vir_addr_L[MIXER_SPU_PCM_BUF_NUM];
	UINT32 pcm_buf_vir_addr_R[MIXER_SPU_PCM_BUF_NUM];
	UINT32 pcm_buf_phy_addr_L[MIXER_SPU_PCM_BUF_NUM];
	UINT32 pcm_buf_phy_addr_R[MIXER_SPU_PCM_BUF_NUM];
	UINT32 start_buf_index;
	UINT32 loop_buf_index;
	UINT32 phase;
}STRUCT_MIXER_SPU;

typedef struct// gp_mixer
{	
	int  fd_mixer;
	unsigned int  hardware_ch;	
		
	char ch_name[32];					//mixer channle name , for AP break on user mode
	int sample_spec_type;
	int sample_spec_freq;
	
	int  fd_mem;		
	
	int  		iLoopQueuSize;		
	int  		iLoopBufLen;		
	
	int			underrun;
	
	int			wave_play_block;
	int			wave_write_block;
	int			wave_block_write_pos;
	
	unsigned int		l_wavedata_addr;	//left channel , must chuck memory
	unsigned int		r_wavedata_addr;	//right channel ,  must chuck memory

	unsigned int		l_wavedata_phy_addr;	//left channel , chuck memory physical adderss
	unsigned int		r_wavedata_phy_addr;	//right channel , chuck memory  physical adderss
}st_gp_mixer;

typedef struct gp_mixer_cmd
{	
	int  hardware_ch;	
	int  parameter;	
}st_gp_mixer_cmd;


/*
 * ioctl calls that are permitted to the /dev/tv interface, if
 * any of the ppu moudle are enabled.
 */

#define SPU_INIT	               				_IOW('D', 0x00, STRUCT_MIDI_SPU)			  /* Set PPU Hardware Enable*/
#define SPU_MIDI_PLAY							_IOW('D', 0x01, STRUCT_MIDI_SPU)			  /* play Midi */
#define SPU_MIDI_STOP							_IOW('D', 0x02, STRUCT_MIDI_SPU)			  /* stop Midi */
#define SPU_MIDI_PAUSE							_IOW('D', 0x03, STRUCT_MIDI_SPU)			  /* pasue Midi play*/
#define SPU_MIDI_RESUME							_IOW('D', 0x04, STRUCT_MIDI_SPU)			  /* resume Midi play */
#define SPU_MIDI_SET_VOL						_IOW('D', 0x05, STRUCT_MIDI_SPU)			  /* set Midi volume */
#define SPU_PLAY_TONE							_IOW('D', 0x06, STRUCT_MIDI_SPU)			  /* Play tone from a fixed channel*/
#define SPU_PLAY_DRUM							_IOW('D', 0x07, STRUCT_MIDI_SPU)			  /* Play drum from a fixed channel*/
#define SPU_PLAY_SFX							_IOW('D', 0x08, STRUCT_MIDI_SPU)			  /* Play sound effect from a fixed channel*/

#define SPU_SET_CH_VOL							_IOW('D', 0x09, STRUCT_MIDI_SPU)			  
#define SPU_SET_CH_PAN							_IOW('D', 0x0A, STRUCT_MIDI_SPU)			  
#define SPU_SET_BEAT_BASE_CNT					_IOW('D', 0x0B, STRUCT_MIDI_SPU)			  

#define SPU_MIDI_SET_PAN						_IOW('D', 0x0C, STRUCT_MIDI_SPU)				//pan of midi
#define SPU_MIDI_BUF_WR							_IOW('D', 0x0D, STRUCT_MIDI_SPU)				//
#define SPU_MIDI_BUF_RD							_IOW('D', 0x0E, STRUCT_MIDI_SPU)				//

#define SPU_MIDI_SET_SPU_Channel_Mask				_IOW('D', 0x10, STRUCT_MIDI_SPU)
#define SPU_PLAY_DRM						_IOW('D', 0x18, STRUCT_MIDI_SPU)

#define MIXER_SPU_PLAYPCM_INIT_CH							_IOW('D', 0x20, STRUCT_MIXER_SPU)
#define MIXER_SPU_PLAYPCM_SET_VOL							_IOW('D', 0x21, unsigned int)
#define MIXER_SPU_PLAYPCM_SET_PAN							_IOW('D', 0x22, unsigned int)
#define MIXER_SPU_PLAYPCM_SET_CH_STARTADDR					_IOW('D', 0x23, unsigned int)
#define MIXER_SPU_PLAYPCM_SET_CH_LOOPADDR					_IOW('D', 0x24, unsigned int)
#define MIXER_SPU_PLAYPCM_SET_CH_PHASE						_IOW('D', 0x25, unsigned int)
#define MIXER_SPU_PLAYPCM_START								_IOW('D', 0x26, unsigned int)

//----------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------
#define MIXER_CMD_SET_VOL							_IOW('D', 0x8003, st_gp_mixer_cmd)	//0x8303
#define MIXER_CMD_PAUSE								_IOW('D', 0x8004, st_gp_mixer_cmd)
#define MIXER_CMD_RESUME								_IOW('D', 0x8005, st_gp_mixer_cmd)
#define MIXER_CMD_STOP								_IOW('D', 0x8006, st_gp_mixer_cmd)

#define MIXER_NEW_CHANNEL							_IOW('D', 0x9000, st_gp_mixer)	//0x9000
#define MIXER_RELEASE_CHANNEL						_IOW('D', 0x9001, st_gp_mixer)	//0x9001
#define MIXER_GET_READ_POS_CHANNEL					_IOW('D', 0x9002, st_gp_mixer)	//0x9002
#define MIXER_SET_WRITE_POS_CHANNEL				_IOW('D', 0x9003, st_gp_mixer)	//0x9003

#define MIXER_GET_LOOP_BLOCK_CHANNEL				_IOW('D', 0x9004, int)	//0x9003

//channel format 
#define FORMAT_2CH_SS_S16		0	//SIGNED L16,R16,L16,R16...
#define FORMAT_2CH_SS_U16		1	//UNSIGNED L16,R16,L16,R16...
#define FORMAT_2CH_PS_S16		2	//SIGNED lbuffer: L16,L16,L16...  rbuffer:R16,R16,R16...
#define FORMAT_2CH_PS_U16		3	//UNSIGNED lbuffer: L16,L16,L16...  rbuffer:R16,R16,R16...
#define FORMAT_2CH_PM_S16		4	//SIGNED lbuffer: L16,L16,L16...  rbuffer:NULL
#define FORMAT_2CH_PM_U16		5	//UNSIGNED lbuffer: L16,L16,L16...  rbuffer:NULL
//----------------------------------------------------------------------------------------------------------------


/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/******************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 *****************************************************************************/
/**
* @brief	       gp spu initial
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_spu_init(void);

/**
* @brief	       gp spu playtone
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_spu_play_tone(SPU_MOUDLE_STRUCT *spu_register_set );




void SPU_err(void);

int gp_spu_open(struct inode *inode, struct file *filp);
int gp_spu_release(struct inode *inode, struct file *filp);
int gp_spu_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

void Load_ALP_Header(unsigned int *pAddr);
void SPU_StopChannel(UINT32 StopChannel);
void MidiSRAM_Initial(void);
void MidiPlay_Initialize(void);

void F_StopExpiredCH(void);
UINT16 get_a_word(void);
void F_GetSeqCmd(void);

void SPU_MIDI_IRQ(void);
void SPU_MIXER_IRQ(UINT32 fiq_ch_bitmask);
void SPU_MIDI_Service(void);
void SPU_Init(void);
void SPU_MIDI_Play(void);
void SPU_MIDI_Stop(void);
void SPU_MIDI_Pause(void);
void SPU_MIDI_Resume(void);
void SPU_MIDI_Set_MIDI_Volume(void);
void SPU_MIDI_Set_MIDI_Pan(void);
void SPU_pause_channel(UINT8 spu_channel);
void SPU_resume_channel(UINT8 spu_channel);
void SPU_pause_two_channel(UINT8 spu_channel0, UINT8 spu_channel1);
void SPU_resume_two_channel(UINT8 spu_channel0, UINT8 spu_channel1);
void SPU_PlayTone(SPU_MOUDLE_STRUCT *spu_register_set);
UINT32 Calculate_TempPan(UINT32 Pan1, UINT32 Pan2);
UINT32 Calculate_Pan(UINT32 SPU_Ch);
UINT8 find_channel(UINT8 ch);
void delete_channel(UINT8 index);
void insert_channel(UINT8 ch);
UINT8 get_oldest_channel(void);
UINT32 FindEmptyChannel(void);
void F_CheckDuration(void);
void ProcessNoteEvent(void);
void ProcessBeatCountEvent(void);
void ProcessPitchBendEvent(void);
void ProcessControlEvent(void);
void ProcessProgramChangeEvent(void);
void ProcessEndEvent(void);
void ProcessTempoEvent(void);

UINT32 FindEmptyChannel(void);                                                                         
UINT32 Calculate_Pan(UINT32 SPU_Ch);                                                                   
void SPU_PlayDrum(UINT8 uiDrumIndex, UINT32* pAddr, UINT32 phyAddr, UINT8 uiPan, UINT8 uiVelocity, UINT8 uiSPUChannel);
void SPU_PlayNote(UINT8 uiPlayPitch, UINT32* pAddr, UINT32 phyAddr, UINT8 uiPan, UINT8 uiVelocity, UINT8 uiSPUChannel);


#endif /* _GP_TV_H_ */
