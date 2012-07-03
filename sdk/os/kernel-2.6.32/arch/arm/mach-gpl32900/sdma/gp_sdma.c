#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/typedef.h>
#include <mach/hal/hal_sdma.h>
#include <mach/hal/regmap/reg_sdma.h>
#include <mach/gp_chunkmem.h>
#include <mach/gp_sdma.h>
#include <mach/gp_cache.h>
#include <linux/delay.h>

#define _MODE_INTTERUPT_ 
#define SDMA_MAX_SIZE_LIMIT		(0xFFF80)//(0x80000) //512KB
#define CHANNEL_NUM 2

static SINT32 refCount = 0;
static SINT32 hw_status_ch[CHANNEL_NUM] = {0, 0};
static gpSdma_t sdma_ch[CHANNEL_NUM];
static DECLARE_MUTEX(gp_sdma_lock);
static DECLARE_MUTEX(gp_sdma_chan_lock);
static DECLARE_WAIT_QUEUE_HEAD(gp_sdma_done_wait);
static DECLARE_WAIT_QUEUE_HEAD(gp_sdma_freech_wait);
static SINT32 freeChan = -1;

static SINT32 gp_sdma_open(struct inode *ip, struct file *fp);
static SINT32 gp_sdma_release(struct inode *ip, struct file* fp);
static SINT32 gp_sdma_ioctl(struct inode *inode, struct file *flip, unsigned int cmd, unsigned long arg);
static SINT32 gp_sdma_memcpy_user(gp_Sdma_t* sdma );
static SINT32 gp_sdma_irq_done(SINT32 indexChan);
/*!<static SINT32 driver_use_test(void);*/
static SINT32 gp_sdma_perTrans(UINT8* src, UINT8* dst, SINT32 blockSize, SINT32 blockCnt);

gp_irqInfo_t irq_flag[] = {
	{SDMA_IRR_MLL_ENDMODE, "Irq Occured:End of List mode\n", 0},
	{SDMA_IRR_MLL_TRIGGER, "Irq Occured:Tigger of List mode\n", 0},
	{SDMA_IRR_MSIDX_END_HBLOCK, "Irq Occured:Source end of half block\n", 0},
	{SDMA_IRR_MSIDX_END_BLOCK, "Irq Occured:Source end of block\n", 0},
	{SDMA_IRR_MSIDX_END_FRAME, "Irq Occured:Source end of frame\n", 0},
	{SDMA_IRR_MSIDX_END_PACKET, "Irq Occured:Source end of packet\n", 0},
	{SDMA_IRR_MSIDX_END_INDEXMODE, "Irq Occured:Source end of Index mode\n", 0},
	{SDMA_IRR_MSERR, "Irq Occured:Source bus error\n", -1},
	{SDMA_IRR_MFIN, "Irq Occured:DMA finish\n", 0},
	{SDMA_IRR_MDIDX_END_HBLOCK, "Irq Occured:Dest end of half block\n", 0},
	{SDMA_IRR_MDIDX_END_BLOCK, "Irq Occured:Dest end of block\n", 0},
	{SDMA_IRR_MDIDX_END_FRAME, "Irq Occured:Dest end of frame\n", 0},
	{SDMA_IRR_MDIDX_END_PACKET, "Irq Occured:Dest end of packet\n", 0},
	{SDMA_IRR_MDIDX_END_INDEXMODE, "Irq Occured:Dest of index mode\n", 0},
	{SDMA_IRR_MDERR, "Irq Occured:Dest bus error\n", -1}};

static struct file_operations gp_sdma_fops = {
	.owner = THIS_MODULE,
	.open = gp_sdma_open,
	.release = gp_sdma_release,
    .ioctl = gp_sdma_ioctl,
};

static struct miscdevice gp_sdma_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sdma",
	.fops = &gp_sdma_fops,
};

static SINT32 
gp_sdma_open(
	struct inode *ip,
	struct file *fp
)
{
	SINT32 i = 0;
	
	if (refCount == 0) {
		gpHalSdmaInit();
		for (; i < CHANNEL_NUM; i++) {
			memset(&sdma_ch[i], 0, sizeof(gpSdma_t));
			gpHalMaskIrq(i);
		}
		
		refCount++;
	}
	else {
		refCount++;
	}
	
	return 0;
}

static SINT32 
gp_sdma_release(
	struct inode *ip,
	struct file* fp
) 
{
	if(refCount == 0) {
		gpHalSdmaUninit();
	}
	else {
		refCount--;
	}
	
	return 0;
}


static SINT32 
gp_sdma_ioctl(
	struct inode *inode, 
	struct file *flip, 
	unsigned int cmd, 
	unsigned long arg
)
{
	gp_Sdma_t *sdma = NULL;
	SINT32 ret = 0;
	switch(cmd)
	{
		case SDMA_IOCTL_TRIGGER:
			sdma = kmalloc(sizeof(gp_Sdma_t), GFP_KERNEL);
			if (sdma == NULL) {
				DIAG_ERROR("kmalloc error\n");
				break;
			}
			copy_from_user(sdma, (void __user*)arg, sizeof(gp_Sdma_t));
			ret = gp_sdma_memcpy_user(sdma);
			kfree(sdma);
		    break;
		default:
			DIAG_ERROR("unsupport cmd:%d\n", cmd);
			break;
	}
	
	return ret;
}

static SINT8 
gp_alloc_channel(
	void
)
{		
	SINT32 indexChan = -1;
	
	down_interruptible(&gp_sdma_chan_lock);
	if (sdma_ch[0].useFlag == 0) {
		sdma_ch[0].useFlag = 1;
		indexChan =  0;
	}
	else if (sdma_ch[1].useFlag == 0) {
		sdma_ch[1].useFlag = 1;
		indexChan = 1;
	}
	else {
		freeChan = indexChan;
	}
	up(&gp_sdma_chan_lock);
	
	return indexChan;
}

static SINT32 
gp_sdma_perTrans(
	UINT8* src,
	UINT8* dst,
	SINT32 blockSize,
	SINT32 blockCnt
)
{
	SINT8 indexChannel = 0;
	SINT32 ret = 0;
	
	indexChannel = gp_alloc_channel();
	if (indexChannel < 0) {
		wait_event_interruptible_timeout(gp_sdma_freech_wait, (freeChan != -1), HZ * 3);
		if (freeChan == -1) {
			return -ERROR_ALLOCCHAN_TIMEOUT;
		}
		indexChannel = freeChan;
	}
	if (gpHalCheckStatus(indexChannel, SDMA_STATUS_ACT)) {
		gpHalSdmaReset(indexChannel);
	}
	down_interruptible(&gp_sdma_lock);
	gpHalClearIrq(indexChannel);
	
	sdma_ch[indexChannel].srcAddr = src;
	sdma_ch[indexChannel].dstAddr = dst;
	sdma_ch[indexChannel].blockSize = blockSize;
	sdma_ch[indexChannel].bStepSize = 0;
	sdma_ch[indexChannel].frameSize = blockCnt;
	sdma_ch[indexChannel].fStepSize = 0;
	if (blockCnt != 0) {
		sdma_ch[indexChannel].packetSize = 1;
	}
	else {
		sdma_ch[indexChannel].packetSize = 0;
	}
	
	gpHalSdmaTrriger(indexChannel, &sdma_ch[indexChannel]);

	wait_event_interruptible_timeout(gp_sdma_done_wait, (hw_status_ch[indexChannel] != 0), HZ * 3);
	ret = gp_sdma_irq_done(indexChannel);
	up(&gp_sdma_lock);	
	
	return ret;
}

SINT32 gp_sdma_run(UINT8* src, UINT8* dst, SINT32 size) 
{
	SINT32 ret = 0;
	SINT32 blockCnt = size / SDMA_MAX_SIZE_LIMIT;
	SINT32 leaveSize = size % SDMA_MAX_SIZE_LIMIT;
#ifndef GP_SYNC_OPTION
	void* vAddrSrc = NULL;
	void* vAddrDst = NULL;
#endif
	
	#if 0
	while (blockCnt--) {
		gp_sync_cache();
		ret = gp_sdma_perTrans(src, dst, SDMA_MAX_SIZE_LIMIT, 0);
		gp_sync_cache();
		src += SDMA_MAX_SIZE_LIMIT;
		dst += SDMA_MAX_SIZE_LIMIT;
	}
	gp_sync_cache();
	ret = gp_sdma_perTrans(src, dst, leaveSize, 0);
	gp_sync_cache();
	#else
#ifndef GP_SYNC_OPTION
	vAddrSrc = gp_chunk_va((unsigned int)src);
	if (vAddrSrc == NULL) {
		DIAG_ERROR("srcAddr convert to virtual addr from phyAddr error\n");
		ret = -1;
		goto _ERROR_;
	}
	vAddrDst = gp_chunk_va((unsigned int)dst);
	if (vAddrDst == NULL) {
		DIAG_ERROR("srcAddr convert to virtual addr from phyAddr error\n");
		ret = -1;
		goto _ERROR_;
	}

	gp_clean_dcache_range((unsigned int)vAddrSrc, size);
	gp_clean_dcache_range((unsigned int)vAddrDst, size);
#else
	GP_SYNC_CACHE();
#endif
	if (blockCnt > 0) {
		ret = gp_sdma_perTrans(src, dst, SDMA_MAX_SIZE_LIMIT, blockCnt);
	}
#ifdef GP_SYNC_OPTION
	GP_SYNC_CACHE();
#endif
	if (leaveSize > 0) {
		src += (blockCnt * SDMA_MAX_SIZE_LIMIT);
		dst += (blockCnt * SDMA_MAX_SIZE_LIMIT);
		ret = gp_sdma_perTrans(src, dst, leaveSize, 0);
	}
#ifndef GP_SYNC_OPTION
	gp_invalidate_dcache_range((unsigned int)vAddrSrc, size);
	gp_invalidate_dcache_range((unsigned int)vAddrDst, size);
#else
	GP_SYNC_CACHE();
#endif
	#endif

	_ERROR_:
	return ret;
}

SINT32
gp_sdma_memcpy_kernel(
	gp_Sdma_t* sdma 
)
{
	UINT8* srcPhyAddr;
	UINT8* dstPhyAddr;
	SINT32 ret = 0;
	
	/*!<convert address vir to phy*/
	srcPhyAddr = (UINT8*)gp_chunk_pa((void*)sdma->srcAdress);
	if (srcPhyAddr == NULL) {
		DIAG_ERROR("source address convert to phy error\n");
		return -1;
	}
	dstPhyAddr = (UINT8*)gp_chunk_pa((void*)sdma->dstAdress);
	if (dstPhyAddr == NULL) {
		DIAG_ERROR("destination address convert to phy error\n");
		return -1;
	}
	if (sdma->size <= 0) {
		DIAG_ERROR("invalid data size\n");
		return -1;
	}
	ret = gp_sdma_run(srcPhyAddr, dstPhyAddr, sdma->size);
	
	return ret;
}
EXPORT_SYMBOL(gp_sdma_memcpy_kernel);

SINT32
gp_sdma_memcpy_user(
	gp_Sdma_t* sdma 
)
{
	UINT8* srcPhyAddr;
	UINT8* dstPhyAddr;
	SINT32 ret = 0;
	
	/*!<convert address vir to phy*/
	srcPhyAddr = (UINT8*)gp_user_va_to_pa((void*)sdma->srcAdress);
	if (srcPhyAddr == NULL) {
		DIAG_ERROR("source address convert to phy error\n");
		return -1;
	}
	dstPhyAddr = (UINT8*)gp_user_va_to_pa((void*)sdma->dstAdress);
	if (dstPhyAddr == NULL) {
		DIAG_ERROR("destination address convert to phy error\n");
		return -1;
	}
	if (sdma->size <= 0) {
		DIAG_ERROR("invalid data size\n");
		return -1;
	}
	ret = gp_sdma_run(srcPhyAddr, dstPhyAddr, sdma->size);
	
	return ret;
}

static SINT32 
gp_sdma_detect_irq(
	SINT8 indexChan
)
{
	SINT32 irqStatus = gpHalGetIrq(indexChan);
	
	/*!<No interrupt occured*/
	if (!irqStatus) {
		return 0;
	}
	
	hw_status_ch[indexChan] = irqStatus;
	
	return 1;
}

static irqreturn_t 
gp_sdma_irq(
	int irq,
	void *dev_id
)
{
	SINT8 indexChannel = -1;

	#if 1
	if (!gp_sdma_detect_irq(0) && (!gp_sdma_detect_irq(1))) {
		return IRQ_HANDLED;
	}

	/*!<if there is a idle channel,spin_unlock*/
	if (gp_sdma_detect_irq(0) ) {
		indexChannel = 0;
		gpHalClearIrq(indexChannel);
		wake_up_interruptible(&gp_sdma_done_wait);
	}
	
	if (gp_sdma_detect_irq(1) ) {
		indexChannel = 1;
		gpHalClearIrq(indexChannel);
		wake_up_interruptible(&gp_sdma_done_wait);
	}
	#endif
	
	return IRQ_HANDLED;
}

static SINT32 
gp_sdma_irq_done(
	SINT32 indexChannel
)
{
	SINT32 status, ret;
	SINT32 i;
	
	down_interruptible(&gp_sdma_chan_lock);
	status = hw_status_ch[indexChannel];
	ret = -ERROR_TRANSFER_TIMEOUT;
	for (i = 0; i < sizeof(irq_flag) / sizeof(gp_irqInfo_t); i++) {
		if (status & irq_flag[i].bitMask) {
			//DIAG_INFO("%s", irq_flag[i].info);
			ret = irq_flag[i].meaning;
		}
	}
	#if 0
	switch(status) 
	{
		case ERROR_DSTBUS:
			DIAG_ERROR("a destination bus erro occured\n");
			ret = -1;
			break;
		case DMA_FINISH:
			DIAG_INFO("DMA transferd finish\n");
			ret = 0;
			break;
		case ERROR_SRCBUS:
			DIAG_ERROR("a source bus erro occured\n");
			ret = -1;
			break;
		case END_DST_BLOCK:
			DIAG_INFO("DMA frame finish\n");
		    ret = 0;
			break;
	}
	#endif
	
	hw_status_ch[indexChannel] = 0;
	
	sdma_ch[indexChannel].useFlag = 0;
	if (freeChan  == -1) {
		freeChan = indexChannel;
		wake_up_interruptible(&gp_sdma_freech_wait);
	}
	up(&gp_sdma_chan_lock);
	
	return ret;
}

static int 
gp_sdma_probe(
	struct platform_device *pdev
)
{
	int ret= -ENOENT;
	struct device *dev = &pdev->dev;

	#ifdef _MODE_INTTERUPT_
	ret = request_irq(IRQ_DMAC0_M410_CH0, gp_sdma_irq, IRQF_DISABLED, "SDMA_CH0_IRQ", dev);
	if (ret < 0) {
		DIAG_ERROR("sdma can't get irq %i, err %d\n", IRQ_DMAC0_M410_CH0, ret);
		return ret;
	}

	ret = request_irq(IRQ_DMAC0_M410_CH1, gp_sdma_irq, IRQF_DISABLED, "SDMA_CH1_IRQ", dev);
	if (ret < 0) {
		DIAG_ERROR("sdma can't get irq %i, err %d\n", IRQ_DMAC0_M410_CH0, ret);
		return ret;
	}
	#endif
	if((ret = misc_register(&gp_sdma_misc))) {
		DIAG_ERROR("misc_register returned %d in goldfish_audio_init\n", ret);
		free_irq(IRQ_DMAC0_M410_CH0, dev);
		free_irq(IRQ_DMAC0_M410_CH1, dev);
		return ret;
	}

	return 0;
}

static int 
gp_sdma_remove(
	struct platform_device *pdev
)
{
	misc_deregister(&gp_sdma_misc);
	#ifdef _MODE_INTTERUPT_
	free_irq(IRQ_DMAC0_M410_CH0, &pdev->dev);
	free_irq(IRQ_DMAC0_M410_CH1, &pdev->dev);
	#endif
	return 0;
}

static struct platform_driver gp_sdma_driver = {
	.probe		= gp_sdma_probe,
	.remove		= gp_sdma_remove,
	.suspend    = NULL,
	.resume     = NULL,
	.driver = {
		.name = "gp_sdma"
	}
};

struct platform_device gp_sdma_device = {
	.name	= "gp_sdma",
	.id		= -1,
};


#if 0
static SINT32 
driver_use_test(
	void
)
{
	SINT8* src = NULL;
	SINT8* dst = NULL;
	gp_Sdma_t sdma;
	
	src = gp_chunk_malloc(current->tgid, 860 * 480 * 4);
	if (src == NULL) {
		DIAG_ERROR("src alloc error\n");
	}
	src[65536] = 119;
	dst = gp_chunk_malloc(current->tgid, 860 * 480 * 4);
	if (dst == NULL) {
		DIAG_ERROR("src alloc error\n");
	}
	
	sdma.srcAdress = src;
	sdma.dstAdress = dst;
	sdma.size = 860 * 480 * 4;
	if (gp_sdma_memcpy_kernel(&sdma) < 0) {
		DIAG_ERROR("kernel sdma error\n");
	}
	
	DIAG_INFO("data:%d\n", dst[65536]);
	if (src != NULL) {
		gp_chunk_free(src);
	}
	if (dst != NULL) {
		gp_chunk_free(dst);
	}

	return 0;
}
#endif

static int __init 
gp_sdma_init(
	void
)
{
	int ret;

	ret = platform_driver_register(&gp_sdma_driver);
	if (ret < 0) {
		DIAG_ERROR("platform_driver_register returned %d\n", ret);
		return ret;
	}

	ret = platform_device_register(&gp_sdma_device);
	if (ret) {
		dev_err(&(gp_sdma_device.dev), "unable to register device: %d\n", ret);
	}
	
	return ret;
}

static void __exit 
gp_sdma_exit(
	void
)
{
	platform_device_unregister(&gp_sdma_device);
	platform_driver_unregister(&gp_sdma_driver);
}

module_init(gp_sdma_init);
module_exit(gp_sdma_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GPL32900 SDMA Driver");
MODULE_LICENSE_GP;
