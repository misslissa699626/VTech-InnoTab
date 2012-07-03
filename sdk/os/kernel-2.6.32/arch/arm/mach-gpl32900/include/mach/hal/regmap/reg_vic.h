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
#ifndef _REG_VIC_H_
#define _REG_VIC_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

#define	LOGI_ADDR_VIC_REG	   (IO0_BASE + 0x10000 )
#define VIC_OFFSET             (0x10000)
#define VIC_PRIORITY_BASE      (IO0_BASE + 0x10000 + 0x200)

#define VIC0_ADDRESS           (*(volatile unsigned int*)(IO0_BASE + 0x10F00))
#define VIC1_ADDRESS           (*(volatile unsigned int*)(IO0_BASE + 0x20F00))
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct vicReg_s {
	/* volatile UINT8 regOffset[0x10000]; */ /* 0x90010000 */ 
	volatile UINT32 vicIrqStatus;	/* P_VICx_IRQSTATUS, 0x90010000*/
	volatile UINT32 vicFiqStatus;	/* P_VICx_FIQSTATUS, 0x90010004 */
	volatile UINT32 vicIrqRawStatus;/* P_VICx_IRQRAWSTATUS, 0x90010008 */
	volatile UINT32 vicIntSelect;	/* P_VICx_INTSELECT, 0x9001000C */
	volatile UINT32 vicIntEnable;	/* P_VICx_INTENABLE, 0x90010010 */
	volatile UINT32 vicIntEnClear;	/* P_VICx_INTENCLEAR, 0x90010014 */
	volatile UINT32 vicSoftInt;		/* P_VICx_SOFTINT, 0x90010018 */
	volatile UINT32 vicSoftIntClear;/* P_VICx_SOFTINTCLEAR, 0x9001001C */
	volatile UINT32 vicProtection;  /* P_VICx_PROTECTION, 0x90010020 */
	volatile UINT32 vicPriorityMask; /* P_VICx_PRIORITYMASK, 0x90010024 */
	volatile UINT8  offset00[0xD8];
	volatile UINT32 vicVectAddr0;    /* P_VICx_VECTADDR0, 0x90010100 */
	volatile UINT32 vicVectAddr1;    /* P_VICx_VECTADDR1, 0x90010104 */
	volatile UINT32 vicVectAddr2;    /* P_VICx_VECTADDR2, 0x90010108 */
	volatile UINT32 vicVectAddr3;    /* P_VICx_VECTADDR3, 0x9001010C */
	volatile UINT32 vicVectAddr4;    /* P_VICx_VECTADDR4, 0x90010110 */
	volatile UINT32 vicVectAddr5;    /* P_VICx_VECTADDR5, 0x90010114 */
	volatile UINT32 vicVectAddr6;    /* P_VICx_VECTADDR6, 0x90010118 */
	volatile UINT32 vicVectAddr7;    /* P_VICx_VECTADDR7, 0x9001011C */
	volatile UINT32 vicVectAddr8;    /* P_VICx_VECTADDR8, 0x90010120 */
	volatile UINT32 vicVectAddr9;    /* P_VICx_VECTADDR9, 0x90010124 */
	volatile UINT32 vicVectAddr10;   /* P_VICx_VECTADDR10, 0x90010128 */
	volatile UINT32 vicVectAddr11;   /* P_VICx_VECTADDR11, 0x9001012C */
	volatile UINT32 vicVectAddr12;   /* P_VICx_VECTADDR12, 0x90010130 */
	volatile UINT32 vicVectAddr13;   /* P_VICx_VECTADDR13, 0x90010134 */
	volatile UINT32 vicVectAddr14;   /* P_VICx_VECTADDR14, 0x90010138 */
	volatile UINT32 vicVectAddr15;   /* P_VICx_VECTADDR15, 0x9001013C */
	volatile UINT32 vicVectAddr16;   /* P_VICx_VECTADDR16, 0x90010140 */
	volatile UINT32 vicVectAddr17;   /* P_VICx_VECTADDR17, 0x90010144 */
	volatile UINT32 vicVectAddr18;   /* P_VICx_VECTADDR18, 0x90010148 */
	volatile UINT32 vicVectAddr19;   /* P_VICx_VECTADDR19, 0x9001014C */
	volatile UINT32 vicVectAddr20;   /* P_VICx_VECTADDR20, 0x90010150 */
	volatile UINT32 vicVectAddr21;   /* P_VICx_VECTADDR21, 0x90010154 */
	volatile UINT32 vicVectAddr22;   /* P_VICx_VECTADDR22, 0x90010158 */
	volatile UINT32 vicVectAddr23;   /* P_VICx_VECTADDR23, 0x9001015C */
	volatile UINT32 vicVectAddr24;   /* P_VICx_VECTADDR24, 0x90010160 */
	volatile UINT32 vicVectAddr25;   /* P_VICx_VECTADDR25, 0x90010164 */
	volatile UINT32 vicVectAddr26;   /* P_VICx_VECTADDR26, 0x90010168 */
	volatile UINT32 vicVectAddr27;   /* P_VICx_VECTADDR27, 0x9001016C */
	volatile UINT32 vicVectAddr28;   /* P_VICx_VECTADDR28, 0x90010170 */
	volatile UINT32 vicVectAddr29;   /* P_VICx_VECTADDR29, 0x90010174 */
	volatile UINT32 vicVectAddr30;   /* P_VICx_VECTADDR30, 0x90010178 */
	volatile UINT32 vicVectAddr31;   /* P_VICx_VECTADDR31, 0x9001017C */
	volatile UINT8  offset01[0x80];
	volatile UINT32 vicPriority0;    /* P_VICx_PRIORITY0, 0x90010200 */
	volatile UINT32 vicPriority1;    /* P_VICx_PRIORITY1, 0x90010204 */
	volatile UINT32 vicPriority2;    /* P_VICx_PRIORITY2, 0x90010208 */
	volatile UINT32 vicPriority3;    /* P_VICx_PRIORITY3, 0x9001020C */
	volatile UINT32 vicPriority4;    /* P_VICx_PRIORITY4, 0x90010210 */
	volatile UINT32 vicPriority5;    /* P_VICx_PRIORITY5, 0x90010214 */
	volatile UINT32 vicPriority6;    /* P_VICx_PRIORITY6, 0x90010218 */
	volatile UINT32 vicPriority7;    /* P_VICx_PRIORITY7, 0x9001021C */
	volatile UINT32 vicPriority8;    /* P_VICx_PRIORITY8, 0x90010220 */
	volatile UINT32 vicPriority9;    /* P_VICx_PRIORITY9, 0x90010224 */
	volatile UINT32 vicPriority10;   /* P_VICx_PRIORITY10, 0x90010228 */
	volatile UINT32 vicPriority11;   /* P_VICx_PRIORITY11, 0x9001022C */
	volatile UINT32 vicPriority12;   /* P_VICx_PRIORITY12, 0x90010230 */
	volatile UINT32 vicPriority13;   /* P_VICx_PRIORITY13, 0x90010234 */
	volatile UINT32 vicPriority14;   /* P_VICx_PRIORITY14, 0x90010238 */
	volatile UINT32 vicPriority15;   /* P_VICx_PRIORITY15, 0x9001023C */
	volatile UINT32 vicPriority16;   /* P_VICx_PRIORITY16, 0x90010240 */
	volatile UINT32 vicPriority17;   /* P_VICx_PRIORITY17, 0x90010244 */
	volatile UINT32 vicPriority18;   /* P_VICx_PRIORITY18, 0x90010248 */
	volatile UINT32 vicPriority19;   /* P_VICx_PRIORITY19, 0x9001024C */
	volatile UINT32 vicPriority20;   /* P_VICx_PRIORITY20, 0x90010250 */
	volatile UINT32 vicPriority21;   /* P_VICx_PRIORITY21, 0x90010254 */
	volatile UINT32 vicPriority22;   /* P_VICx_PRIORITY22, 0x90010258 */
	volatile UINT32 vicPriority23;   /* P_VICx_PRIORITY23, 0x9001025C */
	volatile UINT32 vicPriority24;   /* P_VICx_PRIORITY24, 0x90010260 */
	volatile UINT32 vicPriority25;   /* P_VICx_PRIORITY25, 0x90010264 */
	volatile UINT32 vicPriority26;   /* P_VICx_PRIORITY26, 0x90010268 */
	volatile UINT32 vicPriority27;   /* P_VICx_PRIORITY27, 0x9001026C */
	volatile UINT32 vicPriority28;   /* P_VICx_PRIORITY28, 0x90010270 */
	volatile UINT32 vicPriority29;   /* P_VICx_PRIORITY29, 0x90010274 */
	volatile UINT32 vicPriority30;   /* P_VICx_PRIORITY30, 0x90010278 */
	volatile UINT32 vicPriority31;   /* P_VICx_PRIORITY31, 0x9001027C */
	volatile UINT8  offset02[0xC80];
	volatile UINT32 vicAddress;      /* P_VICx_ADDRESS, 0x90010F00 */
} vicReg_t;


#endif /* _REG_VIC_H_ */