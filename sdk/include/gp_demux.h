
/***************************************************************************
 * Name: gp_demux.h
 *
 * Purpose:
 *
 * Developer:
 *     zhoulu, 2009-10-14
 *
 * Copyright (c) 2006-2007 by Sunplus mMobile Inc.
 ***************************************************************************/

#ifndef _GP_DEMUX_H_
#define _GP_DEMUX_H_

/***************************************************************************
 * Header Files
 ***************************************************************************/

#include "typedef.h"
#include "gp_avcodec.h"

#ifdef cplusplus
extern "C" {
#endif

/***************************************************************************
 * Constants
 ***************************************************************************/

#ifndef I_FRAME
#define I_FRAME 	0   // intra
#define P_FRAME	    1   // prediction
#define B_FRAME 	2   // bi-direction
#endif

#define MESP_OK                     0
#define MESP_FAIL                  -1
#define MESP_FULL			       -2
	
#define DEMUX_OK                    0
#define DEMUX_FAIL                 -1
	
// define for contain type
#define MEDIA_CONT_TYPE_UNSUPPORT	0
#define MEDIA_CONT_TYPE_RM			1
#define MEDIA_CONT_TYPE_AVI			2
#define MEDIA_CONT_TYPE_ASF			3
#define MEDIA_CONT_TYPE_FLV			4
#define MEDIA_CONT_TYPE_MP4			5
#define MEDIA_CONT_TYPE_MKV			6
#define MEDIA_CONT_TYPE_VOB    		7
#define MEDIA_CONT_TYPE_TS			8
#define MEDIA_CONT_TYPE_DAT			9
#define MEDIA_CONT_TYPE_PS			10 // compatible ps, vob, dat
	
// define struct for element stream packet, holds one packet or frame
#define MEDIA_ES_FLAG_FRAME     (1<<0) // packet is a frame, or not
#define MEDIA_ES_FLAG_SEQEND    (1<<1)  // ES sequence end
#define MEDIA_ES_FLAG_EOF       (1<<2)  // end of file
#define MEDIA_ES_FLAG_FILLED    (1<<3)  // packet is filled
#define MEDIA_ES_FLAG_FS        (1<<4) // frame start
#define MEDIA_ES_FLAG_FE        (1<<5) // frame end
#define MEDIA_ES_FLAG_SUB       (1<<6) // show default subpictrue
#define MEDIA_ES_FLAG_SYNC      (1<<7) // done sync
#define MEDIA_ES_FLAG_VIDEO     (1<<8) // find video es 
#define MEDIA_ES_FLAG_KEY       (1<<9) // find video es 
#define MEDIA_ES_FLAG_AUIDO     (1<<10) // find audio es
#define MEDIA_ES_FLAG_SUBPIC    (1<<11) // find subpic es
#define MEDIA_ES_FLAG_BREAK     (1<<12) // es break
#define MEDIA_ES_FLAG_FULL      (1<<13) // es full
#define MEDIA_ES_FLAG_ERROR     (1<<14) // parse error happy
#define MEDIA_ES_FLAG_CUTTED    (1<<15) // es packet was cutted
	
// define for element stream type
#define MEDIA_ES_TYPE_UNNEED    (0<<0)
#define MEDIA_ES_TYPE_VIDEO     (1<<0)
#define MEDIA_ES_TYPE_AUDIO     (1<<1)
#define MEDIA_ES_TYPE_SUBPIC    (1<<2)
#define MEDIA_ES_TYPE_NV        (1<<3)
	
// define for seek mode
#define MEDIA_SEEK_MODE_NO      (0<<0)
#define MEDIA_SEEK_MODE_MS      (1<<0)
#define MEDIA_SEEK_MODE_MSX     (1<<1)
#define MEDIA_SEEK_MODE_BYTE    (1<<2)
#define MEDIA_SEEK_MODE_PER     (1<<3)// percent to file size
#define MEDIA_SEEK_MODE_ALL     (MEDIA_SEEK_MODE_MS|MEDIA_SEEK_MODE_BYTE|MEDIA_SEEK_MODE_PER)
	
//define element stream buffer max size
#define MEDIA_VESB_MAX_SIZE   (4 * 1024 * 1024)
#define MEDIA_AESB_MAX_SIZE   (256 * 1024)
#define MEDIA_SESB_MAX_SIZE   (256 * 1024)
#define MEDIA_VESB_MAX_SYNC   (512 * 1024)

//define demuxer max size
#define MEDIA_DEMUX_MAX_SIZE  (MEDIA_VESB_MAX_SIZE + MEDIA_AESB_MAX_SIZE + MEDIA_SESB_MAX_SIZE)
	
//define element stream delay time in buffer
#define MEDIA_ES_DELAY_TIME    1000 // ms

//define probe type
#define MEDIA_PROBE_FOR_INFO  0
#define MEDIA_PROBE_FOR_PLAY  1
	
// define demux es max count
#define MEDIA_VES_MAX_CNT     8
#define MEDIA_AES_MAX_CNT     16
#define MEDIA_SES_MAX_CNT     16

// define media message
#define MEDIA_MSG_VIDEO_END   1
#define MEDIA_MSG_AUDIO_END   2
#define MEDIA_MSG_SUB_END     3
#define MEDIA_MSG_PLAY_END    4

#define DEMUX_DEFAULT_PLAY_ES  (~0)
#define MEDIA_FILE_IO_BUFFER  64 * 1024

/***************************************************************************
 * Macros
 ***************************************************************************/
#define MAX(a,b)                        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)                        (((a) < (b)) ? (a) : (b))

/***************************************************************************
 * Data Types
 ***************************************************************************/
typedef enum TxtSubTitleFormat
{
	TXT_SUBFORMAT_NONE = 0, 
	TXT_SUBFORMAT_SRT,
	TXT_SUBFORMAT_ASS,
	TXT_SUBFORMAT_SMI,
	TXT_SUBFORMAT_SUB,
	TXT_SUBFORMAT_AQT,
	TXT_SUBFORMAT_UTF,
	TXT_SUBFORMAT_LRC,
}TxtSubTitleFormat_E;

typedef enum TxtSubTitleType
{
	TXT_SUBTYPE_UNKNOWN = 0,
	TXT_SUBTYPE_UNICODE,
	TXT_SUBTYPE_UTF8,
	TXT_SUBTYPE_GB2312,
	TXT_SUBTYPE_BIG5,
	TXT_SUBTYPE_SHIFT_JIS,
	TXT_SUBTYPE_EUC_JP,
	TXT_SUBTYPE_EUC_KR,
}TxtSubTitleType_E;

typedef enum SubTitleSupport
{
	SUB_TITLE_NONE = 0,
	SUB_TITLE_TXT,
	SUB_TITLE_PICTURE,
}SubTitleSupport_t;

typedef struct TxtSTParser
{
	SINT32	iTFHandle;
	UINT32	uiTFSize;
	TxtSubTitleFormat_E genTSTFormat;
	TxtSubTitleType_E enSubTxtType;
	UINT32	uiStartPos;
	UINT32	uiSubNum;
	UINT8	*pucTDataBuf;
	
}TxtSTParser_S;


typedef struct ParserSubTitle_t
{
	SubTitleSupport_t enSubTitleSup;
	UINT32 uiTimeStart;
	UINT32 uiTimeEnd;
	UINT32 uiPicWidth;
	UINT32 uiPicHeight;
	UINT32 uiSubX;
	UINT32 uiSubY;
	UINT32 uiBufSize;
	UINT8  *pucSubBuf;
	UINT8  *pucMMIShowBuffer;
	gp_bitmap_t *subpic;
}ParserSubTitle_S;

typedef struct es_packet_s
{
	UINT8 *ptr_data; // packet prt
	UINT32 size; // unit in byte
	UINT32 frame_type; // 0: I, 1: P, 2: B
	UINT32 pts; // presentation time, unit in ms
	UINT32 duration; // for audio es data 
	UINT32 base_time; // base_time + pts = show time, unit in ms
	UINT32 sync_request; //0: no, 1: only video, 2: only audio, 3: audio / video at the sync time
	UINT64 url_pos; //unit in bytes, 
	UINT32 flag; // frame_start/end, seq_end, no_fill;
	UINT32 addr; // this es packet struct malloc address
	struct es_packet_s *next;
}es_packet_t;

typedef void (*mesp_lock_func)(void);
// define for media es packet manage
typedef struct mesp_s
{
	UINT8 *buf;     // buffer address
	UINT32 size;     // buffer size
	UINT8 *w, *r, *e; // buffer write, read ,end address
	es_packet_t *first_esp; // ready packet link
	es_packet_t *last_esp;
	es_packet_t *first_unfill_esp; 
	UINT32 delay_time; // last parsed valid pts
	UINT32 delay_size; // parsed size from last parsed pts
	UINT32 self_malloc;     // buffer address
	UINT32 bottom_max_size;// the bottom max size for probe frame end
	UINT32 count;
	void (*mutex_lock)(void);
	void (*mutex_unlock)(void);
	// set the bottom max size
	void (*set_bottom_max_size)(struct mesp_s *mesp, UINT32 size);
	SINT32 (*get_bottom_max_size)(struct mesp_s *mesp);
	// get total size of filled data
	SINT32 (*fill_size)(struct mesp_s *mesp);
	// get bottom size of filled data
	SINT32 (*fill_bot_size)(struct mesp_s *mesp);
	// get idle buffer, space size, bottom size
	UINT8 *(*buf_space)(struct mesp_s *mesp, UINT32 *space_size, UINT32 *bottom_size);
	// add a packet to ready packet link
	SINT32 (*add)(struct mesp_s *mesp, es_packet_t *esp, UINT32 flag);
	// turn write ptr to begin
	SINT32 (*turn)(struct mesp_s *mesp);
	// update info for not fill esp
	SINT32 (*update)(struct mesp_s *mesp, es_packet_t *esp, UINT32 flag);
	// get a packet from ready packet link
	es_packet_t *(*get)(struct mesp_s *mesp, UINT32 flag);
	// check video frame
	SINT32 (*check_frame)(struct mesp_s *mesp);
	// clear ready packet link
	SINT32 (*clear)(struct mesp_s *mesp);
	// skip some packet in ready packet link, 
	// flag: 0: skip "size" bytes, 1: skip "size" packet
	SINT32 (*skip)(struct mesp_s *mesp, UINT32 size, UINT32 flag);
	// creat a new packet
	es_packet_t *(*newp)(void);
	// free a packet
	SINT32 (*freep)(es_packet_t *esp);
	// find frame start position
	SINT32 (*sync_frame)(UINT8 *buf, UINT32 size);
	// parse frame type
	SINT32 (*parse_frame)(UINT8 *buf, UINT32 size);
}mesp_t;

struct media_demux_s;
struct media_contain_s;

// define for video element stream
typedef struct video_info_s 
{
	UINT32 es_id;// the id of video element stream in all element streams
	UINT32 code_format;// video codec standard
	UINT32 width, height;// unit in pixel
	UINT32 frame_rate;// unit in fps
	UINT32 bit_rate;// unit in bps
	UINT32 interlace;// if 1, video codec used interlace , or not
	UINT32 duration;// video duration, unit in ms
	UINT32 cfps; // if 1, video is constant fps, or not
	UINT32 apect;// display apect
	UINT32 start_time;// video begin time, unit in ms
	UINT32 frame_interval; // unit in ms
	UINT32 decode_size;   // min 5s data, max 500 * 1024 for 720p 
	UINT32 seek_mode;	   // 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	struct media_contain_s *contain;// the contain that include the video es 
	// find frame start position
	SINT32 (*sync_frame)(UINT8 *buf, UINT32 size);// find the begin position of frame in es buffer
	// parse frame type
	SINT32 (*parse_frame)(UINT8 *buf, UINT32 size);// parse frame type
}video_info_t;

// define for audio element stream
typedef struct audio_info_s 
{
	UINT32 es_id;// the id of audio element stream in all element streams
	UINT32 code_format;// audio codec standard
	UINT32 channel_cnt;// audio codec channel count
	UINT32 sample_rate;// audio codec sample rate
	UINT32 bit_rate;// audio codec bit rate, unit in bps
	UINT32 block_align;// audio duration, unit in ms
	UINT32 duration;// audio duration, unit in ms
	UINT32 v_es_id; // the  element stream id of video that attend by this audio
	UINT32 start_time;// audio begin time, unit in ms
	UINT32 audBits;
	UINT32 vbr;
	UINT32 seek_mode;	   // 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	UINT32 extra_data_size;// audio extra data size, unit in byte
	UINT8 *extra_data;// audio extra data
	struct media_contain_s *contain;
	
}audio_info_t;

// define for sub element stream
typedef struct sub_info_s 
{
	UINT32 es_id;// the id of subtitle element stream in all element streams
	UINT32 code_format; // text, picture in contain, or not in contain
	UINT32 type; // text, picture
	UINT32 start_time;// subtitle begin time, unit in ms
	UINT8 name[8];// the language name of the subtitle
	UINT32 seek_mode;	   // 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	struct media_contain_s *contain;// the contain that include the subtitle es 
}sub_info_t;

// define for demux of mcp
typedef struct mcp_demux_ctrl_s
{
	SINT32 (*open)(SINT8 *file_name, UINT32 *cid, UINT32 decrpyt_type, SINT8 *decrpyt_key);
	SINT32 (*duration)(SINT8 *file_name);
	void (*close)(void);
	// unit: 0: unit in bytes, 1: unit in ms, 2: unit in 1/100
	// return the destination after seek
	SINT32 (*seek)(UINT32 *dest, UINT32 unit);
	SINT32 (*fast_play)(UINT32 step, UINT32 unit);
	SINT32 (*resume)(UINT64 dest, UINT32 unit);
	UINT32 (*get_subtitle_num)(void);
	UINT32 (*change_subtitle)(UINT32 idx, UINT32 ms);
	
	// video element stream operation
	SINT32 (*get_v_cnt)(void);
	SINT32 (*set_v_play_es)(SINT32 idx);
	SINT32 (*get_v_play_es)(void);
	SINT32 (*get_v_info)(video_info_t **vi, SINT32 idx);
	SINT32 (*fill_v_es)(void);
	SINT32 (*fill_v_unload)(void);
	SINT32 (*get_v_bits_delay)(void);// return ms
	SINT32 (*trans_v_packet)(void);
	SINT32 (*get_v_mesp)(mesp_t **mesp);
	SINT32 (*set_v_enable)(UINT32 flag);
	
	// audio element stream operation
	SINT32 (*get_a_cnt)(void);
	SINT32 (*set_a_play_es)(SINT32 idx);
	SINT32 (*get_a_play_es)(void);
	SINT32 (*get_a_info)(audio_info_t **ai, SINT32 idx);
	SINT32 (*fill_a_es)(void);
	SINT32 (*fill_a_unload)(void);
	SINT32 (*get_a_bits_delay)(void);// return ms
	SINT32 (*get_a_mesp)(mesp_t **mesp);
	SINT32 (*set_a_enable)(UINT32 flag);
	SINT32 (*get_a_max_sr)(void);
		
	// sub picture element stream operation
	SINT32 (*get_s_cnt)(void);
	SINT32 (*set_s_play_es)(SINT32 idx);
	SINT32 (*get_s_play_es)(void);
	SINT32 (*get_s_info)(sub_info_t **si, SINT32 idx);
	SINT32 (*fill_s_es)(ParserSubTitle_S *pstPSubTitle);
	SINT32 (*fill_s_unload)(void);
	SINT32 (*get_s_bits_delay)(void);// return ms
	SINT32 (*get_s_mesp)(mesp_t **mesp);
	SINT32 (*set_s_enable)(UINT32 flag);
	
	// for player
	SINT32 (*get_seek_mode)(void);
	SINT32 (*get_play_ess)(void);
	SINT32 (*get_es_delay)(void);
	SINT32 (*get_lenght)(void);
	SINT32 (*get_bookmark_mode)(void);
	SINT32 (*get_parsed_pts)(void);

}mcp_demux_ctrl_t;


/***************************************************************************
 * Global Data
 ***************************************************************************/

/***************************************************************************
 * Function Declarations
 ***************************************************************************/

//define function called by play control
// init demux
SINT32 demux_open(SINT8 *file_name, UINT32 *cid, UINT32 decrpyt_type, SINT8 *decrpyt_key);
SINT32 demux_duration(SINT8 *file_name);
void demux_close(void);
// unit: 0: unit in bytes, 1: unit in ms, 2: unit in 1/100
// return the destination after seek
SINT32 demux_seek(UINT32 *dest, UINT32 unit);
SINT32 demux_seek2(UINT32 *dest, UINT32 unit, UINT32 flag);

SINT32 demux_fast_play(UINT32 step, UINT32 unit);
SINT32 demux_resume(UINT64 dest, UINT32 unit);
UINT32 demux_get_subtitle_num(void);
UINT32 demux_change_subtitle(UINT32 idx, UINT32 ms);

// video element stream operation
SINT32 demux_get_v_cnt(void);
SINT32 demux_set_v_play_es(SINT32 idx);
SINT32 demux_get_v_play_es(void);
video_info_t *demux_get_v_info(SINT32 idx);
SINT32 demux_fill_v_es(void);
SINT32 demux_fill_v_unload(void);
SINT32 demux_get_v_bits_delay(void);// return ms
SINT32 demux_trans_v_packet(void);
mesp_t *demux_get_v_mesp(void);
SINT32 demux_set_v_enable(UINT32 flag);
SINT32 demux_set_v_decrypt(UINT32 type);
SINT32 demux_set_v_exact_seek(UINT32 flag);

// audio element stream operation
SINT32 demux_get_a_cnt(void);
SINT32 demux_set_a_play_es(SINT32 idx);
SINT32 demux_get_a_play_es(void);
audio_info_t *demux_get_a_info(SINT32 idx);
SINT32 demux_fill_a_es(void);
SINT32 demux_fill_a_unload(void);
SINT32 demux_get_a_bits_delay(void);// return ms
mesp_t *demux_get_a_mesp(void);
SINT32 demux_set_a_enable(UINT32 flag);
SINT32 demux_get_a_max_sr(void);
	
// sub picture element stream operation
SINT32 demux_get_s_cnt(void);
SINT32 demux_set_s_play_es(SINT32 idx);
SINT32 demux_get_s_play_es(void);
sub_info_t *demux_get_s_info(SINT32 idx);
SINT32 demux_fill_s_es(ParserSubTitle_S *pstPSubTitle);
SINT32 demux_fill_s_unload(void);
SINT32 demux_get_s_bits_delay(void);// return ms
mesp_t *demux_get_s_mesp(void);
SINT32 demux_set_s_enable(UINT32 flag);
SINT8 *demux_get_s_language(void);
SINT32 demux_set_s_language(SINT8 *language);

// for player
SINT32 demux_get_seek_mode(void);
SINT32 demux_get_play_ess(void);
SINT32 demux_get_es_delay(void);
SINT32 demux_get_length(void);
SINT32 demux_get_bookmark_mode(void);
SINT32 demux_get_parsed_pts(void);

SINT32 demux_choose_ves(UINT32 idx);
SINT32 demux_choose_aes(UINT32 idx);
SINT32 demux_choose_ses(UINT32 idx);



SINT32 fio_write_log(UINT8 *buf, UINT32 size);
SINT32 fio_close_log(void);


/***************************************************************************
 * Inline Function Definitions
 ***************************************************************************/

#ifdef cplusplus
}
#endif

#endif   /* _MESP_H_  */

/***************************************************************************
 * The gp_demux.h file end
 ***************************************************************************/
