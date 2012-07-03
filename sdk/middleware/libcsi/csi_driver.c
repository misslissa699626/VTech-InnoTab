#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <mach/gp_csi.h>
#include <mach/gp_csi1.h>
#include <mach/gp_cdsp.h>
#include <mach/gp_mipi.h>

#include <csi_driver.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/ 
#define BUFFER_NO			4

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define RETURN(x)\
{\
	nRet = x;\
	goto __exit;\
}\

#if 1
#define DEBUG	printf
#else
#define DEBUG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct CsiBuf_s {
	unsigned short width;
	unsigned short height;
	unsigned long addr;
}CsiBuf_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static int CsiHandle = -1;
static int MipiHandle = -1;
static unsigned int SensorFmt = V4L2_PIX_FMT_YVYU;
static unsigned int MipiFmt = MIPI_YUV422;
static char SensorName[64] = "sensor_6AA_mipi";
static CsiBuf_t CsiBuf[BUFFER_NO];

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
int 
CsiDrv_SetDevice(
	char *DeviceName,
	unsigned int inFormat
)
{
	strcpy(SensorName, DeviceName);
	SensorFmt = inFormat;
	switch(inFormat)
	{
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		MipiFmt = MIPI_YUV422;
		break;
		
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
		MipiFmt = MIPI_RAW8;
		break;
		
	default:
		return -1;
	}

	return 0;
}

int 
CsiDrv_init(
	CsiConfig* config
)
{
	int nRet;
	int i, index;
	unsigned int reg;
	struct v4l2_capability cap;
	struct v4l2_input input;
	struct v4l2_format fmt;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	gpMipiCCIR601_t mipi_ccir601;

	/* init varaible */
	for(i=0; i<BUFFER_NO; i++) {
		CsiBuf[i].width = 0;
		CsiBuf[i].height = 0;
		CsiBuf[i].addr = 0;
	}

	/* mipi device */
	MipiHandle = open("/dev/mipi", O_RDWR);
	if(MipiHandle < 0) {
		DEBUG("MipiOpenFail!!!\n");
		RETURN(ERR_OPEN_FAIL);
	}

	/* set mipi size */
	memset((void *)&mipi_ccir601, 0, sizeof(mipi_ccir601));
	mipi_ccir601.data_type = MipiFmt;
	mipi_ccir601.data_type_to_cdsp = DISABLE;
	mipi_ccir601.h_size = config->mImageWidth;
	mipi_ccir601.v_size = config->mImageHeight;
	mipi_ccir601.h_back_porch = 0;
	mipi_ccir601.h_front_porch = 4;
	mipi_ccir601.blanking_line_en = ENABLE;
	if (ioctl(MipiHandle, MIPI_IOCTL_S_CCIR601, &mipi_ccir601) < 0) {
		DEBUG("MIPI_IOCTL_S_CCIR601 fail!\n");
		RETURN(ERR_FMT_FAIL);
	}
	
	/* set mipi start */
	if (ioctl(MipiHandle, MIPI_IOCTL_S_START, SensorName) < 0) {
		DEBUG("MIPI_IOCTL_S_START fail!\n");
		RETURN(ERR_START_FAIL);
	} 
	
	/* csi device */
	CsiHandle = open("/dev/csi1", O_RDWR);
	if(CsiHandle < 0) {
		DEBUG("OpenCsiFail!!!\n");
		RETURN(ERR_OPEN_FAIL);
	}

	/* capacity */
	if(ioctl(CsiHandle, VIDIOC_QUERYCAP, &cap) < 0) {
		DEBUG("VIDIOC_QUERYCAP fail\n");
		RETURN(ERR_DEVICE_FAIL);
	}
	
	if(cap.capabilities != V4L2_CAP_VIDEO_CAPTURE) {
		DEBUG("capabilities fail = 0x%x\n", cap.capabilities);
		RETURN(ERR_DEVICE_FAIL);
	}

	/* Find sensor driver */
	for(i=0; i<4; i++) { 
		memset((void *)&input, 0x0, sizeof(input));
		input.index = i;
		if(ioctl(CsiHandle, VIDIOC_ENUMINPUT, &input) < 0) {
			DEBUG("VIDIOC_ENUMINPUT fail\n");
			RETURN(ERR_FMT_FAIL);
		}

		DEBUG("Device[%d]=%s\n", i, input.name);
		if(strcmp(input.name, SensorName) == 0) {
			index = i;
			break;
		}
	}

	if(i == 4) {
		DEBUG("NotFindSensorDevice!\n");
		RETURN(ERR_DEVICE_FAIL);
	}

	/* set input source */
	if(ioctl(CsiHandle, VIDIOC_S_INPUT, index) < 0) {
		DEBUG("VIDIOC_S_INPUT fail!\n");
		RETURN(ERR_DEVICE_FAIL);
	}

	/* set format and size */
	memset((void *)&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = config->mImageWidth;
	fmt.fmt.pix.height = config->mImageHeight;
	fmt.fmt.pix.pixelformat = SensorFmt;
	fmt.fmt.pix.priv = 0xFF; 
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if(ioctl(CsiHandle, VIDIOC_S_FMT, &fmt) < 0) {
		DEBUG("VIDIOC_S_FMT fail!\n");
		RETURN(ERR_FMT_FAIL);
	}

	/* set frame buffer number */
	index = 0;
	reg = config->mFlag;
	reg &= CSI_IMAGE_BUFFER_0 | CSI_IMAGE_BUFFER_1 | CSI_IMAGE_BUFFER_2 | CSI_IMAGE_BUFFER_3;
	reg >>= 2;
	for(i=0; i<BUFFER_NO; i++) {
		if(reg >> i) {
			index++;
		}
	}

	memset((void *)&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = index;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if(ioctl(CsiHandle, VIDIOC_REQBUFS, &req) < 0) {
		DEBUG("VIDIOC_REQBUFS Fail!\n");
		RETURN(ERR_BUFFER_FAIL);
	}
	
	/* set frame buffer address */
	memset((void *)&buf, 0, sizeof(buf));	
	for(i=0; i<index; i++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.userptr = (unsigned long)config->mImageBuffer[i];
		buf.length = config->mImageWidth*config->mImageHeight*2;
		if(ioctl(CsiHandle, VIDIOC_QBUF, &buf) < 0) {
			DEBUG("VIDIOC_QBUF(0) Fail!\n");
			RETURN(ERR_BUFFER_FAIL);
		}

		DEBUG("QBuf[%d] = 0x%x\n", buf.index, buf.m.userptr);
		CsiBuf[i].width = config->mImageWidth;
		CsiBuf[i].height = config->mImageHeight;
		CsiBuf[i].addr = (unsigned long)config->mImageBuffer[i];
	}
	
	/* set start */
	if(ioctl(CsiHandle, VIDIOC_STREAMON, 0) < 0) {
		DEBUG("VIDIOC_STREAMON Fail!\n");
		RETURN(ERR_START_FAIL);
	}

__exit:
	if(nRet < 0) {
		CsiDrv_cleanup();
	}
	return nRet;
}

void 
CsiDrv_cleanup(
	void
)
{
	if(CsiHandle >= 0) {
		if(ioctl(CsiHandle, VIDIOC_STREAMOFF, 0) < 0) {
			DEBUG("VIDIOC_STREAMOFF Fail\n");
		}
		close(CsiHandle);
		CsiHandle = -1;
	}
 
	if(MipiHandle >= 0) {
		close(MipiHandle);
		MipiHandle = -1;
	} 
}

int 
CsiDrv_setParameters(
	CsiConfig* config
)
{
	int nRet = 0;
	
	if(config->mFlag & CSI_CUBIC_DISABLE){
 		struct v4l2_control ctrl;
		gpCsi1Cubic_t Cubic;

		ctrl.id = MSG_CSI1_CUBIC;
		ctrl.value = &Cubic;
		Cubic.CubicEn = 0;
		Cubic.CubicMode = CUBIC_64X64;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	} else if(config->mFlag & CSI_CUBIC_32X32){
 		struct v4l2_control ctrl;
		gpCsi1Cubic_t Cubic;

		if((CsiBuf[0].width %32) || (CsiBuf[0].height %32)) {
			DEBUG("W/H must be 32*N\n");
			RETURN(ERR_CTRL_FAIL);
		}

		ctrl.id = MSG_CSI1_CUBIC;
		ctrl.value = &Cubic;
		Cubic.CubicEn = 1;
		Cubic.CubicMode = CUBIC_32X32;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	} else if(config->mFlag & CSI_CUBIC_64X64){
 		struct v4l2_control ctrl;
		gpCsi1Cubic_t Cubic;

		if((CsiBuf[0].width % 64) || (CsiBuf[0].height % 64)) {
			DEBUG("W/H must be 32*N\n");
			RETURN(ERR_CTRL_FAIL);
		}

		ctrl.id = MSG_CSI1_CUBIC;
		ctrl.value = &Cubic;
		Cubic.CubicEn = 1;
		Cubic.CubicMode = CUBIC_64X64;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	}

	if(config->mFlag & CSI_NIGHT_MODE_EN){
 		struct v4l2_control ctrl;
		struct v4l2_control sensor_ctrl;

		ctrl.id = MSG_CSI1_SENSOR;
		ctrl.value = &sensor_ctrl;
		sensor_ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
		sensor_ctrl.value = 1;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	} else if(config->mFlag & CSI_NIGHT_MODE_DIS){
 		struct v4l2_control ctrl;
		struct v4l2_control sensor_ctrl;

		ctrl.id = MSG_CSI1_SENSOR;
		ctrl.value = &sensor_ctrl;
		sensor_ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
		sensor_ctrl.value = 0;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	}

	if(config->mFlag & CSI_POWER_LINE_DIS){
 		struct v4l2_control ctrl;
		struct v4l2_control sensor_ctrl;

		ctrl.id = MSG_CSI1_SENSOR;
		ctrl.value = &sensor_ctrl;
		sensor_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
		sensor_ctrl.value = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	} else if(config->mFlag & CSI_POWER_LINE_50HZ){
 		struct v4l2_control ctrl;
		struct v4l2_control sensor_ctrl;

		ctrl.id = MSG_CSI1_SENSOR;
		ctrl.value = &sensor_ctrl;
		sensor_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
		sensor_ctrl.value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	} else if(config->mFlag & CSI_POWER_LINE_60HZ){
 		struct v4l2_control ctrl;
		struct v4l2_control sensor_ctrl;

		ctrl.id = MSG_CSI1_SENSOR;
		ctrl.value = &sensor_ctrl;
		sensor_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
		sensor_ctrl.value = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
		if(ioctl(CsiHandle, VIDIOC_S_CTRL, &ctrl) < 0) {
			DEBUG("VIDIOC_S_CTRL fail!\n");
			RETURN(ERR_CTRL_FAIL);
		}
	}

__exit:
	return nRet;
}

int 
CsiDrv_getParameters(
	CsiConfig* config
)
{
	int nRet = 0;

	if(config->mFlag & (CSI_IMAGE_WIDTH | CSI_IMAGE_HEIGHT)){
		struct v4l2_format fmt;

		memset((void *)&fmt, 0, sizeof(fmt));
		if(ioctl(CsiHandle, VIDIOC_G_FMT, &fmt) < 0) {
			DEBUG("VIDIOC_G_FMT fail!\n");
			RETURN(ERR_FMT_FAIL);
		}
 		config->mImageWidth = fmt.fmt.pix.width;
		config->mImageHeight = fmt.fmt.pix.height;
	} 
	
	
__exit:
	return nRet;
}

unsigned char* 
CsiDrv_lockDisplayBuffer(
	void 
)
{
	int i;
	struct v4l2_buffer buf;
	fd_set fds;
	struct timeval tv;
	
	FD_ZERO(&fds);
	FD_SET(CsiHandle, &fds);
		
	/* Timeout. */
	tv.tv_sec = 100;
	tv.tv_usec = 0;
	if(select(CsiHandle + 1, &fds, NULL, NULL, &tv) == 0) {
		DEBUG("select timeout\n");
		goto __fail;
	} 

	/* get ready buffer */
	memset((void *)&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;
	if(ioctl(CsiHandle, VIDIOC_DQBUF, &buf) < 0) {
		DEBUG("VIDIOC_DQBUF fail\n");
		goto __fail;
	}
	
#if 0
	DEBUG("DQBuf[%d] = 0x%x\n", buf.index, buf.m.userptr);
#endif
	return (unsigned char *)buf.m.userptr;

__fail:
	return 0;
}

void 
CsiDrv_unlockDisplayBuffer(
	unsigned char* ptr
)
{
	int i;
	unsigned int index, addr;
	struct v4l2_buffer buf;

	addr = (unsigned int)ptr;
	for(i=0; i<BUFFER_NO; i++) {	
		if(addr == CsiBuf[i].addr) {
			index = i;
			break;
		}
	}

	if(i == BUFFER_NO) {
		index = 0;
	}

	/* send empty buffer */
	memset((void *)&buf, 0, sizeof(buf));
	buf.index = index;
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;
	buf.m.userptr = (unsigned long)ptr;
	buf.length = CsiBuf[index].width * CsiBuf[index].height * 2;
	if(ioctl(CsiHandle, VIDIOC_QBUF, &buf) < 0) {
		DEBUG("VIDIOC_QBUF fail\n");
	}
	
#if 0	
	DEBUG("QBuf[%d] = 0x%x\n", buf.index, buf.m.userptr);
#endif
}

int 
CsiDrv_captureImage(
	CsiCaptureConfig* capture_config
)
{
	int nRet = 0;
	struct v4l2_format fmt;
	gpCsiCapture_t capture;
	gpMipiCCIR601_t mipi_ccir601, mipi_ccir601_bk;

	/* backup parameters */
	memset((void *)&mipi_ccir601_bk, 0, sizeof(mipi_ccir601_bk));
	if (ioctl(MipiHandle, MIPI_IOCTL_G_CCIR601, &mipi_ccir601_bk) < 0) {
		DEBUG("MIPI_IOCTL_S_CCIR601 fail!\n");
		RETURN(ERR_FMT_FAIL);
	}

	/* chage to capture size */
	memcpy((void *)&mipi_ccir601, (void *)&mipi_ccir601_bk, sizeof(mipi_ccir601));
	mipi_ccir601.h_size = capture_config->mImageWidth;
	mipi_ccir601.v_size = capture_config->mImageHeight;
	if (ioctl(MipiHandle, MIPI_IOCTL_S_CCIR601, &mipi_ccir601) < 0) {
		DEBUG("MIPI_IOCTL_S_CCIR601 fail!\n");
		RETURN(ERR_FMT_FAIL);
	}

	/* capture once */
	memset((void *)&capture, 0, sizeof(capture));
	capture.width = capture_config->mImageWidth;
	capture.height = capture_config->mImageHeight;
	capture.pixelformat = SensorFmt;
	capture.priv = 0xFF;
	capture.buffaddr = (unsigned int)capture_config->mImageBuffer;
	capture.waitcnt = capture_config->mWaitFrame;
	if(ioctl(CsiHandle, VIDIOC_CAPTURE, &capture) < 0) {
		DEBUG("VIDIOC_CAPTURE fail!\n");
		RETURN(ERR_FMT_FAIL);
	} 

	/* resume */
	if (ioctl(MipiHandle, MIPI_IOCTL_S_CCIR601, &mipi_ccir601_bk) < 0) {
		DEBUG("MIPI_IOCTL_S_CCIR601 fail!\n");
		RETURN(ERR_FMT_FAIL);
	}

#if 1
	/* skin fps, it so can do by user*/
	nRet = 2;
	while(nRet > 0) {
		unsigned char *addr = CsiDrv_lockDisplayBuffer();
		CsiDrv_unlockDisplayBuffer(addr);
		nRet--;
	}
#endif

__exit:
	return nRet;
}

void 
CsiDrv_onVBlkIrq( 
	void
)
{
	/* nothing to do */

}


