/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2002 by Sunplus Technology Co., Ltd.             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *  Author: Timothy Wu, Matt Wang                                         *
 *                                                                        *
 **************************************************************************/
/**
 * @file general.h
 * @brief Contains definition for general API
 */

#ifndef __GENERAL_H__
#define __GENERAL_H__

#include "common.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define GNU_PACK __attribute__ ((packed))

/*
 * access memory without optimization.
 */
#define READ8(_reg_)            (*((volatile UINT8  *)(_reg_)))
#define READ16(_reg_)           (*((volatile UINT16 *)(_reg_)))
#define READ32(_reg_)           (*((volatile UINT32 *)(_reg_)))
#define WRITE8(_reg_, _value_)  (*((volatile UINT8  *)(_reg_)) = (_value_))
#define WRITE16(_reg_, _value_) (*((volatile UINT16 *)(_reg_)) = (_value_))
#define WRITE32(_reg_, _value_) (*((volatile UINT32 *)(_reg_)) = (_value_))

/*
 * address space conversion and alignment.
 */
#ifndef WIN32
/* logical cached byte address <=> logical uncached byte address. */
#define LOGI_CACH_BADDR_TO_LOGI_UNCACH_BADDR(addr) \
	((void *)((UINT32)(addr) | 0x20000000))

#define LOGI_UNCACH_BADDR_TO_LOGI_CACH_BADDR(addr) \
	((void *)((UINT32)(addr) & ~0x20000000))

/* logical cached byte address <=> physical byte address. */
#define LOGI_CACH_BADDR_TO_PHY_BADDR(addr) \
	((void *)((UINT32)(addr) & ~0x80000000))

#define PHY_BADDR_TO_LOGI_CACH_BADDR(addr) \
	((void *)((UINT32)(addr) | 0x80000000))
#else
/* logical cached byte address <=> logical uncached byte address. */
#define LOGI_CACH_BADDR_TO_LOGI_UNCACH_BADDR(addr) \
	((void *)((UINT32)(addr)))

#define LOGI_UNCACH_BADDR_TO_LOGI_CACH_BADDR(addr) \
	((void *)((UINT32)(addr)))

/* logical cached byte address <=> physical byte address. */
#define LOGI_CACH_BADDR_TO_PHY_BADDR(addr) \
	((void *)((UINT32)(addr)))

#define PHY_BADDR_TO_LOGI_CACH_BADDR(addr) \
	((void *)((UINT32)(addr)))
#endif

/* logical uncached byte address <=> physical byte address. */
#define LOGI_UNCACH_BADDR_TO_PHY_BADDR(addr) \
	LOGI_CACH_BADDR_TO_PHY_BADDR(LOGI_UNCACH_BADDR_TO_LOGI_CACH_BADDR(addr))

#define PHY_BADDR_TO_LOGI_UNCACH_BADDR(addr) \
	LOGI_CACH_BADDR_TO_LOGI_UNCACH_BADDR(PHY_BADDR_TO_LOGI_CACH_BADDR(addr))

/* logical uncached byte address <=> physical word address. */
#define LOGI_UNCACH_BADDR_TO_PHY_WADDR(addr) \
	((UINT32)LOGI_UNCACH_BADDR_TO_PHY_BADDR(addr) >> 1)

#define PHY_WADDR_TO_LOGI_UNCACH_BADDR(addr) \
	PHY_BADDR_TO_LOGI_UNCACH_BADDR((UINT32)(addr) << 1)

/* logical cached byte address <=> physical word address. */
#define LOGI_CACH_BADDR_TO_PHY_WADDR(addr) \
	((UINT32)LOGI_CACH_BADDR_TO_PHY_BADDR(addr) >> 1)

#define PHY_WADDR_TO_LOGI_CACH_BADDR(addr) \
	PHY_BADDR_TO_LOGI_CACH_BADDR((UINT32)(addr) << 1)


#define BADDR_IS_DRAM_CACHE(addr)    ((UINT32)(addr) >> 28 == 0x08)
#define BADDR_IS_DRAM_UNCACHE(addr)  ((UINT32)(addr) >> 28 == 0x0a)
#ifndef WIN32
#define BADDR_IS_DRAM(addr)        ((((UINT32)(addr) >> 28) & ~0x02) == 0x08)
#else
#define BADDR_IS_DRAM(addr)        (1)
#endif

#define ROUND_DOWN_TO_DIVISIBLE(num,div) \
	( (UINT32)(num) & ~((UINT32)(div) - 1) )
#define ROUND_UP_TO_DIVISIBLE(num,div) \
	( ((UINT32)(num) + (div) - 1) & ~((UINT32)(div) - 1) )

/* byte address alignment. */
#define BADDR_ALIGN(addr, align) ((void *)ROUND_UP_TO_DIVISIBLE(addr,align))

#define BADDR_IS_ALIGNED(addr,align) \
	(((UINT32)(addr) & ((align) - 1)) == 0)

/*DCACHE architectures*/
/*cache coherence problems*/
#define D_CACHE_SIZE             4096
#define D_CACHE_LINE_SIZE          16
#define D_CACHE_WAY                 2
#define D_CACHE_HAS_WRITE_BACK      1 /*0: WRITE_THROUGH*/
#define D_CACHE_HAS_WRITE_ALLOCATE  0 /*1: WRITE_ALLOCATE*/
#define D_CACHE_HAS_CACHE_INSN      1

#define D_CACHE_ALIGN_BOTH_END_N_ITEM(type,n_item) \
	((((sizeof(type) * (n_item) + D_CACHE_LINE_SIZE - 1) & \
	~(D_CACHE_LINE_SIZE - 1)) + (D_CACHE_LINE_SIZE - 1)) / sizeof(type))

#define BADDR_ALIGN_CACHE_LINE(addr) BADDR_ALIGN(addr,D_CACHE_LINE_SIZE)

#define BADDR_TO_CACHE(addr) \
	(BADDR_IS_DRAM_UNCACHE(addr) ? \
	(void *)LOGI_UNCACH_BADDR_TO_LOGI_CACH_BADDR(addr) : (void *)(addr))

#define BADDR_TO_UNCACHE(addr) \
	(BADDR_IS_DRAM_CACHE(addr) ? \
	(void *)LOGI_CACH_BADDR_TO_LOGI_UNCACH_BADDR(addr) : (void *)(addr))

#define BADDR_TO_UNCACHE_ALIGN_CACHE_LINE(addr) \
	BADDR_TO_UNCACHE( BADDR_ALIGN_CACHE_LINE(addr) )

void cpuDCacheFlush(void *addrstart, UINT32 size);
void cpuDCacheWB(void *addrstart, UINT32 size);
void cpuDCacheInv(void *addrstart, UINT32 size);

/*"CCTL" register for flush all op:0x001=inv; 0x200=wbinv; 0x100=wb;*/
#define D_CACHE_FLUSH_ALL_INV    0x001
#if D_CACHE_HAS_WRITE_BACK
 #define D_CACHE_FLUSH_ALL_WBINV  0x200
 #define D_CACHE_FLUSH_ALL_WB     0x100
#else
 #define D_CACHE_FLUSH_ALL_WBINV  D_CACHE_FLUSH_ALL_INV
 #define D_CACHE_FLUSH_ALL_WB     0
#endif
void cpuCacheFlushAll(UINT32 op);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef void (*irqIsr_t)(void);
typedef irqIsr_t isr_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/* Get the timer value of system clock */
UINT32 tmrMsTimeGet(void);

/* Memory access gadgets */
UINT32 read16(void *);
UINT32 read32(void *);
UINT64 read64(void *);
void   write16(void *, UINT16);
void   write32(void *, UINT32);
void   write64(void *, UINT64);
UINT32 bitsRead(UINT8 *, UINT32, UINT32);
void   bitsWrite(UINT8 *, UINT32, UINT32, UINT32);

#endif  /* __GENERAL_H__ */

