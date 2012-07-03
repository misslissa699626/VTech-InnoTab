#ifndef _CSI_DRIVER_H_
#define _CSI_DRIVER_H_

/* error message */
#define ERR_OPEN_FAIL		-1
#define ERR_DEVICE_FAIL		-2
#define ERR_FMT_FAIL		-3
#define ERR_BUFFER_FAIL		-4
#define ERR_START_FAIL		-5
#define ERR_CTRL_FAIL		-6

/*!
	\enum CsiFlag
	\brief specify which fields in CsiConfig are going to be read/ written
*/
typedef enum {
	CSI_ALL_PARAM = 0xFFFFFFFF, /*!< all fileds in CsiConfig */
	CSI_IMAGE_WIDTH = 0x00000001, /*!< image width */
	CSI_IMAGE_HEIGHT = 0x00000002, /*!< image height */
	CSI_IMAGE_BUFFER_0 = 0x00000004, /*!< image buffer 0 */
	CSI_IMAGE_BUFFER_1 = 0x00000008, /*!< image buffer 1 */
	CSI_IMAGE_BUFFER_2 = 0x00000010, /*!< image buffer 2 */
	CSI_IMAGE_BUFFER_3 = 0x00000020, /*!< image buffer 3 */
	// ¡K other CMOSE sensor parameters
	CSI_CUBIC_DISABLE = 0x00000040, /*!< disable cubic mode */
	CSI_CUBIC_32X32 = 0x00000080, /*!< 32x32 cubic mode */
	CSI_CUBIC_64X64 = 0x00000100, /*!< 64x64 cubic mode */
	CSI_NIGHT_MODE_EN = 0x00000200,
	CSI_NIGHT_MODE_DIS = 0x00000400,
	CSI_POWER_LINE_DIS = 0x00000800,
	CSI_POWER_LINE_50HZ = 0x00001000,
	CSI_POWER_LINE_60HZ = 0x00002000,
} CsiFlag;

/*!
	\brief data struct used to get/ set CMOS sensor parameters
*/
typedef struct {
	/*! \brief flag indicates which fields in the structure is going to be read/ written */
	unsigned int mFlag;
	/*! \brief image width */
	unsigned int mImageWidth;
	/*! \brief image height */
	unsigned int mImageHeight;
	/*! \brief image buffer 0 - 3 */
	unsigned char* mImageBuffer[4];
	// ¡K other CMOS sensor parameters
} CsiConfig;

 
/*!
	\brief data struct used to capture CMOS sensor image
*/
typedef struct {
	/*! \brief image width */
	unsigned int mImageWidth;
	/*! \brief image height */
	unsigned int mImageHeight;
	/*! \brief image buffer for capture */
	unsigned char* mImageBuffer;
	/*! \brief wait csi frame */
	unsigned int mWaitFrame;
	// ¡K other CMOS sensor capture parameters
} CsiCaptureConfig;

/*!
	\fn int CsiDrv_SetDevice(const CsiConfig* config)
	\brief initialize CSI and CMOS sensor
	\param config initial configuration of CSI and CMOS sensor
	\return 0 if initialization succeed or otherwise -1
*/
int CsiDrv_SetDevice(char *DeviceName, unsigned int inFormat);

/*!
	\fn int CsiDrv_init(const CsiConfig* config)
	\brief initialize CSI and CMOS sensor
	\param config initial configuration of CSI and CMOS sensor
	\return 0 if initialization succeed or otherwise -1
*/
int CsiDrv_init(CsiConfig* config);

/*!
	\fn void CsiDrv_cleanup()
	\brief cleanup and stop CSI and CMOS sensor
*/
void CsiDrv_cleanup();

/*!
	\fn int CsiDrv_getParameters(CsiConfig* config)
	\brief get CMOS sensor parameters, the fields mFlag in CsiConfig is used to specify which parameter is going to retrieve 
	\param config output parameter, store specified CMOS sensor parameters at the end of operation
	\return 0 if operation succeed or otherwise -1
*/
int CsiDrv_getParameters(CsiConfig* config);

/*!
	\fn int CsiDrv_setParameters(const CsiConfig* config)
	\brief set CMOS sensor parameter, the fields mFlag in CsiConfig is used to specify which parameter is going to modify
	\param config update CMOS sensor parameters
	\return 0 if operation succeed or otherwise -1
*/
int CsiDrv_setParameters(CsiConfig* config);

/*!
	\fn unsigned char* CsiDrv_lockDisplayBuffer()
	\brief lock current display buffer of CMOS sensor
	\return address of locked display buffer (> 0) if operation succeed or otherwise null pointer (0)
*/
unsigned char* CsiDrv_lockDisplayBuffer();

/*!
	\fn void CsiDrv_unlockDisplayBuffer(unsigned char* ptr)
	\brief unlock previously locked CMOS sensor display buffer
	\param ptr pointer to previously locked CMOS sensor display buffer
*/
void CsiDrv_unlockDisplayBuffer(unsigned char* ptr);

/*!
	\fn int CsiDrv_captureImage(CsiConfig* capture_config)
	\brief capture image with specified parameter
	\param capture_config CMOS sensor parameters used to capture the image
	\return 0 if operation succeed or otherwise -1
*/
int CsiDrv_captureImage(CsiCaptureConfig* capture_config); 

/*!
	\fn void CsiDrv_onVBlkIrq()
	\brief interrupt handler when CMOS sensor vertical blank interrupt occur
*/
void CsiDrv_onVBlkIrq();


#endif
