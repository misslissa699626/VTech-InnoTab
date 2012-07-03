/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file gp_display2.c
 * @brief Display2 interface, this driver is dedicated for TV1
 * @author Anson Chuang
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/hal/hal_disp2.h>
#include <mach/hal/hal_ppu.h>
#include <mach/gp_board.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_display2.h>
#include <mach/hardware.h>
#include <mach/hal/regmap/reg_scu.h>

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
enum display_mutex {
	DISP2_MUTEX_PRIMARY = 0,
	DISP2_MUTEX_OSD0,
	DISP2_MUTEX_OSD1,
	DISP2_MUTEX_OTHERS,
	DISP2_MUTEX_MAX,
};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
	#define DEBUG	printk
#else
	#define DEBUG(...)
#endif

#define GP_DISP2_OUTPUT_MAX 1	/* max output device */

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_disp2_buf_s {
	gp_disp_bufinfo_t info;	/*!< @brief Buffer info */
	void *ptr;				/*!< @brief Buffer address */
} gp_disp2_buf_t;

typedef struct gp_disp2_info_s
{
	struct miscdevice disp2_dev;
	int32_t state;
	struct semaphore sem[DISP2_MUTEX_MAX];

	/* Interrupt */
	int32_t intFlag;

	/* Panel */
	int32_t outputIndex;
	int32_t outputDevCnt;
	gp_size_t panelSize;
	gp_disp_output_t outputDev[GP_DISP2_OUTPUT_MAX];

	/* Primary */
	uint32_t priEnable;
	gp_bitmap_t priBitmap;

	/* TV mode */
	uint32_t tvMode;
	
	/* Buffer control */
	uint32_t mmap_enable;
	gp_disp2_buf_t dispBuf[GP_DISP_BUFFER_MAX];

	/* Flip */
	uint32_t flipEnableValue;

	/* update flag */
	int32_t updateFlag;

} gp_disp2_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/



/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int32_t disp2_module_init(void);
static void disp2_module_exit(void);

static int32_t disp2_open(struct inode *inode, struct file *file);
static int32_t disp2_release(struct inode *inode, struct file *file);
static int32_t disp2_mmap(struct file *file, struct vm_area_struct *vma);
static long disp2_ioctl(struct file *file, uint32_t cmd, unsigned long arg);

static void disp2_panel_suspend(uint32_t outputIndex);
static void disp2_panel_resume(uint32_t outputIndex);


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gp_disp2_info_t *gpDisp2Info = NULL;
static DECLARE_WAIT_QUEUE_HEAD(disp2_fe_done);

/* Structure that declares the usual file */
/* access functions */
static struct file_operations disp2_fops = {
	open: disp2_open,
	release: disp2_release,
	mmap: disp2_mmap,
	unlocked_ioctl: disp2_ioctl
};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
 * \brief Output device, get empty entry
 */
static int32_t
disp2_find_output_nil_index(
	void
)
{
	int32_t i;

	for (i=0; i<GP_DISP2_OUTPUT_MAX; i++) {
		if (!gpDisp2Info->outputDev[i].ops) {
			return i;
		}
	}

	return -1;
}

/**
 * \brief Output device, find entry
 */
static int32_t
disp2_find_output_index(
	uint32_t type,
	uint8_t *name
)
{
	uint32_t i;

	for (i=0; i<GP_DISP2_OUTPUT_MAX; i++) {
		if (gpDisp2Info->outputDev[i].type == type &&
			strncmp(gpDisp2Info->outputDev[i].name, name, strlen(name)) == 0) {
			return i;
		}
	}

	return -1;
}

/**
 * \brief Muxtex lock in ioctl
 */
static uint32_t
disp2_mux_get_id(
	uint32_t cmd
)
{
	uint32_t id = cmd & 0x00f0;

	if (id == 0x10)
		return DISP2_MUTEX_PRIMARY;
	else if (id == 0x80)
		return DISP2_MUTEX_OSD0;
	else if (id == 0x90)
		return DISP2_MUTEX_OSD1;
	else
		return DISP2_MUTEX_OTHERS;
}

/**
 * \brief Muxtex lock in ioctl
 */
static uint32_t
disp2_mux_lock(
	uint32_t cmd
)
{
	uint32_t id = disp2_mux_get_id(cmd);

	if (down_interruptible(&gpDisp2Info->sem[id]) != 0) {
		return -ERESTARTSYS;
	}

	return 0;
}

/**
 * \brief Muxtex unlock in ioctl
 */
static void
disp2_mux_unlock(
	uint32_t cmd
)
{
	uint32_t id = disp2_mux_get_id(cmd);

	up(&gpDisp2Info->sem[id]);
}

/**
 * \brief Display device update parameter
 */
void
disp2_update(
	void
)
{
	gpDisp2Info->updateFlag = 1;
	disp2_wait_frame_end();
}
EXPORT_SYMBOL(disp2_update);

/**
 * \brief Display wait frame end
 */
int32_t
disp2_wait_frame_end(
	void
)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	if (gpHalDisp2GetEnable() == 0)
		return -1;

	/* Enable frame end interrupt */
	gpHalDisp2ClearIntFlag(HAL_DISP2_INT_TV_VBLANK);
	gpDisp2Info->intFlag = 0;
	gpHalDisp2SetIntEnable(HAL_DISP2_INT_TV_VBLANK);

   	wait_event_interruptible(disp2_fe_done, gpDisp2Info->intFlag != 0);

	return 0;
}
EXPORT_SYMBOL(disp2_wait_frame_end);

/**
 * \brief Display get format and type of primary layer
 */
void
disp2_set_pri_enable(
	uint32_t enable
)
{
	gpDisp2Info->priEnable = enable;
	gpHalDisp2SetEnable(enable);
}
EXPORT_SYMBOL(disp2_set_pri_enable);

/**
 * \brief Display get format and type of primary layer
 */
static uint32_t
disp2_set_pri_fmt_type(
	uint8_t srcType
)
{
	switch (srcType) {
		case SP_BITMAP_RGBA8888:
			gpHalPPUSetFbFormat(1);
			gpHalPPUSetFbMono(2);
			break;

		case SP_BITMAP_RGB565:
			gpHalPPUSetFbFormat(0);
			gpHalPPUSetFbMono(0);
			break;

		case SP_BITMAP_VYUY:
			gpHalPPUSetFbFormat(1);
			gpHalPPUSetFbMono(1);
			gpHalPPUSetYuvType(3);
			break;

		case SP_BITMAP_UYVY:
			gpHalPPUSetFbFormat(1);
			gpHalPPUSetFbMono(1);
			gpHalPPUSetYuvType(1);
			break;

		case SP_BITMAP_YVYU:
			gpHalPPUSetFbFormat(1);
			gpHalPPUSetFbMono(1);
			gpHalPPUSetYuvType(2);
			break;

		case SP_BITMAP_YUYV:
			gpHalPPUSetFbFormat(1);
			gpHalPPUSetFbMono(1);
			gpHalPPUSetYuvType(0);
			break;

		default:
			printk("[%s:%d] Error! Unkonwn type\n", __FUNCTION__, __LINE__);
			return -1;
			break;
	}

	return 0;
}

/**
 * \brief Display check format and type of primary layer
 */
static uint32_t
disp2_check_pri_fmt_type(
	uint8_t srcType
)
{
	switch (srcType) {
		case SP_BITMAP_RGBA8888:
		case SP_BITMAP_RGB565:
		case SP_BITMAP_YUYV:
		case SP_BITMAP_YVYU:
		case SP_BITMAP_UYVY:
		case SP_BITMAP_VYUY:
			break;

		default:
			printk("[%s:%d] Error! Unkonwn type\n", __FUNCTION__, __LINE__);
			return -1;
			break;
	}

	return 0;
}

/**
 * \brief Display set primary bitmap
 */
int32_t
disp2_set_pri_bitmap(
	gp_bitmap_t *pbitmap
)
{
	memcpy(&gpDisp2Info->priBitmap, pbitmap, sizeof(gp_bitmap_t));
	DEBUG("[%s:%d], DISP2_IO_SET_PRI_BITMAP, width=%d, height=%d, bpl=%d, type%d, addr=0x%x\n",
		__FUNCTION__, __LINE__, pbitmap->width, pbitmap->height, pbitmap->bpl, pbitmap->type, (uint32_t) pbitmap->pData);

	if (disp2_check_pri_fmt_type(pbitmap->type) < 0)
		return -1;

	return 0;
}
EXPORT_SYMBOL(disp2_set_pri_bitmap);


/**
 * \brief Display get format and type of primary layer
 */
void
disp2_set_pri_frame_addr(
	uint32_t addr
)
{
	gpDisp2Info->priBitmap.pData = (uint8_t*)addr;
}
EXPORT_SYMBOL(disp2_set_pri_frame_addr);

/**
 * \brief Switch panel
 */
static int32_t
disp2_switch_panel(
	uint32_t newOutputIndex
)
{
	int32_t ret;

	/* Is new panel available? */
	if (gpDisp2Info->outputDev[newOutputIndex].ops == NULL ||
		gpDisp2Info->outputDev[newOutputIndex].ops->init == NULL) {

		DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	/* Suspend old panel */
	if (gpDisp2Info->outputIndex != -1)	/* gpDisp2Info->outputIndex = -1 => Initial value */
		disp2_panel_suspend(gpDisp2Info->outputIndex);

	/* Switch to new panel */
	gpDisp2Info->outputIndex = newOutputIndex;

	/* Turn on new panel clock & set parameters */
	ret = (gpDisp2Info->outputDev[gpDisp2Info->outputIndex].ops->init)();
	if (ret < 0) {
		return ret;
	}

	/* Set data path to TFT1 or TV1 */
	if (gpDisp2Info->outputDev[gpDisp2Info->outputIndex].type == SP_DISP_OUTPUT_TV) {
		SCUA_SAR_GPIO_CTRL |= 0x21;
	}
	else {
		SCUA_SAR_GPIO_CTRL |= 0x02;
	}

	/* Init global variable */
	disp2_get_panel_res(&gpDisp2Info->panelSize);

	/* Disable primary  layer */
	disp2_set_pri_enable(0);

	disp2_update();

	disp2_wait_frame_end();

	gpDisp2Info->state = DISP_STATE_RESUME;
	return 0;
}

/**
 * \brief Display device initial
 */
static int32_t
disp2_init(
	void
)
{
	/* If current output index exist, then disable controller */
	if (gpDisp2Info->outputIndex >= 0) {
		/* Disable primary layer */
		disp2_set_pri_enable(0);

		disp2_update();
		return 0;
	}

	/* Initial output device : default index = 0 */
	return disp2_switch_panel(0);
}


/**
 * \brief Display get format and type of primary layer
 */
void
disp2_get_panel_res(
	gp_size_t *res
)
{
	gpHalDisp2GetRes(&res->width, &res->height);
}
EXPORT_SYMBOL(disp2_get_panel_res);


/**
 * \brief Display device allocate buffer
 */
void*
disp2_allocate_buffer(
	gp_disp_bufinfo_t info
)
{
	void *ptr;
	if ((info.id >= GP_DISP_BUFFER_MAX) || gpDisp2Info->dispBuf[info.id].ptr) {
		/* Occupied */
		printk("[%s:%d] Fail, id=%d\n", __FUNCTION__, __LINE__, info.id);
		return NULL;
	}
	else {
		/* Allocate buffer */
		ptr = gp_chunk_malloc(current->tgid, info.size);
		if (ptr == NULL) {
			return NULL;
		}
		gpDisp2Info->dispBuf[info.id].info = info;
		gpDisp2Info->dispBuf[info.id].ptr = ptr;
	}

	return ptr;
}
EXPORT_SYMBOL(disp2_allocate_buffer);

void
disp2_set_flip_function(
	uint32_t value
)
{
	gpDisp2Info->flipEnableValue = value;
}
EXPORT_SYMBOL(disp2_set_flip_function);

/**
 * \brief Display device free buffer
 */
int32_t
disp2_free_buffer(
	uint32_t id
)
{
	if ((id >= GP_DISP_BUFFER_MAX)) {
		printk("[%s:%d] Fail, id=%d\n", __FUNCTION__, __LINE__, id);
		return -1;
	}
	else {
		/* Free buffer */
		gp_chunk_free(gpDisp2Info->dispBuf[id].ptr);
		gpDisp2Info->dispBuf[id].ptr = NULL;
	}

	return 0;
}
EXPORT_SYMBOL(disp2_free_buffer);

/**
 * \brief Panel suspend
 */
static void
disp2_panel_suspend(
	uint32_t outputIndex
)
{
	if (gpDisp2Info->outputDev[outputIndex].ops->suspend)
		(gpDisp2Info->outputDev[outputIndex].ops->suspend)();

	return;
}

/**
 * \brief Panel resume
 */
static void
disp2_panel_resume(
	uint32_t outputIndex
)
{
	if (gpDisp2Info->outputDev[outputIndex].ops->resume)
		(gpDisp2Info->outputDev[outputIndex].ops->resume)();

	/* Restore display parameters */
	/* Primary */
	if (gpDisp2Info->priEnable) {
		disp2_set_pri_bitmap(&gpDisp2Info->priBitmap);
	}
	disp2_set_pri_enable(gpDisp2Info->priEnable);

	/* Set data path to TFT1 or TV1 */
	if (gpDisp2Info->outputDev[gpDisp2Info->outputIndex].type == SP_DISP_OUTPUT_TV) {
		SCUA_SAR_GPIO_CTRL |= 0x21;
	}
	else {
		SCUA_SAR_GPIO_CTRL |= 0x02;
	}

	disp2_update();

	return;
}

/**
 * \brief Panel driver register
 */
int32_t
register_paneldev2(
	uint32_t panelType,
	char *name,
	gp_disp_panel_ops_t *panelOps
)
{
	int32_t index;
	printk("[%s:%d], type=%d, name=%s\n", __FUNCTION__, __LINE__, panelType, name);

	/* Find empty output device entry */
	index = disp2_find_output_nil_index();
	if (index < 0) {
		printk("[%s:%d], out of memory\n", __FUNCTION__, __LINE__);
		return -1;
	}

	gpDisp2Info->outputDevCnt ++;
	gpDisp2Info->outputDev[index].type = panelType;

	memset(gpDisp2Info->outputDev[index].name, 0, sizeof(gpDisp2Info->outputDev[index].name));
	strncpy(gpDisp2Info->outputDev[index].name, name, strlen(name));

	gpDisp2Info->outputDev[index].ops = panelOps;

	return 0;
}
EXPORT_SYMBOL(register_paneldev2);

/**
 * \brief Panel driver unregister
 */
int32_t
unregister_paneldev2(
	uint32_t panelType,
	char *name
)
{
	int32_t i;
	int32_t index;

	printk("[%s:%d], type=%d, name=%s\n", __FUNCTION__, __LINE__, panelType, name);

	index = disp2_find_output_index(panelType, name);
	if (index < 0) {
		printk("[%s:%d], can not find this entry\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (index < gpDisp2Info->outputIndex) {
		gpDisp2Info->outputIndex --;
	}
	else if (index == gpDisp2Info->outputIndex) {
		gpDisp2Info->outputIndex = 0;
	}

	gpDisp2Info->outputDevCnt --;
	for (i = index; i < gpDisp2Info->outputDevCnt; i++) {
		gpDisp2Info->outputDev[i] = gpDisp2Info->outputDev[i + 1];
	}
	gpDisp2Info->outputDev[gpDisp2Info->outputDevCnt].ops = NULL;

	return 0;
}
EXPORT_SYMBOL(unregister_paneldev2);


static irqreturn_t
disp2_irq(
	int32_t irq,
	void *dev_id
)
{
	if ((gpHalDisp2GetIntStatus() & HAL_DISP2_INT_TV_VBLANK) == 0) {
		return IRQ_NONE;
	}

	/* Disable frame end interrupt & clear flag */
	gpHalDisp2SetIntDisable(HAL_DISP2_INT_TV_VBLANK);
	gpHalDisp2ClearIntFlag(HAL_DISP2_INT_TV_VBLANK);

	/* update parameters */
	if (gpDisp2Info->updateFlag) {
		gpDisp2Info->updateFlag = 0;

		/* disp2_set_pri_bitmap */
		disp2_set_pri_fmt_type(gpDisp2Info->priBitmap.type);
		gpHalDisp2SetPriFrameAddr((uint32_t) gpDisp2Info->priBitmap.pData); /* Set Frame Address */

		/* disp2_set_flip_function */
		gpHalDisp2SetFlip(gpDisp2Info->flipEnableValue);
	}

	if (gpDisp2Info->intFlag == 0) {
		gpDisp2Info->intFlag = 1;
		wake_up_interruptible(&disp2_fe_done);
	}
	return IRQ_HANDLED;
}

/**
 * \brief Open display device
 */
static int32_t
disp2_open(
	struct inode *inode,
	struct file *file
)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	/* Success */
	return 0;
}

/**
 * \brief Release display device
 */
static int32_t
disp2_release(
	struct inode *inode,
	struct file *file
)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	/* Success */
	return 0;
}

/**
 * \brief mmap of display device
 */
static int32_t
disp2_mmap(
	struct file *file,
	struct vm_area_struct *vma
)
{
	int ret;

	if (!gpDisp2Info->mmap_enable) {
		ret = -EPERM; /* disable calling mmap from user AP */
		goto out;
	}

	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;
	ret = io_remap_pfn_range(vma,
							 vma->vm_start,
							 vma->vm_pgoff,
							 vma->vm_end - vma->vm_start,
							 vma->vm_page_prot);
	if (ret != 0) {
		ret = -EAGAIN;
	}
out:
	return ret;
}

/**
 * \brief Ioctl of display1 device
 */
static long
disp2_ioctl(
	struct file *file,
	uint32_t cmd,
	unsigned long arg
)
{
	long err = 0;

	DEBUG("[%s:%d], cmd = 0x%x\n", __FUNCTION__, __LINE__, cmd);

	/* mutex lock (primary, osd0, osd1, others) */
	if (disp2_mux_lock(cmd)) {
		return -ERESTARTSYS;
	}

	switch (cmd) {
		case DISPIO_SET_INITIAL:
			err = disp2_init();
			break;

		case DISPIO_SET_UPDATE:
			disp2_update();
			break;

		case DISPIO_GET_PANEL_RESOLUTION:
			copy_to_user ((void __user *) arg, (const void *) &gpDisp2Info->panelSize, sizeof(gp_disp_res_t));
			break;

		case DISPIO_SET_SUSPEND:
			gpDisp2Info->state = DISP_STATE_SUSPEND;
			disp2_panel_suspend(gpDisp2Info->outputIndex);
			break;

		case DISPIO_SET_RESUME:
			gpDisp2Info->state = DISP_STATE_RESUME;
			disp2_panel_resume(gpDisp2Info->outputIndex);
			break;

		case DISPIO_WAIT_FRAME_END:
			disp2_wait_frame_end();
			break;

		case DISPIO_SET_TV_MODE:
			{
				//printk("tv demo test!\n");
				///*
				if (gpDisp2Info->outputDev[gpDisp2Info->outputIndex].ops->set_param) {
					if ((gpDisp2Info->outputDev[gpDisp2Info->outputIndex].ops->set_param)(&arg) < 0) {
						err = -EIO;
						break;
					}
				}				
				else {
					printk("[%s:%d]\n", __FUNCTION__, __LINE__);
					err = -EIO;
					break;
				}
        //*/
				gpDisp2Info->tvMode = (uint32_t) arg;
			}
			break;

		case DISPIO_GET_TV_MODE:
			copy_to_user ((void __user *) arg, (const void *) &gpDisp2Info->tvMode, sizeof(uint32_t));
			break;
			
		/* Primary layer */
		case DISPIO_SET_PRI_ENABLE:
			DEBUG("[%s:%d], DISPIO_SET_PRI_ENABLE = %d\n", __FUNCTION__, __LINE__, (uint32_t) arg);
			gpDisp2Info->priEnable = (uint32_t) arg;
			disp2_set_pri_enable((uint32_t) arg);
			break;

		case DISPIO_GET_PRI_ENABLE:
			DEBUG("[%s:%d], DISPIO_GET_PRI_ENABLE = %d\n", __FUNCTION__, __LINE__, gpDisp2Info->priEnable);
			copy_to_user ((void __user *) arg, (const void *) &gpDisp2Info->priEnable, sizeof(uint32_t));
			break;

		case DISPIO_SET_PRI_BITMAP:
			{
				gp_bitmap_t bitmap;

				copy_from_user((void*) &bitmap, (const void __user *) arg, sizeof(gp_bitmap_t));
				bitmap.pData = (uint8_t*) gp_user_va_to_pa(bitmap.pData);
				if (disp2_set_pri_bitmap(&bitmap) < 0) {
					err = -EIO;
				}
			}
			break;

		//sz modify , for double buffer switch 		
		case DISPIO_CHANGE_PRI_BITMAP_BUF:
			{
				gp_bitmap_t bitmap;

				copy_from_user((void*) &bitmap, (const void __user *) arg, sizeof(gp_bitmap_t));
				bitmap.pData = (uint8_t*) gp_user_va_to_pa(bitmap.pData);				
				disp2_set_pri_frame_addr((uint32_t)bitmap.pData);
			}
			break;	
		//-----------------------------------		
		
		case DISPIO_GET_PRI_BITMAP:
			copy_to_user ((void __user *) arg, (const void *) &gpDisp2Info->priBitmap, sizeof(gp_bitmap_t));
			break;

		/* Buffer control */
		case DISPIO_BUF_ALLOC:
			{
				gp_disp_bufinfo_t info;

				copy_from_user((void*) &info, (const void __user *) arg, sizeof(gp_disp_bufinfo_t));
				DEBUG("[%s:%d], Disp2_IO_BUF_ALLOC, id=%d, width=%d, height=%d, bpp=%d, size=%d\n",
					__FUNCTION__, __LINE__, info.id, info.width, info.height, info.bpp, info.size);

				if (disp2_allocate_buffer(info) == NULL) {
					err = -EIO;
					break;
				}
			}
			break;

		case DISPIO_BUF_FREE:
			{
				gp_disp_bufinfo_t info;

				copy_from_user((void*) &info, (const void __user *) arg, sizeof(gp_disp_bufinfo_t));

				if (disp2_free_buffer(info.id) < 0) {
					printk("DISPIO_BUF_FREE Fail, id=%d\n", info.id);
					err = -EIO;
					break;
				}
			}
			break;

		case DISPIO_BUF_MMAP:
			{
				unsigned long va;
				gp_disp_bufaddr_t bufaddr;
				uint32_t size;
				uint32_t pa;

				copy_from_user((void*) &bufaddr, (const void __user *) arg, sizeof(gp_disp_bufaddr_t));
				size = gpDisp2Info->dispBuf[bufaddr.id].info.size;
				pa = gp_chunk_pa(gpDisp2Info->dispBuf[bufaddr.id].ptr);

				down_write(&current->mm->mmap_sem);
				gpDisp2Info->mmap_enable = 1; /* enable mmap in Disp2_IO_BUF_MMAP */
				va = do_mmap_pgoff(
					file, 0, size,
					PROT_READ|PROT_WRITE,
					MAP_SHARED,
					pa >> PAGE_SHIFT);
				gpDisp2Info->mmap_enable = 0; /* disable it */
				up_write(&current->mm->mmap_sem);

				bufaddr.ptr = (void *)va;
				copy_to_user ((void __user *) arg, (const void *) &bufaddr, sizeof(gp_disp_bufaddr_t));
			}
			break;

		case DISPIO_BUF_MUNMAP:
			{
				gp_disp_bufaddr_t bufaddr;
				uint32_t size;

				copy_from_user((void*) &bufaddr, (const void __user *) arg, sizeof(gp_disp_bufaddr_t));
				size = gpDisp2Info->dispBuf[bufaddr.id].info.size;

				down_write(&current->mm->mmap_sem);
				do_munmap(current->mm, (unsigned int)bufaddr.ptr, size);
				up_write(&current->mm->mmap_sem);
			}
			break;

		case DISPIO_BUF_GETINFO:
			{
				gp_disp_bufinfo_t info;

				copy_from_user((void*) &info, (const void __user *) arg, sizeof(gp_disp_bufinfo_t));

				if ((info.id >= GP_DISP_BUFFER_MAX) || (gpDisp2Info->dispBuf[info.id].ptr == NULL)) {
					err = -EIO;
					break;
				}
				else {
					copy_to_user ((void __user *) arg, (const void *) &gpDisp2Info->dispBuf[info.id].info, sizeof(gp_disp_bufinfo_t));
				}
			}
			break;
		        
		case DISPIO_SET_FLIP:
			disp2_set_flip_function((uint32_t) arg);
			break;

		default:
			DEBUG("[%s:%d], unknow cmd\n", __FUNCTION__, __LINE__);
			break;
	}

	/* mutex unlock */
	disp2_mux_unlock(cmd);
	return err;
}

/**
 * @brief display device release                                              
 */                                                                         
static void
disp2_device_release(
	struct device *dev
)
{
	printk("remove display2 device ok\n");
}

static struct platform_device disp2_device = {
	.name	= "gp-disp2",
	.id		= 0,
	.dev	= {
		.release = disp2_device_release,
	},
};

#ifdef CONFIG_PM
static int
disp2_suspend(
	struct platform_device *pdev,
	pm_message_t state
)
{
	/* Panel suspend */
	gpDisp2Info->state = DISP_STATE_SUSPEND;
	disp2_panel_suspend(gpDisp2Info->outputIndex);

	return 0;
}

static int
disp2_resume(
	struct platform_device *pdev
)
{
	/* Panel Resume */
	gpDisp2Info->state = DISP_STATE_RESUME;
	disp2_panel_resume(gpDisp2Info->outputIndex);

	return 0;
}
#else                                                                       
#define disp2_suspend NULL                                                 
#define disp2_resume NULL                                                  
#endif                                                                      

/**                                                                         
 * @brief display driver define                                               
 */                                                                         
static struct platform_driver disp2_driver = {
	.suspend = disp2_suspend,
	.resume = disp2_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "gp-disp2"
	}
};

/**
 * \brief Initialize display device
 */
static int32_t __init
disp2_module_init(
	void
)
{
	int32_t retval;
	int32_t i;

	DEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	/* Initial display hal */
	if (gpHalDisp2Init() != 0) {
		DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
		retval = -EIO;
		goto fail_init;
	}

	/* malloc */
	gpDisp2Info = kmalloc(sizeof(gp_disp2_info_t), GFP_KERNEL);
	if (!gpDisp2Info) {
		printk("[%s:%d], Error\n", __FUNCTION__, __LINE__);
		retval = -ENOMEM;
		goto fail_malloc;
	}
	memset(gpDisp2Info, 0, sizeof(gp_disp2_info_t));
	gpDisp2Info->state = DISP_STATE_SUSPEND;
	gpDisp2Info->outputIndex = -1;
  gpDisp2Info->tvMode = SP_DISP_TV_MODE_NTSC;

	/* irq request */
	retval = request_irq(IRQ_2D_ENGINE, disp2_irq, IRQF_SHARED, "PPU_IRQ", (void *) gpDisp2Info);		 
	if (retval) {
		printk("[%s:%d], request_irq error %d\n", __FUNCTION__, __LINE__, retval);
		goto fail_irq;
	}

	/* init mutex */
	for (i=0; i<DISP2_MUTEX_MAX; i++) {
		init_MUTEX(&gpDisp2Info->sem[i]);
	}

	/* Registering device */
	gpDisp2Info->disp2_dev.minor = MISC_DYNAMIC_MINOR;
	gpDisp2Info->disp2_dev.name = "disp2";
	gpDisp2Info->disp2_dev.fops = &disp2_fops;
	retval = misc_register(&gpDisp2Info->disp2_dev);
	if (retval) {
		goto fail_register;
	}

	printk("disp2 dev minor : %i\n", gpDisp2Info->disp2_dev.minor);

	platform_device_register(&disp2_device);
	return platform_driver_register(&disp2_driver);

fail_register:
	free_irq(IRQ_2D_ENGINE, (void *)gpDisp2Info);
fail_irq:
	kfree(gpDisp2Info);
	gpDisp2Info = NULL;
fail_malloc:
fail_init:
	return retval;
}

/**
 * \brief Exit display device
 */
static void __exit
disp2_module_exit(
	void
)
{
	DEBUG("%s:%d\n", __FUNCTION__, __LINE__);

	if (gpDisp2Info->state == DISP_STATE_RESUME) {
		gpDisp2Info->state = DISP_STATE_SUSPEND;
		disp2_panel_suspend(gpDisp2Info->outputIndex);
	}

	/* Freeing the major number */
	misc_deregister(&gpDisp2Info->disp2_dev);
	free_irq(IRQ_2D_ENGINE, (void *)gpDisp2Info);
	kfree(gpDisp2Info);
	gpDisp2Info = NULL;

	platform_device_unregister(&disp2_device);
	platform_driver_unregister(&disp2_driver);

	DEBUG("Removing disp2 module\n");
}


/* Declaration of the init and exit functions */
module_init(disp2_module_init);
module_exit(disp2_module_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP Display2 Driver");
MODULE_LICENSE_GP;
