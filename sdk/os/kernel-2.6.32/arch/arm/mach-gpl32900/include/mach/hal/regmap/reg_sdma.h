#ifndef _REG_SDMA_H_
#define _REG_SDMA_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

#define GSDMA_BASE_CH0   IO2_ADDRESS(0x40)
#define GSDMA_BASE_CH1   IO2_ADDRESS(0xA0)

/*P_SDMA_ISR*/
#define SDMA_ISR_LL_ENDMODE (0X1 << 0)             /*end of ll mode*/
#define SDMA_ISR_LL_TRIGGER (0X2 << 0)             /*lld is triggered*/
#define SDMA_ISR_SIDX_END_HBLOCK (0X01 << 2)       /*end of half block*/
#define SDMA_ISR_SIDX_END_BLOCK (0X02 << 2)        /*end of block*/
#define SDMA_ISR_SIDX_END_FRAME (0X04 << 2)        /*end of frame*/
#define SDMA_ISR_SIDX_END_PACKET (0X08 << 2)       /*end of packet*/
#define SDMA_ISR_SIDX_END_INDEXMODE (0X10 << 2)    /*end of index mode*/
#define SDMA_ISR_SERR (0X1 << 7)
#define SDMA_ISR_FIN (0X1 << 8)
#define SDMA_ISR_DIDX_END_HBLOCK (0X01 << 18)      /*end of half block*/
#define SDMA_ISR_DIDX_END_BLOCK (0X02 << 18)       /*end of block*/
#define SDMA_ISR_DIDX_END_FRAME (0X04 << 18)       /*end of frame*/
#define SDMA_ISR_DIDX_END_PACKET (0X08 << 18)      /*end of packet*/
#define SDMA_ISR_DIDX_END_INDEXMODE (0X10 << 18)   /*end of index mode*/
#define SDMA_ISR_DERR (0X1 << 23)

/*P_SDMA_ICR*/
#define SDMA_ICR_CLL_ENDMODE (0X1 << 0)             /*mask end of ll mode*/
#define SDMA_ICR_CLL_TRIGGER (0X2 << 0)             /*mask lld is triggered*/
#define SDMA_ICR_CSIDX_END_HBLOCK (0X01 << 2)       /*mask end of half block*/
#define SDMA_ICR_CSIDX_END_BLOCK (0X02 << 2)        /*mask end of block*/
#define SDMA_ICR_CSIDX_END_FRAME (0X04 << 2)        /*mask end of frame*/
#define SDMA_ICR_CSIDX_END_PACKET (0X08 << 2)       /*mask end of packet*/
#define SDMA_ICR_CSIDX_END_INDEXMODE (0X10 << 2)    /*mask end of index mode*/
#define SDMA_ICR_CSERR (0X1 << 7)
#define SDMA_ICR_CFIN (0X1 << 8)
#define SDMA_ICR_CDIDX_END_HBLOCK (0X01 << 18)      /*mask end of half block*/
#define SDMA_ICR_CDIDX_END_BLOCK (0X02 << 18)       /*mask end of block*/
#define SDMA_ICR_CDIDX_END_FRAME (0X04 << 18)       /*mask end of frame*/
#define SDMA_ICR_CDIDX_END_PACKET (0X08 << 18)      /*mask end of packet*/
#define SDMA_ICR_CDIDX_END_INDEXMODE (0X10 << 18)   /*mask end of index mode*/
#define SDMA_ICR_CDERR (0X1 << 23)

/*P_SDMA0_IMR*/
#define SDMA_IMR_MLL_ENDMODE (0X1 << 0)             /*mask end of ll mode*/
#define SDMA_IMR_MLL_TRIGGER (0X2 << 0)             /*mask lld is triggered*/
#define SDMA_IMR_MSIDX_END_HBLOCK (0X01 << 2)       /*mask end of half block*/
#define SDMA_IMR_MSIDX_END_BLOCK (0X02 << 2)        /*mask end of block*/
#define SDMA_IMR_MSIDX_END_FRAME (0X04 << 2)        /*mask end of frame*/
#define SDMA_IMR_MSIDX_END_PACKET (0X08 << 2)       /*mask end of packet*/
#define SDMA_IMR_MSIDX_END_INDEXMODE (0X10 << 2)    /*mask end of index mode*/
#define SDMA_IMR_MSERR (0X1 << 7)
#define SDMA_IMR_MFIN (0X1 << 8)
#define SDMA_IMR_MDIDX_END_HBLOCK (0X01 << 18)      /*mask end of half block*/
#define SDMA_IMR_MDIDX_END_BLOCK (0X02 << 18)       /*mask end of block*/
#define SDMA_IMR_MDIDX_END_FRAME (0X04 << 18)       /*mask end of frame*/
#define SDMA_IMR_MDIDX_END_PACKET (0X08 << 18)      /*mask end of packet*/
#define SDMA_IMR_MDIDX_END_INDEXMODE (0X10 << 18)   /*mask end of index mode*/
#define SDMA_IMR_MDERR (0X1 << 23)

/*P_SDMA_IRR*/
#define SDMA_IRR_MLL_ENDMODE (0X1 << 0)             /*mask end of ll mode*/
#define SDMA_IRR_MLL_TRIGGER (0X2 << 0)             /*mask lld is triggered*/
#define SDMA_IRR_MSIDX_END_HBLOCK (0X01 << 2)       /*mask end of half block*/
#define SDMA_IRR_MSIDX_END_BLOCK (0X02 << 2)        /*mask end of block*/
#define SDMA_IRR_MSIDX_END_FRAME (0X04 << 2)        /*mask end of frame*/
#define SDMA_IRR_MSIDX_END_PACKET (0X08 << 2)       /*mask end of packet*/
#define SDMA_IRR_MSIDX_END_INDEXMODE (0X10 << 2)    /*mask end of index mode*/
#define SDMA_IRR_MSERR (0X1 << 7)
#define SDMA_IRR_MFIN (0X1 << 8)
#define SDMA_IRR_MDIDX_END_HBLOCK (0X01 << 18)      /*mask end of half block*/
#define SDMA_IRR_MDIDX_END_BLOCK (0X02 << 18)       /*mask end of block*/
#define SDMA_IRR_MDIDX_END_FRAME (0X04 << 18)       /*mask end of frame*/
#define SDMA_IRR_MDIDX_END_PACKET (0X08 << 18)      /*mask end of packet*/
#define SDMA_IRR_MDIDX_END_INDEXMODE (0X10 << 18)   /*mask end of index mode*/
#define SDMA_IRR_MDERR (0X1 << 23)

/*P_SDMA_STATUS*/
#define SDMA_STATUS_CHEN (0X1 << 0)
#define SDMA_STATUS_STOP (0X1 << 1)
#define SDMA_STATUS_PAR (0X1 << 2)
#define SDMA_STATUS_LPI (0X1 << 4)
#define SDMA_STATUS_ACT (0X1 << 16)
#define SDMA_STATUS_PAU (0X1 << 18)
#define SDMA_STATUS_ERR_NOERR (0X0 << 28)
#define SDMA_STATUS_ERR_FIFOOVERFLOW (0X1 << 28)
#define SDMA_STATUS_ERR_FIFOUNDERFLOW (0X2 << 28)
#define SDMA_STATUS_ERR_WRBUSERR (0X3 << 28)
#define SDMA_STATUS_ERR_RDBUSERR (0X4 << 28)

/*P_SDMA_ECFG*/
#define SDMA_ECFG_SMASK_0 (0X1 << 0)
#define SDMA_ECFG_SMASK_1 (0X2 << 0)
#define SDMA_ECFG_SMASK_2 (0X4 << 0)
#define SDMA_ECFG_SMASK_3 (0X8 << 0)
#define SDMA_ECFG_EMASK_0 (0X1 << 4)
#define SDMA_ECFG_EMASK_1 (0X2 << 4)
#define SDMA_ECFG_EMASK_2 (0X4 << 4)
#define SDMA_ECFG_EMASK_3 (0X8 << 4)
#define SDMA_ECFG_COLCPY_DLSB (0X1 << 8)      /*duplicate lsb*/
#define SDMA_ECFG_COLCPY_DMSB (0X2 << 8)      /*duplicate msb*/

/*P_SDMA_CFG*/
#define SDMA_CFG_SSIZE_8BIT (0X0 << 6)
#define SDMA_CFG_SSIZE_16BIT (0X1 << 6)
#define SDMA_CFG_SSIZE_32BIT (0X2 << 6)
#define SDMA_CFG_DSIZE_8BIT (0X0 << 8)
#define SDMA_CFG_DSIZE_16BIT (0X1 << 8)
#define SDMA_CFG_DSIZE_32BIT (0X2 << 8)
#define SDMA_CFG_SBST_1 (0X0 << 10)
#define SDMA_CFG_SBST_4 (0X1 << 10)
#define SDMA_CFG_SBST_8 (0X2 << 10)
#define SDMA_CFG_SBST_16 (0X3 << 10)
#define SDMA_CFG_DBST_1 (0X0 << 12)
#define SDMA_CFG_DBST_4 (0X1 << 12)
#define SDMA_CFG_DBST_8 (0X2 << 12)
#define SDMA_CFG_DBST_16 (0X3 << 12)
#define SDMA_CFG_SPEC_NORMAL (0X0 << 27)
#define SDMA_CFG_SPEC_COLCPY (0X1 << 27)

/*P_SDMA_CTRL*/
#define SDMA_CTRL_EDSW (0X1 << 1)
#define SDMA_CTRL_SID_MEMSTICK (0X1 << 2)
#define SDMA_CTRL_SID_UARTTX (0X3 << 2)
#define SDMA_CTRL_SID_UARTRX (0X4 << 2)
#define SDMA_CTRL_SID_SD0 (0X5 << 2)
#define SDMA_CTRL_SID_SD1 (0X6 << 2)
#define SDMA_CTRL_SID_ADMA1_CH0 (0X8 << 2)
#define SDMA_CTRL_SID_ADMA1_CH1 (0X9 << 2)
#define SDMA_CTRL_SID_SPIRX (0X0A << 2)
#define SDMA_CTRL_SID_SPITX (0X0B << 2)
#define SDMA_CTRL_SID_ADMA0_CH0 (0X0C << 2)
#define SDMA_CTRL_SID_ADMA0_CH1 (0X0D << 2)
#define SDMA_CTRL_SID_ADMA0_CH2 (0X0E << 2)
#define SDMA_CTRL_SID_MEM (0X0F << 2)
#define SDMA_CTRL_DID_MEMSTICK (0X1 << 7)
#define SDMA_CTRL_DID_UARTTX (0X3 << 7)
#define SDMA_CTRL_DID_UARTRX (0X4 << 7)
#define SDMA_CTRL_DID_SD0 (0X5 << 7)
#define SDMA_CTRL_DID_SD1 (0X6 << 7)
#define SDMA_CTRL_DID_ADMA1_CH0 (0X8 << 7)
#define SDMA_CTRL_DID_ADMA1_CH1 (0X9 << 7)
#define SDMA_CTRL_DID_SPIRX (0X0A << 7)
#define SDMA_CTRL_DID_SPITX (0X0B << 7)
#define SDMA_CTRL_DID_ADMA0_CH0 (0X0C << 7)
#define SDMA_CTRL_DID_ADMA0_CH1 (0X0D << 7)
#define SDMA_CTRL_DID_ADMA0_CH2 (0X0E << 7)
#define SDMA_CTRL_DID_MEM (0X0F << 7)
#define SDMA_CTRL_SAM_NOINC (0X0 << 12)     /*non increatment mode*/
#define SDMA_CTRL_SAM_INC (0X1 << 12)       /*increatmen mode*/
#define SDMA_CTRL_SAM_INDEX (0X2 << 12)     /*index mode*/
#define SDMA_CTRL_SAM_CONST (0X3 << 12)     /*const mode*/
#define SDMA_CTRL_SAM_TOE (0X4 << 12)       /*toe mode*/
#define SDMA_CTRL_SAM_LL (0X6 << 12)        /*link list mode*/
#define SDMA_CTRL_DAM_NOINC (0X0 << 15)     /*non increatment mode*/
#define SDMA_CTRL_DAM_INC (0X1 << 15)       /*increatmen mode*/
#define SDMA_CTRL_DAM_INDEX (0X2 << 15)     /*index mode*/

typedef struct sdma_reg_s {
	volatile SINT32 sdma_reg_isr;
	volatile SINT32 sdma_reg_icr;
	volatile SINT32 sdma_reg_imr;
	volatile SINT32 sdma_reg_irr;
	volatile SINT32 sdma_reg_status;
	volatile SINT32 sdma_reg_sum;
	volatile SINT32 sdma_reg_llcnt;
	volatile SINT32 sdma_reg_ecfg;
	volatile SINT32 sdma_reg_cfg;
	volatile SINT32 sdma_reg_ctrl;
	volatile SINT32 sdma_reg_sadr;
	volatile SINT32 sdma_reg_dadr;
	volatile SINT32 sdma_reg_lldadr;
	volatile SINT32 sdma_reg_reserved;
	volatile SINT32 sdma_reg_sbsize;
	volatile SINT32 sdma_reg_dbsize;
	volatile SINT32 sdma_reg_sfsize;
	volatile SINT32 sdma_reg_dfsize;
	volatile SINT32 sdma_reg_spsize;
	volatile SINT32 sdma_reg_dpsize;
	volatile SINT32 sdma_reg_sbstep;
	volatile SINT32 sdma_reg_dbstep;
	volatile SINT32 sdma_reg_sfstep;
	volatile SINT32 sdma_reg_dfstep;
}sdma_reg_t;
#endif