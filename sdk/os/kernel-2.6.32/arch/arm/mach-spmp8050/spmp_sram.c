#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>

#include <asm/tlb.h>
#include <asm/cacheflush.h>

#include <asm/mach/map.h>

#include <mach/spmp_sram.h>
#include <mach/hardware.h>


#define SPMP_SRAM_PA           SRAM_START
#define SPMP_SRAM_VA           SRAM_ADDRESS(0x0)

#define ROUND_DOWN(value,boundary)	((value) & (~((boundary)-1)))

static unsigned long spmp_sram_base;
static unsigned long spmp_sram_size;
static unsigned long spmp_sram_ceil;

void __init spmp_detect_sram(void)
{
	spmp_sram_base = SPMP_SRAM_VA;
	spmp_sram_size = 0x8000; /* 32K */
	spmp_sram_ceil = spmp_sram_base + spmp_sram_size;
}

static struct map_desc spmp_sram_io_desc[] __initdata = {
	{	/* .length gets filled in at runtime */
		.virtual	= SPMP_SRAM_VA,
		.pfn		= __phys_to_pfn(SPMP_SRAM_PA),
		.type		= MT_MEMORY
	}
};

/*
 * Note that we cannot use ioremap for SRAM, as clock init needs SRAM early.
 */
void __init spmp_map_sram(void)
{
	unsigned long base;

	if (spmp_sram_size == 0)
		return;

	spmp_sram_io_desc[0].virtual = SPMP_SRAM_VA;
	base = SPMP_SRAM_PA;
	base = ROUND_DOWN(base, PAGE_SIZE);
	spmp_sram_io_desc[0].pfn = __phys_to_pfn(base);

	spmp_sram_io_desc[0].length = 1024 * 1024;	/* Use section desc */
	iotable_init(spmp_sram_io_desc, ARRAY_SIZE(spmp_sram_io_desc));

	printk(KERN_INFO "SRAM: Mapped pa 0x%08lx to va 0x%08lx size: 0x%lx\n",
	__pfn_to_phys(spmp_sram_io_desc[0].pfn),
	spmp_sram_io_desc[0].virtual,
	       spmp_sram_io_desc[0].length);

	/*
	 * Normally devicemaps_init() would flush caches and tlb after
	 * mdesc->map_io(), but since we're called from map_io(), we
	 * must do it here.
	 */
	local_flush_tlb_all();
	flush_cache_all();

	/*
	 * Looks like we need to preserve some bootloader code at the
	 * beginning of SRAM for jumping to flash for reboot to work...
	 */
	printk(KERN_INFO "SRAM: Mapped va 0x%08lx size: 0x%lx\n",spmp_sram_base,spmp_sram_size);

	memset((void *)spmp_sram_base, 0, spmp_sram_size);
}
void * spmp_sram_push(void * start, unsigned long size)
{
	if (size > (spmp_sram_ceil - (spmp_sram_base))) {
		printk(KERN_ERR "Not enough space in SRAM\n");
		return NULL;
	}

	spmp_sram_ceil -= size;
	spmp_sram_ceil = ROUND_DOWN(spmp_sram_ceil, sizeof(void *));
	memcpy((void *)spmp_sram_ceil, start, size);
//	flush_icache_range((unsigned long)start, (unsigned long)(start + size));

	return (void *)spmp_sram_ceil;
}


//static void (*_omap_sram_reprogram_clock)(u32 dpllctl, u32 ckctl);

//void spmp_sram_reprogram_clock(u32 dpllctl, u32 ckctl)
//{
/*
if (!_omap_sram_reprogram_clock)
		omap_sram_error();

	_omap_sram_reprogram_clock(dpllctl, ckctl);
*/	
//}

int __init spmp_sram_prog_init(void)
{
/*
	_omap_sram_reprogram_clock =
			omap_sram_push(omap1_sram_reprogram_clock,
					omap1_sram_reprogram_clock_sz);
*/		
	return 0;

}

int __init spmp_sram_init(void)
{
	spmp_detect_sram();
	spmp_map_sram();
	spmp_sram_prog_init();

	return 0;
}

