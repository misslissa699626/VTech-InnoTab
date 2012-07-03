/*=============================================================================
* Copyright 2002-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_icon.h
*
* DESCRIPTION   : I-CON - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_ICON_H
#define     __MSPROAL_ICON_H

#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if         (2 == MSPROAL_SUPPORT_IP)
/**************************
        I-CON
 **************************/
/* ICON Register */
#define ICON_CTRL_REG(adrs)         ((adrs) - 0x0030)
#define ICON_PC_REG(adrs)           ((adrs) - 0x002E)
#define ICON_SYS_REG(adrs)          ((adrs) - 0x002C)
#define ICON_FLG_REG(adrs)          ((adrs) - 0x0028)
#define ICON_MEM_CTRL_REG(adrs)     ((adrs) - 0x0024)
#define ICON_GEN_REG0(adrs)         ((adrs) - 0x0020)
#define ICON_GEN_REG1(adrs)         ((adrs) - 0x001E)
#define ICON_GEN_REG2(adrs)         ((adrs) - 0x001C)
#define ICON_GEN_REG3(adrs)         ((adrs) - 0x001A)
#define ICON_GEN_REG4(adrs)         ((adrs) - 0x0018)
#define ICON_GEN_REG5(adrs)         ((adrs) - 0x0016)
#define ICON_TIMER_REG(adrs)        ((adrs) - 0x0014)
#define ICON_INST_QUEUE(adrs)       ((adrs) - 0x0010)
#define ICON_GDFIFO(adrs)           ((adrs) - 0x000C)
#define ICON_PBUFF(adrs)            ((adrs) - 0x0008)
#define ICON_VER_REG(adrs)          ((adrs) - 0x0004)

/*--    Control Register            --*/
#define ICON_CTRL_GDIR_CPU_TO_MS    0x0080
#define ICON_CTRL_GRPN_R0           0x0000
#define ICON_CTRL_GRPN_R1           0x0020
#define ICON_CTRL_GRPN_R2           0x0040
#define ICON_CTRL_GRPN_NOWRITE      0x0060
#define ICON_CTRL_PDIR_CPU_TO_MS    0x0010
#define ICON_CTRL_INTC              0x0008
#define ICON_CTRL_INT               0x0004
#define ICON_CTRL_STRT              0x0001

/*--    Program Counter Register    --*/
#define ICON_PC_PRGC_MASK           0x00FF  /* mask PRGC */

/*--    System Register             --*/
#define ICON_SYS_DMAIM1             0x8000
#define ICON_SYS_DMAIM0             0x4000
#define ICON_SYS_DMAE1              0x2000
#define ICON_SYS_DMAE0              0x1000
#define ICON_SYS_DMASL_GD           0x0000
#define ICON_SYS_DMASL_PB           0x0400
#define ICON_SYS_DMASL_DR           0x0800
#define ICON_SYS_DMASL_MASK         0x0c00
#define ICON_SYS_GDSZ_4             0x0000
#define ICON_SYS_GDSZ_16            0x0100
#define ICON_SYS_GDSZ_32            0x0200
#define ICON_SYS_GDSZ_64            0x0300
#define ICON_SYS_GDSZ_MASK          0x0300
#define ICON_SYS_DMACH              0x0080
#define ICON_SYS_PDSZ_4             0x0000
#define ICON_SYS_PDSZ_16            0x0010
#define ICON_SYS_PDSZ_32            0x0020
#define ICON_SYS_PDSZ_64            0x0030
#define ICON_SYS_PDSZ_128           0x0040
#define ICON_SYS_PDSZ_256           0x0050
#define ICON_SYS_PDSZ_MASK          0x0070  /* mask PDSZ */
#define ICON_SYS_INTE               0x0004
#define ICON_SYS_SRST               0x0001

/*--    Flag Register               --*/
#define ICON_FLAG_MIEF              0x8000
#define ICON_FLAG_DMAF1             0x0200
#define ICON_FLAG_DMAF0             0x0100
#define ICON_FLAG_FLG               0x0080
#define ICON_FLAG_HLTF              0x0040
#define ICON_FLAG_ITOF              0x0020
#define ICON_FLAG_STPF              0x0010
#define ICON_FLAG_EXTS_MASK         0x000F  /* mask EXTS */

/*--    Memory Control Register     --*/
#define ICON_MEM_CTRL_GFVD12        0x0C000000
#define ICON_MEM_CTRL_GFVD_MASK     0xFF000000  /* mask GFVD */
#define ICON_MEM_CTRL_PBBC_256      0x00000000
#define ICON_MEM_CTRL_PBBC_1K       0x00200000
#define ICON_MEM_CTRL_PBBC_MASK     0x00300000
#define ICON_MEM_CTRL_PBCR          0x00080000
#define ICON_MEM_CTRL_GFCR          0x00040000
#define ICON_MEM_CTRL_PBFUL         0x00020000
#define ICON_MEM_CTRL_PBEMP         0x00010000
#define ICON_MEM_CTRL_GFDN_MASK     0x00000FFF  /* mask GFDN */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */
#if         (4 == MSPROAL_SUPPORT_IP)
/**************************
        I-CON
 **************************/
/* ICON Register */
#define ICON_CTRL_REG(adrs)         ((adrs) - 0x0030)
#define ICON_PC_REG(adrs)           ((adrs) - 0x002E)
#define ICON_SYS_REG(adrs)          ((adrs) - 0x002C)
#define ICON_FLG_REG(adrs)          ((adrs) - 0x0028)
#define ICON_MEM_CTRL_REG(adrs)     ((adrs) - 0x0024)
#define ICON_GEN_REG0(adrs)         ((adrs) - 0x0020)
#define ICON_GEN_REG1(adrs)         ((adrs) - 0x001E)
#define ICON_GEN_REG2(adrs)         ((adrs) - 0x001C)
#define ICON_GEN_REG3(adrs)         ((adrs) - 0x001A)
#define ICON_GEN_REG4(adrs)         ((adrs) - 0x0018)
#define ICON_GEN_REG5(adrs)         ((adrs) - 0x0016)
#define ICON_TIMER_REG(adrs)        ((adrs) - 0x0014)
#define ICON_INST_QUEUE(adrs)       ((adrs) - 0x0010)
#define ICON_GDFIFO(adrs)           ((adrs) - 0x000C)
#define ICON_PBUFF(adrs)            ((adrs) - 0x0008)
#define ICON_VER_REG(adrs)          ((adrs) - 0x0004)

/*--    Control Register            --*/
#define ICON_CTRL_GDIR_CPU_TO_MS    0x0080
#define ICON_CTRL_GRPN_R0           0x0000
#define ICON_CTRL_GRPN_R1           0x0020
#define ICON_CTRL_GRPN_R2           0x0040
#define ICON_CTRL_GRPN_NOWRITE      0x0060
#define ICON_CTRL_PDIR_CPU_TO_MS    0x0010
#define ICON_CTRL_INTC              0x0008
#define ICON_CTRL_INT               0x0004
#define ICON_CTRL_STRT              0x0001

/*--    Program Counter Register    --*/
#define ICON_PC_PRGC_MASK           0x007F  /* mask PRGC */

/*--    System Register             --*/
#define ICON_SYS_DMAIM1             0x0800
#define ICON_SYS_DMAIM0             0x0400
#define ICON_SYS_DMACH              0x0200
#define ICON_SYS_DMASZ_4            0x0000
#define ICON_SYS_DMASZ_16           0x0040
#define ICON_SYS_DMASZ_32           0x0080
#define ICON_SYS_DMASZ_64           0x00C0
#define ICON_SYS_GDSZ_MASK          0x00C0
#define ICON_SYS_DMASL              0x0020
#define ICON_SYS_DMAE1              0x0010
#define ICON_SYS_DMAE0              0x0008
#define ICON_SYS_INTE               0x0004
#define ICON_SYS_SRST               0x0001

/*--    Flag Register               --*/
#define ICON_FLAG_DMAF1             0x0200
#define ICON_FLAG_DMAF0             0x0100
#define ICON_FLAG_FLG               0x0080
#define ICON_FLAG_HLTF              0x0040
#define ICON_FLAG_ITOF              0x0020
#define ICON_FLAG_STPF              0x0010
#define ICON_FLAG_EXTS_MASK         0x000F  /* mask EXTS */

/*--    Memory Control Register     --*/
#define ICON_MEM_CTRL_GFVD12        0x0C00
#define ICON_MEM_CTRL_GFVD_MASK     0x0F00  /* mask GFVD */
#define ICON_MEM_CTRL_PBCR          0x0008
#define ICON_MEM_CTRL_GFCR          0x0004
#define ICON_MEM_CTRL_PBFUL         0x0002
#define ICON_MEM_CTRL_PBEMP         0x0001
#define ICON_MEM_CTRL_GFDN_MASK     0x0FFF  /* mask GFDN */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */
#if         (5 == MSPROAL_SUPPORT_IP)
/**************************
        I-CON
 **************************/
/* ICON Register */
#define ICON_CTRL_REG(adrs)         ((adrs) - 0x0030)
#define ICON_PC_REG(adrs)           ((adrs) - 0x002E)
#define ICON_SYS_REG(adrs)          ((adrs) - 0x002C)
#define ICON_FLG_REG(adrs)          ((adrs) - 0x0028)
#define ICON_MEM_CTRL_REG(adrs)     ((adrs) - 0x0024)
#define ICON_GEN_REG0(adrs)         ((adrs) - 0x0020)
#define ICON_GEN_REG1(adrs)         ((adrs) - 0x001E)
#define ICON_GEN_REG2(adrs)         ((adrs) - 0x001C)
#define ICON_GEN_REG3(adrs)         ((adrs) - 0x001A)
#define ICON_GEN_REG4(adrs)         ((adrs) - 0x0018)
#define ICON_GEN_REG5(adrs)         ((adrs) - 0x0016)
#define ICON_TIMER_REG(adrs)        ((adrs) - 0x0014)
#define ICON_INST_QUEUE(adrs)       ((adrs) - 0x0010)
#define ICON_GDFIFO(adrs)           ((adrs) - 0x000C)
#define ICON_PBUFF(adrs)            ((adrs) - 0x0008)
#define ICON_VER_REG(adrs)          ((adrs) - 0x0004)
#define ICON_DMA0_TADR_REG(adrs)    ((adrs) + 0x0050)
#define ICON_DMA0_LADR_REG(adrs)    ((adrs) + 0x0054)
#define ICON_DMA0_CNF_REG(adrs)     ((adrs) + 0x0058)
#define ICON_DMA0_STTS_REG(adrs)    ((adrs) + 0x005C)
#define ICON_DMA1_TADR_REG(adrs)    ((adrs) + 0x0060)
#define ICON_DMA1_LADR_REG(adrs)    ((adrs) + 0x0064)
#define ICON_DMA1_CNF_REG(adrs)     ((adrs) + 0x0068)
#define ICON_DMA1_STTS_REG(adrs)    ((adrs) + 0x006C)
#define ICON_PO_REG(adrs)           ((adrs) + 0x0070)

/*--    Control Register            --*/
#define ICON_CTRL_GDIR_CPU_TO_MS    0x0080
#define ICON_CTRL_GRPN_R0           0x0000
#define ICON_CTRL_GRPN_R1           0x0020
#define ICON_CTRL_GRPN_R2           0x0040
#define ICON_CTRL_GRPN_NOWRITE      0x0060
#define ICON_CTRL_PDIR_CPU_TO_MS    0x0010
#define ICON_CTRL_INTC              0x0008
#define ICON_CTRL_INT               0x0004
#define ICON_CTRL_STRT              0x0001

/*--    Program Counter Register    --*/
#define ICON_PC_PRGC_MASK           0x00FF  /* mask PRGC */

/*--    System Register             --*/
#define ICON_SYS_INTE               0x0004
#define ICON_SYS_SRST               0x0001

/*--    Flag Register               --*/
#define ICON_FLAG_MIEF              0x8000
#define ICON_FLAG_CMP               0x1000
#define ICON_FLAG_FLG               0x0080
#define ICON_FLAG_HLTF              0x0040
#define ICON_FLAG_ITOF              0x0020
#define ICON_FLAG_STPF              0x0010
#define ICON_FLAG_EXTS_MASK         0x000F  /* mask EXTS */

/*--    Memory Control Register     --*/
#define ICON_MEM_CTRL_GFVD12        0x0C000000
#define ICON_MEM_CTRL_GFVD_MASK     0xFF000000  /* mask GFVD */
#define ICON_MEM_CTRL_PBBC_256      0x00000000
#define ICON_MEM_CTRL_PBBC_1K       0x00200000
#define ICON_MEM_CTRL_PBBC_MASK     0x00300000
#define ICON_MEM_CTRL_PBCR          0x00080000
#define ICON_MEM_CTRL_GFCR          0x00040000
#define ICON_MEM_CTRL_PBFUL         0x00020000
#define ICON_MEM_CTRL_PBEMP         0x00010000
#define ICON_MEM_CTRL_GFDN_MASK     0x00000FFF  /* mask GFDN */

/*--    DMA Configuration Register  --*/
#define ICON_DMA_CNF_DMAEN          0x80000000
#define ICON_DMA_CNF_LLTEN          0x40000000
#define ICON_DMA_CNF_ADFIX          0x20000000
#define ICON_DMA_CNF_BSZ_4          0x00000000
#define ICON_DMA_CNF_BSZ_16         0x00030000
#define ICON_DMA_CNF_BSZ_32         0x00050000
#define ICON_DMA_CNF_BSZ_64         0x00070000
#define ICON_DMA_CNF_BSZ_MASK       0x00070000  /* mask BSZ */
#define ICON_DMA_CNF_TRCNT_MASK     0x0000FFFF  /* mask TRCNT */

/*--    DMA Status Register         --*/
#define ICON_DMA_STTS_ACT           0x80000000
#define ICON_DMA_STTS_FSTOP         0x08000000
#define ICON_DMA_STTS_OVFL          0x04000000
#define ICON_DMA_STTS_TRLCNT        0x0001FFFF
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

/******************************************************************************
*   DEFINE
******************************************************************************/
/* Prototype Declaration */
extern SINT msproal_icon_exec_mc(MSIFHNDL *, SINT);
extern SINT msproal_icon_load_mc(MSIFHNDL *, SINT, SINT, UWORD *);
extern SINT msproal_icon_reset_icon(MSIFHNDL *);
extern SINT msproal_icon_wait_int(MSIFHNDL *, SINT, SINT);
#if         (5 == MSPROAL_SUPPORT_IP)
extern void msproal_icon_end_dma(MSIFHNDL *);
extern SINT msproal_icon_start_dma(MSIFHNDL *, ULONG *, SINT);
extern SINT msproal_icon_write_pout_reg(MSIFHNDL *, ULONG, ULONG);
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#ifdef __cplusplus
}
#endif

#endif  /* __MSPROAL_ICON_H */
