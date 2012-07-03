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
 * @file    gp_rotate.c
 * @brief   Implement of rotate driver.
 * @author  qinjian
 * @since   2010/10/15
 * @date    2010/10/15
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/gp_rotate.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_line_buffer.h>
#include <mach/hal/hal_rotate.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Rotate timeout (ms) */
#define ROTATE_TIMEOUT  3000

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct rotate_info_s {
	struct miscdevice dev;          /*!< @brief rotate device */
	struct semaphore sem;           /*!< @brief mutex semaphore for rotate ops */
	wait_queue_head_t done_wait;    /*!< @brief rotating done wait queue */
	bool done;                      /*!< @brief rotating done flag */
	unsigned int open_count;        /*!< @brief rotate device open count */
} rotate_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static rotate_info_t *rotate = NULL;

/**
 * @brief   Rotate do rotating function
 * @param   ctx [in] rotate parameter
 * @return  success: 0,  fail: errcode
 * @see
 */
static int rotate_trigger(rotate_content_t *ctx)
{
	int ret = 0;

	if (down_interruptible(&rotate->sem) != 0) {
		return -ERESTARTSYS;
	}

	/* start rotating */
	gp_line_buffer_request(LINE_BUFFER_MODULE_ROTATOR, 512);
	rotate->done = false;
	gp_sync_cache();
	ret = gpHalRotateExec(&ctx->src_img, &ctx->clip_rgn,
						 &ctx->dst_img, &ctx->dst_pos, ctx->opt);
	if (ret == SP_OK) {
		/* waiting for done */
		if (wait_event_timeout(rotate->done_wait, rotate->done, ROTATE_TIMEOUT * HZ / 1000) == 0) {
			ret = -ETIMEDOUT;
		}
		else {
			gp_sync_cache();
		}
	}
	gp_line_buffer_release(LINE_BUFFER_MODULE_ROTATOR);

	up(&rotate->sem);
	return ret;
}

/**
 * @brief   Rotate irq handler
 */
static irqreturn_t rotate_irq_handler(int irq, void *dev_id)
{
	if (gpHalRotateDone()) {
		gpHalRotateClearDone();
		rotate->done = true;
		wake_up(&rotate->done_wait);
	}

	return IRQ_HANDLED;
}

/**
 * @brief   Rotate device open function
 */
static int rotate_open(struct inode *ip, struct file *fp)
{
	if (down_interruptible(&rotate->sem) != 0) {
		return -ERESTARTSYS;
	}

	if (rotate->open_count == 0) {
		gpHalRotateClkEnable(1);
		gpHalRotateEnableIrq(1);
	}
	rotate->open_count++;

	up(&rotate->sem);
	return 0;
}

/**
 * @brief   Rotate device release function
 */
static int rotate_release(struct inode *ip, struct file* fp)
{
	if (down_interruptible(&rotate->sem) != 0) {
		return -ERESTARTSYS;
	}

	rotate->open_count--;
	if (rotate->open_count == 0) {
		gpHalRotateEnableIrq(0);
		gpHalRotateClkEnable(0);
	}

	up(&rotate->sem);
	return 0;
}

/**
 * @brief   Rotate device ioctl function
 */
static long rotate_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	switch (cmd) {
	case ROTATE_IOCTL_TRIGGER:
		{
			rotate_content_t ctx;

			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}

			/* translate address from user_va to pa */
			ctx.src_img.pData  = (void *)gp_user_va_to_pa(ctx.src_img.pData);
			ctx.dst_img.pData  = (void *)gp_user_va_to_pa(ctx.dst_img.pData);

			ret = rotate_trigger(&ctx);
		}
		break;

	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	return ret;
}

static const struct file_operations rotate_fops = {
	.owner          = THIS_MODULE,
	.open           = rotate_open,
	.release        = rotate_release,
	.unlocked_ioctl = rotate_ioctl,
};

/**
 * @brief   Rotate driver init function
 */
static int __init rotate_init(void)
{
	int ret = -ENXIO;

	rotate = (rotate_info_t *)kzalloc(sizeof(rotate_info_t),  GFP_KERNEL);
	if (rotate == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("rotate kmalloc fail\n");
		goto fail_kmalloc;
	}

	ret = request_irq(IRQ_ROTATE_ENGINE,
					  rotate_irq_handler,
					  IRQF_DISABLED,
					  "ROTATE_IRQ",
					  rotate);
	if (ret < 0) {
		DIAG_ERROR("rotate request irq fail\n");
		goto fail_request_irq;
	}

	/* initialize */
	init_MUTEX(&rotate->sem);
	init_waitqueue_head(&rotate->done_wait);

	/* register device */
	rotate->dev.name  = "rotator";
	rotate->dev.minor = MISC_DYNAMIC_MINOR;
	rotate->dev.fops  = &rotate_fops;
	ret = misc_register(&rotate->dev);
	if (ret != 0) {
		DIAG_ERROR("rotator device register fail\n");
		goto fail_device_register;
	}

	return 0;

	/* error rollback */
fail_device_register:
	free_irq(IRQ_ROTATE_ENGINE, rotate);
fail_request_irq:
	kfree(rotate);
	rotate = NULL;
fail_kmalloc:
	return ret;
}

/**
 * @brief   Rotate driver exit function
 */
static void __exit rotate_exit(void)
{
	misc_deregister(&rotate->dev);
	free_irq(IRQ_ROTATE_ENGINE, rotate);
	kfree(rotate);
	rotate = NULL;
}

module_init(rotate_init);
module_exit(rotate_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus Rotation Engine Driver");
MODULE_LICENSE_GP;
