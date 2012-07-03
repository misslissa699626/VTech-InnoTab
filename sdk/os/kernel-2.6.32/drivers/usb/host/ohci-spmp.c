#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/signal.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <mach/regs-usbhost.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/hal/hal_clock.h>
struct device;

struct spmpohci_platform_data {
	int (*init)(struct device *);
	void (*exit)(struct device *);
};

struct spmp_ohci {
	/* must be 1st member here for hcd_to_ohci() to work */
	struct ohci_hcd ohci;

	struct device	*dev;
	struct clk	*clk;
};

extern int usb_disabled(void);

static int spmp_start_ohc(struct spmp_ohci *ohci, struct device *dev)
{
	int retval = 0;
	struct spmpohci_platform_data *inf;

	inf = dev->platform_data;
#ifdef CONFIG_PM
	gpHalScuClkEnable(SCU_C_PERI_SYS_A, SCU_C, 1);
	gpHalScuClkEnable(SCU_A_PERI_USB0 | SCU_A_PERI_USB1, SCU_A, 1);
	gpHalScuUsbPhyClkEnable(1);
#else
	clk_enable(ohci->clk);
#endif
	if (inf->init)
		retval = inf->init(dev);

	if (retval < 0)
		return retval;

	return 0;
}

static void spmp_stop_ohc(struct spmp_ohci *ohci, struct device *dev)
{
	struct spmpohci_platform_data *inf;

	inf = dev->platform_data;

	if (inf->exit)
		inf->exit(dev);
	
#ifdef CONFIG_PM
	gpHalScuUsbPhyClkEnable(0);
	gpHalScuClkEnable(SCU_A_PERI_USB0 | SCU_A_PERI_USB1, SCU_A, 0);
	gpHalScuClkEnable(SCU_C_PERI_SYS_A, SCU_C, 0);
#else
	clk_disable(ohci->clk);
#endif
}

static int __devinit ohci_spmp_start(struct usb_hcd *hcd)
{
  
//  #define	FSMP_NEW(fi)		(0x7fff & ((6 * ((fi) - 210)) / 7))	
// modify by eddie 20110217 FSMP shoule have 15 valid bit, but hardware only support 13 bit.
//  #define	FSMP_NEW(fi)		(0x7fff & (((6 * ((fi) - 210)) / 7))>>2)
  #define	FSMP_NEW(fi)		2441
  
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;
	u32 temp;

	ohci_dbg(ohci, "ohci_spmp_start, ohci:%p", ohci);
	/* The value of NDP in roothub_a is incorrect on this hardware */
	ohci->num_ports = 2;

	
	if ((ret = ohci_init(ohci)) < 0)
		return ret;
	
	temp = ohci_readl (ohci, &ohci->regs->fminterval);
	ohci->fminterval = temp & 0x7fff;
	if (ohci->fminterval != FI)
	{
		ohci_dbg(ohci, "fminterval delta %d\n", ohci->fminterval - FI);
	}
	ohci->fminterval |= FSMP_NEW (ohci->fminterval) << 16;
	/* also: power/overcurrent flags in roothub.a */
	
	if ((ret = ohci_run(ohci)) < 0) {
		err ("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_spmp_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"spmp OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_spmp_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};


int usb_hcd_spmp_probe (const struct hc_driver *driver, struct platform_device *pdev)
{
	int retval, irq;
	struct usb_hcd *hcd;
	struct spmpohci_platform_data *inf;
	struct spmp_ohci *ohci;
	struct resource *r;
	struct clk *usb_clk = NULL;

	inf = pdev->dev.platform_data;

	if (!inf)
		return -ENODEV;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		pr_err("no resource of IORESOURCE_IRQ");
		return -ENXIO;
	}	

#ifndef CONFIG_PM
	usb_clk = clk_get(&pdev->dev, "USB_HOST");
	if (IS_ERR(usb_clk))
		return PTR_ERR(usb_clk);
#else
	usb_clk = NULL;
#endif
	hcd = usb_create_hcd (driver, &pdev->dev, "spmp");
	if (!hcd)
		return -ENOMEM;
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		pr_err("no resource of IORESOURCE_MEM");
		retval = -ENXIO;
		goto err1;
	}

	hcd->rsrc_start = r->start;
	hcd->rsrc_len = resource_size(r);
#if 0
	/*The memory region was requested in device.c*/
	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed");
		retval = -EBUSY;
		goto err1;
	}
#endif	
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	
	if (!hcd->regs) {
		pr_debug("ioremap failed");
		retval = -ENOMEM;
		goto err2;
	}

	/* initialize "struct spmp ohci" */
	ohci = (struct spmp_ohci *)hcd_to_ohci(hcd);
	ohci->dev = &pdev->dev;
#ifndef CONFIG_PM
	ohci->clk = usb_clk;
#endif
	if ((retval = spmp_start_ohc(ohci, &pdev->dev)) < 0) {
		pr_debug("spmp_start_ohc failed");
		goto err3;
	}
	
	ohci_hcd_init(hcd_to_ohci(hcd));
	retval = usb_add_hcd(hcd, irq, IRQF_DISABLED);	

	if (retval == 0)
		return retval;

	spmp_stop_ohc(ohci, &pdev->dev);
 err3:
	iounmap(hcd->regs);
 err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
 err1:
	usb_put_hcd(hcd);
#ifndef CONFIG_PM
	clk_put(usb_clk);
#endif
	return retval;
}




void usb_hcd_spmp_remove (struct usb_hcd *hcd, struct platform_device *pdev)
{
	struct spmp_ohci *ohci = (struct spmp_ohci *)hcd_to_ohci(hcd);

	usb_remove_hcd(hcd);
	spmp_stop_ohc(ohci, &pdev->dev);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
#ifndef CONFIG_PM
	clk_put(ohci->clk);
#endif
	
}

#ifdef CONFIG_PM
static int ohci_hcd_spmp_drv_suspend(struct platform_device *pdev,
					pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	struct spmp_ohci *spmpohci = (struct spmp_ohci *)hcd_to_ohci(hcd);
	unsigned long flags;
	int rc;

	rc = 0;

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
	(void)ohci_readl(ohci, &ohci->regs->intrdisable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW)
		ohci_usb_reset(ohci);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	spmp_stop_ohc(spmpohci, &pdev->dev);
bail:
	spin_unlock_irqrestore(&ohci->lock, flags);

	return rc;
}

static int ohci_hcd_spmp_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct spmp_ohci *spmpohci = (struct spmp_ohci *)hcd_to_ohci(hcd);
	spmp_start_ohc(spmpohci, &pdev->dev);

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	ohci_finish_controller_resume(hcd);

	return 0;
}
#else
#define ohci_hcd_spmp_drv_suspend NULL
#define ohci_hcd_spmp_drv_resume NULL
#endif

static int ohci_hcd_spmp_drv_probe(struct platform_device *pdev)
{

	if (usb_disabled())
		return -ENODEV;

	return usb_hcd_spmp_probe(&ohci_spmp_hc_driver, pdev);
}

static int ohci_hcd_spmp_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_hcd_spmp_remove(hcd, pdev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}




static struct platform_driver ohci_hcd_spmp_driver = {
	.probe		= ohci_hcd_spmp_drv_probe,
	.remove		= ohci_hcd_spmp_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ohci_hcd_spmp_drv_suspend,
	.resume		= ohci_hcd_spmp_drv_resume,
	.driver		= {
		.name	= "spmp-ohci",
		.owner	= THIS_MODULE,
	},
};

MODULE_ALIAS("platform:spmp-ohci");
