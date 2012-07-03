#ifndef __IMAGE_DECODER_H__
#define __IMAGE_DECODER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

//#include <linux/uaccess.h>

#include <mach/common.h>
#include <mach/typedef.h>
#include <mach/gp_display.h>
#include <mach/gp_ceva.h>
#include <mach/gp_scale.h>
#include <mach/gp_chunkmem.h>
#include "ceva.h"
#include "chunkmem.h"

#define STATUS_FAIL             -1
#define STATUS_OK                0
#define DECODE_SUCCESS			 1
#define FILE_HANDLE_READ_ERROR	-1
#define INPUT_DATA_FORMAT_ERROR	-2
#define HAVE_NO_THUMBNAIL		-3
#define SCALER_FAIL				-4
#define MEMORY_MALLOC_ERROR		-5
#define MEMORY_FREE_ERROR		-6
#define DECODE_FAIL				-7


#define DBG_PRINT	printf


typedef struct {
	SINT32 image_source;          	// File handle/resource handle/pointer
	UINT8  output_format;
	UINT8  output_ratio;             // 0=Fit to output_buffer_width and output_buffer_height, 1=Maintain ratio and fit to output_buffer_width or output_buffer_height, 2=Same as 1 but without scale up, 3=Special case for thumbnail show
	UINT16 output_buffer_width;
	UINT16 output_buffer_height;
	UINT16 output_image_width;
	UINT16 output_image_height;
	UINT16 output_image_pos_x;	
	UINT16 output_image_pos_y;		
	UINT32 out_of_boundary_color;
	UINT32 output_decode_type;     //for  decode thumbnail 
	void*  output_buffer_pointer;
} IMAGE_DECODE_STRUCT;

typedef enum
{
		IMG_AUTOFORMAT=0,
		JPEG,
		JPEG_P,		// JPEG Progressive
		MJPEG_S,	// Single JPEG from M-JPEG video
		GIF,
		BMP
} IMAGE_FORMAT;

typedef struct {
		IMAGE_FORMAT	Format;
		UINT8*			SubFormat;	
		UINT32		Width;					// unit: pixel
		UINT32		Height;					// unit: pixel
		UINT8			Color;					// unit: color number
		UINT32		FileSize;				// unit: byte
		UINT8			*FileDate;				// string pointer
		UINT8			*FileName;				// file name pointer
		UINT8			Orientation;			// Portrait or Landscape
		UINT8			*PhotoDate;				// string pointer
		UINT8			ExposureTime;			// Exporsure Time
		UINT8			FNumber;				// F Number
		UINT32		ISOSpeed;				// ISO Speed Ratings		
} IMAGE_INFO;


typedef enum
{
		SOURCE_TYPE_FS=0,
		SOURCE_TYPE_SDRAM,
		SOURCE_TYPE_NVRAM,
		SOURCE_TYPE_USER_DEFINE,
		SOURCE_TYPE_MAX
} SOURCE_TYPE;


typedef struct {
	SINT32 	image_source;          	// File handle/resource handle/pointer
	UINT32 	source_size;            // File size or buffer size
	UINT8 	source_type;            // TK_IMAGE_SOURCE_TYPE_FILE/TK_IMAGE_SOURCE_TYPE_BUFFER/TK_IMAGE_SOURCE_TYPE_NVRAM
	SINT8 	parse_status;						// 0=ok, others=fail
	UINT8 	image_type;							// TK_IMAGE_TYPE_ENUM
	UINT8 	orientation;						
	UINT8 	date_time_ptr[20];			// 20 bytes including NULL terminator. Format="YYYY:MM:DD HH:MM:SS"
	UINT16 	width;
	UINT16 	height;
} IMAGE_HEADER_PARSE_STRUCT;

typedef enum {
	TK_IMAGE_TYPE_JPEG = 0x1,
	TK_IMAGE_TYPE_PROGRESSIVE_JPEG,
	TK_IMAGE_TYPE_MOTION_JPEG,
	TK_IMAGE_TYPE_BMP,
	TK_IMAGE_TYPE_GIF,
	TK_IMAGE_TYPE_PNG,
	TK_IMAGE_TYPE_GPZP,
	TK_IMAGE_TYPE_MOV_JPEG,		//mov
	TK_IMAGE_TYPE_MPEG4,
	TK_IMAGE_TYPE_MAX
} TK_IMAGE_TYPE_ENUM;

typedef enum
{
		FIT_OUTPUT_SIZE=0,//Fit to output_buffer_width and output_buffer_height
		MAINTAIN_IMAGE_RATIO_1,//Maintain ratio and fit to output_buffer_width or output_buffer_height
		MAINTAIN_IMAGE_RATIO_2//Same as MAINTAIN_IMAGE_RATIO_1 but without scale up
}SCALER_OUTPUT_RATIO;

typedef enum
{
		SCALER_INPUT_FORMAT_RGB1555=0,
		SCALER_INPUT_FORMAT_RGB565,
		SCALER_INPUT_FORMAT_RGBG,
		SCALER_INPUT_FORMAT_GRGB,
		SCALER_INPUT_FORMAT_YUYV,
    SCALER_INPUT_FORMAT_UYVY			
} IMAGE_INPUT_FORMAT;

typedef enum
{
		IMAGE_OUTPUT_FORMAT_RGB1555=0,
		IMAGE_OUTPUT_FORMAT_RGB565,
		IMAGE_OUTPUT_FORMAT_RGBG,
		IMAGE_OUTPUT_FORMAT_GRGB,
		IMAGE_OUTPUT_FORMAT_YUYV,
		IMAGE_OUTPUT_FORMAT_UYVY,	
		IMAGE_OUTPUT_FORMAT_YUYV8X32,		
		IMAGE_OUTPUT_FORMAT_YUYV8X64,		
		IMAGE_OUTPUT_FORMAT_YUYV16X32,		
		IMAGE_OUTPUT_FORMAT_YUYV16X64,
		IMAGE_OUTPUT_FORMAT_YUYV32X32,	
		IMAGE_OUTPUT_FORMAT_YUYV64X64,
		IMAGE_OUTPUT_FORMAT_YUV422,
		IMAGE_OUTPUT_FORMAT_YUV420,
		IMAGE_OUTPUT_FORMAT_YUV411,
		IMAGE_OUTPUT_FORMAT_YUV444,        
		IMAGE_OUTPUT_FORMAT_Y_ONLY        
} IMAGE_OUTPUT_FORMAT;


typedef struct {
		SINT16		FileHandle;    
		UINT8      *OutputBufPtr;     
		UINT16		OutputBufWidth;
		UINT16		OutputBufHeight;
		UINT16		OutputWidth;
		UINT16		OutputHeight;
		UINT32    OutBoundaryColor;
		SCALER_OUTPUT_RATIO ScalerOutputRatio;
		IMAGE_OUTPUT_FORMAT	OutputFormat;
} IMAGE_ARGUMENT;

typedef struct {
		UINT32 					 MemoryID;
		chunk_block_t 			 Out_file_block;
}ST_IMAGE_MEMORY;

typedef struct {
		IMAGE_DECODE_STRUCT  image_input_param;
		ceva_video_decode_t  vdt;
		scale_content_t 		 sct;
		ST_IMAGE_MEMORY			 BlockMM;
		UINT32 	source_size;            // File size or buffer size
		UINT8 	source_type;            // TK_IMAGE_SOURCE_TYPE_FILE/TK_IMAGE_SOURCE_TYPE_BUFFER/TK_IMAGE_SOURCE_TYPE_NVRAM
		SINT8 	decode_status;						// 0=ok, others=fail
		UINT8 	image_type;							// TK_IMAGE_TYPE_ENUM
		UINT8 	orientation;						
		UINT8 	date_time_ptr[20];			// 20 bytes including NULL terminator. Format="YYYY:MM:DD HH:MM:SS"
		UINT16 	width;
		UINT16 	height;
		SINT32  position;
}ST_IMAGE_DECODE_OBJ;

typedef struct ImageHeader_S{
	UINT32 ImgFormat;
	UINT32 ImgSize;
	UINT32 ImgMainWidth;
	UINT32 ImgMainHeight;
	UINT8  ImgDateTime[20];
	UINT32 ImgOrient;
	UINT32 ImgPosition;
}ImageHeader_T;

typedef struct gpDecodeImg_s { 
        UINT16 width;             
        UINT16 height; 
        UINT16 bpl;               
        UINT32  type;               
        UINT32  ScalerType;
		ST_IMAGE_MEMORY *outmemory;
		ImageHeader_T ImgHeader;		
}gpDecodeImg_t;

#define C_SCALER1_CTRL			1
#define C_SCALER2_CTRL			2

extern SINT32 Image_Codec_Load(SINT32 TargetDecodeType,SINT32 iForceChange);
extern void   Image_Codec_Unload(void);

extern SINT32 ap_image_chunkfree(ST_IMAGE_MEMORY *chunkmem);
extern SINT32 flash_jpeg_image_decode(UINT8 *cInBuffer, UINT8 *cOutBuffer,SINT32 iImgWidth,SINT32 iImgheight,SINT32 imgsize,SINT32 OutDataType); 
extern SINT32 jpeg_image_decode(UINT8 *filename,gpDecodeImg_t *OutputImg,UINT32 parameter);
extern SINT32 jpeg_thumbnail_decode (UINT8 *filename,gpDecodeImg_t *OutputImg,UINT32 parameter);
extern SINT32 jpeg_parse_thumnail_header(UINT8 *filename,ImageHeader_T *pimageobj);
extern SINT32 jpeg_parse_header(UINT8 *filename,ImageHeader_T *pimageobj);
extern SINT32 jpeg_thumbnail_decode_outbuf (UINT8 *filename,gpDecodeImg_t *OutputImg,UINT8 *outputbuf);
extern SINT32 jpeg_image_decode_outbuf(UINT8 *filename,gpDecodeImg_t *OutputImg,UINT8 *outputbuf);
extern void jpeg_image_set_working_buffer(UINT8 *workingbuf,UINT32 WorkBufSize);
extern SINT32 jpeg_decode_init(void);
extern void jpeg_decode_uninit(void);
#endif