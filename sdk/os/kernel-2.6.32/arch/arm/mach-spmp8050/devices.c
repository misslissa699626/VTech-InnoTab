#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/signal.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/cacheflush.h>
#include <mach/pm.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/regs-sd.h>
#include <mach/spmp_gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-scu.h>
#include <mach/regs-lcd.h>
#include <mach/regs-usbhost.h>
#include <mach/regs-wdt.h>
#include <mach/sd.h>
#include <linux/i2c.h>
#include <linux/usb/android_composite.h>
#include <media/soc_camera.h>
#include <mach/spmp_sram.h>
#include <mach/spmptv.h>
#include <mach/gp_board.h>

struct spmpehci_platform_data {
	int (*init)(struct device *);
	void (*exit)(struct device *);
};

struct spmpohci_platform_data {
	int (*init)(struct device *);
	void (*exit)(struct device *);
};

struct spmpmci_platform_data {
	int(*cd_setup)(void *mmc_host, int on);
	int(*card_inserted)(void *mmc_host);
	int(*card_readonly)(void *mmc_host);
	void(*set_power)(void *mmc_host, int state);
};
#if 0
/*****************************************************************
 * USB HOST DEVICE SETTING
*****************************************************************/
static int usbhost_usage = 0;
static int spmp_usbhost_en(int en)
{
  if(en == 0)
  {
    if(usbhost_usage <= 1)
    {
 	  UH_CTRL = ~MASTER_EN;
      udelay(1000);
	  usbhost_usage = 0;
    }
	else
	{
	  usbhost_usage--;
	}
  }
  else
  {
    if(usbhost_usage == 0)
    {
 	  UH_CTRL = MASTER_EN;
	  udelay(1000);
    }
    usbhost_usage++;
  }

  return 0;
}
#endif
/*****************************************************************
 * USB HOST OHCI Info
*****************************************************************/
static int spmp_ohci_init(struct device *dev)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	//spmp_usbhost_en(1);
	return 0;
}

static void spmp_ohci_exit(struct device *dev)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	//spmp_usbhost_en(0);
}

static struct spmpohci_platform_data spmp_ohci_platform_data = {
    .init = spmp_ohci_init,
    .exit = spmp_ohci_exit,
};

static struct resource spmp_resource_ohci[] = {
	[0] = {
		.start  = 0x93004080,
		.end    = 0x930040FF,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_USB_OHCI,
		.end    = IRQ_USB_OHCI,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 spmp_ohci_dma_mask = DMA_BIT_MASK(32);

struct platform_device spmp_device_ohci = {
	.name		= "spmp-ohci",
	.id		= -1,
	.dev		= {
		.dma_mask = &spmp_ohci_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data		= &spmp_ohci_platform_data,
	},
	.num_resources  = ARRAY_SIZE(spmp_resource_ohci),
	.resource       = spmp_resource_ohci,
};

static int __init spmp_regdev_ohci(void)
{
    int ret;
	ret = platform_device_register(&spmp_device_ohci);
	if (ret)
		dev_err(&(spmp_device_ohci.dev), "unable to register device: %d\n", ret);
	return ret;
}


/*Android */
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.vendor = "Gplus",
	.product = "GPlus Phone",
	.release = 0x0100,
	.nluns = 1,
};
	
static struct platform_device usb_mass_storage_device = {
	.name = "usb_mass_storage",
	.id = -1,
	.dev = {
		.platform_data = &mass_storage_pdata,
	},
};

#if 0
static struct platform_device fsg_platform_device =  
{  
    .name = "usb_mass_storage",  
    .id   = -1,  
};  
#endif

struct usb_composite_product {
	u16 product_id;
	char** functions;
};

#if 1
static char *usb_functions_adb[] = { 
  "adb",
};
#endif
#if 1
static char *usb_functions_mass_storage[] = {
    "usb_mass_storage",
};
#endif
#if 1
static char *usb_functions_all[] = {
    "adb",
	"usb_mass_storage",
};
#endif
#if 0
static struct android_usb_product usb_products[] = {
	{
		.product_id = 0x8053,
		.num_functions = ARRAY_SIZE(usb_functions_mass_storage),
		.functions = usb_functions_mass_storage, /* "usb_mass_storage" */
	},
};



static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id    = 0x18d1,
	.product_id    = 0x0D02,
	.version    = 0x0100,
	.serial_number  = "HT9CTP812056",
	.product_name        = "Nexus One",
	.manufacturer_name    = "Google, Inc.",
	.num_products = 1,
	.products = usb_products,
	.num_functions = 1,  /* adb + mass_storage */
	.functions = usb_functions_mass_storage,
};
#else
static struct android_usb_product usb_products[] = {
	{
		.product_id = 0x8052,
		.num_functions = ARRAY_SIZE(usb_functions_adb),
		.functions = usb_functions_adb, /* "usb_adb" only */
	},
	{
		.product_id = 0x8053,
		.num_functions = ARRAY_SIZE(usb_functions_mass_storage),
		.functions = usb_functions_mass_storage, /* "usb_mass_storage" */
	},
};



static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id    = 0x18d1,
	.product_id    = 0x0D02,
	.version    = 0x0100,
	.serial_number  = "HT9CTP812056",
	.product_name        = "Nexus One",
	.manufacturer_name    = "Google, Inc.",
	.num_products = 2,
	.products = usb_products,
	.num_functions = 2,  /* adb + mass_storage */
	.functions = usb_functions_all,
};
#endif

struct platform_device spmp_device_android = {
	.name		= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init spmp_regdev_android(void)
{		
	
	int ret;

	ret = platform_device_register(&usb_mass_storage_device);
	if (ret){
		dev_err(&(usb_mass_storage_device.dev), "unable to register device: %d\n", ret);
	}	
	ret = platform_device_register(&spmp_device_android);
	if (ret)
		dev_err(&(spmp_device_android.dev), "unable to register device: %d\n", ret);
	return ret;
}
/*****************************************************************
 * USB HOST EHCI Info
*****************************************************************/
static int spmp_ehci_init(struct device *dev)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	//return spmp_usbhost_en(1);
	return 0;
}

static void spmp_ehci_exit(struct device *dev)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	//spmp_usbhost_en(0);
}

static struct spmpehci_platform_data spmp_ehci_platform_data = {
    .init = spmp_ehci_init,
    .exit = spmp_ehci_exit,
};

static struct resource spmp_resource_ehci[] = {
	[0] = {
		.start  = 0x93004100,
		.end    = 0x930041FF,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_USB_EHCI,
		.end    = IRQ_USB_EHCI,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 spmp_ehci_dma_mask = DMA_BIT_MASK(32);

struct platform_device spmp_device_ehci = {
	.name		= "spmp-ehci",
	.id		= -1,
	.dev		= {
		.dma_mask = &spmp_ehci_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data		= &spmp_ehci_platform_data,
	},
	.num_resources  = ARRAY_SIZE(spmp_resource_ehci),
	.resource       = spmp_resource_ehci,
};

static int __init spmp_regdev_ehci(void)
{
   int ret;
	ret = platform_device_register(&spmp_device_ehci);
	if (ret)
	dev_err(&(spmp_device_ehci.dev), "unable to register device: %d\n", ret);
	return ret;
}

static struct resource spmp_resource_udc[] = {
	[0] = {
		.start  = 0x93006000,
		.end   = 0x93006FFF,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_USB_DEV,
		.end    = IRQ_USB_DEV,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device spmp_device_udc = {
	.name		= "spmp-udc",
	.id		= -1,
	.dev		= {
		.dma_mask = &spmp_ehci_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(spmp_resource_udc),
	.resource       = spmp_resource_udc,
};

static int __init spmp_regdev_udc(void)
{
    int ret;
	ret = platform_device_register(&spmp_device_udc);
	if (ret)
		dev_err(&(spmp_device_udc.dev), "unable to register device: %d\n", ret);
	return ret;
}

/*****************************************************************
 * SD / SDIO Device Info
*****************************************************************/

#if 0
static struct sd_data_s spmpmci0_platdata = {
	.info = {
	         .device_id = 0,
             .p_addr = 0,
	         .v_addr = 0,
	         .dma_chan = -1,
	         .is_irq = 1,
	         .is_dmairq = 1,
	         .is_detectirq = 0,
	         .is_readonly = 0,
	         .clk_rate = 0,
	         .clk_div = 2,
	         .max_clkdiv = 256,
	         .real_rate = 0,
	         .dma_cb = NULL,
	         .detect_chan = -1,
	         .detect_delay = 20,
	         .detect_cb = NULL,
	},
	.ops = &spmpmci_ops0,
};

static struct resource spmp_resources_mci0[] = {
	[0] = {
		.start	= 0x92B0B000,
		.end	= 0x92B0CFFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SD0,
		.end	= IRQ_SD0,
		.flags	= IORESOURCE_IRQ,
	},
};

#if 0
static struct resource spmp_resources_mci1[] = {
	[0] = {
		.start	= 0x92B0C000,
		.end	= 0x92B0CFFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SD1,
		.end	= IRQ_SD1,
		.flags	= IORESOURCE_IRQ,
	},
};
#endif

static u64 spmp_mci0_dma_mask = DMA_BIT_MASK(32);
struct platform_device spmp_mci0_device = {
	.name	= "spmp-mci",
	.id		= 0,
	.dev	= {
		.platform_data		= &spmpmci0_platdata,
		.dma_mask = &spmp_mci0_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(spmp_resources_mci0),
	.resource	= spmp_resources_mci0,
};

#if 0
static u64 spmp_mci1_dma_mask = DMA_BIT_MASK(32);
struct platform_device spmp_mci1_device = {
	.name	= "spmp-mci",
	.id		= 1,
	.dev	= {
		.platform_data		= &spmpmci_platdata,
		.dma_mask = &spmp_mci1_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(spmp_resources_mci1),
	.resource	= spmp_resources_mci1,
};
#endif

static int __init spmp_regdev_sd0(void)
{
    int ret;
	ret = platform_device_register(&spmp_mci0_device);
	if (ret)
		dev_err(&(spmp_mci0_device.dev), "unable to register device: %d\n", ret);
	return ret;
}

#if 0
static int __init spmp_regdev_sd1(void)
{
    int ret;
	ret = platform_device_register(&spmp_mci1_device);
	if (ret)
		dev_err(&(spmp_mci1_device.dev), "unable to register device: %d\n", ret);
	return ret;
}
#endif
#endif
/*****************************************************************
 * APBDMAA APBDMAC Device Info
*****************************************************************/

/*****************************************************************
 * RTC Device Info
*****************************************************************/
static struct resource spmp_rtc_resources[] = {
	[0] = {
		.start  = 0x9000B000,
		.end	= 0x9000B000 + 0xFFF,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_REALTIME_CLOCK,
		.end    = IRQ_REALTIME_CLOCK,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device spmp_device_rtc = {
	.name		= "gp-rtc",
	.id		= -1,
	.num_resources  = ARRAY_SIZE(spmp_rtc_resources),
	.resource       = spmp_rtc_resources,
};

static int __init spmp_regdev_rtc(void)
{
    int ret;
	ret = platform_device_register(&spmp_device_rtc);
	if (ret)
		dev_err(&(spmp_device_rtc.dev), "unable to register device: %d\n", ret);
	return ret;
}

/*****************************************************************
 * SARADC Controller Device Info
*****************************************************************/
static struct resource spmp_saacc_resources[] = {
	[0] = {
		.start  = 0x9301F000,
		.end	= 0x9301F000 + 0xFFF,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SAACC,
		.end    = IRQ_SAACC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device spmp_device_saacc = {
	.name		= "spmp-saacc",
	.id		= -1,
	.num_resources  = ARRAY_SIZE(spmp_saacc_resources),
	.resource       = spmp_saacc_resources,
};

static int __init spmp_regdev_saacc(void)
{
    int ret;
	ret = platform_device_register(&spmp_device_saacc);
	if (ret)
		dev_err(&(spmp_device_saacc.dev), "unable to register device: %d\n", ret);
	return ret;
}

/*****************************************************************
 * I2C Info
*****************************************************************/
struct s3c2410_platform_i2c {
	int		bus_num;	/* bus number to use */
	unsigned int	flags;
	unsigned int	slave_addr;	/* slave address for controller */
	unsigned long	bus_freq;	/* standard bus frequency */
	unsigned long	max_freq;	/* max frequency for the bus */
	unsigned long	min_freq;	/* min frequency for the bus */
	unsigned int	sda_delay;	/* pclks (s3c2440 only) */

	void	(*cfg_gpio)(struct platform_device *dev);
};

static struct s3c2410_platform_i2c  i2cB_info = {
	.flags		= 0,
	.slave_addr	= 0x60,
	.bus_freq	= 100*1000,
	.max_freq	= 130*1000,
};

static struct resource spmp_i2cB_resource[] = {
	[0] = {
		.start = 0x92B03000,
		.end   = 0x92B03000 + 0xFFF,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C_C,
		.end   = IRQ_I2C_C,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device spmp_i2cB_device = {
	.name	= "spmp-i2cB",
	.id		= -1,
	.dev	= {
		.platform_data	= &i2cB_info,
	},
	.num_resources  = ARRAY_SIZE(spmp_i2cB_resource),
	.resource       = spmp_i2cB_resource,
};

/*****************************************************************
 * DISPLAY Module
*****************************************************************/
static u64 spmp_display_dma_mask = DMA_BIT_MASK(32);
#ifndef FB_USE_CHUNKMEM
static struct resource spmp_resource_display[] = {
	[0] = {
		.start  = FRAMEBUF_BASE,
		.end    = FRAMEBUF_BASE+FRAMEBUF_SIZE -1,
		.flags  = IORESOURCE_MEM,
	},
};
#endif

struct platform_device spmp_device_display = {
	.name		= "spmp-display",
	.id		= -1,
	.dev		= {
		.dma_mask = &spmp_display_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
#ifndef FB_USE_CHUNKMEM
	.num_resources  = ARRAY_SIZE(spmp_resource_display),
	.resource       = spmp_resource_display,
#endif
};

int __init spmp_regdev_display(void)
{
    int ret;
	ret = platform_device_register(&spmp_device_display);
	if (ret)
		dev_err(&(spmp_device_display.dev), "unable to register device: %d\n", ret);
	return ret;
}


/*****************************************************************
 * PM Module
*****************************************************************/
#ifdef CONFIG_PM
/* configuration for the IRQ mask over sleep */
extern unsigned long spmp_irqwake_int1mask;
extern unsigned long spmp_irqwake_int2mask;

#define SAVE(x)		sleep_save[SLEEP_SAVE_##x] = x
#define RESTORE(x)	x = sleep_save[SLEEP_SAVE_##x]
#define MMP_IRQ_VIC0_BASE			IO0_ADDRESS(0x10000)
#define MMP_IRQ_VIC1_BASE			IO0_ADDRESS(0x20000)
#define MMP_IRQENABLE				0x10
#define MMP_IRQENABLECLEAR		0x14


#define CYG_DEVICE_IRQ0_EnableSet \
    (*(volatile unsigned int *) (MMP_IRQ_VIC0_BASE + MMP_IRQENABLE))
    // Enable (1's only), write only
#define CYG_DEVICE_IRQ0_EnableClear \
    (*(volatile unsigned int *) (MMP_IRQ_VIC0_BASE + MMP_IRQENABLECLEAR))
    // Disable (1's only), write only

#define CYG_DEVICE_IRQ1_EnableSet \
    (*(volatile unsigned int *) (MMP_IRQ_VIC1_BASE + MMP_IRQENABLE))
    // Enable (1's only), write only
#define CYG_DEVICE_IRQ1_EnableClear \
    (*(volatile unsigned int *) (MMP_IRQ_VIC1_BASE + MMP_IRQENABLECLEAR))
    // Disable (1's only), write only
/*
 * List of global PXA peripheral registers to preserve.
 * More ones like CP and general purpose register values are preserved
 * with the stack pointer in sleep.S.
 */
enum {
	SLEEP_SAVE_SCUA_A_PERI_CLKEN,
	SLEEP_SAVE_SCUB_B_PERI_CLKEN,
	SLEEP_SAVE_SCUC_C_PERI_CLKEN,
	SLEEP_SAVE_COUNT
};
static void (*spmp_sram_suspend)(void) = NULL;
static void spmp_cpu_pm_suspend(void)
{
    extern void  spmp_cpu_suspend(void);
    unsigned long spmp_irqwakeup_int1mask;
    unsigned long spmp_irqwakeup_int2mask;
	local_irq_disable();
	local_fiq_disable();
      spmp_irqwakeup_int1mask = CYG_DEVICE_IRQ0_EnableSet;
      spmp_irqwakeup_int2mask = CYG_DEVICE_IRQ1_EnableSet;
      CYG_DEVICE_IRQ0_EnableClear = CYG_DEVICE_IRQ0_EnableSet & spmp_irqwake_int1mask;
      CYG_DEVICE_IRQ1_EnableClear = CYG_DEVICE_IRQ1_EnableSet & spmp_irqwake_int2mask;
	flush_cache_all();
	spmp_sram_suspend();
      CYG_DEVICE_IRQ0_EnableSet = spmp_irqwakeup_int1mask;
      CYG_DEVICE_IRQ1_EnableSet = spmp_irqwakeup_int2mask;
	local_irq_enable();
	local_fiq_enable();
	printk("suspend = %p\n",spmp_sram_suspend);
}

static void spmp_cpu_pm_save(unsigned long *sleep_save)
{
	SAVE(SCUA_A_PERI_CLKEN);
	SAVE(SCUB_B_PERI_CLKEN);
	SAVE(SCUC_C_PERI_CLKEN);
}

static void spmp_cpu_pm_restore(unsigned long *sleep_save)
{
	RESTORE(SCUA_A_PERI_CLKEN);
	RESTORE(SCUB_B_PERI_CLKEN);
	RESTORE(SCUC_C_PERI_CLKEN);
}

static void spmp_cpu_pm_enter(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_MEM:
	    spmp_cpu_pm_suspend();
		printk("exit suspend \n");
		break;
	}
}

static int spmp_cpu_pm_prepare(void)
{
	return 0;
}

static void spmp_cpu_pm_finish(void)
{
}

static struct spmp_cpu_pm_fns spmp8050_cpu_pm_fns = {
	.save_count	= SLEEP_SAVE_COUNT,
	.valid		= suspend_valid_only_mem,
	.save		= spmp_cpu_pm_save,
	.restore	= spmp_cpu_pm_restore,
	.enter		= spmp_cpu_pm_enter,
	.prepare	= spmp_cpu_pm_prepare,
	.finish		= spmp_cpu_pm_finish,
};

static void __init spmp_init_pm(void)
{
	spmp_sram_suspend = spmp_sram_push(spmp_cpu_suspend,
				   spmp_cpu_suspend_sz);
	spmp_cpu_pm_fns = &spmp8050_cpu_pm_fns;
}
#else
static inline void spmp_init_pm(void) {}
#endif


/*****************************************************************
 * register mapping
*****************************************************************/
void __init spmp_regdev_reregister_map(void)
{

	if (check_mem_region(LOGI_ADDR_REG, LOGI_ADDR_REG_RANGE)) {
	    printk("register_address: memory already in use\n");
    	return;
	}
	/* request all register address for hal asscess */
	if (request_mem_region(LOGI_ADDR_REG, LOGI_ADDR_REG_RANGE, "register_address")) {
	    printk("!!!!!!!!!!!!!!!!!!request_mem_region: fail\n");
    	return;
	}

    //release_mem_region(mem_addr, mem_size);

}

/*****************************************************************
 *
*****************************************************************/
void __init spmp8050_devinit(void)
{
	printk(KERN_INFO "[%s][%d] run\n",__FUNCTION__, __LINE__);
#ifdef CONFIG_PM
   spmp_init_pm();
#endif
#ifdef CONFIG_FB_SPMP
   spmp_regdev_display();
#endif
	spmp_regdev_reregister_map();

//   spmp_regdev_gpio();
   spmp_regdev_ehci();
   spmp_regdev_ohci();
	spmp_regdev_udc();
	spmp_regdev_android();
//   spmp_regdev_sd0();
   spmp_regdev_rtc();
   spmp_regdev_saacc();

//   spmp_regdev_i2cB();
//   spmp_regdev_cmos();
//   i2c_register_board_info(0, vo9655_i2c_devices, ARRAY_SIZE(vo9655_i2c_devices));
//   spmp_regdev_sd1();
}

void spmp8050_power_off(void)
{
	gp_board_t *config;

	printk(KERN_INFO "powering system down...\n");

	config = gp_board_get_config("board", gp_board_t);
	if (config != NULL && config->power_off != NULL) {
		config->power_off();
	}

	SCUB_PWRC_CFG = 0;
}

void spmp8050_power_on(void)
{
	printk(KERN_INFO "powering system down...\n");

	SCUB_PWRC_CFG = 0;
}

#define SCUB_TIMER1_CLKENABLE ((0x01<<10))

#define MSEC_10                 (1000 * 10)  // 10 ms

#define SYSTEM_CLOCK            (27*1000000)   // Watchdog Clk = 27 Mhz
#define PRESCALER_USEC_1      	((SYSTEM_CLOCK / 1000000) - 1)
#define TICKS_PER_USEC          (SYSTEM_CLOCK / (PRESCALER_USEC_1+1) / 1000000)
#define TIMER_INTERVAL          ((TICKS_PER_USEC * MSEC_10) -1) //10ms

#define TICKS2USECS(x)          ( (x) / TICKS_PER_USEC)

#define TIMER_USEC_SHIFT 16
#define RESETWDT_BASE			(WDT_BASE + 0)

/* Watchdog Timer 0 register */
#define WDTCTR_R   (*(volatile unsigned int*)(RESETWDT_BASE+0x00))  //control Register
#define WDTPSR_R   (*(volatile unsigned int*)(RESETWDT_BASE+0x04))  //pre-scare Register
#define WDTLDR_R   (*(volatile unsigned int*)(RESETWDT_BASE+0x08))  //load value Register
#define WDTVLR_R   (*(volatile unsigned int*)(RESETWDT_BASE+0x0c))  //current counter value Register
#define WDTCMP_R   (*(volatile unsigned int*)(RESETWDT_BASE+0x10))  //compare Register
void spmp8050_power_reset(void)
{
	gp_board_t *config;
	printk(KERN_INFO "powering system reset...\n");

	config = gp_board_get_config("board", gp_board_t);
	if (config != NULL && config->power_reset != NULL) {
		config->power_reset();
	}
	else {
//       SCUB_B_PERI_CLKEN |= SCUB_TIMER1_CLKENABLE;
		WDTCTR_R  = WDT_DISABLE;
		WDTPSR_R  = PRESCALER_USEC_1;
		WDTLDR_R  = TIMER_INTERVAL;
		WDTCMP_R  = 0;
		WDTCTR_R  = WDT_RE_ENABLE | WDT_IE_ENABLE | WDT_PWMON_WDT | WDT_ENABLE ;
	}
}
