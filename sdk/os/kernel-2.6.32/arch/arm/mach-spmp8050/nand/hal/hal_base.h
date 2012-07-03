#ifndef __NF_HAL_BASE_H__
#define __NF_HAL_BASE_H__

#include <mach/regs-gpio.h>

//=============================================================================
//
//      hal_base.h
//
//      NF HAL Base API
//
//=============================================================================
#define	SPMP_DEBUG_PRINT(fmt, args...) 	 printk("Debug %d[%s]: " fmt, __LINE__, __FUNCTION__, ##args)

#ifndef SCU_A_BASE
#define	SCU_A_BASE					(IO3_ADDRESS(0x7000))
#endif

typedef struct NFC_StartAddr_Map_
{
	unsigned long nf_physAddr;
	unsigned long nf_virtAddr;
	
	unsigned long bch_physAddr;
	unsigned long bch_virtAddr;
}NFC_StartAddr_Map;

extern NFC_StartAddr_Map nf_addr_remap;

#define NAND_S330_BASE				(nf_addr_remap.nf_virtAddr)
#define NAND_S330_BASE1				(IO3_ADDRESS(0x9000))
#define BCH_BASE_ADDRESS 			(nf_addr_remap.bch_virtAddr)//BCH address

#ifdef	CYGPKG_REDBOOT
#define nf_mutex_t INT32
#else
#include <linux/semaphore.h>
#include <linux/wait.h>
typedef struct semaphore nf_mutex_t;
#endif

#define		memFree(ptr)		kfree(ptr)
#define		memAlloc(size)		kmalloc((size), GFP_KERNEL)

typedef struct nf_flag_t_
{
	wait_queue_head_t queue;
	unsigned int flag;
} nf_flag_t;

//////////////////////////////////////////////////////////////////////////////////
extern unsigned int debug_flag;
extern void print_buf(unsigned char* buf,int len);
#define DEAD_LOCK() {SPMP_DEBUG_PRINT("Dead Lock");while(1);}
#define NF_TRACE(flag, fmt, args...) { if( (flag & 0xff) ==1) printk("NF TRACE %d[%s]: " fmt, __LINE__, __FUNCTION__, ##args);\
	if( (flag & 0xf00) ==0x100) DEAD_LOCK();}
#define DUMP_NF_BUFFER(flag,ptr1,len1, ptr2, len2) { \
			if( (flag & 0xff) ==1)			\
			{								\
				SPMP_DEBUG_PRINT("DumpBuffer:(%x)\n", flag);			\
				if(ptr1!=NULL)				\
					print_buf((unsigned char*)ptr1, len1); \
				if(ptr2!=NULL)				\
					print_buf((unsigned char*)ptr2,len2);	\
				if( (flag & 0xf00) ==0x100)		\
					DEAD_LOCK();			\
			}								\
		}
//////////////////////////////////////////////////////////////////////////////////

//#define HAL_READ_UINT32( _register_, _value_ ) ((_value_) = *((volatile unsigned int *)(_register_)))
//#define HAL_WRITE_UINT32( _register_, _value_ ) (*((volatile unsigned int *)(_register_)) = (_value_))

extern void nand_cache_sync(void);
extern void nand_cache_invalidate(void);
extern void cache_sync(void);
extern void cache_invalidate(void);

extern unsigned int gpio_get_dir(unsigned int aGrp, unsigned int aPin);
extern void gpio_set_data(unsigned int aGrp, unsigned int aPin, unsigned int aData);
extern void flag_setbits(unsigned int *flag, unsigned int value);
#endif // ifndef __NF_HAL_BASE_H__
// End of hal_base.h

