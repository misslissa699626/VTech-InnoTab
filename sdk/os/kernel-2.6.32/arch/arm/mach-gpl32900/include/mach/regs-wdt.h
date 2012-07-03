/*
 * arch/arm/mach-spmp8000/include/mach/regs-wdt.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Watchdog Timer (WDT) - System peripherals regsters.
 *
 */



#define WDT_BASE			IO0_ADDRESS(0x1000)
#define WDT0_BASE			(WDT_BASE + 0x0)
#define WDT1_BASE			(WDT_BASE + 0x20)
#define WDT2_BASE			(WDT_BASE + 0x40)

/* Watchdog Timer 0 register */
#define WDTCTR_0   (*(volatile unsigned int*)(WDT0_BASE+0x00))  //control Register
#define WDTPSR_0   (*(volatile unsigned int*)(WDT0_BASE+0x04))  //pre-scare Register
#define WDTLDR_0   (*(volatile unsigned int*)(WDT0_BASE+0x08))  //load value Register
#define WDTVLR_0   (*(volatile unsigned int*)(WDT0_BASE+0x0c))  //current counter value Register
#define WDTCMP_0   (*(volatile unsigned int*)(WDT0_BASE+0x10))  //compare Register


/* Watchdog Timer 1 register */
#define WDTCTR_1   (*(volatile unsigned int*)(WDT1_BASE+0x00))
#define WDTPSR_1   (*(volatile unsigned int*)(WDT1_BASE+0x04))
#define WDTLDR_1   (*(volatile unsigned int*)(WDT1_BASE+0x08))
#define WDTVLR_1   (*(volatile unsigned int*)(WDT1_BASE+0x0c))
#define WDTCMP_1   (*(volatile unsigned int*)(WDT1_BASE+0x10))


/* Watchdog Timer 2 register */
#define WDTCTR_2   (*(volatile unsigned int*)(WDT2_BASE+0x00))
#define WDTPSR_2   (*(volatile unsigned int*)(WDT2_BASE+0x04))
#define WDTLDR_2   (*(volatile unsigned int*)(WDT2_BASE+0x08))
#define WDTVLR_2   (*(volatile unsigned int*)(WDT2_BASE+0x0c))
#define WDTCMP_2   (*(volatile unsigned int*)(WDT2_BASE+0x10))

// For Watch Dog Timer Control Timer Register (CTR)
/* bit 0 Timer enable */
#define WDT_ENABLE     0x0001
#define WDT_DISABLE    0x0000

/* bit 1 Interrupt enable */
#define WDT_IE_ENABLE   0x0002
#define WDT_IE_DISABLE  0x0000

/* bit 2 Output enable */
#define WDT_OE_ENABLE  0x0004
#define WDT_OE_DISABLE 0x0000

/* bit 2 Reset enable */
#define WDT_RE_ENABLE   0x0008
#define WDT_RE_DISABLE  0x0000

/* bit 3 Output mode of timer */
#define WDT_PWMON_WDT   0x0000
#define WDT_PWMON_PWM   0x0010

