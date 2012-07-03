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
 * @file    gp_sd.c
 * @brief   Implement of SD card module driver.
 * @author  Dunker Chen
 */
 
#include <mach/io.h>
#include <mach/module.h> 
#include <mach/kernel.h>
#include <mach/bdev.h>
#include <mach/gp_sd.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_sd.h>
#include <mach/gp_apbdma0.h>
 
/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

#define SD_NUM 						2
#define SD_MINORS					16

#define SD_MAX_SECTORS				255
#define SD_MAX_PHY_SEGMENTS			128
#define SD_MAX_HW_SEGMENTS			128
#define SD_MAX_PHY_SEGMENTS_SIZE	0x10000

#define SD_DEBOUNCE					50			/* unit: 10 ms */
#define SD_CD_POLL					HZ/8		/* SD card detect polling duration time */

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#if 0
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

#define DERROR printk 

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern int gp_sdio_init(void);
extern void gp_sdio_exit(void);
extern void gp_sdio_remove_device(void);
extern void gp_sdio_insert_device(UINT32 device_id, UINT32 ocr);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
 
static int gp_sdcard_open(struct block_device *dev, fmode_t mode);
static int gp_sdcard_release(struct gendisk *blk, fmode_t mode);
static int gp_sdcard_ioctrl (struct block_device *dev, fmode_t mode, unsigned cmd, unsigned long arg);
static int gp_sdcard_getgeo (struct block_device *dev, struct hd_geometry *hd);

static void gp_sdcard_request(struct request_queue *q);

extern int gp_sdcard_cardinit (gpSDInfo_t* sd);
extern int gp_sdcard_setbus(gpSDInfo_t* sd, unsigned int mode);
extern int gp_sdcard_read_scatter(gpSDInfo_t * sd, unsigned int sector, struct scatterlist *sg, unsigned int ln);
extern int gp_sdcard_write_scatter(gpSDInfo_t * sd, unsigned int sector, struct scatterlist *sg, unsigned int ln);

/**************************************************************************
*                         G L O B A L    D A T A                         *
**************************************************************************/
 
static int sd_major = 0;
module_param(sd_major, int, 0);

gpSDInfo_t *sd_info = NULL;

/* ----- SD Class OPS, for SD Driver internal reference ----- */
static const struct block_device_operations gp_sdcard_ops = {
	.open 		= gp_sdcard_open,
	.release 	= gp_sdcard_release,
	.ioctl		= gp_sdcard_ioctrl,
	.getgeo 	= gp_sdcard_getgeo,
	.owner 		= THIS_MODULE,
};

/**************************************************************************
*             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/
 
/**
* @brief 	Check card insert function.
* @param 	sd[in]: Card information.
* @return 	0 for remove, 1 for insert.
*/ 
static int gp_sdcard_ckinsert(gpSDInfo_t* sd)
{
	if(sd->sd_func==NULL)
	{
		DEBUG("SD%d: no detect function\n", sd->device_id);
		return 0;	
	}
	return (sd->sd_func->detect()==1)?1:0;
}

/**
* @brief 	Scatter list data  transfer function.
* @param 	sd[in]: Card information.
* @param 	sector[in]: Start sector.
* @param 	sg[in]: Scatter list pointer.
* @param 	ln[in]: List number.
* @return 	Actual sectors by reading/ERROR_ID(<0).
*/ 
static int gp_sdcard_transfer_scatter(gpSDInfo_t *sd, unsigned long sector, struct scatterlist *sg, unsigned int ln, int write)
{
	int ret;
	int pin_handle;
	/* ----- Check card status ----- */
	if((gp_sdcard_ckinsert(sd)==0)||(sd->handle_dma==0))
		return -ENXIO;
	/* ----- Get pin handle ----- */
	pin_handle = gp_board_pin_func_request((sd->device_id==0)?GP_PIN_SD0:GP_PIN_SD1, GP_BOARD_WAIT_FOREVER);
	if(pin_handle<0)
	{
		DERROR("SD%d: can't get pin handle\n", sd->device_id);
		return -EIO;
	}
	if(write==0)
	{
		ret = gp_sdcard_read_scatter(sd, sector, sg, ln);
	}
	else
	{
		/* ----- Check write protect ----- */
		if(sd->sd_func->is_write_protected()==1)
			ret = -EROFS;
		else
			ret = gp_sdcard_write_scatter(sd, sector, sg, ln);
	}	
	gp_board_pin_func_release(pin_handle);
	return ret;
}

/**
* @brief 	Request service function.
* @param 	sd[in]: Card information.
* @param 	req[in]: Start sector.
* @return 	SUCCESS/ERROR_ID.
*/ 
static int gp_sdcard_xfer_request(gpSDInfo_t *sd, struct request *req)
{
	int ret = 1;

	while (ret) 
	{
		unsigned int ln;
		
		ln = blk_rq_map_sg(sd->queue, req, sd->sg);
		
		ret = gp_sdcard_transfer_scatter(sd, blk_rq_pos(req), sd->sg, ln, rq_data_dir(req));
		if(ret<0)
			goto out_error;
		/* ----- End of request ----- */
		spin_lock_irq(&sd->lock);
		ret = __blk_end_request(req, 0, ret<<9);
		spin_unlock_irq(&sd->lock);
	}
	return 1;
out_error:
	spin_lock_irq(&sd->lock);
	__blk_end_request_all(req, ret);;
	spin_unlock_irq(&sd->lock);
	return 0;
}

/**
* @brief 	Request thread function.
* @param 	d[in]: Private data.
* @return 	SUCCESS/ERROR_ID.
*/ 
static int gp_sdcard_queue_thread(void *d)
{
	gpSDInfo_t *sd = d;
	struct request_queue *q = sd->queue;
	
	current->flags |= PF_MEMALLOC;
	
	down(&sd->thread_sem);
	do 
	{
		struct request *req = NULL;
		
		spin_lock_irq(q->queue_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!blk_queue_plugged(q))
			req = blk_fetch_request(q);
		sd->req = req;
		spin_unlock_irq(q->queue_lock);
		
		if (!req)
		{
			if (kthread_should_stop())
			{
				set_current_state(TASK_RUNNING);
				break;
			}
			up(&sd->thread_sem);
			schedule();
			down(&sd->thread_sem);
			continue;
		}
		set_current_state(TASK_RUNNING);
		if((gp_sdcard_ckinsert(sd)==0))
		{
			__blk_end_request_all(req, -ENXIO);
			continue;			
		}
		gp_sdcard_xfer_request(sd, req);
	} while (1);
	up(&sd->thread_sem);
	return 0;
}

/**
* @brief 	Card initial function.
* @param 	work[in]: Work structure.
* @return 	None.
*/ 
static void gp_sdcard_work_init(struct work_struct *work)
{
	gpSDInfo_t* sd = container_of(work, gpSDInfo_t,init);
	int pin_handle;
	pin_handle = gp_board_pin_func_request((sd->device_id==0)?GP_PIN_SD0:GP_PIN_SD1, GP_BOARD_WAIT_FOREVER);
	if(pin_handle<0)
	{
		DERROR("SD%d: can't get pin handle\n", sd->device_id);
		goto init_work_end;
	}
	/* ----- Initial SD module (controller) ----- */
	gpHalSDInit(sd->device_id);
	/* ----- Initial SD card ----- */
	gp_sdcard_cardinit(sd);
	gp_board_pin_func_release(pin_handle);	
	if(sd->present==1)
	{
		if(sd->card_type == SDIO)
		{
			sd->pin_handle = gp_board_pin_func_request((sd->device_id==0)?GP_PIN_SD0:GP_PIN_SD1, GP_BOARD_WAIT_FOREVER);
			if(sd->pin_handle<0)
			{
				DERROR("SD%d: can't get pin handle\n", sd->device_id);
				goto init_work_end;
			}
			DEBUG("SDIO card detected\n");
			gp_sdio_insert_device(sd->device_id, sd->RCA);	
		}
		else
		{
			sd->queue = blk_init_queue(gp_sdcard_request, &sd->lock);
			if(sd->queue==NULL)
			{
				DERROR("NO MEMORY: queue\n");
				goto init_work_end;
			} 	 
			blk_queue_ordered(sd->queue, QUEUE_ORDERED_DRAIN, NULL);
			queue_flag_set_unlocked(QUEUE_FLAG_NONROT, sd->queue);
			blk_queue_logical_block_size(sd->queue, 512);
			blk_queue_max_sectors(sd->queue, SD_MAX_SECTORS );
			blk_queue_max_phys_segments(sd->queue, SD_MAX_PHY_SEGMENTS);
			blk_queue_max_hw_segments(sd->queue, SD_MAX_HW_SEGMENTS);
			blk_queue_max_segment_size(sd->queue, SD_MAX_PHY_SEGMENTS_SIZE);
			/* ----- Initial scatter list ----- */
			sd->sg = kmalloc(sizeof(struct scatterlist) *SD_MAX_PHY_SEGMENTS, GFP_KERNEL);
			if (!sd->sg) 
			{
				DERROR("NO MEMORY: queue\n");
				goto fail_thread;
			}
			sg_init_table(sd->sg, SD_MAX_PHY_SEGMENTS);
		
			init_MUTEX(&sd->thread_sem);
			/* ----- Enable thread ----- */
			sd->thread = kthread_run(gp_sdcard_queue_thread, sd, "sd-qd");
			if (IS_ERR(sd->thread)) 
			{
				goto fail_thread;
			}
			sd->queue->queuedata = sd;
			/* ----- Setup gendisk structure ----- */
			sd->gd = alloc_disk(SD_MINORS);
			if (sd->gd==NULL) 
			{
				DERROR("NO MEMORY: gendisk\n");
				blk_cleanup_queue(sd->queue);	
				goto fail_gd;	
			}
			/* ----- Set gendisk structure ----- */
			sd->gd->major = sd_major;
			sd->gd->first_minor = sd->device_id*SD_MINORS;
			sd->gd->fops = &gp_sdcard_ops;
			sd->gd->queue = sd->queue;
			sd->gd->private_data = sd;
			snprintf (sd->gd->disk_name, 32, "sdcard%c", sd->device_id + 'a');
			set_capacity(sd->gd,sd->capacity);
			add_disk(sd->gd);
		}
		goto init_work_end;
	}
	else
	{
		DERROR("Initial fail\n");
		goto init_work_end;
	}
fail_gd:
	/* ----- Then terminate our worker thread ----- */
	kthread_stop(sd->thread);
fail_thread:
	if (sd->sg)
		kfree(sd->sg);
	sd->sg = NULL;
	blk_cleanup_queue (sd->queue);	
init_work_end:	
	sd->timer.expires = jiffies + SD_CD_POLL;
	add_timer(&sd->timer);
}

/**
* @brief 	Card un-initial function.
* @param 	work[in]: Work structure.
* @return 	None.
*/ 
static void gp_sdcard_work_uninit(struct work_struct *work)
{
	gpSDInfo_t* sd = container_of(work, gpSDInfo_t,uninit);
	
	if(sd->card_type == SDIO)
	{
		gp_sdio_remove_device();
		gp_board_pin_func_release(sd->pin_handle);
	}
	else
	{
		/* ----- Stop new requests from getting into the queue ----- */
		del_gendisk(sd->gd);
		/* ----- Then terminate our worker thread ----- */
		kthread_stop(sd->thread);
		if (sd->sg)
			kfree(sd->sg);
		sd->sg = NULL;
		blk_cleanup_queue (sd->queue);
		put_disk(sd->gd);
		/* ----- free dma channel ----- */
		if(sd->handle_dma)
			gp_apbdma0_release(sd->handle_dma);
		sd->handle_dma = 0;
	}
	/* ----- Uninitial SD controller ----- */
	gpHalSDUninit(sd->device_id);
	sd->users = 0;
	sd->timer.expires = jiffies + SD_CD_POLL;
	add_timer(&sd->timer);
}

/**
* @brief 	Timer function for check card insert.
* @param 	arg[in]: Private data.
* @return 	None.
*/ 
static void gp_sdcard_timer_fn(unsigned long arg)
{
	gpSDInfo_t* sd = (gpSDInfo_t*)arg;
	int insert = gp_sdcard_ckinsert(sd);
	/* ----- Card status change ----- */
	if( sd->present ^ insert)
	{
		if(insert==0)
		{
			sd->present = 0;	
			sd->cnt = 0;
			sd->users = 0;
			DEBUG("Card Remove\n");
			sd->sd_func->set_power(0);
			schedule_work(&sd->uninit);		
		}
		else
		{
			sd->cnt ++;
			if(sd->cnt>=SD_DEBOUNCE)
			{
				sd->present = 0;	
				sd->cnt = 0;
				sd->users = 0;			
				DEBUG("Card insert\n");
				sd->sd_func->set_power(1);
				schedule_work(&sd->init);
			}
			else
			{
				sd->timer.expires = jiffies + HZ/100;		/* 10ms */	
				add_timer(&sd->timer);
			}
		}
	}
	/* ----- Card status not change ----- */
	else
	{
		sd->timer.expires = jiffies + SD_CD_POLL;
		add_timer(&sd->timer);	
		sd->cnt = 0;
	}	
}

/**
* @brief 	Request queue function.
* @param 	request_queue[in]: Request.
* @return 	SUCCESS/ERROR_ID
*/ 
static void gp_sdcard_request(struct request_queue *q)
{
	
	gpSDInfo_t *sd = (gpSDInfo_t*)q->queuedata;
	struct request *req;

	if (!sd) 
	{
		while ((req = blk_fetch_request(q)) != NULL) 
		{
			req->cmd_flags |= REQ_QUIET;
			__blk_end_request_all(req, -EIO);
		}
		return;
	}

	if (!sd->req)
		wake_up_process(sd->thread);
}

/**
* @brief 	SD card open.
* @param 	dev[in]: Block device pointer.
* @param 	mode[in]: Mode.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_sdcard_open(struct block_device *dev, fmode_t mode)
{
	gpSDInfo_t *sd = dev->bd_disk->private_data;
	/* ----- Check write protect ----- */
	if((mode&FMODE_WRITE)&&(sd->sd_func->is_write_protected()==1))
	{
		DEBUG("Read only card\n");
		return -EROFS;
	}
	if(sd->users==0)
	{
		/* ----- Wait 1 second ----- */
		sd->handle_dma = gp_apbdma0_request(1000);
		if(sd->handle_dma==0)
			return -EBUSY;
	}
	sd->users++;
	return 0;	
}

/**
* @brief 	SD card release.
* @param 	dev[in]: Block device pointer.
* @param 	mode[in]: Mode.
* @return 	SUCCESS/ERROR_ID.
*/
static int gp_sdcard_release(struct gendisk *gd, fmode_t mode)
{
	gpSDInfo_t *sd = gd->private_data;
	sd->users--;
	if(sd->users==0)
	{
		gp_apbdma0_release(sd->handle_dma);
		sd->handle_dma = 0;
	}
	return 0;
}

/**
* @brief 	SD card io control function.
* @param 	dev[in]: Block device pointer.
* @param 	mode[in] : Mode.
* @param	cmd[in]: Command.
* @param	arg[in]: Argument.
* @return 	SUCCESS/ERROR_ID.
*/
static int gp_sdcard_ioctrl (struct block_device *dev, fmode_t mode, unsigned cmd, unsigned long arg)
{
	gpSDInfo_t *sd = dev->bd_disk->private_data;
	//unsigned short sys_apb = 810;
	int ret = 0;
	
	switch(cmd)
	{
		case SD_CLK_SET:
		{
			unsigned long sys_apb_clk = 0;
			struct clk *pclk;
			pclk = clk_get(NULL,"clk_sys_apb");
			if(pclk)
			{
				sys_apb_clk = clk_get_rate(pclk)/100000;	
				clk_put(pclk);
			}
			else
			{
				sys_apb_clk = 270;
			}
			ret = gpHalSDSetClk(sd->device_id, sys_apb_clk, (sd->speed>arg)?arg:sd->speed);
			break;
		}
		case SD_CLK_BUS:
			ret = gp_sdcard_setbus(sd,arg);
			break;
		default:
			ret = -ENOIOCTLCMD;
			break;
	}
	return ret;	
}

/**
* @brief 	SD card release.
* @param 	dev[in]: Block device pointer.
* @param 	hd[in]: Hard disk geometry information.
* @return 	SUCCESS/ERROR_ID.
*/
static int gp_sdcard_getgeo (struct block_device *dev, struct hd_geometry *hd)
{
	unsigned int size;
	gpSDInfo_t *sd = dev->bd_disk->private_data;

	/* ----- We have no real geometry, of course, so make something up. ----- */
	size = sd->capacity;
	hd->cylinders = (size & ~0x3f) >> 6;
	hd->heads = 4;
	hd->sectors = 16;
	hd->start = 0;
	return 0;	
}

/**
* @brief 	SD driver exit function.
* @return 	None.
*/
static void __exit gp_sdcard_exit(void)
{
	int i;
	/* ----- Free all alloc memory ----- */
	for (i = 0; i < SD_NUM; i++) 
	{
		gpSDInfo_t *sd = sd_info + i;
		/* ----- Free gendisk structure ----- */
		if (sd->gd) 
		{
			del_gendisk(sd->gd);
			/* ----- Free request ----- */
			if (sd->queue) 
			{
				blk_cleanup_queue(sd->queue);
			}
			/* ----- Then terminate our worker thread -----*/
			kthread_stop(sd->thread);
			if (sd->sg)
				kfree(sd->sg);
			sd->sg = NULL;
			put_disk(sd->gd);
		}
		/* ----- free dma channel ----- */
		if(sd->handle_dma)
			gp_apbdma0_release(sd->handle_dma);
	}
	unregister_blkdev(sd_major, "sdcard");
	gp_sdio_exit();
	kfree(sd_info);
}

/**
* @brief 	SD driver initial function.
* @return 	SUCCESS/ERROR_ID.
*/
static int __init gp_sdcard_init(void)
{
	int i;
	/* ----- Get registered. ----- */
	sd_major = register_blkdev(sd_major, "sdcard");
	if (sd_major <= 0) 
	{
		DERROR(KERN_WARNING "SD card : unable to get major number\n");
		return -EBUSY;
	}
	/* ----- Allocate the device array, and initialize each one. ----- */
	sd_info = kmalloc(SD_NUM*sizeof (gpSDInfo_t), GFP_KERNEL);
	if (sd_info == NULL)
		goto out_unregister;
	/* ----- Initial SD information ----- */
	for(i=0; i<SD_NUM; i++)
	{
		gpSDInfo_t *sd = &sd_info[i];
		/* ----- Clear memory ----- */
		memset (sd, 0, sizeof (gpSDInfo_t));
		sd->device_id = i;
		/* ----- Initial spinlock ----- */
		spin_lock_init(&sd->lock);
		spin_lock_init(&sd->hal_lock);
		/* ----- Initial work queue ----- */
		INIT_WORK(&sd->init, gp_sdcard_work_init);
		INIT_WORK(&sd->uninit, gp_sdcard_work_uninit);
		if(i==0)
		{
			/* ----- request IO function ----- */
			sd->sd_func = gp_board_get_config("sd0",gp_board_sd_t);
			/* ----- Initial dma module ----- */
			sd->dma_param.module = SD0;
		}
		else
		{
			/* ----- request IO function ----- */
			sd->sd_func = gp_board_get_config("sd1",gp_board_sd_t);
			/* ----- Initial dma module ----- */
			sd->dma_param.module = SD1;
		}
		/* ----- Init timer ----- */
		init_timer(&sd->timer);
		sd->timer.data = (unsigned long) sd;
		sd->timer.function = gp_sdcard_timer_fn;
		sd->timer.expires = jiffies + SD_CD_POLL;
		add_timer(&sd->timer);
	}
	gp_sdio_init();
	return 0;
out_unregister:
	unregister_blkdev(sd_major, "sdcard");
	return -ENOMEM;
}
	
module_init(gp_sdcard_init);
module_exit(gp_sdcard_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus SD Driver");
MODULE_LICENSE_GP;
