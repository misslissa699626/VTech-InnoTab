/*
 * arch/arm/mach-gpl32900/snapshot.c
 *
 *  Copyright (C) 2011  Solutions, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/pm.h>
#include <linux/gpfb_param.h>
#include <asm/cacheflush.h>
#include <mach/gp_ceva.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_timer.h>
#include <mach/hardware.h>
#include <mach/gp_board.h>

static gp_board_system_t *gp_system_func;

static struct timeval tv;
static u8 check_part_gpfb = 0;
static u8 check_part_snapimg1 = 0;
static u8 check_part_snapimg2 = 0;

static struct savearea
{
    unsigned long bootflag_area;
    unsigned long bootflag_size;
    unsigned long snapshot_area;
    unsigned long snapshot_size;
} savearea[GPFB_SNAPSHOT_NUM];

#define MMP_IRQ_VIC0_BASE			IO0_ADDRESS(0x10000)
#define MMP_IRQ_VIC1_BASE			IO0_ADDRESS(0x20000)
#define MMP_IRQENABLE				0x10
#define MMP_IRQENABLECLEAR			0x14
#define CYG_DEVICE_IRQ0_EnableSet \
    (*(volatile unsigned int *) (MMP_IRQ_VIC0_BASE + MMP_IRQENABLE))
#define CYG_DEVICE_IRQ0_EnableClear \
	(*(volatile unsigned int *) (MMP_IRQ_VIC0_BASE + MMP_IRQENABLECLEAR))
#define CYG_DEVICE_IRQ1_EnableSet \
	(*(volatile unsigned int *) (MMP_IRQ_VIC1_BASE + MMP_IRQENABLE))
#define CYG_DEVICE_IRQ1_EnableClear \
	(*(volatile unsigned int *) (MMP_IRQ_VIC1_BASE + MMP_IRQENABLECLEAR))

#ifdef CONFIG_PM_GPFB_DEBUG
#include <linux/serial_reg.h>
#include <mach/hal/hal_uart.h> 
#define UART_ID		0
#define BOTH_EMPTY	(UART_LSR_TEMT | UART_LSR_THRE)

static regs_uart_t* pUartPorts[]= {
	((regs_uart_t*)(UART2_BASE)),
	((regs_uart_t*)(UART0_BASE)),
};

static void gpfb_putchar(char c)
{
	unsigned long status;

	if (c == '\n')
	 	gpfb_putc('\r');
   
	/* Wait up to 2ms for the character(s)  to be sent. */
	do {
		status = pUartPorts[UART_ID]->regLSR;
	} while ((status & BOTH_EMPTY) != BOTH_EMPTY);

	pUartPorts[UART_ID]->regTHR = c;
}

static void dbg_regdump(void)
{
	int i;
	scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	scubReg_t *pScubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	scucReg_t *pScucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;

	gpfb_printf("+++++ IRQ +++++\n");
	gpfb_printf("    VIC0 IRQ EnableSet=0x%08x\n", CYG_DEVICE_IRQ0_EnableSet);
	gpfb_printf("    VIC0 IRQ EnableClear=0x%08x\n", CYG_DEVICE_IRQ0_EnableClear);
	gpfb_printf("    VIC1 IRQ EnableSet=0x%08x\n", CYG_DEVICE_IRQ1_EnableSet);
	gpfb_printf("    VIC1 IRQ EnableClear=0x%08x\n", CYG_DEVICE_IRQ1_EnableClear);
	gpfb_printf("+++++ TIMER +++++\n");
	for (i = 0; i < 4; i++) {
		timerReg_t *ptimerReg =
			(timerReg_t *)(LOGI_ADDR_TIMER_REG + (i * LOGI_TIMER_OFFSET));
		gpfb_printf("    TIMER%d tmCtr=0x%08x\n", i, ptimerReg->tmCtr);
		gpfb_printf("    TIMER%d tmPsr=0x%08x\n", i, ptimerReg->tmPsr);
		gpfb_printf("    TIMER%d tmLdr=0x%08x\n", i, ptimerReg->tmLdr);
		gpfb_printf("    TIMER%d tmVlr=0x%08x\n", i, ptimerReg->tmVlr);
		gpfb_printf("    TIMER%d tmCmp=0x%08x\n", i, ptimerReg->tmCmp);
	}

	gpfb_printf("+++++ CLOCK +++++\n");
	gpfb_printf("    SCUA PeriClockEnable=0x%08x\n", pScuaReg->scuaPeriClkEn);
	gpfb_printf("    SCUB PeriClockEnable=0x%08x\n", pScubReg->scubPeriClkEn);
	gpfb_printf("    SCUC PeriClockEnable=0x%08x\n", pScucReg->scucPeriClkEn);
	gpfb_printf("    SCUA PeriClockEnable2=0x%08x\n", pScuaReg->scuaPeriClkEn2);
	gpfb_printf("    SCUB SysCntEn=0x%08x\n", pScubReg->scubSysCntEn);
	gpfb_printf("    SCUC CevaCntEn=0x%08x\n", pScucReg->scucCevaCntEn);
	gpfb_printf("+++++ GPIO +++++\n");
	gpfb_printf("    SCUB_GPIO0_IE=0x%08x\n", SCUB_GPIO0_IE);
	gpfb_printf("    SCUB_GPIO0_DS=0x%08x\n", SCUB_GPIO0_DS);
	gpfb_printf("    SCUB_GPIO0_PE=0x%08x\n", SCUB_GPIO0_PE);
	gpfb_printf("    SCUB_GPIO0_PS=0x%08x\n", SCUB_GPIO0_PS);
	gpfb_printf("    SCUB_GPIO1_IE=0x%08x\n", SCUB_GPIO1_IE);
	gpfb_printf("    SCUB_GPIO1_DS=0x%08x\n", SCUB_GPIO1_DS);
	gpfb_printf("    SCUB_GPIO1_PE=0x%08x\n", SCUB_GPIO1_PE);
	gpfb_printf("    SCUB_GPIO1_PS=0x%08x\n", SCUB_GPIO1_PS);
	gpfb_printf("    SCUB_GPIO2_IE=0x%08x\n", SCUB_GPIO2_IE);
	gpfb_printf("    SCUB_GPIO2_DS=0x%08x\n", SCUB_GPIO2_DS);
	gpfb_printf("    SCUB_GPIO2_PE=0x%08x\n", SCUB_GPIO2_PE);
	gpfb_printf("    SCUB_GPIO2_PS=0x%08x\n", SCUB_GPIO2_PS);
	gpfb_printf("    SCUB_GPIO3_IE=0x%08x\n", SCUB_GPIO3_IE);
	gpfb_printf("    SCUB_GPIO3_DS=0x%08x\n", SCUB_GPIO3_DS);
	gpfb_printf("    SCUB_GPIO3_PE=0x%08x\n", SCUB_GPIO3_PE);
	gpfb_printf("    SCUB_GPIO3_PS=0x%08x\n", SCUB_GPIO3_PS);
	gpfb_printf("    SCUB_PIN_MUX=0x%08x\n", SCUB_PIN_MUX);
}
#else
#define gpfb_putchar   NULL
#define dbg_regdump()
#endif

/* Nand IF functions */
extern void NandAPPGetNandInfo(u32*, u32*);
extern int NandAppReadSector(u32, u16, u32, u8);
extern int NandAppInit(void);
extern void NandAppFlush(void);
extern int gp_sdcard_app_read_sector(u32, u32, u16, u32);
/* Chunk functions */
#include <mach/gp_chunkmem.h>
extern int gp_chunk_suspend(save_data_proc);

#define AH_START_LBA	0
#define AH_SIZE			(4096 / 512)	/* page alignment */

#define BOOT_DEVICE NAND_BOOT
#define NAND_BOOT 0
#define SDC_BOOT  1

static int app_header_read(void *buf)
{
	#if BOOT_DEVICE == NAND_BOOT
	u8 mode = 0;	/* 0: for kernel mode, 1: for user mode */
	#endif
	int ret = 0;

	gpfb_printf("%s:%d\n", __func__, __LINE__);
	#if BOOT_DEVICE == NAND_BOOT 
	NandAppFlush();
	#endif
	gpfb_printf("%s:%d\n", __func__, __LINE__);
	#if BOOT_DEVICE == NAND_BOOT 
	ret = NandAppReadSector(AH_START_LBA, AH_SIZE, (u32)buf, mode);
	#else
	ret = gp_sdcard_app_read_sector(0, AH_START_LBA, AH_SIZE, (u32)buf);
	#endif
	gpfb_printf("%s:%d\n", __func__, __LINE__);
	flush_cache_all();

	gpfb_printf("[%s][%d] ret=%d\n", __func__, __LINE__, ret);

#if 0
	{	/* debug dump */
		int i;
		u8 *p = (u8*)buf;
		gpfb_printf("---------- App Header dump ------------ \n");
		for (i = 0; i < 128; i++) {
			if (!(i % 8))
				printk("\n%08x:  ", i);
			printk("%02x ", p[i]);
		}
		gpfb_printf("\n---------------------------------------\n");
	}
#endif

	return ret;
}

#define AH_PARTTYPE_KERNEL		0	/* kernel area */
#define AH_PARTTYPE_GPFB		2	/* hibdrv.bin */
#define AH_PARTTYPE_SNAPIMG		3	/* snapshot image 1 */
#define AH_PARTTYPE_HIBIMG		4	/* snapshot image 2 */
#define AH_PARTTYPE_GPFBFLAG	5	/* reserved */

#define AH_OFFS_HTAG_0			 0	/* App Header Tag: 8bit access */
#define AH_OFFS_HTAG_1			 1	/* App Header Tag: 8bit access */
#define AH_OFFS_HTAG_2			 2	/* App Header Tag: 8bit access */
#define AH_OFFS_HTAG_3			 3	/* App Header Tag: 8bit access */
#define AH_OFFS_PART_N			12	/* App Part Nums: 16bit access */
#define AH_OFFS_PART_START_SECT_ID(x)	(24 + ((x) * 16)) /* PartX Start Sect ID: 32bit access */
#define AH_OFFS_PART_IMAGE_SIZE(x)	(28 + ((x) * 16)) /* PartX Image Size: 32bit access */
#define AH_OFFS_PART_TYPE(x)		(32 + ((x) * 16)) /* PartX Image Size: 8bit access */

#define HDR_APP_IMAGE_TYPE	296


#if 0
extern SINT32 nf_hdr_get_from_bt_area(UINT8* header);
static SINT32 Snapshot_Nand_header_parse(void)
{
	void * Buffer = NULL;
	SINT32 ret;
	SINT32 work_size = 10 * 1024;
	SINT32 ShiftSector = 0;
	
	Buffer = kmalloc(work_size, GFP_KERNEL);
	if(Buffer == NULL)
	{
        gpfb_printf(KERN_ERR "Snapshot_Nand_header_parse Kmalloc Fail!! \n");
		return 0;
	}
	ret = nf_hdr_get_from_bt_area(Buffer);
	if(ret!=0)
	{
		gpfb_printf ("==Get nand info from boot head failed!!==\n");
		kfree(Buffer);
		return 0;
	}
	
	if (*(u8*)(Buffer + HDR_APP_IMAGE_TYPE) == 1)
	{
		ShiftSector = 1;
		gpfb_printf("========New APP Parttion Format========\n");
	}else{
		ShiftSector = 0;
		gpfb_printf("========Old APP Parttion Format========\n");
	}
	
	kfree(Buffer);
	
	return ShiftSector;
}
#endif

static int app_header_check(void *buf,
	u32 *hibdrv_area_sect, u32 *hibdrv_area_size)
{
	int partno = 0, i;
	u32 sect_id = 0, sect_size = 0;
	u32 area_total_size;
	SINT32 ShiftSector = 0;
	#if BOOT_DEVICE == NAND_BOOT
	u32 nand_blk_pages, nand_page_size;
	#endif


	check_part_gpfb = 0;
	check_part_snapimg1 = 0;
	check_part_snapimg2 = 0;

	if (buf == NULL) {
		printk("[%s][%d] no Memory\n", __func__, __LINE__);
		return -ENOMEM;
	}
	
	/* check Header Tag */
	if ((*(u8*)(buf + AH_OFFS_HTAG_0) != 'G') ||
	    (*(u8*)(buf + AH_OFFS_HTAG_1) != 'P') ||
	    (*(u8*)(buf + AH_OFFS_HTAG_2) != 'A') ||
	    (*(u8*)(buf + AH_OFFS_HTAG_3) != 'P')) {
		printk("[%s][%d] App Header bad format\n", __func__, __LINE__);
		return -EINVAL;
	}

	#if 0
	ShiftSector = Snapshot_Nand_header_parse();
	#else
	ShiftSector = 1;
	#endif
	
#if BOOT_DEVICE == NAND_BOOT	/* Read Nand Info*/
	NandAPPGetNandInfo(&nand_blk_pages, &nand_page_size);
	gpfb_printf("[%s][%d] nand info: page= %d byte, blk= %d pages\n", __func__, __LINE__, nand_page_size, nand_blk_pages);
#endif

	partno = *(u16*)(buf + AH_OFFS_PART_N);
	gpfb_printf("[%s][%d] partno=%d\n", __func__, __LINE__, partno);

	for (i = 0; i < partno; i++) {
		u8 parttype = *(u8*)(buf + AH_OFFS_PART_TYPE(i));
		sect_id = *(u32*)(buf + AH_OFFS_PART_START_SECT_ID(i));
		sect_size = *(u32*)(buf + AH_OFFS_PART_IMAGE_SIZE(i));
		switch (parttype) {
		case AH_PARTTYPE_GPFB:
			/* hibdrv */
			/* hibdrv area & size */
			*hibdrv_area_sect = sect_id + ShiftSector;
			*hibdrv_area_size = sect_size;
			check_part_gpfb = 1;
			gpfb_printf("[%s][%d] part gpfb: sect=%x, size=%x\n",
				__func__, __LINE__, sect_id, sect_size);
			break;
		case AH_PARTTYPE_SNAPIMG:
			/* snapshot image 1 */
			savearea[0].bootflag_area = sect_id * 512;
			area_total_size = sect_size * 512;
			#if BOOT_DEVICE == NAND_BOOT
    		savearea[0].bootflag_size = (nand_blk_pages * nand_page_size);
			#else
			savearea[0].bootflag_size = 1*4096;
			#endif
			savearea[0].snapshot_area =
				savearea[0].bootflag_area + savearea[0].bootflag_size;
			savearea[0].snapshot_size =
				area_total_size - savearea[0].bootflag_size;
			check_part_snapimg1 = 1;
			gpfb_printf("[%s][%d] part snapimg: sect=%x, size=%x\n",
				__func__, __LINE__, sect_id, sect_size);
			break;
		case AH_PARTTYPE_HIBIMG:
			/* snapshot image 2 */
			savearea[1].bootflag_area = sect_id * 512;
			area_total_size = sect_size * 512;
			
			#if BOOT_DEVICE == NAND_BOOT
    		savearea[1].bootflag_size = (nand_blk_pages * nand_page_size);
			#else
			savearea[1].bootflag_size = 1*4096;
			#endif
			
			savearea[1].snapshot_area =
				savearea[1].bootflag_area + savearea[1].bootflag_size;
			savearea[1].snapshot_size =
				area_total_size - savearea[1].bootflag_size;
			check_part_snapimg2 = 1;
			gpfb_printf("[%s][%d] part hibimg: sect=%x, size=%x\n",
				__func__, __LINE__, sect_id, sect_size);
			break;
		case AH_PARTTYPE_GPFBFLAG:
			/* reserved */
			break;
		default:
			break;
		}
	}

	if ( check_part_gpfb != 1 ||
		(check_part_snapimg1 != 1 && check_part_snapimg2 != 1)) {
		printk("[%s][%d] Partition not find.\n", __func__, __LINE__);
		return -EINVAL;
	}

	return 0;
}

static int hibdrv_copy_to_sram(void *buf, u32 start_sect, u32 work_size)
{
	int ret = 0;
	#if BOOT_DEVICE == NAND_BOOT
	u8 mode = 0;	/* 0: for kernel mode, 1: for user mode */
	#endif
	u32 hibdrv_size = 0;
	u32 sect_num = 0;

	if (work_size < 512)
		return -ENOMEM;

	gpfb_printf("%s:%d\n", __func__, __LINE__);
	#if BOOT_DEVICE == NAND_BOOT 
	NandAppFlush();
	#endif
	gpfb_printf("%s:%d: start_sect=%x, buf=%p\n", __func__, __LINE__, start_sect, buf);

	/* GPH header read */
	#if BOOT_DEVICE == NAND_BOOT 
	ret = NandAppReadSector(start_sect, 1, (u32)buf, mode);
	#else
	ret = gp_sdcard_app_read_sector(0, start_sect, 1, (u32)buf);
	#endif
	
	if (ret)
		return -EIO;

	hibdrv_size = *(u32*)(buf + GPFB_HEADER_COPY_SIZE);
	
	gpfb_printf("%s:%d: hibdrv_size=%x work_size=%x\n", __func__, __LINE__, hibdrv_size, work_size);

	if (hibdrv_size > work_size)
		return -ENOMEM;

	/* sector aliment */
	sect_num = hibdrv_size / 512;
	if (hibdrv_size % 512)
		sect_num++;

	/* GPH read (size - 1sector) */
	#if BOOT_DEVICE == NAND_BOOT 
	ret = NandAppReadSector((start_sect + 1), (sect_num - 1), 
				(u32)(buf + 512), mode);
	#else
	ret = gp_sdcard_app_read_sector(0, (start_sect + 1), (sect_num - 1), (u32)(buf + 512));
	#endif
	gpfb_printf("%s:%d: ret=%d\n", __func__, __LINE__, ret);

	if (ret)
		return -EIO;

#if 0
	{	/* debug dump */
		int i;
		u8 *p = (u8*)buf;
		gpfb_printf("---------- Copy to hibdrv dump ------------ \n");
		for (i = 0; i < 128; i++) {
			if (!(i % 8))
				printk("\n%08x:  ", i);
			printk("%02x ", p[i]);
		}
		gpfb_printf("\n---------------------------------------\n");
	}
#endif
	return 0;
}

static int gpfb_drv_init(void)
{
	int ret = 0;
	gp_ceva_nop();

#ifndef GPFB_HIBDRV_FLOATING
	void *buf = (void *)GPFB_HIBDRV_VIRT;
	u32 hibdrv_area_size = 0, hibdrv_area_sect_id = 0;
	
	#if BOOT_DEVICE == NAND_BOOT  
	ret = NandAppInit();
	if (ret != 0) {
		printk("[%s][%d] NandAppInit error. (return: %d)\n",
			__func__, __LINE__, ret);
		return ret;
	}
	#endif
	/* header read to NAND */
	ret = app_header_read(buf);
	if (ret != 0)
		return ret;

	/* header format check & setting */
	ret = app_header_check(buf, &hibdrv_area_sect_id, &hibdrv_area_size);
	if (ret != 0)
		return ret;
	
	/* hibdrv copy to SRAM */
	ret = hibdrv_copy_to_sram(buf, hibdrv_area_sect_id, hibdrv_area_size);
	if (ret != 0)
		return ret;
#endif

	do_gettimeofday(&tv);

	return ret;
}


static int gpfb_drv_load (void *buf, size_t size)
{
#ifdef GPFB_HIBDRV_DEV_LOAD
	int ret = 0;
	
#if 0
	/* Load to file. for debug */
	ret = gpfb_dev_load("/media/sdcarda1/hibdrv.bin", buf, size);
#else
	/* Load to NAND */
	u32 hibdrv_area_size = 0, hibdrv_area_sect_id = 0;
	
	#if BOOT_DEVICE == NAND_BOOT  
	ret = NandAppInit();
	if (ret != 0) {
		printk("[%s][%d] NandAppInit error. (return: %d)\n",
			__func__, __LINE__, ret);
		return ret;
	}
	#endif
	
	/* header read to NAND */
	ret = app_header_read(buf);
	if (ret != 0)
		return ret;

	/* header format check & setting */
	ret = app_header_check(buf, &hibdrv_area_sect_id, &hibdrv_area_size);
	if (ret != 0)
		return ret;
	
	/* hibdrv copy to SRAM */
	ret = hibdrv_copy_to_sram(buf, hibdrv_area_sect_id, size);
	if (ret != 0)
		return ret;
#endif

	return ret;
#else
	return 0;
#endif
}


static void gpfb_drv_uninit (void)
{
	return;
}

static int gpfb_device_suspend(void)
{
 	return 0;
}

static void gpfb_device_resume(void)
{
}

static void gpfb_chunk_save_mem(unsigned long addr, unsigned long size)
{
	unsigned long pfn, end;
    
    #if 0
	if (addr == 0) {
		addr = 0x4000000;
		size = 0x100000;
		gpfb_printf("chunk addr = 0, modify addr to %x, size = %x\n",addr,size);
	}
	#endif
	
	pfn = addr & PAGE_MASK;
	if (size >= PAGE_SIZE) {
		end = pfn + (size & PAGE_MASK);
		if (size % PAGE_SIZE)
			end += PAGE_SIZE;
	} else
		end = pfn + PAGE_SIZE;

	gpfb_printf("T-DEBUG [%s][%d] addr=0x%x, size=0x%x pfn=0x%x - 0x%x\n", __func__, __LINE__, addr, size, pfn,end);
	gpfb_set_savearea(pfn, end);
}

static int gpfb_snapshot(void)
{
	int ret = 0, i;
	unsigned int gpio_ie[4], gpio_ds[4], gpio_pe[4], gpio_ps[4], pin_mux;
	scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	scubReg_t *pScubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	scucReg_t *pScucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	scuaReg_t saveScuaReg;
	scubReg_t saveScubReg;
	scucReg_t saveScucReg;
	unsigned int irq0_es, irq1_es;
	unsigned int tmCtr[5], tmPsr[5], tmLdr[5], tmVlr[5], tmCmp[5];

	int gpfb_saveno = gpfb_param.bootflag_area;

	/* Save chunk memory */
	gp_chunk_suspend(gpfb_chunk_save_mem);

	/* save regs */
	/* irq */
	irq0_es = CYG_DEVICE_IRQ0_EnableSet;
	irq1_es = CYG_DEVICE_IRQ1_EnableSet;

	/* timer */
	for (i = 0; i < 5; i++) {
		timerReg_t *ptimerReg =
			(timerReg_t *)(LOGI_ADDR_TIMER_REG + (i * LOGI_TIMER_OFFSET));
		tmCtr[i] = ptimerReg->tmCtr;
		tmPsr[i] = ptimerReg->tmPsr;
		tmLdr[i] = ptimerReg->tmLdrVlr;
		tmVlr[i] = ptimerReg->tmIsr;
		tmCmp[i] = ptimerReg->tmCmp;
	}

	/* gpio */
	for (i = 0; i < 4; i++) {
		gpio_ie[i] = *(volatile unsigned int*)(SCU_B_BASE+0x100 + (i * 0x10));
		gpio_ds[i] = *(volatile unsigned int*)(SCU_B_BASE+0x104 + (i * 0x10));
		gpio_pe[i] = *(volatile unsigned int*)(SCU_B_BASE+0x108 + (i * 0x10));
		gpio_ps[i] = *(volatile unsigned int*)(SCU_B_BASE+0x10c + (i * 0x10));
	}
	pin_mux = SCUB_PIN_MUX;

	/* clock */
	saveScuaReg.scuaPeriClkEn = pScuaReg->scuaPeriClkEn;
	saveScuaReg.scuaPeriClkEn2 = pScuaReg->scuaPeriClkEn2;
	saveScubReg.scubPeriClkEn = pScubReg->scubPeriClkEn;
	saveScubReg.scubSysCntEn = pScubReg->scubSysCntEn;
	saveScucReg.scucPeriClkEn = pScucReg->scucPeriClkEn;
	saveScucReg.scucCevaCntEn = pScucReg->scucCevaCntEn;

	/* set param */
	gpfb_param.bootflag_area = savearea[gpfb_saveno].bootflag_area;
	gpfb_param.bootflag_size = savearea[gpfb_saveno].bootflag_size;
	gpfb_param.snapshot_area = savearea[gpfb_saveno].snapshot_area;
	gpfb_param.snapshot_size = savearea[gpfb_saveno].snapshot_size;

	gpfb_param.snapshot_ver = tv.tv_usec;

	//gpfb_param.private = UART2_BASE; //debug uart : for hibdrv

	dbg_regdump();

	if (gp_system_func->prepare) {
		gp_system_func->prepare();
	}

	gpfb_printf("[%s][%d] hibdrv_snapshot() start\n",
			__func__, __LINE__);

	ret = hibdrv_snapshot();
	gpfb_printf("[%s][%d] hibdrv_snapshot() end(return=%d)\n",
		__func__, __LINE__, ret);

	if (gp_system_func->finish) {
		gp_system_func->finish();
	}

	/* restore regs */
	/* clock */
	pScuaReg->scuaPeriClkEn = saveScuaReg.scuaPeriClkEn;
	pScuaReg->scuaPeriClkEn2 = saveScuaReg.scuaPeriClkEn2;
	pScubReg->scubPeriClkEn = saveScubReg.scubPeriClkEn;
	pScubReg->scubSysCntEn = saveScubReg.scubSysCntEn;
	pScucReg->scucPeriClkEn = saveScucReg.scucPeriClkEn;
	pScucReg->scucCevaCntEn = saveScucReg.scucCevaCntEn;
	/* gpio */
	for (i = 0; i < 4; i++) {
		*(volatile unsigned int*)(SCU_B_BASE+0x100 + (i * 0x10)) = gpio_ie[i];
		*(volatile unsigned int*)(SCU_B_BASE+0x104 + (i * 0x10)) = gpio_ds[i];
		*(volatile unsigned int*)(SCU_B_BASE+0x108 + (i * 0x10)) = gpio_pe[i];
		*(volatile unsigned int*)(SCU_B_BASE+0x10c + (i * 0x10)) = gpio_ps[i];
	}
	SCUB_PIN_MUX = pin_mux;
	/* irq */
	CYG_DEVICE_IRQ0_EnableSet = irq0_es;
	CYG_DEVICE_IRQ1_EnableSet = irq1_es;
	/* timer */
	for (i = 0; i < 5; i++) {
		timerReg_t *ptimerReg =
			(timerReg_t *)(LOGI_ADDR_TIMER_REG + (i * LOGI_TIMER_OFFSET));
		ptimerReg->tmCtr = tmCtr[i];
		ptimerReg->tmPsr = tmPsr[i];
		ptimerReg->tmLdrVlr = tmLdr[i];
		ptimerReg->tmIsr = tmVlr[i];
		ptimerReg->tmCmp = tmCmp[i];
	}

	dbg_regdump();

	return ret;
}

static struct gpfb_ops gpfb_machine_ops = {
	.drv_load		= gpfb_drv_load,	/* T-DEBUG: for debug */
	.drv_init       = gpfb_drv_init,
	.device_suspend = gpfb_device_suspend,
	.device_resume  = gpfb_device_resume,
	.snapshot       = gpfb_snapshot,
	.drv_uninit     = gpfb_drv_uninit,
	.putc           = gpfb_putchar,
};

static int __init gpfb_machine_init(void)
{

	gp_system_func = gp_board_get_config("sys_pwr",gp_board_system_t);
	if (gp_system_func == NULL) {
		printk(KERN_ERR "Can not get System Power config function\n");
		return -EINVAL;
	}
	
	return gpfb_register_machine(&gpfb_machine_ops);
}

static void __exit gpfb_machine_exit(void)
{
	gpfb_unregister_machine(&gpfb_machine_ops);
}


module_init(gpfb_machine_init);
module_exit(gpfb_machine_exit);
MODULE_AUTHOR("GP");
MODULE_DESCRIPTION("Snapshot Driver");
MODULE_LICENSE("GPL");
