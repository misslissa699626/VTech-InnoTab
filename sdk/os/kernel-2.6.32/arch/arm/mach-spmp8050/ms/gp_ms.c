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
#include <linux/module.h> 
#include <linux/hdreg.h> 		/* HDIO_GETGEO */
#include <linux/blkdev.h>
#include <mach/gp_gpio.h>
#include <linux/kernel.h>
#include <mach/hal/hal_ms.h>
#include <mach/ms/gp_ms.h>
#include <mach/ms/msproal.h>
#include <mach/ms/msproal_defs.h>
#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_user.h>
#include <mach/ms/msproal_msif.h>
#include <mach/gp_apbdma0.h>
#include <linux/kthread.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <mach/diag.h>
#include <mach/module.h>

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int gp_mscard_ioctrl (struct block_device * device, fmode_t mode, unsigned int cmd, unsigned long arg);
static int gp_mscard_open(struct block_device * device, fmode_t mode);
static int gp_mscard_release(struct gendisk *disk, fmode_t mode);

/**************************************************************************
*                         G L O B A L    D A T A                         *
**************************************************************************/
static int ms_major = 0;
module_param(ms_major, int, 0);
static struct ms_dev *Devices = NULL;
static unsigned char *WorkArea = NULL;

static struct block_device_operations gp_mscard_ops = {
	.owner          = THIS_MODULE,
	.open 			= gp_mscard_open,
	.release 		= gp_mscard_release,
	.ioctl	        = gp_mscard_ioctrl
};

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

#define MS_MINORS    1
#define DRIVER_NAME "gp-ms"
#define MS_CDPIN   (0<<24)|(2<<16)|(1<<8)|12 /* pin B_CMOS2 */
#define MS_DEBOUNCE				10

#define MS_MAX_SECTORS				255
#define MS_MAX_PHY_SEGMENTS			128
#define MS_MAX_HW_SEGMENTS			128
#define MS_MAX_PHY_SEGMENTS_SIZE	0x10000

/**
 * @brief 		MS card open. 
 * @param 	dev[in]: Block device pointer.
* @param 	mode[in]: Mode.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_mscard_open(struct block_device * device, fmode_t mode)
{
	struct ms_dev *msdev = device->bd_disk->private_data;

	if(msdev->users==0) {
		/* ----- Wait 1 second ----- */
		msdev->handle_dma = gp_apbdma0_request(100);
		if(msdev->handle_dma==0) {
			DIAG_ERROR("request dma fail\n");
			return -EBUSY;
		}
	}

	msdev->users++;

	return 0;	
	
}

/**
 * @brief 		MS card release.
 * @param 	dev[in]: Block device pointer.
* @param 	mode[in]: Mode.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_mscard_release(struct gendisk *disk, fmode_t mode)
{
	struct ms_dev *msdev = disk->private_data;

	msdev->users--;
	if(msdev->users==0) {
		gp_apbdma0_release(msdev->handle_dma);
		msdev->handle_dma = 0;
	}
	
	return 0;	
}

/**
* @brief 	Check card insert function.
* @param 	msdev[in]: Card information.
* @return 	0 for remove, 1 for insert.
*/ 
static int gp_mscard_ckinsert(struct ms_dev *msdev)
{
	unsigned int ret; 
	
	if(msdev->cd_pin) {
		gp_gpio_get_value(msdev->cd_pin, &ret);
		return (ret)?0:1;	
	}	
	return 0;
}

/**
* @brief 	Timer function for check card insert.
* @param 	arg[in]: Private data.
* @return 	None.
*/ 
static void gp_mscard_timer_fn(unsigned long arg)
{
	struct ms_dev *msdev = (struct ms_dev *)arg;
	int insert = gp_mscard_ckinsert(msdev);
	
	if( msdev->present ^ insert) {
		msdev->timer.expires = jiffies + 100/HZ;		/* 10ms */	
		msdev->db_cnt++;
		add_timer(&msdev->timer);
		/* ----- Debounce ----- */
		if(msdev->db_cnt >= MS_DEBOUNCE) {
			del_timer_sync(&msdev->timer);
			msdev->present = insert;	
			msdev->db_cnt = 0;
			msdev->users = 0;
			if(insert) {
				DIAG_INFO("Crad insert\n");
				schedule_work(&msdev->init);
			}
			else {
				schedule_work(&msdev->uninit);
				DIAG_INFO("Crad Remove\n");
			}
		}	
	}
	/* ----- Card status not change ----- */
	else {
		msdev->timer.expires = jiffies + 2/HZ;			/* 500ms */
		add_timer(&msdev->timer);	
		msdev->db_cnt = 0;
	}	
}

/**
* @brief 	MS card read function with scatter list.
* @param 	ms[in]: Card information.
* @param 	sector[in]: Start sector.
* @param 	sg[in]: Scatter list pointer.
* @param 	ln[in]: List number.
* @return 	Actual sectors by reading/ERROR_ID(<0).
*/
static int gp_mscard_read_scatter(struct ms_dev *msdev, unsigned int sector, struct scatterlist *sg, unsigned int ln)
{
	int i;
	struct scatterlist *sg_ev;
	int ret = 0;
	int sector_num = 0;
	unsigned sgln = 0;
	
	MSIFHNDL *msifhd = (MSIFHNDL *) WorkArea;
	
	if (msifhd->XfrMode == MS_DMA_MODE) {
		msifhd->handle_dma = msdev->handle_dma;
		sgln = dma_map_sg(NULL, sg, ln, DMA_FROM_DEVICE);
		if(sgln!=ln) {
			dma_unmap_sg(NULL, sg, sgln, DMA_FROM_DEVICE);
			return -ENOMEM;
		}
	}
	
	for_each_sg(sg, sg_ev, ln, i) {
		unsigned int number = sg_dma_len(sg_ev)>>9;
		//DIAG_ERROR("read sector = %d, number = %d\n",sector,number);
		ret = msproal_read_sect(sector, number, (unsigned char*)sg_phys(sg_ev));
			
		if(ret!=0) {
			gp_apbdma0_stop(msdev->handle_dma);
			DIAG_ERROR("DMA read error: %d\n",ret);
			goto out_error;
		}
		sector_num += number;
	}	
out_error:

	if (msifhd->XfrMode == MS_DMA_MODE) {
		dma_unmap_sg(NULL, sg, sgln, DMA_FROM_DEVICE);
	}
	
	if(ret)
		return ret;
	else
		return sector_num;
}

/**
* @brief 	SD card write function with scatter list.
* @param 	sd[in]: Card information.
* @param 	sector[in]: Start sector.
* @param 	sg[in]: Scatter list pointer.
* @param 	ln[in]: List number.
* @return 	Actual sectors by reading/ERROR_ID(<0).
*/
static int gp_mscard_write_scatter(struct ms_dev *msdev, unsigned int sector, struct scatterlist *sg, unsigned int ln)
{
	int i;
	struct scatterlist *sg_ev;
	int ret = 0;
	int sector_num = 0;
	unsigned sgln = 0;
	
	MSIFHNDL *msifhd = (MSIFHNDL *) WorkArea;
	
	if (msifhd->XfrMode == MS_DMA_MODE) {
		msifhd->handle_dma = msdev->handle_dma;
		sgln = dma_map_sg(NULL, sg, ln, DMA_TO_DEVICE);
		if(sgln!=ln) {
			dma_unmap_sg(NULL, sg, sgln, DMA_TO_DEVICE);
			return -ENOMEM;
		}
	}
	
	for_each_sg(sg, sg_ev, ln, i) {
		unsigned int number = sg_dma_len(sg_ev)>>9;
		//DIAG_ERROR("write sector = %d, number = %d\n",sector,number);
		ret = msproal_write_sect(sector, number, (unsigned char*)sg_phys(sg_ev));
	
		if(ret!=0) {
			gp_apbdma0_stop(msdev->handle_dma);
			DIAG_ERROR("DMA write error: %d\n",ret);
			goto out_error;
		}
		sector_num += number;
	}
	
out_error:
	if (msifhd->XfrMode == MS_DMA_MODE) {
		dma_unmap_sg(NULL, sg, sgln, DMA_TO_DEVICE);
	}
	
	if(ret)
		return ret;
	else
		return sector_num;
}

/**
* @brief 	Scatter list data  transfer function.
* @param 	msdev[in]: Card information.
* @param 	sector[in]: Start sector.
* @param 	sg[in]: Scatter list pointer.
* @param 	ln[in]: List number.
* @return 	Actual sectors by reading/ERROR_ID(<0).
*/ 
static int gp_mscard_transfer_scatter(struct ms_dev *msdev, unsigned long sector, struct scatterlist *sg, unsigned int ln, int write)
{
	if(write==0) {
		return gp_mscard_read_scatter(msdev, sector, sg, ln);
	}
	else {
		return gp_mscard_write_scatter(msdev, sector, sg, ln);
	}	
}

/**
* @brief 	Request service function
* @param 	msdev[in]: Card information.
* @param 	req[in]: Start sector.
* @return 	SUCCESS/ERROR_ID.
*/ 
static int gp_mscard_xfer_request(struct ms_dev *msdev, struct request *req)
{
	int ret = 1;

	while (ret) {
		unsigned int sector = blk_rq_pos(req);
		unsigned int blks = blk_rq_cur_sectors(req);
		unsigned int dir = rq_data_dir(req);
		unsigned int ln;
		ln = blk_rq_map_sg(msdev->queue, req, msdev->sg);
		/* ----- Adjust the sg list so it is the same size as the request. ----- */
		if (blks != blk_rq_sectors(req)) {
			int i, data_size = blks << 9;
			struct scatterlist *sg;
			
			for_each_sg(msdev->sg, sg, ln, i) {
				data_size -= sg->length;
				if (data_size <= 0) {
					sg->length += data_size;
					i++;
					break;
				}
			}
			ln = i;
		}
		ret = gp_mscard_transfer_scatter(msdev, sector, msdev->sg, ln, dir);
		if(ret<0)
			goto out_error;
		/* ----- End of request ----- */
		spin_lock_irq(&msdev->lock);
		ret = __blk_end_request(req, 0, ret<<9);
		spin_unlock_irq(&msdev->lock);
	}
	return 1;
out_error:
	spin_lock_irq(&msdev->lock);
	while (ret)
		ret = __blk_end_request(req, -EIO, blk_rq_cur_bytes(req));
	spin_unlock_irq(&msdev->lock);
	return 0;
}

/**
* @brief 	Request queue function.
* @param 	request_queue[in]: Request.
* @return 	SUCCESS/ERROR_ID
*/ 
static void gp_mscard_request(struct request_queue *q)
{
	
	struct ms_dev *msdev = (struct ms_dev *)q->queuedata;
	struct request *req;

	if (!msdev) {
		while ((req = blk_fetch_request(q)) != NULL) {
			req->cmd_flags |= REQ_QUIET;
			__blk_end_request_all(req, -EIO);
		}
		return;
	}

	if (!msdev->req)
		wake_up_process(msdev->thread);
}

/**
* @brief 	Request thread function
* @param 	d[in]: Private data.
* @return 	SUCCESS/ERROR_ID.
*/ 
static int gp_mscard_queue_thread(void *d)
{
	struct ms_dev *msdev = d;
	struct request_queue *q = msdev->queue;
	
	current->flags |= PF_MEMALLOC;
	
	down(&msdev->thread_sem);
	do {
		struct request *req = NULL;
		
		spin_lock_irq(q->queue_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!blk_queue_plugged(q))
			req = blk_fetch_request(q);
		msdev->req = req;
		spin_unlock_irq(q->queue_lock);
		
		if (!req) {
			if (kthread_should_stop()) {
				set_current_state(TASK_RUNNING);
				break;
			}
			up(&msdev->thread_sem);
			schedule();
			down(&msdev->thread_sem);
			continue;
		}
		set_current_state(TASK_RUNNING);
		
		gp_mscard_xfer_request(msdev, req);
	} while (1);
	up(&msdev->thread_sem);
	return 0;
}

/**
* @brief 	Card initial function.
* @param 	work[in]: Work structure.
* @return 	None.
*/ 
static void gp_mscard_work_init(struct work_struct *work)
{
	struct ms_dev* msdev = container_of(work, struct ms_dev,init);
	MSIFHNDL *msifhd;
	unsigned int disk_capacity;
	int	 ret;
	
	if(msdev->present==1) {
		memset(WorkArea,0,sizeof (struct __msifhndl));
		
		msifhd = (MSIFHNDL *) WorkArea;
		
		msproal_init();
		msproal_user_change_clock(MSPROAL_SERIAL_MODE);
	
		ret = msproal_start(WorkArea);
		if (ret != 0) {
			DIAG_ERROR("msproal_start fail\n");
			goto init_work_end;
		}
    	
		msproal_set_xfr_mode(MS_CPU_MODE);
		
		ret = msproal_mount(MSPROAL_PRO_4PARALLEL_MODE);
    	if (ret != 0) {
			DIAG_ERROR("msproal_mount fail\n");
			goto init_work_end;
		}
		msproal_set_xfr_mode(MS_DMA_MODE);

		msdev->queue = blk_init_queue(gp_mscard_request, &msdev->lock);
		if(msdev->queue==NULL) {
			DIAG_ERROR("NO MEMORY: queue\n");
			goto init_work_end;
		} 	 
		blk_queue_ordered(msdev->queue, QUEUE_ORDERED_DRAIN, NULL);
		queue_flag_set_unlocked(QUEUE_FLAG_NONROT, msdev->queue);
		blk_queue_logical_block_size(msdev->queue, 512);
		blk_queue_max_sectors(msdev->queue, MS_MAX_SECTORS );
		blk_queue_max_phys_segments(msdev->queue, MS_MAX_PHY_SEGMENTS);
		blk_queue_max_hw_segments(msdev->queue, MS_MAX_HW_SEGMENTS);
		blk_queue_max_segment_size(msdev->queue, MS_MAX_PHY_SEGMENTS_SIZE);
		/* ----- Initial scatter list ----- */
		msdev->sg = kmalloc(sizeof(struct scatterlist) *MS_MAX_PHY_SEGMENTS, GFP_KERNEL);
		if (!msdev->sg) {
			DIAG_ERROR("NO MEMORY: queue\n");
			goto fail_thread;
		}
		sg_init_table(msdev->sg, MS_MAX_PHY_SEGMENTS);
	
		init_MUTEX(&msdev->thread_sem);
		/* ----- Enable thread ----- */
		msdev->thread = kthread_run(gp_mscard_queue_thread, msdev, "msdev-qd");
		if (IS_ERR(msdev->thread)) {
			goto fail_thread;
		}	
		
		msdev->queue->queuedata = msdev;
    	
		/*
		 * And the gendisk structure.
		 */
		msdev->gd = alloc_disk(MS_MINORS);
		if (! msdev->gd) {
			DIAG_ERROR("alloc_disk failure\n");
			blk_cleanup_queue(msdev->queue);
			goto fail_gd;
		}
		
		if(MSPROAL_STICK_PRO & msifhndl->Stick) {
    	    disk_capacity = msifhd->UserAreaBlocks * msifhd->BlockSize * msifhd->UnitSize;
    	} else {
    	    disk_capacity = msifhd->UserAreaBlocks * msifhd->BlockSize * 1024;
    	} 
		DIAG_ERROR("disk_capacity = %d\n",disk_capacity);
		
		msdev->gd->major = ms_major;
		msdev->gd->first_minor = 0;
		msdev->gd->fops = &gp_mscard_ops;
		msdev->gd->queue = msdev->queue;
		msdev->gd->private_data = msdev;
		snprintf (msdev->gd->disk_name, 32, "mscard");
		set_capacity(msdev->gd, disk_capacity >> 9);
		add_disk(msdev->gd);
		goto init_work_end;
    }
fail_gd:
	/* ----- Then terminate our worker thread ----- */
	kthread_stop(msdev->thread);
fail_thread:
	if (msdev->sg)
		kfree(msdev->sg);
	msdev->sg = NULL;
	blk_cleanup_queue (msdev->queue);	
init_work_end:	
	msdev->timer.expires = jiffies + 2/HZ;		/* 500ms */
	add_timer(&msdev->timer);
}

/**
* @brief 	Card un-initial function.
* @param 	work[in]: Work structure.
* @return 	None.
*/ 
static void gp_mscard_work_uninit(struct work_struct *work)
{
	struct ms_dev *msdev = container_of(work, struct ms_dev,uninit);
	
	del_gendisk(msdev->gd);
	
	kthread_stop(msdev->thread);
	if (msdev->sg)
		kfree(msdev->sg);
	msdev->sg = NULL;
	
	blk_cleanup_queue (msdev->queue);
	
	put_disk(msdev->gd);
	
	if(msdev->handle_dma)
		gp_apbdma0_release(msdev->handle_dma);
	msdev->users = 0;
	
	msdev->timer.expires = jiffies + 2/HZ;		/* 500ms */
	add_timer(&msdev->timer);
	msproal_user_uninit_system();
}

/**
 * @brief 		MS driver initial function.
* @return 	SUCCESS/ERROR_ID.
 */

static int __init gp_mscard_init(void)
{
	/*
	 * Get registered.
	 */
	ms_major = register_blkdev(ms_major, "mscard");
	if (ms_major <= 0) {
		DIAG_ERROR("mscard: unable to get major number\n");
		return -EBUSY;
	}
	/*
	 * Allocate the device array, and initialize each one.
	 */
	Devices = kmalloc(sizeof (struct ms_dev), GFP_KERNEL);
	if (Devices == NULL)
		goto out_unregister;
	
	memset(Devices,0,sizeof (struct ms_dev));

	WorkArea = kmalloc(sizeof (struct __msifhndl), GFP_KERNEL);
	if (WorkArea == NULL)
		goto out_unregister;
			
	spin_lock_init(&Devices->lock);
	
	Devices->cd_pin = gp_gpio_request(MS_CDPIN,"mscard_detect");
	gp_gpio_set_function(Devices->cd_pin, GPIO_FUNC_GPIO);
	gp_gpio_set_direction(Devices->cd_pin, GPIO_DIR_INPUT);
	gp_gpio_set_pullfunction(Devices->cd_pin, GPIO_PULL_FLOATING);
		
	INIT_WORK(&Devices->init, gp_mscard_work_init);
	INIT_WORK(&Devices->uninit, gp_mscard_work_uninit);
	
	init_timer(&Devices->timer);
	Devices->timer.data = (unsigned long) Devices;
	Devices->timer.function = gp_mscard_timer_fn;
	Devices->timer.expires = jiffies + 2/HZ;
	add_timer(&Devices->timer);
	
	DIAG_ERROR("MS module init\n");
	
	return 0;

  out_unregister:
	unregister_blkdev(ms_major, "mscard");
	return -ENOMEM;
}

/**
 * @brief 		MS driver exit function.
* @return 	None.
 */
static void __exit gp_mscard_exit(void)
{
	struct ms_dev *msdev = Devices;
	
	if (msdev->gd) {
		del_gendisk(msdev->gd);
		put_disk(msdev->gd);
		if(msdev->handle_dma)
			gp_apbdma0_release(msdev->handle_dma);
	}
	
	if (msdev->queue) {
		blk_cleanup_queue(msdev->queue);
	}
	
	gp_gpio_release(msdev->cd_pin);	

	unregister_blkdev(ms_major, "mscard");
	
	kfree(msdev);
	kfree(WorkArea);
}

/**
 * @brief 		MS card io control function.
 * @param 	dev[in]: Block device pointer.
* @param 	mode[in] : Mode.
* @param	cmd[in]: Command.
* @param	arg[in]: Argument.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_mscard_ioctrl (struct block_device * device, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct ms_dev *msdev = device->bd_disk->private_data;
	MSIFHNDL *msifhd = (MSIFHNDL *) WorkArea;
	
	int ret = 0;
	
	spin_lock(msdev->lock);
	
	switch(cmd) {
		case MS_CLK_SET:
			msproal_user_set_clock(arg);
			break;
		case MS_BUS_MODE:
			ret = msproal_msif_change_ifmode(msifhd, arg);
			break;
		case MS_XFR_MODE:
			msproal_set_xfr_mode(arg);
		default:
			ret = -ENOIOCTLCMD;
			break;
	}
	
	spin_unlock(msdev->lock);
	return ret;	
}

module_init(gp_mscard_init);
module_exit(gp_mscard_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP Ms card Driver");
MODULE_LICENSE_GP;

