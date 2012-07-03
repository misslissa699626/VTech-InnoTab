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
 * \par Overview:
 * Use Image Reader to decode image into spBitmap_t object.
 * User can further process the spBitmap_t object via \link gcCreateFromBitmap() Graphics Context APIs\endlink.
 *
 * \par Creation:
 * To begin using the Image Reader, create an Image Reader object via imageReaderCreate().
 * \note It is the caller's responsibility to destroy the Image Reader object via imageReaderDestroy() once it is not used.
 *
 * \par Retrieve Information:
 * Image Reader provides APIs to retrieve information on the image file, such as:
 * - imageReaderGetSize();
 * - imageReaderGetOrientation();
 * - imageReaderGetMetadata().
 *
 * \par Configuration:
 * Before decoding the image file, the source and destination ROI (region of interest) must be configurated first:
 * - imageReaderSetSrcROI();
 * - imageReaderSetDstROI().
 *
 * \par
 * Helper function imageReaderSetScale() is provided to configurate the ROIs to predefined styles:
 * - #IMAGE_ORIGINAL;
 * - #IMAGE_FIT;
 * - #IMAGE_FILL;
 * - #IMAGE_STRETCH.
 *
 * \par
 * Image Reader also provides APIs to rotate and flip the image:
 * - imageReaderSetRotation();
 * - imageReaderSetFlip();
 *
 * \par Decode:
 * Finally, invoke imageReaderLoad() to decode the image.
 * Caller has to provide the memory for the spBitmap_t object.
 * \note After imageReaderLoad() is invoked, setting ROI, rotation and flip will have no effects.
 *
 * \par Example:
 * This is an example of using \link image_reader.h Image Reader \endlink to decode an image to
 * a spBitmap_t object of the size of the screen.
 * \include image_reader_fit_screen.c
 *
 * \note Image Reader currently only supports decoding JPEG files.
 */

/**
 * \example image_reader_fit_screen.c
 * This is an example of using \link image_reader.h Image Reader \endlink to decode an image to
 * a spBitmap_t object of the size of the screen.
 */

#ifndef _IMAGE_READER_H_
#define _IMAGE_READER_H_

#include <stdio.h>
#include "typedef.h"
//#include "spgui.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/** \brief Scaling of the image */
enum {
	IMAGE_SCALE_ORIGINAL, /*!< Don't scale the image */
	IMAGE_SCALE_FIT, /*!< Inform the Image Reader to fit the decoded bitmap */
	IMAGE_SCALE_CROP, /*!< Inform the Image Reader to crop the decoded bitmap */
	IMAGE_SCALE_FILL = IMAGE_SCALE_CROP, /*!< Inform the Image Reader to crop the decoded bitmap */
	IMAGE_SCALE_STRETCH, /*!< Inform the Image Reader to stretch the decoded bitmap */
	IMAGE_SCALE_MAX /*!< Unused */
};

/**
 * \brief Orientation of the image.
 * Description here are the same as jhead.
 */
enum {
	IMAGE_ORIENTATION_UNDEFINED, /*!< Undefined orientation */
	IMAGE_ORIENTATION_NORMAL, /*!< 1 */
	IMAGE_ORIENTATION_FLIP_HORIZONTAL, /*!< Left right reversed mirror */
	IMAGE_ORIENTATION_ROTATE_180, /*!< 3 */
	IMAGE_ORIENTATION_FLIP_VERTICAL, /*!< Upside down mirror */
	IMAGE_ORIENTATION_TRANSPOSE, /*!< Flipped about top-left <--> bottom-right axis */
	IMAGE_ORIENTATION_ROTATE_90, /*!< Rotate 90 to right it */
	IMAGE_ORIENTATION_TRANSVERSE, /*!< Flipped about top-right <--> bottom-left axis */
	IMAGE_ORIENTATION_ROTATE_270, /*!< Rotate 270 to right it */
	IMAGE_ORIENTATION_MAX /*!< Unused */
};

/** \brief Rotation of the image */
enum {
	IMAGE_CLOCKWISE_0, /*!< Do not rotate the image  */
	IMAGE_CLOCKWISE_90, /*!< Rotate the image 90 degrees clockwise */
	IMAGE_CLOCKWISE_180, /*!< Rotate the image 180 degrees clockwise */
	IMAGE_CLOCKWISE_270, /*!< Rotate the image 270 degrees clockwise */
	IMAGE_ROTATION_MAX /*!< Unused */
};

/** \brief Flip style of the image */
enum {
	IMAGE_NONE, /*!< Do not flip the image */
	IMAGE_HORIZONTAL, /*!< Horizontally flip the image */
	IMAGE_VERTICAL, /*!< Vertically flip the image */
	IMAGE_FLIP_MAX /*!< Unused */
};

/** \brief Image type */
enum {
	IMAGE_TYPE_UNKNOWN, /*!< Unknown image type */
	IMAGE_TYPE_JPEG_BASELINE, /*!< JPEG */
	IMAGE_TYPE_JPEG_PROGRESS, /*!< JPEG */
	IMAGE_TYPE_TIFF, /*!< TIFF */
	IMAGE_TYPE_GIF, /*!< GIF */
	IMAGE_TYPE_PNG, /*!< PNG */
	IMAGE_TYPE_TIF, /*!< TIF */
	IMAGE_TYPE_RAW, /*!< RAW */
	IMAGE_TYPE_BMP, /*!< BMP */
	IMAGE_TYPE_MAX /*!< Unused */
};

/** \brief Status of the Image Reader */
enum {
	IMAGE_READER_IDLE, /*!< Indicate the Image Reader is currently idle */
	IMAGE_READER_LOADING, /*!< Indicate the Image Reader is currently loading image */
	IMAGE_READER_DONE, /*!< Indicate the Image Reader loading is done */
	IMAGE_READER_CANCELLED /*!< Indicate the Image Reader is cancelled */
};

#define EXIF_IMAGE_DESC_LEN 64
#define EXIF_MAKER_NAME_LEN 64
#define EXIF_MODEL_NAME_LEN 64
#define EXIF_SOFT_NAME_LEN 20
#define EXIF_DATETIME_LEN 20
#define EXIF_COMMENTS_LEN 128
#define EXIF_GPS_LAT_LEN 31
#define EXIF_GPS_LONG_LEN 31
#define EXIF_GPS_ALT_LEN 20

#define EXTENSION_SIGNATURE 0xd839ab26
#define SP_THREAD_IMAGE_READER_PRIORITY 16
#define IMAGE_CDSP_NORMAL 2

#define IMAGE_HWALIGN_LIMIT 8 /*large than 8, use scaler1*/
#define IMAGE_HWALIGN_LIMIT2 1024 /*large than 1024, use scaler2*/
/*for scaler1*/
#define IMAGE_HWALIGN_DST_X 2
#define IMAGE_HWALIGN_DST_XMASK 0x01
#define IMAGE_HWALIGN_DST_WIDTH 2
#define IMAGE_HWALIGN_DST_WMASK 0x01
#define IMAGE_HWALIGN_SRC_X 8
#define IMAGE_HWALIGN_SRC_XMASK 0x07
#define IMAGE_HWALIGN_SRC_WIDTH 16
#define IMAGE_HWALIGN_SRC_WMASK 0x0F
#define IMAGE_HWALIGN_RGB565_SRC_X 2
#define IMAGE_HWALIGN_RGB565_SRC_XMASK 0x01
#define IMAGE_HWALIGN_RGB565_SRC_WIDTH 2
#define IMAGE_HWALIGN_RGB565_SRC_WMASK 0x01
/*for scaler2*/
#define IMAGE_HWALIGN_DST_X2 8
#define IMAGE_HWALIGN_DST_XMASK2 0x07
#define IMAGE_HWALIGN_DST_WIDTH2 8
#define IMAGE_HWALIGN_DST_WMASK2 0x07
#define IMAGE_HWALIGN_SRC_X2 4
#define IMAGE_HWALIGN_SRC_XMASK2 0x03
#define IMAGE_HWALIGN_SRC_WIDTH2 4
#define IMAGE_HWALIGN_SRC_WMASK2 0x03
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef void (*imageReaderCallback_t)(void *userdata, HANDLE hreader, UINT32 retCode, spBitmap_t *pbitmap);

/**
 * This structure stores Exif header image elements in a simple manner
 * Used to store camera data as extracted from the various ways that it can be
 * stored in an exif header.
 * Shamelessly copied from jhead.
 */
typedef struct spExif_s {
	UINT8  bExif;
	UINT32 dataFmt;
	UINT64 fileDateTime;
	UINT32 fileSize;
	SP_CHAR imageDesc[EXIF_IMAGE_DESC_LEN];
	SP_CHAR cameraMake[EXIF_MAKER_NAME_LEN];
	SP_CHAR cameraModel[EXIF_MODEL_NAME_LEN];
	SP_CHAR dateTime[EXIF_DATETIME_LEN];
	SP_CHAR dateTimeDigi[EXIF_DATETIME_LEN];
	SP_CHAR dateTimeOrig[EXIF_DATETIME_LEN];
	SINT32 height;
	SINT32 width;
	SINT32 orientation;
	SINT32 isColor;
	SINT32 process;
	SINT32 flashUsed;
	float  focalLength;
	UINT32 	flash;
	UINT32 	meteringmode;
	UINT32 exposureprogram;
	UINT32 exposureTime1;
	UINT32 exposureTime2;	
	UINT32 aperture1;
	UINT32 aperture2;	
	UINT32	FNumber1;
	UINT32  FNumber2;
	UINT32 focallength1;
	UINT32 focallength2;	
	UINT32 maxaperture1;
	UINT32 maxaperture2;
	UINT32 flashenergy1;
	UINT32 flashenergy2;	
	float distance;
	float ccdWidth;
	SINT32 shutterspeed1;
	SINT32 shutterspeed2;	
	SINT32 exposureBias1;
	SINT32 exposureBias2;
	float digitalZoomRatio;
	SINT32 focalLength35mmEquiv; /* Exif 2.2 tag - usually not present. */
	SINT32 whitebalance;
	SINT32 exposureMode;
	SINT32 isoEquivalent;
	SINT32 lightSource;
	SINT32 distanceRange;
	SP_CHAR comments[EXIF_COMMENTS_LEN];

	UINT32 thumbnailOffset; /* Exif offset to thumbnail */
	UINT32 thumbnailSize; /* Size of thumbnail. */
	UINT32 thumborientation;
	SINT32 gpsInfoPresent;
	SP_CHAR gpsLat[EXIF_GPS_LAT_LEN];
	SP_CHAR gpsLong[EXIF_GPS_LONG_LEN];
	SP_CHAR gpsAlt[EXIF_GPS_ALT_LEN];
	/*
	 *	legacy ,now we should get information from the above variable
	 *  here is just for compatible!
	 */
	float exposureTime;
	float apertureFNumber;
	SP_CHAR softName[EXIF_SOFT_NAME_LEN];
	SINT32 mainOrientOffset;
	SINT32 thumbOrientOffset;
} spExif_t;

/**
 * This structure stores user extension to the image.
 */
typedef struct spExtension_s {
	UINT32 signature;
	UINT8 style; /* CDSP styles, such as IMAGE_CDSP_BLACKWHITE, IMAGE_CDSP_SEPIA and etc. */
	UINT8 brightness; /* Parameter to CDSP's IMAGE_CDSP_BRIGHT. */
	UINT8 contrast; /* Parameter to CDSP's IMAGE_CDSP_CONTRAST. */
	UINT8 rotation; /* Rotation, such as IMAGE_CLOCKWISE_0, IMAGE_CLOCKWISE_90 and etc. */
} spExtension_t;

/**
 * This structure stores reader operations interface.
 */
typedef struct spImageReaderOp_s {
	UINT32 (*destroy)(HANDLE hreader);

	SP_CHAR *(*getFilename)(HANDLE hreader);
	UINT32 (*getType)(HANDLE hreader);
	UINT32 (*getSize)(HANDLE hreader, spRectSize_t *psize);
	UINT32 (*getOrientation)(HANDLE hreader, UINT8 *porientation);
	UINT32 (*getThumbnailOrientation)(HANDLE hreader, UINT8 *porientation);
	UINT32 (*getExif)(HANDLE hreader, spExif_t *pexif);
	UINT32 (*getUserExtension)(HANDLE hreader, spExtension_t *pextension);

	UINT32 (*setRotation)(HANDLE hreader, UINT8 rotate);
	UINT32 (*setFlip)(HANDLE hreader, UINT8 flip);
	UINT32 (*setSrcROI)(HANDLE hreader, spRect_t roi);
	UINT32 (*setDstROI)(HANDLE hreader, spRect_t roi);

	UINT32 (*load)(HANDLE hreader, spBitmap_t *pbitmap);
	UINT32 (*loadCancel)(HANDLE hreader);

	SP_BOOL (*containsThumbnail)(HANDLE hreader);
	UINT32 (*getThumbnailSize)(HANDLE hreader, spRectSize_t *psize);
	UINT32 (*loadThumbnail)(HANDLE hreader, spBitmap_t *pbitmap);

	UINT32 (*setScaleBuf)(HANDLE hreader, UINT8 *buf, UINT32 size);
} spImageReaderOp_t;

/**
 * Base Data structure for image reader
 */
typedef struct spImageReaderBase_s {
	spImageReaderOp_t *op;
} spImageReaderBase_t;


#if 0
	HANDLE (*create)(SP_CHAR *pfilename);
	HANDLE (*createFromStream)(HANDLE hStream);

	UINT32 (*loadAsync)(HANDLE hreader, spBitmap_t *pbitmap, (*Callback_t callback);
	SP_BOOL (*isAsyncFinished)(HANDLE hreader);
	UINT32 (*getAsyncResult)(HANDLE hreader);

	UINT32 (*getMetadata)(HANDLE hreader, HANDLE *pmetadata);
	UINT32 (*getMcuSize)(HANDLE hreader, spRectSize_t *psize);
	UINT32 (*getThumbnailMcuSize)(HANDLE hreader, spRectSize_t *psize);

	UINT32 (*scale)(spBitmap_t *psrcbitmap, spBitmap_t *pdstbitmap);
	UINT32 (*convert)(spBitmap_t *pbitmap, UINT32 format);
	UINT32 (*freeScale)(spBitmap_t *psrcbitmap, spBitmap_t *pdstbitmap, UINT8 rot, UINT8 flip, UINT8 type);
#endif

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * \brief Create a new image reader.
 * \param[in] pfilename Null-terminated string containing the filename to the image.
 * \return Handle to the image reader if success. NULL if failed.
 * \sa imageReaderDestroy(), imageReaderLoad()
 *
 * Caller is responsible of destroying the image reader via imageReaderDestroy().
 * The \a pfilename must contain the absolute path, such as <tt>D:\\PATH\\TO\\IMAGE.JPG</tt>
 */
HANDLE imageReaderCreate(SP_CHAR *pfilename);


UINT32 imageReaderInit(void);
/**
 * \brief Create a new image reader.
 * \param[in] type of image and the address to bts and the length of bts in memory 
 * \return Handle to the image reader if success. NULL if failed.
 * \sa imageReaderDestroy(), imageReaderLoad()
 *
 * Caller is responsible of destroying the image reader via imageReaderDestroy().
 * The \a pfilename must contain the absolute path, such as <tt>D:\\PATH\\TO\\IMAGE.JPG</tt>
 */
HANDLE imageReaderCreateFromBTS(UINT32 image_type,SP_CHAR * bts,UINT32 btslen);

/**
 * \brief Create a new image reader from stream.
 * \param[in] hStream Stream containing the image data.
 * \return Handle to the image reader if success. NULL if failed.
 * \sa imageReaderDestroy(), imageReaderLoad()
 *
 * Caller is responsible of destroying the image reader via imageReaderDestroy().
 * The \a pfilename must contain the absolute path, such as <tt>D:\\PATH\\TO\\IMAGE.JPG</tt>
 */
HANDLE imageReaderCreateFromStream(HANDLE hStream);

/**
 * \brief Destroy the image reader.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \return SP_OK if success. Otherwise fail.
 * \sa imageReaderCreate()
 *
 * Must invoke this function once the image reader is no longer needed.
 */
UINT32 imageReaderDestroy(HANDLE hreader);

/**
 * \brief Retrieves the filename of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \return Filename.
 *
 * \note The size retrieved is the size of the original image, not the size of the image decoded.
 */
SP_CHAR *imageReaderGetFilename(HANDLE hreader);

/**
 * \brief Retrieves the type of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \return type
 */
UINT32 imageReaderGetType(HANDLE hreader);

/**
 * \brief Retrieves the original size of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] psize will contain the size of the image
 * \return SP_OK if success. Otherwise fail.
 *
 * \note The size retrieved is the size of the original image, not the size of the image decoded.
 */
UINT32 imageReaderGetSize(HANDLE hreader, spRectSize_t *psize);

/**
 * \brief Retrieves the orientation of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] porientation will contain the orientation of the image.
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderGetOrientation(HANDLE hreader, UINT8 *porientation);

/**
 * \brief Retrieves the orientation of the image thumbnail.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] porientation will contain the orientation of the image.
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderGetThumbnailOrientation(HANDLE hreader, UINT8 *porientation);

/**
 * \brief Retrieves the MCU size of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] psize will contain the MCU size of the image
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderGetMcuSize(HANDLE hreader, spRectSize_t *psize);

/**
 * \brief Retrieves the MCU size of the image thumbnail.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] psize will contain the MCU size of the image thumbnail
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderGetThumbnailMcuSize(HANDLE hreader, spRectSize_t *psize);

/**
 * \brief Retrieves the metadata of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] pmetadata will contain the metadata of the image.
 * \return SP_OK if success. Otherwise fail.
 *
 * \note This is currently not implemented yet.
 */
UINT32 imageReaderGetMetadata(HANDLE hreader, HANDLE *pmetadata);

/**
 * \brief Retrieves the EXIF of the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] pexif will contain the EXIF of the image.
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderGetExif(HANDLE hreader, spExif_t *pexif);

/**
 * \brief Specify the scaling type when decoding the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in] scale accepts IMAGE_ORIGINAL, IMAGE_FIT, IMAGE_FILL and IMAGE_STRETCH.
 * \return SP_OK if success. Otherwise fail.
 *
 * \par Example for each of the style:
 * \image html image_original.png "IMAGE_ORIGINAL"
 * \image html image_fit.png "IMAGE_FIT"
 * \image html image_fill.png "IMAGE_FILL"
 * \image html image_stretch.png "IMAGE_STRETCH"
 *
 * \note Default value is IMAGE_ORIGINAL.
 */
UINT32 imageReaderSetScale(HANDLE hreader, UINT8 scale);


UINT32 imageReaderSetScaleBuf(HANDLE hreader, UINT8 *buf, UINT32 size);


/**
 * \brief Specify the rotation when decoding the image.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in] rotate accepts IMAGE_CLOCKWISE_0, IMAGE_CLOCKWISE_90, IMAGE_CLOCKWISE_180 and IMAGE_CLOCKWISE_270.
 * \return SP_OK if success. Otherwise fail.
 *
 * \note Default value is IMAGE_CLOCKWISE_0.
 * \note Rotation will be applied before flip.
 */
UINT32 imageReaderSetRotation(HANDLE hreader, UINT8 rotate);

/**
 * \brief Specify whether to flip the image when decoding it.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in] flip Will only accept IMAGE_NONE, IMAGE_HORIZONTAL and IMAGE_VERTICAL.
 * \return SP_OK if success. Otherwise fail.
 *
 * \note Default value is IMAGE_NONE.
 * \note Rotation will be applied before flip.
 */
UINT32 imageReaderSetFlip(HANDLE hreader, UINT8 flip);

/**
 * \brief Specify the source ROI (region of interest).
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in] roi The ROI to specify.
 * \return SP_OK if success. Otherwise fail.
 *
 * This must be set before invoking imageReaderLoad().
 * The source ROI must be in the region of the original image size.
 * Image Reader will fail if source ROI is larger than the image size.
 */
UINT32 imageReaderSetSrcROI(HANDLE hreader, spRect_t roi);

/**
 * \brief Specify the destination ROI (region of interest).
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in] roi The ROI to specify.
 * \return SP_OK if success. Otherwise fail.
 *
 * This must be set before invoking imageReaderLoad().
 * The destination ROI must be in the region of the bitmap size.
 */
UINT32 imageReaderSetDstROI(HANDLE hreader, spRect_t roi);

/**
 * \brief Decode image to the specified bitmap
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in,out] pbitmap Pointer to the bitmap to store the decoded image.
 * \return SP_OK if success. Otherwise fail.
 *
 * This will block until the image is decoded.
 * Caller has to provide the memory for \a pbitmap.
 * Caller is also responsible for releasing \a pbitmap.
 */
UINT32 imageReaderLoad(HANDLE hreader, spBitmap_t *pbitmap);

/**
 * \brief Decode image to the specified bitmap
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in,out] pbitmap Pointer to the bitmap to store the decoded image.
 * \param[in] callback Function pointer to the callback function.
 * \param[in] userdata Pointer to the userdata. Will be one of the parameters in the callback.
 * \return SP_OK if success. Otherwise fail.
 *
 * This will return immediately.
 * The caller will be informed via \a callback whether the decoding is completed or failed.
 * Caller has to provide the memory for \a pbitmap.
 * Caller is also responsible for releasing \a pbitmap.
 */
UINT32 imageReaderLoadAsync(HANDLE hreader, spBitmap_t *pbitmap, imageReaderCallback_t callback, void *userdata);

/**
 * \brief Cancel previous asynchronous decode.
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \return SP_OK if success. Otherwise fail.
 *
 * This will block until the asynchronous decode is actually cancelled.
 */
UINT32 imageReaderLoadAsyncCancel(HANDLE hreader);

/**
 * \brief Decode thumbnail to the specified bitmap
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in,out] pbitmap Pointer to the bitmap to store the decoded thumbnail.
 * \param[in] callback Function pointer to the callback function.
 * \param[in] userdata Pointer to the userdata. Will be one of the parameters in the callback.
 * \return SP_OK if success. Otherwise fail.
 *
 * This will return immediately.
 * The caller will be informed via \a callback whether the decoding is completed or failed.
 * Caller has to provide the memory for \a pbitmap.
 * Caller is also responsible for releasing \a pbitmap.
 */
UINT32 imageReaderLoadThumbnailAsync(HANDLE hreader, spBitmap_t *pbitmap, imageReaderCallback_t callback, void *userdata);

/**
 * \brief Check if the async loading is finished
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 */
SP_BOOL imageReaderIsAsyncFinished(HANDLE hreader);

/**
 * \brief Get the async loading result code
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 */
UINT32 imageReaderGetAsyncResult(HANDLE hreader);

/**
 * \brief Cancel the image decode
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderLoadCancel(HANDLE hreader);

/**
 * \brief Determine whether the image contains thumbnail
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \return SP_TRUE if contains thumbnail. Otherwise SP_FALSE.
 */
SP_BOOL imageReaderContainsThumbnail(HANDLE hreader);

/**
 * \brief Determine whether the image contains thumbnail
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] psize will contain the size of the thumbnail.
 * \return SP_OK if success. Otherwise fail.
 */
UINT32 imageReaderGetThumbnailSize(HANDLE hreader, spRectSize_t *psize);

/**
 * \brief Determine whether the image contains thumbnail
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[in,out] pbitmap Pointer to the bitmap to store the thumbnail.
 * \return SP_OK if success. Otherwise fail.
 *
 * Caller has to provide the memory for \a pbitmap.
 * Caller is also responsible for releasing \a pbitmap.
 */
UINT32 imageReaderLoadThumbnail(HANDLE hreader, spBitmap_t *pbitmap);

UINT32 imageReaderScale(spBitmap_t *psrcbitmap, spBitmap_t *pdstbitmap);
UINT32 imageReaderConvert(spBitmap_t *pbitmap, UINT32 format, SP_BOOL bDithering);
UINT32 imageReaderFreeScale(spBitmap_t *psrcbitmap, spBitmap_t *pdstbitmap, UINT8 rot, UINT8 flip, UINT32 type);

/**
 * \brief Retrieves the user extension to the image
 * \param[in] hreader Pointer to the image reader created by imageReaderCreate().
 * \param[out] pextension Pointer to the spExtension_t.
 * \return SP_OK if success.
 *         SP_ERR_NOT_FOUND if there are no existing user extension. pextension will contain default value.
 *         Otherwise fail.
 */
UINT32 imageReaderGetUserExtension(HANDLE hreader, spExtension_t *pextension);

spRgbQuad_t MAKE_RGB( UINT8 red, UINT8 green, UINT8 blue );
spRgbQuad_t MAKE_ARGB( UINT8 alpha, UINT8 red, UINT8 green, UINT8 blue );

/**
 * @brief
 * @param size
 * @param type Can be any of the following: SP_BITMAP_YUV422, SP_BITMAP_RGB565, SP_BITMAP_8BPP and SP_BITMAP_1BPP.
 * @retval
 */
spBitmap_t *bitmapCreate(spRectSize_t size, UINT8 type);

/**
 * @brief
 * @param width
 * @param height
 * @param type Can be any of the following: SP_BITMAP_YUV422, SP_BITMAP_RGB565, SP_BITMAP_8BPP and SP_BITMAP_1BPP.
 * @retval
 */
spBitmap_t *bitmapCreateHelper(SINT16 width, SINT16 height, UINT8 type);

/**
 * @brief
 * @param size
 * @param type Can be any of the following: SP_BITMAP_YUV422, SP_BITMAP_RGB565, SP_BITMAP_8BPP and SP_BITMAP_1BPP.
 * @param bgcolor Default background color for the newly created bitmap.
 * @retval
 */
spBitmap_t *bitmapCreateEx(spRectSize_t size, UINT8 type, spRgbQuad_t bgcolor);

/**
 * @brief
 * @param pbitmap
 * @retval
 */
UINT32 bitmapDestroy(spBitmap_t *pbitmap);

/**
 * @brief
 * @param pbitmap
 * @param size
 * @param type Can be any of the following: SP_BITMAP_YUV422, SP_BITMAP_RGB565, SP_BITMAP_8BPP and SP_BITMAP_1BPP.
 * @param pData
 * @param pPalette
 * @retval
 */
UINT32 bitmapInit(spBitmap_t *pBitmap, spRectSize_t size, UINT8 type, UINT8 *pData, spRgbQuad_t *pPalette);

UINT32 imageReaderUnInit(void);
#endif
