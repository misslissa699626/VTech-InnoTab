#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
//#include <linux/fs.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
//#include "mach/gp_csi.h"

#include "mach/gp_display.h"
#include "mach/gp_chunkmem.h"

#include <mach/gp_2d.h>
#include <mach/gp_scale.h>
#include "diag.h"
#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define NTSC_WIDTH 720
#define NTSC_HEIGHT 480
#define G2D_DEV_NAME "/dev/graphic"

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
 #define CHECK_ALIGN(var) (((SINT32)(var) & 0x3 ) ? 0 : 1)
 #define CHECK_SIZE(var) (((SINT32)(var) > 16) ? 1 : 0)
static void init_userp(
	int fd,
	int bufnum
)
{
	struct v4l2_requestbuffers req;
	
	CLEAR(req);
	
	req.count = bufnum;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	
	ioctl(fd, VIDIOC_REQBUFS, &req);
}

static void init_fb(short *data, short a, short b, short c, short d)
{
	int i, j, k;
	
	for( k=0; k<480; k++ ) {
		for( j=0; j<4; j++) {
			for( i=0; i<160; i++ ) {
				if( j==0 )
					data[k*640+j*160+i]=a;
				else if( j==1 )
					data[k*640+j*160+i]=b;
				else if( j==2 )
					data[k*640+j*160+i]=c;
				else
					data[k*640+j*160+i]=d;
			}
		}
	}
	
}

SINT32 
getBpp(
	SINT32 bitmapType
)
{
	SINT32 bpp = 0;
	
	switch (bitmapType) {
	case SP_BITMAP_YCbCr400:
	case SP_BITMAP_YUV400:
	case SP_BITMAP_YCbCr420:      
	case SP_BITMAP_YUV420:
	case SP_BITMAP_8BPP:
		bpp = 1;
		break;
	case SP_BITMAP_RGB565:
	case SP_BITMAP_RGAB5515:
	case SP_BITMAP_ARGB1555:
	case SP_BITMAP_ARGB4444:
	case SP_BITMAP_BGAR5515:
	case SP_BITMAP_ABGR1555:
	case SP_BITMAP_ABGR4444:
	case SP_BITMAP_BGR565:
	case SP_BITMAP_RGB555:
	case SP_BITMAP_YCbYCr:
	case SP_BITMAP_YUYV:
	case SP_BITMAP_4Y4U4Y4V:
	case SP_BITMAP_4Y4Cb4Y4Cr:
	case SP_BITMAP_YCbCr422:
	case SP_BITMAP_YUV422:
		bpp = 2;
		break;
	case SP_BITMAP_RGB888:
	case SP_BITMAP_BGR888:
	case SP_BITMAP_YCbCr:
	case SP_BITMAP_YUV:
		bpp = 3;
		break;
		
	case SP_BITMAP_ARGB8888:
	case SP_BITMAP_ABGR8888:
	case SP_BITMAP_YCbCr444:
	case SP_BITMAP_YUV444:
		bpp = 4;
		break;
	#if 0	/*is supported?*/
	case SP_BITMAP_1BPP:
		bpp = 1.0 / 8;
		break;
	case SP_BITMAP_2BPP:
		bpp = 2.0 / 8;
		break;
	case SP_BITMAP_4BPP:
		bpp = 4.0 /8;
		break;
	#endif	
	default:
		bpp = 0;
		break;
	}

	return bpp;
}

SINT32 
gp2dScale(
    spBitmap_t *pdstBitmap, 
    spRect_t dstRect, 
    spBitmap_t *psrcBitmap, 
    spRect_t srcRect
)
{
    scale_content_t sct;
	g2d_draw_ctx_t drawCtx;
    SINT32 handle = -1;
    SINT32 ret = SP_OK;

	/*!<scaling with scalar must need the follow conditions
	   1.size is not smaller than 16X16;
	   2.addr and pitch must 4 byte aligned
	   if not use 2D to scale*/
	if (CHECK_ALIGN(pdstBitmap->pData + dstRect.x * getBpp(pdstBitmap->type))
		&& (CHECK_ALIGN(dstRect.width * getBpp(pdstBitmap->type)))
		&& (CHECK_ALIGN(psrcBitmap->pData + srcRect.x * getBpp(psrcBitmap->type)))
		&& (CHECK_ALIGN(srcRect.width * getBpp(psrcBitmap->type)))
		&& (CHECK_SIZE(dstRect.width))
		&& (CHECK_SIZE(dstRect.height))
		&& (CHECK_SIZE(srcRect.width))
		&& (CHECK_SIZE(srcRect.height))) /*if (0)*/{
		/*!<use scalar to scale*/
		//diagLog(DIAG_LVL_INFO, "Use /dev/scalar to do scale...\n");
   		handle = open("/dev/scalar", O_RDONLY);
    	if (handle < 0) {
    		diagLog(DIAG_LVL_ERROR, "open scale dev fail!\n");
    		return SP_FAIL;
    	}

    	memset(&sct, 0, sizeof(sct));    
    	memcpy(&sct.dst_img, pdstBitmap, sizeof(spBitmap_t));
    	memcpy(&sct.scale_rgn, &dstRect, sizeof(spRect_t));
    	memcpy(&sct.src_img, psrcBitmap, sizeof(spBitmap_t));
    	memcpy(&sct.clip_rgn, &srcRect, sizeof(spRect_t));
    
    	if ((ret = ioctl(handle, SCALE_IOCTL_TRIGGER, &sct)) < 0) {
    		diagLog(DIAG_LVL_ERROR, "do scale error occur!\n");
			ret = SP_FAIL;
			goto _resourceCollection;
    	}	

	}
	else {
		/*!<use 2D to scale*/
		//diagLog(DIAG_LVL_INFO, "Use /dev/graphic to do scale...\n");
		handle = open(G2D_DEV_NAME, O_RDWR);
		if (handle < 0) {
			diagLog(DIAG_LVL_ERROR, "[%s:%s:%d] [%s] open fail!\n", __FILE__, __FUNCTION__, __LINE__, G2D_DEV_NAME);	
			return SP_FAIL;
		}	
    
		memset(&drawCtx, 0, sizeof(g2d_draw_ctx_t));
		memcpy(&drawCtx.dst, pdstBitmap, sizeof(spBitmap_t));
		memcpy(&drawCtx.dst_rect, &dstRect, sizeof(spRect_t));
		memcpy(&drawCtx.src, psrcBitmap, sizeof(spBitmap_t));
		memcpy(&drawCtx.src_rect, &srcRect, sizeof(spRect_t));
		drawCtx.func_flag = G2D_FUNC_ROP | G2D_FUNC_SCALE;
		drawCtx.rop.fg_rop = G2D_ROP_SRCCOPY;
	
		ret = ioctl(handle, G2D_IOCTL_DRAW_BITMAP, &drawCtx);
		if (ret < 0) {
			diagLog(DIAG_LVL_ERROR, "[%s:%d]G2D_IOCTL_DRAW_BITMAP, errcode = 0x%x\n", __FUNCTION__, __LINE__, ret);	
			ret = SP_FAIL;
			goto _resourceCollection;
		}
	}
    
	_resourceCollection:
	if (handle >= 0) {
		close(handle);
	}
	
	return ret;
}

int
main(
	int argc,
	char **argv
)
{
	int dispDev, csiDev;
	int chunkMem;
	chunk_block_t priBlk0, priBlk1;
	gp_disp_res_t panelRes;
	gp_bitmap_t priBitmap;
	UINT16 *data;
	
	fd_set fds;
	struct timeval tv;
	int r, bufnum;
	struct v4l2_buffer buf;
	struct v4l2_format fmt;
	struct v4l2_queryctrl qc;
	struct v4l2_input in;
	spRect_t dstRect={0,0,0,0};
	spRect_t srcRect={0,0,0,0};
	spBitmap_t dst;
   	spBitmap_t src;

	/* Opening the device dispDev */
	dispDev = open("/dev/disp0",O_RDWR);
	printf("dispDev = %d\n", dispDev);

	ioctl(dispDev, DISPIO_SET_INITIAL, 0);
	ioctl(dispDev, DISPIO_GET_PANEL_RESOLUTION, &panelRes);
	dstRect.width = panelRes.width;
	dstRect.height=panelRes.height;
    	srcRect.width=NTSC_WIDTH; 
	srcRect.height=NTSC_HEIGHT;
	/* Opening /dev/chunkmem */
	chunkMem = open("/dev/chunkmem", O_RDWR);

	/* Allocate primary frame buffer */
	priBlk0.size = (NTSC_WIDTH) * (NTSC_HEIGHT) * getBpp(SP_BITMAP_YCbYCr);
	priBlk1.size = (panelRes.width) * (panelRes.height) *getBpp(SP_BITMAP_YCbYCr);
	ioctl(chunkMem, CHUNK_MEM_ALLOC, (unsigned long)&priBlk0);
	ioctl(chunkMem, CHUNK_MEM_ALLOC, (unsigned long)&priBlk1);

	dst.width = panelRes.width;
    	dst.height = panelRes.height;
	dst.bpl = panelRes.width * getBpp(SP_BITMAP_YCbYCr);
    	dst.pData =  priBlk1.addr;
    	dst.type = SP_BITMAP_YCbYCr;
   	 src.width = NTSC_WIDTH;
   	 src.height =NTSC_HEIGHT;
	src.bpl = NTSC_WIDTH *getBpp(SP_BITMAP_YCbYCr);
    	src.pData = priBlk0.addr;
   	 src.type = SP_BITMAP_YCbYCr;

	//gp2dScale(&dst, dstRect, &src, srcRect);
	/* Set primary layer bitmap */
	priBitmap.width = panelRes.width;
	priBitmap.height = panelRes.height;
	priBitmap.bpl = panelRes.width*getBpp(SP_BITMAP_YCbYCr);
	priBitmap.type = SP_BITMAP_YCbYCr;
	priBitmap.pData = priBlk1.addr;

	ioctl(dispDev, DISPIO_SET_PRI_BITMAP, &priBitmap);
	/* Fill primary bitmap data */
	#if 0
	data = (UINT16*) priBlk0.addr;
	init_fb(data, 0x0000, 0x001f, 0x0513, 0xffff);
	data = (UINT16*) priBlk1.addr;
	init_fb(data, 0xffff, 0x0513, 0x001f, 0x0000);
	#endif

	ioctl(dispDev, DISPIO_SET_PRI_ENABLE, 1);
	ioctl(dispDev, DISPIO_SET_UPDATE, 0);

	csiDev = open("/dev/csi0",O_RDWR);
	
	in.index = 0;
	ioctl(csiDev, VIDIOC_ENUMINPUT, &in);
	printf("name=%s\n", in.name);
	
	CLEAR(fmt);
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.width = NTSC_WIDTH;
	fmt.fmt.pix.height = NTSC_HEIGHT;
	ioctl(csiDev, VIDIOC_TRY_FMT, &fmt);
	
	CLEAR(qc);
	qc.id = V4L2_CID_BRIGHTNESS;
	ioctl(csiDev, VIDIOC_QUERYCTRL, &qc);
	printf("max=%d\n", qc.maximum);
	
	init_userp(csiDev, 1);

	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;
	buf.index = 0;
	buf.m.userptr = (unsigned long)priBlk0.addr;
	ioctl(csiDev, VIDIOC_QBUF, &buf);

//	buf.index = 1;
//	buf.m.userptr = (unsigned long)priBlk1.addr;
//	ioctl(csiDev, VIDIOC_QBUF, &buf);
	
//	buf.index = 2;
//	buf.m.userptr = (unsigned long)priBlk2.addr;
//	ioctl(csiDev, VIDIOC_QBUF, &buf);

	FD_ZERO (&fds);
	FD_SET (csiDev, &fds);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	ioctl(csiDev, VIDIOC_STREAMON, NULL);

	while(1)
	{
		r = select(csiDev+1, &fds, NULL, NULL, NULL);//&tv);
		gp2dScale(&dst, dstRect, &src, srcRect);
		CLEAR (buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		ioctl(csiDev, VIDIOC_DQBUF, &buf);
		
		ioctl(csiDev, VIDIOC_QBUF, &buf);
/*		ioctl(csiDev, 3, &bufnum);
		if( bufnum==0 )
			priBitmap.pData = priBlk0.addr;
		else if(bufnum==1)
			priBitmap.pData = priBlk1.addr;
		else
			priBitmap.pData = priBlk2.addr;
		ioctl(dispDev, DISPIO_SET_PRI_BITMAP, &priBitmap);
		ioctl(dispDev, DISPIO_SET_PRI_ENABLE, 1);
		ioctl(dispDev, DISPIO_SET_UPDATE, 0);
		ioctl(dispDev, DISPIO_WAIT_FRAME_END, 0);*/
			
/*		r = select(csiDev+1, &fds, NULL, NULL, &tv);
		priBitmap.pData = priBlk1.addr;
		ioctl(dispDev, DISPIO_SET_PRI_BITMAP, &priBitmap);
		ioctl(dispDev, DISPIO_SET_PRI_ENABLE, 1);
		ioctl(dispDev, DISPIO_SET_UPDATE, 0);

		r = select(csiDev+1, &fds, NULL, NULL, &tv);
		priBitmap.pData = priBlk2.addr;
		ioctl(dispDev, DISPIO_SET_PRI_BITMAP, &priBitmap);
		ioctl(dispDev, DISPIO_SET_PRI_ENABLE, 1);
		ioctl(dispDev, DISPIO_SET_UPDATE, 0);*/

	}
	//	close(dispDev);

//	ioctl(chunkMem, CHUNK_MEM_FREE, (unsigned long)&priBlk);
//	close(chunkMem);

	return 0;
	
/*	int i, ret;
	int fdcsi;
	struct v4l2_queryctrl qc;

	printf("csi test!\n");
	
	fdcsi = open("/dev/csi0", O_RDWR);
	
//	init_userp(fdcsi, 0x2000);
	
	close(fdcsi);
	
	return 0;*/
}

