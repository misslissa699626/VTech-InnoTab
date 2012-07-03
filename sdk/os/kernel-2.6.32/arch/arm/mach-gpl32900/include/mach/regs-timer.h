/*
 * arch/arm/mach-spmp8000/include/mach/regs-timer.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Timer - System peripherals regsters.
 *
 */



#define TIMER_BASE			IO0_ADDRESS(0x0000)
#define TIMER0_BASE			(TIMER_BASE + 0x0)
#define TIMER1_BASE			(TIMER_BASE + 0x20)
#define TIMER2_BASE			(TIMER_BASE + 0x40)
#define TIMER3_BASE			(TIMER_BASE + 0x60)
#define TIMER4_BASE			(TIMER_BASE + 0x80)

/* Timer 0 register */
#define TMCTR_0   (*(volatile unsigned int*)(TIMER0_BASE+0x00))  //control Register
#define TMPSR_0   (*(volatile unsigned int*)(TIMER0_BASE+0x04))  //pre-scare Register
#define TMLDR_0   (*(volatile unsigned int*)(TIMER0_BASE+0x08))  //load value set Register
#define TMVLR_0   (*(volatile unsigned int*)(TIMER0_BASE+0x08))  //value load get register
#define TMISR_0   (*(volatile unsigned int*)(TIMER0_BASE+0x0c))  //interrupt register
#define TMCMP_0   (*(volatile unsigned int*)(TIMER0_BASE+0x10))  //compare Register


/* Timer 1 register */
#define TMCTR_1   (*(volatile unsigned int*)(TIMER1_BASE+0x00))  //control Register
#define TMPSR_1   (*(volatile unsigned int*)(TIMER1_BASE+0x04))  //pre-scare Register
#define TMLDR_1   (*(volatile unsigned int*)(TIMER1_BASE+0x08))  //load value set Register
#define TMVLR_1   (*(volatile unsigned int*)(TIMER1_BASE+0x08))  //value load get register
#define TMISR_1   (*(volatile unsigned int*)(TIMER1_BASE+0x0c))  //interrupt register                                                                 //
#define TMCMP_1   (*(volatile unsigned int*)(TIMER1_BASE+0x10))  //compare Register


/* Timer 2 register */
#define TMCTR_2   (*(volatile unsigned int*)(TIMER2_BASE+0x00))  //control Register
#define TMPSR_2   (*(volatile unsigned int*)(TIMER2_BASE+0x04))  //pre-scare Register
#define TMLDR_2   (*(volatile unsigned int*)(TIMER2_BASE+0x08))  //load value set Register
#define TMVLR_2   (*(volatile unsigned int*)(TIMER2_BASE+0x08))  //value load get register
#define TMISR_2   (*(volatile unsigned int*)(TIMER2_BASE+0x0c))  //interrupt register                                                                  //
#define TMCMP_2   (*(volatile unsigned int*)(TIMER2_BASE+0x10))  //compare Register


/* Timer 3 register */
#define TMCTR_3   (*(volatile unsigned int*)(TIMER3_BASE+0x00))  //control Register
#define TMPSR_3   (*(volatile unsigned int*)(TIMER3_BASE+0x04))  //pre-scare Register
#define TMLDR_3   (*(volatile unsigned int*)(TIMER3_BASE+0x08))  //load value set Register
#define TMVLR_3   (*(volatile unsigned int*)(TIMER3_BASE+0x08))  //value load get register
#define TMISR_3   (*(volatile unsigned int*)(TIMER3_BASE+0x0c))  //interrupt register
#define TMCMP_3   (*(volatile unsigned int*)(TIMER3_BASE+0x10))  //compare Register


/* Timer 4 register */
#define TMCTR_4   (*(volatile unsigned int*)(TIMER4_BASE+0x00))  //control Register
#define TMPSR_4   (*(volatile unsigned int*)(TIMER4_BASE+0x04))  //pre-scare Register
#define TMLDR_4   (*(volatile unsigned int*)(TIMER4_BASE+0x08))  //load value set Register
#define TMVLR_4   (*(volatile unsigned int*)(TIMER4_BASE+0x08))  //value load get register
#define TMISR_4   (*(volatile unsigned int*)(TIMER3_BASE+0x0c))  //interrupt register
#define TMCMP_4   (*(volatile unsigned int*)(TIMER4_BASE+0x10))  //compare Register


// definitation for Timer Controller Register
/* bit 0 Timer enable */
#define TMR_ENABLE     0x0001
#define TMR_DISABLE    0x0000

/* bit 1 Interrupt enable */
#define TMR_IE_DISABLE  0x0000
#define TMR_IE_ENABLE  0x0002

/* bit 2 Output enable */
#define TMR_OE_ENABLE  0x0004   
#define TMR_OE_DISABLE 0x0000

/* bit 3 Output mode of timer */
#define TMR_OE_NORMAL  0x0000  
#define TMR_OE_PWM     0x0008

/* bit4 Up/Down counting selection */
#define TMR_UD_DOWN    0x0000   // Down Counting
#define TMR_UD_UP      0x0010   // Up   Counting

/* bit5 Up/Down counting control selection */
#define TMR_UDS_UD     0x0000   //Up/down control by bit 4 in TxCTR
#define TMR_UDS_EXTUD  0x0020   //Up/down control by EXTUDx input

/* bit6 Time output mode */
#define TMR_OM_TOGGLE	0x0000  // Toggle mode
#define TMR_OM_PULSE	0x0040  // pulse mode

/* bit 8..9 External input active edge selection */
#define TMR_ES_PE		0x0000  // Positive edge
#define TMR_ES_NE		0x0100  // Negative edge
#define TMR_ES_BOTH		0x0200  // Both edge

/* bit 10..11 Operating mode */
#define TMR_M_FREE_TIMER		0x0000 // free running timer mode
#define TMR_M_FREE_COUNTER		0x0800 // free running counter mode
#define TMR_M_PERIOD_TIMER		0x0400 // periodic timer mode
#define TMR_M_PERIOD_COUNTER	0x0c00 // periodic counter mode

/* bit 14..15 Clk source */
#define TMR_CLK_SRC_PCLK		0x0000 // PCLK
#define TMR_CLK_SRC_32768		0x4000 // 32768 Hz
#define TMR_CLK_SRC_1P6875		0x8000 // 1.6875MHz

