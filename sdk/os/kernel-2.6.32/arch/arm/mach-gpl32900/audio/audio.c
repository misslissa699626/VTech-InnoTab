#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/hardware.h>
#include <mach/regs-interrupt.h>
#include <mach/regs-iis.h>
#include <mach/regs-saradc.h>
#include <mach/regs-dma.h>
#include <mach/audio/audio_util.h>
#include <mach/audio/soundcard.h>
#include <mach/irqs.h>
#include <mach/gp_cache.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_vic.h>

#define USE_FIQ             0
#if USE_FIQ == 1
#include <mach/gp_fiq.h>
#endif

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif /*  */

//#define AGCEN

#ifdef AGCEN
#include <mach/audio/agc.h>	//Add by Simon
#endif /*  */

#include "i2s.h"
MODULE_LICENSE("GPL");

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef HAL_READ_UINT32
#define HAL_READ_UINT32( _register_, _value_ ) ((_value_) = *((volatile unsigned int *)(_register_)))
#endif

#ifndef HAL_WRITE_UINT32
#define HAL_WRITE_UINT32( _register_, _value_ ) (*((volatile unsigned int *)(_register_)) = (_value_))
#endif

#define DUMP_REG
#if 0
#define DEBUG0(fmt,args...) printk(fmt,##args)
#else
#define DEBUG0(fmt,args...)
#endif

#if 0
#define DEBUG1(fmt,args...) printk(fmt,##args)
#else
#define DEBUG1(fmt,args...)
#endif

#define AUDIO_PRIORITY		5
#define DEVICE_NAME 		"dsp"
#define DEFAULT_BLK_SIZE 	(8*1024)
#define DEFAULT_BLK_NUM		(8)
#define MAX_AUDIO_BLOCKS    128
#define I2S_ENABLE			1
#define I2S_DISABLE			0
#define MAX_ALLOC_BLKS		64
#define ALLOC_BLK_SZ		(32*1024)	/* 32KB */

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct blk_alloc_s {
	char *pblock[MAX_ALLOC_BLKS];
	int remain_bytes;
	int grow;
	int idx;
} blk_alloc_t;

typedef struct audio_block_queue_s {
	unsigned long addr[MAX_AUDIO_BLOCKS];
	int in_idx;
	int out_idx;
	unsigned int in_size;	/*!< The accumerate of input size in bytes */
	unsigned int out_size;	/*!< The accumerate of output size in bytes */
	int blk_size;
} audio_block_queue_t;

typedef struct buf_status_s {
	blk_alloc_t blk_alloc;
	unsigned int a_addr;
	unsigned int b_addr;
	audio_block_queue_t q_ready;
	audio_block_queue_t q_avil;
	unsigned int dummy_addr;	/* buffer address for playback/record underrun */
	unsigned int working_addr;
	unsigned int working_offset;
	int blk_num;
	int pause;
	int isA;
	int skip; /* skip the plaing */
} buf_status_t;

typedef struct dma_info_s {
	void __iomem *iobase;
	char *recordBase;
	buf_status_t playBuf;
	buf_status_t recBuf;
} dma_info_t;

typedef struct dsp_dev_s {
	unsigned int is_tx_start;
	unsigned int is_rx_start;
	unsigned int non_block;
	unsigned int caps;
	unsigned int error;
	int r_count;
	int w_count;
	struct dma_info_s dma;
	struct cdev cdev;
	void __iomem *iisRxBase;
	void __iomem *iisTxBase;
} dsp_dev_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
struct dsp_dev_s *dsp_devices;
static struct class *dsp_class;
static int dsp_major = 0;
static int dsp_minor = 0;
static int dsp_nr_devs = 1;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                            F U N C T I O N                             *
 **************************************************************************/
/**
 * @breif Initalize the memory allocator.
 * @param palloc The structure of memory allocator.
 * @param grow The bytes to grow when memory need to be extended.
 */
static void blk_alloc_init(blk_alloc_t * palloc, int grow)
{
	memset(palloc, 0, sizeof(blk_alloc_t));
	palloc->grow = grow;
}

/**
 * @breif Destroy a memory allocator.
 * @param palloc The structure of memory allocator.
 */
static void blk_alloc_destroy(blk_alloc_t * palloc)
{
	int i;

	for (i = 0; i < palloc->idx; i++) {
		if (palloc->pblock[i]) {
			//printk("free idx=%d, addr=0x%x\n", i, palloc->pblock[i]);
			kfree(palloc->pblock[i]);
			palloc->pblock[i] = 0;
		}
	}
	palloc->idx = 0;
}

/**
 * @breif Extend the memory.
 * @param palloc The structure of memory allocator.
 */
static int blk_alloc_extend(blk_alloc_t * palloc)
{
	int idx = palloc->idx;

	if (idx >= MAX_ALLOC_BLKS)
		return -1;	/* fail */
	palloc->pblock[idx] = kmalloc(palloc->grow, GFP_KERNEL);
	if (palloc->pblock[idx] == 0)
		return -2;	/* fail */
	//printk("kmalloc idx=%d, addr=0x%x\n", idx, palloc->pblock[idx]);
	palloc->remain_bytes = palloc->grow;
	palloc->idx++;
	return 0;		/* success */
}

/**
 * @breif Create a memory allocator.
 * @param palloc The structure of memory allocator.
 * @param size The requested size.
 */
static char *blk_alloc_create(blk_alloc_t * palloc, int size)
{
	char *retPtr = 0;

	size = (((size + 31) >> 5) << 5);	/* 32 bytes alignment */
	if (size > ALLOC_BLK_SZ) {
		printk("Fatal error: exceed max block size ! %s (%d)\n",
		       __FILE__, __LINE__);
		return 0;
	}
recheck:
	if (palloc->remain_bytes >= size) {
		retPtr =
		    palloc->pblock[palloc->idx - 1] + palloc->grow -
		    palloc->remain_bytes;
		palloc->remain_bytes -= size;
	} else {
		if (blk_alloc_extend(palloc) != 0) {
			printk("Fatal error: cannot extend block ! %s (%d)\n",
			       __FILE__, __LINE__);
			return 0;
		}
		goto recheck;
	}
	return retPtr;
}

/**
 * @brief Initial the audio block queue.
 * @param p_queue The audio block queue structure.
 * @param blk_size The audio block size (in bytes)
 */
void audio_block_queue_init(audio_block_queue_t * p_queue, int blk_size)
{
	p_queue->in_idx = 0;
	p_queue->out_idx = 0;
	p_queue->in_size = 0;
	p_queue->out_size = 0;
	p_queue->blk_size = blk_size;
}

/**
 * @breif Check if the audio block queue is empty or not.
 * @param p_queue The audio block queue structure.
 * @return 1 if empty, else 0
 */
inline int audio_block_queue_is_empty(audio_block_queue_t * p_queue)
{
	return (p_queue->in_idx == p_queue->out_idx) ? 1 : 0;
}

/**
 * @breif Check if the audio block queue is full or not.
 * @param p_queue the audio block queue structure.
 * @return 1 if empty, else 0.
 */
inline int audio_block_queue_is_full(audio_block_queue_t * p_queue)
{
	return (((p_queue->in_idx + 1) % MAX_AUDIO_BLOCKS) ==
		p_queue->out_idx) ? 1 : 0;
}

/**
 * @breif Get the audio block count in a audio block queue.
 * @param p_queue The audio block queue structure.
 * @return The number of audio blocks in the queue.
 */
inline int audio_block_queue_get_count(audio_block_queue_t * p_queue)
{
	int cnt = p_queue->in_idx - p_queue->out_idx;

	return (cnt < 0) ? cnt + MAX_AUDIO_BLOCKS : cnt;
}

/**
 * @breif Get the remaining bytes in the audio queue.
 * @param p_queue The audio block queue structure.
 * @return The remaining bytes in the queue.
 */
inline int audio_block_queue_get_remain_bytes(audio_block_queue_t * p_queue)
{
	unsigned long diff;

	return (p_queue->in_size < p_queue->out_size) ? diff =
	    ((0xffffffff - p_queue->out_size) + 1) +
	    p_queue->in_size : (int)(p_queue->in_size - p_queue->out_size);
}

/**
 * @breif Put a block memory address into a queue.
 * @param p_queue The aduio block queue structure.
 * @param addr The starting address of a block.
 * @return 0 if success, else fail.
 */
inline int audio_block_queue_put(audio_block_queue_t * p_queue, unsigned int addr)
{
	if (audio_block_queue_is_full(p_queue)) {
		return 1;	/* FAIL */
	}
	p_queue->addr[p_queue->in_idx] = addr;
	p_queue->in_idx = (p_queue->in_idx + 1) % MAX_AUDIO_BLOCKS;
	p_queue->in_size += p_queue->blk_size;
	return 0;		/* OK */
}

/**
 * @breif Get a audio block from a queue.
 * @param p_queue The structure of a queue.
 * @ret 0: The queue is empty, else The audio block address
 */
inline unsigned int audio_block_queue_get(audio_block_queue_t * p_queue)
{
	unsigned int addr;

	if (audio_block_queue_is_empty(p_queue))
		return 0;
	addr = p_queue->addr[p_queue->out_idx];
	p_queue->out_idx = (p_queue->out_idx + 1) % MAX_AUDIO_BLOCKS;
	p_queue->out_size += p_queue->blk_size;
	return addr;
}

/**
 * @breif Fill a queue with allocated audio blocks.
 * @param p_queue The structure of a queue.
 * @param palloc The structure of a memory allocator.
 * @param blk_num Number of blocks to be created.
 * @ret 0 --> success, else fail.
 */
inline int audio_block_queue_fill(audio_block_queue_t * p_queue, blk_alloc_t * palloc,
			   int blk_num)
{
	int i;

	unsigned int addr;

	for (i = 0; i < blk_num; i++) {
		addr =
		    (unsigned int)blk_alloc_create(palloc, p_queue->blk_size);
		if (addr == 0)
			return -1;	/* fail */
#if 0
		printk("fill queue addr = 0x%x\n", addr);
#endif
		//memset( (void *)addr, 0, p_queue->blk_size);
		audio_block_queue_put(p_queue, addr);
	}
	return 0;
}

/**
 * @breif Clear the audio block clear.
 * @param p_queue The structure of a audio block queue.
 */
inline void audio_block_queue_clear(audio_block_queue_t * p_queue)
{
	p_queue->in_idx = p_queue->out_idx = 0;
}

/**
 * @breif Initialize the buffer status.
 * @param pbuf The buffer status tructure.
 * @param blk_size The audio block size.
 * @param blk_num The number of audio blocks.
 * @ret 0 --> success, else fail.
 */
static int buf_status_init(buf_status_t * pbuf, int blk_size, int blk_num)
{
	memset(pbuf, 0, sizeof(buf_status_t));
	blk_alloc_init(&pbuf->blk_alloc, ALLOC_BLK_SZ);	/* create allocator */
	audio_block_queue_init(&pbuf->q_ready, blk_size);
	audio_block_queue_init(&pbuf->q_avil, blk_size);

	/* fill blocks */
	if (audio_block_queue_fill
	    (&pbuf->q_avil, &pbuf->blk_alloc, blk_num) != 0)
		return -1;	/* fail */

	/* create dummy */
	pbuf->dummy_addr =
	    (unsigned int)blk_alloc_create(&pbuf->blk_alloc,
					   pbuf->q_avil.blk_size);
	if (pbuf->dummy_addr == 0) {
		/* fail */
		blk_alloc_destroy(&pbuf->blk_alloc);
		return -1;
	}
	memset((char *)pbuf->dummy_addr, 0, pbuf->q_avil.blk_size);	/* clear the buffer */
	pbuf->blk_num = blk_num;
	pbuf->a_addr = 0;
	pbuf->b_addr = 0;
	pbuf->working_addr = 0;
	pbuf->working_offset = 0;
	pbuf->pause = 0; /* for  SETTRIGGER */

	return 0;		/* success */
}

/**
 * @breif Destroy a buffer status.
 * @parma pbuf The pointer to a buffer status.
 * @ret 0 --> success.
 */
static int buf_status_destroy(buf_status_t * pbuf)
{
	blk_alloc_destroy(&pbuf->blk_alloc);
	memset(pbuf, 0, sizeof(buf_status_t));	/* clear all */
	return 0;		/* success */
}

/**
 * @breif Get the available byte count of a given buffer.
 * @param pbuf The structure of the buffer.
 * @param bi The linux audio information buffer.
 */
inline void get_avail_bytes_count(buf_status_t * pbuf, audio_buf_info * bi)
{
	int data_count;
	int blk_size = pbuf->q_avil.blk_size;
	int working_offset = pbuf->working_offset;

	data_count = audio_block_queue_get_remain_bytes(&pbuf->q_avil);
	if (pbuf->working_addr != 0)
		data_count += blk_size - working_offset;
	bi->fragments = data_count / blk_size;
	bi->fragstotal = pbuf->blk_num;	/* total number of fragments */
	bi->fragsize = blk_size;
	bi->bytes = data_count;
}

/**
 * @breif Get the ready byte count of a given buffer.
 * @param pbuf The structure of the buffer.
 * @param bi The linux audio information buffer.
 */
inline void get_ready_bytes_count(buf_status_t *pbuf, audio_buf_info *bi)
{
	int data_count;
	int blk_size = pbuf->q_ready.blk_size;
	int working_offset = pbuf->working_offset;

	data_count = audio_block_queue_get_remain_bytes(&pbuf->q_ready);
	if (pbuf->working_addr != 0)
		data_count += blk_size - working_offset;

	bi->fragments = data_count / blk_size;
	bi->fragstotal = pbuf->blk_num; /* total number of fragments */
	bi->fragsize = blk_size;
	bi->bytes = data_count;
}

/**
 * @breif Get readable space available.
 * @param filp The linux file structure.
 * @param bi The linux audio infor structure.
 * @ret 0 --> success else fail.
 */
inline static int dsp_ioctl_ispace_get(struct file *filp, audio_buf_info * bi)
{
	if (!bi) {
		return -EFAULT;
	}

	if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		dsp_dev_t *dev = (dsp_dev_t *) filp->private_data;
		get_ready_bytes_count(&dev->dma.recBuf, bi);

		return 0;
	}

	return -EFAULT;
}

/**
 * @breif Get writable space available.
 * @param filp The linux file structure.
 * @param bi The linux audio infor structure.
 * @ret 0 --> success else fail.
 */
inline static int dsp_ioctl_ospace_get(struct file *filp, audio_buf_info * bi)
{
	if (!bi) {
		return -EFAULT;
	}
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		dsp_dev_t *dev = (dsp_dev_t *) filp->private_data;
		get_avail_bytes_count(&dev->dma.playBuf, bi);

		return 0;
	}

	return -EFAULT;
}

/**
 * @breif Get the audio format.
 * @param fmt The integer pointer to a format.
 * @return 0 success, else fail.
 */
static int dsp_ioctl_fmt_get(int *fmt)
{
	if (!fmt) {
		return -EINVAL;
	}
	*fmt = AFMT_S16_LE | AFMT_S16_BE;
	return 0;
}

#if 0
/**
 * @breif Enable/disable the playback DSP hardware.
 * @brekf dev The structure to device.
 * @enable 1 -> enable, 0 -> disable.
 */
static void dsp_play_hardware(dsp_dev_t * dev, int enable)
{
	if (enable) {
		/* enable */
		if (dev->is_tx_start == 0) {
			dev->is_tx_start = 1;
			halI2sTxFIFOClear();
			halI2sTxIntEnable();
			halI2sTxEnable();
		}
	} else {
		/* disable */
		if (dev->is_tx_start == 1) {
			dev->is_tx_start = 0;
			halI2sTxFIFOClear();
			halI2sTxIntDisable();
			halI2sTxDisable();
		}
	}
}

/**
 * @breif Enable/Disable the record hardware.
 * @param dev The device structure.
 * @param enable 1 --> enable, 0 --> disable
 */
static void dsp_rec_hardware(dsp_dev_t * dev, int enable)
{
	if (enable) {
		if (!dev->is_rx_start) {
			dev->is_rx_start = 1;
			halI2sRxFIFOClear();
			halI2sRxIntEnable();
			halI2sRxEnable();
		}
	} else {
		if (dev->is_rx_start) {
			dev->is_rx_start = 0;
			halI2sRxFIFOClear();
			halI2sRxIntDisable();
			halI2sRxDisable();
		}
	}
}
#endif

/**
 * @breif Trigger the hardware (enable or disable)
 * @param filp The file structure.
 * @param trig The trigger flag.
 * @ret 0 --> success, else fail.
 */
static void playback_check_start(struct file *filp);
static int dsp_ioctl_trigger_set(struct file *filp, int trig)
{
	dsp_dev_t *dev = (dsp_dev_t *) filp->private_data;
	dma_info_t *dma = &dev->dma;
	buf_status_t *pBuf;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		pBuf = &dma->playBuf;
		pBuf->pause = ( trig & PCM_ENABLE_OUTPUT ) ? 0 : 1;
	} else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		pBuf->pause = ( trig & PCM_ENABLE_INPUT ) ? 0 : 1;
	}
	return 0;
}

/**
 * @breif for ioctl the channel set.
 * @breif filp The file structure.
 * @breif num The number of audio channels.
 * @return 0 --> success, else fail.
 */
static int dsp_ioctl_channel_set(struct file *filp, int num)
{
	int ret = -1;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (num == 1 || num == 2) {
			halI2sTxChlSet(num);
			ret = 0;
		}
	} else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		if (num == 1 || num == 2) {
			halI2sRxChlSet(num);
			ret = 0;
		}
	}
	return ret;
}

/**
 * @breif for ioctl the sampling frequency.
 * @param filp The file structure.
 * @param frequency The sampling frequency.
 * @return 0 --> success, else fail.
 */
static int dsp_ioctl_frequency_set(struct file *filp, int frequency)
{
	int ret = -1;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		ret = audio_FREQ_set(frequency, 1);
	} else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		ret = audio_FREQ_set(frequency, 0);
	}
	return ret;
}

/**
 * @breif Audio set format.
 * @param filp The file structure pointer.
 * @param fmt  one of AFMT_S16_LE, AFMT_S16_BE.
 */
static int dsp_ioctl_fmt_set(struct file *filp, int fmt)
{
	int ret = 0;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (fmt == AFMT_S16_LE)
			halI2sTxFrameOrderSet(1);
		else if (fmt == AFMT_S16_BE)
			halI2sTxFrameOrderSet(0);
		else
			ret = -1;
	} else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		if (fmt == AFMT_S16_LE)
			halI2sRxFrameOrderSet(1);
		else if (fmt == AFMT_S16_BE)
			halI2sRxFrameOrderSet(0);
		else
			ret = -1;
	} else
		ret = -1;	/* fail ! Not supported */

	return ret;
}

/**
 * @brief The the output delay samples.
 * @param filp The pointer of file structure.
 * @param How many bytes are int the buffer.
 * @return 0 --> success, else fail.
 */
static int dsp_ioctl_odelay_get(struct file *filp, int *delay)
{
	int ret = -1;
	int data_count;
	dsp_dev_t *dev = filp->private_data;

	if (!delay) {
		return -EFAULT;
	}

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		data_count =
		    audio_block_queue_get_remain_bytes(&dev->dma.playBuf.
						       q_ready);
		/* 2 block is already in DMA, but one block is in   */
		/* wrtting mode. So average one block added.        */
		data_count += dev->dma.playBuf.q_ready.blk_size;
		*delay = data_count;
		ret = 0;
	}

	return ret;
}

/**
 * @breif Skip the current data.
 * @param filp The file structure parameters.
 * @ret 0 --> success, else fail.
 */
static int dsp_ioctl_skip(struct file *filp)
{
	dsp_dev_t *dev = filp->private_data;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		dev->dma.playBuf.skip = 1; /* mark skip flag */
		return 0;
	}
	return -EFAULT;
}

/**
 * @breif io control for SYNC audio.
 * @param filp The file structure.
 * @ret 0 --> success, elase fail.
 */
static int dsp_ioctl_sync(struct file *filp)
{

	dsp_dev_t *dev = filp->private_data;
	dma_info_t *dma = &dev->dma;
	buf_status_t *pplayBuf = &dma->playBuf;
	int flags = filp->f_flags & O_ACCMODE;
	int blk_size = pplayBuf->q_ready.blk_size;
	int left;

	if (dev->is_tx_start == 0)
		return 0;

	if (flags == O_WRONLY) {
		/* flush the current data ! */
		if ( pplayBuf->working_addr != 0 ) {
			left = blk_size - pplayBuf->working_offset;
			if (left>0) {
				/* fill and queue the block */
				memset( (void *)(pplayBuf->working_addr + pplayBuf->working_offset), 0, left );
				pplayBuf->working_addr = 0; /* clear */
				pplayBuf->working_offset = 0;
				audio_block_queue_put(&pplayBuf->q_ready, pplayBuf->working_addr);	/* put into DMA queue */
			}
		}

		/* wait until all all samples are played */
		while (dev->is_tx_start) {
			if ((audio_block_queue_get_count
			    (&dma->playBuf.q_ready) == 0) &&
				(dma->playBuf.a_addr == dma->playBuf.dummy_addr) &&
				(dma->playBuf.b_addr == dma->playBuf.dummy_addr)) {

				/* stop the hardware */
				halI2sTxFIFOClear();
				halI2sTxIntDisable();
				halI2sTxDisable();
				dev->is_tx_start = 0;
				break;
			}
			msleep(10);
		}
	}
	return 0;
}

static int dsp_ioctl_silence(struct file *filp)
{
	/* same as skip, we play silence while no data available. */
	dsp_dev_t *dev = filp->private_data;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		dev->dma.playBuf.skip = 1; /* mark skip flag */
		return 0;
	}
	return -EFAULT;
}

/**
 * @breif Set data buffer fragement.
 * @param filp The structure of file.
 * @param fragment The fragment inforamtion.
 * @ret 0 --> sucdess, else fail.
 */
static int dsp_ioctl_fragment_set(struct file *filp, int fragment)
{
	dsp_dev_t *dev = filp->private_data;
	dma_info_t *dma = &dev->dma;
	int blk_num, blk_size, ret;

	blk_num = fragment >> 16;
	if (blk_num < 4) {
		return -EDOM;	/* at least 4 buffers */
	}
	blk_size = 1 << (fragment & 0xFFFF);
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		/* check playback buffer */
		if (dev->is_tx_start)
			return -EDOM;	/* hardware already start ! cannot change the buffer */

		buf_status_destroy(&dma->playBuf);	/* free playBuf memory */
		ret = buf_status_init(&dma->playBuf, blk_size, blk_num);
		if (ret != 0)
			return ret;	/* return fail status */
	} else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		if (dev->is_rx_start)
			return -EDOM;	/* hardware already start ! cannot change the buffer */
		buf_status_destroy(&dma->recBuf);	/* free recBuf memory */
		ret = buf_status_init(&dma->recBuf, blk_size, blk_num);
		if (ret != 0)
			return ret;
	} else {

		/* fail */
		return -EDOM;
	}
	return 0;		/* success */
}

static irqreturn_t dma_irq_handle_play(int irq, void *dev_id)
{
	unsigned int status;
	unsigned int addr;
	unsigned int phy_addr;
	//unsigned int play_buf;
	struct dsp_dev_s *dev;
	struct dma_info_s *dma;
	buf_status_t *pplayBuf;
#if AUDIO_LOOP_CHECK
	int counter = 0 ;
#endif
	
	dev = (struct dsp_dev_s *)dev_id;
	dma = &dev->dma;
	pplayBuf = &dma->playBuf;
	/* clear interrupt first to 
	   decrease the possibility of lost interrupt */
	HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_IRQ_STATUS), 1);
	//writel(1, dma->iobase + DMAX_IRQ_STATUS);

	if ( pplayBuf->skip ) {
		/* skip all packets in queue */
		addr = audio_block_queue_get( &pplayBuf->q_ready );
		while ( addr ) {
			audio_block_queue_put( &pplayBuf->q_avil, addr );
			addr = audio_block_queue_get( &pplayBuf->q_ready ); /* read again */
			#if AUDIO_LOOP_CHECK
				counter = counter + 1 ;
				if ( counter > 2000 ) {
					printk("ERROR : Audio while loop 2000\n");
				}
			#endif
		}
		pplayBuf->skip = 0; /* already skip */
	}

	if ( pplayBuf->pause ) {
		addr = pplayBuf->dummy_addr;
	} else if ( audio_WAVE_ismute() ) {
		/* workaroud for audio leakage while mute */
		addr = audio_block_queue_get( &pplayBuf->q_ready );
		if (addr)
			audio_block_queue_put( &pplayBuf->q_avil, addr );

		addr = pplayBuf->dummy_addr;
	} else {
		addr = audio_block_queue_get(&pplayBuf->q_ready);
		if (addr == 0)
			addr = pplayBuf->dummy_addr;	/* under run, give it a dummy silence block */
	}

	phy_addr = virt_to_phys((void *)addr);
	/* playback */
	HAL_READ_UINT32((unsigned int)dma->iobase, status);
	//play_buf = ((status >> 8) & 0x1);	/* 0 -> a_buf, 1 -> b_buf */

	// Ad hoc patchwork for a junk driver...
	if ( pplayBuf->isA != ((status>>8)&0x01) )
	{
		pplayBuf->isA = 1 - pplayBuf->isA; 
	}
	if (pplayBuf->isA) {
		/* now sending the B buffer */
		/* setup the address of A */
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXA),
				 phy_addr);
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXA),
				 phy_addr + pplayBuf->q_ready.blk_size - 4);

		if (pplayBuf->a_addr != pplayBuf->dummy_addr)
			audio_block_queue_put(&pplayBuf->q_avil, pplayBuf->a_addr);	/* put into available queue */

		pplayBuf->a_addr = addr;
	} else {
		/* busy on buffer A, finishing transferring buffer B */
		/* setup the address of B */
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXB),
				 phy_addr);
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXB),
				 phy_addr + pplayBuf->q_ready.blk_size - 4);
		if (pplayBuf->b_addr != pplayBuf->dummy_addr)
			audio_block_queue_put(&pplayBuf->q_avil, pplayBuf->b_addr);	/* put into available queue */

		pplayBuf->b_addr = addr;
	}
	pplayBuf->isA = 1 - pplayBuf->isA; /* switch */

	return IRQ_HANDLED;
}

static irqreturn_t dma_irq_handle_record(int irq, void *dev_id)
{
	unsigned int status;
	unsigned int addr;
	//unsigned int rec_buf;
	unsigned int phy_addr;
	unsigned int blk_size;
	struct dsp_dev_s *dev;
	struct dma_info_s *dma;
	buf_status_t *precBuf;

	dev = (struct dsp_dev_s *)dev_id;
	dma = &dev->dma;
	precBuf = &dma->recBuf;
	blk_size = precBuf->q_ready.blk_size;

	/* clear interrupt first to reduce the possibility of lost interrupt */
	writel(1 << 1, dma->iobase + DMAX_IRQ_STATUS);

	if (precBuf->pause) {
		addr = precBuf->dummy_addr;
	} else {
		addr = audio_block_queue_get(&precBuf->q_avil);
		if (addr == 0)
			addr = precBuf->dummy_addr;	/* under run, give it a dummy block */
	}

	phy_addr = virt_to_phys((void *)addr);
	
	HAL_READ_UINT32((unsigned int)dma->iobase, status);
	//rec_buf = ((status >> 9) & 0x1);	/* 0 -> a_buf, 1 -> b_buf */
	if (precBuf->isA) {
		/* busy on buffer B, finishing transferring buffer A */
		/* setup the address of A, and put finished A into queue */
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXA +
						1 * 4), phy_addr);
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXA +
						1 * 4),
				 (unsigned int)phy_addr + blk_size - 4);
		if (precBuf->a_addr != precBuf->dummy_addr) {
			audio_block_queue_put(&precBuf->q_ready, precBuf->a_addr);	/* put into ready queue */
		}
		precBuf->a_addr = addr;
	} else {
		/* busy on buffer A, finishing transferring buffer B */
		/* setup the address of B, and put finished B into queue */
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXB +
						1 * 4), phy_addr);
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXB +
						1 * 4),
				 (unsigned int)phy_addr + blk_size - 4);
		if (precBuf->b_addr != precBuf->dummy_addr) {
			audio_block_queue_put(&precBuf->q_ready, precBuf->b_addr);	/* put into ready queue */
		}
		precBuf->b_addr = addr;
	}
	precBuf->isA = 1 - precBuf->isA;

	return IRQ_HANDLED;
}

/**
 * @breif Start record DMA hardware.
 * @param filp The file structure.
 */
static void start_record(struct file *filp)
{
	struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
	struct dma_info_s *dma = &dev->dma;
	buf_status_t *precBuf = &dma->recBuf;
	int blk_size = precBuf->q_avil.blk_size;
	unsigned int dmaar, iiscr;

	/* I2S */
	if (dev->is_rx_start)
		return;		/* already started ! do nothing */

	precBuf->a_addr = audio_block_queue_get(&precBuf->q_avil);	/* A */
	precBuf->b_addr = audio_block_queue_get(&precBuf->q_avil);	/* B */

	/* I2S */
	iiscr =
	    IISRX_FIRSTFRAME_L | IISRX_FRAME_POLARITY_L | IISRX_EDGEMODE_FALLING
	    | IISRX_SENDMODE_MSB | IISRX_MODE_ALIGN_RIGHT | IISRX_VDMODE_16 |
	    IISRX_FSMODE_16 | IISRX_MODE_I2S | IISRX_SLAVE_MODE |
	    IISRX_IRT_POLARITY_HIGH | IISRX_CLRFIFO | IISRX_MERGE | 0x200000;

	iiscr = iiscr & (~IISRX_MASTER_MODE);
	writel(iiscr, dev->iisRxBase);

	/* APBDMA */
	/* setup buffer A */
	HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXA + 1 * 4),
			 (unsigned int)virt_to_phys((void *)precBuf->a_addr));
	HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXA + 1 * 4),
			 (unsigned int)
			 virt_to_phys((void *)(precBuf->a_addr + blk_size -
					       4)));

	/* setup buffer B */
	HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXB + 1 * 4),
			 (unsigned int)virt_to_phys((void *)precBuf->b_addr));
	HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXB + 1 * 4),
			 (unsigned int)
			 virt_to_phys((void *)(precBuf->b_addr + blk_size -
					       4)));

	/* setup APB access address (I2S) */
	HAL_WRITE_UINT32(dma->iobase + DMAX_APB_SAX + 1 * 4, 0x9301D004);

	/* DMA channel settings */
	dmaar =
	    DMA_P2M | DMA_REQ | DMA_FIX | DMA_DOUBLE_BUF | DMA_32BIT | DMA_IRQON
	    | DMA_ON;

	HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1 * 4, dmaar);

	precBuf->isA = 1; /* start from A */
	dev->is_rx_start = 1;
	/* start FIFO, enable DMA */
	halI2sRxFIFOClear();
	halI2sRxIntEnable();
	halI2sRxEnable();
}

/**
 * @breif The device read.
 * @param filp The file structure.
 * @param buffer The pointer to a user buffer.
 * @param count The byte count that is going to be read.
 * @param ppos The position parameter.
 * @ret The size of read bytes.
 */
static ssize_t dsp_dev_read(struct file *filp, char __user * buffer,
			    size_t count, loff_t * ppos)
{
	dsp_dev_t *dev = (struct dsp_dev_s *)filp->private_data;
	dma_info_t *dma = &dev->dma;
	buf_status_t *precBuf = &dma->recBuf;
	int read = 0, left = 0, blk_size;

	if (!filp || !buffer) {
		return -EINVAL;
	}

	/* enable the hardware */
	start_record(filp);

	blk_size = precBuf->q_ready.blk_size;
	while (count > 0) {
		while (precBuf->working_addr == 0) {
			precBuf->working_addr =
			    audio_block_queue_get(&precBuf->q_ready);
			if (precBuf->working_addr == 0) {	/* no ready buffer, just wait */
				if (dev->non_block) {
					return read;
				}

				msleep(10);	/* wait until ready */
			} else {
				gp_invalidate_dcache_range(precBuf->working_addr, blk_size); /* invalidate dcache */
				precBuf->working_offset = 0;	/* clear */
			}
		}

		left = blk_size - precBuf->working_offset;	/* how many bytes can be read */
		if (count >= left) {
			copy_to_user(buffer + read,
				     (void *)(precBuf->working_addr +
					      precBuf->working_offset), left);
			audio_block_queue_put(&precBuf->q_avil, precBuf->working_addr);	/* put into avail queue */
			precBuf->working_addr = 0;	/* clear block */
			precBuf->working_offset = 0;
			read += left;
			count -= left;
		} else {
			copy_to_user(buffer + read,
				     (void *)(precBuf->working_addr +
					      precBuf->working_offset), count);
			precBuf->working_offset += count;
			read += count;
			count = 0;	/* finish */
		}
	}

	return read;
}

/**
 * @breif Check playback start.
 * @param filp The file structure.
 */
static void playback_check_start(struct file *filp)
{
	struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
	struct dma_info_s *dma = &dev->dma;
	buf_status_t *pplayBuf = &dma->playBuf;
	int blk_size = pplayBuf->q_ready.blk_size;

	if (dev->is_tx_start)
		return;		/* already started ! do nothing */

	if (audio_block_queue_get_count(&pplayBuf->q_ready) >= 2) {
		pplayBuf->a_addr = audio_block_queue_get(&pplayBuf->q_ready);	/* A */
		pplayBuf->b_addr = audio_block_queue_get(&pplayBuf->q_ready);	/* B */

		/* APBDMA */
		/* setup buffer A */
		HAL_WRITE_UINT32((unsigned int)(dma->iobase +
						DMAX_AHB_SAXA),
				 (unsigned int)virt_to_phys((void *)
							    pplayBuf->a_addr));
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXA),
				 (unsigned int)
				 virt_to_phys((void *)(pplayBuf->a_addr +
						       blk_size - 4)));

		/* setup buffer B */
		HAL_WRITE_UINT32((unsigned int)(dma->iobase +
						DMAX_AHB_SAXB),
				 (unsigned int)virt_to_phys((void *)
							    pplayBuf->b_addr));
		HAL_WRITE_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXB),
				 (unsigned int)
				 virt_to_phys((void *)(pplayBuf->b_addr +
						       blk_size - 4)));

		/* setup APB access address */
		HAL_WRITE_UINT32(dma->iobase + DMAX_APB_SAX, 0x93012004);

		/* DMA channel settings */
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX,
				 (DMA_M2P | DMA_REQ | DMA_FIX |
				  DMA_DOUBLE_BUF | DMA_32BIT_BURST |
				  DMA_IRQON | DMA_ON));

		pplayBuf->isA = 1; /* start from A */
		dev->is_tx_start = 1;	/* start from buffer A */

		/* start Fifo */
		halI2sTxFIFOClear();
		halI2sTxIntEnable();
		halI2sTxEnable();
	}
}

#define AUDIO_CRASH_DEBUG 0
#define AUDIO_LOOP_CHECK  1
/**
 * @breif The write() implementation
 * @param filp The Linux file structure.
 * @param buffer The user buffer address.
 * @param count The size of data to be written (in bytes).
 * @ret The size that has been written.
 */
static ssize_t dsp_dev_write(struct file *filp, const char __user * buffer,
			     size_t count, loff_t * ppos)
{
	struct dsp_dev_s *dev = (struct dsp_dev_s *)filp->private_data;
	struct dma_info_s *dma = &dev->dma;

	buf_status_t *pplayBuf = &dma->playBuf;
	int written = 0, left = 0, blk_size;
	int waiting_counter;


/**/
#if AUDIO_CRASH_DEBUG
	static unsigned long base = 0;
	static unsigned long adcbase = 0;
	if (base == 0)
		base = (unsigned long)ioremap(0x90010000, 128);	/* ARM11 VIC0 (0~31) priority setting address */

	if (adcbase == 0)
		adcbase = (unsigned long)ioremap(0x93007000, 128);
#endif
/**/

	if (!filp || !buffer) {
		return -EINVAL;
	}

	pplayBuf->pause = 0; /* clear the pause flag */
	blk_size = pplayBuf->q_avil.blk_size;
	while (count > 0) {
		waiting_counter = 0;
		while (pplayBuf->working_addr == 0) {
			pplayBuf->working_addr =
			    audio_block_queue_get(&pplayBuf->q_avil);
			pplayBuf->working_offset = 0;	/* clear */

			playback_check_start(filp);

			if (pplayBuf->working_addr == 0) {
				if (dev->non_block)
					return written;

				if (++waiting_counter>100) {
                    printk("ERROR : Audio DMA stopped !! (Timeout !)\n");
                #if AUDIO_CRASH_DEBUG
					unsigned int iiscr;
					unsigned int reg;
					unsigned int regStartA, regEndA, regStartB, regEndB;
					printk("ERROR : Audio DMA stopped !! (Timeout !)\n");
                    HAL_READ_UINT32( dev->iisTxBase, iiscr );
					printk("I2S = 0x%x\n", iiscr);
					/* I2S */
					iiscr =
					    IISTX_FIRSTFRAME_L | IISTX_FRAME_POLARITY_L | IISTX_EDGEMODE_FALLING
					    | IISTX_SENDMODE_MSB | IISTX_MODE_ALIGN_LEFT | IISTX_VDMODE_16
					    | IISTX_FSMODE_32 | IISTX_MODE_I2S | IISTX_MASTER_MODE
					    | IISTX_IRT_POLARITY_HIGH | IISTX_EN_OVWR | IISTX_MERGE;
					iiscr = iiscr & (~IISTX_SLAVE_MODE);
					printk("orig I2S = 0x%x\n", iiscr);

					HAL_READ_UINT32( base, reg );
					printk("reg (0x90010000) bit 24 = %d\n", ((reg>>24)&1));
					HAL_READ_UINT32( (base+0x10), reg );
					printk("reg (0x90010010) bit 24 = %d\n", ((reg>>24)&1));

					HAL_READ_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXA), regStartA);
					HAL_READ_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXA), regEndA);
					HAL_READ_UINT32((unsigned int)(dma->iobase + DMAX_AHB_SAXB), regStartB);
					HAL_READ_UINT32((unsigned int)(dma->iobase + DMAX_AHB_EAXB), regEndB);
					printk("A start=0x%x, end=0x%x\n", regStartA, regEndA);
					printk("B start=0x%x, end=0x%x\n", regStartB, regEndB);
					HAL_READ_UINT32((unsigned int)(adcbase + 0x4), reg);
					printk("ADC bit9 =%d\n", (reg>>9)&1) ;
					printk("ADC bit17=%d\n", (reg>>17)&1) ;
					printk("ADC bit18=%d\n", (reg>>18)&1) ;
					printk("ADC bit19=%d\n", (reg>>19)&1) ;
					HAL_READ_UINT32((unsigned int)(adcbase + 0x44), reg);
					printk("0x44 value = 0x%x\n", reg);
                #endif
					return written;
				}
				msleep(10);	/* wait until get a buffer to write */
			}
		}
		left = blk_size - pplayBuf->working_offset;
		if (count >= left) {
			copy_from_user((void *)(pplayBuf->working_addr +
						pplayBuf->working_offset),
				       buffer + written, left);
			audio_block_queue_put(&pplayBuf->q_ready, pplayBuf->working_addr);	/* put into DMA queue */
			pplayBuf->working_addr = 0;	/* clear block */
			pplayBuf->working_offset = 0;
			written += left;
			count -= left;
		} else {
			copy_from_user((void *)pplayBuf->working_addr +
				       pplayBuf->working_offset,
				       buffer + written, count);
			pplayBuf->working_offset += count;
			written += count;
			count = 0;	/* finish */
		}
		playback_check_start(filp);
	}

	return written;
}

/**
 @breif Release device node.
 @param inode The Linux inode structure.
 @param filp The Linux file structure.
*/
static int dsp_dev_release_write(struct inode *inode, struct file *filp)
{
	struct dsp_dev_s *dev;
	struct dma_info_s *dma;


	if (!inode || !filp) {
		return -EINVAL;
	}

	dev = filp->private_data;
	dma = &dev->dma;

	if (dev->w_count <= 0) {
		return 0;
	}

	halI2sTxFIFOClear();
	halI2sTxIntDisable();
	halI2sTxDisable();
	dev->is_tx_start = 0;
	buf_status_destroy(&dma->playBuf);	/* free memory */
	if (dma->iobase != NULL) {
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX, DMA_OFF);
//		if(dev->r_count==0)
//        	dma->iobase = NULL;
	}
	
	#if USE_FIQ == 1
	free_fiq(IRQ_APBDMA_A_CH0);
	#else
	free_irq(IRQ_APBDMA_A_CH0, dev);
	#endif
	if (dev->iisTxBase) {
		dev->iisTxBase = NULL;
	}
	
	dev->non_block = 0;	/* clear non-block setting */
	dev->w_count--;		/* decrease reference count */
	if (dev->w_count < 0) {
		printk("FATAL ERROR: close count %s(%d)\n", __FILE__, __LINE__);
		dev->w_count = 0;
	}
	return 0;
}

/**
 * @breif Cloase the device node.
 * @param indoe The Linux inode data structure.
 * @param filp The pointer to file structure.
 */
static int dsp_dev_release_read(struct inode *inode, struct file *filp)
{
	dsp_dev_t *dev;
	dma_info_t *dma;

	if (!inode || !filp) {
		return -EINVAL;
	}

	dev = filp->private_data;
	dma = &dev->dma;

	if (dev->r_count <= 0) {
		return 0;
	}

	halI2sRxFIFOClear();
	halI2sRxIntDisable();
	halI2sRxDisable();
	dev->is_rx_start = 0;
	buf_status_destroy(&dma->recBuf);	/* free memory */
	if (dma->iobase != NULL) {
		HAL_WRITE_UINT32(dma->iobase + DMAX_CRX + 1 * 4, DMA_OFF);
//		if(dev->w_count==0)
//        	dma->iobase = NULL;
	}

	#if USE_FIQ == 1
	free_fiq(IRQ_APBDMA_A_CH1);
	#else
	free_irq(IRQ_APBDMA_A_CH1, dev);
	#endif

    if (dev->iisRxBase) {
		dev->iisRxBase = NULL;
	}

	dev->non_block = 0;	/* clear non-block setting */
	dev->r_count--;		/* decrease reference count */
	if (dev->r_count < 0) {
		printk("FATAL ERROR: close count %s(%d)\n", __FILE__, __LINE__);
		dev->r_count = 0;
	}
	return 0;
}

/**
 * @breif Close a device node.
 * @param inode The Linux inode structure.
 * @param filp The Linux file structre.
 */
static int dsp_dev_release(struct inode *inode, struct file *filp)
{
	int result = 0;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		result = dsp_dev_release_write(inode, filp);
	} else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		result = dsp_dev_release_read(inode, filp);
	}

	return result;
}

/**
 * @breif Adjust the interrupt priority.
 */
#if 0
static void adjust_priority(void)
{
	unsigned long pbase;
	int result;

	/* setup the audio interrupt priority */
	pbase = (unsigned long)ioremap(0x90010200, 128);	/* ARM11 VIC0 (0~31) priority setting address */
	result = AUDIO_PRIORITY;
	HAL_WRITE_UINT32((unsigned int)pbase + (IRQ_APBDMA_A_CH0 * 4), result);	/* raise the playback priority */
	HAL_WRITE_UINT32((unsigned int)pbase + (IRQ_APBDMA_A_CH1 * 4), result);	/* raise the record priority */
	iounmap((volatile void *)pbase);
}
#endif

/**
 * @breif Open a audio block device for write.
 * @param dev The structure to a device structure.
 * @param indoe The inode structure.
 * @param filp The file structure.
 * @return 0 --> success, else fail
 */
static int dsp_open_for_write(dsp_dev_t * dev, struct inode *inode,
			      struct file *filp)
{
	dma_info_t *dma = &dev->dma;
	buf_status_t *pplayBuf = &dma->playBuf;
	unsigned int iiscr;
	int result;

	dev->is_tx_start = 0;
	/* setup playback buffer management */
	if (buf_status_init(pplayBuf, DEFAULT_BLK_SIZE, DEFAULT_BLK_NUM) != 0) {
		/* fail */
		printk("DSP device no memory ! (play)\n");
		return -ENOMEM;
	}

	/* DMA init */
	dma = &dev->dma;
	pplayBuf = &dma->playBuf;
    dma->iobase = (void __iomem *) APBDMAA_BASE;
//		HAL_WRITE_UINT32(dma->iobase + DMAX_RST, 1); /* rest DMA */
//	HAL_WRITE_UINT32(dma->iobase + DMAX_CRX, DMA_OFF);
//	HAL_WRITE_UINT32(dma->iobase + DMAX_RST, 1);	
	
	/* parameter init */
	dev->caps = DSP_CAP_TRIGGER | DSP_CAP_BATCH;
	dev->error = 0;

	/* play back */
	dev->iisTxBase = (void __iomem *) IO3_ADDRESS(0x12000) ; // ioremap(0x93012000, 32 * 3);
	audio_FREQ_set(48000, 1);
	audio_WAVE_ctrl(1);

	/* I2S */
	iiscr =
	    IISTX_FIRSTFRAME_L | IISTX_FRAME_POLARITY_L | IISTX_EDGEMODE_FALLING
	    | IISTX_SENDMODE_MSB | IISTX_MODE_ALIGN_LEFT | IISTX_VDMODE_16
	    | IISTX_FSMODE_32 | IISTX_MODE_I2S | IISTX_MASTER_MODE
	    | IISTX_IRT_POLARITY_HIGH | IISTX_EN_OVWR | IISTX_MERGE;

	iiscr = iiscr & (~IISTX_SLAVE_MODE);
	writel(iiscr, dev->iisTxBase);
	printk("I2S = 0x%x\n", iiscr);
	
	/* register interrupt */
	#if USE_FIQ == 1
	result = request_fiq(IRQ_APBDMA_A_CH0, dma_irq_handle_play, "DMAA0", dev);
	if (result) {
		dsp_dev_release_write(inode, filp);
		printk("Audio: Request FIQ (play) failed\n");
		return -ENODEV;
	}
	#else
	result =
	    request_irq(IRQ_APBDMA_A_CH0, dma_irq_handle_play, IRQF_DISABLED,
			"DMAA0", dev);
	if (result) {
		dsp_dev_release_write(inode, filp);
		printk("Audio: Request IRQ (play) failed\n");
		return -ENODEV;
	}
	#endif
	dev->w_count++;		/* open success, increase the reference count */

	return 0;		/* success */
}

/**
 * @breif Open for read.
 * @param dev The device structure
 * @param indoe The Linux inode structure.
 * @param filp The file handle structure.
 * @ret 0 --> success, else fail.
 */
static int dsp_open_for_read(dsp_dev_t * dev, struct inode *inode,
			     struct file *filp)
{
	int result;
	unsigned int iiscr;
	dma_info_t *dma = &dev->dma;
	buf_status_t *precBuf = &dma->recBuf;

	dev->is_rx_start = 0;
	if (buf_status_init(precBuf, DEFAULT_BLK_SIZE, DEFAULT_BLK_NUM) != 0) {
		/* fail */
		printk("DSP device no memory ! (record)\n");
		return -ENOMEM;
	}

    dma->iobase = (void __iomem *) APBDMAA_BASE;

	/* recording */
	dev->iisRxBase = (void __iomem *) IO3_ADDRESS(0x1D000) ; //ioremap(0x9301D000, 32 * 3);
	audio_FREQ_set(16000, 0);
	audio_MIC_ctrl(1);

#ifdef AGCEN
	I2SRx_init_mic_agc();
#else /*  */
	audio_MIC_volset(0x02);
#endif /*  */

	/* I2S */
	iiscr =
	    IISRX_FIRSTFRAME_L | IISRX_FRAME_POLARITY_L |
	    IISRX_EDGEMODE_FALLING | IISRX_SENDMODE_MSB |
	    IISRX_MODE_ALIGN_RIGHT | IISRX_VDMODE_16 | IISRX_FSMODE_16
	    | IISRX_MODE_I2S | IISRX_SLAVE_MODE |
	    IISRX_IRT_POLARITY_HIGH | IISRX_CLRFIFO | IISRX_MERGE | 0x200000;

	iiscr = iiscr & (~IISRX_MASTER_MODE);
	writel(iiscr, dev->iisRxBase);
	
	#if USE_FIQ == 1
	result = request_fiq(IRQ_APBDMA_A_CH1, dma_irq_handle_record, "DMAA1", dev);

	if (result) {
		dsp_dev_release_read(inode, filp);
		printk("Audio: Request FIQ (record) failed, code=%d.\n",
		       result);
		return -ENODEV;
	}
	
	#else	
	result = request_irq(IRQ_APBDMA_A_CH1, dma_irq_handle_record,
			     IRQF_DISABLED, "DMAA1", dev);

	if (result) {
		dsp_dev_release_read(inode, filp);
		printk("Audio: Request IRQ (record) failed, code=%d.\n",
		       result);
		return -ENODEV;
	}
	#endif
	dev->r_count++;		/* open success, increase the reference count */

	return 0;
}

/**
 * @breif The open() implementation.
 * @param indoe The inode structure.
 * @param filp The file structure.
 * @ret 0 --> success, else fail
 */
static int dsp_dev_open(struct inode *inode, struct file *filp)
{
	dsp_dev_t *dev;

	int result;

	if (!inode || !filp) {
		return -EINVAL;
	}

	dev = container_of(inode->i_cdev, struct dsp_dev_s, cdev);
	filp->private_data = dev;	/* for other methods */
/*	if (dev->count > 0) {
		printk("DSP device is occupied (BUSY)\n");
		return -EBUSY;
	}*/
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if(dev->w_count > 0) {
			printk("DSP device is occupied (BUSY)\n");
			return -EBUSY;
		}
		result = dsp_open_for_write(dev, inode, filp);
	} else {
		/* open_for_read( inode, filp ) */
		if(dev->r_count >0) {
			printk("DSP device is occupied (BUSY)\n");
			return -EBUSY;
		}
		result = dsp_open_for_read(dev, inode, filp);
	}
	return result;
}

/**
 * @breif The ioctl entry point.
 * @param inode The linux inode.
 * @param filp The pointer to the file structure.
 * @param cmd The IOCTL command.
 * @param arg The control argument.
 * @ret 0 --> success, else fail.
 */
int dsp_dev_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		  unsigned long arg)
{
	int ret = 0;
	struct dsp_dev_s *dev;

	if (!inode || !filp) {
		return -EINVAL;
	}

	dev = (struct dsp_dev_s *)filp->private_data;

	// DEBUG0("dsp ioctrl command: %d, arg %d\n", cmd, *(int __user*)arg);
	switch (cmd) {
	case SNDCTL_DSP_GETOSPACE:
		ret = dsp_ioctl_ospace_get(filp, (audio_buf_info *) arg);
		break;
	case SNDCTL_DSP_GETODELAY:
		ret = dsp_ioctl_odelay_get(filp, (int __user *)arg);
		break;
	case SNDCTL_DSP_GETISPACE:
		ret = dsp_ioctl_ispace_get(filp, (audio_buf_info *) arg);
		break;
	case SNDCTL_DSP_GETFMTS:
		ret = dsp_ioctl_fmt_get((int __user *)arg);
		break;
	case SNDCTL_DSP_SETFMT:
		ret = dsp_ioctl_fmt_set(filp, *(int __user *)arg);
		break;
	case SNDCTL_DSP_CHANNELS:
		ret = dsp_ioctl_channel_set(filp, *(int __user *)arg);
		break;
	case SNDCTL_DSP_STEREO:
		{
			int channel = (*(int __user *)arg) ? 2 : 1;
			ret = dsp_ioctl_channel_set(filp, channel);
		}
		break;
	case SNDCTL_DSP_SPEED:
		ret = dsp_ioctl_frequency_set(filp, *(int __user *)arg);
		break;
	case SNDCTL_DSP_SETFRAGMENT:
		ret = dsp_ioctl_fragment_set(filp, *(int __user *)arg);
		break;
	case SNDCTL_DSP_SETTRIGGER:
		ret = dsp_ioctl_trigger_set(filp, *(int __user *)arg);
		break;
	case SNDCTL_DSP_POST:
		ret = dsp_ioctl_trigger_set(filp, PCM_ENABLE_OUTPUT);
		break;
	case SNDCTL_DSP_HALT:
		dsp_ioctl_skip(filp); /* skip all samples */
		ret = dsp_ioctl_trigger_set(filp, 0);
		break;
	case SNDCTL_DSP_GETERROR:
		*(int __user *)arg = dev->error;
		break;
	case SNDCTL_DSP_NONBLOCK:
		dev->non_block = 1;
		break;
	case SNDCTL_DSP_SKIP:	/* Discards all samples in the playback buffer  */
		ret = dsp_ioctl_skip(filp);
		break;
	case SNDCTL_DSP_SYNC:	/* Suspend the application until all samples have been played */
		ret = dsp_ioctl_sync(filp);
		break;
	case SNDCTL_DSP_SILENCE:	/* Clears the playback buffer with silence */
		if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
			ret = dsp_ioctl_silence(filp);
		}
		break;
	case SNDCTL_DSP_GETCAPS:
		*(int __user *)arg = dev->caps;
		break;
	default:
		DEBUG0("[%s] Unknown IO control %d.\n", __FUNCTION__, cmd);
		break;
	}
	dev->error = ret;
	return ret;
}

static const struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.read = dsp_dev_read,
	.write = dsp_dev_write,
	.ioctl = dsp_dev_ioctl,
	.open = dsp_dev_open,
	.release = dsp_dev_release,
};

/**
 * @breif Setupte the device file.
 * @param dev The device driver structure.
 * @param index The device index.
 */
static void dsp_setup_cdev(struct dsp_dev_s *dev, int index)
{
	int err, devno = MKDEV(dsp_major, dsp_minor + index);

	cdev_init(&dev->cdev, &dev_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &dev_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		DEBUG0("mknod /dev/%s%d failed\n", DEVICE_NAME, index);
	}
	device_create(dsp_class, NULL, MKDEV(dsp_major, dsp_minor + index),
		      NULL, DEVICE_NAME);
}

/**
 * @brief display device release                                              
 * @param dev The device pointer
 */
static void dsp_device_release(struct device *dev)
{

//  printk("remove dsp device ok\n");
}

static struct platform_device dsp_device = {
	.name = "gp-dsp",
	.id = 0,
	.dev = {
		.release = dsp_device_release,
		},
};

#ifdef CONFIG_PM
static int dsp_suspend(struct platform_device *pdev, pm_message_t state)
{

	audio_suspend();
	return 0;
}

static int dsp_resume(struct platform_device *pdev)
{
	HAL_WRITE_UINT32((unsigned int)(APBDMAA_BASE+0x007C), 1);
	audio_resume();
	audio_Power_ctrl(1);
	audio_WAVE_volset(0x10, 0x10);	/*default volume to about 50% */

	return 0;
}

#else /*  */
#define dsp_suspend NULL
#define	dsp_resume NULL
#endif /*  */

/**                                                                         
 * @brief audio driver define                                               
 */
static struct platform_driver dsp_driver = {
	.suspend = dsp_suspend,
	.resume = dsp_resume,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "gp-dsp"}
};

/**
 * @bried Exit the DSP module.
 */
static void dsp_module_exit(void)
{
	int i;

	dev_t devno = MKDEV(dsp_major, dsp_minor);
	if (dsp_devices) {
		for (i = 0; i < dsp_nr_devs; i++) {
			device_destroy(dsp_class,
				       MKDEV(dsp_major, dsp_minor + i));
			cdev_del(&dsp_devices[i].cdev);
		}
		kfree(dsp_devices);
	}
	unregister_chrdev_region(devno, dsp_nr_devs);
	class_destroy(dsp_class);
	platform_device_unregister(&dsp_device);
	platform_driver_unregister(&dsp_driver);
}

/**
 * @breif Initialize the DSP module.
 */
static int dsp_module_init(void)
{
	int result, i;

	dev_t dev = 0;

	#if USE_FIQ == 1	
		gpHalVicIntSel(IRQ_APBDMA_A_CH0,5);
		gpHalVicIntSel(IRQ_APBDMA_A_CH1,5);
		printk("USE_FIQ Enable\n");
	#else
		printk("USE_FIQ Disable\n");
	#endif
	
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
		dsp_setup_cdev(&dsp_devices[i], i);
	}
	
//	HAL_WRITE_UINT32((unsigned int)(APBDMAA_BASE+DMAX_IRQ_STATUS), 3);
	HAL_WRITE_UINT32((unsigned int)(APBDMAA_BASE+0x007C), 1);
	
	audio_Power_ctrl(1);
	audio_WAVE_volset(0x10, 0x10);	/*default volume to about 50% */
	platform_device_register(&dsp_device);
	return platform_driver_register(&dsp_driver);
fail:	dsp_module_exit();
	return result;
}

module_init(dsp_module_init);
module_exit(dsp_module_exit);
MODULE_DESCRIPTION("DSP module");
