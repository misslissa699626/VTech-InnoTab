/*
 * Sample disk driver, from the beginning.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/timer.h>
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>        /* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>  /* invalidate_bdev */
#include <linux/bio.h>

#include "drv_l2_nand_app.h"
#include <mach/app_dev.h>

#include <mach/kernel.h>
#include <mach/module.h>

#define USE_MAKE_REQUEST

MODULE_LICENSE("Dual BSD/GPL");

UINT32 Nand_Chip = 0;
int disable_parse_header = 0;
module_param(disable_parse_header, int, 0);
static int app_dev_major = 0;
module_param(app_dev_major, int, 0);
static int hardsect_size = 512;
module_param(hardsect_size, int, 0);
static int nsectors = 0x40000;//1024;     /* How big the drive is */
module_param(nsectors, int, 0);
static int ndevices = 1;
module_param(ndevices, int, 0);

/*
 * Minor number and partition management.
 */
#define APP_DEV_MINORS    1//16
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
struct app_dev {
	int size;                       /* Device size in sectors */
	u8 *data;                       /* The data array */
	short users;                    /* How many users */
	short media_change;             /* Flag a media change? */
	spinlock_t lock;                /* For mutual exclusion */
	struct request_queue *queue;    /* The device request queue */
	struct gendisk *gd;             /* The gendisk structure */
};
//gpAppHeaderUser_t AppHeaderUser;
//gpAppPartUser_t   AppPartInfo;

SINT32 app_header_info(gpAppHeaderUser_t * AppHdrInfo);
SINT32 app_partition_info(SINT32 PartNum,gpAppPartUser_t * PartInfo);
SINT32 app_read_partition(gpAppPartUser_t *PartInfo,UINT32 wLenSector,UINT32 DataBufAddr);
SINT32 app_write_partition(gpAppPartUser_t *PartInfo,UINT32 wLenSector,UINT32 DataBufAddr);

static struct app_dev *Devices = NULL;
u8 AppBuf[512];
/*
 * Handle an I/O request.
 */
static void app_dev_transfer(struct app_dev *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
	
}


#ifdef USE_MAKE_REQUEST
/*
 * Transfer a single BIO.
 */
static int app_dev_xfer_bio(struct app_dev *dev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;

	/* Do each segment independently. */
	bio_for_each_segment(bvec, bio, i) {
		char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);//luowl
		app_dev_transfer(dev, sector, bio_cur_bytes(bio) >> 9,
				buffer, bio_data_dir(bio) == WRITE);
		sector += bio_cur_bytes(bio) >> 9;
		__bio_kunmap_atomic(bio, KM_USER0);
	}
	return 0; /* Always "succeed" */
}

/*
 * The direct make request version.
 */
   static int app_dev_make_request(struct request_queue *q, struct bio *bio)
{
	struct app_dev *dev = q->queuedata;
	int status;

	status = app_dev_xfer_bio(dev, bio);
	bio_endio(bio, status);
	return 0;
}

#else

/*
* The simple form of the request function.
*/
static void app_dev_request(struct request_queue *q)
{
	struct request *req;

	req = blk_fetch_request(q);
	while (req != NULL) {
		struct app_dev *dev = req->rq_disk->private_data;
		if (! blk_fs_request(req)) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}

		//       printk (KERN_NOTICE "Req dev %d dir %ld sec %ld, nr %d f %lx\n",
		//             dev - Devices, rq_data_dir(req),
		//             req->sector, req->current_nr_sectors,
		//             req->flags);
		app_dev_transfer(dev, blk_rq_pos(req), blk_rq_cur_sectors(req), req->buffer, rq_data_dir(req));

		/* end_request(req, 1); */
		if(!__blk_end_request_cur(req, 0)) {
			req = blk_fetch_request(q);
		}
	}
}
#endif

/*
 * Open and close.
 */
static int app_dev_open(struct block_device *device, fmode_t mode)
{	
	printk (KERN_NOTICE "Open Function !!\n");

	return 0;
}

static int app_dev_release(struct gendisk *disk, fmode_t mode)
{
	struct app_dev *dev = disk->private_data;

	spin_lock(&dev->lock);
	dev->users--;
	printk (KERN_NOTICE "app_dev_release enter!!\n");
 // DrvNand_flush_allblk();	
	spin_unlock(&dev->lock);

	return 0;
}

/*
 * Look for a (simulated) media change.
 */
int app_dev_media_changed(struct gendisk *gd)
{
	struct app_dev *dev = gd->private_data;
	
	return dev->media_change;
}

/*
 * Revalidate.  WE DO NOT TAKE THE LOCK HERE, for fear of deadlocking
 * with open.  That needs to be reevaluated.
 */
int app_dev_revalidate(struct gendisk *gd)
{
	struct app_dev *dev = gd->private_data;
	
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
int app_dev_getgeo(struct block_device * device, struct hd_geometry * geo) {
	long size;
	struct app_dev *dev = device->bd_disk->private_data;

	/* We have no real geometry, of course, so make something up. */
	size = dev->size * (hardsect_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

static int app_dev_ioctl(struct block_device *device, fmode_t mode, unsigned int cmd, unsigned long arg)
//static int app_dev_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg)
{	
	int ret;
	nand_info_arg nand_info;
	Xsfer_arg 	  arg_info;
	process_arg   process_info;
	gpAppHeaderUser_t AppHeaderUser;
	gpAppPartUser_t  AppPartInfo;
	XsferPartArg_t  XsferAppPart;
	
	switch(cmd)
	{
		case APP_AREA_FORMAT:			
			NandAppEnableWrite();
			ret = NandAppFormat(0x10);
			break;
		case APP_AREA_INIT:			
			ret = NandAppInit();
			break;
		case APP_AREA_WRITE_SECTOR:	
			copy_from_user((void*)&arg_info,(const void __user*)arg,sizeof(Xsfer_arg));			
			ret = NandAppWriteSector(arg_info.start_sector, arg_info.sector_cnt, (UINT32)(arg_info.buffer),1);
			break;
		case APP_AREA_READ_SECTOR:
			copy_from_user((void*)&arg_info,(const void __user*)arg,sizeof(Xsfer_arg));
			ret = NandAppReadSector(arg_info.start_sector, arg_info.sector_cnt, (UINT32)(arg_info.buffer),1);
			break;
		case APP_AREA_PARTITION_WRITE_SECTOR_END:
		case APP_AREA_FLUSH:			
			ret = NandAppFlush();			
			break;
		case APP_AREA_GET_NANDINFO:			
			NandAPPGetNandInfo(&(nand_info.block_size),&(nand_info.page_size));
			copy_to_user((void __user*)arg,(const void*)&nand_info,sizeof(nand_info_arg));
			ret = 0;
			break;
		case APP_AREA_GET_PROCESS:			
			NandAPPGetProcess(&(process_info.targe_sectors),&(process_info.xsfered_sectors));
			copy_to_user((void __user*)arg,(const void*)&process_info,sizeof(process_arg));
			ret = 0;
			break;
		case APP_AREA_HEADER_INFO:
			ret = app_header_info(&AppHeaderUser);
			copy_to_user((void __user*)arg,(const void*)&AppHeaderUser,sizeof(gpAppHeaderUser_t));
//			printk ("====CMD APP_AREA_HEADER_INFO!==== \n");	
			break;
			
		case APP_AREA_PARTITION_INFO:
			copy_from_user((void*)&AppPartInfo,(const void __user*)arg,sizeof(gpAppPartUser_t));	
			ret = app_partition_info(AppPartInfo.partNumber,&AppPartInfo);
			ret |= copy_to_user((void __user*)arg,(const void*)&AppPartInfo,sizeof(gpAppPartUser_t));
//			printk ("====CMD APP_AREA_PARTITION_INFO!==== \n");	
			break;	

		case APP_AREA_PARTITION_READ_SECTOR:
			copy_from_user((void*)&XsferAppPart,(const void __user*)arg,sizeof(XsferAppPart));
			ret = app_read_partition((gpAppPartUser_t *)&XsferAppPart.partparam,XsferAppPart.sector_cnt,(UINT32)(XsferAppPart.buffer));
			ret |= copy_to_user((void __user*)arg,(const void*)&XsferAppPart,sizeof(XsferAppPart));
			//app_read_partition
//			printk ("====CMD APP_AREA_READ_SECTOR!==== \n");			
			break;
		
		case APP_AREA_PARTITION_WRITE_SECTOR:
			copy_from_user((void*)&XsferAppPart,(const void __user*)arg,sizeof(XsferAppPart));
			ret = app_write_partition((gpAppPartUser_t *)&XsferAppPart.partparam,XsferAppPart.sector_cnt,(UINT32)(XsferAppPart.buffer));
			ret |= copy_to_user((void __user*)arg,(const void*)&XsferAppPart,sizeof(XsferAppPart));
//			printk ("====CMD APP_AREA_WRITE_SECTOR!==== \n");	

			break;
		
	
		default:
			printk (KERN_NOTICE "~~~~~~~~~~~CMD defalut!~~~~~~~~~~~~~~~~~~~~~~~ \n");	
			ret = -1;
			break;		
	}
	
	//printk (KERN_NOTICE "IOCTL CMD EXIT! \n");	
	
	return ret;	
}

/*
 * The device operations structure.
 */
static struct block_device_operations app_dev_ops = {
	.owner           = THIS_MODULE,	
	.open            = app_dev_open,	
	.release         = app_dev_release,
	.ioctl			     = app_dev_ioctl,
	.media_changed   = app_dev_media_changed,
	.revalidate_disk = app_dev_revalidate,
	.getgeo          = app_dev_getgeo
};


/*
 * Set up our internal device.
 */
static void setup_device(struct app_dev *dev, int which)
{
	/*
	 * Get some memory.
	 */
	memset (dev, 0, sizeof (struct app_dev));
	dev->size = nsectors * hardsect_size;
	dev->data = 0;//vmalloc(dev->size);
	/*if (dev->data == NULL) {
		printk (KERN_NOTICE "vmalloc failure.\n");
		return;
	}*/
	spin_lock_init(&dev->lock);
	
	/*
	 * The I/O queue, make_request function.
	 */
#ifdef USE_MAKE_REQUEST
	dev->queue = blk_alloc_queue(GFP_KERNEL);
	if (dev->queue == NULL)
		goto out_vfree;
	blk_queue_make_request(dev->queue, app_dev_make_request);
#else
	dev->queue = blk_init_queue(app_dev_request, &dev->lock);
#endif
	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;

	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(APP_DEV_MINORS);
	if (! dev->gd) {
		printk (KERN_NOTICE "app device alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = app_dev_major;
	dev->gd->first_minor = 0;//which*APP_DEV_MINORS;
	dev->gd->fops = &app_dev_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "app_dev%c", which + 'a');
	set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	add_disk(dev->gd);
	return;

  out_vfree:
	if (dev->data)
		vfree(dev->data);
}

static int gp_nand_app_suspend(struct platform_device *pdev, pm_message_t state)
{	
	printk(" Nand app driver suspend! \n");
	NandAppUninit();
	Nand_UnInit();	// call nand hal suspend	
	return 0;
}

static int gp_nand_app_resume(struct platform_device *pdev)
{	
	printk(" Nand app driver resume! \n");	
	NandAppInit();
	return 0;
}

static void gp_nandapp_device_release(struct device *dev)
{
	printk("remove NandApp device ok\n");
}

static struct platform_device gp_nandapp_device = {
	.name	= "gp-nand-app",
	.id	= -1,
	.dev	= {
		.release = gp_nandapp_device_release,
	}
};

static struct platform_driver gp_nand_app_driver = {
	.driver		= {
		.name	= "gp-nand-app",
		.owner	= THIS_MODULE,
	},
	.probe		= NULL,
	.remove		= NULL,//__devexit_p(gp_adc_remove),
	.suspend	= gp_nand_app_suspend,
	.resume		= gp_nand_app_resume,
};


static int __init app_dev_init(void)
{
	int i,ret;
	/*
	 * Get registered.
	 */
	 
	Nand_Chip = 0;
	printk("===App device init Nand_Chip  0 enter!!===\n");
	printk("===App driver version:V0.1.4===\n");
	platform_device_register(&gp_nandapp_device);
	ret = platform_driver_register(&gp_nand_app_driver);
	if (ret)
	{
		printk("%s: failed to add gp_app driver\n", __func__);	
		return ret;
	}
	 
	app_dev_major = register_blkdev(app_dev_major, "app_dev");
	if (app_dev_major <= 0) {
		printk(KERN_WARNING "app device: unable to get major number\n");
		return -EBUSY;
	}
	/*
	 * Allocate the device array, and initialize each one.
	 */
	Devices = kmalloc(ndevices*sizeof (struct app_dev), GFP_KERNEL);
	if (Devices == NULL)
		goto out_unregister;
	for (i = 0; i < ndevices; i++) 
		setup_device(Devices + i, i);
		//setup_device(Devices, 0);
    
    NandAppInit();

	return 0;

  out_unregister:
	unregister_blkdev(app_dev_major, "app_dev");
	return -ENOMEM;
}

static void app_dev_exit(void)
{
	int i;
	printk("===app_dev_exit enter!!===\n");
	platform_device_unregister(&gp_nandapp_device);
	platform_driver_unregister(&gp_nand_app_driver);
	
	for (i = 0; i < ndevices; i++) {
		struct app_dev *dev = Devices + i;

		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		if (dev->queue) {
#if 0 //#ifdef USE_MAKE_REQUEST
			kobject_put (&dev->queue->kobj);
			/* blk_put_queue() is no longer an exported symbol */
#else
			blk_cleanup_queue(dev->queue);
#endif
		}
		
	//	DrvNand_flush_allblk();	
		if (dev->data)
			vfree(dev->data);
	}
	unregister_blkdev(app_dev_major, "app_dev");
	kfree(Devices);
}
	



SINT32 app_header_info(gpAppHeaderUser_t * AppHdrInfo)
{
	SINT32 ret = 0;
	SINT32 *AppWord = (int *) &AppBuf[0];
	
	gpAppHeader_t *appHeader;
	
	ret = NandAppReadSector(0,1,(UINT32)&AppBuf[0],0);
	if((AppWord[APP_HDR_TAG]!=APP_TAG)||(ret != 0))
	{
		printk("AppHdr Read Fail, BootLoader Stop\r\n");
		return ret;
	}
	
	appHeader = (gpAppHeader_t *)&AppBuf;
	
	AppHdrInfo->headerTag = appHeader->headerTag;
	AppHdrInfo->headerSize = appHeader->headerSize;
	AppHdrInfo->totalAppSize = appHeader->totalAppSize;
	AppHdrInfo->totalPartNumber = appHeader->totalPartNumber;

	return ret;
}


SINT32 app_partition_info(SINT32 PartNum,gpAppPartUser_t * PartInfo)
{
	SINT32 ret = 0;
	SINT32 *AppWord = (SINT32 *) &AppBuf[0];
	SINT8  *AppByte = (SINT8 *)&AppBuf[0];
	
	gpAppPart_t	 *appPartN;
	
	ret = NandAppReadSector(0,1,(UINT32)&AppBuf[0],0);
	if((AppWord[APP_HDR_TAG]!=APP_TAG)||(ret != 0))
	{
		printk ("AppHdr Read Fail, BootLoader Stop\r\n");
		return ret;
	}
	
		
	appPartN = (gpAppPart_t *)&AppBuf[24 + 16*PartNum];
	
	
	PartInfo->partNumber = PartNum;
	PartInfo->startSector = appPartN->startSector;
	PartInfo->partSize = appPartN->partSize;
	PartInfo->partType = appPartN->partType;
	PartInfo->dstAddress = appPartN->dstAddress;
	PartInfo->CurSectorId = appPartN->startSector;
	
	ret = NandAppReadSector(appPartN->startSector,1,(UINT32)&AppBuf[0],0);
	//PartInfo->CurSectorId += 1;
	if((ret == 0)&&(AppWord[APP_PART_TAG_ADD/4] == APP_PART_TAG))
	{
		printk ("PartInfo->partNumber = 0x%x,Had Partition Header!!\r\n",PartInfo->partNumber);
		PartInfo->partType = AppByte[APP_PART_IMG_TYPE_ADD];
		PartInfo->dstAddress = AppWord[APP_PART_IMG_DEST_ADDR/4];
		PartInfo->ImageSize = AppWord[APP_PART_IMG_SIZESEC_ADD/4];
		PartInfo->ImageStartSector = AppWord[APP_PART_IMG_STARTSEC_ADD/4];
	} else {

		printk ("PartInfo->partNumber = 0x%x,Have No Partition Header!!\r\n",PartInfo->partNumber);
		PartInfo->ImageSize = PartInfo->partSize;
		PartInfo->ImageStartSector = PartInfo->startSector;
	}
	
	
	return ret;
	
}

SINT32 app_read_partition(gpAppPartUser_t *PartInfo,UINT32 wLenSector,UINT32 DataBufAddr)
{
	SINT32 ret = 0;
	
	if(PartInfo->partSize < (PartInfo->CurSectorId - PartInfo->startSector) + wLenSector)
	{
		printk ("app_read_partition over size\r\n");
		return -1;	
	}
	
	if((PartInfo->partType != APP_TYPE_FASTBOOT_BIN)&&(PartInfo->partType != APP_TYPE_QUICK_IMAGE))
	{
		if(PartInfo->CurSectorId == (PartInfo->startSector))
		{
			PartInfo->CurSectorId ++;
		}
	}
	
	ret = NandAppReadSector(PartInfo->CurSectorId,wLenSector,(UINT32)DataBufAddr,1);
	if(ret != 0)
	{
		printk ("app_read_partition fail!!\r\n");
		return -1;	
	}
	
	PartInfo->CurSectorId += wLenSector;
	
	return ret; 
}

SINT32 app_write_partition(gpAppPartUser_t *PartInfo,UINT32 wLenSector,UINT32 DataBufAddr)
{
	SINT32 ret = 0;
	SINT32 *AppWord = (SINT32 *)&AppBuf[0];
	SINT8  *AppByte = (SINT8 *)&AppBuf[0];
	
	if((PartInfo->partType != APP_TYPE_FASTBOOT_BIN)&&(PartInfo->partType != APP_TYPE_QUICK_IMAGE))
	{
		if(PartInfo->CurSectorId == (PartInfo->startSector))
		{
			/*write Part Header */
			memset((UINT8 *)&AppBuf[0],0,512);
			
			AppWord[APP_PART_TAG_ADD/4] = APP_PART_TAG;
			AppByte[APP_PART_VERSION_ADD] = 0;
			AppByte[APP_PART_DRV_TYPE_ADD] = 0;
			AppWord[APP_PART_SEC_SIZE_ADD/4] = PartInfo->partSize;
			AppWord[APP_PART_IMG_CHKSUM_ADD/4] = 0;	
			AppWord[APP_PART_IMG_STARTSEC_ADD/4] = PartInfo->startSector;		
			AppWord[APP_PART_IMG_SIZESEC_ADD/4] = PartInfo->ImageSize;
			AppByte[APP_PART_IMG_TYPE_ADD] = PartInfo->partType;
			AppWord[APP_PART_IMG_DEST_ADDR/4] = PartInfo->dstAddress;
			
			ret = NandAppWriteSector(PartInfo->CurSectorId,1,(UINT32)&AppBuf[0],0);
			PartInfo->CurSectorId ++;
			if(ret != 0)
			{
				printk ("read fail!!\r\n");
				return -1;	
			}
		}
	}
	
	if(PartInfo->partSize < (PartInfo->CurSectorId - PartInfo->startSector) + wLenSector)
	{
		printk ("write over size\r\n");
		return -1;	
	}
	
	ret = NandAppWriteSector(PartInfo->CurSectorId,wLenSector,(UINT32)DataBufAddr,1);
	if(ret != 0)
	{
		printk ("read fail!!\r\n");
		return -1;	
	}
	
	PartInfo->CurSectorId += wLenSector;
	return 0; 
}

SINT32 app_write_partition_end(gpAppPartUser_t *PartInfo)
{
	NandAppFlush();	
	
	return 0;
}
	
	
module_init(app_dev_init);
module_exit(app_dev_exit);
