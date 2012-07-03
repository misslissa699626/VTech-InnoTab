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
#include <mach/module.h>
#include <mach/kernel.h>
#include <mach/cdev.h>
#include <linux/cdev.h>

#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <mach/sensor_mgr.h>
#include <mach/hal/hal_mipi.h>
#include <mach/gp_mipi.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define RETURN(x)	{nRet = x; goto __return;}
#define DERROR	printk 
#if 1
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpMipiDev_s 
{
	struct miscdevice dev;
	struct semaphore  sem;
	unsigned char 	open_cnt;
	unsigned char	start_flag;
	unsigned char	reserved0;
	unsigned char	reserved1;
	gpMipiCfg_t		cfg;
	gpMipiEcc_t		ecc;
	gpMipiCCIR601_t	ccir601;
} gpMipiDev_t;
	
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gpMipiDev_t *p_mipi_dev;

static void 
gp_mipi_init_para(
	gpMipiDev_t *argp
)
{
	argp->cfg.mipi_sep_clk_en = ENABLE;
	argp->cfg.mipi_sep_clk = 200000000;
	argp->cfg.mipi_sep_clk_src = MIPI_D_PHY_CLK;
	argp->cfg.byte_clk_edge = D_PHY_SAMPLE_POS;
	
	argp->cfg.low_power_en = DISABLE;
	argp->cfg.lane_num = MIPI_1_LANE;

	argp->ecc.ecc_check_en = ENABLE;
	argp->ecc.ecc_order = MIPI_ECC_ORDER3;
	argp->ecc.da_mask_cnt = 0x08;
	argp->ecc.check_hs_seq = MIPI_CHECK_HS_SEQ;

	argp->ccir601.data_from_mmr = DISABLE;
	argp->ccir601.data_type = MIPI_RAW8;
	argp->ccir601.data_type_to_cdsp = DISABLE;
	argp->ccir601.h_size = 2048;
	argp->ccir601.v_size = 1536;
	argp->ccir601.h_back_porch = 0x0;
	argp->ccir601.h_front_porch = 0x4;
	argp->ccir601.blanking_line_en = ENABLE;
}

static void
gp_mipi_set_cfg(
	gpMipiCfg_t	*argp
)
{
	if(argp->mipi_sep_clk_en) {
		int div;
		int clock_out;
		struct clk *clock;

		clock = clk_get(NULL, "clk_ref_ceva");
	    clock_out = clk_get_rate(clock);
		div = clock_out/argp->mipi_sep_clk;
		gpHalMipiSetSepPClk(ENABLE, div, argp->mipi_sep_clk_src);
	} else {
		gpHalMipiSetSepPClk(DISABLE, 1, 0);
	}

	gpHalMipiSetGloblaCfg(ENABLE, argp->low_power_en, argp->lane_num, argp->mipi_sep_clk_en);
}


static void
gp_mipi_set_ecc(
	gpMipiEcc_t	*argp
)
{
	gpHalMipiSetEcc(argp->ecc_order, argp->ecc_check_en, argp->da_mask_cnt, argp->check_hs_seq);
}

static void
gp_mipi_set_ccir601(
	gpMipiCCIR601_t	*argp
)
{
	if(argp->data_from_mmr) {
		gpHalMipiSetDataFmt(argp->data_from_mmr, argp->data_type, argp->data_type_to_cdsp);
	} else {
		gpHalMipiSetDataFmt(DISABLE, 0, argp->data_type_to_cdsp);
	}
	
	if(argp->data_type == MIPI_YUV422) {
		gpHalMipiSetImageSize(1, argp->h_size << 1, argp->v_size);
	} else {
		gpHalMipiSetImageSize(0, argp->h_size, argp->v_size);
	}
	
	gpHalMipiSetCCIR601IF(argp->h_back_porch, argp->h_front_porch, argp->blanking_line_en);
}

static long 
gp_mipi_ioctl(
	struct file *filp, 
	unsigned int cmd, 
	unsigned long arg
)
{
	int nRet = 0;
	
	if(down_interruptible(&p_mipi_dev->sem) != 0) {
		return -ERESTARTSYS;
	}
	
	switch(cmd)
	{
	case MIPI_IOCTL_S_CFG:
		nRet = copy_from_user((void *)&p_mipi_dev->cfg, (void *)arg, sizeof(gpMipiCfg_t));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if(p_mipi_dev->start_flag) {
			gp_mipi_set_cfg(&p_mipi_dev->cfg);
		}
		break;
		
	case MIPI_IOCTL_G_CFG:
		nRet = copy_to_user((void *)arg, (void *)&p_mipi_dev->cfg, sizeof(gpMipiCfg_t));
		break;
		
	case MIPI_IOCTL_S_ECC:
		nRet = copy_from_user((void *)&p_mipi_dev->ecc, (void *)arg, sizeof(gpMipiEcc_t));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}
		
		if(p_mipi_dev->start_flag) {
			gp_mipi_set_ecc(&p_mipi_dev->ecc);
		}
		break;
	case MIPI_IOCTL_G_ECC:
		nRet = copy_to_user((void *)arg, (void *)&p_mipi_dev->ecc, sizeof(gpMipiEcc_t));
		break;
		
	case MIPI_IOCTL_S_CCIR601:
		nRet = copy_from_user((void *)&p_mipi_dev->ccir601, (void *)arg, sizeof(gpMipiCCIR601_t));
		if(nRet < 0) {
			RETURN(-EINVAL);
		}

		if(p_mipi_dev->start_flag) {
			gp_mipi_set_ccir601(&p_mipi_dev->ccir601);
		}
		break;
		
	case MIPI_IOCTL_G_CCIR601:
		nRet = copy_to_user((void *)arg, (void *)&p_mipi_dev->ccir601, sizeof(gpMipiCCIR601_t));
		break;
		
	case MIPI_IOCTL_S_START:
		if(arg) {
			char sensor_name[32];
			int i;
			
			nRet = copy_from_user((void *)sensor_name, (void *)arg, sizeof(sensor_name));
			if(nRet < 0) {
				RETURN(-EINVAL);
			}
			
			for(i=0; i<4; i++) {
				struct v4l2_subdev *sd;
				callbackfunc_t *cb;
				char *port;
				sensor_config_t *sensor;
				
				nRet = gp_get_sensorinfo(i, (int*)&sd, (int*)&cb, (int*)&port, (int*)&sensor);
				if(nRet < 0) {
					RETURN(-EINVAL);
				}
				
				if(strcmp(sensor_name, sd->name) == 0) {
					printk("mipi setting = %s\n", sensor_name);
					p_mipi_dev->cfg.mipi_sep_clk_en = sensor->mipi_config->mipi_sep_clk_en;
					p_mipi_dev->cfg.mipi_sep_clk = sensor->mipi_config->mipi_sep_clk;
					p_mipi_dev->cfg.mipi_sep_clk_src = sensor->mipi_config->mipi_sep_clk_src;
					p_mipi_dev->cfg.byte_clk_edge = sensor->mipi_config->byte_clk_edge;
					p_mipi_dev->cfg.low_power_en = sensor->mipi_config->low_power_en;
					p_mipi_dev->cfg.lane_num = sensor->mipi_config->lane_num;

					p_mipi_dev->ecc.ecc_check_en = sensor->mipi_config->ecc_check_en;
					p_mipi_dev->ecc.ecc_order = sensor->mipi_config->ecc_order;
					p_mipi_dev->ecc.da_mask_cnt = sensor->mipi_config->da_mask_cnt;
					p_mipi_dev->ecc.check_hs_seq = sensor->mipi_config->check_hs_seq;
					break;
				}
			}
		} 
		
		gp_mipi_set_cfg(&p_mipi_dev->cfg);
		gp_mipi_set_ecc(&p_mipi_dev->ecc);
		gp_mipi_set_ccir601(&p_mipi_dev->ccir601);
		p_mipi_dev->start_flag = 1;
		break;
		
	default:
		RETURN(-ENOTTY);	/* Inappropriate ioctl for device */
	}

__return:
	up(&p_mipi_dev->sem);
	return nRet; 
}

static int 
gp_mipi_open(
	struct inode *inode, 
	struct file *filp
)
{
	if(p_mipi_dev->open_cnt == 0) {
		DEBUG(KERN_WARNING "MipiOpen\n");
		filp->private_data = p_mipi_dev;		
		p_mipi_dev->open_cnt = 1;
		p_mipi_dev->start_flag = 0;
		gp_mipi_init_para(p_mipi_dev);		
		gpHalMipiSetModuleClk(ENABLE);
		gpHalMipiSetGloblaCfg(ENABLE, DISABLE, MIPI_1_LANE, MIPI_D_PHY_CLK);
		return 0;
	}
	DERROR(KERN_WARNING "MipiOpenFail\n");
	return -1;
}

static int 
gp_mipi_release(
	struct inode *inode, 
	struct file *filp
)
{
	if(p_mipi_dev->open_cnt == 1) {
		DEBUG(KERN_WARNING "MipiClose\n");
		p_mipi_dev->open_cnt = 0;
		p_mipi_dev->start_flag = 0;
		gpHalMipiSetSepPClk(DISABLE, 1, 0);
		gpHalMipiSetGloblaCfg(DISABLE, DISABLE, MIPI_1_LANE, MIPI_D_PHY_CLK);
		gpHalMipiSetModuleClk(DISABLE);
		return 0;
	}
	DERROR(KERN_WARNING "MipiCloseFail\n");
	return -1;
}

struct file_operations mipi_fops = 
{
	.owner = THIS_MODULE, 
	.unlocked_ioctl = gp_mipi_ioctl,
	.open = gp_mipi_open,
	.release = gp_mipi_release,
};

static void 
gp_mipi_device_release(
	struct device *dev
)                       
{                                                                           
	DEBUG("remove mipi device ok\n");                                      
}                                                                           
                                                                            
static struct platform_device gp_mipi_device = {                             
	.name = "gp-mipi",                                                         
	.id	= 0,                                                                  
	.dev = 
	{                                                                 
		.release = gp_mipi_device_release,                                       
	},                                                                        
};

#ifdef CONFIG_PM
static int 
gp_mipi_suspend(
	struct platform_device *pdev, 
	pm_message_t state)
{
	if(p_mipi_dev->open_cnt > 0) {
		if(down_interruptible(&p_mipi_dev->sem) != 0) {
			return -ERESTARTSYS;
		}
		
		gpHalMipiSetModuleClk(DISABLE);
		up(&p_mipi_dev->sem);
	}
	return 0;
}

static int 
gp_mipi_resume(
	struct platform_device *pdev
)
{
	if(p_mipi_dev->open_cnt > 0) {
		gpHalMipiSetModuleClk(ENABLE);
	}
	return 0;
}
#else
#define gp_mipi_suspend NULL
#define gp_mipi_resume NULL
#endif

static struct platform_driver gp_mipi_driver = 
{
	.suspend = gp_mipi_suspend,
	.resume = gp_mipi_resume,
	.driver	= 
	{
		.owner	= THIS_MODULE,
		.name	= "gp-mipi"
	},
};

static int __init 
mipi_init_module(
	void
)
{
	int nRet = -ENOMEM;
	
	DEBUG(KERN_WARNING "ModuleInit: mipi\n");
	p_mipi_dev = (gpMipiDev_t *)kzalloc(sizeof(gpMipiDev_t), GFP_KERNEL);
	if(p_mipi_dev == NULL) {
		DERROR("mipi kmalloc fail\n");
		RETURN(-ENOMEM);
	}

	/* initialize */
	init_MUTEX(&p_mipi_dev->sem);

	/* register char device */
	p_mipi_dev->dev.name  = "mipi";
	p_mipi_dev->dev.minor = MISC_DYNAMIC_MINOR;
	p_mipi_dev->dev.fops  = &mipi_fops;
	nRet = misc_register(&p_mipi_dev->dev);
	if(nRet != 0) {
		DERROR("mipi device register fail\n");
		RETURN(-ENXIO);
	}
	
	/* register platform driver */
	platform_device_register(&gp_mipi_device);
	platform_driver_register(&gp_mipi_driver);
	
__return:
	if(nRet < 0) {
		DERROR(KERN_WARNING "MipiInitFail\n");
		kfree(p_mipi_dev);
		p_mipi_dev = NULL;
	}
	return nRet;
}

static void __exit 
mipi_exit_module(
	void
)
{
	/* free char device */
	misc_deregister(&p_mipi_dev->dev);
	kfree(p_mipi_dev);
	p_mipi_dev = NULL;

	platform_device_unregister(&gp_mipi_device);
	platform_driver_unregister(&gp_mipi_driver);
}

module_init(mipi_init_module);
module_exit(mipi_exit_module);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus MIPI Driver");
MODULE_LICENSE_GP;
MODULE_VERSION("1.0");



