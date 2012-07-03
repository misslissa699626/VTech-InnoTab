#
# Makefile for the linux kernel.
#

# Object file lists.
obj-$(CONFIG_MACH_GPL32900) += hal/hal_ppu.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_disp.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_disp1.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_disp2.o
obj-$(CONFIG_MACH_GPL32900) += timer.o
obj-$(CONFIG_MACH_GPL32900) += gp_version.o

# pmpmci.o sd_core.o
obj-$(CONFIG_MACH_GPL32900) += dlmalloc.o gp_chunkmem.o
obj-$(CONFIG_MACH_GPL32900) += gp_reg.o
obj-$(CONFIG_MACH_GPL32900) += gp_cache.o
obj-$(CONFIG_MACH_GPL32900) += gp_fiq.o

obj-$(CONFIG_MACH_GPL32900) += hal/hal_dac.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_i2s.o

obj-$(CONFIG_MACH_GPL32900) += hal/hal_gpio.o gp_gpio.o 
#obj-$(CONFIG_MACH_GPL32900) += hal/hal_2d.o gp_graphic.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_clock.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_clock_list.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_ms.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_pwrc.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_usb.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_vic.o

obj-$(CONFIG_MACH_GPL32900) += gp_diag.o

obj-$(CONFIG_PM) += sleep.o gp_sram.o pm.o
obj-$(CONFIG_MACH_GPL32900) += hal/hal_uart.o gp_uart.o

obj-$(CONFIG_MACH_GPL32900)	+= rtc/
obj-$(CONFIG_MACH_GPL32900)	+= display/
obj-$(CONFIG_MACH_GPL32900)	+= ceva/
obj-$(CONFIG_MACH_GPL32900)	+= i2c-bus/
obj-$(CONFIG_MACH_GPL32900)	+= 2d/
obj-$(CONFIG_MACH_GPL32900)	+= scale/
obj-$(CONFIG_MACH_GPL32900)	+= scale2/
obj-$(CONFIG_MACH_GPL32900)	+= watchdog/
obj-$(CONFIG_MACH_GPL32900)	+= timer/
obj-$(CONFIG_MACH_GPL32900)	+= pwm/
obj-$(CONFIG_MACH_GPL32900)	+= line_buffer/
obj-$(CONFIG_MACH_GPL32900)	+= spi/
obj-$(CONFIG_MACH_GPL32900)	+= spi_nor/
obj-$(CONFIG_MACH_GPL32900)	+= usb/
obj-$(CONFIG_MACH_GPL32900)	+= apbdma0/
obj-$(CONFIG_MACH_GPL32900)	+= sensor_mgr/
obj-$(CONFIG_MACH_GPL32900)	+= csi/
obj-$(CONFIG_MACH_GPL32900)	+= tv/
obj-$(CONFIG_MACH_GPL32900)	+= ppu/
obj-$(CONFIG_MACH_GPL32900)	+= ppu_simple/
obj-$(CONFIG_MACH_GPL32900)	+= mipi/
obj-$(CONFIG_MACH_GPL32900)	+= cdsp/
obj-$(CONFIG_MACH_GPL32900)	+= aes/
obj-$(CONFIG_MACH_GPL32900)	+= gpl32900.o devices.o
obj-$(CONFIG_MACH_GPL32900)	+= gp_partition.o
obj-$(CONFIG_MACH_GPL32900)	+= spu/
obj-$(CONFIG_MACH_GPL32900)	+= sdma/

#obj-$(CONFIG_MACH_GPL32900)	+= hal/hal_sar.o gp_adc.o
obj-$(CONFIG_MACH_GPL32900)	+= adc/

obj-$(CONFIG_MACH_GPL32900) += board/board.o
#obj-$(CONFIG_MACH_GPL32900) += os/
obj-$(CONFIG_MACH_GPL32900) += clock_mgr/
#obj-$(CONFIG_MACH_GPL32900)	+= sdio/
obj-$(CONFIG_MACH_GPL32900) += sd/
obj-$(CONFIG_MACH_GPL32900) += ms/
obj-$(CONFIG_MACH_GPL32900) += audio2/
obj-$(CONFIG_MACH_GPL32900) += audio/
#Modified by Kevin
obj-$(CONFIG_MACH_GPL32900)	+= gp_nand/
#obj-$(CONFIG_MACH_GPL32900)	+= vtech_mtd/

obj-$(CONFIG_MACH_GPL32900)	+= dc2dc/
obj-$(CONFIG_MACH_GPL32900)	+= spi1/

ifeq ($(CONFIG_PM_GPFB),y)
obj-m += snapshot_nand.o
obj-m += snapshot_sd.o
endif
##obj-$(CONFIG_PM_GPFB) += snapshot.o

obj-$(CONFIG_MACH_GPL32900) += idle.o

obj-$(CONFIG_MACH_GPL32900)	+= vpad-debug/
obj-$(CONFIG_MACH_GPL32900)	+= dac-jack/

