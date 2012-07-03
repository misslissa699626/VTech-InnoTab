#include <asm/io.h>
#include "SpuReg.h"

unsigned int spu_ioremap_ptr(void);		//defined in spu.c

//#define SPUBASE 0xD0400E00
#define SPUBASE 0xBE00				//for VPad

#define	P_SPU_ChEnL 	(volatile unsigned int*)(SPUBASE + 0x00000000 + spu_ioremap_ptr()) // Channel Enable [15:0]                                                
#define	P_SPU_ChEnH 	(volatile unsigned int*)(SPUBASE + 0x00000080 + spu_ioremap_ptr()) // Channel Enable [31:16]                                                

//#define SPUCHBASE (volatile unsigned*)0xD0401000 // for ch00~31
#define SPUCHBASE 		(0xC000) 			// for ch00~31 (for VPad)
#define P_SPUCHBASE 	(volatile unsigned int*)(SPUCHBASE + spu_ioremap_ptr())
#define P_SPU_CH_Phase 	(volatile unsigned int*)(SPUCHBASE + 0x00000800 + spu_ioremap_ptr()) // for ch00~31                                     

unsigned Spu_getChannelMaskWord(unsigned channel) {
	return 0x1 << channel;
}

void Spu_getChannelMaskWordByRange(unsigned start_idx, unsigned end_idx, unsigned* channel_maskLH) {
	*channel_maskLH = 0xffffffff << start_idx;
	if (end_idx + 1 < 32) {
		*channel_maskLH &= ~(0xffffffff << (end_idx + 1));
	}
}

unsigned Spu_getControlRegL(SpuControlReg reg) {
	volatile unsigned* temp = 0;
	
	temp  = P_SPU_ChEnL;
	temp += reg;
	//return *temp & 0x0000FFFF;
	return ioread32(temp) & 0x0000FFFF;
}

unsigned Spu_getControlRegH(SpuControlReg reg) {
	volatile unsigned* temp = 0;
	
	temp  = P_SPU_ChEnH;
	temp += reg;
	//return *temp & 0x0000FFFF;
	return ioread32(temp) & 0x0000FFFF;
}

unsigned Spu_getControlRegLH(SpuControlReg reg) {
	return	Spu_getControlRegL(reg) | (Spu_getControlRegH(reg) << 16);	
}

unsigned Spu_getChannelControlReg(unsigned channel, SpuChannelControlReg reg) {
	volatile unsigned* temp = 0;
	
	temp = P_SPUCHBASE;
	temp += channel * 16;
	temp += reg;
	//return *temp;
	return ioread32(temp);		
}

unsigned Spu_getChannelPhaseReg(unsigned channel, SpuChannelPhaseReg reg) {
	volatile unsigned* temp = 0;

	temp  = P_SPU_CH_Phase;
	temp += channel * 16;
	temp += reg;
	//return *temp;
	return ioread32(temp);
}

void Spu_setControlRegL(SpuControlReg reg, unsigned value) {
	volatile unsigned* temp = 0;

	temp  = P_SPU_ChEnL;
	temp += reg;
	//*temp = value & 0x0000FFFF;
	iowrite32(value & 0x0000ffff, temp);		
}

void Spu_setControlRegH(SpuControlReg reg, unsigned value) {
	volatile unsigned* temp = 0;
	
	temp  = P_SPU_ChEnH;
	temp += reg;
	//*temp = value & 0x0000ffff;
	iowrite32(value & 0x0000ffff, temp);		
}

void Spu_setControlRegLH(SpuControlReg reg, unsigned value) {
	Spu_setControlRegL(reg, value);
	Spu_setControlRegH(reg, value >> 16);
}

void Spu_setChannelControlReg(unsigned channel, SpuChannelControlReg reg, unsigned value) {
	volatile unsigned* temp = 0;

	temp = P_SPUCHBASE;
	temp += channel * 16;
	temp += reg;
	//*temp = value;
	iowrite32(value, temp);		
}

void Spu_setChannelPhaseReg(unsigned channel, SpuChannelPhaseReg reg, unsigned value) {
	volatile unsigned* temp = 0;
	
	temp = P_SPU_CH_Phase;
	temp += channel * 16;
	temp += reg;
	//*temp = value;
	iowrite32(value, temp);		
}

void Spu_syncDacOutput(void) {
	volatile unsigned* temp = 0;
	
	temp  = P_SPU_ChEnH;
	temp += 5;
	iowrite32(0x0100, temp);		
}
