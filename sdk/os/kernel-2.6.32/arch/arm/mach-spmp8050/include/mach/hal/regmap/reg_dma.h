/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
#ifndef _REG_DMA_H_
#define _REG_DMA_H_

#include <mach/hardware.h>                          
#include <mach/typedef.h>

#define	LOGI_ADDR_DMA_REG		(IO2_BASE + 0xb00000)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct dmaReg_s {
    /* volatile UINT8  regOffset[0xFDb00000]; */  /* 0x92B00000 */
    volatile UINT32 rsv000[0x02];          /* 0x0000 ~ 0x0007 */
    volatile UINT32 dmacgisr;              /* 0x0008 ~ 0x000b */
    volatile UINT32 rsv00c[0x0d];          /* 0x000c ~ 0x003f */
    volatile UINT32 dmacIntrStatus0;       /* 0x0040 ~ 0x0043 */
    volatile UINT32 dmacIntrClr0;          /* 0x0044 ~ 0x0047 */
    volatile UINT32 dmacIntrMask0;         /* 0x0048 ~ 0x004b */
    volatile UINT32 dmacIntrRaw0;          /* 0x004c ~ 0x004f */
    volatile UINT32 dmacChStatus0;         /* 0x0050 ~ 0x0053 */
    volatile UINT32 rsv054[0x03];          /* 0x0054 ~ 0x005f */
    volatile UINT32 dmacCfg0;              /* 0x0060 ~ 0x0063 */
    volatile UINT32 dmacCtrl0;             /* 0x0064 ~ 0x0067 */
    volatile UINT32 dmacSrcAdr0;           /* 0x0068 ~ 0x006b */
    volatile UINT32 dmacDstAdr0;           /* 0x006c ~ 0x006f */
    volatile UINT32 dmacSrcNadr0;          /* 0x0070 ~ 0x0073 */
    volatile UINT32 dmacDstNadr0;          /* 0x0074 ~ 0x0077 */
    volatile UINT32 dmacSrcBsize0;         /* 0x0078 ~ 0x007b */
    volatile UINT32 dmacDstBsize0;         /* 0x007c ~ 0x007f */
    volatile UINT32 dmacSrcFsize0;         /* 0x0080 ~ 0x0083 */
    volatile UINT32 dmacDstFsize0;         /* 0x0084 ~ 0x0087 */
    volatile UINT32 dmacSrcPsize0;         /* 0x0088 ~ 0x008b */
    volatile UINT32 dmacDstPsize0;         /* 0x008c ~ 0x008f */
    volatile UINT32 dmacSrcBstep0;         /* 0x0090 ~ 0x0093 */
    volatile UINT32 dmacDstBstep0;         /* 0x0094 ~ 0x0097 */
    volatile UINT32 dmacSrcFstep0;         /* 0x0098 ~ 0x009b */
    volatile UINT32 dmacDstFstep0;         /* 0x009c ~ 0x009f */

    volatile UINT32 dmacCfg1;              /* 0x00a0 ~ 0x00a3 */
    volatile UINT32 dmacCtrl1;             /* 0x00a4 ~ 0x00a7 */
    volatile UINT32 dmacSrcAdr1;           /* 0x00a8 ~ 0x00ab */
    volatile UINT32 dmacDstAdr1;           /* 0x00ac ~ 0x00af */
    volatile UINT32 dmacSrcNadr1;          /* 0x00b0 ~ 0x00b3 */
    volatile UINT32 dmacDstNadr1;          /* 0x00b4 ~ 0x00b7 */
    volatile UINT32 dmacSrcBsize1;         /* 0x00b8 ~ 0x00bb */
    volatile UINT32 dmacDstBsize1;         /* 0x00bc ~ 0x00bf */
    volatile UINT32 dmacSrcFsize1;         /* 0x00c0 ~ 0x00c3 */
    volatile UINT32 dmacDstFsize1;         /* 0x00c4 ~ 0x00c7 */
    volatile UINT32 dmacSrcPsize1;         /* 0x00c8 ~ 0x00cb */
    volatile UINT32 dmacDstPsize1;         /* 0x00cc ~ 0x00cf */
    volatile UINT32 dmacSrcBstep1;         /* 0x00d0 ~ 0x00d3 */
    volatile UINT32 dmacDstBstep1;         /* 0x00d4 ~ 0x00d7 */
    volatile UINT32 dmacSrcFstep1;         /* 0x00d8 ~ 0x00db */
    volatile UINT32 dmacDstFstep1;         /* 0x00dc ~ 0x00df */

    volatile UINT32 dmacCfg2;              /* 0x00e0 ~ 0x00e3 */
    volatile UINT32 dmacCtrl2;             /* 0x00e4 ~ 0x00e7 */
    volatile UINT32 dmacSrcAdr2;           /* 0x00e8 ~ 0x00eb */
    volatile UINT32 dmacDstAdr2;           /* 0x00ec ~ 0x00ef */
    volatile UINT32 dmacSrcNadr2;          /* 0x00f0 ~ 0x00f3 */
    volatile UINT32 dmacDstNadr2;          /* 0x00f4 ~ 0x00f7 */
    volatile UINT32 dmacSrcBsize2;         /* 0x00f8 ~ 0x00fb */
    volatile UINT32 dmacDstBsize2;         /* 0x00fc ~ 0x00ff */
    volatile UINT32 dmacSrcFsize2;         /* 0x0100 ~ 0x0103 */
    volatile UINT32 dmacDstFsize2;         /* 0x0104 ~ 0x0107 */
    volatile UINT32 dmacSrcPsize2;         /* 0x0108 ~ 0x010b */
    volatile UINT32 dmacDstPsize2;         /* 0x010c ~ 0x010f */
    volatile UINT32 dmacSrcBstep2;         /* 0x0110 ~ 0x0113 */
    volatile UINT32 dmacDstBstep2;         /* 0x0114 ~ 0x0117 */
    volatile UINT32 dmacSrcFstep2;         /* 0x0118 ~ 0x011b */
    volatile UINT32 dmacDstFstep2;         /* 0x011c ~ 0x011f */

    volatile UINT32 dmacCfg3;              /* 0x0120 ~ 0x0123 */
    volatile UINT32 dmacCtrl3;             /* 0x0124 ~ 0x0127 */
    volatile UINT32 dmacSrcAdr3;           /* 0x0128 ~ 0x012b */
    volatile UINT32 dmacDstAdr3;           /* 0x012c ~ 0x012f */
    volatile UINT32 dmacSrcNadr3;          /* 0x0130 ~ 0x0133 */
    volatile UINT32 dmacDstNadr3;          /* 0x0134 ~ 0x0137 */
    volatile UINT32 dmacSrcBsize3;         /* 0x0138 ~ 0x013b */
    volatile UINT32 dmacDstBsize3;         /* 0x013c ~ 0x013f */
    volatile UINT32 dmacSrcFsize3;         /* 0x0140 ~ 0x0143 */
    volatile UINT32 dmacDstFsize3;         /* 0x0144 ~ 0x0147 */
    volatile UINT32 dmacSrcPsize3;         /* 0x0148 ~ 0x014b */
    volatile UINT32 dmacDstPsize3;         /* 0x014c ~ 0x014f */
    volatile UINT32 dmacSrcBstep3;         /* 0x0150 ~ 0x0153 */
    volatile UINT32 dmacDstBstep3;         /* 0x0154 ~ 0x0157 */
    volatile UINT32 dmacSrcFstep3;         /* 0x0158 ~ 0x015b */
    volatile UINT32 dmacDstFstep3;         /* 0x015c ~ 0x015f */

    volatile UINT32 dmacCfg4;              /* 0x0160 ~ 0x0163 */
    volatile UINT32 dmacCtrl4;             /* 0x0164 ~ 0x0167 */
    volatile UINT32 dmacSrcAdr4;           /* 0x0168 ~ 0x016b */
    volatile UINT32 dmacDstAdr4;           /* 0x016c ~ 0x016f */
    volatile UINT32 dmacSrcNadr4;          /* 0x0170 ~ 0x0173 */
    volatile UINT32 dmacDstNadr4;          /* 0x0174 ~ 0x0177 */
    volatile UINT32 dmacSrcBsize4;         /* 0x0178 ~ 0x017b */
    volatile UINT32 dmacDstBsize4;         /* 0x017c ~ 0x017f */
    volatile UINT32 dmacSrcFsize4;         /* 0x0180 ~ 0x0183 */
    volatile UINT32 dmacDstFsize4;         /* 0x0184 ~ 0x0187 */
    volatile UINT32 dmacSrcPsize4;         /* 0x0188 ~ 0x018b */
    volatile UINT32 dmacDstPsize4;         /* 0x018c ~ 0x018f */
    volatile UINT32 dmacSrcBstep4;         /* 0x0190 ~ 0x0193 */
    volatile UINT32 dmacDstBstep4;         /* 0x0194 ~ 0x0197 */
    volatile UINT32 dmacSrcFstep4;         /* 0x0198 ~ 0x019b */
    volatile UINT32 dmacDstFstep4;         /* 0x019c ~ 0x019f */

    volatile UINT32 dmacCfg5;              /* 0x01a0 ~ 0x01a3 */
    volatile UINT32 dmacCtrl5;             /* 0x01a4 ~ 0x01a7 */
    volatile UINT32 dmacSrcAdr5;           /* 0x01a8 ~ 0x01ab */
    volatile UINT32 dmacDstAdr5;           /* 0x01ac ~ 0x01af */
    volatile UINT32 dmacSrcNadr5;          /* 0x01b0 ~ 0x01b3 */
    volatile UINT32 dmacDstNadr5;          /* 0x01b4 ~ 0x01b7 */
    volatile UINT32 dmacSrcBsize5;         /* 0x01b8 ~ 0x01bb */
    volatile UINT32 dmacDstBsize5;         /* 0x01bc ~ 0x01bf */
    volatile UINT32 dmacSrcFsize5;         /* 0x01c0 ~ 0x01c3 */
    volatile UINT32 dmacDstFsize5;         /* 0x01c4 ~ 0x01c7 */
    volatile UINT32 dmacSrcPsize5;         /* 0x01c8 ~ 0x01cb */
    volatile UINT32 dmacDstPsize5;         /* 0x01cc ~ 0x01cf */
    volatile UINT32 dmacSrcBstep5;         /* 0x01d0 ~ 0x01d3 */
    volatile UINT32 dmacDstBstep5;         /* 0x01d4 ~ 0x01d7 */
    volatile UINT32 dmacSrcFstep5;         /* 0x01d8 ~ 0x01db */
    volatile UINT32 dmacDstFstep5;         /* 0x01dc ~ 0x01df */

    volatile UINT32 dmacCfg6;              /* 0x01e0 ~ 0x01e3 */
    volatile UINT32 dmacCtrl6;             /* 0x01e4 ~ 0x01e7 */
    volatile UINT32 dmacSrcAdr6;           /* 0x01e8 ~ 0x01eb */
    volatile UINT32 dmacDstAdr6;           /* 0x01ec ~ 0x01ef */
    volatile UINT32 dmacSrcNadr6;          /* 0x01f0 ~ 0x01f3 */
    volatile UINT32 dmacDstNadr6;          /* 0x01f4 ~ 0x01f7 */
    volatile UINT32 dmacSrcBsize6;         /* 0x01f8 ~ 0x01fb */
    volatile UINT32 dmacDstBsize6;         /* 0x01fc ~ 0x01ff */
    volatile UINT32 dmacSrcFsize6;         /* 0x0200 ~ 0x0203 */
    volatile UINT32 dmacDstFsize6;         /* 0x0204 ~ 0x0207 */
    volatile UINT32 dmacSrcPsize6;         /* 0x0208 ~ 0x020b */
    volatile UINT32 dmacDstPsize6;         /* 0x020c ~ 0x020f */
    volatile UINT32 dmacSrcBstep6;         /* 0x0210 ~ 0x0213 */
    volatile UINT32 dmacDstBstep6;         /* 0x0214 ~ 0x0217 */
    volatile UINT32 dmacSrcFstep6;         /* 0x0218 ~ 0x021b */
    volatile UINT32 dmacDstFstep6;         /* 0x021c ~ 0x021f */

    volatile UINT32 dmacCfg7;              /* 0x0220 ~ 0x0223 */
    volatile UINT32 dmacCtrl7;             /* 0x0224 ~ 0x0227 */
    volatile UINT32 dmacSrcAdr7;           /* 0x0228 ~ 0x022B */
    volatile UINT32 dmacDstAdr7;           /* 0x022C ~ 0x022F */
    volatile UINT32 dmacSrcNadr7;          /* 0x0230 ~ 0x0233 */
    volatile UINT32 dmacDstNadr7;          /* 0x0234 ~ 0x0237 */
    volatile UINT32 dmacSrcBsize7;         /* 0x0238 ~ 0x023b */
    volatile UINT32 dmacDstBsize7;         /* 0x023c ~ 0x023f */
    volatile UINT32 dmacSrcFsize7;         /* 0x0240 ~ 0x0243 */
    volatile UINT32 dmacDstFsize7;         /* 0x0244 ~ 0x0247 */
    volatile UINT32 dmacSrcPsize7;         /* 0x0248 ~ 0x024b */
    volatile UINT32 dmacDstPsize7;         /* 0x024c ~ 0x024f */
    volatile UINT32 dmacSrcBstep7;         /* 0x0250 ~ 0x0253 */
    volatile UINT32 dmacDstBstep7;         /* 0x0254 ~ 0x0257 */
    volatile UINT32 dmacSrcFstep7;         /* 0x0258 ~ 0x025b */
    volatile UINT32 dmacDstFstep7;         /* 0x025c ~ 0x025f */

    volatile UINT32 rsv260[0x367];         /* 0x0260 ~ 0x0ffb */
    volatile UINT32 dmacId;                /* 0x0ffc ~ 0x0fff */

} dmaReg_t;

#endif /* _REG_DMA_H_ */

