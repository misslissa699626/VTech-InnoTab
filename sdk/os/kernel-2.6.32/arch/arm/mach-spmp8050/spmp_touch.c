#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/regs-saradc.h>

/* For ts.dev.id.version */
#define spmpTSVERSION	0x0101

#define DEBUG_LVL    KERN_DEBUG

MODULE_AUTHOR("Arnaud Patard <arnaud.patard@rtp-net.org>");
MODULE_DESCRIPTION("spmp touchscreen driver");
MODULE_LICENSE("GPL");

/*
 * Definitions & global arrays.
 */


static char *spmpts_name = "spmp TouchScreen";

/*
 * Per-touchscreen data.
 */

struct spmpts {
	struct input_dev *dev;
	long xp;
	long yp;
	int count;
	int shift;
};

static struct spmpts ts;
static struct resource	*ress;
static void __iomem *base_addr;
static struct clk	*saacc_clock;
static int			irq_saacc;
static int g_uiXhigh = 970, g_uiXlow = 40,g_uiYhigh = 970 ,g_uiYlow = 40;
static int pendown;
static struct timer_list aux_timer;
static struct input_dev *adc_key_dev;
static unsigned short keymap[8] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_BACK, KEY_HOME, KEY_MENU, 0};
static int keyadc[8] = {321, 470, 618, 927, 768, 6, 161, 0};
static unsigned short last_key;

static int adc_key_open(struct input_dev *dev)
{
	return 0;
}

static void adc_key_close(struct input_dev *dev)
{
}

static void adc_key_update(int adc_value)
{
	int i;
	unsigned short key = 0;
	//printk("adc_value = %d\n", adc_value);

	for (i = 0; keymap[i] != 0; i++) {
		if (adc_value > keyadc[i] - 50 && adc_value < keyadc[i] + 50) {
			key = keymap[i];
			break;
		}
	}

	if (last_key == key)
		return;

	if (last_key != 0) {
		input_report_key(adc_key_dev, last_key, 0);
		//printk("adc key up = %d\n", last_key);
	}
	last_key = key;

	if (key != 0) {
		input_report_key(adc_key_dev, key, 1);
		//printk("adc key down = %d\n", key);
	}

	input_sync(adc_key_dev);
}

static int adc_key_init(void)
{
	int error;
	unsigned short *pkey;
	int i;

	adc_key_dev = input_allocate_device();
	if (!adc_key_dev) {
		printk(KERN_ERR "spmp_touch.c: Not enough memory\n");
		error = -ENOMEM;
		goto err_free_dev;
	}

    set_bit(EV_KEY, adc_key_dev->evbit);
	for (pkey = keymap; *pkey != 0; pkey++)
		set_bit(*pkey, adc_key_dev->keybit);

	error = input_register_device(adc_key_dev);
	if (error) {
		printk(KERN_ERR "spmp_touch.c: Failed to register device\n");
		goto err_free_dev;
	}
	adc_key_dev->open = adc_key_open;
	adc_key_dev->close = adc_key_close;

	return 0;

 err_free_dev:
	input_free_device(adc_key_dev);

	return error;
}

static int adc_key_exit(void)
{
	input_unregister_device(adc_key_dev);
	input_free_device(adc_key_dev);
}

static void start_aux_timer()
{
   del_timer(&aux_timer);
   //printk("start_aux_timer\n");
   aux_timer.expires = jiffies + HZ / 10;
   add_timer(&aux_timer);
}

static void stop_aux_timer()
{
   //printk("stop_aux_timer\n");
   del_timer(&aux_timer);
}

static void aux_timeout(unsigned long arg)
{
	int sarctrl;
	writel(0 , base_addr + SAACC_INTEN_OFST);

	sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);

	sarctrl &= ~(SAACC_SAR_SARS_MASK << SAACC_SAR_SARS_OFST);
	sarctrl |= 0x04 << SAACC_SAR_SARS_OFST;

	sarctrl &= ~(SAACC_SAR_MODE_MASK << SAACC_SAR_MODE_OFST);
	sarctrl |= 0x04 << SAACC_SAR_MODE_OFST;

	sarctrl &= ~(SAACC_SAR_TPS_MASK << SAACC_SAR_TPS_OFST);
	sarctrl |= 0x01 << SAACC_SAR_TPS_OFST;

	sarctrl &= ~(SARCTL_SAR_AUTO_CON_ON);
	sarctrl |= SARCTL_SAR_MAN_CON_ON;

	writel(SAACC_SAR_IENAUX, base_addr + SAACC_INTEN_OFST);

	writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);

	//start_aux_timer();
}

static void init_aux_timer()
{
   init_timer(&aux_timer);
   aux_timer.function = aux_timeout;
}

static void del_aux_timer()
{
   del_timer(&aux_timer);
}

static irqreturn_t stylus_action(int irq, void *dev_id)
{
	unsigned long uiIntrFlags;
	unsigned long data1;
    int sarctrl;
    uiIntrFlags = readl(base_addr + SAACC_INTF_OFST);
	//printk("uiIntrFlags =%08x\n",uiIntrFlags);
    if(uiIntrFlags & SAACC_SAR_IENAUX)
    {
	  unsigned int v;
	  v = 0x3ff & (unsigned short)( (readl(base_addr + SAACC_AUX_OFST) & 0xFFFF) + 1024);
      //printk("AUX %d\n",v);
	  adc_key_update(v);

      sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);
      //printk("sarctrl %08x\n",sarctrl);

	  sarctrl &= ~(SAACC_SAR_MODE_MASK << SAACC_SAR_MODE_OFST);
	  sarctrl |= 0x01 << SAACC_SAR_MODE_OFST;

	  sarctrl &= ~(SAACC_SAR_TPS_MASK << SAACC_SAR_TPS_OFST);
	  sarctrl |= 0x03 << SAACC_SAR_TPS_OFST;

	  sarctrl &= ~(SARCTL_SAR_AUTO_CON_ON);
	  sarctrl |= SARCTL_SAR_MAN_CON_ON;

	  writel(SAACC_SAR_IENPNL | SAACC_SAR_IENPENUP | SAACC_SAR_IENPENDN , base_addr + SAACC_INTEN_OFST);
      writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);

      start_aux_timer();
    }

    if(uiIntrFlags & SAACC_SAR_IENPNL)
    {

      unsigned short sPosx,sPosy,tmpPosx,tmpPosy;
	  unsigned long tPosx,tPosy,g_uiPanelXY;

      g_uiPanelXY = readl(base_addr + SAACC_PNL_OFST);
      ts.yp = 0x3ff & (unsigned short)( (g_uiPanelXY & 0xFFFF) + 1024);
      ts.xp = 0x3ff & (unsigned short)( (g_uiPanelXY >> 16)    + 1024);
      ts.count++;
      pendown = 1;
	  if(ts.count > 1)
	  {
     	  if(pendown == 1 )
	      {
 			input_report_abs(ts.dev, ABS_X, ts.xp);
 			input_report_abs(ts.dev, ABS_Y, ts.yp);
 			input_report_key(ts.dev, BTN_TOUCH, 1);
 			input_report_abs(ts.dev, ABS_PRESSURE, 1);
                        printk("[out] %d %d \n",ts.xp,ts.yp);
 			input_sync(ts.dev);
    	  }
	  }
      sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);
      sarctrl &= 0xFFFF80;
      sarctrl |= (SARCTL_SAR_AUTO_CON_ON | 0x40);
      writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);
    }

    if(uiIntrFlags & SAACC_SAR_IENPENUP)
    {
      sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);

      sarctrl &= 0xFFFF80;
	  sarctrl &= ~(SARCTL_SAR_AUTO_CON_ON);
	  sarctrl |= SARCTL_SAR_MAN_CON_ON | 0x60;
      writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);
	  if(pendown == 1)
	  {
 		input_report_key(ts.dev, BTN_TOUCH, 0);
 		input_report_abs(ts.dev, ABS_PRESSURE, 0);
 		input_sync(ts.dev);
        printk("penup\n");
  	    start_aux_timer();
	  }
      pendown = 0;
    }

    if(uiIntrFlags & SAACC_SAR_IENPENDN)
    {
	  stop_aux_timer();
      ts.xp = 0;
      ts.yp = 0;
      ts.count = 0;

      sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);
      sarctrl &= 0xFFFF80;
	  sarctrl |= SARCTL_SAR_MAN_CON_ON | 0x40;
      printk("pendwon\n");
      writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);
    }

	return IRQ_HANDLED;
}


/*
 * The functions for inserting/removing us as a module.
 */
static void start_sar()
{
#define SARCTL_SARS_TPXP (0x00<<2)
#define SARCTL_SARS_TPXN (0x01<<2)
#define SARCTL_SARS_TPYP (0x02<<2)
#define SARCTL_SARS_TPYN (0x03<<2)
#define SARCTL_SARS_AUX1 (0x04<<2)
   int sarctrl;
   int debtime;

   // setup clock divider number
   sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);
   sarctrl &= ~(SAACC_SAR_DIVNUM_MASK << SAACC_SAR_DIVNUM_OFST);
   sarctrl |= ( 16 << SAACC_SAR_DIVNUM_OFST);
   writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);


   writel(SAACC_SAR_IENPNL | SAACC_SAR_IENPENUP | SAACC_SAR_IENPENDN , base_addr + SAACC_INTEN_OFST);

   debtime = readl(base_addr + SAACC_DEBTIME_OFST);
   debtime  = 0x2000 |(0x20<<16);
   writel(debtime,base_addr + SAACC_DEBTIME_OFST);
   printk("debtime =%d\n",debtime);

//   debtime = readl(base_addr + SAACC_CONDLY_OFST);
//   debtime  = 0x60;
//   writel(debtime,base_addr + SAACC_CONDLY_OFST);
//   printk("sarctrl =%d\n",debtime);

   sarctrl = readl(base_addr + SAACC_SARCTRL_OFST);

   sarctrl &= ~(SAACC_SAR_MODE_MASK << SAACC_SAR_MODE_OFST);
   sarctrl |= 0x01 << SAACC_SAR_MODE_OFST;

   sarctrl &= ~(SAACC_SAR_TPS_MASK << SAACC_SAR_TPS_OFST);
   sarctrl |= 0x03 << SAACC_SAR_TPS_OFST;

   sarctrl &= ~(SARCTL_SAR_AUTO_CON_ON);
   sarctrl |= SARCTL_SAR_MAN_CON_ON;

   printk("sarctrl =%x\n",sarctrl);
   writel(sarctrl,base_addr + SAACC_SARCTRL_OFST);

   init_aux_timer();
   start_aux_timer();
}

static void stop_sar()
{
   del_aux_timer();
   writel(0 , base_addr + SAACC_INTEN_OFST);
}

static int __init spmpts_probe(struct platform_device *pdev)
{
	int rc,ret;
	struct input_dev *input_dev;

    printk("Entering spmpts_init\n");
	ret = -ENXIO;
	ress = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!ress) {
		printk("SAACC : No I/O memory resource defined\n");
		goto err_ress;
	}

	irq_saacc = platform_get_irq(pdev, 0);
	if (irq_saacc < 0) {
		printk("SAACC : No IRQ resource defined\n");
		goto err_ress;
	}

	ret = -ENOMEM;
	base_addr = ioremap(ress->start,
				resource_size(ress));
	if (!base_addr) {
		dev_err(&pdev->dev, "SAACC : Unable to map I/O memory\n");
		goto err_map;
	}

	saacc_clock= clk_get(&pdev->dev, "SAACC");
	if (!saacc_clock) {
		dev_err(&pdev->dev, "Unable to get saacc clock\n");
      	ret = -ENOENT;
		goto err_saacc_reg;
	}
	clk_enable(saacc_clock);

	printk("got and enabled clock\n");
	memset(&ts, 0, sizeof(struct spmpts));
	input_dev = input_allocate_device();

	if (!input_dev) {
		printk(KERN_ERR "Unable to allocate the input device !!\n");
		goto err_saacc_clock;
	}

	ts.dev = input_dev;
	ts.dev->evbit[0] = BIT_MASK(EV_SYN)| BIT_MASK(EV_KEY) |BIT_MASK(EV_ABS);
	ts.dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    set_bit(EV_SW, input_dev->evbit);
    bitmap_fill(input_dev->swbit, SW_MAX);
//    bitmap_fill(input_dev->absbit, ABS_MAX);
	input_set_abs_params(ts.dev, ABS_X, g_uiXlow, g_uiXhigh, 0, 0);
	input_set_abs_params(ts.dev, ABS_Y, g_uiYlow, g_uiYhigh, 0, 0);
	input_set_abs_params(ts.dev, ABS_PRESSURE, 0, 1, 0, 0);
//	input_set_abs_params(ts.dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);
//	ts.dev->private = &ts;
	ts.dev->name = spmpts_name;
	ts.dev->id.bustype = BUS_RS232;
	ts.dev->id.vendor  = 0xDEAD;
	ts.dev->id.product = 0xBEEF;
	ts.dev->id.version = spmpTSVERSION;

	ts.shift = 2; //info->oversampling_shift;

	/* Get irqs */
	if (request_irq(irq_saacc, stylus_action, IRQF_SAMPLE_RANDOM,
		"spmp_saacc", ts.dev)) {
		printk(KERN_ERR "spmp_ts.c: Could not allocate ts IRQ_ADC !\n");
		goto err_saacc_input;
	}

	printk(KERN_INFO "%s successfully loaded\n", spmpts_name);

	/* All went ok, so register to the input system */
	rc = input_register_device(ts.dev);
	if (rc) {
		ret = -EIO;
		goto err_irq;
	}
	adc_key_init();
    start_sar();
	return 0;
err_irq:
	free_irq(irq_saacc, ts.dev);
err_saacc_input:
    input_free_device(input_dev);
err_saacc_clock:
	clk_disable(saacc_clock);
err_saacc_reg:
    iounmap(base_addr);
err_ress:
err_map:
	return ret;
}

static int spmpts_remove(struct platform_device *pdev)
{
    stop_sar();
	free_irq(irq_saacc, ts.dev);

	if (saacc_clock) {
		clk_disable(saacc_clock);
		clk_put(saacc_clock);
		saacc_clock = NULL;
	}

	input_unregister_device(ts.dev);
	iounmap(base_addr);

	adc_key_exit();
	return 0;
}

#ifdef CONFIG_PM
static int spmpts_suspend(struct platform_device *pdev, pm_message_t state)
{
#if 0
	writel(TSC_SLEEP, base_addr+spmp_ADCTSC);
	writel(readl(base_addr+spmp_ADCCON) | spmp_ADCCON_STDBM,
	       base_addr+spmp_ADCCON);

	disable_irq(IRQ_ADC);
	disable_irq(IRQ_TC);

	clk_disable(adc_clock);
#endif
	return 0;
}

static int spmpts_resume(struct platform_device *pdev)
{
#if 0
	struct spmp_ts_mach_info *info =
		( struct spmp_ts_mach_info *)pdev->dev.platform_data;

	clk_enable(adc_clock);
	msleep(1);

	enable_irq(IRQ_ADC);
	enable_irq(IRQ_TC);

	if ((info->presc&0xff) > 0)
		writel(spmp_ADCCON_PRSCEN | spmp_ADCCON_PRSCVL(info->presc&0xFF),\
			     base_addr+spmp_ADCCON);
	else
		writel(0,base_addr+spmp_ADCCON);

	/* Initialise registers */
	if ((info->delay&0xffff) > 0)
		writel(info->delay & 0xffff,  base_addr+spmp_ADCDLY);

	writel(WAIT4INT(0), base_addr+spmp_ADCTSC);
#endif
	return 0;
}

#else
#define spmpts_suspend NULL
#define spmpts_resume  NULL
#endif

static struct platform_driver spmpts_driver = {
       .driver         = {
	       .name   = "spmp-saacc",
	       .owner  = THIS_MODULE,
       },
       .probe          = spmpts_probe,
       .remove         = spmpts_remove,
       .suspend        = spmpts_suspend,
       .resume         = spmpts_resume,

};

static int __init spmpts_init(void)
{
	int rc;

	rc = platform_driver_register(&spmpts_driver);
	return rc;
}

static void __exit spmpts_exit(void)
{
	platform_driver_unregister(&spmpts_driver);
}

module_init(spmpts_init);
module_exit(spmpts_exit);

