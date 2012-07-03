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
 * @file    gp_scale.c
 * @brief   Implement of scale driver.
 * @author  qinjian
 * @since   2010/10/9
 * @date    2010/10/9
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/gp_scale.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_line_buffer.h>
#include <mach/hal/hal_scale.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Scale default timeout (ms) */
#define SCALE_DEFAULT_TIMEOUT   3000

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define POLLING_TEST    0
#define DEBUG_DUMP      0

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct scale_info_s {
	struct miscdevice dev;          /*!< @brief scale device */
	struct semaphore sem;           /*!< @brief mutex semaphore for scale ops */
	wait_queue_head_t done_wait;    /*!< @brief scaling done wait queue */
	bool done;                      /*!< @brief scaling done flag */
	unsigned int open_count;        /*!< @brief scale device open count */
} scale_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static scale_info_t *scale = NULL;

#if DEBUG_DUMP
/**
 * @brief   Scale parameter dump function
 * @param   ctx [in] scale parameter
 */
static void scale_param_dump(scale_content_t *ctx)
{
	DIAG_PRINTF("src_img.type     = %d\n", ctx->src_img.type);
	DIAG_PRINTF("src_img.width    = %d\n", ctx->src_img.width);
	DIAG_PRINTF("src_img.height   = %d\n", ctx->src_img.height);
	DIAG_PRINTF("src_img.bpl      = %d\n", ctx->src_img.bpl);
	DIAG_PRINTF("src_img.strideUV = %d\n", ctx->src_img.strideUV);
	DIAG_PRINTF("clip_rgn.x       = %d\n", ctx->clip_rgn.x);
	DIAG_PRINTF("clip_rgn.y       = %d\n", ctx->clip_rgn.y);
	DIAG_PRINTF("clip_rgn.width   = %d\n", ctx->clip_rgn.width);
	DIAG_PRINTF("clip_rgn.height  = %d\n", ctx->clip_rgn.height);
	DIAG_PRINTF("dst_img.type     = %d\n", ctx->dst_img.type);
	DIAG_PRINTF("dst_img.width    = %d\n", ctx->dst_img.width);
	DIAG_PRINTF("dst_img.height   = %d\n", ctx->dst_img.height);
	DIAG_PRINTF("dst_img.bpl      = %d\n", ctx->dst_img.bpl);
	DIAG_PRINTF("scale_rgn.x      = %d\n", ctx->scale_rgn.x);
	DIAG_PRINTF("scale_rgn.y      = %d\n", ctx->scale_rgn.y);
	DIAG_PRINTF("scale_rgn.width  = %d\n", ctx->scale_rgn.width);
	DIAG_PRINTF("scale_rgn.height = %d\n", ctx->scale_rgn.height);
}
#endif

/**
 * @brief   Scale dither param setting function
 * @param   dither [in] scale dither parameter
 * @return  success: 0,  fail: errcode
 * @see
 */
static int scale_set_dither(scale_dither_t *dither)
{
	int ret;

	if (down_interruptible(&scale->sem) != 0) {
		return -ERESTARTSYS;
	}

	ret = gpHalScaleSetDither(dither->mode,
							  dither->sequence,
							  dither->map_upper,
							  dither->map_lower);

	up(&scale->sem);
	return 0;
}

/**
 * @brief   Scale do scaling function
 * @param   ctx [in] scale parameter
 * @return  success: 0,  fail: errcode
 * @see
 */
static int scale_trigger(scale_content_t *ctx)
{
	int ret = 0;

	if (down_interruptible(&scale->sem) != 0) {
		return -ERESTARTSYS;
	}

	/* start scaling */
	ret = gp_line_buffer_request(LINE_BUFFER_MODULE_SCALER, ctx->scale_rgn.width);
	if (ret != 0) {
		DIAG_ERROR("Scalar request line buffer fail: %d\n", ret);
		goto out;
	}

	/* clean dcache */
	gp_sync_cache();
	scale->done = false;
	ret = gpHalScaleExec(&ctx->src_img, &ctx->clip_rgn,
						 &ctx->dst_img, &ctx->scale_rgn);
	if (ret != SP_OK) {
		gp_line_buffer_release(LINE_BUFFER_MODULE_SCALER);
		DIAG_ERROR("gpHalScaleExec fail: %d\n", ret);
		ret = -ret;
		goto out;
	}

	if (ctx->timeout == 0) {
		ctx->timeout = SCALE_DEFAULT_TIMEOUT;
	}
#if POLLING_TEST
	DIAG_VERB("Waiting for Scaling Done\n");
	if (HAL_BUSY_WAITING(gpHalScaleDone(), ctx->timeout) >= 0) {
		gpHalScaleClearDone();
		DIAG_VERB("Scaling Done\n");
	}
	else {
		DIAG_ERROR("---------------------------------> Scaler1 Timeout (polling %dms) !!!!!!!!!!!!\n", ctx->timeout);
#if DEBUG_DUMP
		scale_param_dump(ctx);
		gpHalScaleRegDump();
#endif
	}
	gp_line_buffer_release(LINE_BUFFER_MODULE_SCALER);
#else
	if (ctx->timeout != 0xFFFFFFFF) {
		/* waiting for done */
		if (wait_event_timeout(scale->done_wait, scale->done, (ctx->timeout * HZ) / 1000) == 0) {
			ret = -ETIMEDOUT;
			gp_line_buffer_release(LINE_BUFFER_MODULE_SCALER);

			DIAG_ERROR("---------------------------------> Scaler1 Timeout (IRQ %dms) !!!!!!!!!!!!\n", ctx->timeout);
#if DEBUG_DUMP
			scale_param_dump(ctx);
			gpHalScaleRegDump();
#endif
		}
		else {
			/* invalidate dcache */
			gp_sync_cache();
		}
	}
#endif
out:
	up(&scale->sem);
	return ret;
}

/**
 * @brief   Scale irq handler
 */
static irqreturn_t scale_irq_handler(int irq, void *dev_id)
{
	if (gpHalScaleDone()) {
		DIAG_VERB("Scaling Done\n");
		gpHalScaleClearDone();
		scale->done = true;
		wake_up(&scale->done_wait);
		gp_line_buffer_release(LINE_BUFFER_MODULE_SCALER);
	}

	return IRQ_HANDLED;
}

/**
 * @brief   Scale device open function
 */
static int scale_open(struct inode *ip, struct file *fp)
{
	if (down_interruptible(&scale->sem) != 0) {
		return -ERESTARTSYS;
	}

	if (scale->open_count == 0) {
		gpHalScaleClkEnable(1);
#if !POLLING_TEST
		gpHalScaleEnableIrq(1);
#endif
	}
	scale->open_count++;

	up(&scale->sem);
	return 0;
}

/**
 * @brief   Scale device release function
 */
static int scale_release(struct inode *ip, struct file* fp)
{
	if (down_interruptible(&scale->sem) != 0) {
		return -ERESTARTSYS;
	}

	scale->open_count--;
	if (scale->open_count == 0) {
#if !POLLING_TEST
		gpHalScaleEnableIrq(0);
#endif
		gpHalScaleClkEnable(0);
	}

	up(&scale->sem);
	return 0;
}

/**
 * @brief   Scale device ioctl function
 */
static long scale_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	switch (cmd) {
	case SCALE_IOCTL_TRIGGER:
		{
			scale_content_t ctx;

			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}

			/* translate address from user_va to pa */
			ctx.src_img.pData  = (void *)gp_user_va_to_pa(ctx.src_img.pData);
			if (ctx.src_img.pDataU != NULL) {
				ctx.src_img.pDataU = (void *)gp_user_va_to_pa(ctx.src_img.pDataU);
				ctx.src_img.pDataV = (void *)gp_user_va_to_pa(ctx.src_img.pDataV);
			}
			ctx.dst_img.pData  = (void *)gp_user_va_to_pa(ctx.dst_img.pData);
			DIAG_VERB("[SCALE_IOCTL_TRIGGER] src:%08X  dst:%08X\n", ctx.src_img.pData, ctx.dst_img.pData);

			ret = scale_trigger(&ctx);
		}
		break;

	case SCALE_IOCTL_DITHER:
		{
			scale_dither_t dither;

			if (copy_from_user(&dither, (void __user*)arg, sizeof(dither))) {
				ret = -EFAULT;
				break;
			}

			/* set dither param */
			ret = scale_set_dither(&dither);
			if (ret != SP_OK) {
				ret = -EINVAL;
			}
		}
		break;

	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	return ret;
}

static const struct file_operations scale_fops = {
	.owner          = THIS_MODULE,
	.open           = scale_open,
	.release        = scale_release,
	.unlocked_ioctl = scale_ioctl,
};

/**
 * @brief   Scale driver init function
 */
static int __init scale_init(void)
{
	int ret = -ENXIO;

	scale = (scale_info_t *)kzalloc(sizeof(scale_info_t),  GFP_KERNEL);
	if (scale == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("scale kmalloc fail\n");
		goto fail_kmalloc;
	}

	ret = request_irq(IRQ_SCALE_ENGINE,
					  scale_irq_handler,
					  IRQF_DISABLED,
					  "SCALE_IRQ",
					  scale);
	if (ret < 0) {
		DIAG_ERROR("scale request irq fail\n");
		goto fail_request_irq;
	}

	/* initialize */
	init_MUTEX(&scale->sem);
	init_waitqueue_head(&scale->done_wait);

	scale->dev.name  = "scalar";
	scale->dev.minor = MISC_DYNAMIC_MINOR;
	scale->dev.fops  = &scale_fops;

	/* register device */
	ret = misc_register(&scale->dev);
	if (ret != 0) {
		DIAG_ERROR("scalar device register fail\n");
		goto fail_device_register;
	}

	return 0;

	/* error rollback */
fail_device_register:
	free_irq(IRQ_SCALE_ENGINE, scale);
fail_request_irq:
	kfree(scale);
	scale = NULL;
fail_kmalloc:
	return ret;
}

/**
 * @brief   Scale driver exit function
 */
static void __exit scale_exit(void)
{
	misc_deregister(&scale->dev);
	free_irq(IRQ_SCALE_ENGINE, scale);
	kfree(scale);
	scale = NULL;
}

module_init(scale_init);
module_exit(scale_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus Scaling Engine Driver");
MODULE_LICENSE_GP;
