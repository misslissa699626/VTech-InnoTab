/*!
	\file CSIDrv.h
	\brief file contains API to access/ control CSI and CMOS sensor
*/

#ifdef __cplusplus
extern "C" {
#endif

#define CSI_BUF_NUM 3


/*!
	\enum CsiFlag
	\brief specify which fields in CsiConfig are going to be read/ written
*/
typedef enum {
	CSI_ALL_PARAM = 		0xFFFFFFFF, 		/*!< all fileds in CsiConfig */
	CSI_IMAGE_WIDTH = 		0x00000001, 		/*!< image width */
	CSI_IMAGE_HEIGHT = 		0x00000002, 		/*!< image height */
	CSI_IMAGE_BUFFER_0 = 	0x00000004, 		/*!< image buffer 0 */
	CSI_IMAGE_BUFFER_1 = 	0x00000008, 		/*!< image buffer 1 */
	CSI_IMAGE_BUFFER_2 = 	0x00000010, 		/*!< image buffer 2 */
	CSI_IMAGE_BUFFER_3 = 	0x00000020, 		/*!< image buffer 3 */
	CSI_IMAGE_CONTRAST = 	0x00000040,			//Contrast
	CSI_IMAGE_BRIGHTNESS = 	0x00000080,			//Brightness
	CSI_IMAGE_SATURATION = 	0x00000100,			//Saturation
	CSI_IMAGE_SHARPNESS = 	0x00000200,			//Sharpness
	CSI_IMAGE_WHITEBALANCE= 0x00000400,			//White balance 
	CSI_IMAGE_50HZ = 		0x00000800,			//50HZ flicker quantization 
	CSI_IMAGE_60HZ = 		0x00001000,			//60HZ flicker quantization
	CSI_IMAGE_MIRROR = 		0x00002000,			//mirror
	CSI_IMAGE_FLIP = 		0x00004000,			//flip
	CSI_IMAGE_DEFAULT = 	0x00008000,			//default
	CSI_CUBIC_DISABLE = 	0x00010000, 		/*!< disable cubic mode */
	CSI_CUBIC_32X32 = 		0x00020000, 		/*!< 32x32 cubic mode */
	CSI_CUBIC_64X64 = 		0x00040000, 		/*!< 64x64 cubic mode */	
	// … other CMOSE sensor parameters
} CsiFlag;

/*!
	\brief data struct used to get/ set CMOS sensor parameters
*/
typedef struct {
	/*! \brief flag indicates which fields in the structure is going to be read/ written */
	unsigned mFlag;
	/*! \brief image width */
	unsigned mImageWidth;
	/*! \brief image height */
	unsigned mImageHeight;
	/*! \brief image buffer 0 - 3 */
	unsigned char* mImageBuffer[CSI_BUF_NUM];
	// … other CMOS sensor parameters
	unsigned Contrast;
	unsigned Brightness;
	unsigned Saturation;
	unsigned Sharpness;
	unsigned WhiteBalance;
	
} CsiConfig;

/*!
	\brief data struct used to capture CMOS sensor image
*/
typedef struct {
	/*! \brief image width */
	unsigned mImageWidth;
	/*! \brief image height */
	unsigned mImageHeight;
	/*! \brief image buffer for capture */
	unsigned char* mImageBuffer;
	// … other CMOS sensor capture parameters
} CsiCaptureConfig;

/*!
	\fn int CsiDrv_init(const CsiConfig* config)
	\brief initialize CSI and CMOS sensor
	\param config initial configuration of CSI and CMOS sensor
	\return 0 if initialization succeed or otherwise -1
*/
int CsiDrv_init(const CsiConfig* config);

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
int CsiDrv_setParameters(const CsiConfig* config);

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

/*!
	\fn int CsiDrv_enableVideoStreamOutput(int handle)
	\brief enable video stream output from CSI
	\parma handle handle obtained from vs_open(VideoStreamMode)
	\return 0 if operation succeed or otherwise -1
*/
int CsiDrv_enableVideoStreamOutput(int handle);

/*!
	\fn void CsiDrv_disableVideoStreamOutput()
	\brief disable video stream output from CSI
*/
void CsiDrv_disableVideoStreamOutput();


#ifdef __cplusplus
}
#endif








