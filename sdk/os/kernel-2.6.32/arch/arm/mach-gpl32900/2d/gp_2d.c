/**
 * @file    gp_2d.c
 * @brief    2D driver interface.
 * @author  clhuang
 * @since   2010-10-07
 * @date    2010-10-07
 */
#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/typedef.h>
#include <mach/diag.h>
#include <mach/gp_2d.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_line_buffer.h>
#include <mach/hal/hal_2d.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define G2D_DELTA_SIGN_OFFSET   15
#define G2D_DELTA_NUMB_OFFSET   7
#define G2D_DELTA_FLOAT_OFFSET  0
#define G2D_DELTA_SIGN_MASK     0x1
#define G2D_DELTA_NUMB_MASK     0xff
#define G2D_DELTA_FLOAT_MASK    0x7f

/**************************************************************************
 *                               M A C R O S                       *
 **************************************************************************/
#define GP_CPY_RECT(dst, src) {\
    dst.x = src.x;\
    dst.y = src.y;\
    dst.width = src.width;\
    dst.height = src.height;\
}

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static unsigned int hw_status = 0;
static int open_count = 0;
static int hw_down = 0;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static DECLARE_WAIT_QUEUE_HEAD(gp_2d_done_wait);
static DEFINE_MUTEX(gp_2d_mutex);

/**
 * @brief   2D driver clear wait condition function
 */
static void gp_2d_clear_wait_condition(void)
{
	hw_down = 0;
}

/**
 * @brief   2D driver wait exec done function
 */
static int gp_2d_wait_exec_done(void)
{
	wait_event_interruptible(gp_2d_done_wait, (hw_down == 1));
	
	return 0;
}

/**
 * @brief   2D driver wakeup exec done function
 */
static int gp_2d_wakeup_exec_done(void)
{
	hw_down = 1;
	wake_up_interruptible(&gp_2d_done_wait);
	
	return 0;
}

/**
 * @brief   2D driver open function
 */
static int gp_2d_open(struct inode *ip, struct file *fp)
{
	mutex_lock(&gp_2d_mutex);
	if (open_count == 0) {
		gpHal2dClkEnable(SP_TRUE);

		gpHal2dInit(SP_TRUE);
		gpHal2dEnableInterrupt(SP_TRUE);
	}
	open_count++;
	mutex_unlock(&gp_2d_mutex);

	return 0;
}

/**
 * @brief   2D driver release function
 */
static int gp_2d_release(struct inode *ip, struct file* fp)
{
	mutex_lock(&gp_2d_mutex);
	if(open_count == 1) {	
		gpHal2dClkEnable(SP_FALSE);
	}
	open_count--;
	mutex_unlock(&gp_2d_mutex);
	
	return 0;
}

#ifdef G2D_DEBUG
/**
 * @brief   2D driver hexdump function
 */
static void hexdump(unsigned char *buf, unsigned int len)
{
    int i = 0;
    while (len--) {
        if (i % 32 == 0) {
            DIAG_DEBUG("\n");
        }
        DIAG_DEBUG("%02x", *buf++);
        i++;
    }
    DIAG_DEBUG("\n");
}
#endif
/**
 * @brief   2D driver convert delta rgb function
 */
unsigned short gp_2d_convert_delta_rgb(int value)
{
    unsigned int neg_value;
    unsigned short hal_value;

    if (value < 0) {
        neg_value = 1;
        value = -value;
    }
    else {
        neg_value = 0;
    }

    hal_value = ((neg_value & G2D_DELTA_SIGN_MASK) << G2D_DELTA_SIGN_OFFSET)
        | (((value / 100) & G2D_DELTA_NUMB_MASK) << G2D_DELTA_NUMB_OFFSET)
        | ((value % 100) & G2D_DELTA_FLOAT_MASK);


    return hal_value;
}

/**
 * @brief   2D driverdraw bitmap function
 */
int gp_2d_draw_bitmap(g2d_draw_ctx_t *draw_ctx)
{
    unsigned int src_addr_phy;
    unsigned int dst_addr_phy;
    unsigned int msk_addr_phy;
    unsigned int src_addr_kernel;
    unsigned int dst_addr_kernel;
    unsigned int msk_addr_kernel;
    gp2dGrdtCtx_t grdt_ctx;
    unsigned int length;
    spRect_t src_rect;
    spRect_t dst_rect;
    spRect_t tmp_rect;
    spRectSize_t msk_size;
    spRectSize_t src_size = {draw_ctx->src.width, draw_ctx->src.height};
    spRectSize_t dst_size = {draw_ctx->dst.width, draw_ctx->dst.height};
    spPoint_t src_ref_point = {draw_ctx->src_rect.x, draw_ctx->src_rect.y};
    spPoint_t dst_ref_point = {draw_ctx->dst_rect.x, draw_ctx->dst_rect.y};
    int ret = SP_OK;

    DIAG_DEBUG("\nsrc data func=%x,fg_rop=%x,bpl=%d,type=%d,addr=%x,addrU=%x,.", draw_ctx->func_flag,
        draw_ctx->rop.fg_rop, draw_ctx->src.bpl,
        draw_ctx->src.type,(unsigned int)draw_ctx->src.pData, (unsigned int)draw_ctx->src.pDataU);

    if ((draw_ctx->func_flag & G2D_FUNC_ROP) && (draw_ctx->func_flag & G2D_FUNC_BLEND)) {
        return -EINVAL;
    }

    mutex_lock(&gp_2d_mutex);
    gp_line_buffer_request(LINE_BUFFER_MODULE_2D, 1024);
    src_addr_phy = gp_user_va_to_pa(draw_ctx->src.pData);
    dst_addr_phy = gp_user_va_to_pa(draw_ctx->dst.pData);
    GP_CPY_RECT(src_rect, draw_ctx->src_rect);
    GP_CPY_RECT(dst_rect, draw_ctx->dst_rect);
    dst_addr_kernel = (unsigned int)gp_chunk_va(dst_addr_phy);                  /* phy_addr to kernel_addr */
    if (dst_addr_kernel == 0) {
        ret = -EINVAL;
        DIAG_ERROR("CHUNK_MEM_FREE: not a chunkmem address!\n");
        goto out_draw_bitmap;
    }
    src_addr_kernel = (unsigned int)gp_chunk_va(src_addr_phy);                  /* phy_addr to kernel_addr */
    if (src_addr_kernel == 0) {
        ret = -EINVAL;
        DIAG_ERROR("CHUNK_MEM_FREE: not a chunkmem address!\n");
        goto out_draw_bitmap;
    }

    gpHal2dInit(SP_FALSE);
    gpHal2dEnableInterrupt(SP_TRUE);
    DIAG_DEBUG("\nsrc bitmap,w=%d,h=%d,type=%d,rect=[%d,%d,%d,%d].",
        src_size.width, src_size.height, draw_ctx->src.type,
        src_rect.x, src_rect.y, src_rect.width, src_rect.height);
    gpHal2dSetSrcBitmap(src_addr_phy, src_size, draw_ctx->src.type, src_rect);
    DIAG_DEBUG("\ndst bitmap,w=%d,h=%d,type=%d,rect=[%d,%d,%d,%d].",
        dst_size.width, dst_size.height, draw_ctx->dst.type,
        dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height);
    gpHal2dSetDstBitmap(dst_addr_phy, &dst_size, draw_ctx->dst.type, &dst_rect);

    if (draw_ctx->func_flag & G2D_FUNC_ROP) {
        gpHal2dEnableRop(draw_ctx->rop.fg_rop, draw_ctx->rop.fg_pattern);

        /*enable gradient fill*/
        if (draw_ctx->func_flag & G2D_FUNC_GRDT_FILL) {
            DIAG_DEBUG("\ng2d gradient fill, h enable=%d,dir=%d,delta hrgb=%d,%d,%d.",
                draw_ctx->grdt_fill.hor_enable, draw_ctx->grdt_fill.hor_left_right,
                draw_ctx->grdt_fill.hor_delta_r,
                draw_ctx->grdt_fill.hor_delta_g, draw_ctx->grdt_fill.hor_delta_b);
            if (draw_ctx->grdt_fill.hor_enable != 0) {
                grdt_ctx.flipDir = draw_ctx->grdt_fill.hor_left_right;
                grdt_ctx.deltaR = gp_2d_convert_delta_rgb(draw_ctx->grdt_fill.hor_delta_r);
                grdt_ctx.deltaG = gp_2d_convert_delta_rgb(draw_ctx->grdt_fill.hor_delta_g);
                grdt_ctx.deltaB = gp_2d_convert_delta_rgb(draw_ctx->grdt_fill.hor_delta_b);
                gpHal2dEnableGradientFill(G2DENG_GRDT_HORIZONTAL, &grdt_ctx);
                DIAG_DEBUG("\ng2d gradient in, dir=%d,delta hrgb=%d,%d,%d.",
                    grdt_ctx.flipDir, grdt_ctx.deltaR,grdt_ctx.deltaG, grdt_ctx.deltaB);
            }
            if (draw_ctx->grdt_fill.ver_enable != 0) {
                grdt_ctx.flipDir = draw_ctx->grdt_fill.ver_top_bottom;
                grdt_ctx.deltaR = gp_2d_convert_delta_rgb(draw_ctx->grdt_fill.ver_delta_r);
                grdt_ctx.deltaG = gp_2d_convert_delta_rgb(draw_ctx->grdt_fill.ver_delta_g);
                grdt_ctx.deltaB = gp_2d_convert_delta_rgb(draw_ctx->grdt_fill.ver_delta_b);
                gpHal2dEnableGradientFill(G2DENG_GRDT_VERTICAL, &grdt_ctx);
            }
        }

        /*enable mask*/
        if (draw_ctx->func_flag & G2D_FUNC_MASK) {
            msk_addr_phy = gp_user_va_to_pa((void*)draw_ctx->mask.msk_addr);
            msk_addr_kernel = (unsigned int)gp_chunk_va(msk_addr_phy);
            if (draw_ctx->mask.load_msk != 0 && msk_addr_kernel != 0) {
                gpHal2dSetStippleMsk (*((unsigned int*)msk_addr_kernel), *((unsigned int*)msk_addr_kernel+1));
            }
            msk_size.width =  draw_ctx->mask.msk_width;
            msk_size.height =  draw_ctx->mask.msk_height;
            gpHal2dEnableMsk(draw_ctx->mask.msk_type, draw_ctx->mask.fifo_mode,
                draw_ctx->mask.bg_rop, draw_ctx->mask.bg_pattern, msk_addr_phy, &msk_size);

        }
    }

    if (draw_ctx->func_flag & G2D_FUNC_BLEND) {
        gpHal2dEnableBlend(draw_ctx->blend.alpha_fmt, draw_ctx->blend.blend_op,
            draw_ctx->blend.src_alpha, draw_ctx->blend.dst_alpha);
    }

    if (draw_ctx->func_flag & G2D_FUNC_TROP) {
        gpHal2dEnableTrop(draw_ctx->trop.src_hi_color_key, draw_ctx->trop.src_lo_color_key,
            draw_ctx->trop.dst_hi_color_key, draw_ctx->trop.dst_lo_color_key, draw_ctx->trop.trop);
    }

    if (draw_ctx->func_flag & G2D_FUNC_SCALE) {
        gpHal2dEnableScale(draw_ctx->src_rect.width, draw_ctx->src_rect.height,
            draw_ctx->dst_rect.width, draw_ctx->dst_rect.height);
    }

    /*rotate and mirror*/
    if ((draw_ctx->rotate.rotation != G2D_ROTATE_0) || (draw_ctx->rotate.flip != 0)) {
        GP_CPY_RECT(tmp_rect, dst_rect);
        if (draw_ctx->rotate.flip == 0) {
            if (draw_ctx->rotate.rotation == G2D_ROTATE_90) {
                tmp_rect.x = dst_rect.x + dst_rect.height;
                tmp_rect.y = dst_rect.y;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
            else if (draw_ctx->rotate.rotation == G2D_ROTATE_180) {
                tmp_rect.x = dst_rect.x + dst_rect.width;
                tmp_rect.y = dst_rect.y + dst_rect.height;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
            else if (draw_ctx->rotate.rotation == G2D_ROTATE_270) {
                tmp_rect.x = dst_rect.x;
                tmp_rect.y = dst_rect.y + dst_rect.width;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
        }
        else {
            if (draw_ctx->rotate.rotation == G2D_ROTATE_0) {
                tmp_rect.x = dst_rect.x;
                tmp_rect.y = dst_rect.y + dst_rect.height;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
            else if (draw_ctx->rotate.rotation == G2D_ROTATE_90) {
                tmp_rect.x = dst_rect.x + dst_rect.height;
                tmp_rect.y = dst_rect.y + dst_rect.width;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
            else if (draw_ctx->rotate.rotation == G2D_ROTATE_180) {
                tmp_rect.x = dst_rect.x + dst_rect.width;
                tmp_rect.y = dst_rect.y;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
            else if (draw_ctx->rotate.rotation == G2D_ROTATE_270) {
                tmp_rect.x = dst_rect.x;
                tmp_rect.y = dst_rect.y;
                length = (dst_size.width > dst_size.height ? dst_size.height : dst_size.width) / 2;
                src_ref_point.y = 0;
                src_ref_point.x = 0;
                dst_ref_point = src_ref_point;

            }
        }

        gpHal2dEnableRotate(draw_ctx->rotate.rotation, draw_ctx->rotate.flip, &src_ref_point, &dst_ref_point, &tmp_rect);
    }

    if (draw_ctx->func_flag & G2D_FUNC_CLIP) {
        GP_CPY_RECT(dst_rect, draw_ctx->clip.clip_rect);
        gpHal2dEnableClip(draw_ctx->clip.clip_type, &dst_rect);
    }

    DIAG_DEBUG("[%s:%s:%d] Test !\n", __FILE__, __FUNCTION__, __LINE__);
    gp_2d_clear_wait_condition();
    gpHal2dExec();
    gp_2d_wait_exec_done();

    #ifdef G2D_DEBUG
    DIAG_DEBUG("\nstatus=%x.", hw_status);
    hexdump((unsigned char*)dst_addr_kernel, 512);
    gpHal2dDump();
    #endif

out_draw_bitmap:
    gp_line_buffer_release(LINE_BUFFER_MODULE_2D);
    mutex_unlock(&gp_2d_mutex);

    return ret;
}

/**
 * @brief   2D driver ioctl function
 */
static int gp_2d_ioctl(struct inode* ip, struct file* fp, unsigned int cmd, unsigned long arg)
{
    g2d_draw_ctx_t draw_ctx;
	long ret = 0;

    DIAG_DEBUG("[%s:%s:%d] Test !\n", __FILE__, __FUNCTION__, __LINE__);
	switch (cmd) {
    case G2D_IOCTL_DRAW_BITMAP:
        if (copy_from_user(&draw_ctx, (void __user*)arg, sizeof(draw_ctx))) {
            return -EFAULT;
        }
        gp_2d_draw_bitmap(&draw_ctx);
        DIAG_DEBUG("[%s:%s:%d] Test !\n", __FILE__, __FUNCTION__, __LINE__);
        break;
    default:
		ret = -ENOTTY;
	}
	
	return ret;
}


/* file operations for /dev/eac */
static struct file_operations gp_2d_fops = {
	.owner = THIS_MODULE,
	.open = gp_2d_open,
	.release = gp_2d_release,
    .ioctl = gp_2d_ioctl,
};

static struct miscdevice gp_2d_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "graphic",
	.fops = &gp_2d_fops,
};


/**
 * @brief   2D driver irq function
 */
static irqreturn_t gp_2d_irq(int irq, void *dev_id)
{
    hw_status = gpHal2dGetStatus();

	gpHal2dClearInterrupt();
	gp_2d_wakeup_exec_done();
	
	return IRQ_HANDLED;
}


/**
 * @brief   2D driver probe function
 */
static int gp_2d_probe(struct platform_device *pdev)
{
	int ret= -ENOENT;
	struct device *dev = &pdev->dev;

	ret = request_irq(IRQ_2D_ENGINE, gp_2d_irq, IRQF_DISABLED, "G2D_IRQ", dev);
	if (ret < 0) {
		dev_err(dev, "g2d can't get irq %i, err %d\n", IRQ_2D_ENGINE, ret);
		return ret;
	}

	if((ret = misc_register(&gp_2d_misc))) {
		printk("misc_register returned %d in goldfish_audio_init\n", ret);
		free_irq(IRQ_2D_ENGINE, dev);
		return ret;
	}

	return 0;
}

/**
 * @brief   2D driver remove function
 */
static int gp_2d_remove(struct platform_device *pdev)
{
	misc_deregister(&gp_2d_misc);
	free_irq(IRQ_2D_ENGINE, &pdev->dev);
	return 0;
}

static struct platform_driver gp_2d_driver = {
	.probe		= gp_2d_probe,
	.remove		= gp_2d_remove,
	.suspend    = NULL,
	.resume     = NULL,
	.driver = {
		.name = "gp_graphic"
	}
};

/*****************************************************************
 * Graphic Device Info
*****************************************************************/
struct platform_device gp_2d_device = {
	.name	= "gp_graphic",
	.id		= -1,
};

/**
 * @brief   2D  init function
 */
static int __init gp_2d_init(void)
{
	int ret;

	ret = platform_driver_register(&gp_2d_driver);
	if (ret < 0) {
		DIAG_ERROR("platform_driver_register returned %d\n", ret);
		return ret;
	}

	ret = platform_device_register(&gp_2d_device);
	if (ret) {
		dev_err(&(gp_2d_device.dev), "unable to register device: %d\n", ret);
	}
	
	return ret;
}

/**
 * @brief   2D driver exit function
 */
static void __exit gp_2d_exit(void)
{
	platform_device_unregister(&gp_2d_device);
	platform_driver_unregister(&gp_2d_driver);
}

module_init(gp_2d_init);
module_exit(gp_2d_exit);


/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("8050 2D Engine Driver");
MODULE_LICENSE_GP;
