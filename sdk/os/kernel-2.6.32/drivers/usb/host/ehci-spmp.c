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

struct spmpehci_platform_data {
	int (*init)(struct device *);
	void (*exit)(struct device *);
};

struct spmp_ehci {
	/* must be 1st member here for hcd_to_ehci() to work */
	struct ehci_hcd ehci;

	struct device	*dev;
	struct clk	*clk;
	void __iomem	*mmio_base;
};

extern int usb_disabled(void);

static int spmp_start_ehc(struct spmp_ehci *ehci, struct device *dev)
{
	int retval = 0;
	struct spmpehci_platform_data *inf;
	
	inf = dev->platform_data;
#ifdef CONFIG_PM
	//gp_enable_clock((int*)"SYS_A", 1);
	gpHalScuClkEnable(SCU_C_PERI_SYS_A, SCU_C, 1);
	gpHalScuClkEnable(SCU_A_PERI_USB0 | SCU_A_PERI_USB1, SCU_A, 1);
	gpHalScuUsbPhyClkEnable(1);
#else
	clk_enable(ehci->clk);
#endif
	if (inf->init)
		retval = inf->init(dev);

	if (retval < 0)
		return retval;

	return 0;
}

static void spmp_stop_ehc(struct spmp_ehci *ehci, struct device *dev)
{
	struct spmpehci_platform_data *inf;
	
	inf = dev->platform_data;

	if (inf->exit)
		inf->exit(dev);
#ifdef CONFIG_PM
	gpHalScuUsbPhyClkEnable(0);
	gpHalScuClkEnable(SCU_A_PERI_USB0 | SCU_A_PERI_USB1, SCU_A, 0);
	gpHalScuClkEnable(SCU_C_PERI_SYS_A, SCU_C, 0);
#else
	clk_disable(ehci->clk);
#endif
}

static const struct hc_driver ehci_spmp_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "spmp EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,
};

static int usb_hcd_spmp_probe (const struct hc_driver *driver, struct platform_device *pdev)
{
	int retval, irq;
	struct usb_hcd *hcd;
	struct spmpehci_platform_data *inf;
	struct spmp_ehci *spmpehci;
	struct ehci_hcd *ehci;	
	struct resource *r;
	struct clk *usb_clk;

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
	
	/* initialize "struct spmp ehci" */

	spmpehci = (struct spmp_ehci *)hcd_to_ehci(hcd);	
	spmpehci->dev = &pdev->dev;
	spmpehci->clk = usb_clk;
	if ((retval = spmp_start_ehc(spmpehci, &pdev->dev)) < 0) {
		pr_debug("spmp_start_ehc failed");
		goto err3;
	}	
	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	retval = usb_add_hcd(hcd, irq, IRQF_DISABLED);	
	if (retval == 0)
		return retval;

	spmp_stop_ehc(spmpehci, &pdev->dev);
 err3:
	iounmap(hcd->regs);
 err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
 err1:
	usb_put_hcd(hcd);
	clk_put(usb_clk);
	return retval;
}

static void usb_hcd_spmp_remove (struct usb_hcd *hcd, struct platform_device *pdev)
{
	struct spmp_ehci *ehci = (struct spmp_ehci *)hcd_to_ehci(hcd);

	usb_remove_hcd(hcd);
	spmp_stop_ehc(ehci, &pdev->dev);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
#ifndef CONFIG_PM
	clk_put(ehci->clk);
#endif
}

#ifdef CONFIG_PM
static int ehci_hcd_spmp_drv_suspend(struct platform_device *pdev,
					pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct spmp_ehci *spmpehci = (struct spmp_ehci *)hcd_to_ehci(hcd);
	unsigned long flags;
	int rc;

	return 0;
	rc = 0;

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW) {
		ehci_halt(ehci);
		ehci_reset(ehci);
	}

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	spmp_stop_ehc(spmpehci,&pdev->dev);

bail:
	spin_unlock_irqrestore(&ehci->lock, flags);

	// could save FLADJ in case of Vaux power loss
	// ... we'd only use it to handle clock skew

	return rc;
}


static int ehci_hcd_spmp_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct spmp_ehci *spmpehci = (struct spmp_ehci *)hcd_to_ehci(hcd);
	spmp_start_ehc(spmpehci,&pdev->dev);

	// maybe restore FLADJ

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	/* If CF is still set, we maintained PCI Vaux power.
	 * Just undo the effect of ehci_pci_suspend().
	 */
	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having let BIOS kick in during reboot.
	 */
	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;
}

#else
#define ehci_hcd_spmp_drv_suspend NULL
#define ehci_hcd_spmp_drv_resume NULL
#endif

static int ehci_hcd_spmp_drv_probe(struct platform_device *pdev)
{

	if (usb_disabled())
		return -ENODEV;

	return usb_hcd_spmp_probe(&ehci_spmp_hc_driver, pdev);
}

static int ehci_hcd_spmp_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_hcd_spmp_remove(hcd, pdev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver ehci_hcd_spmp_driver = {
	.probe		= ehci_hcd_spmp_drv_probe,
	.remove		= ehci_hcd_spmp_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_spmp_drv_suspend,
	.resume		= ehci_hcd_spmp_drv_resume,
	.driver = {
		.name	= "spmp-ehci",
		.owner	= THIS_MODULE,
	}
};


MODULE_ALIAS("platform:spmp-ehci");
