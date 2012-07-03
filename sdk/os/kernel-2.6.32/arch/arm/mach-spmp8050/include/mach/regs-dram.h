#ifndef _DMAC_M320_H_
#define _DMAC_M320_H_

#define DRAM_M320_VIA_BASE	IO2_ADDRESS(0x4000)
#define DRAM_M320_PHY_BASE  0x92004000

//For suspend
#define SP_DRAM_INI_SET	    (DRAM_M320_VIA_BASE+0x08)
#define SP_DRAM_REF_ISSUE	(DRAM_M320_VIA_BASE+0x0C)

#define DRAM_DRAM_TYPE	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x00))
#define DRAM_PRI_TMR	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x04))
#define DRAM_INI_SET	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x08))
#define DRAM_REF_ISSUE	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x0C))
#define DRAM_AC_CONFIG1	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x10))
#define DRAM_AC_CONFIG2	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x14))
#define DRAM_OPT_SET	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x18))
#define DRAM_EMRS_MRS	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x1C))
#define DRAM_EMRS3_MRS2	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x20))
#define DRAM_RD_SPD_DQS_SEL	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x24))
#define DRAM_CAP_RD_SEL	((volatile unsigned *)(DRAM_M320_VIA_BASE+0x28))

#define DRAM_DRAM_TYPE_OFST  0x00
#define DRAM_PRI_TMR_OFST    0x04
#define DRAM_INI_SET_OFST    0x08
#define DRAM_REF_ISSUE_OFST  0x0C
#define DRAM_AC_CONFIG1_OFST 0x10
#define DRAM_AC_CONFIG2_OFST 0x14
#define DRAM_OPT_SET_OFST    0x18
#define DRAM_EMRS_MRS_OFST   0x1C
#define DRAM_EMRS3_MRS2_OFST 0x20
#define DRAM_RD_SPD_DQS_SEL_OFST  0x24
#define DRAM_CAP_RD_SEL_OFST 0x28



#endif

