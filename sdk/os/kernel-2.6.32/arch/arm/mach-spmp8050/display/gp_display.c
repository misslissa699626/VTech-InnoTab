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
 * @file display.c
 * @brief Display interface
 * @author Anson Chuang
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/hal/hal_disp.h>
#include <mach/gp_board.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_display.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
enum display_mutex {
	DISP_MUTEX_PRIMARY = 0,
	DISP_MUTEX_OSD0,
	DISP_MUTEX_OSD1,
	DISP_MUTEX_OTHERS,
	DISP_MUTEX_MAX,
};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
	#define DEBUG	printk
#else
	#define DEBUG(...)
#endif

#define GP_DISP_OUTPUT_MAX 3	/* max output device */

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_disp_buf_s {
	gp_disp_bufinfo_t info;	/*!< @brief Buffer info */
	void *ptr;				/*!< @brief Buffer address */
} gp_disp_buf_t;

typedef struct gp_disp_info_s
{
	struct miscdevice disp_dev;
	int32_t state;
	struct semaphore sem[DISP_MUTEX_MAX];

	/* Interrupt */
	int32_t intFlag;

	/* Panel */
	int32_t outputIndex;
	int32_t outputDevCnt;
	gp_size_t panelSize;
	gp_disp_output_t outputDev[GP_DISP_OUTPUT_MAX];

	/* Primary */
	uint32_t priEnable;
	gp_bitmap_t priBitmap;
	gp_disp_scale_t priScaleInfo;

	/* Osd */
	uint32_t osdEnable[HAL_DISP_OSD_MAX];
	gp_bitmap_t osdBitmap[HAL_DISP_OSD_MAX];
	gp_disp_scale_t osdScaleInfo[HAL_DISP_OSD_MAX];
	gp_disp_osdpalette_t osdPalette[HAL_DISP_OSD_MAX];
	uint32_t osdPaletteOffset[HAL_DISP_OSD_MAX];
	gp_disp_osdalpha_t osdAlpha[HAL_DISP_OSD_MAX];
	uint32_t osdColorKey[HAL_DISP_OSD_MAX];

	/* Dithering */
	uint32_t ditherEnable;
	uint32_t ditherType;
	gp_disp_ditherparam_t ditherParam;

	/* color matrix */
	gp_disp_colormatrix_t colorMatrix;

	/* Gamma */
	uint32_t gammaEnable;
	uint8_t gammaTable[3][256];

	/* TV mode */
	uint32_t tvMode;

	/* Buffer control */
	uint32_t mmap_enable;
	gp_disp_buf_t dispBuf[GP_DISP_BUFFER_MAX];
} gp_disp_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/



/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int32_t disp_module_init(void);
static void disp_module_exit(void);

static int32_t disp_open(struct inode *inode, struct file *file);
static int32_t disp_release(struct inode *inode, struct file *file);
static int32_t disp_mmap(struct file *file, struct vm_area_struct *vma);
static long disp_ioctl(struct file *file, uint32_t cmd, unsigned long arg);

static void disp_panel_suspend(uint32_t outputIndex);
static void disp_panel_resume(uint32_t outputIndex);


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gp_disp_info_t *gpDispInfo = NULL;
static DECLARE_WAIT_QUEUE_HEAD(disp_fe_done);

/* Structure that declares the usual file */
/* access functions */
static struct file_operations disp_fops = {
	open: disp_open,
	release: disp_release,
	mmap: disp_mmap,
	unlocked_ioctl: disp_ioctl
};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
 * \Send LCD SPI command
 */
int32_t
disp_spi(
	uint32_t val,
	uint32_t bitsLen,
	uint32_t lsbFirst
)
{
	UINT32	i, j;
	UINT32	data;
	gp_board_panel_t *panel_spi;

	panel_spi = gp_board_get_config("panel", gp_board_panel_t);

#if	0
	printk("val=%04x, bitsLen=%d, lsbFirst=%d\n",val, bitsLen, lsbFirst);
#endif

	panel_spi->set_panel_spi_cs(1);
	panel_spi->set_panel_spi_scl(1);
	panel_spi->set_panel_spi_sda(1);

	if ( lsbFirst ) {
		data = val;
	}
	else {
		data = 0;
		for ( i = 0; i < 24; i++ ) {
			if ( val & 0x800000 ) {
				data |= (0x0001 << i);
			}
			val <<= 1;
		}
		data >>= (24 - bitsLen);
	}

	panel_spi->set_panel_spi_cs(0);
	for ( i = 0; i < bitsLen; i++ ) {
		panel_spi->set_panel_spi_scl(0);
		if ( data & 0x0001 ) {
			panel_spi->set_panel_spi_sda(1);
		}
		else {
			panel_spi->set_panel_spi_sda(0);
		}
		for ( j = 0; j < 3; j++ );
		panel_spi->set_panel_spi_scl(1);
		data >>= 1;
		for ( j = 0; j < 3; j++ );

	}
	panel_spi->set_panel_spi_cs(1);

	return 0;
}
EXPORT_SYMBOL(disp_spi);
/**
 * \brief Output device, get empty entry
 */
static int32_t
disp_find_output_nil_index(
	void
)
{
	int32_t i;

	for (i=0; i<GP_DISP_OUTPUT_MAX; i++) {
		if (!gpDispInfo->outputDev[i].ops) {
			return i;
		}
	}

	return -1;
}

/**
 * \brief Output device, find entry
 */
static int32_t
disp_find_output_index(
	uint32_t type,
	uint8_t *name
)
{
	uint32_t i;

	for (i=0; i<GP_DISP_OUTPUT_MAX; i++) {
		if (gpDispInfo->outputDev[i].type == type &&
			strncmp(gpDispInfo->outputDev[i].name, name, strlen(name)) == 0) {
			return i;
		}
	}

	return -1;
}

/**
 * \brief Muxtex lock in ioctl
 */
static uint32_t
disp_mux_get_id(
	uint32_t cmd
)
{
	uint32_t id = cmd & 0x00f0;

	if (id == 0x10)
		return DISP_MUTEX_PRIMARY;
	else if (id == 0x80)
		return DISP_MUTEX_OSD0;
	else if (id == 0x90)
		return DISP_MUTEX_OSD1;
	else
		return DISP_MUTEX_OTHERS;
}

/**
 * \brief Muxtex lock in ioctl
 */
static uint32_t
disp_mux_lock(
	uint32_t cmd
)
{
	uint32_t id = disp_mux_get_id(cmd);

	if (down_interruptible(&gpDispInfo->sem[id]) != 0) {
		return -ERESTARTSYS;
	}

	return 0;
}

/**
 * \brief Muxtex unlock in ioctl
 */
static void
disp_mux_unlock(
	uint32_t cmd
)
{
	uint32_t id = disp_mux_get_id(cmd);

	up(&gpDispInfo->sem[id]);
}

/**
 * \brief Display device update parameter
 */
void
disp_update(
	void
)
{
	gpHalDispUpdateParameter();
}
EXPORT_SYMBOL(disp_update);

/**
 * \brief Display wait frame end
 */
int32_t
disp_wait_frame_end(
	void
)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpDispInfo->intFlag = 0;
   	wait_event_interruptible(disp_fe_done, gpDispInfo->intFlag != 0);
	return 0;
}
EXPORT_SYMBOL(disp_wait_frame_end);

/**
 * \brief Display get format and type of primary layer
 */
static void
disp_set_pri_enable(
	uint32_t enable
)
{
	if (enable) {
		/* Disable color bar */
		gpHalDispSetColorBarEnable(0);
	}
	else {
		/* Enable color bar */
		gpHalDispBlankInfo_t blank;

		blank.top = 0;
		blank.bottom = 0;
		blank.left = 0;
		blank.right = 0;
		blank.pattern = 0x000000;
		gpHalDispSetPriBlank(blank);
		gpHalDispSetPriRes(gpDispInfo->panelSize.width, gpDispInfo->panelSize.height);
		gpHalDispSetColorBar(0, 0xff, 0x00);
		gpHalDispSetColorBarEnable(1);
	}

	gpHalDispSetPriEnable(1);
}

/**
 * \brief Display get format and type of primary layer
 */
static uint32_t
disp_get_pri_fmt_type(
	uint8_t srcType,
	uint8_t *format,
	uint8_t *type,
	uint8_t *bpp
)
{
	*format = 0;
	*type = 0;
	*bpp = 0;
	switch (srcType) {
		case SP_BITMAP_RGB888:
			*format = HAL_DISP_INPUT_FMT_RGB;
			*type = HAL_DISP_INPUT_TYPE_RGB888;
			*bpp = 24;
			break;

		case SP_BITMAP_RGB565:
			*format = HAL_DISP_INPUT_FMT_RGB;
			*type = HAL_DISP_INPUT_TYPE_RGB565;
			*bpp = 16;
			break;

		case SP_BITMAP_RGB555:
			*format = HAL_DISP_INPUT_FMT_RGB;
			*type = HAL_DISP_INPUT_TYPE_RGB555;
			*bpp = 16;
			break;

		case SP_BITMAP_YCbYCr:
		case SP_BITMAP_YUYV:
			*format = HAL_DISP_INPUT_FMT_YCbCr;
			*type = HAL_DISP_INPUT_TYPE_YCbYCr;
			*bpp = 16;
			break;

		case SP_BITMAP_4Y4Cb4Y4Cr:
		case SP_BITMAP_4Y4U4Y4V:
			*format = HAL_DISP_INPUT_FMT_YCbCr;
			*type = HAL_DISP_INPUT_TYPE_4Y4Cb4Y4Cr;
			*bpp = 16;
			break;

		case SP_BITMAP_YCbCr:
		case SP_BITMAP_YUV:
			*format = HAL_DISP_INPUT_FMT_YCbCr;
			*type = HAL_DISP_INPUT_TYPE_YCbCr;
			*bpp = 24;
			break;

		default:
			printk("[%s:%d] Error! Unkonwn type\n", __FUNCTION__, __LINE__);
			return 1;	/* SP_FAIL */
			break;
	}

	return 0;	/* SP_OK */
}

/**
 * \brief Display set primary bitmap
 */
static void
disp_set_pri_bitmap(
	gp_bitmap_t *pbitmap
)
{
	uint8_t format, type, bpp;
	gpHalDispBlankInfo_t blankInfo;
	uint32_t hScale, vScale;

	memcpy(&gpDispInfo->priBitmap, pbitmap, sizeof(gp_bitmap_t));
	DEBUG("[%s:%d], DISPIO_SET_PRI_BITMAP, width=%d, height=%d, bpl=%d, type%d, addr=0x%x\n",
		__FUNCTION__, __LINE__, pbitmap->width, pbitmap->height, pbitmap->bpl, pbitmap->type, (uint32_t) pbitmap->pData);

	gpHalDispSetPriRes(pbitmap->width,pbitmap->height); /* Set Resolution */
	gpHalDispSetPriFrameAddr((uint32_t) pbitmap->pData); /* Set Frame Address */

	disp_get_pri_fmt_type(pbitmap->type, &format, &type, &bpp);
	gpHalDispSetPriInputInfo(format, type);	/* Set format (RGB/YCbCr) & type */
	gpHalDispSetPriPitch(pbitmap->bpl, pbitmap->width * (bpp / 8));	/* Set pitch (src & active) */

	/* Set blank according to scale info */
	blankInfo.top = gpDispInfo->priScaleInfo.y;
	blankInfo.bottom = gpDispInfo->panelSize.height - gpDispInfo->priScaleInfo.height - gpDispInfo->priScaleInfo.y;
	blankInfo.left = gpDispInfo->priScaleInfo.x;
	blankInfo.right = gpDispInfo->panelSize.width - gpDispInfo->priScaleInfo.width - gpDispInfo->priScaleInfo.x;
	blankInfo.pattern = gpDispInfo->priScaleInfo.blankcolor;
	gpHalDispSetPriBlank(blankInfo);
	if (gpDispInfo->priScaleInfo.width != pbitmap->width || gpDispInfo->priScaleInfo.height != pbitmap->height) {
		/* Scale enable */
		gpHalDispSclInfo_t scale;

		scale.srcWidth = pbitmap->width;
		scale.srcHeight = pbitmap->height;
		scale.dstWidth = gpDispInfo->priScaleInfo.width;
		scale.dstHeight = gpDispInfo->priScaleInfo.height;
		scale.hInit = 0;
		scale.vInit0 = 0;
		scale.vInit1 = 0;
		gpHalDispSetPriSclInfo(scale);

		hScale = 0;
		if (gpDispInfo->priScaleInfo.width != pbitmap->width)
			hScale = 1;

		vScale = 0;
		if (gpDispInfo->priScaleInfo.height != pbitmap->height)
			vScale = 1;

		gpHalDispSetPriSclEnable(hScale, vScale);
	}
	else {
		/* Scale NOT enable */
		gpHalDispSetPriSclEnable(0, 0);
	}

}

/**
 * \brief Display set primary scale
 */
void
disp_set_pri_scale(
	gp_disp_scale_t *pscale
)
{
	gpHalDispBlankInfo_t blankInfo;
	uint32_t hScale, vScale;

	memcpy(&gpDispInfo->priScaleInfo, pscale, sizeof(gp_disp_scale_t));
	DEBUG("[%s:%d], DISPIO_SET_PRI_SCALEINFO, x=%d, y=%d, width=%d, height=%d, blankcolor=%d\n",
		__FUNCTION__, __LINE__, pscale->x, pscale->y, pscale->width, pscale->height, pscale->blankcolor);

	/* Set blank according to scale info */
	blankInfo.top = pscale->y;
	blankInfo.bottom = gpDispInfo->panelSize.height - pscale->height - pscale->y;
	blankInfo.left = pscale->x;
	blankInfo.right = gpDispInfo->panelSize.width - pscale->width - pscale->x;
	blankInfo.pattern = pscale->blankcolor;
	gpHalDispSetPriBlank(blankInfo);
	if (pscale->width != gpDispInfo->priBitmap.width || pscale->height != gpDispInfo->priBitmap.height) {
		/* Scale enable */
		gpHalDispSclInfo_t scale;

		scale.srcWidth = gpDispInfo->priBitmap.width;
		scale.srcHeight = gpDispInfo->priBitmap.height;
		scale.dstWidth = pscale->width;
		scale.dstHeight = pscale->height;
		scale.hInit = 0;
		scale.vInit0 = 0;
		scale.vInit1 = 0;
		gpHalDispSetPriSclInfo(scale);

		hScale = 0;
		if (pscale->width != gpDispInfo->priBitmap.width)
			hScale = 1;

		vScale = 0;
		if (pscale->height != gpDispInfo->priBitmap.height)
			vScale = 1;

		gpHalDispSetPriSclEnable(hScale, vScale);
	}
	else {
		/* Scale NOT enable */
		gpHalDispSetPriSclEnable(0, 0);
	}

}

/**
 * \brief Display set osd enable
 */
void
disp_set_osd_enable(
	uint32_t layerNum,
	uint32_t enable
)
{
	gpHalDispSetOsdEnable(layerNum, enable);
}
EXPORT_SYMBOL(disp_set_osd_enable);

/**
 * \brief Display get format and type of primary layer
 */
static uint32_t
disp_get_osd_fmt(
	uint8_t srcType,
	uint32_t *format
)
{
	*format = 0;
	switch (srcType) {
		case SP_BITMAP_RGB565:
			*format = HAL_DISP_OSD_FMT_RGB565;
			break;

		case SP_BITMAP_RGAB5515:
			*format = HAL_DISP_OSD_FMT_RGB5515;
			break;

		case SP_BITMAP_ARGB1555:
			*format = HAL_DISP_OSD_FMT_RGB1555;
			break;

		default:
			printk("[%s:%d] Error! Unsupport type %d\n", __FUNCTION__, __LINE__, srcType);
			return 1;	/* SP_FAIL */
			break;
	}

	return 0;	/* SP_OK */
}

/**
 * \brief Display get format and type of primary layer
 */
void
disp_set_osd_bitmap(
	uint32_t layerNum,
	gp_bitmap_t *pbitmap
)
{
	uint32_t format;
	uint32_t hScale, vScale;

	memcpy(&gpDispInfo->osdBitmap[layerNum], pbitmap, sizeof(gp_bitmap_t));

	DEBUG("[%s:%d], DISPIO_SET_OSD_BITMAP[%d], width=%d, height=%d, bpl=%d, type%d, addr=0x%x\n",
		__FUNCTION__, __LINE__, layerNum, pbitmap->width, pbitmap->height, pbitmap->bpl, pbitmap->type, (uint32_t) pbitmap->pData);

	gpHalDispSetOsdRes(layerNum, pbitmap->width,pbitmap->height);	/* Set Resolution */
	gpHalDispSetOsdFrameAddr(layerNum, (uint32_t) pbitmap->pData); /* Set Frame Address */

	disp_get_osd_fmt(pbitmap->type, &format);
	gpHalDispSetOsdInputFmt(layerNum, format);
	gpHalDispSetOsdPitch(layerNum, pbitmap->bpl, pbitmap->width * (16 / 8));	/* Set pitch (src & active) */

	if (gpDispInfo->osdScaleInfo[layerNum].width != pbitmap->width || gpDispInfo->osdScaleInfo[layerNum].height != pbitmap->height) {
		/* Scale enable */
		gpHalDispSclInfo_t scale;

		scale.srcWidth = pbitmap->width;
		scale.srcHeight = pbitmap->height;
		scale.dstWidth = gpDispInfo->osdScaleInfo[layerNum].width;
		scale.dstHeight = gpDispInfo->osdScaleInfo[layerNum].height;
		scale.hInit = 0;
		scale.vInit0 = 0;
		scale.vInit1 = 0;
		gpHalDispSetOsdSclInfo(layerNum, scale);

		hScale = 0;
		if (gpDispInfo->osdScaleInfo[layerNum].width != pbitmap->width)
			hScale = 1;

		vScale = 0;
		if (gpDispInfo->osdScaleInfo[layerNum].height != pbitmap->height)
			vScale = 1;

		gpHalDispSetOsdSclEnable(layerNum, hScale, vScale);
	}
	else {
		/* Scale NOT enable */
		gpHalDispSetOsdSclEnable(layerNum, 0, 0);
	}

}
EXPORT_SYMBOL(disp_set_osd_bitmap);

/**
 * \brief Display set osd scale
 */
void
disp_set_osd_scale(
	uint32_t layerNum,
	gp_disp_scale_t *pscale
)
{
	uint32_t hScale, vScale;

	memcpy(&gpDispInfo->osdScaleInfo[layerNum], pscale, sizeof(gp_disp_scale_t));

	DEBUG("[%s:%d], DISPIO_SET_OSD_SCALEINFO %d, x=%d, y=%d, width=%d, height=%d, blankcolor=%d\n",
		__FUNCTION__, __LINE__, layerNum, pscale->x, pscale->y, pscale->width, pscale->height, pscale->blankcolor);

	/* Set osd layer start coordinate */
	gpHalDispSetOsdXY(layerNum, pscale->x, pscale->y);

	if (pscale->width != gpDispInfo->osdBitmap[layerNum].width || pscale->height != gpDispInfo->osdBitmap[layerNum].height) {
		/* Scale enable */
		gpHalDispSclInfo_t scale;

		scale.srcWidth = gpDispInfo->osdBitmap[layerNum].width;
		scale.srcHeight = gpDispInfo->osdBitmap[layerNum].height;
		scale.dstWidth = pscale->width;
		scale.dstHeight = pscale->height;
		scale.hInit = 0;
		scale.vInit0 = 0;
		scale.vInit1 = 0;
		gpHalDispSetOsdSclInfo(layerNum, scale);

		hScale = 0;
		if (pscale->width != gpDispInfo->osdBitmap[layerNum].width)
			hScale = 1;

		vScale = 0;
		if (pscale->height != gpDispInfo->osdBitmap[layerNum].height)
			vScale = 1;

		gpHalDispSetOsdSclEnable(layerNum, hScale, vScale);
	}
	else {
		/* Scale NOT enable */
		gpHalDispSetOsdSclEnable(layerNum, 0, 0);
	}
}
EXPORT_SYMBOL(disp_set_osd_scale);

/**
 * \brief Display get format and type of primary layer
 */
void
disp_set_osd_alpha(
	uint32_t layerNum,
	gp_disp_osdalpha_t *palpha
)
{

	memcpy(&gpDispInfo->osdAlpha[layerNum], palpha, sizeof(gp_disp_osdalpha_t));

	gpHalDispSetOsdAlpha(layerNum, palpha->consta, palpha->ppamd, palpha->alpha);

}
EXPORT_SYMBOL(disp_set_osd_alpha);

/**
 * \brief Display get format and type of primary layer
 */
void
disp_set_osd_frame_addr(
	uint32_t layerNum,
	uint8_t *addr
)
{
	gpHalDispSetOsdFrameAddr(layerNum, (uint32_t) addr); /* Set Frame Address */
}
EXPORT_SYMBOL(disp_set_osd_frame_addr);

/**
 * \brief Switch panel
 */
static int32_t
disp_switch_panel(
	uint32_t newOutputIndex
)
{
	uint32_t i;
	int32_t ret;

	/* Is new panel available? */
	if (gpDispInfo->outputDev[newOutputIndex].ops == NULL ||
		gpDispInfo->outputDev[newOutputIndex].ops->init == NULL) {

		DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	/* Suspend old panel */
	if (gpDispInfo->outputIndex != -1)	/* gpDispInfo->outputIndex = -1 => Initial value */
		disp_panel_suspend(gpDispInfo->outputIndex);

	/* Switch to new panel */
	gpDispInfo->outputIndex = newOutputIndex;

	/* Turn on new panel clock & set parameters */
	ret = (gpDispInfo->outputDev[gpDispInfo->outputIndex].ops->init)();
	if (ret < 0) {
		return ret;
	}

	/* Init global variable */
	disp_get_panel_res(&gpDispInfo->panelSize);
	gpDispInfo->priScaleInfo.x = 0;
	gpDispInfo->priScaleInfo.y = 0;
	gpDispInfo->priScaleInfo.width = gpDispInfo->panelSize.width;
	gpDispInfo->priScaleInfo.height = gpDispInfo->panelSize.height;
	gpDispInfo->priScaleInfo.blankcolor = 0;

	gpDispInfo->priBitmap.width = gpDispInfo->panelSize.width;
	gpDispInfo->priBitmap.height = gpDispInfo->panelSize.height;

	for (i=0; i<HAL_DISP_OSD_MAX; i++) {
		gpDispInfo->osdScaleInfo[i].x = 0;
		gpDispInfo->osdScaleInfo[i].y = 0;
		gpDispInfo->osdScaleInfo[i].width = gpDispInfo->panelSize.width;
		gpDispInfo->osdScaleInfo[i].height = gpDispInfo->panelSize.height;
	}

	/* Fix me */
	gpHalDispSetPriBurst(3);
	for (i=0; i<HAL_DISP_OSD_MAX; i++)
		gpHalDispSetOsdBurst(i, 3);

	/* Disable primary/osd layer */
	disp_set_pri_enable(0);
	for (i=0; i<HAL_DISP_OSD_MAX; i++) {
		disp_set_osd_enable(i, 0);
	}

	disp_update();

	/* Enable frame end interrupt */
	gpHalDispSetIntEnable(HAL_DISP_INT_FRAME_END);

	disp_wait_frame_end();

	gpDispInfo->state = DISP_STATE_RESUME;
	return 0;
}

/**
 * \brief Display device initial
 */
static int32_t
disp_init(
	void
)
{
	uint32_t i;

	/* If current output index exist, then display black screen */
	if (gpDispInfo->outputIndex >= 0) {
		/* Disable primary/osd layer */
		disp_set_pri_enable(0);
		for (i=0; i<HAL_DISP_OSD_MAX; i++) {
			disp_set_osd_enable(i, 0);
		}

		disp_update();
		return 0;
	}

	/* Initial output device : default index = 0 */
	return disp_switch_panel(0);
}

/**
 * \brief Display get format and type of primary layer
 */
void
disp_get_panel_res(
	gp_size_t *res
)
{
	gpHalDispGetRes(&res->width, &res->height);
}
EXPORT_SYMBOL(disp_get_panel_res);

/**
 * \brief Display device set dithering enable
 */
void
disp_set_dither_enable(
	uint32_t enable
)
{
	gpDispInfo->ditherEnable = enable;
	gpHalDispSetDitherEnable(enable);
}
EXPORT_SYMBOL(disp_set_dither_enable);

/**
 * \brief Display device set dithering type
 */
void
disp_set_dither_type(
	uint32_t type
)
{
	gpDispInfo->ditherType = type;
	gpHalDispSetDitherType(type);
}
EXPORT_SYMBOL(disp_set_dither_type);

/**
 * \brief Display device set dithering parameter
 */
void
disp_set_dither_param(
	gp_disp_ditherparam_t *pDitherParam
)
{
	uint32_t map0, map1;
	memcpy(&gpDispInfo->ditherParam, pDitherParam, sizeof(gp_disp_ditherparam_t));

	map0 = ((pDitherParam->d00 & 0xF) << 28 |
			(pDitherParam->d01 & 0xF) << 24 |
			(pDitherParam->d02 & 0xF) << 20 |
			(pDitherParam->d03 & 0xF) << 16 |
			(pDitherParam->d10 & 0xF) << 12 |
			(pDitherParam->d11 & 0xF) << 8 |
			(pDitherParam->d12 & 0xF) << 4 |
			(pDitherParam->d13 & 0xF));

	map1 = ((pDitherParam->d20 & 0xF) << 28 |
			(pDitherParam->d21 & 0xF) << 24 |
			(pDitherParam->d22 & 0xF) << 20 |
			(pDitherParam->d23 & 0xF) << 16 |
			(pDitherParam->d30 & 0xF) << 12 |
			(pDitherParam->d31 & 0xF) << 8 |
			(pDitherParam->d32 & 0xF) << 4 |
			(pDitherParam->d33 & 0xF));

	gpHalDispSetDitherMap(map0, map1);
}
EXPORT_SYMBOL(disp_set_dither_param);

/**
 * \brief Display device set color matrix
 */
void
disp_set_color_matrix(
	gp_disp_colormatrix_t *pColorMatrix
)
{
	uint16_t *psrc;

	memcpy(&gpDispInfo->colorMatrix, pColorMatrix, sizeof(gp_disp_colormatrix_t));
	psrc = (uint16_t *) &gpDispInfo->colorMatrix;

	gpHalDispSetColorMatrix(psrc);
}
EXPORT_SYMBOL(disp_set_color_matrix);

/**
 * \brief Display device set gamma table
 */
void
disp_set_gamma_enable(
	uint32_t enable
)
{
	gpDispInfo->gammaEnable = enable;
	gpHalDispSetGammaEnable(enable);
}
EXPORT_SYMBOL(disp_set_gamma_enable);

/**
 * \brief Display device set gamma table
 */
void
disp_set_gamma_table(
	uint32_t id,
	uint8_t *pTable
)
{
	memcpy(&gpDispInfo->gammaTable[id], pTable, sizeof(uint8_t) * 256);

	if (gpHalDispGetPriEnable() != 0) {
		disp_wait_frame_end();
	}
	gpHalDispSetGammaTable(id, (uint8_t *) &gpDispInfo->gammaTable[id]);
}
EXPORT_SYMBOL(disp_set_gamma_table);

/**
 * \brief Display device allocate buffer
 */
void*
disp_allocate_buffer(
	gp_disp_bufinfo_t info
)
{
	void *ptr;
	if ((info.id >= GP_DISP_BUFFER_MAX) || gpDispInfo->dispBuf[info.id].ptr) {
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
		gpDispInfo->dispBuf[info.id].info = info;
		gpDispInfo->dispBuf[info.id].ptr = ptr;
	}

	return ptr;
}
EXPORT_SYMBOL(disp_allocate_buffer);

/**
 * \brief Display device free buffer
 */
int32_t
disp_free_buffer(
	uint32_t id
)
{
	if ((id >= GP_DISP_BUFFER_MAX)) {
		printk("[%s:%d] Fail, id=%d\n", __FUNCTION__, __LINE__, id);
		return -1;
	}
	else {
		/* Free buffer */
		gp_chunk_free(gpDispInfo->dispBuf[id].ptr);
		gpDispInfo->dispBuf[id].ptr = NULL;
	}

	return 0;
}
EXPORT_SYMBOL(disp_free_buffer);

/**
 * \brief Panel suspend
 */
static void
disp_panel_suspend(
	uint32_t outputIndex
)
{
	if (gpDispInfo->outputDev[outputIndex].ops->suspend)
		(gpDispInfo->outputDev[outputIndex].ops->suspend)();

	return;
}

/**
 * \brief Panel resume
 */
static void
disp_panel_resume(
	uint32_t outputIndex
)
{
	uint32_t i;

	if (gpDispInfo->outputDev[outputIndex].ops->resume)
		(gpDispInfo->outputDev[outputIndex].ops->resume)();

	/* Restore display parameters */
	/* Primary */
	if (gpDispInfo->priEnable) {
		disp_set_pri_bitmap(&gpDispInfo->priBitmap);
		disp_set_pri_scale(&gpDispInfo->priScaleInfo);
	}
	disp_set_pri_enable(gpDispInfo->priEnable);

	/* Osd */
	for (i=0; i<HAL_DISP_OSD_MAX; i++) {
		if (gpDispInfo->osdEnable[i]) {
			disp_set_osd_bitmap(i, &gpDispInfo->osdBitmap[i]);
			disp_set_osd_scale(i, &gpDispInfo->osdScaleInfo[i]);

			/* palette */
			gpHalDispSetOsdInputType(i, gpDispInfo->osdPalette[i].type);
			gpHalDispSetOsdPaletteOffset(i, gpDispInfo->osdPaletteOffset[i]);
			if (i == 0)
				gpHalDispSetOsdPalette(i, 0, 256, (uint32_t*) &gpDispInfo->osdPalette[i].table[0]);
			else
				gpHalDispSetOsdPalette(i, 0, 16, (uint32_t*) &gpDispInfo->osdPalette[i].table[0]);

			disp_set_osd_alpha(i, &gpDispInfo->osdAlpha[i]);
			gpHalDispSetOsdColorKey(i, gpDispInfo->osdColorKey[i]);
			disp_set_osd_enable(i, gpDispInfo->osdEnable[i]);
		}
	}

	/* Dither */
	if (gpDispInfo->ditherEnable) {
		disp_set_dither_type(gpDispInfo->ditherType);
		disp_set_dither_param(&gpDispInfo->ditherParam);
		disp_set_dither_enable(gpDispInfo->ditherEnable);
	}

	/* Color matrix */
	disp_set_color_matrix(&gpDispInfo->colorMatrix);

	/* Gamma */
	if (gpDispInfo->gammaEnable) {
		disp_set_gamma_table(SP_DISP_GAMMA_R, (uint8_t*) &gpDispInfo->gammaTable[SP_DISP_GAMMA_R]);
		disp_set_gamma_table(SP_DISP_GAMMA_G, (uint8_t*) &gpDispInfo->gammaTable[SP_DISP_GAMMA_G]);
		disp_set_gamma_table(SP_DISP_GAMMA_B, (uint8_t*) &gpDispInfo->gammaTable[SP_DISP_GAMMA_B]);
		disp_set_gamma_enable(gpDispInfo->gammaEnable);
	}

	disp_update();

	/* Enable frame end interrupt */
	gpHalDispSetIntEnable(HAL_DISP_INT_FRAME_END);
	return;
}

/**
 * \brief Panel driver register
 */
int32_t
register_paneldev(
	uint32_t panelType,
	char *name,
	gp_disp_panel_ops_t *panelOps
)
{
	int32_t index;
	printk("[%s:%d], type=%d, name=%s\n", __FUNCTION__, __LINE__, panelType, name);

	/* Find empty output device entry */
	index = disp_find_output_nil_index();
	if (index < 0) {
		printk("[%s:%d], out of memory\n", __FUNCTION__, __LINE__);
		return -1;
	}

	gpDispInfo->outputDevCnt ++;
	gpDispInfo->outputDev[index].type = panelType;

	memset(gpDispInfo->outputDev[index].name, 0, sizeof(gpDispInfo->outputDev[index].name));
	strncpy(gpDispInfo->outputDev[index].name, name, strlen(name));

	gpDispInfo->outputDev[index].ops = panelOps;

	return 0;
}
EXPORT_SYMBOL(register_paneldev);

/**
 * \brief Panel driver unregister
 */
int32_t
unregister_paneldev(
	uint32_t panelType,
	char *name
)
{
	int32_t i;
	int32_t index;

	printk("[%s:%d], type=%d, name=%s\n", __FUNCTION__, __LINE__, panelType, name);

	index = disp_find_output_index(panelType, name);
	if (index < 0) {
		printk("[%s:%d], can not find this entry\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (index < gpDispInfo->outputIndex) {
		gpDispInfo->outputIndex --;
	}
	else if (index == gpDispInfo->outputIndex) {
		gpDispInfo->outputIndex = 0;
	}

	gpDispInfo->outputDevCnt --;
	for (i = index; i < gpDispInfo->outputDevCnt; i++) {
		gpDispInfo->outputDev[i] = gpDispInfo->outputDev[i + 1];
	}
	gpDispInfo->outputDev[gpDispInfo->outputDevCnt].ops = NULL;

	return 0;
}
EXPORT_SYMBOL(unregister_paneldev);


static irqreturn_t
disp_irq(
	int32_t irq,
	void *dev_id
)
{
	gpHalDispClearIntFlag(HAL_DISP_INT_FRAME_END); /* Clear interrupt flag */
	if (gpDispInfo->intFlag == 0) {
		gpDispInfo->intFlag = 1;
		wake_up_interruptible(&disp_fe_done);
	}
	return IRQ_HANDLED;
}

/**
 * \brief Open display device
 */
static int32_t
disp_open(
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
disp_release(
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
disp_mmap(
	struct file *file,
	struct vm_area_struct *vma
)
{
	int ret;

	if (!gpDispInfo->mmap_enable) {
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
 * \brief Ioctl of display device
 */
static long
disp_ioctl(
	struct file *file,
	uint32_t cmd,
	unsigned long arg
)
{
	long err = 0;

	DEBUG("[%s:%d], cmd = 0x%x\n", __FUNCTION__, __LINE__, cmd);

	/* mutex lock (primary, osd0, osd1, others) */
	if (disp_mux_lock(cmd)) {
		return -ERESTARTSYS;
	}

	switch (cmd) {
		case DISPIO_SET_INITIAL:
			err = disp_init();
			break;

		case DISPIO_SET_UPDATE:
			disp_update();
			break;

		case DISPIO_GET_PANEL_RESOLUTION:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->panelSize, sizeof(gp_disp_res_t));
			break;

		case DISPIO_GET_PANEL_SIZE:
			{
				gp_size_t size;
				if (gpDispInfo->outputDev[gpDispInfo->outputIndex].ops->get_size) {
					(gpDispInfo->outputDev[gpDispInfo->outputIndex].ops->get_size)(&size);
					copy_to_user ((void __user *) arg, (const void *) &size, sizeof(gp_size_t));
				}
				else {
					err = -EIO;
				}
			}
			break;

		case DISPIO_SET_OUTPUT:
			err = disp_switch_panel((uint32_t) arg);
			break;

		case DISPIO_GET_OUTPUT:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->outputIndex, sizeof(uint32_t));
			break;

		case DISPIO_ENUM_OUTPUT:
			{
				uint32_t index;
				gp_disp_output_t *pOutputDev = (gp_disp_output_t *) arg;

				copy_from_user((void*) &index, (const void __user *) &pOutputDev->index, sizeof(uint32_t));
				if (index < gpDispInfo->outputDevCnt) {
					copy_to_user ((void __user *) &pOutputDev->type, (const void *) &gpDispInfo->outputDev[index].type, sizeof(uint32_t));
					copy_to_user ((void __user *) &pOutputDev->name, (const void *) &gpDispInfo->outputDev[index].name, sizeof(gpDispInfo->outputDev[index].name));
				}
				else {
					err = -EIO;
				}
			}
			break;

		case DISPIO_SET_SUSPEND:
			gpDispInfo->state = DISP_STATE_SUSPEND;
			disp_panel_suspend(gpDispInfo->outputIndex);
			break;

		case DISPIO_SET_RESUME:
			gpDispInfo->state = DISP_STATE_RESUME;
			disp_panel_resume(gpDispInfo->outputIndex);
			break;

		case DISPIO_SET_BACKLIGHT:
			{
				gp_board_panel_t *panel_config;
				panel_config = gp_board_get_config("panel", gp_board_panel_t);
				if (panel_config == NULL || panel_config->set_backlight == NULL) {
					printk("Panel backlight not found!\n");
				}
				else {
					panel_config->set_backlight((uint32_t) arg);
				}
			}
			break;

		case DISPIO_WAIT_FRAME_END:
			disp_wait_frame_end();
			break;

		case DISPIO_DUMP_REGISTER:
			gpHalDispDumpRegister();
			break;

		case DISPIO_SET_TV_MODE:
			{
				int i;

				for (i=0; i<GP_DISP_OUTPUT_MAX; i++) {
					if (gpDispInfo->outputDev[i].type == SP_DISP_OUTPUT_TV)
						break;
				}

				if (i == GP_DISP_OUTPUT_MAX) {
					err = -EIO;
					printk("[%s:%d], Error, Max number\n", __FUNCTION__, __LINE__);
					break;
				}

				if (gpDispInfo->outputDev[i].ops->set_param) {
					if ((gpDispInfo->outputDev[i].ops->set_param)(&arg) < 0) {
						err = -EIO;
						break;
					}
				}
				else {
					printk("[%s:%d]\n", __FUNCTION__, __LINE__);
					err = -EIO;
					break;
				}

				gpDispInfo->tvMode = (uint32_t) arg;
			}
			break;

		case DISPIO_GET_TV_MODE:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->tvMode, sizeof(uint32_t));
			break;

		/* Primary layer */
		case DISPIO_SET_PRI_ENABLE:
			DEBUG("[%s:%d], DISPIO_SET_PRI_ENABLE = %d\n", __FUNCTION__, __LINE__, (uint32_t) arg);
			gpDispInfo->priEnable = (uint32_t) arg;
			disp_set_pri_enable((uint32_t) arg);
			break;

		case DISPIO_GET_PRI_ENABLE:
			DEBUG("[%s:%d], DISPIO_GET_PRI_ENABLE = %d\n", __FUNCTION__, __LINE__, gpDispInfo->priEnable);
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->priEnable, sizeof(uint32_t));
			break;

		case DISPIO_SET_PRI_BITMAP:
			{
				gp_bitmap_t bitmap;

				copy_from_user((void*) &bitmap, (const void __user *) arg, sizeof(gp_bitmap_t));
				bitmap.pData = (uint8_t*) gp_user_va_to_pa(bitmap.pData);
				disp_set_pri_bitmap(&bitmap);
			}
			break;

		case DISPIO_GET_PRI_BITMAP:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->priBitmap, sizeof(gp_bitmap_t));
			break;

		case DISPIO_SET_PRI_SCALEINFO:
			{
				gp_disp_scale_t scale;

				copy_from_user((void*) &scale, (const void __user *) arg, sizeof(gp_disp_scale_t));
				disp_set_pri_scale(&scale);
			}
			break;

		case DISPIO_GET_PRI_SCALEINFO:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->priScaleInfo, sizeof(gp_disp_scale_t));
			break;

		/* Dithering */
		case DISPIO_SET_DITHER_ENABLE:
			disp_set_dither_enable((uint32_t) arg);
			break;

		case DISPIO_GET_DITHER_ENABLE:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->ditherEnable, sizeof(uint32_t));
			break;

		case DISPIO_SET_DITHER_TYPE:
			disp_set_dither_type((uint32_t) arg);
			break;

		case DISPIO_GET_DITHER_TYPE:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->ditherType, sizeof(uint32_t));
			break;

		case DISPIO_SET_DITHER_PARAM:
			{
				gp_disp_ditherparam_t ditherParam;
				copy_from_user((void*) &ditherParam, (const void __user *) arg, sizeof(gp_disp_ditherparam_t));
				disp_set_dither_param((gp_disp_ditherparam_t *) &ditherParam);
			}
			break;

		case DISPIO_GET_DITHER_PARAM:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->ditherParam, sizeof(gp_disp_ditherparam_t));
			break;

		/* Color matrix */
		case DISPIO_SET_CMATRIX_PARAM:
			{
				gp_disp_colormatrix_t colorMatrix;
				copy_from_user((void*) &colorMatrix, (const void __user *) arg, sizeof(gp_disp_colormatrix_t));
				disp_set_color_matrix(&colorMatrix);
			}
			break;

		case DISPIO_GET_CMATRIX_PARAM:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->colorMatrix, sizeof(gp_disp_colormatrix_t));
			break;

		/* Gamma table */
		case DISPIO_SET_GAMMA_ENABLE:
			disp_set_gamma_enable((uint32_t) arg);
			break;

		case DISPIO_GET_GAMMA_ENABLE:
			copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->gammaEnable, sizeof(uint32_t));
			break;

		case DISPIO_SET_GAMMA_PARAM:
			{
				gp_disp_gammatable_t *pGammaTable;
				pGammaTable = kmalloc(sizeof(gp_disp_gammatable_t), GFP_KERNEL);
				if (!pGammaTable) {
					err = -ENOMEM;
					break;
				}
				copy_from_user((void*) pGammaTable, (const void __user *) arg, sizeof(gp_disp_gammatable_t));
				DEBUG("Set gamma parameter, id=%d\n", pGammaTable->id);
				disp_set_gamma_table(pGammaTable->id, (uint8_t*) &pGammaTable->table);
				kfree(pGammaTable);
			}
			break;

		case DISPIO_GET_GAMMA_PARAM:
			{
				gp_disp_gammatable_t *pGammaTable = (gp_disp_gammatable_t *)arg;
				uint32_t id;

				copy_from_user((void*) &id, (const void __user *) &pGammaTable->id, sizeof(uint32_t));
				DEBUG("Get gamma parameter, id=%d\n", id);
				copy_to_user ((void __user *) &pGammaTable->table, (const void *) gpDispInfo->gammaTable[id], sizeof(uint8_t) * 256);
			}
			break;

		/* Color bar */
		case DISPIO_SET_CBAR_ENABLE:
			gpHalDispSetColorBarEnable((uint32_t) arg);
			break;

		case DISPIO_GET_CBAR_ENABLE:
			{
				uint32_t enable;
				enable = gpHalDispGetColorBarEnable();
				DEBUG("[%s:%d], DISPIO_GET_CBAR_ENABLE = %d\n", __FUNCTION__, __LINE__, enable);
				copy_to_user ((void __user *) arg, (const void *) &enable, sizeof(uint32_t));
			}
			break;

		case DISPIO_SET_CBARINFO:
			{
				gp_disp_colorbar_t colorBar;

				copy_from_user((void*) &colorBar, (const void __user *) arg, sizeof(gp_disp_colorbar_t));
				DEBUG("[%s:%d], DISPIO_SET_CBARINFO, type=%d, size=%d, color=%d\n",
					__FUNCTION__, __LINE__, colorBar.type, colorBar.size, colorBar.color);

				gpHalDispSetColorBar(colorBar.type, colorBar.size, colorBar.color);
			}
			break;

		/* Buffer control */
		case DISPIO_BUF_ALLOC:
			{
				gp_disp_bufinfo_t info;

				copy_from_user((void*) &info, (const void __user *) arg, sizeof(gp_disp_bufinfo_t));
				DEBUG("[%s:%d], DISPIO_BUF_ALLOC, id=%d, width=%d, height=%d, bpp=%d, size=%d\n",
					__FUNCTION__, __LINE__, info.id, info.width, info.height, info.bpp, info.size);

				if (disp_allocate_buffer(info) == NULL) {
					err = -EIO;
					break;
				}
			}
			break;

		case DISPIO_BUF_FREE:
			{
				gp_disp_bufinfo_t info;

				copy_from_user((void*) &info, (const void __user *) arg, sizeof(gp_disp_bufinfo_t));

				if (disp_free_buffer(info.id) < 0) {
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
				size = gpDispInfo->dispBuf[bufaddr.id].info.size;
				pa = gp_chunk_pa(gpDispInfo->dispBuf[bufaddr.id].ptr);

				down_write(&current->mm->mmap_sem);
				gpDispInfo->mmap_enable = 1; /* enable mmap in DISPIO_BUF_MMAP */
				va = do_mmap_pgoff(
					file, 0, size,
					PROT_READ|PROT_WRITE,
					MAP_SHARED,
					pa >> PAGE_SHIFT);
				gpDispInfo->mmap_enable = 0; /* disable it */
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
				size = gpDispInfo->dispBuf[bufaddr.id].info.size;

				down_write(&current->mm->mmap_sem);
				do_munmap(current->mm, (unsigned int)bufaddr.ptr, size);
				up_write(&current->mm->mmap_sem);
			}
			break;

		case DISPIO_BUF_GETINFO:
			{
				gp_disp_bufinfo_t info;

				copy_from_user((void*) &info, (const void __user *) arg, sizeof(gp_disp_bufinfo_t));

				if ((info.id >= GP_DISP_BUFFER_MAX) || (gpDispInfo->dispBuf[info.id].ptr == NULL)) {
					err = -EIO;
					break;
				}
				else {
					copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->dispBuf[info.id].info, sizeof(gp_disp_bufinfo_t));
				}
			}
			break;

		/* Osd layer */
		case DISPIO_GET_OSD_TOTALNUM:
			{
				uint32_t num = HAL_DISP_OSD_MAX;
				copy_to_user ((void __user *) arg, (const void *) &num, sizeof(uint32_t));
			}
			break;

		case DISPIO_SET_OSD_ENABLE(0):
		case DISPIO_SET_OSD_ENABLE(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gpDispInfo->osdEnable[layerNum] = (uint32_t) arg;
				disp_set_osd_enable(layerNum, (uint32_t) arg);
			}
			break;

		case DISPIO_GET_OSD_ENABLE(0):
		case DISPIO_GET_OSD_ENABLE(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;

				DEBUG("[%s:%d], DISPIO_GET_OSD_ENABLE = %d\n", __FUNCTION__, __LINE__, gpDispInfo->osdEnable[layerNum]);
				copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->osdEnable[layerNum], sizeof(uint32_t));
			}
			break;

		case DISPIO_SET_OSD_BITMAP(0):
		case DISPIO_SET_OSD_BITMAP(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gp_bitmap_t bitmap;

				copy_from_user((void*) &bitmap, (const void __user *) arg, sizeof(gp_bitmap_t));
				bitmap.pData = (uint8_t*) gp_user_va_to_pa(bitmap.pData);
				disp_set_osd_bitmap(layerNum, &bitmap);
			}
			break;

		case DISPIO_GET_OSD_BITMAP(0):
		case DISPIO_GET_OSD_BITMAP(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->osdBitmap[layerNum], sizeof(gp_bitmap_t));
			}
			break;

		case DISPIO_SET_OSD_SCALEINFO(0):
		case DISPIO_SET_OSD_SCALEINFO(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gp_disp_scale_t scale;

				copy_from_user((void*) &scale, (const void __user *) arg, sizeof(gp_disp_scale_t));
				disp_set_osd_scale(layerNum, &scale);
			}
			break;

		case DISPIO_GET_OSD_SCALEINFO(0):
		case DISPIO_GET_OSD_SCALEINFO(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->osdScaleInfo[layerNum], sizeof(gp_disp_scale_t));
			}
			break;

		case DISPIO_SET_OSD_PALETTE(0):
		case DISPIO_SET_OSD_PALETTE(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gp_disp_osdpalette_t *pOsdPalette;

				pOsdPalette = kmalloc(sizeof(gp_disp_osdpalette_t), GFP_KERNEL);
				copy_from_user((void*) pOsdPalette, (const void __user *) arg, sizeof(gp_disp_osdpalette_t));
				DEBUG("[%s:%d], set palette, index=%d, count=%d\n", __FUNCTION__, __LINE__, pOsdPalette->startIndex, pOsdPalette->count);
				gpDispInfo->osdPalette[layerNum].type = pOsdPalette->type;
				memcpy(&gpDispInfo->osdPalette[layerNum].table[pOsdPalette->startIndex], &pOsdPalette->table[0], sizeof(uint32_t) * pOsdPalette->count);
				gpHalDispSetOsdInputType(layerNum, gpDispInfo->osdPalette[layerNum].type);
				gpHalDispSetOsdPalette(layerNum, pOsdPalette->startIndex, pOsdPalette->count, (uint32_t*) &pOsdPalette->table[0]);
				kfree(pOsdPalette);
			}
			break;

		case DISPIO_GET_OSD_PALETTE(0):
		case DISPIO_GET_OSD_PALETTE(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gp_disp_osdpalette_t *pOsdPalette = (gp_disp_osdpalette_t *) arg;
				uint32_t startIndex;
				uint32_t count;

				copy_from_user((void*) &startIndex, (const void __user *) &pOsdPalette->startIndex, sizeof(uint32_t));
				copy_from_user((void*) &count, (const void __user *) &pOsdPalette->count, sizeof(uint32_t));
				DEBUG("[%s:%d], get palette, index=%d, count=%d\n", __FUNCTION__, __LINE__, startIndex, count);
				copy_to_user ((void __user *) &pOsdPalette->type, (const void *) &gpDispInfo->osdPalette[layerNum].type, sizeof(uint32_t));
				copy_to_user ((void __user *) &pOsdPalette->table[0], (const void *) &gpDispInfo->osdPalette[layerNum].table[startIndex], sizeof(uint32_t) * count);
			}
			break;

		case DISPIO_SET_OSD_PALETTEOFFSET(0):
		case DISPIO_SET_OSD_PALETTEOFFSET(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gpDispInfo->osdPaletteOffset[layerNum] = (uint32_t) arg;
				gpHalDispSetOsdPaletteOffset(layerNum, (uint32_t) arg);
			}
			break;

		case DISPIO_GET_OSD_PALETTEOFFSET(0):
		case DISPIO_GET_OSD_PALETTEOFFSET(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->osdPaletteOffset[layerNum], sizeof(uint32_t));
			}
			break;

		case DISPIO_SET_OSD_ALPHA(0):
		case DISPIO_SET_OSD_ALPHA(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gp_disp_osdalpha_t alpha;

				copy_from_user((void*) &alpha, (const void __user *) arg, sizeof(gp_disp_osdalpha_t));
				disp_set_osd_alpha(layerNum, &alpha);
			}
			break;

		case DISPIO_GET_OSD_ALPHA(0):
		case DISPIO_GET_OSD_ALPHA(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->osdAlpha[layerNum], sizeof(gp_disp_osdalpha_t));
			}
			break;

		case DISPIO_SET_OSD_KEY(0):
		case DISPIO_SET_OSD_KEY(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				gpDispInfo->osdColorKey[layerNum] = (uint32_t) arg;
				gpHalDispSetOsdColorKey(layerNum, (uint32_t) arg);
			}
			break;

		case DISPIO_GET_OSD_KEY(0):
		case DISPIO_GET_OSD_KEY(1):
			{
				uint32_t layerNum = ((cmd & 0xff) - DISPIO_OSD_BASE) >> 4;
				copy_to_user ((void __user *) arg, (const void *) &gpDispInfo->osdColorKey[layerNum], sizeof(uint32_t));
			}
			break;

		default:
			DEBUG("[%s:%d], unknow cmd\n", __FUNCTION__, __LINE__);
			break;
	}

	/* mutex unlock */
	disp_mux_unlock(cmd);
	return err;
}

/**
 * \brief Initialize display device
 */
static int32_t __init
disp_module_init(
	void
)
{
	int32_t retval;
	int32_t i;

	DEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	/* Initial display hal */
	if (gpHalDispInit() != 0) {
		DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
		retval = -EIO;
		goto fail_init;
	}

	/* malloc */
	gpDispInfo = kmalloc(sizeof(gp_disp_info_t), GFP_KERNEL);
	if (!gpDispInfo) {
		printk("[%s:%d], Error\n", __FUNCTION__, __LINE__);
		retval = -ENOMEM;
		goto fail_malloc;
	}
	memset(gpDispInfo, 0, sizeof(gp_disp_info_t));
	gpDispInfo->state = DISP_STATE_SUSPEND;
	gpDispInfo->outputIndex = -1;
	gpDispInfo->tvMode = SP_DISP_TV_MODE_NTSC;

	/* irq request */
	retval = request_irq(IRQ_LCD_CTRL, disp_irq, IRQF_DISABLED, "LCD_CTRL", (void *) gpDispInfo);
	if (retval) {
		printk("[%s:%d], request_irq error %d\n", __FUNCTION__, __LINE__, retval);
		goto fail_irq;
	}

	/* init mutex */
	for (i=0; i<DISP_MUTEX_MAX; i++) {
		init_MUTEX(&gpDispInfo->sem[i]);
	}

	/* Registering device */
	gpDispInfo->disp_dev.minor = MISC_DYNAMIC_MINOR;
	gpDispInfo->disp_dev.name = "disp0";
	gpDispInfo->disp_dev.fops = &disp_fops;
	retval = misc_register(&gpDispInfo->disp_dev);
	if (retval) {
		goto fail_register;
	}

	printk("disp dev minor : %i\n", gpDispInfo->disp_dev.minor);

	return 0;

fail_register:
	free_irq(IRQ_LCD_CTRL, (void *)gpDispInfo);
fail_irq:
	kfree(gpDispInfo);
	gpDispInfo = NULL;
fail_malloc:
fail_init:
	return retval;
}

/**
 * \brief Exit display device
 */
static void __exit
disp_module_exit(
	void
)
{
	DEBUG("%s:%d\n", __FUNCTION__, __LINE__);

	if (gpDispInfo->state == DISP_STATE_RESUME) {
		gpDispInfo->state = DISP_STATE_SUSPEND;
		disp_panel_suspend(gpDispInfo->outputIndex);
	}

	/* Freeing the major number */
	misc_deregister(&gpDispInfo->disp_dev);
	free_irq(IRQ_LCD_CTRL, (void *)gpDispInfo);
	kfree(gpDispInfo);
	gpDispInfo = NULL;

	DEBUG("Removing disp module\n");
}


/* Declaration of the init and exit functions */
module_init(disp_module_init);
module_exit(disp_module_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP Display Driver");
MODULE_LICENSE_GP;
