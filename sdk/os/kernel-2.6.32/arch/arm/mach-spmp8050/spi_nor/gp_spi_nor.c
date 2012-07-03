/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
 
/**
 * @file gp_spi_nor.c
 * @brief spi nor flash based level interface 
 * @author Daniel Huang
 */

#include <mach/module.h>
#include <mach/kernel.h>
#include <mach/bdev.h>
#include <mach/gp_spi.h>
#include <mach/gp_spi_nor.h>
#include <mach/spmp_gpio.h>
#include <mach/gp_timer.h>

int gp_spi_nor_ioctrl(struct block_device * device, fmode_t mode, unsigned int cmd, unsigned long arg);
int gp_spi_nor_open(struct block_device * device, fmode_t mode);
int gp_spi_nor_release(struct gendisk *disk, fmode_t mode);

static int gp_spi_nor_read_status(void);
static int gp_spi_nor_check_status(void);
static int gp_spi_nor_erase_sector(unsigned long arg);
static int gp_spi_nor_erase_all(void);

static int spi_nor_major = 0;
module_param(spi_nor_major, int, 0);

#define SPI_NOR_MINORS    1
#define DRIVER_NAME	"gp-spi-nor"
#define SPI_NOR_BLK_SIZE	65536

#if (SPI_NOR_BLK_SIZE == 65536)			/* 1 block = 64KB */
	#define	ERASE_BLK_COMMAND	0xD8
#elif(SPI_NOR_BLK_SIZE == 32768)			/* 1 block = 32KB */
	#define	ERASE_BLK_COMMAND	0x52
#elif(SPI_NOR_BLK_SIZE == 4096)			/* 1 sector = 4KB */
	#define	ERASE_BLK_COMMAND	0x20
#endif

#define SPI_NOR_READ_STATUS_TIMEOUT	1999	/* 2 seconds */

struct gp_spi_nor_dev {
	unsigned char xfr_mode;         				/* CPU mode or DMA mode */
	unsigned char spi_nor_id;                    		/* SPI NOR id */
	unsigned char spi_controller_id;          		/* SPI controller id */
	unsigned char users;                    			/* How many users */
	spinlock_t lock;                					/* For mutual exclusion */
	struct request_queue *queue;    			/* The device request queue */
	struct gendisk *gd;             				/* The gendisk structure */
};

static struct gp_spi_nor_dev *Devices = NULL;
signed int timerID;

static struct block_device_operations gp_spi_nor_ops = {
	.owner          		= THIS_MODULE,
	.open 			= gp_spi_nor_open,
	.release 			= gp_spi_nor_release,
	.ioctl				= gp_spi_nor_ioctrl
};

/**
 * @brief 		Spi nor flash open function. 
 * @param 	device[in]: Block device pointer.
 * @param 	mode[in]: Mode.
 * @return 	SUCCESS/ERROR_ID.
 */
int gp_spi_nor_open(struct block_device * device, fmode_t mode)
{
	struct gp_spi_nor_dev *dev = device->bd_disk->private_data;

	spin_lock(&dev->lock);
	if (!dev->users) 
		check_disk_change(device);

	dev->users++;
	spin_unlock(&dev->lock);

	return 0;	
	
}

/**
 * @brief 		Spi nor flash release function.
 * @param 	disk[in]: Block device disk.
 * @param 	mode[in]: Mode.
 * @return 	SUCCESS/ERROR_ID.
 */
int gp_spi_nor_release(struct gendisk *disk, fmode_t mode)
{
	struct gp_spi_nor_dev *dev = disk->private_data;

	spin_lock(&dev->lock);
	dev->users--;
	spin_unlock(&dev->lock);
	
	return 0;	
}

/**
 * @brief 		Spi nor flash read function.
 * @param 	dev [in]: device info.
 * @param 	sector[in]: Start sector.
 * @param 	number[in]: Number of sector will be read.
 * @param 	buf[in]: Buffer address.
 * @return 	SUCCESS/ERROR_ID.
 */
static int gp_spi_nor_read(struct gp_spi_nor_dev *dev,unsigned int sector, unsigned int number,unsigned char* buf)
{
	char cmd_addr[4];
	char ret=0;
	int sector_to_addr;	
	
	sector_to_addr = sector<<9;
	
  	cmd_addr[0] = 0x03;
    	cmd_addr[1] = ((sector_to_addr>>16) & 0xFF);
    	cmd_addr[2] = ((sector_to_addr>>8) & 0xFF);
    	cmd_addr[3] = (sector_to_addr & 0xFF);	
	
	gp_spi_cs_enable((int)dev->gd->private_data);
	gp_spi_write((int)dev->gd->private_data,cmd_addr,4);
  	if (ret != 0) {
  		gp_spi_cs_disable((int)dev->gd->private_data);
  		return ret;
  	}
	
	
	printk (KERN_NOTICE "call gp_spi_read\n");
	/*printk (KERN_NOTICE "sector = 0x%x,num = 0x%x\n",sector,number);*/
	gp_spi_read((int)dev->gd->private_data,buf,(number<<9));
	

	gp_spi_cs_disable((int)dev->gd->private_data);
	return 0;			
	
}

/**
 * @brief 		Spi nor flash write function.
 * @param 	dev [in]: device info.
 * @param 	sector[in]: Start sector.
 * @param 	number[in]: Number of sector will be writed.
 * @param 	buf[in]: Buffer address.
 * @return 	SUCCESS/ERROR_ID.
 */

static int gp_spi_nor_write(struct gp_spi_nor_dev *dev,unsigned int sector, unsigned int number, unsigned char* buf)
{
	char cmd_addr[4];
	char ret=0,clean_flag=0;
	unsigned char *write_buf = NULL;
	unsigned char *tempBuf = NULL;
	unsigned int i,write_addr=0,write_amount=0,temp_number=0;
	unsigned int block_order=0, temp_sector=0,left_write_sector_amount=0;
	
	block_order = sector/(SPI_NOR_BLK_SIZE>>9);			/* get block order */
	left_write_sector_amount = number;
	
	tempBuf = (char *)kmalloc(SPI_NOR_BLK_SIZE,GFP_KERNEL);	/* 1 block = 64Kbytes = 65536bytes */
	if(tempBuf == NULL)
	{
		return -ENOMEM;
	}
	
	while(left_write_sector_amount > 0)	/* left write sector amount */
	{		
		if(block_order!=0)
		{
			if(sector == 0)
			{
				temp_sector = 0;
			}
			else
			{
				temp_sector = sector - (block_order<<7);
			}
		}
		else
		{
			temp_sector = sector;
		}
		
		if((temp_sector+number) > (SPI_NOR_BLK_SIZE>>9))
		{
			temp_number = (SPI_NOR_BLK_SIZE>>9) - temp_sector;
			left_write_sector_amount = number - temp_number;
		}
		else
		{
			temp_number = number;
			left_write_sector_amount = 0;
		}
			
		/* read the whole block */
		gp_spi_nor_read(dev, (block_order<<7), (SPI_NOR_BLK_SIZE>>9), tempBuf);	/* 1 block = 128 sectors */
		
		for(i=0;i<SPI_NOR_BLK_SIZE;i++)
		{
			if(tempBuf[i]!=0xFF)
			{
				clean_flag = 0;
				printk(KERN_ALERT "clean_flag = 0\n");
				break;
			}
			if( i==(SPI_NOR_BLK_SIZE-1) )
			{
				clean_flag = 1;
				printk(KERN_ALERT "clean_flag = 1\n");
			}
		}
						
		if(clean_flag == 0)		/* need erase whole block */
		{		
			gp_spi_nor_erase_sector((block_order<<7));
			
			for(i=0;i<(temp_number<<9);i++)
			{
				tempBuf[(temp_sector<<9)+i] = buf[i];
			}
		}
		
		if(gp_spi_nor_check_status() != 0)
		{
			return -ETIME;
		}
	  	
	  	if(clean_flag == 0)
	  	{
	  		write_addr = (block_order<<7)<<9;
	  		write_amount = (SPI_NOR_BLK_SIZE>>9)<<1;		/* 512bytes need write 2 times */
	  		write_buf = tempBuf;
	  		/* printk(KERN_ALERT "0 write addr = 0x%x, 0 write_buf addr = 0x%x\n",write_addr,&write_buf[0]); */
	  		
	  	}
	  	else if(clean_flag == 1)
	  	{
	  		write_addr = (temp_sector+(block_order<<7))<<9;
	  		write_amount = temp_number<<1;
	  		write_buf = buf;
	  		/* printk(KERN_ALERT "1 write addr = 0x%x, 1 write amount = 0x%x,1 write_buf addr = 0x%x\n",write_addr, write_amount,&buf[0]); */
	  	}
	  	
	  	
	  	for(i=0;i<write_amount;i++)
	  	{	
			/* write Enable */
			printk (KERN_NOTICE "write Enable!! ");
			cmd_addr[0] = 0x06;
			gp_spi_cs_enable((int)dev->gd->private_data);
			gp_spi_write((int)dev->gd->private_data,cmd_addr,1);			
			gp_spi_cs_disable((int)dev->gd->private_data);						
			
			cmd_addr[0] = 0x02;
		  	cmd_addr[1] = ((write_addr>>16) & 0xFF);
		  	cmd_addr[2] = ((write_addr>>8) & 0xFF);
		  	cmd_addr[3] = (write_addr & 0xFF);
		  	
			gp_spi_cs_enable((int)dev->gd->private_data);
		  	gp_spi_write((int)dev->gd->private_data,cmd_addr,4);
		  	if (ret != 0) {
		  		gp_spi_cs_disable((int)dev->gd->private_data);
		  		return ret;
		  	}
		
		   	/* gp_spi_write((int)dev->gd->private_data,&write_buf[i<<8],256); */
		   	gp_spi_write((int)dev->gd->private_data,write_buf+(i<<8),256);			 
			gp_spi_cs_disable((int)dev->gd->private_data);
			write_addr+=256;
		}
		
		if(left_write_sector_amount>0)
		{
			block_order++;
			/* printk(KERN_ALERT "pre buf addr = 0x%x\n",&buf[0]); */
			buf += temp_number<<9;
			/* printk(KERN_ALERT "after buf addr = 0x%x\n",&buf[0]); */
			sector = 0;
			number -= temp_number;
		}
		
		if(gp_spi_nor_check_status() != 0)
		{
			return -ETIME;
		}	
	
	}/* while(left_write_sector_amount!=0) */
	

	kfree(tempBuf);	  	
	return 0;
}

/**
 * @brief 		Spi nor flash transfer function.
 * @param 	dev [in]: device info.
 * @param 	sector [in]: start sector.
 * @param 	nsect [in]: Number of sector will be writed.
 * @param 	buffer [in]: Buffer address.
 * @param 	write [in]: will be 1 if this is a write function.
 * @return 	None.
 */
static void gp_spi_nor_transfer(struct gp_spi_nor_dev *dev, unsigned long sector, unsigned long nsect, char *buffer, int write)
{
	if (write)
	{
		/* printk(KERN_ALERT "write number= 0x%x\n",(unsigned int)nsect); */
		gp_spi_nor_write(dev, sector, nsect, buffer);
	}
	else
	{
		/* printk(KERN_ALERT "read number= 0x%x\n",(unsigned int)nsect); */
		gp_spi_nor_read(dev, sector, nsect, buffer);
	}
}

/**
 * @brief 		Spi nor flash transfer bio function.
 * @param 	dev [in]: device info.
 * @param 	bio [in]: device bio info.
 * @return 	SUCCESS/ERROR_ID.
 */
static int gp_spi_nor_xfer_bio(struct gp_spi_nor_dev *dev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;

	/* Do each segment independently. */
	bio_for_each_segment(bvec, bio, i) {
		char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);
		gp_spi_nor_transfer(dev, sector, bio_cur_bytes(bio) >> 9, buffer, bio_data_dir(bio) == WRITE);
		sector += bio_cur_bytes(bio) >> 9;
		__bio_kunmap_atomic(bio, KM_USER0);
	}
	return 0; /* Always "succeed" */
}

/*
 * @brief		Spi nor flash erase sector function.
 * @param 	arg [in]: erase address (sector)
 * @return 	SUCCESS/ERROR_ID.
 */
static int gp_spi_nor_erase_sector(unsigned long arg)
{
	struct gp_spi_nor_dev *dev = Devices;
	char cmd_addr[4];
	
	if(gp_spi_nor_check_status() != 0)
	{
		return -ETIME;
	}
	
	/* write Enable */
	printk (KERN_NOTICE "erase sector write Enable\n");
	cmd_addr[0] = 0x06;
	gp_spi_cs_enable((int)dev->gd->private_data);
	gp_spi_write((int)dev->gd->private_data,cmd_addr,1);			
	gp_spi_cs_disable((int)dev->gd->private_data);
	
	cmd_addr[0] = ERASE_BLK_COMMAND;
  	cmd_addr[1] = (((arg<<9)>>16) & 0xFF);
  	cmd_addr[2] = (((arg<<9)>>8) & 0xFF);
  	cmd_addr[3] = ((arg<<9) & 0xFF);
  	
	gp_spi_cs_enable((int)dev->gd->private_data);
	printk (KERN_NOTICE "erase sector!\n");
	gp_spi_write((int)dev->gd->private_data,cmd_addr,4);
	gp_spi_cs_disable((int)dev->gd->private_data);
	
	if(gp_spi_nor_check_status() != 0)
	{
		return -ETIME;
	}
	
	return 0;
}

/*
 * @brief		Spi nor flash erase whole chip function.
 * @return 	SUCCESS/ERROR_ID.
 */
static int gp_spi_nor_erase_all(void)
{
	struct gp_spi_nor_dev *dev = Devices;
	char cmd_addr[4];
	
	if(gp_spi_nor_check_status() != 0)
	{
		return -ETIME;
	}
	
	/* write Enable */
	printk (KERN_NOTICE "write Enable\n");
	cmd_addr[0] = 0x06;
	gp_spi_cs_enable((int)dev->gd->private_data);
	gp_spi_write((int)dev->gd->private_data,cmd_addr,1);			
	gp_spi_cs_disable((int)dev->gd->private_data);
	
	/* Chip erase */
	cmd_addr[0] = 0x60;			  	
	gp_spi_cs_enable((int)dev->gd->private_data);
	printk (KERN_NOTICE "erase whole chip!\n");
	gp_spi_write((int)dev->gd->private_data,cmd_addr,1);
	gp_spi_cs_disable((int)dev->gd->private_data);
	
	if(gp_spi_nor_check_status() != 0)
	{
		return -ETIME;
	}
	
	return 0;
}

/*
 * @brief		Spi nor flash make request function.
 * @param 	q [in]: request queue.
* @param 	bio [in]: disk bio info. 
 * @return 	SUCCESS/ERROR_ID.
 */
static int gp_spi_nor_make_request(struct request_queue *q, struct bio *bio)
{
	struct gp_spi_nor_dev *dev = q->queuedata;
	int status;
	
	status = gp_spi_nor_xfer_bio(dev, bio);
	bio_endio(bio, status);
	return 0;
}

/*
 * @brief		Spi nor flash set-up internal device.
 * @param 	dev [in]: device info.
 * @return 	None.
 */
static void gp_spi_nor_setup_device(struct gp_spi_nor_dev *dev)
{
	/*
	 * Get some memory.
	 */
	memset (dev, 0, sizeof (struct gp_spi_nor_dev));
	dev->xfr_mode = 0;
	dev->spi_controller_id = 0;
	
	spin_lock_init(&dev->lock);
	
	/*
	 * The I/O queue, make_request function.
	 */

	dev->queue = blk_alloc_queue(GFP_KERNEL);
	if (dev->queue == NULL)
		return;
	blk_queue_make_request(dev->queue, gp_spi_nor_make_request);

	blk_queue_logical_block_size(dev->queue, 512);
	dev->queue->queuedata = dev;

	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(SPI_NOR_MINORS);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		return;
	}
	dev->gd->major = spi_nor_major;
	dev->gd->first_minor = 0;
	dev->gd->fops = &gp_spi_nor_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = (void*)gp_spi_request(0);
	snprintf (dev->gd->disk_name, 32, "spi_nor");
	set_capacity(dev->gd,8192 ); /* 128M */
	add_disk(dev->gd);
	return;
}

/*
 * @brief		Spi nor flash initial function.
 * @return 	SUCCESS/ERROR_ID. 
 */
static int __init gp_spi_nor_init(void)
{

    unsigned int apbHz;
	struct clk *apbClk;
	unsigned int temp,i;
	
	/* apply timer service, get the apb clock value */
	apbClk = clk_get(NULL,"clk_arm_apb");
	if(apbClk){
		apbHz = clk_get_rate(apbClk);
		printk (KERN_NOTICE "apbHz = %d\n",apbHz);
		clk_put(apbClk);
	}
	else{
		apbHz = 21000000;
		printk("apb use default clock 21M\n");
	}
	
	temp = apbHz/1000 - 1 ;
	
	timerID = 0;
	for (i=0;i<5 && timerID==0;i++) {
		timerID = gp_tc_request(i,"spi_nor");
	}
	/* setup timer */
	gp_tc_set_prescale(timerID,temp);
	gp_tc_set_operation_mode(timerID,1); /* period timer mode */
	gp_tc_set_count_mode(timerID,1);
	gp_tc_enable_int(timerID,1);
			
	spi_nor_major = register_blkdev(spi_nor_major, "spi_nor");
	if (spi_nor_major <= 0) {
		printk(KERN_WARNING "gp_spi_nor: unable to get major number\n");
		return -EBUSY;
	}
	
	Devices = kmalloc(sizeof (struct gp_spi_nor_dev), GFP_KERNEL);
	if (Devices == NULL)
		return -EBUSY;
	
	gp_spi_nor_setup_device(Devices);
	
	printk (KERN_NOTICE "SPI NOR module init\n");
	
	return 0;
}

/*
 * @brief		Spi nor flash exit function.
 */
static void __exit gp_spi_nor_exit(void)
{

}

/*
 * @brief		Spi nor flash check status function.
 * @return 	SUCCESS/ERROR_ID. 
 */
static int gp_spi_nor_check_status(void)
{
	signed int iflag;

	/* start timer */
	gp_tc_set_load(timerID,65536-SPI_NOR_READ_STATUS_TIMEOUT-1);
	gp_tc_enable(timerID);
	
	while((gp_spi_nor_read_status() & 0x01) != 0) 
  	{
		/* printk (KERN_NOTICE "erase before SPI_Flash_ReadStatus not ready!\n"); */
		gp_tc_get_int_state(timerID,&iflag);
		if (iflag & 0x1) {
			printk (KERN_NOTICE "check status:Timer...time-out!\n");
			gp_tc_set_int_state(timerID,0);
			gp_tc_disable(timerID);
			return -ETIME;
		}
  	}
  	/* end timer */
  	gp_tc_disable(timerID);

  	return 0;	
}

/*
 * @brief		Spi nor flash read status function.
 * @return 	SUCCESS/ERROR_ID. 
 */
static int gp_spi_nor_read_status(void)
{
	struct gp_spi_nor_dev *dev = Devices;
	
	char cmd_addr[4];
	char buffer[4];
	
	cmd_addr[0] = 0x05;
	
	gp_spi_cs_enable((int)dev->gd->private_data);
	gp_spi_write((int)dev->gd->private_data,cmd_addr,1);
	gp_spi_read((int)dev->gd->private_data,buffer,1);			
	gp_spi_cs_disable((int)dev->gd->private_data);
  	
	return buffer[0];
}

/*
 * @brief		Spi nor flash IO control function.
 * @param 	dev [in]: device info.
 * @param 	mode[in]: Mode.
 * @param 	cmd[in]: commands for different IO control.
 * @param 	arg[in]: arguments for commands.
 * @return 	SUCCESS/ERROR_ID.
 */
int gp_spi_nor_ioctrl (struct block_device * device, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	//struct gp_spi_nor_dev *dev = device->bd_disk->private_data;
	struct gp_spi_nor_dev *dev = Devices;
	
	int ret = 0;
	char cmd_addr[4];
	char buffer[4];
	
	switch(cmd)
	{
		case SPI_NOR_IOCTL_READ_ID:
		{
			if(dev->gd->private_data == NULL){
				printk (KERN_NOTICE "SPI_NOR_IOCTL_READ_ID cmd fail\n");
				ret = -EFAULT;
			}
			else
			{
				printk (KERN_NOTICE "SPI_NOR_IOCTL_READ_ID cmd = %d\n",cmd);
				cmd_addr[0] = 0x9f;
				gp_spi_cs_enable((int)dev->gd->private_data);
				
				gp_spi_write((int)dev->gd->private_data,cmd_addr,1);			
				
				gp_spi_read((int)dev->gd->private_data,buffer,3);
				
				printk (KERN_NOTICE "read ID==%x,%x,%x\n",buffer[0],buffer[1],buffer[2]);
				
				gp_spi_cs_disable((int)dev->gd->private_data);
			}
			break;		
		}
		
		case SPI_NOR_IOCTL_ERASE_SECTOR:
		{
			if(dev->gd->private_data == NULL){
				ret = -EFAULT;
			}
			else
			{
				ret = gp_spi_nor_erase_sector(arg);
				if(ret != 0)
				{
					return ret;
				}
			}
			break;		
		} 
		
		case SPI_NOR_IOCTL_ERASE_CHIP:
		{
			printk (KERN_NOTICE "SPI_NOR_IOCTL_ERASE_CHIP cmd = %d\n",cmd);
			if(dev->gd->private_data == NULL){
				ret = -EFAULT;
			}
			else
			{
				ret = gp_spi_nor_erase_all();
				if(ret != 0)
				{
					return ret;
				}
			}
			break;		
		} 	
		
		default:
		{
			break;
		}
	
	}
	return ret;
}

module_init(gp_spi_nor_init);
module_exit(gp_spi_nor_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP SPI NOR Driver");
MODULE_LICENSE_GP;

