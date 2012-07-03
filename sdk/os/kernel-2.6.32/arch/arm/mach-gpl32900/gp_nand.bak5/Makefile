#
# Makefile for the MS card
#

## Modified by Kevin
#obj-m += nand_hal.o	
obj-y += nand_hal.o
	nand_hal-objs := gp_nand_hal/hal_nand_dev.o\
	gp_nand_hal/hal_nand.o\
	gp_nand_hal/hal_nand_bch.o\
	gp_nand_hal/hal_nand_smart_id.o\
	gp_nand_hal/hal_nand_bthdr.o

## Added by Kevin
obj-y += nand_mtd.o	
	nand_mtd-objs := \
	vtech_mtd/innotab_nand_dev.o\
	vtech_mtd/nand_innotab.o
#	gp_nand_hal/hal_nand.o\
#	gp_nand_hal/hal_nand_bch.o\
#	gp_nand_hal/hal_nand_smart_id.o\
#	gp_nand_hal/hal_nand_bthdr.o\
	

obj-y += gp_blk_app.o
	gp_blk_app-objs := gp_nand_app/app_dev.o\
	gp_nand_app/drv_l2_nand_app.o\
	gp_nand_config/drv_l2_nand_config.o
	
##Modified by Kevin
obj-m += gp_nand_module.o
#obj-n += gp_nand_module.o
	gp_nand_module-objs := gp_nand_data/nand_data_dev.o\
	gp_nand_data/drv_l2_nand_global.o\
	gp_nand_data/drv_l2_nand_manage.o\
	gp_nand_data/drv_l2_nand_test.o\
	gp_nand_config/drv_l2_nand_config.o

##Modified by Kevin
obj-m += gp_nand_module2.o
#obj-n += gp_nand_module2.o
	gp_nand_module2-objs := gp_nand_data/nand_data_dev2.o\
	gp_nand_data/drv_l2_nand_global.o\
	gp_nand_data/drv_l2_nand_manage.o\
	gp_nand_data/drv_l2_nand_test.o\
	gp_nand_config/drv_l2_nand_config.o
