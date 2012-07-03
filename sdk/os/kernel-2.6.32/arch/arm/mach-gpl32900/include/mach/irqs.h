/* include/asm-arm/arch-spmp8000/irqs.h
**
**
*/

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

//////////  define for the VIC0

#define IRQ_DMAC0_M410_CH0            	   	0
#define IRQ_DMAC0_M410_CH1            	   	1
#define IRQ_DMAC1_XDMA_CH0            	   	2
#define IRQ_DMAC1_XDMA_CH1            	   	3
#define IRQ_DMAC1_XDMA_CH2            	   	4
#define IRQ_DMAC1_XDMA_CH3            	   	5
#define IRQ_DMAC1_XDMA_CHX            	   	6
#define IRQ_TIMERINT0						7
#define IRQ_TIMERINT1						8
#define IRQ_TIMERINTX						9
#define IRQ_WDT0							10
#define IRQ_WDT1							11
#define IRQ_WDTX							12
#define IRQ_I2STX							13
#define IRQ_I2SRX							14
#define IRQ_AC97							15
#define IRQ_USB_DEV							16		// device
#define IRQ_USB_EHCI                  		17		// host
#define IRQ_USB_OHCI                  		18		// host
#define IRQ_SENSOR	                    	19
#define IRQ_2D_ENGINE                    	20
#define IRQ_TVOUT	                    	21
#define IRQ_SCALE_ENGINE                  	22
#define IRQ_LCD_CTRL	 					23
#define IRQ_APBDMA_A_CH0	                24
#define IRQ_APBDMA_A_CH1	            	25
#define IRQ_APBDMA_A_CH2	             	26
#define IRQ_APBDMA_A_CH3	             	27
#define IRQ_PIU_SEM0						28		// semaphore0
#define IRQ_PIU_SEM1						29		// semaphore1
#define IRQ_PIU_SEM2						30		// semaphore2
#define IRQ_PIU_CM							31		// command/reply

///////////  define for VIC1

#define IRQ_GPIO0							32
#define IRQ_GPIO1							33
#define IRQ_PWRC							34
#define IRQ_I2C_B							35		// ARM subsystem
#define IRQ_I2C_C							36		// system
#define IRQ_REALTIME_CLOCK					37		//
#define IRQ_POWERON							38
#define IRQ_XDMA_BP							39
#define IRQ_SD0								40
#define IRQ_SD1								41
#define IRQ_CF								42
#define IRQ_BCH								43
#define IRQ_ROTATE_ENGINE					44
#define IRQ_GPIO2                           45
#define IRQ_GPIO3                           46
#define IRQ_DRM_AES_DONE					44
#define IRQ_DRM_WM_DRM_DONE					45
#define IRQ_DRM_TDES_DONE					46

#define IRQ_UART_B0							49		// ARM subsystem
#define IRQ_NAND0							50
#define IRQ_NAND1							51
#define IRQ_SSP0							52
#define IRQ_SSP1							53
#define IRQ_MS								54

#define IRQ_UART_C0							56
#define IRQ_UART_C1							57
#define IRQ_UART_C2							58
#define IRQ_SAACC							59

#define IRQ_APBDMA_C_CH0					60
#define IRQ_APBDMA_C_CH1					61
#define IRQ_APBDMA_C_CH2					62
#define IRQ_APBDMA_C_CH3					63

#define MIN_IRQ_NUM							0
#define MAX_IRQ_NUM							63
#define NR_IRQS								64

#endif
