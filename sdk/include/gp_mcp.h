
/***************************************************************************
 * Name: gp_mcp.h
 *
 * Purpose:
 *
 * Developer:
 *     zhoulu, 2010-8-18
 *
 * Copyright (c) 2010-2011 by Sunplus mMobile Inc.
 ***************************************************************************/

#ifndef _GP_MCP_H_
#define _GP_MCP_H_

/***************************************************************************
 * Header Files
 ***************************************************************************/
#include <typedef.h>
#include "gp_demux.h"
#include "gp_mcpvo.h"
#include "gp_mcpao.h"

/***************************************************************************
 * Constants
 ***************************************************************************/
/** @brief MCPlayer open type */
#define MCP_OPEN_TYPE_INFO            0
#define MCP_OPEN_TYPE_RUN             1
#define MCP_OPEN_TYPE_PAUSE           2

/** @brief MCPlayer contain id */
#define MCP_CONTAIN_ID_DEFAULT        (-1) // other is gpCsType_t that defined in gp_demux.h 

/** @brief MCPlayer thumbnail */
#define MCP_THUMBNAIL_TIME_DEFAULT    (-1)

/** @brief MCPlayer bookmark */
#define MCP_BOOKMARK_VALUE_DEFAULT    -1
#define MCP_BOOKMARK_MODE_MS          0
#define MCP_BOOKMARK_MODE_BYTE        1
#define MCP_BOOKMARK_AOLUME_DEFAULT   (-1)

/** @brief MCPlayer element stream play index */
#define MCP_ES_INDEX_DEFAULT          (-1)

/** @brief MCPlayer video out type */
#define MCP_VOUT_TYPE_DEFAULT         VO_TYPE_DISP0 /** @brief MCPlayer video out to default device */
#define MCP_VOUT_TYPE_DISP0           VO_TYPE_DISP0 /** @brief MCPlayer video out to disp0 device */
#define MCP_VOUT_TYPE_DISP1           VO_TYPE_DISP1 /** @brief MCPlayer video out to disp1 device */
#define MCP_VOUT_TYPE_DISP2           VO_TYPE_DISP2 /** @brief MCPlayer video out to disp2 device */
#define MCP_VOUT_TYPE_TV0             VO_TYPE_TV0   /** @brief MCPlayer video out to disp0 device */
#define MCP_VOUT_TYPE_TV1             VO_TYPE_TV1   /** @brief MCPlayer video out to disp0 device */

/** @brief MCPlayer audio out type */
#define MCP_AOUT_TYPE_DEFAULT         AO_TYPE_OSS   /** @brief MCPlayer audio out to default device */
#define MCP_AOUT_TYPE_OSS             AO_TYPE_OSS   /** @brief MCPlayer audio out to OSS device */
#define MCP_AOUT_TYPE_MIXER           AO_TYPE_MIXER /** @brief MCPlayer audio out to mixer library device */

/** @brief MCPlayer display mode  while out mode is screen*/
#define MCP_DISP_MODE_SLAVE           0
#define MCP_DISP_MODE_MASTER          1

/** @brief MCPlayer display layer while out mode is screen */
#define MCP_DISP_LAYER_PRIMARY        0
#define MCP_DISP_LAYER_OSD0           1
#define MCP_DISP_LAYER_OSD1           2

/** @brief MCPlayer video out */
#define MCP_VIDEO_OUT_DEFAULT         (NULL)
#define MCP_AUDIO_OUT_DEFAULT         (NULL)

/** @brief MCPlayer aspect type */
#define MCP_ASPECT_TYPE_FULL          0
#define MCP_ASPECT_TYPE_KEEP          1

/** @brief MCPlayer rotate direction type */
#define MCP_ROTATE_DIRECTION_NO       0
#define MCP_ROTATE_DIRECTION_LEFT     90
#define MCP_ROTATE_DIRECTION_DOWN     180
#define MCP_ROTATE_DIRECTION_RIGHT    270

/** @brief subtitle type */
#define MCP_SUBTITAL_TYPE_TXT         (1<<0)
#define MCP_SUBTITAL_TYPE_PIC         (1<<1)

/** @brief media run status */
#define MEDIA_PLAY_STATE_STOP         (0)
#define MEDIA_PLAY_STATE_RUN          (1)
#define MEDIA_PLAY_STATE_PAUSE        (2)
#define MEDIA_PLAY_STATE_BUFFER       (3)
#define MEDIA_PLAY_STATE_SLOW         (4)
#define MEDIA_PLAY_STATE_SEEK         (5)
#define MEDIA_PLAY_STATE_READY        (6)
#define MEDIA_PLAY_STATE_END          (7)

/** @brief frame phases of renderFrame callback  in mcpMediaInfo_t */
#define MEDIA_FRAME_PHASE_DEC         (0) /* frame phase  is after decode */
#define MEDIA_FRAME_PHASE_SCALE       (1) /* frame phase is after scale */
#define MEDIA_FRAME_PHASE_DISP        (2) /* frame phase is at display time */

/** @brief return value of renderFrame callback in mcpMediaInfo_t */
#define MEDIA_RENDER_RET_CONTINUE     (0) /* let player to continue after render */
#define MEDIA_RENDER_RET_DROP         (1) /* let player to drop this frame after render */

#define LONG_FILENAME_LEN             512 /*for video record file name*/
/***************************************************************************
 * Macros
 ***************************************************************************/

/***************************************************************************
 * Data Types
 ***************************************************************************/

	/** @brief MCPlayer message */
	typedef enum {
		MCP_MSG_ACITION_UPDATE = 1,  /*!> the progress of done function, data is percent*/
		MCP_MSG_TIME_UPDATE,         /*!> play time update, data is ms */
		MCP_MSG_BUFFER_UPDATE,       /*!> the progress in buffer state, data is percent */
		MCP_MSG_SUBTITLE_UPDATE,     /*!> the subtitle of display, data is mcpSubtitle_t pointer */
		MCP_MSG_NEW_ES,              /*!> find new element stream, data is type */
		MCP_MSG_PLAY_FATAL,          /*!> fatal errror, can't continue play */
		MCP_MSG_PLAY_END             /*!> play to end  */
	}mcpMsg_e;

	/** @brief MCPlayer return code */
	typedef enum {
		MCP_RET_OK           = 0,    /*!> success */
		MCP_RET_FAIL         = -1,   /*!> failed */
		MCP_RET_NOPEN        = -2,   /*!> don't open */
		MCP_RET_NOCLOSE       = -3,   /*!> don't close */
		MCP_RET_NOTCONTINUE  = -4,   /*!> fatal errror, can't continue play*/
		MCP_RET_NOTSUPPORT   = -5,   /*!> not supportable contain or element stream  */
		MCP_RET_NOPLAYES     = -6,	 /*!> no elemet stream in program */
		MCP_RET_OUTRANGE     = -7,   /*!> out range  */
		MCP_RET_EOF          = -8,	 /*!> play to end  */
		MCP_RET_HWERR 		 = -9	 /*!> hardware platform error  */
	}mcpRet_e;

	typedef video_info_t videoInfo_t;
	typedef audio_info_t audioInfo_t;
	typedef sub_info_t subInfo_t;
	
	/** @brief MCPlayer thumbnail data type */
	typedef struct mcpThumbnail_s
	{
		UINT32 time;	        /*!< [in/out] ms, -1: no thumbnail */
		gp_bitmap_t image;      /*!< [in/out] user malloc buffer*/
	}mcpThumbnail_t;

	/** @brief MCPlayer bookmark data type */
	typedef struct mcpMark_s
	{
		SINT64 value;          /*!< [in/out] -1: no mark for openState is 1, 2, no thumbnail for openState is 0 */
		UINT32 mode;           /*!< [in/out] 0: time, unit in ms, 1:offset, unit in byte. */
		UINT32 volume;         /*!< [in/out] 0~100, -1: default volume  */
		SINT32 vidIdx;         /*!< [in/out] -1: default video stream */
		SINT32 audIdx;         /*!< [in/out] -1: default audio stream */
		SINT32 subIdx;         /*!< [in/out] -1: default subtitle stream */
	}mcpMark_t;

	typedef struct mcpVidOut_s
	{
		UINT32 mode;           /*!< [in] 0:out ot screen, 1:out to image, 2:out to share memory */
		UINT32 aspect;         /*!< [in] 0:full, 1:keep aspect */
		UINT32 rotateAngle;    /*!< [in] 0, 90, 180,270 */
		gp_bitmap_t outMap;    /*!< [in] video output window map, struct define reference typedef.h 
								*	if outMap.pData is null, mcplayer output video to screen  */
		/* out information for display to screen */
		UINT32 dispMode;	   /*!< [in] 0: slave, 1:  master*/
		UINT32 dispLayer;	   /*!< [in] 0: primary layer, 1: osd0 layer, 2: osd1 layer */
		UINT32 dispFrames;	   /*!< [in] display frame buffer count, minimum is 3 */
		
		/* out information for output to share memory */
		UINT32 lock;		   /*!< 1: lock, 0: unlock */
		UINT32 update;		   /*!< 1: update, 0: no update */
		UINT32 updateIdx;	   /*!< update index while frame count more than one */
		UINT32 count;		   /*!< frame count */
		UINT32 frameSize;	   /*!< the bytes of frame */
		UINT32 shareMemId;     /*!< [in] share buffer id, 0: don't use share buffer */

		UINT32 decryptType;    /*!< [in] video es decryption type, 0: no seting */
		UINT8 *decryptKey;    /*!< [in] video es decryption key */
	} mcpVidOut_t;
	
	typedef struct mcpAudOut_s
	{
		UINT32 type;           /*!< [in] audio out type */
		UINT32 speed;           /*!< [in] audio out type */
		
	} mcpAudOut_t;
	
	typedef struct mcpSubOut_s
	{
		SINT8 *language;           /*!< [in] subtitle language */
		
	} mcpSubOut_t;

	
	typedef void (*mcpCallMsg_t)(UINT32 handle, mcpMsg_e message, UINT32 data);
	typedef UINT32 (*mcpCallRender_t)(gp_bitmap_t *frame, UINT32 pts /* ms */, UINT32 phase);
	
	/** @brief MCPlayer boot data type */
	typedef struct mcpMediaInfo_s 
	{
		SINT8 *filename;       /*!< [in] */
		UINT32 containerId;    /*!< [in/out] id for identify container, -1: unkown container; */
		UINT32 openType;	   /*!< [in] 0: get info. only, 1: get info. and play, 2:get info. and pause */
		/* media information */
		videoInfo_t vi;        /*!< [out] video information*/
		audioInfo_t ai;        /*!< [out] audio information */
		subInfo_t si;          /*!< [out] subtitle information */

		/* will be conceled thumbnail, it's time replace with bookmark->value, it's image replace with vidOut->outMap */
		//mcpThumbnail_t thumbnail; /*!< [in/out] */
		
		/* video and audio out parameter */
		mcpMark_t bookmark;    /*!< [in] */
		mcpVidOut_t vidOut;    /*!< [in] for video output*/
		mcpAudOut_t audOut; /*!< [in] for audio output*/
		mcpSubOut_t subOut;
		mcpCallMsg_t sendMsg; /*!< [in] if null, don't send msg */
		mcpCallRender_t renderFrame; /*!< [in] if null, don't send video frame */

		SINT8 *opt[];
	} mcpMediaInfo_t;

	/** @brief MCPlayer playing status data type */
	typedef struct mcpStatus_s 
	{
		UINT32 state;		   /*!< [out] running, pause, seek, slow, buffer, stopping */
		UINT32 time;		   /*!< [out] current play time unit in ms */
		UINT32 vidCnt;	       /*!< [out] count of video element stream */
		UINT32 vidIdx;	       /*!< [in/out] index of playing video, 0~(vidCnt-1)*/
		UINT32 audCnt;	       /*!< [out] count of audio element stream */
		UINT32 audIdx;	       /*!< [in/out] index of playing audio, 0~(audCnt-1)*/
		UINT32 subCnt;	       /*!< [out] count of subtitle element stream */
		UINT32 subIdx;	       /*!< [in/out] index of playing subtitle 0~(subCnt-1)*/
		UINT32 progress;	   /*!< [out] progress of filled data if state is buffer, 0~100*/
		UINT32 volume;		   /*!< [in/out] 0~100*/
		UINT32 bufferTime;     /*!< [out] buffer time of es data */
		mcpVidOut_t *vidOut;   /*!< [in/out] video output map*/
		/*mcpAudOut_t *audOut;*/   /*!< [in/out] audio output map*/
		videoInfo_t *vi;	   /*!< [out] information of playing video */
		audioInfo_t *ai;	   /*!< [out] information of playing audio */
		subInfo_t *si;		   /*!< [out] information of playing subtitle */
	} mcpStatus_t;

	typedef struct mcpSubtitle_s
	{
		UINT32 subType;        /*!< subtitle type */
		UINT32 colorType;      /*!< reference bitmap */
		UINT32 width;          /*!< for picture */
		UINT32 height;         /*!< for picture */
		UINT32 size;           /*!< subtitle data size */
		UINT8 *data;           /*!< subtitle data */
	}mcpSubtitle_t;

/***************************************************************************
 * Inline Function Definitions
 ***************************************************************************/

	
/***************************************************************************
 * Global Data
  ***************************************************************************/


/***************************************************************************
 * Function Declarations
 ***************************************************************************/
#ifdef cplusplus
	extern "C" {
#endif

	/** @brief MCPlayer Set/Get Parameter type for mcpSet/mcpGet function*/
	typedef enum mcpParam_e{
		MCP_PARAM_NO,
	}mcpParam_t;
	
	SINT32 mcpOpen(mcpMediaInfo_t *mi); 
	SINT32 mcpClose(mcpMark_t *bookmark);
	SINT32 mcpRun(void);
	SINT32 mcpPause(void);
	SINT32 mcpSeek(UINT32 ms);
	SINT32 mcpSlow(UINT32 speed); 
	SINT32 mcpGetStatus(mcpStatus_t *status); 
	SINT32 mcpSetStatus(mcpStatus_t *status);
	SINT32 mcpSet(mcpParam_t param, UINT32 value);
	SINT32 mcpGet(mcpParam_t param, UINT32 *value);
	SINT8* mcpVersion(void); 



#ifdef cplusplus
}
#endif



#if 1 // TODO: MCplayer New Version library interface
	/** @brief MCPlayer Set/Get Parameter type for mcpSet/mcpGet function*/
	typedef enum mcpParamType_e{
		MCP_PARAM_VERSION = 0,		  /*!> get: version sting */
		MCP_PARAM_CONTAINER_TYPE,	  /*!> set/get: the format of container */
		MCP_PARAM_CODEC_TYPE,		  /*!> set/get:: the format of video codec*/
		MCP_PARAM_URL,				  /*!> set: the uniform resource locator of movie*/
		MCP_PARAM_TIMESTAMP,		  /*!> set: the timestamp of start process site */
		MCP_PARAM_DURATION, 		  /*!> get: the duration of movie */
		MCP_PARAM_SEEK_MODE,		  /*!> get: the seek mode of movie */
		MCP_PARAM_PLAY_TIME,		  /*!> get: the play time of movie */
		MCP_PARAM_PLAY_STATE,		  /*!> get: the play state of movie */
		MCP_PARAM_PLAY_MSG, 		  /*!> set: the play callback message */
		MCP_PARAM_PLAY_RENDER,		  /*!> set: the play callback render frame */
		MCP_PARAM_CLIENT_ID,		  /*!> set: the client id */
	
		MCP_PARAM_VID = 100,		  /*!> video parameter */
		MCP_PARAM_VID_INFO, 		  /*!> get: the information of video */
		MCP_PARAM_VID_DISABLE,		  /*!> set: disable video play */
		MCP_PARAM_VID_OUT,			  /*!> set: the device of video out */
		MCP_PARAM_VID_MODE, 		  /*!> set: the mode of video out */
		MCP_PARAM_VID_LAYER,		  /*!> set: the display layer of video */
		MCP_PARAM_VID_COLOR,		  /*!> set: the display color of video */
		MCP_PARAM_VID_WIDTH,		  /*!> set: the display width of video */
		MCP_PARAM_VID_HEIGHT,		  /*!> set: the display height of video */
		MCP_PARAM_VID_RECT, 		  /*!> set: the display rectangle of video */
		MCP_PARAM_VID_WINDOW,		  /*!> set: the display window of video */
		MCP_PARAM_VID_ASPECT,		  /*!> set: the display aspect of video */
		MCP_PARAM_VID_DECRYPT,        /*!> set: the decrpyt type of video es */
		MCP_PARAM_VID_DECRYPT_KEY,	  /*!> set: the decrpyt key of video es */
		MCP_PARAM_VID_EXACT_SEEK,	  /*!> set: set video exact seek */
		MCP_PARAM_VID_CANCEL_SEEK,	  /*!> set: cancel video exact seek */
		
		MCP_PARAM_AUD = 200,		  /*!> audio parameter */
		MCP_PARAM_AUD_INFO, 		  /*!> get the information of audio */
		MCP_PARAM_AUD_DISABLE,		  /*!> set: disable audio play */
		MCP_PARAM_AUD_OUT,			  /*!> set: the device of audio out */
		MCP_PARAM_AUD_COUNT,		  /*!> get: the count of audio */
		MCP_PARAM_AUD_INDEX,		  /*!> get/set: the index of play audio */
		MCP_PARAM_AUD_VOLUME,		  /*!> get/set: the volume of audio*/
		MCP_PARAM_AUD_SPEED,		  /*!> get/set: the speed of audio*/
	
		MCP_PARAM_SUB = 300,		  /*!> subtitle parameter */
		MCP_PARAM_SUB_INFO, 		  /*!> get the information of subtitle */
		MCP_PARAM_SUB_DISABLE,		  /*!> set: disable subtitle play */
		MCP_PARAM_SUB_COUNT,		  /*!> get: the count of subtitle */
		MCP_PARAM_SUB_INDEX,		  /*!> get/set: the index of play subtitle */
		MCP_PARAM_SUB_LANGUAGE,       /*!> get/set: subtitle language */
	
		MCP_PARAM_DEBUG = 10000,	  /*!> follow parameters for debug */
		MCP_PARAM_END				  /*!> end */
	}mcpParamType_t;
	
	typedef struct mcpVidInfo_s
	{
		UINT32 type;
		UINT32 width;/*!> pixel */
		UINT32 height;/*!> pixel */
		UINT32 duration;/*!> ms */
	} mcpVidInfo_t;
	
	typedef struct mcpAudInfo_s
	{
		UINT32 type;
		UINT32 samplerate;/*!> Hz*/
		UINT32 channels;
		UINT32 duration;/*!> ms */
	} mcpAudInfo_t;
	
	typedef struct mcpSubInfo_s
	{
		UINT32 type;
		UINT32 isPicture;
		SINT8 *language;
	} mcpSubInfo_t;
	
#ifdef cplusplus
extern "C" {
#endif

	void *mcpOpenEx(SINT8 *type);
	SINT32 mcpCloseEx(void *hd);
	SINT32 mcpSetEx(void *hd, mcpParamType_t param, UINT32 value);
	SINT32 mcpGetEx(void *hd, mcpParamType_t param, UINT32 *value);
	SINT32 mcpThumbnailEx(void *hd, gp_bitmap_t *bitmap); 
	SINT32 mcpPlayEx(void *hd);
	SINT32 mcpPauseEx(void *hd);
	SINT32 mcpResumeEx(void *hd);
	SINT32 mcpSeekEx(void *hd, UINT32 ms);
	SINT32 mcpStopEx(void *hd);

	
#ifdef cplusplus
	}
#endif

#endif
	

#endif   /* _GP_MCP_H_  */


/***************************************************************************
 * Thegp_mcp.h file end
 ***************************************************************************/

