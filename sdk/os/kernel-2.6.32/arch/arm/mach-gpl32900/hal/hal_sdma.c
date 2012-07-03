#include <mach/hal/hal_sdma.h>
#include <mach/hal/regmap/reg_sdma.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/kernel.h>
#include <mach/hal/hal_clock.h>

#define SDMA_CLK_EN				1
#define SDMA_CLK_DIS				0

static SINT32 gSdmaRegBase[] = {GSDMA_BASE_CH0, GSDMA_BASE_CH1};        /*!<need implement*/

void
gpHalSdmaReset(
	UINT8 indexChan
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];
	
	sdma_reg->sdma_reg_status = 0;
	sdma_reg->sdma_reg_status = SDMA_STATUS_PAR;
	
	while(!gpHalCheckStatus(indexChan, SDMA_STATUS_PAU));
	sdma_reg->sdma_reg_status |= SDMA_STATUS_STOP;
}

void 
gpHalEnble(
	UINT8 indexChan)
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];
	
	gpHalSdmaReset(indexChan);
	sdma_reg->sdma_reg_status = SDMA_STATUS_CHEN;
}

static void 
gpHalSdmaClk(
	UINT32 en
)
{	
	if( en ) {
		gpHalScuClkEnable( SCU_C_PERI_DMAC0 | SCU_C_PERI_DMAC1, SCU_C, 1);
	}
	else{
		gpHalScuClkEnable( SCU_C_PERI_DMAC0, SCU_C, 0);
		gpHalScuClkEnable( SCU_C_PERI_DMAC1, SCU_C, 0);
	}
}


SINT8
gpHalCheckStatus(
	UINT8 indexChan,
	UINT32 statusBit
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];

	return (sdma_reg->sdma_reg_status &= statusBit);
}

SINT8
gpHalCheckIrq(
	UINT8 indexChan,
	UINT32 statusBit
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];

	return (sdma_reg->sdma_reg_irr &= statusBit);
}

SINT32
gpHalGetIrq(
	UINT8 indexChan
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];

	return sdma_reg->sdma_reg_irr;
}

void
gpHalClearIrq(
	UINT8 indexChan
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];

	sdma_reg->sdma_reg_icr = SDMA_ICR_CFIN | SDMA_ICR_CSERR | SDMA_ICR_CDERR
		| SDMA_ICR_CDIDX_END_BLOCK | SDMA_ICR_CDIDX_END_FRAME;
	//sdma_reg->sdma_reg_irr = 0;
	//sdma_reg->sdma_reg_isr = 0;
}

void
gpHalClearBlock(
	UINT8 indexChan)
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];

	sdma_reg->sdma_reg_icr = SDMA_ICR_CDIDX_END_BLOCK;
}

void gpHalDump(
	UINT8 indexChan
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];
	SINT32 i = 0;
	
	#if 0
	printk("channel %d dump infomation\n", indexChan);
	printk("sdma_reg_cfg:%8x\n", sdma_reg->sdma_reg_cfg);
	printk("sdma_reg_status:%8x\n", sdma_reg->sdma_reg_status);
	printk("sdma_reg_irr:%8x\n", sdma_reg->sdma_reg_irr);
	printk("sdma_reg_imr:%8x\n", sdma_reg->sdma_reg_imr);
	printk("sdma_reg_sadr:%8x\n", sdma_reg->sdma_reg_sadr);
	printk("sdma_reg_ctrl:%8x\n", sdma_reg->sdma_reg_ctrl);
	#else
	for (; i < sizeof(sdma_reg_t) / 4; i++) {
		printk("register_%d:%08x\n", i, *(((SINT32*)sdma_reg)+i) );
	}
	#endif
}

SINT32 gpHalGetIrqStatus(
	UINT8 indexChan
)
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];

	return sdma_reg->sdma_reg_irr;
}

static SINT32
gpHalGetMask(
	SINT32 size
)
{
	//SINT8 mask = size % 4;
	SINT32 maskBit;

	switch(size) 
	{
		case 0:
			maskBit = 0;
			break;
		case 1:
			maskBit = SDMA_ECFG_EMASK_3;
			break;
		case 2:
			maskBit = SDMA_ECFG_EMASK_2;
			break;
		case 3:
			maskBit = SDMA_ECFG_EMASK_1;
			break;
		default:
			maskBit = 0;
			break;
	}

	return maskBit;
}

void 
gpHalSdmaInit(
	void
)
{
	SINT32 i;
	
	gpHalSdmaClk(1);
	
	/*register initial*/
	for (i = 0; i < sizeof(sdma_reg_t); i += 4) {
		*((UINT32*)(gSdmaRegBase[0] + i)) = 0;
		*((UINT32*)(gSdmaRegBase[1] + i)) = 0;
	}

}

void 
gpHalSdmaUninit(
	void
)
{	
	gpHalSdmaClk(0);	

	return;
}

void gpHalMaskIrq(UINT8 indexChan) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];
	
	sdma_reg->sdma_reg_imr = SDMA_IMR_MLL_ENDMODE
		| SDMA_IMR_MLL_TRIGGER
		| SDMA_IMR_MSIDX_END_HBLOCK
		| SDMA_IMR_MSIDX_END_BLOCK
		| SDMA_IMR_MSIDX_END_FRAME
		| SDMA_IMR_MSIDX_END_PACKET
		| SDMA_IMR_MSIDX_END_INDEXMODE
		| SDMA_IMR_MSERR
		//| SDMA_IMR_MFIN
		| SDMA_IMR_MDIDX_END_HBLOCK
		| SDMA_IMR_MDIDX_END_BLOCK
		| SDMA_IMR_MDIDX_END_FRAME
		| SDMA_IMR_MDIDX_END_PACKET
		| SDMA_IMR_MDIDX_END_INDEXMODE
		| SDMA_IMR_MDERR;
}

void
gpHalSdmaTrriger(
	UINT8 indexChan, 
	gpSdma_t *pSdma
) 
{
	sdma_reg_t *sdma_reg = (sdma_reg_t *)gSdmaRegBase[indexChan];
	SINT32 size = (pSdma->blockSize + 3) >> 2 << 2;
	
	/*!<set option of the channel[indexChannel]*/
	sdma_reg->sdma_reg_cfg = SDMA_CFG_SBST_16 | SDMA_CFG_DBST_16 
		| SDMA_CFG_SSIZE_32BIT | SDMA_CFG_DSIZE_32BIT;
	if (pSdma->frameSize == 0) {
		sdma_reg->sdma_reg_ctrl = SDMA_CTRL_SAM_INC | SDMA_CTRL_DAM_INC
			| SDMA_CTRL_DID_MEM | SDMA_CTRL_SID_MEM;
	}
	else {
		sdma_reg->sdma_reg_ctrl = SDMA_CTRL_SAM_INDEX | SDMA_CTRL_DAM_INDEX
			| SDMA_CTRL_DID_MEM | SDMA_CTRL_SID_MEM;
	}

	sdma_reg->sdma_reg_dadr = (SINT32)pSdma->dstAddr;
	sdma_reg->sdma_reg_sadr = (SINT32)pSdma->srcAddr;
	sdma_reg->sdma_reg_dbsize = size;
	sdma_reg->sdma_reg_sbsize = size;
	sdma_reg->sdma_reg_ecfg = gpHalGetMask(size - pSdma->blockSize);
	sdma_reg->sdma_reg_sfsize = pSdma->frameSize;
	sdma_reg->sdma_reg_dfsize = pSdma->frameSize;
	sdma_reg->sdma_reg_sbstep = pSdma->bStepSize;
	sdma_reg->sdma_reg_dbstep = pSdma->bStepSize;
	sdma_reg->sdma_reg_sfstep = pSdma->fStepSize;
	sdma_reg->sdma_reg_sfstep = pSdma->fStepSize;
	sdma_reg->sdma_reg_spsize = pSdma->packetSize;
	sdma_reg->sdma_reg_dpsize = pSdma->packetSize;
	
	/*!<set sdma start bit*/
	sdma_reg->sdma_reg_status |= SDMA_STATUS_CHEN;
	
	return;
}