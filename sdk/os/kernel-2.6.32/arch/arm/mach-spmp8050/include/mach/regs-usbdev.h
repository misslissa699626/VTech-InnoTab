/*
 * arch/arm/mach-spmp8000/include/mach/regs-timer.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Timer - System peripherals regsters.
 *
 */

#define UDC_BASE			IO3_ADDRESS(0x6000)

#define  UDC_DMA_CS	 (*(volatile unsigned *)(UDC_BASE+0x000))
#define  UDC_DMA_DA		(*(volatile unsigned *)(UDC_BASE+0x004))

//#define  UDC_DMA_ND		(*(volatile unsigned *)(UDC_BASE+0x00C))
//#define  UDC_DMA_CDA		(*(volatile unsigned *)(UDC_BASE+0x014))
#define  UDC_CS		(*(volatile unsigned *)(UDC_BASE+0x080))
#define  UDC_IE		(*(volatile unsigned *)(UDC_BASE+0x084))	  
#define  UDC_IF		(*(volatile unsigned *)(UDC_BASE+0x088))	
#define  UDC_CIS    (*(volatile unsigned *)(UDC_BASE+0x08C))


#define  UDC_LLCFS		(*(volatile unsigned *)(UDC_BASE+0x0320))
#define  UDC_MSTC     (*(volatile unsigned *)(UDC_BASE+0x0328))
#define  UDC_EP12C      (*(volatile unsigned *)(UDC_BASE+0x0330))
#define  UDC_EP12PPC  (*(volatile unsigned *)(UDC_BASE+0x0334))
#define  UDC_EP0SCS    (*(volatile unsigned *)(UDC_BASE+0x0344))
#define  UDC_EP0SDP    (*(volatile unsigned *)(UDC_BASE+0x0348))
#define  UDC_EP0CS      (*(volatile unsigned *)(UDC_BASE+0x034C))
#define  UDC_EP0DC      (*(volatile unsigned *)(UDC_BASE+0x0350))
#define  UDC_EP0DP      (*(volatile unsigned *)(UDC_BASE+0x0354))
#define  UDC_EP1SCS    (*(volatile unsigned *)(UDC_BASE+0x0358))
#define  UDC_EP1SDP    (*(volatile unsigned *)(UDC_BASE+0x035C))
#define  UDC_EP12FS    (*(volatile unsigned *)(UDC_BASE+0x0364))
#define  UDC_EP12FCL  (*(volatile unsigned *)(UDC_BASE+0x0368))
#define  UDC_EP12FCH  (*(volatile unsigned *)(UDC_BASE+0x036C))

#define  UDC_EP12FDP  (*(volatile unsigned *)(UDC_BASE+0x0370))
#define  UDC_EP3CS      (*(volatile unsigned *)(UDC_BASE+0x0374))
#define  UDC_EP3DC      (*(volatile unsigned *)(UDC_BASE+0x0378))
#define  UDC_EP3DP      (*(volatile unsigned *)(UDC_BASE+0x037C))
#define  UDC_EP0ONKC  (*(volatile unsigned *)(UDC_BASE+0x0384))
#define  UDC_EP0INAKCN  (*(volatile unsigned *)(UDC_BASE+0x0388))
#define  UDC_EP1INAKCN  (*(volatile unsigned *)(UDC_BASE+0x038C))
#define  UDC_EP2INAKCN  (*(volatile unsigned *)(UDC_BASE+0x0390))
#define  UDC_LLCSET0  (*(volatile unsigned *)(UDC_BASE+0x03B0))
#define  UDC_LLCSET1  (*(volatile unsigned *)(UDC_BASE+0x03B4))
#define  UDC_LLCS    (*(volatile unsigned *)(UDC_BASE+0x03B8))
#define  UDC_LLCSTL  (*(volatile unsigned *)(UDC_BASE+0x03BC))
#define  UDC_LLCSET2	(*(volatile unsigned *)(UDC_BASE+0x03C0))
#define  USBPATTERN_GEN	(*(volatile unsigned *)(UDC_BASE+0x03F0))

#define  UDC_LLCIF		(*(volatile unsigned *)(UDC_BASE+0x0400))
#define  UDC_LLCIE		(*(volatile unsigned *)(UDC_BASE+0x0404))
#define  UDC_LLCIS		(*(volatile unsigned *)(UDC_BASE+0x0408))


#define  UDC_DMA_CS_OFST		0x000
#define  UDC_DMA_DA_OFST		0x004

#define  UDC_DMA_ND_OFST		0x00C
#define  UDC_DMA_CDA_OFST		0x014
#define  UDC_CS_OFST			0x080
#define  UDC_IE_OFST			0x084
#define  UDC_IF_OFST			0x088
#define  UDC_CIS_OFST			0x08C


#define  UDC_LLCIE_OFST			0x0404
#define  UDC_LLCIF_OFST			0x0400
#define  UDC_LLCIS_OFST			0x0408
#define  UDC_LLCFS_OFST			0x0320

#define  UDC_MSTC_OFST           0x0328

#define  UDC_EP12C_OFST				0x0330
#define  UDC_EP12PPC_OFST		0x0334
#define  UDC_EP0SCS_OFST			0x0344
#define  UDC_EP0SDP_OFST			0x0348
#define  UDC_EP0CS_OFST				0x034C
#define  UDC_EP0DC_OFST			0x0350
#define  UDC_EP0DP_OFST			0x0354
#define  UDC_EP1SCS_OFST			0x0358
#define  UDC_EP1SDP_OFST			0x035C
#define  UDC_EP12FS_OFST			0x0364
#define  UDC_EP12FCL_OFST		0x0368
#define  UDC_EP12FCH_OFST		0x036C
#define  UDC_EP12FDP_OFST		0x0370
#define  UDC_EP3CS_OFST				0x0374
#define  UDC_EP3DC_OFST			0x0378
#define  UDC_EP3DP_OFST			0x037C
#define  UDC_EP0ONKC_OFST		0x0384
#define  UDC_EP0INAKCN_OFST	0x0388
#define  UDC_EP1INAKCN_OFST	0x038C
#define  UDC_EP2INAKCN_OFST	0x0390
#define  UDC_LLCSET0_OFST			0x03B0
#define  UDC_LLCSET1_OFST			0x03B4
#define  UDC_LLCS_OFST				0x03B8
#define  UDC_LLCSTL_OFST				0x03BC
#define  UDC_LLCSET2_OFST			0x03C0
#define  USBPATTERN_GEN_OFST	0x03F0
#define  UDC_LLCIF_OFST			0x0400
#define  UDC_LLCIE_OFST			0x0404
#define  UDC_LLCIS_OFST			0x0408

/* DMA_CS */
#define  DMACS_ONWER_DMA       0x80000000  /*other is CPU*/

#define  DMACS_BURST_FIXADDR 0x00000000 
#define  DMACS_BURST_INCADDR 0x02000000
#define  DMACS_BURST_MASK       0x30000000

#define  DMACS_NO_EOF               0x08000000

#define  DMACS_EOF_DISABLE      0x04000000

#define  DMACS_RESPONSE_OK           0x00000000
#define  DMACS_RESPONSE_EXOK       0x0100000
#define  DMACS_RESPONSE_SLVERR   0x02000000
#define  DMACS_RESPONSE_DECERR   0x03000000
#define  DMACS_RESPONSE_MASK      0x03000000

//#define  DMACS_DMA_EN                    0x00800000
#define  DMACS_DMA_EN              0x80000000


#define  DMACS_DMA_MODE_DATA     0x00000000
#define  DMACS_DMA_MODE_SCATTER  0x00400000

#define  DMACS_DMA_READ               0x00000000
//#define  DMACS_DMA_WRITE             0x00200000
#define  DMACS_DMA_WRITE          0x10000000


#define  DMACS_DMA_END                 0x00100000

//#define  DMACS_DMA_FLUSH             0x00080000
#define  DMACS_DMA_FLUSH          0x20000000


//#define  DMACS_DMA_FLUSHEND       0x00040000
#define  DMACS_DMA_FLUSHEND       0x40000000

#define DMA_IF_BIT_UDC (0x01<<25)		
#define  DMACS_DMA_BYTECNT_MASK 0x0000FFFF

/* DMA_DA */
/* CIS */
#define CIS_FDISCONN_IF         0x08000000
#define CIS_FCONN_IF            0x04000000
#define CIS_DMA_IF              0x02000000
#define CIS_UDLC_IF             0x00020000
#define CIS_VBUS_IF             0x01000000
//#define CIS_VBUS_STATUS    		0x00000001

/* DMA_ND */
#define DMAND_NDADDR_MASK   0xFFFFFFFC
#define DMAND_AUTO_ND            0x00000001

/* DMA_CDA */
/* CIE */
#define CIE_FDISCONN_IE  	  0x08000000       
#define CIE_FCONN_IE          0x04000000
#define CIE_DMA_IE            0x02000000
#define CIE_VBUS_IE           0x01000000

/* CCS */
#define UDLC_WAKEUP           0x80000000       
#define UDLC_FCONN            0x40000000
#define UDLC_FDISCONN         0x20000000
#define UDLC_SUSPEND_BLK  	  0x10000000
#define VBUS_SAMP_PERIOD_MASK  0x0000FFFF

/* LCIE */
#define UDLC_RESETN_IE 		0x80000
#define UDLC_SCONF_IE 		0x40000
#define UDLC_RESUME_IE 		0x20000
#define UDLC_SUSPEND_IE 	0x10000
#define UDLC_EP1INN_IE 		0x08000
#define UDLC_EP3I_IE		0x04000
#define UDLC_PIPO_IE		0x02000
#define UDLC_HCS_IE			0x01000
#define UDLC_EP2N_IE		0x00800
#define UDLC_EP1N_IE		0x00400
#define UDLC_EP0N_IE		0x00200
#define UDLC_HSS_IE			0x00100
#define UDLC_EP2O_IE		0x00080
#define UDLC_EP1I_IE		0x00040
#define UDLC_EP1SI_IE		0x00020
#define UDLC_EP0I_IE		0x00010
#define UDLC_EP0O_IE		0x00008
#define UDLC_EP0S_IE		0x00004 
#define UDLC_ESUSP_IE		0x00002
#define UDLC_RESET_IE		0x00001

/* LCIS */
#define UDLC_RESETN_IF 		0x80000
#define UDLC_SCONF_IF 		0x40000
#define UDLC_RESUME_IF 		0x20000
#define UDLC_SUSPEND_IF 	0x10000
#define UDLC_EP1INN_IF 		0x08000
#define UDLC_EP3I_IF		0x04000
#define UDLC_PIPO_IF		0x02000
#define UDLC_HCS_IF			0x01000
#define UDLC_EP2N_IF		0x00800
#define UDLC_EP1N_IF		0x00400
#define UDLC_EP0N_IF		0x00200
#define UDLC_HSS_IF			0x00100
#define UDLC_EP2O_IF		0x00080
#define UDLC_EP1I_IF		0x00040
#define UDLC_EP1SI_IF		0x00020
#define UDLC_EP0I_IF		0x00010
#define UDLC_EP0O_IF		0x00008
#define UDLC_EP0S_IF		0x00004 
#define UDLC_ESUSP_IF		0x00002
#define UDLC_RESET_IF		0x00001

/* LCIF */
#define UDLC_RESETN_IS 		0x80000
#define UDLC_SCONF_IS 		0x40000
#define UDLC_RESUME_IS 		0x20000
#define UDLC_SUSPEND_IS 	0x10000
#define UDLC_EP1INN_IS 		0x08000
#define UDLC_EP3I_IS		0x04000
#define UDLC_PIPO_IS		0x02000
#define UDLC_HCS_IS			0x01000
#define UDLC_EP2N_IS		0x00800
#define UDLC_EP1N_IS		0x00400
#define UDLC_EP0N_IS		0x00200
#define UDLC_HSS_IS			0x00100
#define UDLC_EP2O_IS		0x00080
#define UDLC_EP1I_IS		0x00040
#define UDLC_EP1SI_IS		0x00020
#define UDLC_EP0I_IS		0x00010
#define UDLC_EP0O_IS		0x00008
#define UDLC_EP0S_IS		0x00004 
#define UDLC_ESUSP_IS		0x00002
#define UDLC_RESET_IS		0x00001

/* LCFS */
#define CLR_EP2_OVLD      0x00020
#define SET_EP1_IVLD      0x00010
#define MSDC_CMD_VLD      0x00008
#define CUR_FIFO_EMPTY 	  0x00004
#define LCFS_EP2_OVLD     0x00002
#define LCFS_EP1_IVLD     0x00001
/* MSTC */
#define CP_CBW_TAG_MASK 0xFF

/* EP12C */
#define EP12C_MSDC_CMD_VLD 0x80
#define EP12C_EP2_OVLD          0x40
#define EP12C_EP1_IVLD           0x20
#define EP12C_SET_EP1_IVLD   0x10
#define EP12C_CLR_EP2_OVLD   0x08
#define EP12C_RESET_PIPO       0x04
#define EP12C_ENABLE_BULK    0x02
#define EP12C_DIR_IN               0x01

/* EP12PPC */
#define EP12PPC_N_EP2_OVLD  0x80
#define EP12PPC_P_EP1_IVLD   0x40
#define EP12PPC_EP2_OVLD      0x20
#define EP12PPC_EP1_IVLD      0x10
#define EP12PPC_DIR_IN           0x08
#define EP12PPC_CURR_BUF_NUM   0x04
#define EP12PPC_SW_BUF               0x02
#define EP12PPC_AUTO_SW_EN      0x01

/* EP0SCS */
#define EP0SCS_SFIO_UPDATE  0x40
#define EP0SCS_SFIFO_VALID   0x20

/* EP0SDP */
#define EP0SDP_SET_DATA_MSK   0xFF       

/* EP0CS */
#define EP0CS_OUT_EMPTY     0x80  
#define EP0CS_OVLD                0x40
#define EP0CS_CLR_EP0_OVLD  0x20
#define EP0CS_IVLD                 0x10
#define EP0CS_SET_EP0_IVLD    0x08
#define EP0CS_SFIFO_UPDATE   0x04
#define EP0CS_SFIFO_VALID      0x02
#define EP0CS_DIR_IN                0x01

/* EP0DC */
#define EP0_DATA_CNTR_MASK  0x7F

/* EP1SCS */
#define EP1SCS_FIFO_CNTR_MSK  0xF0
#define EP1SCS_CLR_IVLD             0x08
#define EP1SCS_RESET_FIFO        0x04
#define EP1SCS_IVLD             0x02
#define EP1SCS_SET_IVLD     0x01
/* EP1SDP */
/* EP12FS */
#define  EP12FS_N_MSDC_CMD      0x80
#define  EP12FS_A_FIFO_EMPTY   0x40
#define  EP12FS_N_EP2_OVLD       0x20
#define  EP12FS_P_EP1_IVLD        0x10
#define  EP12FS_MSDC_CMD_VLD  0x08
#define  EP12FS_FIFO_EMPTY       0x04
#define  EP12FS_EP2_OVLD           0x02
#define  EP12FS_EP1_IVLD            0x01

/* EP12FCL */
#define  EP12FCL_FIFO_CNTRL_MASK      0xFF

/* EP12FCH */
#define  EP12FCH_RESET_CNTR  0x04
#define  EP12FCH_FIFO_CNTRH_MASK  0x03
/* EP12FDP */
/* EP3CS */
#define EP3CS_IVLD            0x08
#define EP3CS_CLR_IVLD    0x04
#define EP3CS_SET_IVLD    0x02
#define EP3CS_IN_EN         0x01

/* EP3DC */
#define EP3_DATA_CNTR_MASK  0x7F
/* EP3DP */

/* EP0ONAKCN */
#define EP0_OUT_NAK_CNT_MASK 0xFF
/* EP0INAKCN */
#define EP0_IN_NAK_CNT_MASK 0xFF
/* EP1INAKCN */
#define EP1_IN_NAK_CNT_MASK 0xFF
/* EP2INAKCN */
#define EP2_OUT_NAK_CNT_MASK 0xFF

/* LCSET0 */
#define LCSET0_CLR_SUSP_CNT   0x80
#define LCSET0_SIM_MODE          0x40
#define LCSET0_DISC_SUSP_EN   0x20
#define LCSET0_CPU_WKUP_EN    0x10
#define LCSET0_PWR_PART_N      0x08
#define LCSET0_PWR_SUSP_N      0x04
#define LCSET0_ISSUE_RESUME   0x02
#define LCSET0_SOFT_DISC         0x01
/* LCSET1 */
#define LCSET0_NO_SET_SUSP_OPT         0x80
#define LCSET0_NO_STOP_CHIRP              0x40
#define LCSET0_INTER_PACKET_DLY        0x20
#define LCSET0_FORCE_FULLSP                0x10
#define LCSET0_VBUS_LOW_AUTO_DISC  0x08
#define LCSET0_DISC_AUTO_DPDMPD      0x04
#define LCSET0_SUPP_RWAKE         0x02
#define LCSET0_SELF_POWER         0x01

/*LCS*/
#define LCS_DISC_CONN_STATUS             0x80
#define LCS_HOST_CONFIGED                   0x40
#define LCS_LNK_SUSP                              0x20
#define LCS_HOST_ALLOW_RWAKE           0x10
#define LCS_CURR_LINESTATE_SE0          0x00
#define LCS_CURR_LINESTATE_F_J           0x04
#define LCS_CURR_LINESTATE_F_K          0x08
#define LCS_CURR_LINESTATE_H_SQUELCH  0x04
#define LCS_CURR_LINESTATE_MASK       0xC0
#define LCS_CURR_SPEED_F                     0x02                         
#define LCS_VBUS_HIGH                           0x01

/* LCSTL */
#define LCSTL_CLREP3STL   0x80
#define LCSTL_CLREP2STL   0x40
#define LCSTL_CLREP1STL   0x20
#define LCSTL_CLREP0STL   0x10
#define LCSTL_SETEP3STL   0x08
#define LCSTL_SETEP2STL   0x04
#define LCSTL_SETEP1STL   0x02
#define LCSTL_SETEP0STL   0x01

/*LCSET2 */
#define LCSET2_SUSP_REF  0x8
#define LCSET2_USBC_EP1S_FST  0x4
#define LCSET2_USBC_STOP_SPEED  0x2
#define LCSET2_USBC_SUPP_EP3  0x1

/* LCS2 */
/* PATTER*/
