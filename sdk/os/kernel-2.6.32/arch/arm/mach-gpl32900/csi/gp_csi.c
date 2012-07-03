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

#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_csi.h>

#include <mach/gp_chunkmem.h>
#include <mach/gp_cache.h>
#include <mach/gp_csi.h>
#include <mach/gp_board.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/sensor_mgr.h>

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	CSI_MINOR		0
#define CSI_NR_DEVS		1
#define CSI_MAX_BUF		3
#define CSI_MAX_QUE		5
#define	NO_INPUT		0xFFFFFFFF
#define USBPHY_CLK		96000000

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DEBUG	DIAG_ERROR
#else
	#define DEBUG(...)
#endif

#define CLEAR(x, y) memset (&(x), y, sizeof (x))

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_csi_dev_s {
	/*Sub device*/
	int32_t sdidx;
	struct v4l2_subdev *sd;
	callbackfunc_t *cb;
	
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
sensor_config_t *sensor=NULL;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/ 
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
 * @brief   charater device open function
 * @return  success: 0
 * @see
 */ 
static int32_t gp_csi_open(struct inode *inode, struct file *filp)
{
	gp_csi_dev_t *dev = NULL; /* device information */
	
	dev = container_of(inode->i_cdev, gp_csi_dev_t, c_dev);
	filp->private_data = dev;
	gpHalCsiInit();
	DEBUG(KERN_WARNING "CsiOpen.\n");
	return 0;
}

/**
 * @brief   charater device release function
 * @return  success: 0
 * @see
 */ 
static int32_t gp_csi_release(struct inode *inode, struct file *filp)
{
	gpHalCsiSetMclk(0, 0, 0, 0);
	gpHalCsiClose();

	csi_devices->sdidx = NO_INPUT;
	CLEAR(csi_devices->bfaddr, 0);
	CLEAR(csi_devices->bf, 0);
	CLEAR(csi_devices->in_que, 0xFF);
	CLEAR(csi_devices->out_que, 0xFF);
	csi_devices->csi_feint_flag = 0;
	DEBUG(KERN_WARNING "Csiclose.\n");
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
	struct v4l2_interface *interface;
	struct clk *clock;
	callbackfunc_t *cb;
	gpCsiMclk_t mclk;
	char *port;
	uint8_t div; 
	int16_t *addr;
	int32_t idx;
	int32_t ret=0;
	int32_t i;
	int32_t setctrl;
	
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
			ret = gp_get_sensorinfo( in->index, (int*)&sd, (int*)&cb, (int*)&port, (int*)&sensor );
			if( ret==-1 )
			{
				DIAG_ERROR("Index Out of bound or unregister\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			in->type = V4L2_INPUT_TYPE_CAMERA;
			strcpy( in->name, sd->name );
		break;
		
		case VIDIOC_S_INPUT:
			if((int32_t)arg==csi_devices->sdidx) {
				DIAG_ERROR("The Same input\n");
				goto ioctlret;
			}
			if( csi_devices->sdidx!=NO_INPUT )	{
				ret = gp_get_sensorinfo( (int32_t)arg, (int*)&sd, (int*)&cb, (int*)&port, (int*)&sensor );
				if(ret==-1)	{
					DIAG_ERROR("Set input fail\n");
					ret = -EINVAL;
					goto ioctlret;
				}
				ret = gp_get_sensorinfo( csi_devices->sdidx, (int*)&sd, (int*)&cb, (int*)&port, (int*)&sensor );
				if(ret==-1)	{
						DIAG_ERROR("suspend fail\n");
						ret = -EINVAL;
						goto ioctlret;
				}
				csi_devices->sd->ops->ext->suspend(csi_devices->sd);
				if( csi_devices->cb->standby != NULL )
					csi_devices->cb->standby(1); 
				if( csi_devices->cb->powerctl!=NULL )
					csi_devices->cb->powerctl(0);
			}
			ret = gp_get_sensorinfo( (int32_t)arg, (int*)&csi_devices->sd, (int*)&(csi_devices->cb), (int*)&port, (int*)&sensor );
			if(ret==-1)
			{
				DIAG_ERROR("Set input fail\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			csi_devices->sdidx = (int32_t)arg;

			if( csi_devices->cb->powerctl!=NULL )
				csi_devices->cb->powerctl(1);
			if( csi_devices->cb->standby != NULL )
				csi_devices->cb->standby(0); 
			if( csi_devices->cb->set_port!=NULL )
				csi_devices->cb->set_port(port);

			/* open mclk, some sensor need mclk befor init */
	/*		if(sensor->fmt[0].mclk_src == CSI_CLK_SPLL) {	
				clock = clk_get(NULL, "clk_ref_ceva");
				ret = clk_get_rate(clock); 
			}else{
				ret = USBPHY_CLK;
			}
			div = ret/sensor->fmt[0].mclk;
			if((ret % sensor->fmt[0].mclk) == 0) div--;
			DEBUG("mclk = %d\n", ret/(div + 1));
			gpHalCsiSetMclk(sensor->fmt[0].mclk_src, div, 0, 0);*/

			sd = csi_devices->sd;
			ret = sd->ops->core->reset(sd, 0);
			ret = sd->ops->core->init(sd, 0);
			if(ret < 0)
			{
				DIAG_ERROR("sensor init fail\n");
				ret=-EINVAL;
				goto ioctlret;
			}
		break;
		
		case VIDIOC_G_INPUT:
			ret = csi_devices->sdidx;
		break;
		
		case VIDIOC_S_FMT:
			printk("gp_csi \n");
			fmt = (struct v4l2_format*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			
			if(fmt->fmt.pix.pixelformat==V4L2_PIX_FMT_YUYV)
				gpHalCsiSetFormat( CSI_YUVIN | CSI_INSEQ_YUYV );
			else if(fmt->fmt.pix.pixelformat==V4L2_PIX_FMT_UYVY)
				gpHalCsiSetFormat( CSI_YUVIN | CSI_INSEQ_UYVY );
			else if(fmt->fmt.pix.pixelformat==V4L2_PIX_FMT_RGB565)
				gpHalCsiSetFormat( CSI_RGBIN );
			else if(fmt->fmt.pix.pixelformat==V4L2_PIX_FMT_RGB555X)
				gpHalCsiSetFormat( CSI_RGBIN | CSI_RGB1555 );
			else
			{
				ret = -EINVAL;
				goto ioctlret;
			}			

			for(i=0; i<sensor->sensor_fmt_num; i++) {
				if( (fmt->fmt.pix.width == sensor->fmt[i].hpixel) && (fmt->fmt.pix.height == sensor->fmt[i].vline) ) {
					sd = csi_devices->sd;
					ret = sd->ops->video->s_fmt(sd, fmt);
					if(ret >= 0) {
						gpHalCsiSetResolution( sensor->fmt[i].hpixel-1, sensor->fmt[i].vline-1, sensor->fmt[i].hpixel*2,
										sensor->fmt[i].hoffset, sensor->fmt[i].voffset);

					/*	if(sensor->fmt[0].mclk_src == CSI_CLK_SPLL) {	
							clock = clk_get(NULL, "clk_ref_ceva");
							ret = clk_get_rate(clock); 
						}else{
							ret = USBPHY_CLK;
						}
						div = ret/sensor->fmt[0].mclk;
						if((ret % sensor->fmt[0].mclk) == 0) div--;
					//	DEBUG("mclk = %d\n", ret/(div + 1));
					//	gpHalCsiSetMclk(sensor->fmt[0].mclk_src, div, 0, 0);*/
					}
					break;
				}
			}

			if(sensor->sensor_fmt_num == i) {
				ret = -EINVAL;
				goto ioctlret;
			}
		break;
		
		case VIDIOC_G_FMT:
			fmt = (struct v4l2_format*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->g_fmt(sd, fmt);
		break;
		
		case VIDIOC_TRY_FMT:
			fmt = (struct v4l2_format*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->try_fmt(sd, fmt);
		break;

		case VIDIOC_ENUM_FMT:
			fmtd = (struct v4l2_fmtdesc*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->enum_fmt(sd, fmtd);
		break;

		case VIDIOC_QUERYCTRL:
			qc = (struct v4l2_queryctrl*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->core->queryctrl(sd, qc);
		break;
	
		case VIDIOC_G_CTRL:
			ctrl = (struct v4l2_control*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->core->g_ctrl(sd, ctrl);
		break;
		
		case VIDIOC_S_CTRL:
			ctrl = (struct v4l2_control*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->core->s_ctrl(sd, ctrl);
		break;
		
		case VIDIOC_S_INTERFACE:
			interface = (struct v4l2_interface*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			setctrl = 0;		
			
			if( interface->Interface==1 )
				setctrl |= CSI_CCIR656;
			if( interface->HsyncAct==1 )
				setctrl |= CSI_HSYNC_HACT;
			if( interface->VsyncAct==1 )
				setctrl |= CSI_VSYNC_HACT;
			if( interface->Field==1 )
				setctrl |= CSI_FIELD_ODDH;
			if( interface->Interlace==1 )
				setctrl |= CSI_INTERLACE;
			if( interface->FrameEndMode==1 )
				setctrl |= CSI_FMMODE_ODDFIELD;
			else if( interface->FrameEndMode==2 )
				setctrl |= CSI_FMMODE_EVENFIELD;
			if( interface->SampleEdge==1 )
				setctrl |= CSI_SAMPLE_NEG;
			if( interface->FmtOut==1 )
				setctrl |= CSI_YUVOUT;
				
		//	setctrl |= CSI_EN | CSI_UPDATESET;
			
			gpHalCsiSetCtrl(setctrl);
			ret = sd->ops->ext->s_interface(sd, interface);
		break;
		
		case VIDIOC_G_INTERFACE:
			interface = (struct v4l2_interface*)arg;
			gpHalCsiGetCtrl(&setctrl);
			
			if(setctrl&CSI_CCIR656)
				interface->Interface=1;
			if(setctrl&CSI_HSYNC_HACT)
				interface->HsyncAct=1;
			if(setctrl&CSI_VSYNC_HACT)
				interface->VsyncAct=1;
			if(setctrl&CSI_FIELD_ODDH)
				interface->Field=1;
			if(setctrl&CSI_INTERLACE)
				interface->Interlace=1;
			if(setctrl&CSI_FMMODE_ODDFIELD)
				interface->FrameEndMode=1;
			else if(setctrl&CSI_FMMODE_EVENFIELD)
				interface->FrameEndMode=2;
			if(setctrl&CSI_SAMPLE_NEG)
				interface->SampleEdge=1;
			if(setctrl&CSI_YUVOUT)
				interface->FmtOut=1;			
		break;

		case VIDIOC_S_MCLK:
	/*		ret = copy_from_user((void*)&mclk, (void __user*)arg, sizeof(mclk));
			if(ret < 0)
			{
				DIAG_ERROR("mclk set error\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			
			if(mclk.mclk_out == 0)
			{
				mclk.mclk_sel = div = 0;
				mclk.pclk_dly = 0;
				mclk.pclk_revb = 0;
				DEBUG("mclk = 0\n");
			}
			else
			{
				if(mclk.mclk_sel == CSI_CLK_SPLL)
				{
					clock = clk_get(NULL, "clk_ref_ceva"); 
					ret = clk_get_rate(clock); 
				}
				else
				{
					ret = USBPHY_CLK;
				}
				div = ret/mclk.mclk_out;
				if((ret % mclk.mclk_out) == 0) div--;
				DEBUG("mclk = %d\n", ret/(div + 1));
			}
			gpHalCsiSetMclk(mclk.mclk_sel, div, mclk.pclk_dly, mclk.pclk_revb);*/
		break;
		
		case VIDIOC_G_MCLK:
			gpHalCsiGetMclk(&mclk.mclk_sel, &div, &mclk.pclk_dly, &mclk.pclk_revb);
			if(mclk.mclk_sel == CSI_CLK_SPLL)
			{
				clock = clk_get(NULL, "clk_ref_ceva"); 
				ret = clk_get_rate(clock);
			}
			else
			{
				ret = USBPHY_CLK;
			}
			
			mclk.mclk_out = ret/(div + 1);
			ret = copy_to_user((void __user*)arg, (void*)&mclk, sizeof(mclk));
			if(ret < 0)
			{
				DIAG_ERROR("mclk get error\n");
				ret = -EINVAL;
				goto ioctlret;
			}	
		break;
		
		case VIDIOC_CROPCAP:
			cc = (struct v4l2_cropcap*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->cropcap(sd, cc);
		break;
		
		case VIDIOC_G_CROP:
			crop = (struct v4l2_crop*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->g_crop(sd, crop);
		break;
		
		case VIDIOC_S_CROP:
			crop = (struct v4l2_crop*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->s_crop(sd, crop);
		break;
		
		case VIDIOC_G_PARM:
			param = (struct v4l2_streamparm*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->g_parm(sd, param);
		break;
		
		case VIDIOC_S_PARM:
			param = (struct v4l2_streamparm*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			ret = sd->ops->video->s_parm(sd, param);
		break;

		case VIDIOC_REQBUFS:
			copy_from_user(&(csi_devices->rbuf),(struct v4l2_requestbuffers*)arg, sizeof(struct v4l2_requestbuffers));
			ret = check_rqbuf_type();
		break;

		case VIDIOC_STREAMON:
			//sd = csi_devices->sd;
			//ret = sd->ops->core->init(sd, 0);
			if(ret < 0)
			{
				DIAG_ERROR("sensor init fail\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			
			if(arg == (unsigned long)NULL) 
			{
				if(csi_devices->in_que[0] == 0xFF)
				{
					DIAG_ERROR("No buffer in Que\n");
					ret = -EINVAL;
					goto ioctlret;
				}
				gpHalCsiSetBuf(csi_devices->bfaddr[csi_devices->in_que[0]]);
				gpHalCsiStart();
			}
			else
			{
				DIAG_ERROR("csi start fail\n");
				ret=-EINVAL;
			}		
		break;
		
		case VIDIOC_STREAMOFF:
			gpHalCsiSetMclk(0, 0, 0, 0);
			gpHalCsiStop();
			CLEAR(csi_devices->bfaddr, 0);
			CLEAR(csi_devices->bf, 0);
			CLEAR(csi_devices->in_que, 0xFF);
			CLEAR(csi_devices->out_que, 0xFF);
			csi_devices->csi_feint_flag = 0;
		break;
		
		case VIDIOC_QBUF:
			bf = (struct v4l2_buffer*)arg;
			if( bf->type != csi_devices->rbuf.type ) {
				DIAG_ERROR("QBuf Type error\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			if( bf->index>=csi_devices->rbuf.count ) {
				DIAG_ERROR("QBuf index out of bound\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			for( i=0; i<CSI_MAX_QUE; i++ ) {
				if(csi_devices->in_que[i]==0xFF)	{
					csi_devices->in_que[i]=bf->index;
					break;
				}
				if(i==(CSI_MAX_QUE-1)) {
					DIAG_ERROR("Que buffer is full\n");
					ret = -EINVAL;
					goto ioctlret;
				}
			}
			idx = bf->index;
			copy_from_user(&(csi_devices->bf[idx]), (struct v4l2_buffer*)arg, sizeof(struct v4l2_buffer));
			csi_devices->bf[idx].flags = V4L2_BUF_FLAG_QUEUED;
			addr = (int16_t *)gp_user_va_to_pa((int16_t *)bf->m.userptr);
			csi_devices->bfaddr[idx] = (int)addr;
		break;
		
		case VIDIOC_DQBUF:
			bf = (struct v4l2_buffer*)arg;
			if( bf->type != csi_devices->rbuf.type )
			{
				ret = -EINVAL;
				goto ioctlret;
			}
			if( csi_devices->out_que[0]==0xFF ) 
			{
				DIAG_ERROR("no buffer ready\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			copy_to_user((struct v4l2_buffer*)arg, &(csi_devices->bf[csi_devices->out_que[0]]), sizeof(struct v4l2_buffer));
			// shift the out_queue buffer
            for(i=0; i<(CSI_MAX_QUE-1); i++)
				csi_devices->out_que[i] = csi_devices->out_que[i+1];

			csi_devices->out_que[CSI_MAX_QUE-1]=0xFF;
		break;
		
		case VIDIOC_QUERYSTD:
		{
			v4l2_std_id *std = (v4l2_std_id*)arg;
			if(csi_devices->sdidx==NO_INPUT)
			{
				DIAG_ERROR("please set input first\n");
				ret = -EINVAL;
				goto ioctlret;
			}
			sd = csi_devices->sd;
			if ( sd->ops->video->querystd )
				ret = sd->ops->video->querystd(sd, std);
			else
				ret = ENOIOCTLCMD;
			break;
		}
	}
	
ioctlret:
	up(&csi_devices->sem);
		
	return ret;
}

/**
 * @brief   sensor interrupt handler
 * @return  success: IRQ_HANDLED
 * @see
 */ 
static irqreturn_t gp_csi_irq_handler(int32_t irq, void *dev_id)
{
	int32_t ret, i;

	ret = gpHalCsiClearisr();
	if(ret != 0x3)
		return IRQ_NONE;

	//find empty frame
	csi_devices->bfidx = csi_devices->in_que[1];
	if(csi_devices->bfidx == 0xFF)
		return IRQ_HANDLED; 

	//get/set empty buffer to h/w
	gpHalCsiSetBuf(csi_devices->bfaddr[csi_devices->bfidx]);

	//get/set ready buffer
	csi_devices->bfidx = csi_devices->in_que[0];
	csi_devices->bf[csi_devices->bfidx].flags = V4L2_BUF_FLAG_DONE;
	
	//shift the in_que buffer
	for(i=0; i<(CSI_MAX_QUE-1); i++)
		csi_devices->in_que[i] = csi_devices->in_que[i+1];
	
	csi_devices->in_que[CSI_MAX_QUE-1]=0xFF;

	//push the ready index into out_que buffer
	for(i=0; i<CSI_MAX_QUE; i++) 
	{
		if(csi_devices->out_que[i] == 0xFF) 
		{
			csi_devices->out_que[i]=csi_devices->bfidx;
			break;
		}
	}
	
	if(i == CSI_MAX_QUE)
		return IRQ_HANDLED;
	
	if(csi_devices->csi_feint_flag == 0)
	{
		csi_devices->csi_feint_flag = 1;
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
	if(csi_devices->csi_feint_flag == 1)
	{
		csi_devices->csi_feint_flag = 0;
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
 * @brief display device release                                              
 */                                                                         
static void
csi_device_release(
	struct device *dev
)
{
	printk("remove csi device ok\n");
}

static struct platform_device csi_device = {
	.name	= "gp-csi",
	.id		= 0,
	.dev	= {
		.release = csi_device_release,
	},
};

#ifdef CONFIG_PM
static int
csi_suspend(
	struct platform_device *pdev,
	pm_message_t state
)
{
//	struct v4l2_subdev *sd;
	
//	printk("csi_suspend\n");
	
//	sd = csi_devices->sd[csi_devices->sdidx];
//	sd->ops->ext->suspend(sd);
	
	gpHalCsiSuspend();

	return 0;
}

static int
csi_resume(
	struct platform_device *pdev
)
{
//	struct v4l2_subdev *sd;
//	printk("csi_resume\n");
	
//	sd = csi_devices->sd[csi_devices->sdidx];
//	sd->ops->ext->resume(sd);
	
	gpHalCsiResume();
	
	return 0;
}
#else
#define csi_suspend NULL
#define	csi_resume NULL
#endif


/**                                                                         
 * @brief audio driver define                                               
 */                                                                         
static struct platform_driver csi_driver = {
	.suspend = csi_suspend,
	.resume = csi_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "gp-csi"
	}
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
	platform_device_unregister(&csi_device);
	platform_driver_unregister(&csi_driver);
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
	dev_t dev;
	struct device *device;

	DEBUG(KERN_WARNING "ModuleInit: csi \n");
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
	csi_devices->sdidx = NO_INPUT;
	CLEAR(csi_devices->in_que, 0xFF);
	CLEAR(csi_devices->out_que, 0xFF);
	/* initial the semaphore */
	init_MUTEX(&(csi_devices->sem));

	platform_device_register(&csi_device);
	return platform_driver_register(&csi_driver);	

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
