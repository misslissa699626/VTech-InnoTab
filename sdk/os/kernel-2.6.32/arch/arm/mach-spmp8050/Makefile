#
# Makefile for the linux kernel.
#

# Object file lists.

obj-$(CONFIG_MACH_SPMP8050) += hal/hal_disp.o
obj-$(CONFIG_MACH_SPMP8050) += timer.o
# pmpmci.o sd_core.o
obj-$(CONFIG_MACH_SPMP8050) += dlmalloc.o gp_chunkmem.o
obj-$(CONFIG_MACH_SPMP8050) += gp_reg.o
obj-$(CONFIG_MACH_SPMP8050) += gp_cache.o

obj-$(CONFIG_MACH_SPMP8050) += hal/hal_gpio.o gp_gpio.o 
#obj-$(CONFIG_MACH_SPMP8050) += hal/hal_2d.o gp_graphic.o
obj-$(CONFIG_MACH_SPMP8050) += hal/hal_clock.o
obj-$(CONFIG_MACH_SPMP8050) += hal/hal_ms.o
obj-$(CONFIG_MACH_SPMP8050) += hal/hal_usb.o

obj-$(CONFIG_MACH_SPMP8050) += gp_diag.o

obj-$(CONFIG_PM) += sleep.o spmp_sram.o pm.o
obj-$(CONFIG_MACH_SPMP8050) += hal/hal_uart.o gp_uart.o

obj-$(CONFIG_MACH_SPMP8050)	+= rtc/
obj-$(CONFIG_MACH_SPMP8050)	+= display/
obj-$(CONFIG_MACH_SPMP8050)	+= ceva/
obj-$(CONFIG_MACH_SPMP8050)	+= i2c-bus/
obj-$(CONFIG_MACH_SPMP8050)	+= 2d/
obj-$(CONFIG_MACH_SPMP8050)	+= rotate/
obj-$(CONFIG_MACH_SPMP8050)	+= scale/
obj-$(CONFIG_MACH_SPMP8050)	+= watchdog/
obj-$(CONFIG_MACH_SPMP8050)	+= timer/
obj-$(CONFIG_MACH_SPMP8050)	+= pwm/
obj-$(CONFIG_MACH_SPMP8050)	+= line_buffer/
obj-$(CONFIG_MACH_SPMP8050)	+= spi/
obj-$(CONFIG_MACH_SPMP8050)	+= spi_nor/
obj-$(CONFIG_MACH_SPMP8050)	+= usb/
obj-$(CONFIG_MACH_SPMP8050)	+= apbdma0/
obj-$(CONFIG_MACH_SPMP8050)	+= csi/

obj-$(CONFIG_MACH_SPMP8050)	+= spmp8050.o devices.o

#obj-$(CONFIG_MACH_SPMP8050)	+= hal/hal_sar.o gp_adc.o
obj-$(CONFIG_MACH_SPMP8050)	+= adc/

obj-$(CONFIG_MACH_SPMP8050) += board/board.o
#obj-$(CONFIG_MACH_SPMP8050) += os/
obj-$(CONFIG_MACH_SPMP8050) += clock_mgr/
obj-$(CONFIG_MACH_SPMP8050) += sd/
#obj-$(CONFIG_MACH_SPMP8050) += sdio/
obj-$(CONFIG_MACH_SPMP8050) += ms/
obj-$(CONFIG_MACH_SPMP8050)  += audio/
obj-$(CONFIG_MACH_SPMP8050)	+= nand/

