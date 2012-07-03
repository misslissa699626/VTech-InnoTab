#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>

#include <asm/tlb.h>
#include <asm/cacheflush.h>

#include <asm/mach/map.h>

#include <mach/gp_sram.h>
#include <mach/hardware.h>


#define GP_SRAM_PA           SRAM_START
#define GP_SRAM_VA           SRAM_ADDRESS(0x0)

#define GP_CEVAL1_PA		CEVAL1_START
#define GP_CEVAL1_VA		CEVAL1_BASE

#define GP_CEVAL2_PA		CEVAL2_START
#define GP_CEVAL2_VA		CEVAL2_BASE

#define ROUND_DOWN(value,boundary)	((value) & (~((boundary)-1)))

static unsigned long gp_sram_base;
static unsigned long gp_sram_size;
static unsigned long gp_sram_ceil;

void __init gp_detect_sram(void)
{
	gp_sram_base = GP_SRAM_VA;
	//gp_sram_size = 0x8000; /* 32K */
    gp_sram_size = SRAM_SIZE; /* 16K */
	gp_sram_ceil = gp_sram_base + gp_sram_size;
}

static struct map_desc gp_sram_io_desc[] __initdata = {
	{	/* .length gets filled in at runtime */
		.virtual	= GP_SRAM_VA,
		.pfn		= __phys_to_pfn(GP_SRAM_PA),
		.length		= 0x4000,
		.type		= MT_MEMORY
	},
	{	/* .length gets filled in at runtime */
		.virtual	= GP_CEVAL1_VA,
		.pfn		= __phys_to_pfn(GP_CEVAL1_PA),
		.length		= 0x10000,
		.type		= MT_MEMORY
	},
	{	/* .length gets filled in at runtime */
		.virtual	= GP_CEVAL2_VA,
		.pfn		= __phys_to_pfn(GP_CEVAL2_PA),
		.length		= 0x4000,
		.type		= MT_MEMORY
	}
};

/*
 * Note that we cannot use ioremap for SRAM, as clock init needs SRAM early.
 */
void __init gp_map_sram(void)
{
	unsigned long base;

	if (gp_sram_size == 0)
		return;

	gp_sram_io_desc[0].virtual = GP_SRAM_VA;
	base = GP_SRAM_PA;
	base = ROUND_DOWN(base, PAGE_SIZE);
	gp_sram_io_desc[0].pfn = __phys_to_pfn(base);

	gp_sram_io_desc[0].length = 1024 * 1024;	/* Use section desc */

	gp_sram_io_desc[1].virtual = GP_CEVAL1_VA;
	base = GP_CEVAL1_PA;
	base = ROUND_DOWN(base, PAGE_SIZE);
	gp_sram_io_desc[1].pfn = __phys_to_pfn(base);

	gp_sram_io_desc[1].length = 1024 * 1024;	/* Use section desc */

	gp_sram_io_desc[2].virtual = GP_CEVAL2_VA;
	base = GP_CEVAL2_PA;
	base = ROUND_DOWN(base, PAGE_SIZE);
	gp_sram_io_desc[2].pfn = __phys_to_pfn(base);

	gp_sram_io_desc[2].length = 1024 * 1024;	/* Use section desc */

	iotable_init(gp_sram_io_desc, ARRAY_SIZE(gp_sram_io_desc));

	printk(KERN_INFO "SRAM: Mapped pa 0x%08lx to va 0x%08lx size: 0x%lx\n",
	__pfn_to_phys(gp_sram_io_desc[0].pfn),
	gp_sram_io_desc[0].virtual,
	       gp_sram_io_desc[0].length);

	printk(KERN_INFO "SRAM: Mapped pa 0x%08lx to va 0x%08lx size: 0x%lx\n",
	__pfn_to_phys(gp_sram_io_desc[1].pfn),
	gp_sram_io_desc[1].virtual,
	       gp_sram_io_desc[1].length);

	printk(KERN_INFO "SRAM: Mapped pa 0x%08lx to va 0x%08lx size: 0x%lx\n",
	__pfn_to_phys(gp_sram_io_desc[2].pfn),
	gp_sram_io_desc[2].virtual,
	       gp_sram_io_desc[2].length);

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
	printk(KERN_INFO "SRAM: Mapped va 0x%08lx size: 0x%lx\n",gp_sram_base,gp_sram_size);

	//memset((void *)gp_sram_base, 0, gp_sram_size);
}
void * gp_sram_push(void * start, unsigned long size)
{
	if (size > (gp_sram_ceil - (gp_sram_base))) {
		printk(KERN_ERR "Not enough space in SRAM\n");
		return NULL;
	}

	gp_sram_ceil -= size;
	gp_sram_ceil = ROUND_DOWN(gp_sram_ceil, sizeof(void *));
	memcpy((void *)gp_sram_ceil, start, size);
//	flush_icache_range((unsigned long)start, (unsigned long)(start + size));

	return (void *)gp_sram_ceil;
}

int __init gp_sram_prog_init(void)
{
	return 0;
}

int __init gp_sram_init(void)
{
	gp_detect_sram();
	gp_map_sram();
	gp_sram_prog_init();

	return 0;
}

