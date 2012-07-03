/*
 * Sample disk driver, from the beginning.
 */
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>        /* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <mach/diag.h>
#include "drv_l2_nand_manage.h"
#include "../gp_nand_config/drv_l2_nand_config.h"

#include <mach/gp_nand_data.h>

#include <mach/gp_cache.h>
#include <mach/kernel.h>
#include <mach/module.h>

//#define USE_MAKE_REQUEST

#if !defined(USE_MAKE_REQUEST)
#include <linux/scatterlist.h> /* sg_init_table */
#include <linux/kthread.h> /* kthread_run, kthread_stop */
#include <linux/dma-mapping.h>
#endif

#define NAND_MAX_SECTORS							2048
#define NAND_MAX_PHY_SEGMENTS						128
#define NAND_MAX_HW_SEGMENTS						128
#define NAND_MAX_PHY_SEGMENTS_SIZE					0x10000

//extern NF_DATA_HAL_INFO gSTNandDataHalInfo;
//extern UINT16 DrvNand_read_sector(UINT32 wReadLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
//extern UINT16 DrvNand_write_sector(UINT32 wWriteLBA, UINT16 wLen, UINT32 DataBufAddr,UINT8 mode);
//extern UINT32 DrvNand_get_Size(void);
//extern UINT16	DrvNand_flush_allblk(void);
//extern UINT16 DrvNand_lowlevelformat(void);
//extern UINT16 DrvNand_initial(void);
extern UINT16 nand_page_size_get(void);

MODULE_LICENSE("Dual BSD/GPL");
int disable_parse_header = 0;
module_param(disable_parse_header, int, 0);
static int sbull_major = 0;
module_param(sbull_major, int, 0);
static int hardsect_size = 512;
module_param(hardsect_size, int, 0);
static int nsectors = 0;//0x40000;//1024;     /* How big the drive is */
module_param(nsectors, int, 0);
static int ndevices = 1;
module_param(ndevices, int, 0);
UINT32 Nand_Chip = 0;

static UINT32 gNoFSAreaSectorSize = 0;
static UINT8	ture_partition = 0;
/*
 * Minor number and partition management.
 */
#define SBULL_MINORS    16
#define MINOR_SHIFT     4
#define DEVNUM(kdevnum) (MINOR(kdev_t_to_nr(kdevnum)) >> MINOR_SHIFT

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE      512

/*
 * After this much idle time, the driver will simulate a media change.
 */
#define INVALIDATE_DELAY        30*HZ

/*
 * The internal representation of our device.
 */
struct sbull_dev {
	int start;											/* start sector */
	int size;                       /* Device size in sectors */
	u8 *data;                       /* The data array */
	short users;                    /* How many users */
	short media_change;             /* Flag a media change? */
	spinlock_t lock;                /* For mutual exclusion */
	struct request_queue *queue;    /* The device request queue */
	struct gendisk *gd;             /* The gendisk structure */

#if !defined(USE_MAKE_REQUEST)	
	struct scatterlist		*sg;			/* scatter list. */
	struct task_struct 		*thread;
	struct request				*req;	
	//atomic_t 						sync_detected;
	unsigned long         sync_detected;
#endif
};

static struct sbull_dev *Devices = NULL;

SINT32 read_write_status = 0;

/*
 * Handle an I/O request.
 */
static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
	unsigned int ret = 0;
	unsigned long start;	
	
	start = dev->start+sector;
	read_write_status = 0;
	//size	= dev->size;
	if (write)
	{
	
#ifdef PART0_WRITE_MONITOR_DEBUG	
		if(nsect==0)
		{
			printk ("DrvNand_write_sector length 0! \n");
		}
		else
		{
			if(((start+nsect-1)>=gSTNandConfigInfo.Partition[0].offset)&&
			   ((start+nsect-1)<gSTNandConfigInfo.Partition[0].size))
			{
				//while(1)//for(ret=0;ret<50;ret++)
				for(ret=0;ret<50;ret++)
				{				
					printk ("ERROR:CRAM fs Partition,AP Should can`t be writen!!! zurong <-- \n");
					printk (" start:0x%x nsect:0x%x \n",(unsigned int)start,(unsigned int)nsect);
				}
				printk("==Hangup to debug!!==\n");
				while(1);
				return;
			}	
		}	
#endif
		ret = DrvNand_write_sector_NoSem(start, nsect, (UINT32)buffer,0);
		if(ret != 0)
		{
#ifdef PART0_WRITE_MONITOR_DEBUG		
			//while(1)
			for(ret=0;ret<50;ret++)
			{
				printk (" start:0x%x nsect:0x%x \n",(unsigned int)start,(unsigned int)nsect);
				printk ("DrvNand_write_sector Fail! zurong \n");
			}
			printk("==Hangup to debug!!==\n");
			while(1);
#else
			printk ("DrvNand_write_sector Fail! start:0x%x nsect:0x%x \n",(unsigned int)start,(unsigned int)nsect);			
#endif		
			read_write_status = -EIO;
		}
	}
	else
	{
		//printk ("Read start sector:0x%x len:0x%x \n",start,nsect);
		ret = DrvNand_read_sector_NoSem(start, nsect, (UINT32)buffer,0);	
		if(ret != 0)
		{
#ifdef PART0_WRITE_MONITOR_DEBUG		
			//while(1)
			for(ret=0;ret<50;ret++)
			{
				printk ("DrvNand_read_sector Fail! zurong \n");
				printk (" start:0x%x nsect:0x%x \n",(unsigned int)start,(unsigned int)nsect);
			}	
			printk("==Hangup to debug!!==\n");
			while(1);
#else
			printk ("DrvNand_read_sector Fail! start:0x%x nsect:0x%x \n",(unsigned int)start,(unsigned int)nsect);
#endif
			read_write_status = -EIO;
		}
	}	
}


#ifdef USE_MAKE_REQUEST
/*
 * Transfer a single BIO.
 */
static int sbull_xfer_bio(struct sbull_dev *dev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;
	
	bool do_sync; 
  int do_sync_req = 0;

	Nand_OS_LOCK();	
	/* Do each segment independently. */
	bio_for_each_segment(bvec, bio, i) 	//这个是一个宏定义
	{
		char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);//luowl
		
		#if 1
		if(1)//(nand_page_size_get() == 2048)
		{
	  	do_sync = (bio_rw_flagged(bio, BIO_RW_SYNCIO) && bio_data_dir(bio) == WRITE); 
	    if (do_sync) { 
	    	//printk("detect do write sync\n"); 
      	do_sync_req++; 
	    } 
	  }  
    #endif 

		
		sbull_transfer(dev, sector, bio_cur_bytes(bio) >> 9,
				buffer, bio_data_dir(bio) == WRITE);
		sector += bio_cur_bytes(bio) >> 9;
		__bio_kunmap_atomic(bio, KM_USER0);
	}
	
	#if 1
	if(1)//(nand_page_size_get() == 2048)
	{
  	if (do_sync_req) { 
			printk("do_sync_req \n");
      FlushWorkbuffer_NoSem(); 
  	} 
  }    
  #endif 
  
	Nand_OS_UNLOCK();
	
	return 0; /* Always "succeed" */
}

/*
 * The direct make request version.
 */
static int sbull_make_request(struct request_queue *q, struct bio *bio)
{
	struct sbull_dev *dev = q->queuedata;
	int status;

	status = sbull_xfer_bio(dev, bio);
	bio_endio(bio, status);
	return 0;
}

#else

static SINT32 sbull_sg_xfer_request(struct sbull_dev *dev, struct request *req)
{
	int nsect = 0;
	int isWrite;
  unsigned int ln;
	unsigned sgln;
	unsigned int startSect;
	struct scatterlist *sg_ev;
	enum dma_data_direction dir;
	int k;
		
	startSect = blk_rq_pos(req);
	ln = blk_rq_map_sg(dev->queue, req, dev->sg);
	isWrite = (rq_data_dir(req) == WRITE);
	dir = (isWrite) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
		
	sgln = dma_map_sg(NULL, dev->sg, ln, dir);
	if(sgln!=ln) {
		dma_unmap_sg(NULL, dev->sg, sgln, dir);
		printk("dma_map_sg fail, ln[%u]sgln[%u]\n", ln, sgln);
		return 0;
	}
	
	//printk("startSect[%u]ln[%u]\n", startSect, ln);
	
	for_each_sg(dev->sg, sg_ev, ln, k) {
		unsigned int current_nr_sectors = sg_dma_len(sg_ev)>>9;
		//unsigned char *buffer = (unsigned char *)sg_phys(sg_ev);
		unsigned char *virt_buffer = sg_virt(sg_ev);
		
		if (current_nr_sectors == 0) 
			printk("nand warning:req byte not 512 multiple[%u]\n", sg_dma_len(sg_ev));
			
		//printk("startSect[%u]current_nr_sectors[%u]\n", startSect, current_nr_sectors);
		sbull_transfer(dev, startSect, current_nr_sectors, virt_buffer, isWrite);
		
		if (((UINT32)virt_buffer & 0xf)!= 0)
			printk("nand warning:virt_buffer not 16 multiple[%u]\n", (UINT32)virt_buffer);
			
		//gp_clean_dcache_range((UINT32)virt_buffer, current_nr_sectors*512);
		//gp_invalidate_dcache_range((UINT32)virt_buffer, current_nr_sectors*512);	
		
		startSect += current_nr_sectors;
		nsect += current_nr_sectors;
	}
	
	dma_unmap_sg(NULL, dev->sg, sgln, dir);
	
	return nsect;
}

static int sbull_req_handler_thread(void *arg)
{
	struct sbull_dev *dev = arg;
	struct request_queue *q = dev->queue;
	SINT32 sectors_xferred = 0;
	struct request *req = NULL;
	SINT32 ret = 0;
	int do_sync_req = 0;
	bool do_sync;
	unsigned long flags_spin_lock_irq;
 	unsigned long t;
	unsigned long sync_detected = 0;
	
	current->flags |= PF_MEMALLOC;
	
	Nand_OS_LOCK();
	do {
		req = NULL;
		ret = 0;
		spin_lock_irqsave(q->queue_lock, flags_spin_lock_irq);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!blk_queue_plugged(q))
			req = blk_fetch_request(q);
		dev->req = req;
		spin_unlock_irqrestore(q->queue_lock, flags_spin_lock_irq);
		
		if (!req) {
			sync_detected = test_and_clear_bit(0, &dev->sync_detected); 
			if (sync_detected || do_sync_req) {
				
				#if 1
				if (sync_detected)
					printk("flush work buffer because sync_detected. sync_detected[%lu], do_sync_req[%i], t[%lu]\n", sync_detected, do_sync_req, jiffies);
				#endif
				
				t=jiffies;
				FlushWorkbuffer_NoSem(); 
				printk("flush work buffer spend time. t[%lu]. sync_detected[%lu], do_sync_req[%i]\n", jiffies - t, sync_detected, do_sync_req);
				do_sync_req = 0;
			}
			if (kthread_should_stop()) {
				// printk("kthread be aborted\n");
				set_current_state(TASK_RUNNING);
				break;
			}
			Nand_OS_UNLOCK();
			// printk("going to sleep\n");
			schedule();
			Nand_OS_LOCK();
			continue;
		}
		set_current_state(TASK_RUNNING);
		do_sync = (req->cmd_flags & REQ_RW_SYNC) && (rq_data_dir(req) == WRITE);
		if (do_sync)
			do_sync_req++;
		sectors_xferred = sbull_sg_xfer_request(dev, req);

		if (req->special == (void *)0xdeadbeefUL) {
			printk("req->special == 0xdeadbeefUL\n");
			FlushWorkbuffer_NoSem();
		}
		
		//printk("nand %s:sectors_xferred[%d]sync[%d]\n", (rq_data_dir(req) == WRITE) ? "write": "read", sectors_xferred, ((req->cmd_flags & REQ_RW_SYNC) == REQ_RW_SYNC) ? 1: 0);
		if (sectors_xferred ==0) {
			ret = -ENXIO;
			goto out_error;
		}
		else {
			spin_lock_irqsave(q->queue_lock, flags_spin_lock_irq);
			__blk_end_request(req, 0, sectors_xferred*KERNEL_SECTOR_SIZE);
			spin_unlock_irqrestore(q->queue_lock, flags_spin_lock_irq);
		}
		
	} while (1);

	out_error:
	if (!req) {
		spin_lock_irqsave(q->queue_lock, flags_spin_lock_irq);
		__blk_end_request_all(req, ret);
		spin_unlock_irqrestore(q->queue_lock, flags_spin_lock_irq);
	}
	
	Nand_OS_UNLOCK();
	
	return 0;
}

static void sbull_request_receiver(struct request_queue *q)
{
	struct sbull_dev *dev = q->queuedata;
	struct request *req;
	unsigned long flags_spin_lock_irq;
	
	if (!dev) {
		spin_lock_irqsave(q->queue_lock, flags_spin_lock_irq);
		while ((req = blk_fetch_request(q)) != NULL) {
			req->cmd_flags |= REQ_QUIET;
			__blk_end_request_all(req, -EIO);
		}
		spin_unlock_irqrestore(q->queue_lock, flags_spin_lock_irq);
		
		return;
	}
	
	if (IS_ERR(dev->thread) || (dev->gd == NULL) || (dev->sg == NULL) ) {
		printk("sbull_request_receiver thread err. %li, gd[%x], sg[%x]\n", IS_ERR(dev->thread), (UINT32)dev->gd, (UINT32)dev->sg);
	}
	else {
		if (strncmp(current->comm, "flush", 5) == 0) {
			printk("sync process detected.t[%lu]\n", jiffies);
			set_bit(0, &dev->sync_detected);
		}
		if (!dev->req)
			wake_up_process(dev->thread);
	}
}

static void sbull_prepare_flush(struct request_queue *q, struct request *req)
{
	#if 0
	req->cmd_type = REQ_TYPE_BLOCK_PC;
	req->timeout = SD_TIMEOUT;
	req->cmd[0] = SYNCHRONIZE_CACHE;
	req->cmd_len = 10;
	#endif
	
	/* add driver-specific marker, to indicate that this request
	 * is a flush command
	 */
	req->special = (void *) 0xdeadbeefUL;
	printk("sbull_prepare_flush is called. [%lu]\n", (unsigned long)req);
}

#endif

/*
 * Open and close.
 */
static int sbull_open(struct block_device *device, fmode_t mode)
{
	struct sbull_dev *dev = device->bd_disk->private_data;

	spin_lock(&dev->lock);
	if (!dev->users) 
		check_disk_change(device);

	dev->users++;
	spin_unlock(&dev->lock);

	return 0;
}

static int sbull_release(struct gendisk *disk, fmode_t mode)
{
	struct sbull_dev *dev = disk->private_data;

	//spin_lock(&dev->lock); // wiiliam
	dev->users--;
	//printk (KERN_NOTICE "nand_release enter!!\n");
  	//DrvNand_flush_allblk();	
	//spin_unlock(&dev->lock); // william

	return 0;
}

/*
 * Look for a (simulated) media change.
 */
int sbull_media_changed(struct gendisk *gd)
{
	struct sbull_dev *dev = gd->private_data;
	
	return dev->media_change;
}

/*
 * Revalidate.  WE DO NOT TAKE THE LOCK HERE, for fear of deadlocking
 * with open.  That needs to be reevaluated.
 */
int sbull_revalidate(struct gendisk *gd)
{
	struct sbull_dev *dev = gd->private_data;
	
	if (dev->media_change) {
		dev->media_change = 0;
		memset (dev->data, 0, dev->size);
	}
	return 0;
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int sbull_getgeo(struct block_device * device, struct hd_geometry * geo) {
	long size;
	struct sbull_dev *dev = device->bd_disk->private_data;

	/* We have no real geometry, of course, so make something up. */
	size = dev->size * (hardsect_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

/**
 * @brief Nand device ioctl function
 */
int sbull_ioctl(struct block_device *device, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct sbull_dev *dev = NULL;
	int ret = 0;//, k;
	UINT32 sector_start = 0;
	UINT32 sector_cnt = 0;
	Xsfer_arg1 arg_info;
	//UINT8 *buf;
	//static SINT32 bchEncodeSel = -1;
	
	if (device == NULL)
	{
		return -EINVAL;
	}	

	dev = device->bd_disk->private_data;
	if(dev == NULL)
	{
		return -EINVAL;
	}

	switch (cmd)
	{
	case NAND_FLUASH_ALL_BLK:
		printk ("Nand flush all block \n");
		DrvNand_flush_allblk();
		break;
		
	case NAND_DIRECT_WRITE:
		copy_from_user((void*)&arg_info,(const void __user*)arg,sizeof(Xsfer_arg1));
		sector_start = arg_info.start_sector;
		sector_cnt 	 = arg_info.sector_cnt;
		if((sector_start+sector_cnt)>gNoFSAreaSectorSize)
		{
			printk ("NAND_IOCTRL beyond Write start sector:0x%x len:0x%x zurong \n",(unsigned int)sector_start,(unsigned int)sector_cnt);
			ret = -1;
		}
		else
		{	
			printk ("Monitor: NAND_IOCTRL----->DrvNand_write_sector \n");
			printk ("Monitor: NAND_IOCTRL----->Sector_start:0x%x sector_cnt:0x%x  zurong \n",(unsigned int)sector_start,(unsigned int)sector_cnt);
			ret = DrvNand_write_sector(sector_start, sector_cnt, (UINT32)(arg_info.buffer),1);
		#ifdef PART0_WRITE_MONITOR_DEBUG
			while(ret)
			{
				printk (" sector_start:0x%x sector_cnt:0x%x \n",(unsigned int)sector_start,(unsigned int)sector_cnt);
				printk ("Warning: NAND_IOCTRL----->DrvNand_write_sector \n");
			}
		#else
			if(ret)
			{
				printk ("Monitor: NAND_IOCTRL----->DrvNand_write_sector failed! \n");
			}
		#endif
		}
		break;
	
	case NAND_DIRECT_READ:
		copy_from_user((void*)&arg_info,(const void __user*)arg,sizeof(Xsfer_arg1));
		sector_start = arg_info.start_sector;
		sector_cnt 	 = arg_info.sector_cnt;
		if((sector_start+sector_cnt)>gNoFSAreaSectorSize)
		{
			printk ("NAND_IOCTRL beyond Read start sector:0x%x len:0x%x zurong \n",(unsigned int)sector_start,(unsigned int)sector_cnt);	
			ret = -1;
		}
		//else
		{	printk ("Warning: NAND_IOCTRL----->DrvNand_read_sector \n");
			printk ("Warning: NAND_IOCTRL----->Sector_start:0x%x sector_cnt:0x%x zurong  \n",sector_start,sector_cnt);
			ret = DrvNand_read_sector(sector_start, sector_cnt, (UINT32)(arg_info.buffer),1);
		#ifdef PART0_WRITE_MONITOR_DEBUG	
			while(ret)
			{
				printk ("Warning: NAND_IOCTRL----->DrvNand_read_sector \n");
			}
		#else
			printk ("Warning: NAND_IOCTRL----->DrvNand_read_sector \n");
		#endif	
		}	
		break;	

	case 100:
		printk ("Warning: NAND_IOCTRL----->FlushWorkbuffer zurong \n");
		FlushWorkbuffer();
		printk ("Nand flush work buf \n");
		break;
#if 0		
	case 200:
		{
						
			// if (copy_to_user((void __user *) arg, &ms, sizeof(ms)))
			UINT32 startPage;
			
			printk("func[200]dump phy page\n");
			if (copy_from_user(&startPage, (void __user *) arg, sizeof(UINT32)))
				ret = -EFAULT;
			else {
				printk("Nand_DumpPhyPage[%u]\n", startPage);
				ret = Nand_DumpPhyPage(startPage);
			}
		}
		break;
	
	case 201:
		{
			printk("func[201]bch decode\n");

			if (copy_from_user(&bchEncodeSel, (void __user *) arg, sizeof(SINT32)))
				ret = -EFAULT;
			else {			
				buf = nand_get_dbg_buf();
				if (buf != NULL) {
					if (bchEncodeSel == -1)
						bchEncodeSel = nand_bch_get();
					ret = BCH_Decode(bchEncodeSel, (UINT32)buf, nand_page_size_get(), (UINT32) nand_get_spare_buf(), 0);
				}
			}
		}
		break;

	case 202:
		{
			printk("func[202]bch encode\n");

			if (copy_from_user(&bchEncodeSel, (void __user *) arg, sizeof(SINT32)))
				ret = -EFAULT;
			else {			
				buf = nand_get_dbg_buf();
				if (buf != NULL) {
					if (bchEncodeSel == -1)
						bchEncodeSel = nand_bch_get();
					ret = BCH_Encode(bchEncodeSel, (UINT32)buf, nand_page_size_get(), (UINT32) nand_get_spare_buf(), 0);
					dump_buffer(nand_get_spare_buf(), 1024);
				}
			}
		}
		break;		
	case 203:
		{
			printk("func[203]erase whole nand.total block[%d]\n", gSTNandDataHalInfo.wNandBlockNum);
			for (k=0; k < gSTNandDataHalInfo.wNandBlockNum; k++) {
				ret = Nand_ErasePhyBlock(k);
				printk("erase block [%d] %s.\n", k, ret == 0 ? "success" : "fail");
			}
		}
		break;
#endif		
	default:
		ret = -ENOTTY;
		break;
	}
	return ret; 
}


/*
 * The device operations structure.
 */
static struct block_device_operations sbull_ops = {
	.owner           = THIS_MODULE,	
	.open            = sbull_open,
	.release         = sbull_release,
	.media_changed   = sbull_media_changed,	//	检查驱动器介质是否改变
	.revalidate_disk = sbull_revalidate,	//	响应一个介质改变
	.getgeo          = sbull_getgeo,		//	根据驱动器几何信息填充一个hd_geometry结构体，包含磁头、扇区、柱面等信息
	.ioctl			 = sbull_ioctl 
};

/*
 * Set up our internal device.
 */
extern void gp_nand_test(void);
static void setup_device(struct sbull_dev *dev, int which)
{
	/*
	 * Get some memory.
	 */
	 if(dev == NULL)
 	{
		printk(" No this device!!! \n");
		return ;
 	}
	
	clear_bit(0, &dev->sync_detected);
	
	//memset (dev, 0, sizeof (struct sbull_dev));
	//dev->size = nsectors * hardsect_size;
	dev->data = 0;
	
	spin_lock_init(&dev->lock);
	
	/*
	 * The I/O queue, make_request function.
	 */
#if defined(USE_MAKE_REQUEST)
	dev->queue = blk_alloc_queue(GFP_KERNEL);
	if (dev->queue == NULL)
		goto out_vfree;
	blk_queue_make_request(dev->queue, sbull_make_request);
#else
	dev->queue = blk_init_queue(sbull_request_receiver, &dev->lock);
	if (dev->queue == NULL)
		goto out_vfree;
			
	blk_queue_ordered(dev->queue, QUEUE_ORDERED_DRAIN_FLUSH, sbull_prepare_flush);
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, dev->queue);
	// queue_flag_clear(QUEUE_FLAG_NOMERGES, dev->queue);
	blk_queue_max_sectors(dev->queue, NAND_MAX_SECTORS);
	blk_queue_max_phys_segments(dev->queue, NAND_MAX_PHY_SEGMENTS);
	blk_queue_max_hw_segments(dev->queue, NAND_MAX_HW_SEGMENTS);
	blk_queue_max_segment_size(dev->queue, NAND_MAX_PHY_SEGMENTS_SIZE);
	
	/* ----- Initial scatter list ----- */
	dev->sg = kmalloc(sizeof(struct scatterlist)*NAND_MAX_PHY_SEGMENTS, GFP_KERNEL);
	if (!dev->sg) {
		printk("NO MEMORY: sg fail\n");
		goto out_vfree;
	}
	sg_init_table(dev->sg, NAND_MAX_PHY_SEGMENTS);
#endif
	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;

	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(SBULL_MINORS);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		goto out_vfree;
	}
	//gp_nand_test();
	dev->gd->major = sbull_major;
	dev->gd->first_minor = which*SBULL_MINORS;
	dev->gd->fops = &sbull_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	if(ture_partition == 0)
	{
		snprintf (dev->gd->disk_name, 32, "nand%c", which + 'a');	
		printk (KERN_NOTICE "Nand disk %c size: 0x%x \n", which + 'a',dev->size);
	}
	else
	{	
		snprintf (dev->gd->disk_name, 32, "nanda%c", which + '1');
		printk (KERN_NOTICE "Nand diska%c size: 0x%x \n", which + '1',dev->size);
	}
	set_capacity(dev->gd, dev->size);	
	
	#if !defined(USE_MAKE_REQUEST)
	dev->thread = kthread_run(sbull_req_handler_thread, dev, dev->gd->disk_name);
	if (IS_ERR(dev->thread)) 
		goto out_vfree;
	#endif
	
	add_disk(dev->gd);
	return;

  out_vfree:
  	
  #if !defined(USE_MAKE_REQUEST)
  if (dev->thread)
		kthread_stop(dev->thread);
	dev->thread = NULL;
  if (dev->sg)
		kfree(dev->sg);
	dev->sg = NULL;
	#endif
	
	if (dev->data)
		vfree(dev->data);
}

static int gp_nand_data_suspend(struct platform_device *pdev, pm_message_t state)
{	
	printk(" Nand data driver suspend! zurong \n");
	DrvNand_flush_allblk();
	DrvNand_UnIntial();
	Nand_UnInit();	// call nand hal suspend	
	return 0;
}

static int gp_nand_data_resume(struct platform_device *pdev)
{
	int i;
	UINT32 gNandTotalSize = 0;
	UINT32 partition_size = 0;
	
	printk(" Nand data driver resume! zurong \n");
	
	DrvNand_initial();

	gNandTotalSize	= DrvNand_get_Size();	 
	gNoFSAreaSectorSize = GetNoFSAreaSectorSize();
	 if(gSTNandConfigInfo.uiPartitionNum==0)
	 {	
		ture_partition = 0;			
		gSTNandConfigInfo.uiPartitionNum = 1;
	 }
	 else
	 {
		ture_partition = gSTNandConfigInfo.uiPartitionNum;			
	 }
	
	for(i=0;i<gSTNandConfigInfo.uiPartitionNum-1;i++)
	{		
		if(gNandTotalSize>=gSTNandConfigInfo.Partition[i].size)
		{
			partition_size  = gSTNandConfigInfo.Partition[i].size;	
		}
		else
		{	
			partition_size		=  gNandTotalSize;
			gSTNandConfigInfo.Partition[i].size = gNandTotalSize;			
		}		
		gNandTotalSize -= partition_size;
	}
	gSTNandConfigInfo.Partition[ndevices-1].size = gNandTotalSize;
	
	Default_CalculateFATArea();
	
	return 0;
}

static void gp_nand_data_device_release(struct device *dev)
{
	printk("remove NandData device ok zurong \n");
}

static struct platform_device gp_nand_data_device = {
	.name	= "gp-nand-data",
	.id	= -1,
	.dev	= {
		.release = gp_nand_data_device_release,
	}
};

static struct platform_driver gp_nand_data_driver = {
	.driver		= {
		.name	= "gp-nand-data",
		.owner	= THIS_MODULE,
	},
	//.probe		= gp_adc_probe,
	//.remove		= __devexit_p(gp_adc_remove),
	.suspend	= gp_nand_data_suspend,
	.resume		= gp_nand_data_resume,
};

static int __init sbull_init(void)
{
	int i;
	int ret;
	UINT32 gNandTotalSize = 0;
	 
	printk(" Nand data driver version:V0.3.4 \n");
#ifdef PART0_WRITE_MONITOR_DEBUG
	printk(" Nand data driver Cram fs debug version:V0.3.4 zurong \n");
#endif	
	platform_device_register(&gp_nand_data_device);
	ret = platform_driver_register(&gp_nand_data_driver);
	if (ret)
	{
		printk("%s: failed to add gp_nand driver\n", __func__);	
		return ret;
	}
	
	printk("===nand init Nand_Chip 0 enter!!===\n");	
	sbull_major = register_blkdev(sbull_major, "nand");
	if (sbull_major <= 0) {
		printk(KERN_WARNING "nand: unable to get major number\n");
		return -EBUSY;
	}	
	
	/*
	 * Allocate the device array, and initialize each one.
	 */
	 
	 gp_nand_test();
	 gNandTotalSize	= DrvNand_get_Size();	 
	 printk("Nanda total size: 0x%x \n",gNandTotalSize);
	 gNoFSAreaSectorSize = GetNoFSAreaSectorSize();
	 if(gSTNandConfigInfo.uiPartitionNum==0)
	 {	
	 		ture_partition = 0;
			ndevices = 1;
			gSTNandConfigInfo.uiPartitionNum = 1;
	 }
	 else
	 {
	 		ture_partition = gSTNandConfigInfo.uiPartitionNum;
	 		ndevices = gSTNandConfigInfo.uiPartitionNum;
	 }
	 		
	Devices = kmalloc(ndevices*sizeof (struct sbull_dev), GFP_KERNEL);
	if (Devices == NULL)
		goto out_unregister;
	
	memset (Devices, 0, ndevices*sizeof (struct sbull_dev));	
	
	for(i=0;i<ndevices-1;i++)
	{
		Devices[i].start = gSTNandConfigInfo.Partition[i].offset;
		if(gNandTotalSize>=gSTNandConfigInfo.Partition[i].size)
		{
			Devices[i].size  = gSTNandConfigInfo.Partition[i].size;	
		}
		else
		{
			Devices[i].size  = gNandTotalSize;
			gSTNandConfigInfo.Partition[i].size = gNandTotalSize;
		}		
		gNandTotalSize -= Devices[i].size;
	}
	Devices[ndevices-1].start = gSTNandConfigInfo.Partition[ndevices-1].offset;
	Devices[ndevices-1].size  = gNandTotalSize;
	gSTNandConfigInfo.Partition[ndevices-1].size = gNandTotalSize;
	
	Default_CalculateFATArea();
	
	
	for (i = 0; i < ndevices; i++) 
		setup_device(Devices + i, i);
    
	return 0;

  out_unregister:	
	unregister_blkdev(sbull_major, "nand");
	return -ENOMEM;
}

static void sbull_exit(void)
{
	int i;
	printk("===nand_exit enter!!===\n");

	for (i = 0; i < ndevices; i++) {
		struct sbull_dev *dev = Devices + i;

		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		dev->gd = NULL;
		
		#if !defined(USE_MAKE_REQUEST)
		if (dev->thread)
			kthread_stop(dev->thread);
		dev->thread = NULL;
  	if (dev->sg)
			kfree(dev->sg);
		dev->sg = NULL;
		#endif
		
		if (dev->queue) {
			blk_cleanup_queue(dev->queue);
		}
		
		DrvNand_flush_allblk();	
		if (dev->data)
			vfree(dev->data);
	}
		
	unregister_blkdev(sbull_major, "nand");
	kfree(Devices);
	Devices = NULL;
	platform_device_unregister(&gp_nand_data_device);
	platform_driver_unregister(&gp_nand_data_driver);	
}
	
module_init(sbull_init);
module_exit(sbull_exit);
