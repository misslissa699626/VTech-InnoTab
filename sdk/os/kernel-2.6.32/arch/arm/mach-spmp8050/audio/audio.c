#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/delay.h>
//#include <linux/soundcard.h>

#include <asm/io.h>
#include <asm/system.h>         /* cli(), *_flags */
#include <asm/uaccess.h>        /* copy_*_user */

#include <mach/hardware.h>
#include <mach/regs-interrupt.h>
#include <mach/regs-iis.h>
#include <mach/regs-saradc.h>
#include <mach/regs-scu.h>
#include <mach/regs-dma.h>
#include <mach/audio/audio_util.h>
#include <mach/audio/soundcard.h>
#include <mach/irqs.h>

#include "i2s.h"

MODULE_LICENSE("GPL");


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef HAL_READ_UINT32
#define HAL_READ_UINT32( _register_, _value_ ) \
        ((_value_) = *((volatile unsigned int *)(_register_)))
#endif

#ifndef HAL_WRITE_UINT32
#define HAL_WRITE_UINT32( _register_, _value_ ) \
        (*((volatile unsigned int *)(_register_)) = (_value_))
#endif

#define DUMP_REG

#if 1
#define DEBUG0(fmt,args...) printk(fmt,##args)
#else
#define DEBUG0(fmt,args...)
#endif

#if 0
#define DEBUG1(fmt,args...) printk(fmt,##args)
#else
#define DEBUG1(fmt,args...)
#endif

#define DEVICE_NAME "dsp"

#define DEFAULT_BUF_SIZE 32*1024
#define DEFAULT_BLK_SIZE 8*1024
#define	DMA_BUFFER_A		0
#define	DMA_BUFFER_B		1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
static int dsp_major = 0;
static int dsp_minor = 0;
static int dsp_nr_devs = 1;

struct dsp_buf_s {
	char *mem_addr;
	char *base;
	unsigned int bufSize;
	unsigned int blkSize;
	unsigned int inIdx;
	unsigned int outIdx;
	int dataFlag; /* dataFlag > 0 := inIdx > outIdx */
}dsp_buf_t;

struct dma_info_s {
	char *buf_A;
	char *buf_B;
	unsigned int currTx;
	unsigned int currRx;
	void __iomem *iobase;
}dma_info_t;

struct dsp_dev_s {
	unsigned int isTxStart;
	unsigned int isRxStart;
	unsigned int nonBlock;
	unsigned int caps;
	unsigned int error;	
	int count;
	struct dsp_buf_s buf;
	struct dma_info_s dma;
	struct semaphore sem;
	struct cdev cdev;
	void __iomem *iisRxBase;
	void __iomem *iisTxBase;
	bool done;
	wait_queue_head_t done_wait;
}dsp_dev_t;

struct dsp_dev_s *dsp_devices;
static struct class *dsp_class;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                            F U N C T I O N                             *
 **************************************************************************/
static void
dsp_capture_avil(
	struct file *filp,
	int *count
)
{
	struct dsp_dev_s *dev = filp->private_data;
	struct dsp_buf_s *buf = &dev->buf;
	int inIdx = buf->inIdx;
	int outIdx = buf->outIdx;
	int dataFlag = buf->dataFlag;
	//DEBUG0("inIdx %d, outIdx %d, dataFlag %d\n",
	//	inIdx, outIdx, dataFlag);
	if (inIdx == outIdx) {
		if (dataFlag > 0) {
			*count = buf->bufSize;
		}
		else {
			*count = 0;
		}
		return;
	}

	if (inIdx < outIdx) {
		*count = buf->bufSize + inIdx - outIdx;
	}
	else {
		*count = inIdx - outIdx;
	}
}


static void
dsp_free_avil(
	struct file *filp,
	int *count
)
{
	struct dsp_dev_s *dev = filp->private_data;
	struct dsp_buf_s *buf = &dev->buf;
	int dataCnt;

	dsp_capture_avil(filp, &dataCnt);
	*count = buf->bufSize - dataCnt;
}

static int
dsp_ioctl_ispace_get(
	struct file *filp,
	audio_buf_info *bi
)
{
	int ret = -1;
	if (!bi)
	{
		return -EINVAL;
	}

	if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
	{
		struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
		struct dsp_buf_s *buf = &dev->buf;
		int dataCount;
		
		dsp_capture_avil(filp, &dataCount);
		
		bi->fragments = dataCount/(buf->blkSize << 1) + 1;
		bi->fragstotal = buf->bufSize/buf->blkSize + 1;
		bi->fragsize = buf->blkSize << 1;
		bi->bytes = dataCount;
		ret = 0;
	}
	return ret;
}

static int
dsp_ioctl_ospace_get(
	struct file *filp,
	audio_buf_info *bi
)
{
	int ret = -1;
	if (!bi)
	{
		return -EFAULT;
	}
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
		struct dsp_buf_s *buf = &dev->buf;
		int dataCount;

		dsp_free_avil(filp, &dataCount);

		bi->fragments = dataCount/buf->blkSize;
		bi->fragstotal = buf->bufSize/buf->blkSize;
		bi->fragsize = buf->blkSize;
		bi->bytes = dataCount;
		ret = 0;
	}
	return ret;
}

static int
dsp_ioctl_fmt_get(
	int *fmt
)
{
	if (!fmt)
	{
		return -EINVAL;
	}
	*fmt = AFMT_S16_LE | AFMT_S16_BE;
	return 0;
}

static int
dsp_ioctl_trigger_set(
	struct file *filp,
	int trig
)
{
	struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (trig & PCM_ENABLE_OUTPUT) {
			if (!dev->isTxStart)
			{
				dev->isTxStart = 1;
				halI2sTxFIFOClear();
				halI2sTxIntEnable();
				halI2sTxEnable();
			}
		}
		else
		{
			if (dev->isTxStart)
			{
				dev->isTxStart = 0;
				halI2sTxFIFOClear();
				halI2sTxIntDisable();
				halI2sTxDisable();
			}
		}
	}
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		if (trig & PCM_ENABLE_INPUT) {
			if (!dev->isRxStart)
			{
				dev->isRxStart = 1;
				halI2sRxFIFOClear();
				halI2sRxIntEnable();
				halI2sRxEnable();
			}
		}
		else
		{
			if (dev->isRxStart)
			{
				dev->isRxStart = 0;
				halI2sRxFIFOClear();
				halI2sRxIntDisable();
				halI2sRxDisable();
			}
		}
	}
	return 0;
}

static int
dsp_ioctl_channel_set(
	struct file *filp,
	int num
)
{
	int ret = -1;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		if (num == 1 || num == 2) {
			halI2sTxChlSet(num);
			ret = 0;
		}
	}
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
	{
		if (num == 1 || num == 2) {
			halI2sRxChlSet(num);
			ret = 0;
		}
	}

	return ret;
}


static int
dsp_ioctl_frequency_set(
	struct file *filp,
	int frequency
)
{
	int ret = -1;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		ret = audio_FREQ_set(frequency, 1);
	}
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		ret = audio_FREQ_set(frequency, 0);
	}
	return ret;
}

static int
dsp_ioctl_fmt_set(
	struct file *filp,
	int fmt
)
{
	int ret = -1;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		if (fmt == AFMT_S16_LE)
		{
			halI2sTxFrameOrderSet(1);
			ret = 0;
		}
		else if (fmt == AFMT_S16_BE)
		{
			halI2sTxFrameOrderSet(0);
			ret = 0;
		}
		// other not support
	}
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
	{
		if (fmt == AFMT_S16_LE)
		{
			halI2sRxFrameOrderSet(1);
			ret = 0;
		}
		else if (fmt == AFMT_S16_BE)
		{
			halI2sRxFrameOrderSet(0);
			ret = 0;
		}
		// other not support
	}
	return ret;
}

static int
dsp_ioctl_odelay_get(
	struct file *filp,
	int *delay
)
{
	int ret = -1;
	if (!delay)
	{
		return -EFAULT;
	}
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		struct dsp_dev_s *dev = filp->private_data;
		struct dsp_buf_s *buf = &dev->buf;
		dsp_capture_avil(filp, delay);
		if (*delay > buf->bufSize)
		{
			*delay = 0;
		}
		ret = 0;
	}
	return ret;
}

static int
dsp_ioctl_skip(
	struct file *filp
)
{
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		struct dsp_dev_s *dev = filp->private_data;
		struct dsp_buf_s *buf = &dev->buf;
		if (dev->isTxStart)
		{
			halI2sTxFIFOClear();
			halI2sTxIntDisable();
			halI2sTxDisable();
			dev->isTxStart = 0;
			udelay(1000);
		}
		buf->dataFlag = 0;
		buf->inIdx = buf->outIdx;
	}
	return 0;
}

static int
dsp_ioctl_sync(
	struct file *filp
)
{
	struct dsp_dev_s *dev = filp->private_data;
	struct dsp_buf_s *buf = &dev->buf;
	int flags = filp->f_flags & O_ACCMODE;
	while (flags == O_WRONLY) {
		if (dev->isTxStart == 0) {
			break;
		}
		if (buf->dataFlag <= 0 && buf->inIdx < buf->outIdx + buf->blkSize) {
			halI2sTxFIFOClear();
			halI2sTxIntDisable();
			halI2sTxDisable();
			dev->isTxStart = 0;
			break;
		}
		udelay(1000);
	}
	return 0;
}

static int
dsp_ioctl_slience(
	struct file *filp
)
{
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		struct dsp_dev_s *dev = filp->private_data;
		struct dsp_buf_s *buf = &dev->buf;
		memset(buf->base, 0, buf->bufSize);
	}
	return 0;
}

static int
dsp_ioctl_fragment_set(
	struct file *filp,
	int fragment
)
{
	struct dsp_dev_s *dev = filp->private_data;
	struct dsp_buf_s *buf = &dev->buf;
	struct dma_info_s *dma = &dev->dma;
	int blkSize, blkNum, bufSize;
	char *base, *old_base, *mem_addr;
	
	/* parse */
	blkNum = fragment >> 16;
	if (blkNum < 4)
	{
		return -EDOM;
	}
	
	blkSize = 1 << (fragment & 0xFFFF);
	bufSize = blkNum * blkSize;
	if (blkSize == buf->blkSize && bufSize == buf->bufSize)
	{
		DEBUG1("same framgment parameter\n");
		return 0;
	}
	
	/* malloc base buffer */
	old_base = buf->base;
	if (bufSize == buf->bufSize){
		base = old_base;
	}
	else{
		mem_addr = kmalloc(bufSize + blkSize + 1024, GFP_KERNEL);
		if (mem_addr == NULL)
		{
			DEBUG0("kmalloc fail\n");
			return -ENOMEM;
		}
		base = (char *)((((unsigned int)mem_addr) + (1024 - 1)) & ~(1024 - 1)); 
		memset(base, 0, bufSize + blkSize);
	}
	
	/* stop and clear */
	HAL_WRITE_UINT32(dma->iobase + DMAX_CRX, DMA_OFF);
	#if 0
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		//if (dev->isTxStart){				
			//halI2sTxFIFOClear();
			//halI2sTxIntDisable();
			halI2sTxDisable();
			dev->isTxStart = 0;
			DEBUG0("stop playback\n");
		//}
	}
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		//if (dev->isRxStart){				
			//halI2sRxFIFOClear();
			//halI2sRxIntDisable();
			halI2sRxDisable();
			dev->isRxStart = 0;
			DEBUG0("stop record\n");
		//}
	}
	#endif
	
	/* init buf */
	buf->bufSize = bufSize;
	buf->blkSize = blkSize;
	buf->base = base;
	buf->inIdx = 0;
	buf->outIdx = 0;
	buf->dataFlag = 0;
	
	/* init dma */
	dma->currTx = DMA_BUFFER_A;
	dma->currRx = DMA_BUFFER_A;
	dma->buf_A = buf->base;
	dma->buf_B = buf->base + buf->blkSize;
	
	/* setup buffer A/B */
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		/* APBDMA */
		/* setup buffer A */
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXA), 
			(unsigned int )virt_to_phys(dma->buf_A));
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXA), 
			(unsigned int )virt_to_phys(dma->buf_A + buf->blkSize - 4));

		/* setup buffer B */
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB), 
			(unsigned int )virt_to_phys(dma->buf_B));
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB), 
			(unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));
		
		/* setup APB access address */
		HAL_WRITE_UINT32(dma->iobase + DMAX_APB_SAX, 0x93012004);

		/* DMA channel settings */
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX, (DMA_M2P|DMA_REQ|DMA_FIX|DMA_DOUBLE_BUF|DMA_32BIT_BURST|DMA_IRQON|DMA_ON));
		DEBUG1("setup playback buffer A/B\n");

	}
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		unsigned int  iiscr, dmaar;

		/* I2S */
		iiscr = IISRX_FIRSTFRAME_L|IISRX_FRAME_POLARITY_L|IISRX_EDGEMODE_FALLING
			|IISRX_SENDMODE_MSB|IISRX_MODE_ALIGN_RIGHT|IISRX_VDMODE_16|IISRX_FSMODE_16
			|IISRX_MODE_I2S|IISRX_SLAVE_MODE|IISRX_IRT_POLARITY_HIGH|IISRX_CLRFIFO|IISRX_MERGE|0x200000;

		iiscr = iiscr & (~IISRX_MASTER_MODE);
		//writel(iiscr, IISRX_ISCR_TMP);
		writel(iiscr,  dev->iisRxBase);
		/* APBDMA */
		/* setup buffer A */
		writel((unsigned int )virt_to_phys(dma->buf_A), 
			(unsigned int )(dma->iobase + DMAX_AHB_SAXA + 1*4));
		writel((unsigned int )virt_to_phys(dma->buf_A + buf->blkSize - 4), 
			(unsigned int )(dma->iobase + DMAX_AHB_EAXA + 1*4));

		/* setup buffer B */
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB + 1*4), 
			(unsigned int )virt_to_phys(dma->buf_B));
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB + 1*4), 
			(unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));

		/* setup APB access address */
		HAL_WRITE_UINT32(dma->iobase + DMAX_APB_SAX + 1*4, 0x9301D004);
		
		/* DMA channel settings */
		dmaar = DMA_P2M|DMA_REQ|DMA_FIX|DMA_DOUBLE_BUF|DMA_32BIT|DMA_IRQON|DMA_ON;
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1*4, dmaar);
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1*4, dmaar);
		
	}
		
	/* free base buffer*/
	if (base != old_base){
		if (buf->mem_addr != NULL){
			kfree(buf->mem_addr);
		}
		buf->mem_addr = mem_addr;
	}
	return 0;
}

static char *dsp_dma_request_data(struct dma_info_s *dma, struct dsp_buf_s *buf)
{
	int data_size, blk_size = buf->blkSize;
	char *out_buf, *end_ptr = buf->base + buf->bufSize;

	// get data size in user data buffer
	data_size = buf->inIdx - buf->outIdx;
	if (data_size <= 0 && buf->dataFlag > 0) //  
	{
		data_size += buf->bufSize;
	}

	if (data_size >= blk_size){
		if ((dma->currTx == DMA_BUFFER_A && dma->buf_A != end_ptr) 
		 || (dma->currTx == DMA_BUFFER_B && dma->buf_B != end_ptr)) {
			// play end one block of user data
			data_size -= blk_size;
			buf->outIdx += blk_size;
			if(buf->outIdx >= buf->bufSize) {
				buf->dataFlag = 0;
				buf->outIdx = 0;
				DEBUG1("dataFlag = %d\n", buf->dataFlag);
			}
			if (data_size >= blk_size * 2){
				out_buf = buf->base + buf->outIdx + blk_size;
				if (out_buf >= end_ptr){
					out_buf -= buf->bufSize;
				}
			}
			else {
				// if not enough, play white data in left block buffer
				out_buf = end_ptr;
				DEBUG1("1.not enough data, play white data, i %d, o %d\n", 
					buf->inIdx, buf->outIdx);
			}
		}
		else {
			// play end one block of white data
			// come back from white data to user data 
			if (dma->buf_A == end_ptr && dma->buf_B == end_ptr) {
				// first back from white data to user data 
				out_buf = buf->base + buf->outIdx;
				if (out_buf >= end_ptr){
					out_buf -= buf->bufSize;
				}
				DEBUG1("first back from white data to user data, i %d, o %d\n", 
					buf->inIdx, buf->outIdx);
			}
			else {
				// second back from white data to user data 
				if (data_size >= blk_size * 2){
					out_buf = buf->base + buf->outIdx + blk_size;
					if (out_buf >= end_ptr){
						out_buf -= buf->bufSize;
					}
					DEBUG1("second back from white data to user data, i %d, o %d\n", 
					buf->inIdx, buf->outIdx);
				}
				else {
					// if not enough, play white data in left block buffer
					out_buf = end_ptr;
					DEBUG1("2.not enough data, play white data, i %d, o %d\n", 
						buf->inIdx, buf->outIdx);
				}
			}
		}
	}
	else {
		// if no enough, play white data in left block buffer
		out_buf = end_ptr;
		DEBUG1("3.not enough data, play white data, i %d, o %d\n", 
			buf->inIdx, buf->outIdx);
	}
	
	return out_buf;
}

static irqreturn_t
dsp_dma_irq_handle(
	int irq,
	void *dev_id
)
{
	struct dsp_dev_s *dev = (struct dsp_dev_s *)dev_id;
	struct dsp_buf_s *buf = &dev->buf;
	struct dma_info_s *dma = &dev->dma;
	char *out_buf;

	if (irq == IRQ_APBDMA_A_CH0) {
		writel(1, dma->iobase + DMAX_IRQ_STATUS);
		
#if 1 //  if not enough data, used white data to replace, modified by zhou lu, 2010.12.15
		out_buf = dsp_dma_request_data(dma, buf);

		if(dma->currTx == DMA_BUFFER_A) {
			dma->buf_A = out_buf;

			DEBUG1("buffer a %p, %p\n", dma->buf_A, (char *)virt_to_phys(dma->buf_A));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXA), (unsigned int )virt_to_phys(dma->buf_A));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXA), (unsigned int )virt_to_phys(dma->buf_A + buf->blkSize- 4));

			dma->currTx = DMA_BUFFER_B;
		}
		else {
			dma->buf_B = out_buf;

			DEBUG1("buffer b %p, %p\n", dma->buf_B, (char *)virt_to_phys(dma->buf_B));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB), (unsigned int )virt_to_phys(dma->buf_B));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB), (unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));

			dma->currTx = DMA_BUFFER_A;
		}
#else	
		if(dma->currTx == DMA_BUFFER_A) {
			dma->buf_A += buf->blkSize * 2;

			if (dma->buf_A >= buf->base + buf->bufSize){
				dma->buf_A -= buf->bufSize;
			}

			DEBUG1("buffer a %p, %p\n", dma->buf_A, (char *)virt_to_phys(dma->buf_A));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXA), (unsigned int )virt_to_phys(dma->buf_A));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXA), (unsigned int )virt_to_phys(dma->buf_A + buf->blkSize - 4));

			dma->currTx = DMA_BUFFER_B;
		}
		else {
			dma->buf_B += buf->blkSize * 2;

			if (dma->buf_B >= buf->base + buf->bufSize){
				dma->buf_B -= buf->bufSize;
			}

			DEBUG1("buffer b %p, %p\n", dma->buf_B, (char *)virt_to_phys(dma->buf_B));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB), (unsigned int )virt_to_phys(dma->buf_B));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB), (unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));

			dma->currTx = DMA_BUFFER_A;
		}
		buf->outIdx += buf->blkSize;
		if(buf->outIdx >= buf->bufSize) {
			buf->dataFlag -= 1;
			DEBUG1("dataFlag = %d\n", buf->dataFlag);
			buf->outIdx = 0;
		}
	
#endif		

	}
	else if (irq == IRQ_APBDMA_A_CH1) {
		writel(1 << 1, dma->iobase + DMAX_IRQ_STATUS);

		if(dma->currRx == DMA_BUFFER_A) {
			dma->buf_A += buf->blkSize * 2;

			if (dma->buf_A >= buf->base + buf->bufSize){
				dma->buf_A -= buf->bufSize;
			}

			DEBUG1("buffer a %p, %p\n", dma->buf_A, (char *)virt_to_phys(dma->buf_A));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXA + 1*4), (unsigned int )virt_to_phys(dma->buf_A));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXA + 1*4), (unsigned int )virt_to_phys(dma->buf_A + buf->blkSize - 4));

			dma->currRx = DMA_BUFFER_B;
		}
		else {
			dma->buf_B += buf->blkSize * 2;

			if (dma->buf_B >= buf->base + buf->bufSize){
				dma->buf_B -= buf->bufSize;
			}

			DEBUG1("buffer b %p, %p\n", dma->buf_B, (char *)virt_to_phys(dma->buf_B));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB + 1*4), (unsigned int )virt_to_phys(dma->buf_B));
			HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB + 1*4), (unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));

			dma->currRx = DMA_BUFFER_A;
		}

		buf->inIdx += buf->blkSize;
		if(buf->inIdx >= buf->bufSize) {
			buf->dataFlag += 1;
			buf->inIdx = 0;
		}
		DEBUG1("record: inIdx %d, outIdx %d, dataFlag = %d\n", 
			buf->inIdx/buf->blkSize, buf->outIdx/buf->blkSize, buf->dataFlag);
	}
	DEBUG1("input index: 0x%x\n", buf->inIdx);

	dev->done = true;
	wake_up(&dev->done_wait);
	
	return IRQ_HANDLED;
}


/*
 * The read() implementation
 */
static ssize_t
dsp_dev_read(
	struct file *filp,
	char __user *buffer,
	size_t count,
	loff_t *ppos
)
{

	struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
	struct dsp_buf_s *buf = &dev->buf;
	int dataCnt;

	if (!filp || !buffer)
	{
		return -EINVAL;
	}

	if (!dev->isRxStart) {
		halI2sRxFIFOClear();
		halI2sRxIntEnable();
		halI2sRxEnable();

		dev->isRxStart = 1;
	}

	dsp_capture_avil(filp, &dataCnt);
	DEBUG1("availiable data: 0x%x\n", dataCnt);

	while(dataCnt < count) {
		if(dev->nonBlock){
			return -EFAULT;
		}

		DEBUG1("No enough data ...\n");
#if 1
		dev->done = false;
		wait_event_timeout(dev->done_wait, dev->done, HZ);
#else
		mdelay(10);
#endif
		dsp_capture_avil(filp, &dataCnt);
	}

	if(buf->bufSize - buf->outIdx < count) {
		if (copy_to_user(buffer, buf->base + buf->outIdx, buf->bufSize - buf->outIdx)) {
			DEBUG0("read failed\n");
			return -EFAULT;
		}
		if (copy_to_user(buffer + (buf->bufSize - buf->outIdx), buf->base, count - (buf->bufSize - buf->outIdx))) {
			DEBUG0("read failed\n");
			return -EFAULT;
		}

	}
	else {
		if (copy_to_user(buffer, buf->base + buf->outIdx, count)) {
			DEBUG0("read failed\n");
			return -EFAULT;
		}

	}
	buf->outIdx += count;

#if 0
	buf->outIdx = buf->outIdx % buf->bufSize;
#else
	if (buf->outIdx >= buf->bufSize){
		buf->dataFlag -= 1;
		buf->outIdx -= buf->bufSize;
	}
#endif

	return count;
}


/*
 * The write() implementation
 */
static ssize_t
dsp_dev_write(
	struct file *filp,
	const char __user *buffer,
	size_t count,
	loff_t *ppos
)
{
	struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
	struct dsp_buf_s *buf = &dev->buf;

	int dataCnt = 0;

	if (!filp || !buffer)
	{
		return -EINVAL;
	}

	dsp_free_avil(filp, &dataCnt);
	DEBUG1("availiable data: 0x%x\n", dataCnt);

	while(dataCnt < count) {
		if (!dev->isTxStart) {
			dsp_capture_avil(filp, &dataCnt);
			if (dataCnt >= buf->blkSize * 2){
				DEBUG1("playback begin\n");
				dev->isTxStart = 1;
				halI2sTxFIFOClear();
				halI2sTxIntEnable();
				halI2sTxEnable();
			}
		}
		if(dev->nonBlock){
			return -ENOSPC;
		}
	
		DEBUG1("dma buffer full, wait free\n");
#if 1
		dev->done = false;
		wait_event_timeout(dev->done_wait, dev->done, HZ);
#else
		msleep(10);
#endif
		dsp_free_avil(filp, &dataCnt);
	}

	if(buf->bufSize - buf->inIdx < count) {
		if (copy_from_user(buf->base + buf->inIdx, buffer, buf->bufSize - buf->inIdx)) {
			DEBUG0("write failed\n");
			return -EFAULT;
		}
		if (copy_from_user(buf->base, buffer + (buf->bufSize - buf->inIdx), count - (buf->bufSize - buf->inIdx))) {
			DEBUG0("write failed\n");
			return -EFAULT;
		}

	}
	else {
		if (copy_from_user(buf->base + buf->inIdx, buffer, count)) {
			DEBUG0("write failed\n");
			return -EFAULT;
		}

	}
	buf->inIdx += count;
	if (buf->inIdx >= buf->bufSize){
		buf->dataFlag = 1;
		buf->inIdx -= buf->bufSize;
	}
	DEBUG1("write: inIdx %d, outIdx %d, dataFlag = %d\n", 
		buf->inIdx/buf->blkSize, buf->outIdx/buf->blkSize, buf->dataFlag);

	if (!dev->isTxStart) {
		dsp_capture_avil(filp, &dataCnt);
		if (dataCnt >= buf->blkSize * 2){
			dev->isTxStart = 1;
			DEBUG1("playback begin\n");
			halI2sTxFIFOClear();
			halI2sTxIntEnable();
			halI2sTxEnable();
		}
	}

	return count;
}
/*
 * The release() implementation
 */
static int
dsp_dev_release(
	struct inode *inode,
	struct file *filp
)
{
	struct dsp_dev_s *dev;
	struct dsp_buf_s *buf;
	struct dma_info_s *dma;
	
	if (!inode || !filp) {
		return -EINVAL;
	}
	
 	dev = filp->private_data;
 	buf = &dev->buf;
 	dma = &dev->dma;
	
	if (dev->count <= 0){
		return 0;
	}

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		halI2sTxFIFOClear();
		halI2sTxIntDisable();
		halI2sTxDisable();

		dev->isTxStart = 0;

		if (dma->iobase != NULL){
			HAL_WRITE_UINT32(dma->iobase + DMAX_CRX, DMA_OFF);
		}
		free_irq(IRQ_APBDMA_A_CH0, dev);
		if (dev->iisTxBase){
			iounmap(dev->iisTxBase);
			dev->iisTxBase= NULL;
		}
	}

	if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		halI2sRxFIFOClear();
		halI2sRxIntDisable();
		halI2sRxDisable();

		dev->isRxStart = 0;
		if (dma->iobase != NULL){
			HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1*4, DMA_OFF);
		}
		free_irq(IRQ_APBDMA_A_CH1, dev);
		if (dev->iisRxBase){
			iounmap(dev->iisRxBase);
			dev->iisRxBase= NULL;
		}
	}
	
	if (dma->iobase != NULL){
		iounmap(dma->iobase);
		dma->iobase = NULL;
	}
	
	if (buf->mem_addr != NULL){
		kfree(buf->mem_addr);
		buf->base = NULL;
	}
	
	dev->nonBlock = 0;
	dev->count--;
	if (dev->count < 0){	
		dev->count = 0;
	}

	return 0;
}

/*
 * The open() implementation
 */
static int
dsp_dev_open(
	struct inode *inode,
	struct file *filp
)
{
	struct dsp_dev_s *dev;
	struct dsp_buf_s *buf;
	struct dma_info_s *dma;
	unsigned int  iiscr, dmaar;
	int result;

	if (!inode || !filp) {
		return -EINVAL;
	}
	dev = container_of(inode->i_cdev, struct dsp_dev_s, cdev);
	filp->private_data = dev; /* for other methods */

	if (dev->count > 0) {
		DEBUG0("dsp device is not free.\n");
		return -EBUSY;
	}
	dev->count++;

	buf = &dev->buf;
	buf->bufSize = DEFAULT_BUF_SIZE;
	buf->blkSize = DEFAULT_BLK_SIZE;
	buf->mem_addr = kmalloc(buf->bufSize + buf->blkSize + 1024, GFP_KERNEL);
	if (buf->mem_addr == NULL)
	{
		return -ENOMEM;
	}
	buf->base = (char *)((((unsigned int)buf->mem_addr) + (1024 - 1)) & ~(1024 - 1)); 
	buf->inIdx = 0;
	buf->outIdx = 0;
	buf->dataFlag = 0;
	memset(buf->base, 0, buf->bufSize + buf->blkSize);

	/* DMA init */
	dma = &dev->dma;
	dma->currTx = DMA_BUFFER_A;
	dma->currRx = DMA_BUFFER_A;
	dma->buf_A = buf->base;
	dma->buf_B = buf->base + buf->blkSize;
	dma->iobase = ioremap(PA_APBDMAA_BASE, APBDMA_SIZE);

	/* parameter init */
	dev->caps = DSP_CAP_TRIGGER | DSP_CAP_BATCH;
	dev->error = 0;
	init_waitqueue_head(&dev->done_wait);
	
	writel(0xFFFFFFFF, 0xFC807004);

	audio_Power_ctrl(1);
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		/* play back */
		dev->iisTxBase = ioremap(0x93012000, 32*3);
		audio_FREQ_set(48000, 1);

		audio_WAVE_ctrl(1);
		audio_WAVE_volset(0x10,0x10);
		audio_WAVE_muteset(1,1);

		/* I2S */
		iiscr =   IISTX_FIRSTFRAME_L  | IISTX_FRAME_POLARITY_L | IISTX_EDGEMODE_FALLING
			| IISTX_SENDMODE_MSB      | IISTX_MODE_ALIGN_LEFT  | IISTX_VDMODE_16
			| IISTX_FSMODE_32         | IISTX_MODE_I2S         | IISTX_MASTER_MODE
			| IISTX_IRT_POLARITY_HIGH | IISTX_EN_OVWR          | IISTX_MERGE ;

		iiscr = iiscr & (~IISTX_SLAVE_MODE);
		writel(iiscr, dev->iisTxBase);

		/* APBDMA */
		/* setup buffer A */
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXA), (unsigned int )virt_to_phys(dma->buf_A));
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXA), (unsigned int )virt_to_phys(dma->buf_A + buf->blkSize - 4));

		/* setup buffer B */
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB), (unsigned int )virt_to_phys(dma->buf_B));
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB), (unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));

		/* setup APB access address */
		HAL_WRITE_UINT32(dma->iobase + DMAX_APB_SAX, 0x93012004);

		/* DMA channel settings */
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX, (DMA_M2P|DMA_REQ|DMA_FIX|DMA_DOUBLE_BUF|DMA_32BIT_BURST|DMA_IRQON|DMA_ON));

		result = request_irq(IRQ_APBDMA_A_CH0, dsp_dma_irq_handle, IRQF_DISABLED, "DMAA0", dev);
		if (result) {
			dsp_dev_release(inode, filp);
			DEBUG0("Request IRQ_APBDMA_A_CH0 failed\n");
			return -ENODEV;
		}
    }
	else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		/* recording */
		dev->iisRxBase = ioremap(0x9301D000, 32*3);
		audio_FREQ_set(16000, 0);

#if 0
		audio_LINEIN_ctrl(0);
		audio_LINEIN_volset(0x0,0x0);
#endif
		audio_MIC_ctrl(1);
		audio_MIC_volset(0x02);

		/* I2S */
		iiscr = IISRX_FIRSTFRAME_L|IISRX_FRAME_POLARITY_L|IISRX_EDGEMODE_FALLING
			|IISRX_SENDMODE_MSB|IISRX_MODE_ALIGN_RIGHT|IISRX_VDMODE_16|IISRX_FSMODE_16
			|IISRX_MODE_I2S|IISRX_SLAVE_MODE|IISRX_IRT_POLARITY_HIGH|IISRX_CLRFIFO|IISRX_MERGE|0x200000;

		iiscr = iiscr & (~IISRX_MASTER_MODE);
		//writel(iiscr, IISRX_ISCR_TMP);
		writel(iiscr,  dev->iisRxBase);

		/* APBDMA */
		/* setup buffer A */
		writel((unsigned int )virt_to_phys(dma->buf_A), (unsigned int )(dma->iobase + DMAX_AHB_SAXA + 1*4));
		writel((unsigned int )virt_to_phys(dma->buf_A + buf->blkSize - 4), (unsigned int )(dma->iobase + DMAX_AHB_EAXA + 1*4));

		/* setup buffer B */
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_SAXB + 1*4), (unsigned int )virt_to_phys(dma->buf_B));
		HAL_WRITE_UINT32((unsigned int )(dma->iobase + DMAX_AHB_EAXB + 1*4), (unsigned int )virt_to_phys(dma->buf_B + buf->blkSize - 4));

		/* setup APB access address */
		HAL_WRITE_UINT32(dma->iobase + DMAX_APB_SAX + 1*4, 0x9301D004);

		/* DMA channel settings */
		dmaar = DMA_P2M|DMA_REQ|DMA_FIX|DMA_DOUBLE_BUF|DMA_32BIT|DMA_IRQON|DMA_ON;
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1*4, dmaar);
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1*4, dmaar);

		result = request_irq(IRQ_APBDMA_A_CH1, dsp_dma_irq_handle, IRQF_DISABLED, "DMAA1", dev);
		if (result) {
			dsp_dev_release(inode, filp);
			DEBUG0("Request IRQ_APBDMA_A_CH1 failed\n");
			return -ENODEV;
		}
	}
	else
	{
		dsp_dev_release(inode, filp);
		return -EINVAL;
	}

	return 0;
}

/*
 * The ioctl() implementation
 */
int dsp_dev_ioctl(struct inode *inode, struct file *filp,
		 unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct dsp_dev_s *dev;
	
	if (!inode || !filp)
	{
		return -EINVAL;
	}

	dev = (struct dsp_dev_s *)filp->private_data;
	// DEBUG0("dsp ioctrl command: %d, arg %d\n", cmd, *(int __user*)arg);
	
	switch (cmd) {

	case SNDCTL_DSP_GETOSPACE:
		ret = dsp_ioctl_ospace_get(filp, (audio_buf_info *)arg);
		break;
		
	case SNDCTL_DSP_GETODELAY:
		ret = dsp_ioctl_odelay_get(filp, (int __user*)arg);
		break;

	case SNDCTL_DSP_GETISPACE:
		ret = dsp_ioctl_ispace_get(filp, (audio_buf_info *)arg);
		break;
		
	case SNDCTL_DSP_GETFMTS:
		ret = dsp_ioctl_fmt_get((int __user*)arg);
		break;
		
	case SNDCTL_DSP_SETFMT:
		ret = dsp_ioctl_fmt_set(filp, *(int __user*)arg);
		break;
		
	case SNDCTL_DSP_CHANNELS:
		ret = dsp_ioctl_channel_set(filp, *(int __user*)arg);
		break;
		
	case SNDCTL_DSP_STEREO:
		{
			int channel = (*(int __user*)arg) ? 2 : 1;
			ret = dsp_ioctl_channel_set(filp, channel);
		}
		break;
		
	case SNDCTL_DSP_SPEED:
		ret = dsp_ioctl_frequency_set(filp, *(int __user*)arg);
		break;

	case SNDCTL_DSP_SETFRAGMENT:
		ret = dsp_ioctl_fragment_set(filp, *(int __user*)arg);
		break;
		
	case SNDCTL_DSP_SETTRIGGER:
		ret = dsp_ioctl_trigger_set(filp, *(int __user*)arg);
		break;

	case SNDCTL_DSP_POST:
		ret = dsp_ioctl_trigger_set(filp, PCM_ENABLE_OUTPUT);
		break;
		
	case SNDCTL_DSP_HALT:
		ret = dsp_ioctl_trigger_set(filp, 0);
		break;

	case SNDCTL_DSP_GETERROR:
		*(int __user*)arg = dev->error;
		break;

	case SNDCTL_DSP_NONBLOCK:
		dev->nonBlock = 1;
		break;
		
	case SNDCTL_DSP_SKIP: /* Discards all samples in the playback buffer  */
		ret = dsp_ioctl_skip(filp);
		break;
		
	case SNDCTL_DSP_SYNC:  /* Suspend the application until all samples have been played */
		ret = dsp_ioctl_sync(filp);
		break;
		
	case SNDCTL_DSP_SILENCE: /* Clears the playback buffer with silence */
		if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
			ret = dsp_ioctl_slience(filp);
		}
		break;

	case SNDCTL_DSP_GETCAPS:
		*(int __user*)arg = dev->caps;
		break;

	default:
		DEBUG0("[%s] Unknown IO control %d.\n", __FUNCTION__, cmd);
		break;
	}
	dev->error = ret;
	return ret;
}

static const struct file_operations dev_fops =
{
	.owner = THIS_MODULE,
	.read = dsp_dev_read,
	.write = dsp_dev_write,
	.ioctl = dsp_dev_ioctl,
	.open = dsp_dev_open,
	.release = dsp_dev_release,
};

static void
dsp_setup_cdev(
	struct dsp_dev_s *dev,
	int index
)
{
	int err, devno = MKDEV(dsp_major, dsp_minor + index);

	cdev_init(&dev->cdev, &dev_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &dev_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		DEBUG0("mknod /dev/%s%d failed\n", DEVICE_NAME, index);
	}

	device_create(dsp_class, NULL, MKDEV(dsp_major, dsp_minor + index), NULL, DEVICE_NAME);
}

static void
dsp_module_exit(
	void
)
{
	int i;
	dev_t devno = MKDEV(dsp_major, dsp_minor);

	if (dsp_devices) {
		for (i = 0; i < dsp_nr_devs; i++) {
			device_destroy(dsp_class,MKDEV(dsp_major, dsp_minor + i));
			cdev_del(&dsp_devices[i].cdev);
		}
		kfree(dsp_devices);
	}
	unregister_chrdev_region(devno, dsp_nr_devs);
	class_destroy(dsp_class);

}

static int
dsp_module_init(
	void
)
{
	int result, i;
	dev_t dev = 0;

	/* TODO: register_sound_dsp */
	result = alloc_chrdev_region(&dev, dsp_minor, dsp_nr_devs, DEVICE_NAME);
	dsp_major = MAJOR(dev);
	if (result < 0) {
		DEBUG0("allocate device region failed\n");
		return result;
	}

	dsp_devices = kmalloc(dsp_nr_devs * sizeof(dsp_dev_t), GFP_KERNEL);
	if (!dsp_devices) {
		DEBUG0("memory allocate failed\n");
		result = -ENOMEM;
		goto fail;
	}
	memset(dsp_devices, 0, dsp_nr_devs * sizeof(dsp_dev_t));

	dsp_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(dsp_class)) {
		result = -EFAULT;
		goto fail;
	}

	for (i = 0; i < dsp_nr_devs; i++) {
		init_MUTEX(&dsp_devices[i].sem);
		dsp_setup_cdev(&dsp_devices[i], i);
	}

	return 0;

  fail:
	dsp_module_exit();
	return result;
}


module_init(dsp_module_init);
module_exit(dsp_module_exit);

MODULE_DESCRIPTION("DSP module");
MODULE_AUTHOR("gabriel.liao");
