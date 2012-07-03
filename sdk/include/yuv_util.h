/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2007 by Sunplus mMedia Inc.                      *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  mMedia Inc. All rights are reserved by Sunplus mMedia Inc.            *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus mMedia Inc. reserves the right to modify this software        *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus mMedia Inc.                                                   *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *                                                                        *
 **************************************************************************/
/**
 * \file image_reader.h
 * \brief Contains APIs for using the Image Reader.
 *
 */

#ifndef _YUV_UTIL_H_
#define _YUV_UTIL_H_

#include "typedef.h"


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct spYuvQuad_s {
	UINT8 v;
	UINT8 u;
	UINT8 y;
	UINT8 alpha;
} spYuvQuad_t;


typedef struct spYuvWriter_s {
	UINT16 bytesPerLine;
	UINT16 offset;
	UINT16 yOffset;
	UINT8 *pSegY;
	UINT8 *pOrgY;
	UINT8 *pOrgU;
	UINT8 *pOrgV;
	UINT8 *pY;
	UINT8 *pU;
	UINT8 *pV;
} spYuvWriter_t;


typedef struct spYuvReader_s {
	UINT16 bytesPerLine;
	UINT16 offset;
	UINT16 yOffset;
	UINT8 *pSegY;
	UINT8 *pOrgY;
	UINT8 *pOrgU;
	UINT8 *pOrgV;
	UINT8 *pY;
	UINT8 *pU;
	UINT8 *pV;
} spYuvReader_t;


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
#ifdef WIN32
extern spRgbQuad_t tableYuvToRgb[256][256][256];
extern spYuvQuad_t tableRgbToYuv[256][256][256];
#endif


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern spRgbQuad_t yuvPixel(spBitmap_t *pbitmap, SINT32 x,	SINT32 y);


/**************************************************************************
 * YUV422 Writer
 **************************************************************************/
#define YUV_WRITER_INIT(yuvWriter, pDst, dstRoi) \
	do { \
		UINT8 *pSegY; \
		yuvWriter.bytesPerLine = pDst->bpl; \
		yuvWriter.yOffset = yuvWriter.offset = dstRoi.x & 0x7; \
		pSegY = (UINT8 *)pDst->pData + dstRoi.y * yuvWriter.bytesPerLine + ((dstRoi.x >> 3) << 4); \
		yuvWriter.pY = yuvWriter.pOrgY = pSegY + yuvWriter.offset; \
		yuvWriter.pU = yuvWriter.pOrgU = pSegY + 8 + (yuvWriter.offset >> 1); \
		yuvWriter.pV = yuvWriter.pOrgV = pSegY + 12 + (yuvWriter.offset >> 1); \
	} while(0)


#define YUV_WRITER_NEXTLINE(yuvWriter) \
	do { \
		yuvWriter.yOffset = yuvWriter.offset; \
		yuvWriter.pY = yuvWriter.pOrgY = yuvWriter.pOrgY + yuvWriter.bytesPerLine; \
		yuvWriter.pU = yuvWriter.pOrgU = yuvWriter.pOrgU + yuvWriter.bytesPerLine; \
		yuvWriter.pV = yuvWriter.pOrgV = yuvWriter.pOrgV + yuvWriter.bytesPerLine; \
	} while(0)


#define YUV_WRITER_NEXT(yuvWriter) \
	do { \
		yuvWriter.pY++; \
		if ((yuvWriter.yOffset & 1) == 1) { \
			yuvWriter.pU++; \
			yuvWriter.pV++; \
		} \
		if ((yuvWriter.yOffset & 7) == 7) { \
			yuvWriter.pY += 8; \
			yuvWriter.pU += 12; \
			yuvWriter.pV += 12; \
			yuvWriter.yOffset = 0; \
		} \
		else { \
			yuvWriter.yOffset++; \
		} \
	} while(0)


#define YUV_WRITER_WRITE_YUV(yuvWriter, y8, u, v) \
	do { \
		if ((yuvWriter.yOffset & 1) == 0) { \
			*yuvWriter.pY = y8; \
			*yuvWriter.pU = u; \
			*yuvWriter.pV = v; \
			yuvWriter.pY++; \
		} \
		else { \
			*yuvWriter.pY = y8; \
			*yuvWriter.pU = (*yuvWriter.pU + u) >> 1; \
			*yuvWriter.pV = (*yuvWriter.pV + v) >> 1; \
			yuvWriter.pY++; \
			yuvWriter.pU++; \
			yuvWriter.pV++; \
		} \
		if ((yuvWriter.yOffset & 7) == 7) { \
			yuvWriter.pY += 8; \
			yuvWriter.pU += 12; \
			yuvWriter.pV += 12; \
			yuvWriter.yOffset = 0; \
		} \
		else { \
			yuvWriter.yOffset++; \
		} \
	} while(0)


#ifndef WIN32

#define YUV_WRITER_WRITE_RGB(yuvWriter, red, green, blue) \
	do { \
		SINT32 y8, u, v; \
		SINT32 _r = red; \
		SINT32 _g = green; \
		SINT32 _b = blue; \
		y8 = (19595 * _r +  38470 * _g + 7471 * _b) >> 16; \
		u = ((-11076 * _r - 21692 * _g + 32768 * _b) >> 16) + 128; \
		v = ((32768 * _r - 27460 * _g - 5308 * _b) >> 16) + 128; \
		\
		if (y8 < 0) \
			y8 = 0; \
		else if (y8 > 255) \
			y8 = 255; \
		\
		if (u < 0) \
			u = 0; \
		else if (u > 255) \
			u = 255; \
		\
		if (v < 0) \
			v = 0; \
		else if (v > 255) \
			v = 255; \
		\
		if ((yuvWriter.yOffset & 1) == 0) { \
			*yuvWriter.pY = y8; \
			*yuvWriter.pU = u; \
			*yuvWriter.pV = v; \
			yuvWriter.pY++; \
		} \
		else { \
			*yuvWriter.pY = y8; \
			*yuvWriter.pU = (*yuvWriter.pU + u) >> 1; \
			*yuvWriter.pV = (*yuvWriter.pV + v) >> 1; \
			yuvWriter.pY++; \
			yuvWriter.pU++; \
			yuvWriter.pV++; \
		} \
		if ((yuvWriter.yOffset & 7) == 7) { \
			yuvWriter.pY += 8; \
			yuvWriter.pU += 12; \
			yuvWriter.pV += 12; \
			yuvWriter.yOffset = 0; \
		} \
		else { \
			yuvWriter.yOffset++; \
		} \
	} while(0)

#else

#define YUV_WRITER_WRITE_RGB(yuvWriter, red, green, blue) \
	do { \
		spYuvQuad_t *_pyuv = &tableRgbToYuv[red][green][blue]; \
		\
		if ((yuvWriter.yOffset & 1) == 0) { \
			*yuvWriter.pY = _pyuv->y; \
			*yuvWriter.pU = _pyuv->u; \
			*yuvWriter.pV = _pyuv->v; \
			yuvWriter.pY++; \
		} \
		else { \
			*yuvWriter.pY = _pyuv->y; \
			*yuvWriter.pU = (*yuvWriter.pU + _pyuv->u) >> 1; \
			*yuvWriter.pV = (*yuvWriter.pV + _pyuv->v) >> 1; \
			yuvWriter.pY++; \
			yuvWriter.pU++; \
			yuvWriter.pV++; \
		} \
		if ((yuvWriter.yOffset & 7) == 7) { \
			yuvWriter.pY += 8; \
			yuvWriter.pU += 12; \
			yuvWriter.pV += 12; \
			yuvWriter.yOffset = 0; \
		} \
		else { \
			yuvWriter.yOffset++; \
		} \
	} while(0)

#endif



/**************************************************************************
 * YUV422 Reader
 **************************************************************************/
#define YUV_READER_INIT(yuvReader, pSrc, srcRoi) \
	do { \
		UINT8 *pSegY; \
		yuvReader.bytesPerLine = pSrc->bpl; \
		yuvReader.yOffset = yuvReader.offset = srcRoi.x & 0x7; \
		pSegY = (UINT8 *)pSrc->pData + srcRoi.y * yuvReader.bytesPerLine + ((srcRoi.x >> 3) << 4); \
		yuvReader.pY = yuvReader.pOrgY = pSegY + yuvReader.offset; \
		yuvReader.pU = yuvReader.pOrgU = pSegY + 8 + (yuvReader.offset >> 1); \
		yuvReader.pV = yuvReader.pOrgV = pSegY + 12 + (yuvReader.offset >> 1); \
	} while(0)


#define YUV_READER_NEXTLINE(yuvReader) \
	do { \
		yuvReader.yOffset = yuvReader.offset; \
		yuvReader.pY = yuvReader.pOrgY = yuvReader.pOrgY + yuvReader.bytesPerLine; \
		yuvReader.pU = yuvReader.pOrgU = yuvReader.pOrgU + yuvReader.bytesPerLine; \
		yuvReader.pV = yuvReader.pOrgV = yuvReader.pOrgV + yuvReader.bytesPerLine; \
	} while(0)


#ifndef WIN32

#define YUV_READER_READ_RGB(yuvReader, red, green, blue) \
	do { \
		SINT32 y8, u, v; \
		SINT32 _r, _g, _b; \
		\
		y8 = (*yuvReader.pY) << 8; \
		u = *yuvReader.pU - 128; \
		v = *yuvReader.pV - 128; \
		\
		_r = ( y8 + 351 * v ) >> 8; \
		_g = ( y8 - 179 * v - 86 * u ) >> 8; \
		_b = ( y8 + 444 * u ) >> 8; \
		\
		if (_r < 0) \
			_r = 0; \
		else if (_r > 255) \
			_r = 255; \
		\
		if (_g < 0) \
			_g = 0; \
		else if (_g > 255) \
			_g = 255; \
		\
		if (_b < 0) \
			_b = 0; \
		else if (_b > 255) \
			_b = 255; \
		\
		red = _r; \
		green = _g; \
		blue = _b; \
		\
		yuvReader.pY++; \
		if ((yuvReader.yOffset & 1) == 1) { \
			yuvReader.pU++; \
			yuvReader.pV++; \
		} \
		if ((yuvReader.yOffset & 7) == 7) { \
			yuvReader.pY += 8; \
			yuvReader.pU += 12; \
			yuvReader.pV += 12; \
			yuvReader.yOffset = 0; \
		} \
		else { \
			yuvReader.yOffset++; \
		} \
	} while(0)

#else

#define YUV_READER_READ_RGB(yuvReader, r, g, b) \
	do { \
		spRgbQuad_t *_prgb; \
		_prgb = &tableYuvToRgb[*yuvReader.pY][*yuvReader.pU][*yuvReader.pV]; \
		r = _prgb->red; \
		g = _prgb->green; \
		b = _prgb->blue; \
		\
		yuvReader.pY++; \
		if ((yuvReader.yOffset & 1) == 1) { \
			yuvReader.pU++; \
			yuvReader.pV++; \
		} \
		if ((yuvReader.yOffset & 7) == 7) { \
			yuvReader.pY += 8; \
			yuvReader.pU += 12; \
			yuvReader.pV += 12; \
			yuvReader.yOffset = 0; \
		} \
		else { \
			yuvReader.yOffset++; \
		} \
	} while(0)

#endif



/**************************************************************************
 * YUV422 Copy
 **************************************************************************/
#define YUV_WRITER_COPY_ONE(yuvWriter, yuvReader) \
	do { \
		*yuvWriter.pY = *yuvReader.pY; \
		*yuvWriter.pU = *yuvReader.pU; \
		*yuvWriter.pV = *yuvReader.pV; \
		\
		yuvReader.pY++; \
		if ((yuvReader.yOffset & 1) == 1) { \
			yuvReader.pU++; \
			yuvReader.pV++; \
		} \
		if ((yuvReader.yOffset & 7) == 7) { \
			yuvReader.pY += 8; \
			yuvReader.pU += 12; \
			yuvReader.pV += 12; \
			yuvReader.yOffset = 0; \
		} \
		else { \
			yuvReader.yOffset++; \
		} \
		\
		yuvWriter.pY++; \
		if ((yuvWriter.yOffset & 1) == 1) { \
			yuvWriter.pU++; \
			yuvWriter.pV++; \
		} \
		if ((yuvWriter.yOffset & 7) == 7) { \
			yuvWriter.pY += 8; \
			yuvWriter.pU += 12; \
			yuvWriter.pV += 12; \
			yuvWriter.yOffset = 0; \
		} \
		else { \
			yuvWriter.yOffset++; \
		} \
	} while(0)


#endif
