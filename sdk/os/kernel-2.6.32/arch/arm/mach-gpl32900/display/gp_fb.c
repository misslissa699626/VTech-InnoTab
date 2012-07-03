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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <asm/mach/map.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

#include <mach/module.h>
#include <mach/typedef.h>
#include <mach/gp_display.h>
#include <mach/gp_chunkmem.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define FB_BUFFER_ID 0

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct gp_fb_info {
	gp_size_t panelRes;
	gp_rect_t panelRect;
	gp_disp_bufinfo_t bufinfo;
	uint32_t bitmapType;
	void __iomem *fbmem;
	uint32_t first_open;
};

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct fb_info *fbinfo;
static char driver_name[] = "gp-fb";

/* FB parameter
	type : RGB565, RGB888, RGBA8888
	buffer_width
	buffer_height
	buffer_size
	panel_rect_x
	panel_rect_y
	panel_rect_width
	panel_rect_height
*/
static char *fbparam[] = {"RGB565", "0", "0", "0", "0", "0", "0", "0"};
static int nstrs = 8;
module_param_array(fbparam, charp, &nstrs, S_IRUGO);

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static int32_t
gp_fb_open(
	struct fb_info *fbinfo,
	int32_t user
)
{
	struct gp_fb_info *info;

	info = fbinfo->par;
	if (info->first_open == 0) {
		info->first_open = 1;
		/* Enable osd0 */
		disp_set_osd_enable(0, 1);

		disp_update();
	}

	return 0;
}

static int32_t
gp_fb_release(
	struct fb_info *fbinfo,
	int32_t user
)
{
	return 0;
}

static int32_t
gp_fb_check_var(
	struct fb_var_screeninfo *var,
	struct fb_info *fbinfo
)
{
	if (var->bits_per_pixel != 16) {
		return -EINVAL;
	}
	return 0;
}

static int32_t
gp_fb_set_par(
	struct fb_info *fbinfo
)
{
	return 0;
}

#if 0
static int32_t
gp_fb_setcolreg(
	unsigned regno,
	unsigned red,
	unsigned green,
	unsigned blue,
	unsigned transp,
	struct fb_info *fbinfo
)
{
	printk("gp_fb_setcolreg(%d, %d, %d, %d, %d\n",
		regno, red, green, blue, transp);

	return 0;
}
#endif

static int32_t
gp_fb_pan_display(
	struct fb_var_screeninfo *var,
	struct fb_info *fbinfo
)
{
	//struct gp_fb_info *fbi = info->par;
	uint32_t addr;

	fbinfo->var.xoffset = var->xoffset;
	fbinfo->var.yoffset = var->yoffset;

	addr = fbinfo->fix.smem_start + var->yoffset * fbinfo->fix.line_length;
	/* Set frame address */
	disp_set_osd_frame_addr(0, (uint8_t*) addr);

	disp_update();
	disp_wait_frame_end();
	return 0;
}

static void
gp_fb_activate(
	struct fb_info *fbinfo
)
{
	struct gp_fb_info *info;
	gp_bitmap_t osdBitmap;
	gp_disp_scale_t osdScale;
	gp_disp_osdalpha_t osdAlpha;

	info = fbinfo->par;
	/* Set osd0 bitmap */
	osdBitmap.width = fbinfo->var.xres;
	osdBitmap.height = fbinfo->var.yres;
	osdBitmap.bpl = fbinfo->fix.line_length;
	osdBitmap.type = info->bitmapType;
	osdBitmap.pData = (uint8_t*) fbinfo->fix.smem_start;
	disp_set_osd_bitmap(0, &osdBitmap);

	/* Set osd layer 0 scale */
	osdScale.x = info->panelRect.x;
	osdScale.y = info->panelRect.y;
	osdScale.width = info->panelRect.width;
	osdScale.height = info->panelRect.height;
	disp_set_osd_scale(0, &osdScale);

	/* Set osd alpha & color key */
	osdAlpha.consta = SP_DISP_ALPHA_CONSTANT;
	osdAlpha.ppamd = SP_DISP_ALPHA_PERPIXEL_ONLY;
	osdAlpha.alpha = 100;
	disp_set_osd_alpha(0, &osdAlpha);

	/* Enable osd0, move to gp_fb_open */
	//disp_set_osd_enable(0, 1);

	disp_update();
}

static int
gp_fb_setColorType(
	struct fb_info *fbinfo
)
{
	struct gp_fb_info *info;

	info = fbinfo->par;
	if (strncmp(fbparam[0], "RGB565", strlen(fbparam[0])) == 0) {
		info->bitmapType = SP_BITMAP_RGB565;
		fbinfo->var.bits_per_pixel	= 16;
		fbinfo->var.transp.offset	= 0;
		fbinfo->var.red.offset		= 11;
		fbinfo->var.green.offset	= 5;
		fbinfo->var.blue.offset		= 0;
		fbinfo->var.transp.length	= 0;
		fbinfo->var.red.length		= 5;
		fbinfo->var.green.length	= 6;
		fbinfo->var.blue.length		= 5;
	}
	else if (strncmp(fbparam[0], "RGB888", strlen(fbparam[0])) == 0) {
		info->bitmapType = SP_BITMAP_RGB888;
		fbinfo->var.bits_per_pixel	= 24;
		fbinfo->var.transp.offset	= 0;
		fbinfo->var.red.offset		= 16;
		fbinfo->var.green.offset	= 8;
		fbinfo->var.blue.offset		= 0;
		fbinfo->var.transp.length	= 0;
		fbinfo->var.red.length		= 8;
		fbinfo->var.green.length	= 8;
		fbinfo->var.blue.length		= 8;
	}
	else if (strncmp(fbparam[0], "RGBA8888", strlen(fbparam[0])) == 0) {
		/* FIX ME */
		info->bitmapType = SP_BITMAP_ARGB8888;
		fbinfo->var.bits_per_pixel	= 32;
		fbinfo->var.transp.offset	= 0;
		fbinfo->var.red.offset		= 24;
		fbinfo->var.green.offset	= 16;
		fbinfo->var.blue.offset		= 8;
		fbinfo->var.transp.length	= 8;
		fbinfo->var.red.length		= 8;
		fbinfo->var.green.length	= 8;
		fbinfo->var.blue.length		= 8;
	}
	else {
		printk("%s\n", fbparam[0]);
		return -1;
	}

	fbinfo->var.grayscale = 0;
    fbinfo->var.nonstd	= 0;

	return 0;
}

static int
gp_fb_getBufInfo(
	struct fb_info *fbinfo
)
{
	struct gp_fb_info *info;
	char *pEnd;

	info = fbinfo->par;
	info->bufinfo.id = FB_BUFFER_ID;
	/* width */
	if (strncmp(fbparam[1], "0", strlen(fbparam[1])) == 0) {
		info->bufinfo.width = info->panelRes.width;
	}
	else {
		info->bufinfo.width = (uint32_t)simple_strtoul(fbparam[1], &pEnd, 10);
	}
	/* height */
	if (strncmp(fbparam[2], "0", strlen(fbparam[2])) == 0) {
		info->bufinfo.height = info->panelRes.height;
	}
	else {
		info->bufinfo.height = (uint32_t)simple_strtoul(fbparam[2], &pEnd, 10);
	}
	/* size */
	if (strncmp(fbparam[3], "0", strlen(fbparam[3])) == 0) {
		info->bufinfo.size = info->bufinfo.width * info->bufinfo.height * 2 * fbinfo->var.bits_per_pixel / 8;
	}
	else {
		info->bufinfo.size = (uint32_t)simple_strtoul(fbparam[3], &pEnd, 10);
	}

	info->bufinfo.bpp = fbinfo->var.bits_per_pixel;

	return 0;
}

static int
gp_fb_getPanelRect(
	struct fb_info *fbinfo
)
{
	struct gp_fb_info *info;
	char *pEnd;

	info = fbinfo->par;
	/* x */
	if (strncmp(fbparam[4], "0", strlen(fbparam[4])) == 0) {
		info->panelRect.x = 0;
	}
	else {
		info->panelRect.x = (uint32_t)simple_strtoul(fbparam[4], &pEnd, 10);
	}
	/* y */
	if (strncmp(fbparam[5], "0", strlen(fbparam[5])) == 0) {
		info->panelRect.y = 0;
	}
	else {
		info->panelRect.y = (uint32_t)simple_strtoul(fbparam[5], &pEnd, 10);
	}
	/* width */
	if (strncmp(fbparam[6], "0", strlen(fbparam[6])) == 0) {
		info->panelRect.width = info->panelRes.width;
	}
	else {
		info->panelRect.width = (uint32_t)simple_strtoul(fbparam[6], &pEnd, 10);
	}
	/* height */
	if (strncmp(fbparam[7], "0", strlen(fbparam[7])) == 0) {
		info->panelRect.height = info->panelRes.height;
	}
	else {
		info->panelRect.height = (uint32_t)simple_strtoul(fbparam[7], &pEnd, 10);
	}
	return 0;
}

static struct fb_ops gp_fb_ops = {
	.owner          = THIS_MODULE,
	.fb_open        = gp_fb_open,
	.fb_release     = gp_fb_release,
	.fb_check_var	= gp_fb_check_var,
	.fb_set_par	    = gp_fb_set_par,
#if 0
	.fb_setcolreg	= gp_fb_setcolreg,
#endif
	.fb_pan_display = gp_fb_pan_display,
};

static int32_t __init
gp_fb_init(
	void
)
{
	int32_t ret = 0;
	struct gp_fb_info *info;

#if 0
	/* Init display device */
	if (disp_open(0, 0) < 0)
		return -EIO;

	if (disp_init() < 0)
		return -EIO;
#endif

	fbinfo = framebuffer_alloc(sizeof(struct gp_fb_info), NULL);
	if (!fbinfo)
		return -ENOMEM;

	info = fbinfo->par;
	info->first_open = 0;

	/*Get panel resolution */
	disp_get_panel_res(&info->panelRes);

	/* Color */
	ret = gp_fb_setColorType(fbinfo);
	if (ret < 0) {
		printk("[%s:%d], set color type error\n", __FUNCTION__, __LINE__);
		goto dealloc_fb;
	}

	/* Get buffer width, height, and size */
	gp_fb_getBufInfo(fbinfo);

	/* Get panel rect */
	gp_fb_getPanelRect(fbinfo);

	info->fbmem = disp_allocate_buffer(info->bufinfo);
	if (info->fbmem == NULL) {
		ret = -ENXIO;
		goto dealloc_fb;
	}

	memset(info->fbmem, 0x00, info->bufinfo.size);

	fbinfo->fbops = &gp_fb_ops;
	fbinfo->flags = FBINFO_FLAG_DEFAULT;
	fbinfo->pseudo_palette = NULL;
	fbinfo->screen_base = info->fbmem;
	fbinfo->screen_size = 0;

	/* Resolution */
	fbinfo->var.xres = info->bufinfo.width;
	fbinfo->var.yres = info->bufinfo.height;
	fbinfo->var.xres_virtual = fbinfo->var.xres;
	fbinfo->var.yres_virtual = fbinfo->var.yres * 2;

	/* Timing */
	fbinfo->var.left_margin = 0;
	fbinfo->var.right_margin = 0;
	fbinfo->var.upper_margin = 0;
	fbinfo->var.lower_margin = 0;
	fbinfo->var.hsync_len = 0;
	fbinfo->var.vsync_len = 0;

	fbinfo->var.activate = FB_ACTIVATE_FORCE;
	fbinfo->var.accel_flags = 0;
	fbinfo->var.vmode = FB_VMODE_NONINTERLACED;

	/* fixed info */
	strcpy(fbinfo->fix.id, driver_name);
	fbinfo->fix.mmio_start  = 0x93000000;
	fbinfo->fix.mmio_len    = 0x1000;
	fbinfo->fix.type        = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	= 0;
	fbinfo->fix.visual      = FB_VISUAL_TRUECOLOR;
	fbinfo->fix.xpanstep	= 0;
	fbinfo->fix.ypanstep	= 1;
	fbinfo->fix.ywrapstep	= 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;
	fbinfo->fix.smem_start  = gp_chunk_pa(info->fbmem);
	fbinfo->fix.smem_len    = info->bufinfo.size;
	fbinfo->fix.line_length = (fbinfo->var.xres_virtual * fbinfo->var.bits_per_pixel) / 8;

	gp_fb_activate(fbinfo);

	ret = register_framebuffer(fbinfo);
	if (ret < 0) {
		printk(KERN_ERR "Failed to register framebuffer device: %d\n", ret);
		goto release_fbmem;
	}
	printk(KERN_INFO "fb%d: %s frame buffer device\n",
		fbinfo->node, fbinfo->fix.id);

	return 0;

release_fbmem:
	disp_free_buffer(FB_BUFFER_ID);

dealloc_fb:
	framebuffer_release(fbinfo);

	return ret;
}

/*
 *  Cleanup
 */
static void __exit
gp_fb_exit(
	void
)
{
	/*struct gp_fb_info *info = fbinfo->par;*/

	disp_free_buffer(FB_BUFFER_ID);
	unregister_framebuffer(fbinfo);
	framebuffer_release(fbinfo);
	fbinfo = NULL;

	return;
}

module_init(gp_fb_init);
module_exit(gp_fb_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP frame buffer driver");
MODULE_LICENSE_GP;
