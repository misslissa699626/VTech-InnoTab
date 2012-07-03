/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2008 by Sunplus mMedia Inc.                      *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  mMedia Inc. All rights are reserved by Sunplus mMedia Inc.            *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus mMedia Inc.                                *
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
 * @file media_player.h
 * @brief The header file for developing the Media application based on MediaPlayer Module, User must include
 * this file to declare command packet structure ,  command ID group and message patterns of MediaPlayer.
 * @author chao.chen@sunmedia.com.cn
 */

#ifndef _MEDIA_PLAYER_H_
#define _MEDIA_PLAYER_H_

/* include the basic data type */
#include <stdint.h>
#include <mach/typedef.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup Media Player MSG/CMD Packet Implemention
 *  @{
 */
 
/** @brief Media Play Control Messages */
enum {
	PLAYER_MSG_MIN = 0x1FF, 
	#if 0	
	PLAYER_MSG_PLAY_OK,	 				/*!< @brief Play file success and start */
	#endif
	PLAYER_MSG_PLAY_FAIL,			 	/*!< @brief Play file fail */
	PLAYER_MSG_PLAY_TO_END,				/*!< @brief Play to end now */
	PLAYER_MSG_PLAY_ERROR,	 			/*!< @brief Play file data error */
	PLAYER_MSG_ELAPSED_TIME_UPDATE,	 	/*!< @brief Play elapsed time is updated now */
	PLAYER_MSG_SUBTITLE_UPDATE,         /*!< @brief Play subtitle is updated now */
	PLAYER_MSG_BUFFER_UPDATE,           /*!< @brief Player buffering status is updated now */
	PLAYER_MSG_ES_UPDATE,		        /*!< @brief element stream is updated now, ep. find new subtitle in playing */

	PLAYER_MSG_PULSE,						/*!< @brief msg player pulse,  this msg is send by main cycle of player proc, if not, the player proc is abnormal*/

	PLAYER_MSG_MAX						/*!< @brief msg max, to be expanded */
};


/** @brief Media Player Setup command  */ 
enum {
	PLAYER_SETUP_MIN = PLAYER_MSG_MAX + 1,
	PLAYER_SETUP_PLAY,	 				/*!< @brief Play a file, args: <filename> + <args list>, , output: 0:<success or errcode>, <mediaFileInfo_t>*/
	PLAYER_SETUP_PARSE,					/*!< @brief Parse a file,  args: <filename> + <args list>, 
	                                                                           timestamp = -1, output: <success or errcode>, <mediaFileInfo_t>
	                                                                           timestamp = other, output: <success or errcode>, <mediaFileInfo_t> &  <mediaThumbnail_t> & <bitmapData> */

											/*
											 *Setup Args list: 
											 *[-a] action -- "play", "parse"
											 *[-t] timestamp
											 *[-c] contain
											 *[-w] width
											 *[-h] height
											 *[-f] thumbnail format
											 *[-o] offset
											 *[-V] video index
											 *[-A] audio index
											 *[-S] subtitle index
											 *[-m] share memory id for video data output
 */


	PLAYER_SETUP_MAX					/*!< @brief Setup args max, to be expanded */
};


/** @brief Media Player running command ID */
enum {
	PLAYER_CMD_MIN = PLAYER_SETUP_MAX + 1,
	PLAYER_CMD_STOP,						/*!< @briefStop and exit from file playing, output: SUCCESS or Errcode, and bookmark(media_mark_t) */
	PLAYER_CMD_PAUSE,						/*!< @brief Playing pause, output: SUCCESS or Errcode */
	PLAYER_CMD_RESUME, 					/*!< @brief Playing resume, output: SUCCESS or Errcode */
	PLAYER_CMD_GETINFO,					/*!< @brief Playing info get, output: SUCCESS or Errcode, <mediaFileInfo_t>*/
	PLAYER_CMD_SEEK,						/*!< @brief Playing seek, args: <mode><val>, output: SUCCESS or Errcode, <actualTime> */
	PLAYER_CMD_SPEED_MODE,   			/*!< @brief Playing step motion, args: <speed>, output: SUCCESS or Errcode */
	PLAYER_CMD_SET_VOL,					/*!< @brief Playing set volumn, args: <vol>, output: SUCCESS or Errcode */
	PLAYER_CMD_GET_VOL,					/*!< @brief Playing get volumn, output: SUCCESS or Errcode, <vol> */
#if 1
	PLAYER_CMD_GET_VIDEO,                   		/*!< @brief Playing get video stream, output: SUCCESS or Errcode, <id> + <count>*/
	PLAYER_CMD_SET_VIDEO,                   		/*!< @brief Playing set video stream, args: <id> (-1 is default), output: SUCCESS or Errcode */
	PLAYER_CMD_GET_AUDIO,                  		/*!< @brief Playing get audio stream, output:  SUCCESS or Errcode, <id> + <count>*/
	PLAYER_CMD_SET_AUDIO,                 		/*!< @brief Playing set audio stream, args: <id> (-1 is default), output: SUCCESS or Errcode */
	PLAYER_CMD_GET_SUBTITLE,               		/*!< @brief Playing get subtitle stream, output: SUCCESS or Errcode, <id> + <count> */
	PLAYER_CMD_SET_SUBTITLE,                	/*!< @brief Playing set subtitle stream, args: <id> (-1 is default), output: SUCCESS or Errcode */
	PLAYER_CMD_GET_BITMAP,
	PLAYER_CMD_SET_BITMAP,
#else
	PLAYER_CMD_SELECT_AUDIO,                /*!< @brief Playing select audio stream, args: <id> */
	PLAYER_CMD_SELECT_SUBTITLE,             /*!< @brief Playing select subtitle stream, args: <id> */
#endif
	PLAYER_CMD_MAX							/*!< @brief cmd max, to be expanded */	
};


/** @brief Media Player command packet structure */
typedef struct mediaPlayerCmdPacket_s {

	UINT32 infoID;		/*ID of Message, Setup or Command*/

	UINT32 dataSize;		/*Size of variable lenth data*/
	UINT8  data[0]; 		/*Variable length data*/

} mediaPlayerCmdPacket_t;	


/** @} */




/** @defgroup Media Player Attrs Define
 *  @{
 */


/** @brief Media Player running state */
enum {
	PLAYER_STATE_NONE,
	PLAYER_STATE_PLAY,
	PLAYER_STATE_PAUSE,
	PLAYER_STATE_SEEK,
	PLAYER_STATE_SLOW,
	PLAYER_STATE_BUFFER
};

/** @brief Media Player seek mode */
enum {
	PLAYER_SEEK_MODE_MS,
	PLAYER_SEEK_MODE_MS_DELTA,
	PLAYER_SEEK_MODE_BYTE,
};

/** @brief Media file subTitle type */
#define MEDIA_SUBTITAL_TYPE_TXT    (1<<0)
#define MEDIA_SUBTITAL_TYPE_PIC    (1<<1)


#if 0
/** @brief KEYWORD of GETINFO CMD*/
#define MEDIA_INFO_CID				"cid"
#define MEDIA_INFO_NAME			"fn"
#define MEDIA_INFO_DURATION		"dt"	
#define MEDIA_INFO_ELAPSEDTIME	"et"
#define MEDIA_INFO_STATE			"ps"

#define MEDIA_INFO_WIDTH			"wi"
#define MEDIA_INFO_HEIGHT			"he"

#define MEDIA_INFO_VIDEO_CODEC	"vc"
#define MEDIA_INFO_AUDIO_CODEC	"ac"	
#define MEDIA_INFO_VIDEO_FR		"vf"	
#define MEDIA_INFO_VIDEO_BR		"vb"	
#define MEDIA_INFO_AUDIO_SR		"asr"
#define MEDIA_INFO_AUDIO_SB		"asb"
#define MEDIA_INFO_AUDIO_CH		"ach"	
#endif


/** @brief Media File Info Group */
typedef struct mediaFileInfo_s {

	SINT32 containerId; 	/*cid*/    /* ID for identify container, 
                                                  -1: not supported contain; 
                                                  -2: no element stream; 
                                                  -3: not support for all video;
                                                  -4: not support for all audio. */

	UINT8 fileName[256];	/*fn*/

	UINT32 duration;		/*dt*/
	UINT32 elapsedTime;		/*et*/
	UINT32 playState;		/*ps*/

	/*fourCC*/
	UINT32 videoCodec;	/*vc*/	
	UINT32 audioCodec;	/*ac*/
	
	UINT32 width;           /*wi*/  /* Video X resolution */
	UINT32 height;          /*he*/  /* Video Y resolution */

	UINT32 videoFrameRate;	/*vf*/
	UINT32 videoBitRate;	/*vb*/
	
	UINT32 audioSampleRate;	/*asr*/
	UINT32 audioSampleBits;	/*asb*/
	UINT32 audioChannels;	/*ach*/
	
#if 0	/*move to running command */
	UINT32 videoStreams;      /* Video stream count */
	UINT32 audioStreams;      /* Audio stream count */
	UINT32 subtitleStreams;   /* Subtitle stream count */
#endif	
	
	/*to be expanded*/

} mediaFileInfo_t;



/** @brief Media File thumbnail data */
typedef struct mediaThumbnail_s {
	UINT32 time; //ms, in/out -1: no thumbnail
	UINT32 format;
	UINT32 width;
	UINT32 height;
	UINT32 bpl;
	UINT32 dest_size;
	UINT8 bitmap[0];
} mediaThumbnail_t;


/** @brief Media File mark record */
typedef struct mediaMark_s
{
	UINT64 mark_value;	 	/*!< [in/out] 0: no mark */
	UINT32 mark_mode;	 	/*!< [in/out] 0: time, unit in ms, 1:offset, unit in byte. */
	UINT32 volume;		 	/*!< [in/out] -1: default volume */
	UINT32 video_idx;		/*!< [in/out] -1: default video stream */
	UINT32 audio_idx;	 	/*!< [in/out] -1: default audio stream */
	UINT32 sub_idx;		 	/*!< [in/out] -1: default sub stream */
} mediaMark_t;


/** @brief Media File subTitle info*/
typedef struct mediaSubTitle_s {
	UINT32 subType;		/*!< subtitle type */
	UINT32 colorType;	/*!< reference bitmap */
	UINT32 width; 		/*!< for picture */
	UINT32 height;		/*!< for picture */
	UINT32 size; 			/*!< data size */
	UINT8 data[0];		/*!< subtitle data */
} mediaSubTitle_t;


/** @brief Media video out data type */
typedef struct mediaVidOut_s
{
	UINT32 lock;           /*!< 1: lock, 0: unlock */
	UINT32 update;         /*!< 1: update, 0: no update */
	UINT32 updateIdx;      /*!< update index while frame count more than one */
	UINT32 count;          /*!< frame count */
	UINT32 frameSize;      /*!< the bytes of frame */
	UINT32 shareMemId ;   /*!< [in] share buffer id, 0: don't use share buffer */
	UINT32 aspect;         /*!< [in] 0:full, 1:keep aspect */
	UINT32 rotateAngle ;   /*!< [in] 0 - 360 */
	gp_bitmap_t outMap;    /*!< [in] video output map, struct define reference 
							*    typedef.h, if outMap.pData is null, mcplayer output                     							*     video to screen  */
} mediaVidOut_t;


/** @} */


#ifdef __cplusplus
};
#endif

#endif
