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
 *  3F, No.8, Dusing Rd., Science-Based Industrial Park,                  *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
 
 /**
 * @file gp_csi.c
 * @brief CSI interface
 * @author Simon Hsu
 */

#include <media/v4l2-dev.h>
#include <media/v4l2-subdev.h>

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/gp_csi.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_csi.h>
#include <mach/gp_chunkmem.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	CSI_MINOR		0
#define CSI_NR_DEVS		1
#define CSI_MAX_BUF		3
#define CSI_MAX_QUE		3
#define	CSI_MAX_SBDEV	5
#define	CSI_PINNUM		12

static const uint32_t CSI_PINIO[CSI_PINNUM]	= {((0<<24)|(1<<16)|(0<<8)|10),	/* CSI MCLK pin */  
												((0<<24)|(1<<16)|(0<<8)|11),	/* CSI PCLK pin */  
												((0<<24)|(1<<16)|(1<<8)|12),	/* CSI VSYNC pin */ 
												((0<<24)|(1<<16)|(2<<8)|13),	/* CSI HSYNC pin */ 
												((2<<24)|(0<<16)|(4<<8)|0),		/* CSI data 0 pin */ 
												((2<<24)|(0<<16)|(4<<8)|1),		/* CSI data 1 pin */ 
												((2<<24)|(0<<16)|(4<<8)|2),		/* CSI data 2 pin */ 
												((2<<24)|(0<<16)|(4<<8)|3),		/* CSI data 3 pin */ 
												((2<<24)|(0<<16)|(4<<8)|4),		/* CSI data 4 pin */ 
												((2<<24)|(0<<16)|(4<<8)|5),		/* CSI data 5 pin */ 
												((2<<24)|(0<<16)|(4<<8)|6),		/* CSI data 6 pin */  
												((2<<24)|(0<<16)|(4<<8)|7)		/* CSI data 7 pin */ };

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
	#define DEBUG	DIAG_ERROR
#else
	#define DEBUG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_csi_dev_s {
	/*Sub device*/
	int32_t sdcnt;
	int32_t sdidx;
	struct v4l2_subdev *sd[CSI_MAX_SBDEV];
	
	/*buffer control*/
	int32_t bfidx;
	int32_t bfaddr[CSI_MAX_BUF];
	uint8_t in_que[CSI_MAX_QUE];
	uint8_t out_que[CSI_MAX_QUE];
	struct v4l2_buffer bf[CSI_MAX_BUF];
	struct v4l2_requestbuffers rbuf;
	
	int32_t major;
	int32_t csi_feint_flag;
	struct semaphore sem;
	struct cdev c_dev;
	struct class *csi_class;
} gp_csi_dev_t;

DECLARE_WAIT_QUEUE_HEAD(csi_fe_done);

static struct v4l2_capability csi_cap= {
	.driver = "csi driver",
	.card = "CSI",
	.bus_info = "Sensor interface",
	.version = 0,
	.capabilities = V4L2_CAP_VIDEO_CAPTURE
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
gp_csi_dev_t *csi_devices=NULL;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

/**
 * @brief   Gpio irq request function
 * @param   gpio_id [in] gpio channel number
 * @param   name [in] irq handler name
 * @param   irq_handler [in] irq handler function
 * @param   data [in] private data
 * @return  success: callback id(>=0), fail: ERROR_CODE(<0)
 * @see
 */
 
/**
 * @brief   ioctrl to check request buffer
 * @return  success: 0, fail: -EINVAL(<0)
 * @see
 */ 
static int32_t check_rqbuf_type(void)
{
	if(csi_devices->rbuf.count>CSI_MAX_BUF){
		DIAG_ERROR("too many buffers\n");
		return -EINVAL;
	}
	
	if(csi_devices->rbuf.type!=V4L2_BUF_TYPE_VIDEO_CAPTURE){
		DIAG_ERROR("only support video capture mode");
		return -EINVAL;
	}
	
	if(csi_devices->rbuf.memory!=V4L2_MEMORY_USERPTR){
		DIAG_ERROR("only support userptr I/O strea\n");
		return -EINVAL;
	}
	
	return 0;
}

/**
 * @brief   use to register sensor
 * @return  success: 0, fail: -1
 * @see
 */ 
int32_t register_sensor(struct v4l2_subdev *sd)
{
	DEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	
	if( csi_devices==NULL )
	{
		DIAG_ERROR("csi module has not insert yet\n");
		return -1;
	}
	if(csi_devices->sdcnt>=CSI_MAX_SBDEV)
	{
		DIAG_ERROR("register too many sensor module\n");
		return -1;
	}

	csi_devices->sd[csi_devices->sdcnt] = sd;
	csi_devices->sdcnt++;
	return 0;
}
EXPORT_SYMBOL(register_sensor);

/**
 * @brief   use to unregister sensor
 * @return  success: 0, fail: -1
 * @see
 */ 
int32_t unregister_sensor(struct v4l2_subdev *sd)
{
	int32_t i, j;
	
	if( csi_devices==NULL )
	{
		DIAG_ERROR("csi module has not isert yet\n");
		return -1;
	}
	if( csi_devices->sdcnt==0 )
	{
		DIAG_ERROR("No sensor register\n");
		return -1;
	}
	for( i=0; i<csi_devices->sdcnt; i++ )
	{
		if( csi_devices->sd[i]==sd )
		{
			csi_devices->sd[i]=0;
			for( j=i; j<(csi_devices->sdcnt-1); j++ )
				csi_devices->sd[j]=csi_devices->sd[j+1];
			csi_devices->sdcnt--;
			break;
		}
	}
	return 0;
}
EXPORT_SYMBOL(unregister_sensor);

/**
 * @brief   charater device open function
 * @return  success: 0
 * @see
 */ 
static int32_t gp_csi_open(struct inode *inode, struct file *filp)
{
	gp_csi_dev_t *dev = NULL; /* device information */
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

	dev = container_of(inode->i_cdev, gp_csi_dev_t, c_dev);
	filp->private_data = dev;
	
	if( csi_devices->sdcnt==0 )
	{
		DIAG_ERROR("csi open fail: no sensor input\n");
		return -1;
	}

	return 0;
}

/**
 * @brief   charater device release function
 * @return  success: 0
 * @see
 */ 
static int32_t gp_csi_release(struct inode *inode, struct file *filp)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	/* Success */
	
	return 0;
}

/**
 * @brief   charater device ioctl function
 * @return  success: 0
 * @see
 */ 
static long gp_csi_ioctl(struct file *filp, uint32_t cmd, unsigned long arg)
{
	struct v4l2_input *in;
	struct v4l2_queryctrl *qc;
	struct v4l2_fmtdesc *fmtd;
	struct v4l2_format *fmt;
	struct v4l2_buffer *bf;
	struct v4l2_control *ctrl;
	struct v4l2_streamparm *param;
	struct v4l2_subdev *sd;
	struct v4l2_cropcap *cc;
	struct v4l2_crop *crop;
	
	int16_t *addr;
	int32_t idx;
	int32_t ret=0;
	int32_t i;

	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	if( down_interruptible(&csi_devices->sem) ) {
		return -ERESTARTSYS;
	}
		
	/* Success */
	switch(cmd) {
		case VIDIOC_QUERYCAP:
			copy_to_user((struct v4l2_capability*)arg, &csi_cap, sizeof(struct v4l2_capability));
		break;
		
		case VIDIOC_ENUMINPUT:
			in = (struct v4l2_input*)arg;
			if( in->index >= csi_devices->sdcnt)
			{
				DIAG_ERROR("Index Out of bound\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			in->type = V4L2_INPUT_TYPE_CAMERA;
			strcpy( in->name, csi_devices->sd[in->index]->name);
		break;
		
		case VIDIOC_S_INPUT:
			if( (int32_t)arg>=csi_devices->sdcnt )
			{
				DIAG_ERROR("Index Out of bound]n");
				ret = -EINVAL;
				goto ioctlret;
			}
			csi_devices->sdidx = (int32_t)arg;
		break;
		
		case VIDIOC_G_INPUT:
			ret = csi_devices->sdidx;
		break;
		
		case VIDIOC_S_FMT:
			fmt = (struct v4l2_format*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->s_fmt(sd, fmt);
		break;
		
		case VIDIOC_G_FMT:
			fmt = (struct v4l2_format*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->g_fmt(sd, fmt);
		break;
		
		case VIDIOC_TRY_FMT:
			fmt = (struct v4l2_format*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->try_fmt(sd, fmt);
		break;

		case VIDIOC_ENUM_FMT:
			fmtd = (struct v4l2_fmtdesc*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->enum_fmt(sd, fmtd);
		break;

		case VIDIOC_QUERYCTRL:
			qc = (struct v4l2_queryctrl*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->core->queryctrl(sd, qc);
		break;
	
		case VIDIOC_G_CTRL:
			ctrl = (struct v4l2_control*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->core->g_ctrl(sd, ctrl);
		break;
		
		case VIDIOC_S_CTRL:
			ctrl = (struct v4l2_control*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->core->s_ctrl(sd, ctrl);
		break;

		case VIDIOC_CROPCAP:
			cc = (struct v4l2_cropcap*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->cropcap(sd, cc);
		break;
		
		case VIDIOC_G_CROP:
			crop = (struct v4l2_crop*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->g_crop(sd, crop);
		break;
		
		case VIDIOC_S_CROP:
			crop = (struct v4l2_crop*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->s_crop(sd, crop);
		break;
		
		case VIDIOC_G_PARM:
			param = (struct v4l2_streamparm*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->g_parm(sd, param);
		break;
		
		case VIDIOC_S_PARM:
			param = (struct v4l2_streamparm*)arg;
			sd = csi_devices->sd[csi_devices->sdidx];
			ret = sd->ops->video->s_parm(sd, param);
		break;

		case VIDIOC_REQBUFS:
			copy_from_user(&(csi_devices->rbuf),(struct v4l2_requestbuffers*)arg, sizeof(struct v4l2_requestbuffers));
			ret = check_rqbuf_type();
		break;

		case VIDIOC_STREAMON:
			csi_devices->bfidx = 0;
			if(csi_devices->bf[0].flags!=V4L2_BUF_FLAG_QUEUED)
			{
				DIAG_ERROR("Streamon error\n");
				ret=-EINVAL;
				goto ioctlret;
			}
			gpHalCsiStart();
		break;
		
		case VIDIOC_STREAMOFF:
			gpHalCsiStop();
		break;
	
		case VIDIOC_SET_SUSPEND:
		break;
		
		case VIDIOC_SET_RESUME:
		break;
		
		case VIDIOC_QBUF:
			bf = (struct v4l2_buffer*)arg;
			if( bf->type != csi_devices->rbuf.type )
			{
				DIAG_ERROR("QBuf Type error\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			if( bf->index>=csi_devices->rbuf.count )
			{
				DIAG_ERROR("QBuf index out of bound\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			idx = bf->index;
			copy_from_user(&(csi_devices->bf[idx]), (struct v4l2_buffer*)arg, sizeof(struct v4l2_buffer));
			csi_devices->bf[idx].flags = V4L2_BUF_FLAG_QUEUED;
			addr = (int16_t *)gp_user_va_to_pa((int16_t *)bf->m.userptr);
			csi_devices->bfaddr[idx] = (int)addr;
			gpHalCsiSetBuf(idx, addr);
//			for( i=0; i<CSI_MAX_QUE; i++ )
//			{
	//			if()
//			}
		break;
		
		case VIDIOC_DQBUF:
			bf = (struct v4l2_buffer*)arg;
			if( bf->type != csi_devices->rbuf.type )
			{
				ret = -EINVAL;
				goto ioctlret;
			}
			for( i=0; i<csi_devices->rbuf.count; i++ ){
				if( csi_devices->bf[i].flags==V4L2_BUF_FLAG_DONE ){
					copy_to_user((struct v4l2_buffer*)arg, &(csi_devices->bf[i]), sizeof(struct v4l2_buffer));
					csi_devices->bf[i].flags=V4L2_BUF_FLAG_QUEUED;
					ret = 0;
					goto ioctlret;
				}
			}
		break;
	}
	
ioctlret:
	up(&csi_devices->sem);
	
	return 0;
}

/**
 * @brief   sensor interrupt handler
 * @return  success: IRQ_HANDLED
 * @see
 */ 
static irqreturn_t gp_csi_irq_handler(int32_t irq, void *dev_id)
{
	int32_t ret;
	
	csi_devices->bf[csi_devices->bfidx].flags = V4L2_BUF_FLAG_DONE;

	csi_devices->bfidx++;
	if(csi_devices->bfidx>=csi_devices->rbuf.count)
		csi_devices->bfidx=0;
	
	ret = gpHalCsiClearisr(csi_devices->bfidx, csi_devices->bf[csi_devices->bfidx].m.userptr);
	
	if( ret==3 ){
		if(csi_devices->csi_feint_flag==0)
			csi_devices->csi_feint_flag=1;
		wake_up_interruptible(&csi_fe_done);
	}

	return IRQ_HANDLED;
}

/**
 * @brief   character device poll function
 * @return  success: POLLIN
 * @see
 */ 
static uint32_t gp_csi_poll(struct file *filp,	poll_table *poll)
{
	uint32_t mask=0;

	poll_wait(filp, &csi_fe_done, poll);
	if(csi_devices->csi_feint_flag==1){
		csi_devices->csi_feint_flag=0;
		mask = POLLIN | POLLRDNORM;
	}
	return mask;
}

struct file_operations csi_fops = {
	.owner = THIS_MODULE,
	.open = gp_csi_open,
	.unlocked_ioctl = gp_csi_ioctl,
	.release = gp_csi_release,
	.poll = gp_csi_poll,
};

/**
 * @brief   character device module exit function
 * @see
 */ 
static void __exit gp_csi_module_exit(void)
{
	dev_t devno = MKDEV(csi_devices->major, CSI_MINOR);
	
	device_destroy(csi_devices->csi_class, devno);
	cdev_del(&csi_devices->c_dev);	
	free_irq(IRQ_SENSOR, csi_devices);
	class_destroy(csi_devices->csi_class);
	kfree(csi_devices);
	csi_devices = NULL;
	unregister_chrdev_region(devno, CSI_NR_DEVS);
}

/**
 * @brief   character device module init function
 * @return  success: 0
 * @see
 */ 
static int32_t __init gp_csi_module_init(void)
{
	int32_t ret;
	int32_t devno;
	int32_t i;
	dev_t dev;
	struct device *device;
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	
	/* allocate a major number to csi module */
	ret = alloc_chrdev_region( &dev, CSI_MINOR, 1, "csi0" );
	if( ret<0 )	{
		DIAG_ERROR("CSI: can't get major\n");
		goto fail_init;
	}
	
	/* allocate a structure for csi device */
	csi_devices = kmalloc(sizeof(gp_csi_dev_t), GFP_KERNEL);
	if(!csi_devices) {
		DIAG_ERROR("CSI: can't kmalloc\n");
		ret = -ENOMEM;
		goto fail_kmalloc;
	}
	memset(csi_devices, 0, sizeof(gp_csi_dev_t));
	csi_devices->major = MAJOR(dev);
	
	/* create class for csi character device */
	csi_devices->csi_class = class_create(THIS_MODULE, "csi");
	if (IS_ERR(csi_devices->csi_class))
	{
		DIAG_ERROR("CSI: can't create class\n");
		ret = -EFAULT;
		goto fail_create_class;
	}
	
	/* request a irq for csi interrupt service */
	ret = request_irq(IRQ_SENSOR, gp_csi_irq_handler, 0, "CSI_IRQ", csi_devices);
	if (ret < 0) {
		DIAG_ERROR("CSI: request csi irq fail\n");
		goto fail_request_irq;
	}
	
	/* create character device node */
	devno = MKDEV(csi_devices->major, CSI_MINOR);
	cdev_init(&(csi_devices->c_dev), &csi_fops);
	csi_devices->c_dev.owner = THIS_MODULE;
	csi_devices->c_dev.ops = &csi_fops;
	ret = cdev_add(&(csi_devices->c_dev), devno, 1);	
	if(ret < 0){
		DIAG_ERROR("CSI: cdev_add error\n");
		goto fail_device_register;
	}
	device = device_create( csi_devices->csi_class, NULL, devno, NULL, "csi%d", 0);
	if(!device){
		DIAG_ERROR("CSI: device_create error\n");
		goto fail_device_create;
	}
	
	/* initial the semaphore */
	init_MUTEX(&(csi_devices->sem));

	/* ----- Set pin IO ----- */
	for( i=0; i<CSI_PINNUM; i++ ) {
		ret = gp_gpio_request(CSI_PINIO[i],"csi_pin");
		gp_gpio_set_direction(ret,GPIO_DIR_INPUT);
		gp_gpio_set_driving_current(ret, 1);
	}	
	gpHalCsiInit();
	
	return 0;

fail_device_create:	
	cdev_del(&csi_devices->c_dev);
fail_device_register:
	free_irq(IRQ_SENSOR, csi_devices);
fail_request_irq:
	class_destroy(csi_devices->csi_class);
fail_create_class:
	kfree(csi_devices);
fail_kmalloc:
	unregister_chrdev_region(dev, CSI_NR_DEVS);
fail_init:
	return ret;
}

module_init(gp_csi_module_init);
module_exit(gp_csi_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP CSI driver");
MODULE_LICENSE_GP;
