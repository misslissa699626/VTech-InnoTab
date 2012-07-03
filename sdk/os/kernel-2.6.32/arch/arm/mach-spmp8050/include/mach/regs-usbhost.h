/*
 * arch/arm/mach-spmp8000/include/mach/regs-usbhost.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * USBHOST - System peripherals regsters.
 *
 */

#define USBHOST_BASE			IO3_ADDRESS(0x4000)
#define USBDEVICE_BASE			IO3_ADDRESS(0x6000)

#define UH_VERSION   (*(volatile unsigned int*)(USBHOST_BASE+0x00))  
#define UH_CTRL      (*(volatile unsigned int*)(USBHOST_BASE+0x04))
#define UH_POWERCTRL (*(volatile unsigned int*)(USBHOST_BASE+0x10))
#define UHO_FMINTERVAL (*(volatile unsigned int*)(USBHOST_BASE+0xB4))
#define UHO_HCIVERSION (*(volatile unsigned int*)(USBHOST_BASE+0x80))
#define UHE_HCIVERSION (*(volatile unsigned int*)(USBHOST_BASE+0x100))
#define UDC_SOFTDISC (*(volatile unsigned int*)(USBDEVICE_BASE+0x3B0))


#define UH_VERSION_OFST   0x00
#define UH_CTRL_OFST      0x04
#define UH_POWERCTRL_OFST 0x10

// USBPHY_CFG in SCUA
#define USBPHY_HOST_SEL                (1 << 0) 
#define USBPHY1_CTRL_SEL               (1 << 1)
#define USBPHY_XTAL_ENABLE             (1 << 2)
#define USBPHY0_POWER_CTRL             (1 << 8)
#define USBPHY0_POWER_MODE             (1 << 9)
#define USBPHY0_CTRL_DPDMPD_INTESTMODE (1 <<11)
#define USBPHY1_POWER_CTRL             (1 <<12)
#define USBPHY1_POWER_MODE             (1 <<13)
#define USBPHY1_NODRIVE                (1 <<14)
#define USBPHY1_CTRL_DPDMPD_INTESTMODE (1 <<15)
#define USBPHY_TEST_MODE               (0x1F << 16)
#define USBPHY_I2C_ID                  (0x3F << 24)
#define USBPHY_I2C_DISABLE             (1 <<31)



// USB HOST Ctrl
#define MASTER_EN  0x0001

// USB HOST Power ctrl
#define AWAKE_EN                 (1 << 0)    /* 1 = always wakeup , 0 = can partial */
#define UPHY_FORCE_SUSPEND_CTRL  (1 << 8)  /* 1 = force suspend control enable , 0 = suspend control by host controller*/
#define UPHY_FORCE_PARTIAL_CTRL  (1 << 9)  /* 1 = force partial control enable , 0 = partial control by host controller*/
#define UPHY_SUSPEND_EN          (1 <<10)   /* When UPHY_FORCE_SUSPEND_CTRL = 1 --> 1= force suspend , 2= force not suspend */
#define UPHY_PARTIAL_EN          (1 <<11)   /* When UPHY_FORCE_PARTIAL_CTRL = 1 --> 1= force partial , 2= force not partial */
#define USB_CLK_EN               (1 <<16)   /* When UPHY_FORCE_SUSPEND_CTRL = 1 --> 1= force suspend , 2= force not suspend */
#define USBPHY_PARTIALM          (1 <<17)   /* When UPHY_FORCE_SUSPEND_CTRL = 1 --> 1= force suspend , 2= force not suspend */
#define USBPHY_SUSPENDM          (1 <<18)   /* When UPHY_FORCE_SUSPEND_CTRL = 1 --> 1= force suspend , 2= force not suspend */

