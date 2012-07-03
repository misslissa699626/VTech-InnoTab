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
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_spi.c
 * @brief SPI driver interface 
 * @author zaimingmeng
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/hal/hal_spi.h>
#include <mach/gp_spi.h>
#include <mach/gp_board.h>
#include <mach/hal/hal_clock.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define SPI_NAME	"spi"


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_spi_s{
	struct miscdevice dev; 	 /*!< @brief spi device */

	struct semaphore spi_sem;
	int pin_handle;
}gp_spi_t;

typedef struct gp_spi_info_s {
	UINT32 id;		/*!< @brief spi channel index */
	UINT8 isOpened;
	UINT32 freq;		/*!< @brief spi freq value */

	UINT8 dam_mode;		/*!< @brief spi enable/disable dam mode */
	UINT8 clk_pol;		/*!< @brief spi clock polarity at idles state */
	UINT8 clk_pha;		/*!< @brief spi clock phase,data occur at odd/even edge of SCLK clock */
	UINT8 lsbf;		/*!< @brief spi LSB or MSB first */
} gp_spi_info_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct gp_spi_s *gp_spi_data;     	 /*!< @brief spi device */


/**
 * @brief spi setup function,set the spi register
 * @param handle [in] spi handle
 * @return SP_OK(0)/ERROR
 */
static int gp_spi_setup(int handle)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	gpHalSpiDmaCtrl(spi->dam_mode);
	gpHalSpiClkPol(spi->clk_pol);
	gpHalSpiClkPha(spi->clk_pha);
	gpHalSpiSetLsb(spi->lsbf);

	gpHalSpiSetFreq(spi->freq);
	return 0;
}

/**
 * @brief spi request function
 * @param id [in] spi channel index
 * @return  handle/NULL
 */
int gp_spi_request(int id)
{
	struct gp_spi_info_s *spi;

	if(!gp_spi_data){
		return 0;
	}
	
	if(id > 3){
		return 0;
	}

	spi = (struct gp_spi_info_s *)kmalloc(sizeof(gp_spi_info_t), GFP_KERNEL);
	if(NULL == spi){
		return 0;
	}
	memset(spi, 0, sizeof(gp_spi_info_t));

	spi->id = id;
	
	return (int)spi;
}
EXPORT_SYMBOL(gp_spi_request);

/**
 * @brief spi release function
 * @param handle [in] spi handle
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_release(int handle)
{
	struct gp_spi_info_s *spi;

	if(0 == handle){
		return -EINVAL;
	}

	spi = (struct gp_spi_info_s *)handle;

	kfree(spi);

	return 0;
}
EXPORT_SYMBOL(gp_spi_release);

/*
 *@brief spi cs enable function
 *@param handle[in]:spi handle
 *@return :SP_OK(0)/ERROR
 */
int gp_spi_cs_enable(int handle)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	down(&gp_spi_data->spi_sem);

	spi->isOpened = 1;
	gp_spi_setup(handle);
	gpHalSpiCsEnable(spi->id);
	
	return 0;
}
EXPORT_SYMBOL(gp_spi_cs_enable);

/*
 *@brief spi cs disable function
 *@param handle[in]:spi handle
 *@return :SP_OK(0)/ERROR
 */
int gp_spi_cs_disable(int handle)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	spi->isOpened = 0;
	gpHalSpiCsDisable(spi->id);

	up(&gp_spi_data->spi_sem);

	return 0;
}
EXPORT_SYMBOL(gp_spi_cs_disable);

/**
 * @brief set spi dma mode function
 * @param handle [in] spi handle
 * @param mode [in] 0:disable dma; 1:enable dma
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_spi_set_dma_mode(int handle,int mode)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	spi->dam_mode = mode;

	return 0;
}
EXPORT_SYMBOL(gp_spi_set_dma_mode);

/**
 * @brief set spi clock polarity function
 * @param handle [in] spi handle
 * @param mode [in] 0:SCLK idles at low state; 1:SLCK idles at higt state
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_clk_pol(int handle,int mode)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	spi->clk_pol = mode;

	return 0;
}
EXPORT_SYMBOL(gp_spi_set_clk_pol);

/**
 * @brief set spi clock phase function
 * @param handle [in] spi handle
 * @param mode [in] 0:odd edge of SCLK clok; 1:even edge of SLCK clok
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_clk_pha(int handle,int mode)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	spi->clk_pha = mode;

	return 0;
}
EXPORT_SYMBOL(gp_spi_set_clk_pha);


/**
 * @brief set spi LSB/MSB first set function
 * @param handle [in] spi handle
 * @param mode [in] 0:MSB first; 1:LSB first
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_lsb(int handle,int mode)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	spi->lsbf = mode;

	return 0;
}
EXPORT_SYMBOL(gp_spi_set_lsb);

/**
 * @brief set spi freq function
 * @param handle [in] spi handle
 * @param freq [in] freq value(Hz)
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_freq(int handle,int freq)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	spi->freq = freq;

	return 0;
}
EXPORT_SYMBOL(gp_spi_set_freq);

/**
 * @brief set spi interrupt enable mode function
 * @param handle [in] spi handle
 * @param mode [in] interrupt mode
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_set_enable_int(int handle,int mode)
{
	if(0 == handle)
		return -EINVAL;
	gpHalSpiSetIER(mode);

	return 0;
	
}
EXPORT_SYMBOL(gp_spi_set_enable_int);

/**
 * @brief get spi interrupt state function
 * @param handle [in] spi handle
 * @param state [out] interrupt mode
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_get_int_state(int handle,int *state)
{
	if(0 == handle)
		return -EINVAL;

	*state = gpHalSpiGetIIR();

	return 0;
	
}
EXPORT_SYMBOL(gp_spi_get_int_state);

/**
 * @brief spi write function
 * @param handle [in] spi handle
 * @param buffer [in] write data buffer
 * @param len [in] size of buffer
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_write(int handle,char *buffer,int len)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -EINVAL;

	spi = (struct gp_spi_info_s *)handle;

	if(0 == spi->isOpened)
		return -EINVAL;

	gpHalSpiWrite(buffer,len);
	return 0;
}
EXPORT_SYMBOL(gp_spi_write);

/**
 * @brief spi read function
 * @param handle [in] spi handle
 * @param buffer [out] read data buffer
 * @param len [in] size of buffer
 * @return SP_OK(0)/ERROR_ID
 */
int gp_spi_read(int handle,char *buffer,int len)
{
	struct gp_spi_info_s *spi;

	if(0 == handle)
		return -1;

	spi = (struct gp_spi_info_s *)handle;

	if(0 == spi->isOpened)
		return -1;
	
	gpHalSpiRead(buffer,len);
	
	return 0;
}
EXPORT_SYMBOL(gp_spi_read);

/**
 * @brief   spi device open
 */
static int gp_spi_dev_open(struct inode *inode, struct file *file)
{
	struct gp_spi_info_s *spi;

	if(!gp_spi_data){
		return -EBUSY;
	}

	spi = (struct gp_spi_info_s *)kmalloc(sizeof(gp_spi_info_t), GFP_KERNEL);
	if(NULL == spi){
		return -ENOMEM;
	}
	memset(spi, 0, sizeof(gp_spi_info_t));

	file->private_data = spi;
	return 0;
}

/**
 * @brief   spi device close
 */
static int gp_spi_dev_release(struct inode *inode, struct file *file)
{
	struct gp_spi_info_s *spi;

	if(0 == file->private_data){
		return 0;
	}
	
	spi = (struct gp_spi_info_s *)file->private_data;
	
	kfree(spi);
	
	file->private_data = NULL;
	return 0;
}

/**
 * @brief   spi device ioctl
 */
static long gp_spi_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct gp_spi_info_s *spi;

	switch(cmd){
	
	case SPI_IOCTL_CS_ENABLE:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_cs_enable((int)file->private_data);
		}
		break;

	case SPI_IOCTL_CS_DISABLE:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_cs_disable((int)file->private_data);
		}
		break;

	case SPI_IOCTL_SET_FREQ:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_set_freq((int)file->private_data,arg);
		}

		break;

	case SPI_IOCTL_SET_CHANNEL:
		
		spi = (struct gp_spi_info_s *)file->private_data;
		if(arg > 3){
			ret = -EFAULT;
		}
		else{
			spi->id = arg;
		}

		break;

	case SPI_IOCTL_SET_CLK_POL:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_set_clk_pol((int)file->private_data,arg);
		}

		break;

	case SPI_IOCTL_SET_CLK_PHA:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_set_clk_pha((int)file->private_data,arg);
		}

		break;

	case SPI_IOCTL_SET_LSBF:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_set_lsb((int)file->private_data,arg);
		}

		break;

	case SPI_IOCTL_SET_DMA:
		if(file->private_data == NULL){
			ret = -EFAULT;
		}
		else{
			gp_spi_set_dma_mode((int)file->private_data,arg);
		}

		break;

	default:
		break;
	}
	return ret;
}

/**
 * @brief   spi device read
 */
static ssize_t gp_spi_dev_read(struct file *file, char __user *buf,size_t count, loff_t *oppos)
{
	int ret = 0;
	char *recvBuf = NULL;

	if(NULL == file->private_data){
		DIAG_INFO("gp_spi_read erro\n");
		return -EINVAL;
	}
	
	recvBuf = (char *)kmalloc(count, GFP_KERNEL);

	if(NULL == recvBuf){
		DIAG_INFO("gp_spi_read alloc mem fail\n");
		return -ENOMEM;
	}

	gp_spi_read((int)file->private_data, recvBuf, count);

	if(copy_to_user(buf,recvBuf,count)){
		DIAG_INFO("gp_spi_read copy fail\n");
		ret = -EFAULT;
	}

	kfree(recvBuf);

	return ret;
}

/**
 * @brief   spi device write
 */
static ssize_t gp_spi_dev_write(struct file *file, const char __user *buf,size_t count, loff_t *oppos)
{
	int ret = 0;
	char *sendBuf = NULL;

	if(NULL == file->private_data){
		DIAG_INFO("gp_spi_writ erro\n");
		return -EINVAL;
	}
	
	sendBuf = (char *)kmalloc(count, GFP_KERNEL);

	if(NULL == sendBuf){
		DIAG_INFO("gp_spi_write alloc fail\n");
		return -ENOMEM;
	}

	if(copy_from_user(sendBuf,buf,count)){
		ret = -EFAULT;
		DIAG_INFO("gp_spi_write copy fail\n");
		goto __erroCopy;
	}

	gp_spi_write((int)file->private_data, sendBuf, count);

	
__erroCopy:
	kfree(sendBuf);
	return ret;
}

struct file_operations spi_fops = {
	.owner          = THIS_MODULE,
	.open           = gp_spi_dev_open,
	.release        = gp_spi_dev_release,
	.unlocked_ioctl = gp_spi_dev_ioctl,
	.read		= gp_spi_dev_read,
	.write		= gp_spi_dev_write,
};

static void gp_spi_device_release(struct device *dev)                       
{                                                                           
}  

/**
 * @brief   spi device resource
 */
static struct platform_device gp_spi_device = {
	.name	= "gp-spi",
	.id	= 0,
	.dev	= {                                                                 
		.release = gp_spi_device_release,                                       
	},  
};

/**
 * @brief   spi device probe
 */
static int gp_spi_probe(struct platform_device *pdev)
{
	int ret = 0;

	gp_spi_data = kzalloc(sizeof(gp_spi_t), GFP_KERNEL);
	if(!gp_spi_data){
		return -ENOMEM;
	}
	memset(gp_spi_data, 0, sizeof(gp_spi_t));

	gp_spi_data->pin_handle = gp_board_pin_func_request(GP_PIN_SPI, GP_BOARD_WAIT_FOREVER);
	if (gp_spi_data->pin_handle < 0) {
		DIAG_ERROR("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		ret = -EINVAL;
		goto fail_pin;
	}

	gp_spi_data->dev.name = SPI_NAME;
	gp_spi_data->dev.minor  = MISC_DYNAMIC_MINOR;
	gp_spi_data->dev.fops  = &spi_fops;

	gpHalSpiClkEnable(&gp_spi_device.dev, 1);
	
	ret = misc_register(&gp_spi_data->dev);
	if(ret != 0){
		DIAG_ERROR("spi probe register fail\n");
		goto fail_reg;
	}
	
	platform_set_drvdata(pdev,&gp_spi_data);

	init_MUTEX(&gp_spi_data->spi_sem);

	DIAG_INFO("spi probe ok\n");
	return 0;

fail_reg:
	gp_board_pin_func_release(gp_spi_data->pin_handle);
	gpHalSpiClkEnable(&gp_spi_device.dev, 0);

fail_pin:
	kfree(gp_spi_data);

	return ret;
}

/**
 * @brief   spi device remove
 */
static int gp_spi_remove(struct platform_device *pdev)
{
	gp_board_pin_func_release(gp_spi_data->pin_handle);
	gpHalSpiClkEnable(&gp_spi_device.dev, 0);
	
	misc_deregister(&gp_spi_data->dev);
	kfree(gp_spi_data);
	return 0;
}

#ifdef CONFIG_PM
static int gp_spi_suspend(struct platform_device *pdev, pm_message_t state){
	gpHalScuClkEnable(1<<16, 2, 0);
	return 0;
}

static int gp_spi_resume(struct platform_device *pdev){
	gpHalScuClkEnable(1<<16, 2, 1);
	return 0;
}
#else
#define gp_spi_suspend NULL
#define gp_spi_resume NULL
#endif

static struct platform_driver gp_spi_driver = {
	.probe	= gp_spi_probe,
	.remove	= gp_spi_remove,
	.suspend = gp_spi_suspend,
	.resume = gp_spi_resume,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "gp-spi"
	},
};

/**
 * @brief   spi driver init
 */
static int __init gp_spi_drv_init(void)
{
	platform_device_register(&gp_spi_device);
	return platform_driver_register(&gp_spi_driver);
}

/**
 * @brief   spi driver exit
 */
static void __exit gp_spi_drv_exit(void)
{
	platform_device_unregister(&gp_spi_device);
	platform_driver_unregister(&gp_spi_driver);
}


module_init(gp_spi_drv_init);
module_exit(gp_spi_drv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP SPI Driver");
MODULE_LICENSE_GP;
