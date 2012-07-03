#ifndef _HAL_SDMA_H_
#define _HAL_SDMA_H_
#include <mach/typedef.h>
#include <mach/kernel.h>

typedef struct gpSdma_s {
	SINT8* srcAddr;		      /*the source address of the data*/
	SINT8* dstAddr;           /*the destination address of the data*/
	SINT32 blockSize;              /*the data size*/
	SINT32 bStepSize;
	SINT32 frameSize;
	SINT32 fStepSize;
	SINT32 packetSize;
	SINT32 pStepSize;
	
	SINT8 useFlag;
} gpSdma_t;

void gpHalSdmaInit(void);
void gpHalSdmaUninit(void);
void gpHalSdmaReset(UINT8 indexChannel);
void gpHalSdmaPause(UINT8 indexChannel);
void gpHalSdmaResume(UINT8 indexChannel);
void gpHalSdmaStop(UINT8 indexChannel);
void gpHalDump(UINT8 indexChan); 
void gpHalClearIrq(UINT8 indexChan);
void gpHalSdmaTrriger(UINT8 indexChan, gpSdma_t *pSdma);
void gpHalClearBlock(UINT8 indexChan);
void gpHalEnble(UINT8 indexChan);
void gpHalMaskIrq(UINT8 indexChan);
SINT32 gpHalSdmaTrigger(UINT8 indexChannel, gpSdma_t* sdma);
SINT8 gpHalCheckStatus( UINT8 indexChan, UINT32 statusBit); 
SINT32 gpHalGetIrq(UINT8 indexChan); 
SINT8 gpHalCheckIrq(UINT8 indexChan, UINT32 statusBit) ;
#endif
