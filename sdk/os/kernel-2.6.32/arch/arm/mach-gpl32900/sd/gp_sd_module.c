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
#include <mach/diag.h>
#include <mach/bdev.h>
#include <mach/gp_sd.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_sd.h>
#include <mach/hal/hal_clock.h>
#include <mach/gp_apbdma0.h>
#include <mach/gp_partition.h>
#include <linux/scatterlist.h> 
#include <asm/scatterlist.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/


#define SD_MINORS					16
#define GP_APP_MINOR				SD_MINORS-1	/* For GP partitiion tool use */ 

#define SD_MAX_SECTORS				1024
#define SD_MAX_PHY_SEGMENTS			128
#define SD_MAX_HW_SEGMENTS			128
#define SD_MAX_PHY_SEGMENTS_SIZE	0x10000

#define SD_DEBOUNCE					25			/* unit: SD_CD_POLL ms */
#define SD_CD_POLL					HZ/5		/* SD card detect polling duration time */

#define GP_TAG						0x4D4D4453

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#if 0
#define DEBUG(fmt, arg...)	DIAG_DEBUG("[%s][%d][DBG]: "fmt, __FUNCTION__, __LINE__, ##arg)
#else
#define DEBUG(...)
#endif

#define DERROR(fmt, arg...) DIAG_DEBUG("[%s][%d][ERR]: "fmt, __FUNCTION__, __LINE__, ##arg)
#define PIN_REQUEST_FUNCTION

#define gp_sdcard_app_read_sector SdcAppReadSector 
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
extern int gp_sdcard_carduninit (gpSDInfo_t* sd);
extern int gp_sdcard_setbus(gpSDInfo_t* sd, unsigned int mode);
extern int gp_sdcard_read_scatter(gpSDInfo_t * sd, unsigned int sector, struct scatterlist *sg, unsigned int ln);
extern int gp_sdcard_write_scatter(gpSDInfo_t * sd, unsigned int sector, struct scatterlist *sg, unsigned int ln);

/**************************************************************************
*                         G L O B A L    D A T A                         *
**************************************************************************/
 
static int sd_major = 0;
int sd_clock = 40;

//module_param(sd_major, int, 0);
module_param_named(clock, sd_clock, int, S_IRUGO);
MODULE_PARM_DESC(clock, "SD Clock (MHz)");



gpSDInfo_t *sd_info = NULL;

/* ----- SD Class OPS, for SD Driver internal reference ----- */
static const struct block_device_operations gp_sdcard_ops = {
	.open 	= gp_sdcard_open,
	.release 	= gp_sdcard_release,
	.ioctl		= gp_sdcard_ioctrl,
	.getgeo 	= gp_sdcard_getgeo,
	.owner 	= THIS_MODULE,
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
		DEBUG("[%d]: no detect function\n", sd->device_id);
		return 0;	
	}
	return (sd->sd_func->detect()==1)?1:0;
}

/**
* @brief 	SD module clean up function.
* @param 	sd[in]: Card information.
* @return 	None.
*/ 
static void gp_sdcard_cleanup(gpSDInfo_t* sd)
{
	/* ----- Stop new requests from getting into the queue ----- */
	if(sd->gd)
		del_gendisk(sd->gd);
	/* ----- Then terminate our worker thread ----- */
	if(sd->thread)
	{
		kthread_stop(sd->thread);
		sd->thread = NULL;
	}
	
	/* ----- Empty the queue ----- */
	if(sd->queue)
	{
		unsigned long flags;
		spin_lock_irqsave(&sd->lock, flags);
		sd->queue->queuedata = NULL;
		blk_start_queue(sd->queue);
		spin_unlock_irqrestore(&sd->lock, flags);
	}
	
	if (sd->sg)
		kfree(sd->sg);
	sd->sg = NULL;
	/* ----- free dma channel ----- */
	if(sd->handle_dma)
		gp_apbdma0_release(sd->handle_dma);
	sd->handle_dma = 0;
}

/**
* @brief 	SD module clean up function.
* @param 	sd[in]: Card information.
* @return 	None.
*/ 
static void gp_sdcard_blk_put(gpSDInfo_t* sd)
{
	if(sd->queue)
	{
		blk_cleanup_queue (sd->queue);
		sd->queue = NULL;
	}
	if(sd->gd)
	{
		put_disk(sd->gd);
		sd->gd = NULL;
	}
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
	if(gp_sdcard_ckinsert(sd)==0)
	{
		sd->fremove = 1;
		return -ENXIO;
	}
#ifdef PIN_REQUEST_FUNCTION
	/* ----- Get pin handle ----- */
	pin_handle = gp_board_pin_func_request((sd->device_id==0)?GP_PIN_SD0:GP_PIN_SD1, GP_BOARD_WAIT_FOREVER);
	if(pin_handle<0)
	{
		DERROR("[%d]: can't get pin handle\n", sd->device_id);
		return -EBUSY;
	}
#endif
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
#ifdef PIN_REQUEST_FUNCTION
	gp_board_pin_func_release(pin_handle);
#endif
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

#if 0	/* This is used for usb disk check */
		{
			bool do_sync = (rq_is_sync(req) && rq_data_dir(req) == WRITE);
			if (do_sync) 
			{
				DEBUG("[Jerry] detect do write sync\n");
			}
		}
#endif		
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
	DEBUG("[%d]: txrx fail %d\n", sd->device_id, ret);
	__blk_end_request_all(req, ret);;
	spin_unlock_irq(&sd->lock);
	return -ENXIO;
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
		if(sd->queue==NULL)
			continue;
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
			spin_lock_irq(&sd->lock);
			__blk_end_request_all(req, -ENXIO);
			spin_unlock_irq(&sd->lock);
			sd->fremove = 1;
			continue;			
		}
		if(gp_sdcard_xfer_request(sd, req)<0)
			sd->fremove = 1;
	} while (1);
	up(&sd->thread_sem);
	return 0;
}
/**
* @brief 	parse header,get the start sector of partition.
* @param 	sd[in]: Card information.
*/ 
static int gp_sdcard_parse_header(gpSDInfo_t * sd)
{
	struct scatterlist *sg;
	char *buf;
	unsigned int Tag1, Tag2;
	unsigned short part[8] = {292, 294, 300, 302, 52, 54, 60, 62};
	unsigned int mbr_offset = 0;
	unsigned int part_offset = 0;
	unsigned int RtCode;
	int ret = 0;
	int i=0;
	/* ----- Alloc buffer first ----- */
	buf = kmalloc(512, GFP_DMA);
	if(buf==NULL)
	{
		DERROR("No buf memory\n");
		return -ENOMEM;
	}
	/* ----- Alloc scatter list ----- */
	sg = kmalloc(sizeof(struct scatterlist), GFP_KERNEL);
	if(sg==NULL)
	{
		DERROR("No sg memory\n");
		ret = -ENOMEM;	
		goto fail_mem;
	}
	sg_init_one(sg, buf, 512);
	/* ----- Read sector 0 to get table ----- */		
	if(gp_sdcard_transfer_scatter(sd, 0, sg, 1, 0)<0)
	{
		DERROR("Read sector fail\n");
		ret = -EIO;
		goto fail_read;	
	}
	Tag1 = *(unsigned int*)&buf[0];
	Tag2 = *(unsigned int*)&buf[0xc8];
	/* ----- GP MBR ----- */
	if( Tag2 == GP_TAG )
	{
		mbr_offset = *(unsigned short*)&buf[0xcc];
		part_offset = buf[0x1c9];
		part_offset = (part_offset<<8)|buf[0x1c8];
		part_offset = (part_offset<<8)|buf[0x1c7];
		part_offset = (part_offset<<8)|buf[0x1c6];
		/* ----- Read boot header ----- */		
		if(gp_sdcard_transfer_scatter(sd, mbr_offset, sg, 1, 0)<0)
		{
			DERROR("Read sector fail\n");
			ret = -EIO;
			goto fail_read;	
		}
	}
	/* ----- Sector 0 is boot header ----- */
	else if( Tag1 == GP_TAG )
	{
		goto decode_boot;
	}
	/* ----- No MBR ----- */
	else
		goto end;
decode_boot:
	
	if(buf[4] < 0x20 ) { /* version 1.0 */
    	RtCode = mbr_offset + *(unsigned short*)&buf[20];
    }
    else { /* version 2.0 */
    	RtCode = mbr_offset + *(unsigned int*)&buf[312];
    }
    
	for(i=0;i<MAX_SD_PART;i++)
		sd->partition.capacity[i]= *(unsigned short*)&buf[part[i]]*2048;		//Unit: 1MB
	/* ----- Read sector 0 to get table ----- */		
	if(gp_sdcard_read_scatter(sd, RtCode, sg, 1)<0)
	{
		DERROR("Read sector fail\n");
		ret = -EIO;
		goto fail_read;	
	}
	/* ----- Get app area ----- */
	sd->partition.offset[MAX_SD_PART] = RtCode;
	sd->partition.capacity[MAX_SD_PART] = *(unsigned int*)&buf[8];
	/* ----- Get data area ----- */
	sd->partition.offset[0] = (part_offset)? part_offset : (RtCode + *(unsigned int*)&buf[8]);
	for(i=1;i<MAX_SD_PART;i++)
	{
		if(sd->partition.capacity[i]==0)
			continue;
		sd->partition.offset[i]= sd->partition.offset[i-1] + sd->partition.capacity[i-1];
		sd->partition.partition_num ++;
	} 
	sd->partition.activity = 1;
	/* ----- For CodePacker Bug ----- */
	if(sd->partition.offset[MAX_SD_PART]&&(sd->partition.capacity[MAX_SD_PART]==0))
		sd->partition.capacity[MAX_SD_PART] = 0x10000 - sd->partition.offset[MAX_SD_PART];
	//for(i=0;i<MAX_SD_PART+1;i++)
	//	DEBUG("SD%d partition %d, start = 0x%x, capacity = 0x%x\n", sd->device_id, i, sd->partition.offset[i], sd->partition.capacity[i]);
end:
fail_read :	
	kfree(sg);
fail_mem:
	kfree(buf);
	return ret;
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
	int ret = 0,i=0;
	
	pin_handle = gp_board_pin_func_request((sd->device_id==0)?GP_PIN_SD0:GP_PIN_SD1, GP_BOARD_WAIT_FOREVER);
	if(pin_handle<0)
	{
		DERROR("[%d]: can't get pin handle\n", sd->device_id);
		goto init_work_end;
	}
	/* ----- Enable SD clock and init module ----- */
	gpHalScuClkEnable(0x040000<<sd->device_id, SCU_C, 1);
	gpHalSDInit(sd->device_id);
    /* ----- chris: Set Pin state for SD before power on ----- */	
    sd->sd_func->set_power(1);
	/* ----- chris: delay 250ms after card power on ----- */	
	msleep(250);
	/* ----- Initial SD card ----- */
	ret = gp_sdcard_cardinit(sd);
	if (ret != 0)
	{
		DERROR("[%d]: initial fail\n",sd->device_id);
		gp_board_pin_func_release(pin_handle);	
		goto init_work_end;		
	}
	gp_board_pin_func_release(pin_handle);	

	if(sd->present==1)
	{
		if(sd->card_type == SDIO)
		{
			sd->pin_handle = gp_board_pin_func_request((sd->device_id==0)?GP_PIN_SD0:GP_PIN_SD1, GP_BOARD_WAIT_FOREVER);
			if(sd->pin_handle<0)
			{
				DERROR("[%d]: can't get pin handle\n", sd->device_id);
				goto init_work_end;
			}
			DEBUG("SDIO card detected\n");
			gp_sdio_insert_device(sd->device_id, sd->RCA);	
		}
		else
		{
			unsigned int cnt =0;
			/* ----- Wait 30 second for all process close handle ----- */
			while((sd->users)&&cnt<120 * 60)
			{
				msleep(250);
				cnt++;	
			}
			if(sd->users)
			{
				DERROR("Some handle do not free\n");	
			}
			if(sd->status)
			{
				gp_sdcard_blk_put(sd);		
				sd->status = 0;			
			}
			sd->handle_dma = gp_apbdma0_request(1000);
			if(sd->handle_dma==0)
				goto init_work_end;
			sd->queue = blk_init_queue(gp_sdcard_request, &sd->lock);
			if(sd->queue==NULL)
			{
				DERROR("NO MEMORY: queue\n");
				goto fail_queue;
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
			/* ----- Check SD card for GP special header ----- */
			if(gp_sdcard_parse_header(sd)<0)
			{
				goto fail_gd;
			}	
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
			/* ----- Set GP partition ----- */
			if(sd->partition.activity)
			{
				set_capacity(sd->gd,0);
				add_disk(sd->gd);
				for(i=0;i<MAX_SD_PART;i++)
				{
					if(sd->partition.capacity[i]==0)
						continue;
					gp_add_partition(sd->gd,i+1,sd->partition.offset[i],sd->partition.capacity[i],ADDPART_FLAG_WHOLEDISK);
				}
			}
			/* ----- Normal Setting ----- */
			else
			{
				set_capacity(sd->gd,sd->capacity);
				add_disk(sd->gd);	
			}
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
	sd->thread = NULL;
fail_thread:
	if (sd->sg)
		kfree(sd->sg);
	sd->sg = NULL;
	blk_cleanup_queue (sd->queue);
	sd->queue = NULL;
fail_queue:	
	if(sd->handle_dma)
		gp_apbdma0_release(sd->handle_dma);
	sd->handle_dma = 0;	
	/* ----- For re-initialize ----- */
	sd->present = 0;	
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
		gp_sdcard_cleanup(sd);
	}
	memset (&sd->partition, 0, sizeof(partition_t));
	/* ----- Uninitial SD controller ----- */
	gp_sdcard_carduninit(sd);
	sd->fremove = 0;
	sd->status = 1;
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
	if( (sd->present ^ insert)||(sd->fremove))
	{
		if((insert==0)||(sd->fremove))
		{
			sd->present = 0;	
			sd->cnt = 0;
			DEBUG("[%d]:Card Remove\n",sd->device_id);
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
				DEBUG("[%d]:Card insert\n",sd->device_id);
				sd->sd_func->set_power(0); /* chris: enable power in identify, need to set power at first */
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
	
	if(gp_sdcard_ckinsert(sd)==0)
		return -ENXIO;
	/* ----- Check write protect ----- */
	if((mode&FMODE_WRITE)&&(sd->sd_func->is_write_protected()==1))
	{
		DEBUG("Read only card\n");
		return -EROFS;
	}
	sd->users++;
	//DEBUG("[%s]: user %d, status %d\n",__FUNCTION__,sd->users,sd->status);
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
	
	if((sd->users==1)&&sd->status)
	{
		gp_sdcard_blk_put(sd);
		sd->status = 0;
		sd->users = 0;
	}
	else if(sd->users)
		sd->users--;
		
	//DEBUG("[%s]: user %d, status %d\n",__FUNCTION__,sd->users,sd->status);
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
		case SD_APP_SET:
			if(sd->partition.activity)
			{
				/* ----- Enable app area ----- */
				if(arg==1)
				{
					struct hd_struct *part;
					part = gp_add_partition(sd->gd, GP_APP_MINOR, sd->partition.offset[MAX_SD_PART], sd->partition.capacity[MAX_SD_PART], ADDPART_FLAG_WHOLEDISK);
					if (IS_ERR(part))
						ret = -EIO;
				}
				/* ----- Disable app area ----- */
				else
					gp_delete_partition(sd->gd, GP_APP_MINOR);
			}
			else
				ret = -ENOIOCTLCMD;
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
 * @brief   sd device release                                   
 */                                                                         
static void gp_sd_device_release(struct device *dev)                       
{                                                                           
}  

static struct platform_device gp_sd_device = {
	.name	= "gp-sd",
	.id	= -1,
	.dev	= {                                                                 
		.release = gp_sd_device_release,                                       
	},  
};

#ifdef CONFIG_PM
static int gp_sd_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i ;
	for(i = 0; i < SD_NUM; i++)
	{
		gpSDInfo_t *sd = &sd_info[i];
		/* diable sd clock */
		if( (sd->present==1) && (gp_sdcard_ckinsert(sd)==1) )
		{
			gpHalScuClkEnable(0x040000<<sd->device_id, SCU_C, 0);
		}
	}
	return 0;
}

static int gp_sd_resume(struct platform_device *pdev)
{
	int i;
	for(i = 0; i < SD_NUM; i++)
	{
		gpSDInfo_t *sd = &sd_info[i];
		/* enable sd clock */
		if( (sd->present==1) && (gp_sdcard_ckinsert(sd)==1) )
		{
			gpHalScuClkEnable(0x040000<<sd->device_id, SCU_C, 1);
		}
	}
	return 0;
}
#else
#define gp_sd_suspend 	NULL
#define gp_sd_resume 	NULL
#endif

/**
 * @brief   SD driver define
 */
static struct platform_driver gp_sd_driver = {
	.suspend = gp_sd_suspend,
	.resume = gp_sd_resume,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "gp-sd"
	},
};


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
	for(i = 0; i < SD_NUM; i++)
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
            if ( sd->sd_func == NULL ) {
                DEBUG("NO SD0 found in board config\n");
                continue;
            }
		}
		else
		{
			/* ----- request IO function ----- */
			sd->sd_func = gp_board_get_config("sd1",gp_board_sd_t);
			/* ----- Initial dma module ----- */
			sd->dma_param.module = SD1;
            if ( sd->sd_func == NULL ) {
                DEBUG("NO SD1 found in board config\n");
                continue;
            }
		}
        /* ----- Set Power Off Default ----- */
        sd->sd_func->set_power(0);
		/* ----- Init timer ----- */
		init_timer(&sd->timer);
		sd->timer.data = (unsigned long) sd;

		sd->timer.function = gp_sdcard_timer_fn;
		sd->timer.expires = jiffies + SD_CD_POLL;

		add_timer(&sd->timer);
	}

	gp_sdio_init();

	platform_device_register(&gp_sd_device);
	platform_driver_register(&gp_sd_driver);
	
	return 0;
out_unregister:
	unregister_blkdev(sd_major, "sdcard");
	return -ENOMEM;
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
		del_timer_sync(&sd->timer);
		gp_sdcard_cleanup(sd);
		gp_sdcard_blk_put(sd);
	}
	unregister_blkdev(sd_major, "sdcard");
	gp_sdio_exit();
	kfree(sd_info);

	platform_device_unregister(&gp_sd_device);
	platform_driver_unregister(&gp_sd_driver);
	
}

int gp_sd0_ispresent(void)
{
	gpSDInfo_t *sd = &sd_info[0];
	return sd->present;
}
EXPORT_SYMBOL(gp_sd0_ispresent);

int gp_sd1_ispresent(void)
{
	gpSDInfo_t *sd = &sd_info[1];
	return sd->present;
}
EXPORT_SYMBOL(gp_sd1_ispresent);

int SdcAppReadSector(unsigned int sd_num, unsigned int lba, unsigned short sector_num, unsigned int buf_addr)
{
    gpSDInfo_t *sd;
    unsigned int app_start_sector;
    struct scatterlist sg;
    
    if (sd_num > 2) {
    	return -EINVAL;
    }
    
    sd = &sd_info[sd_num];
    app_start_sector = sd->partition.offset[MAX_SD_PART];

	sg_init_one(&sg, (char*)buf_addr, sector_num*512);
		
	if(gp_sdcard_transfer_scatter(sd, app_start_sector+lba, &sg, 1, 0)<0)
	{
		DEBUG("Read sector fail\n");
		return -EIO;
	}

    return 0;
}
EXPORT_SYMBOL(SdcAppReadSector);

module_init(gp_sdcard_init);
module_exit(gp_sdcard_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus SD Driver");
MODULE_LICENSE_GP;
