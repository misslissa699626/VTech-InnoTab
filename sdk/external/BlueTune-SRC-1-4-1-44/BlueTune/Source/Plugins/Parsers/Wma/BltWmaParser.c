/*****************************************************************
|
|   Wma Parser Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltWmaParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"

#if 0
#define DEBUG0 printf
#else
#define DEBUG0(...)
#endif

#if 0
#if 0
#define DEBUG0(args...) fprintf(stderr, args...)
#else
#define DEBUG0(...)
#endif
#endif


/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.wma")

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 wma_type_id;
} WmaParserModule;



typedef struct ComPressedPay_t
{	
	BLT_UInt32 uiCurCPayNum;
	BLT_UInt32 uiSendTime;
	BLT_UInt32 uiTimeFlag;
	BLT_UInt32 uiReGetPay;
}ComPressedPay_s;

typedef struct PPIInfo_t
{
	BLT_UInt16 	usPadLen;
	BLT_UInt8 	ucPayType;
	BLT_UInt8 	ucRepType;
	BLT_UInt8 	ucStrNrType;
	BLT_UInt8 	ucMediaObjType;
	BLT_UInt8 	ucMediaOffType;
} PPIInfo_S;




typedef struct FilePro_t
{
	BLT_UInt32 	uiPktSize;
	BLT_UInt32  uiPktNum;
	BLT_UInt32  uiTotalTime;
	BLT_UInt32 	uiIDXNum;
	BLT_UInt32  uiTimeInterval;
	BLT_UInt32 	uiPreroll;
}FilePro_S;


typedef struct Player_t
{
	BLT_UInt32 	uiAudms;
	BLT_UInt32 	uiVidms;
	BLT_UInt32 	uiStrNum;
	BLT_UInt8 	uiSeekable;
	BLT_UInt8 	uiKeyFrmGet;
}Player_S;


typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    ATX_InputStream* 	stream;
    BLT_MediaType 		media_type;
    BLT_UInt32 			FileHandle;
	BLT_UInt32 			uiFileOffSet;
	BLT_UInt8 			*pMemAddr;
	BLT_UInt32 			uiMemSize;
	BLT_UInt32 			uiMemCur;
	BLT_UInt32 			uiMemFilled;
	BLT_UInt32 			uiStreamCur;
	BLT_UInt32 			uiStreamEnd;
	BLT_UInt16 			uiSeekFlag;
	BLT_UInt8  			uiReadFlag;
	BLT_UInt8  			uiCache;
	BLT_Boolean      	eos;
} WmaParserInput;

typedef struct PktParser_t
{
	BLT_UInt32 			uiNowPkt;
	BLT_UInt32 			uiObjSize;
	BLT_UInt32 			uiPayLoadType;
	BLT_UInt32 			uiNowPayLoad;
	BLT_UInt8 			ucStrNum;
	ComPressedPay_s 	stCPayload;
	PPIInfo_S 			stPpi;
	WmaParserInput*		pIdxStr;	
} PktParser_S;

typedef struct wma_audio_s
{
	/* Stream Properties Object */
	BLT_Int16		wAudioStreamId;
	BLT_Int16		wFormatTag;
	BLT_Int16		nChannels;
	BLT_Int32		nSamplesPerSec;
	BLT_Int32		nAvgBytesPerSec;
	BLT_Int16		nBlockAlign;
	BLT_Int16		wBitsPerSample;
	BLT_Int32		nSamplesPerBlock;
	BLT_Int16		nEncodeOpt;
	
	/* Packet / payload header information */
	BLT_Int8		bStreamId;
	BLT_Int8		cbRepData;
	BLT_UInt16 		cbPayloadSize;
}wma_audio_t;



typedef struct {
	BLT_MediaType   base;
  	wma_audio_t		wma_audio_info;
	BLT_UInt32		extra_data_size;// audio extra data size, unit in byte
    /* variable size array follows */
    BLT_Int8*    	extra_data; 
}BLT_WmaAudioMediaType;




typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_WmaAudioMediaType* 	media_type;
} WmaParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
	BLT_UInt32 			uiMemAddr;	
	Player_S 			stPlayer;	
	PktParser_S 		stAudObj;
	FilePro_S 			stFile;
	BLT_UInt32 			uiDataPos;
	BLT_UInt32 			uiIdxPos;
	BLT_UInt8  			*pAudSeekBuf;
    WmaParserInput  	input;
    WmaParserOutput 	output;	
} WmaParser;



typedef struct mp3_extend_data_s
{
	BLT_UInt16 wId;
	BLT_UInt32 fdwFlags;
	BLT_UInt16 nBlockSize;
	BLT_UInt16 nFramesPerBlock;
	BLT_UInt16 nCodecDelay;
}mp3_extend_data_t ;/*12 bytes in this structure,0x0c*/


typedef struct ad_pcm_extend_s
{
	BLT_UInt16 wSamplesPerBlock;
}ad_pcm_extend_t ;

typedef struct amr_extend_s
{
	BLT_UInt32 dwFlags;
}amr_extend_t ;


typedef struct wma_extend_s
{
	BLT_UInt32 dwSamplesPerBlock;
	BLT_UInt16 wEncodeOptions;
	BLT_UInt32 dwSuperBlockAlign;
}wma_extend_t ;

typedef struct wav_format_s
 {
	BLT_UInt16 wFormatTag;
	BLT_UInt16 nChannels;
	BLT_UInt32 nSamplesPerSec;
	BLT_UInt32 nAvgBytesPerSec;
	BLT_UInt16 nBlockAlign;
	BLT_UInt16 wBitsPerSample;
	BLT_UInt16 cbSize;

	union 
	{
		mp3_extend_data_t 	mp3;
		ad_pcm_extend_t 	ad_pcm;
		amr_extend_t 		amr;
		wma_extend_t 		wma;
	}ext;
} wav_format_t;/*18 bytes in this structure,0x12*/


typedef struct codec_spec_data_s
{
	wav_format_t 	wav;
	BLT_UInt32      Reserve1;
	BLT_UInt32      Reserve2;
}codec_spec_data_t ;


typedef struct MP43VideoObjectLayer_t
{
	BLT_UInt32 uiVol_start_code;
	BLT_UInt32 uiHeader_Length; 
	BLT_UInt32 uiFrame_period;
	BLT_UInt32 uiBit_rate;
	BLT_UInt32 uiHandle;
	BLT_UInt32 uiScale;
	BLT_UInt32 uiRate;
	BLT_UInt32 uiFourcc;
	BLT_UInt32 uiWidth;
	BLT_UInt32 uiHeight;
	BLT_UInt16 uiDepth;
	BLT_UInt16 uiReserve;  /* must zero*/
}MP43VideoObjectLayer_s;

typedef struct MP43VideoObjectPlane_t
{
	BLT_UInt32 uiVop_start_code;
	BLT_UInt32 uiVop_data_length;  /* Vop_data_length(24) + Reserve(8 must zero) */
}MP43VideoObjectPlane_s;

typedef struct WMV78VideoHeader_t
{
	BLT_UInt32 uiFourCC;
	BLT_UInt32 uiWidth;
	BLT_UInt32 uiHeight;
	BLT_UInt32 uiexDataLen;     //for WMV7 must 0
}WMV78VideoHeader_s;



typedef struct WmaInfo_t 
{
	BLT_UInt32 	uiVidType; 			/* e.g. MPEG4, MJPG, etc */
	BLT_UInt32 	uiAudType; 			/* e.g. AMR, AAC, etc */
	BLT_UInt32 	uiWidth; 			/* pixels in x-axis */
	BLT_UInt32 	uiHeight;			/* pixels in y-axis */
	BLT_UInt32 	uiFrmRate;			/* frame rate (fps) */
	BLT_UInt32 	uiVopTimeIncLen; 	/* time increment length */
	BLT_UInt32 	uiDpEn; 			/* Data Partitioned enable */
	BLT_UInt32 	uiRvlcEn; 			/* RVLC enable */
	BLT_UInt32 	uiGobEn; 			/* GOB enable */
	BLT_UInt32 	uiGopNo; 			/* GOP number (length between two I frames) */
	BLT_UInt32 	uiVidLen;			/* video total seconds */
	BLT_UInt32 	uiAudLen;			/* audio total seconds */
	BLT_UInt32 	uiAudSR;			/* audio sample rate */
	BLT_UInt32 	uiVideoFTimeSt;

	BLT_UInt32 	vidExtraLen;   		/* The count in bytes of the size of  video extra information */ 
	BLT_UInt8* 	pVidExtraData; 		/* video extra information */
	BLT_UInt32 	audExtraLen;   		/* The count in bytes of the size of  audio extra information */ 
	BLT_UInt8* 	pAudExtraData; 		/* audio extra information */	
	BLT_UInt32 	vidStreamNum;  		/* video stream number */
	BLT_UInt32 	audStreamNum;  		/* audio stream number */

	BLT_UInt8 	uiAudChannel; 		/* audio channel number(s) */
	BLT_UInt8 	uiAudBit; 			/*audio sample length*/
	BLT_UInt8 	uiSeekable;			/*the Wma file can or not seek*/
	BLT_UInt8 	isVBR;
	BLT_UInt32 	uiAudBitRate;		/*Audio bit rate*/
	BLT_UInt16 	uiBlockAlign;		/*++lyh@20070202_[mantis:6219] for Wma*/
	BLT_UInt32 	uiSamplesPerBlock; 	/* ++ jerry : for wma payload decoding ++ */
	BLT_UInt16 	uiEncodeOpt; 		/* ++ jerry : for wma payload decoding ++ */
	BLT_UInt16 	usScale;
	BLT_UInt16 	usDepth;
} WmaInfo_S;

typedef struct ContainBitsInfo_s 
{
	BLT_UInt32 vidType;       /* e.g. MPEG4, MJPG, etc */
	BLT_UInt32 audType;       /* e.g. AMR, AAC, etc */
	BLT_UInt32 width;         /* pixels in x-axis */
	BLT_UInt32 height;        /* pixels in y-axis */
	BLT_UInt32 frmRate;       /* frame rate (fps) */
	BLT_UInt32 vopTimeIncLen; /* time increment length */
	BLT_UInt32 dpEn;          /* Data Partitioned enable */
	BLT_UInt32 rvlcEn;        /* RVLC enable */
	BLT_UInt32 gobEn;         /* GOB enable */
	BLT_UInt32 gopNo;         /* GOP number (length between two I frames) */
	BLT_UInt32 vidLen;        /* video total million seconds */
	BLT_UInt32 audLen;        /* audio total million seconds */
	BLT_UInt32 TotalFrameCount;  /*total video frame number*/
	BLT_UInt32 IFrameCount;      /*video I frame number*/
	BLT_UInt32 vidBitrate;    /*video bitrate*/
	BLT_UInt32 vInterFlag;    /*the video is interleaved*/
	
	BLT_UInt32 vidExtraLen;   /* The count in bytes of the size of  video extra information */ 
	BLT_UInt8* pVidExtraData; /* video extra information */
	BLT_UInt32 audExtraLen;   /* The count in bytes of the size of  audio extra information */ 
	BLT_UInt8* pAudExtraData; /* audio extra information */	
	BLT_UInt32 vidStreamNum;  /* video stream number */
	BLT_UInt32 audStreamNum;  /* audio stream number */

	BLT_UInt32 audBitrate;    /* audio bitrate*/
	BLT_UInt16 audSR;         /* audio sample rate */
	BLT_UInt8  audChannel;    /* audio channel */
	BLT_UInt8  audBits;       /* Bits of per audio sample */
	BLT_UInt8  bSeekable;     /* Can be seek or not */ 
    /* francis@20061121 : Add PMP Video Record Rotate flow */
	BLT_UInt8  rotate;        /* record rotate */
	BLT_UInt8  reserved[2];
	BLT_UInt8  isVBR; /*Audio:0-noVBR 1-VBR 2-Unknown*/
	BLT_UInt16 uiBlockAlign;  /*++lyh@20070202_[mantis:6219] for Wma*/	
	BLT_UInt32 clusterSize;     /* jerry: add for cluster alighment */
	BLT_UInt32 targetDisk;	/* jerry: save target storage, e.g. sd, nand  */
}ContainBitsInfo_t;

typedef struct guid_s
{
	BLT_UInt32 v1; 
	BLT_UInt16 v2;
	BLT_UInt16 v3; 
	BLT_UInt8  v4[8]; 
} guid_t;

typedef struct bit_map_info_header_s
{ // bmih 
	BLT_UInt32	biSize;
	BLT_UInt32	biWidth;
	BLT_UInt32	biHeight;
	BLT_UInt16	biPlanes;
	BLT_UInt16	biBitCount;
	BLT_UInt32	biCompression;
	BLT_UInt32	biSizeImage;
	BLT_UInt32	biXPelsPerMeter;
	BLT_UInt32	biYPelsPerMeter;
	BLT_UInt32	biClrUsed;
	BLT_UInt32	biClrImportant;
} bit_map_info_header_t;
	

typedef struct codec_spec_data1_s
{
	BLT_UInt32 dwEncodedImageWidth;
	BLT_UInt32 dwEncodedImageHeight;
	BLT_UInt8  cReservedFlags;
	BLT_UInt16 wFormatDataSize;
	bit_map_info_header_t bitmap;
}codec_spec_data1_t;
	
typedef struct error_correction_s
{
	BLT_UInt8  i_span;
	BLT_UInt16 i_virtual_packet_length;
	BLT_UInt16 i_virtual_chunk_length;
	BLT_UInt16 i_silence_data_length;
	BLT_UInt8  i_silence_data;
}error_correction_t ;
	
typedef struct Wma_object_common_s
{
	guid_t	 i_object_id;  
	BLT_UInt64	 i_object_size;
} Wma_object_common_t;
	
typedef struct Wma_index_entry_s
{
	BLT_UInt32 i_packet_number;
	BLT_UInt16 i_packet_count;
} Wma_index_entry_t;

typedef struct Wma_object_header_s
{
	BLT_UInt32 i_sub_object_count;
	BLT_UInt8  i_reserved1;/* 0x01, but could be safely ignored */
	BLT_UInt8  i_reserved2; /* 0x02, if not must failed to source the contain */
} Wma_object_header_t;
	
typedef struct Wma_object_data_s
{
	guid_t	  i_file_id;
	BLT_UInt64	  i_total_data_packets;
	BLT_UInt16	  i_reserved;
} Wma_object_data_t ;
	
	
typedef struct Wma_object_index_s
{
	guid_t	   i_file_id;
	BLT_UInt64	  i_index_entry_time_interval;
	BLT_UInt32	  i_max_packet_count;
	BLT_UInt32	  i_index_entry_count;
} Wma_object_index_t;
	

typedef struct Wma_object_file_properties_s
{
	guid_t	i_file_id;
	BLT_UInt64	i_file_size;
	BLT_UInt64	i_creation_date;
	BLT_UInt64	i_data_packets_count;
	BLT_UInt64	i_play_duration;
	BLT_UInt64	i_send_duration;
	BLT_UInt64	i_preroll;
	BLT_UInt32	i_flags;
	BLT_UInt32	i_min_data_packet_size;
	BLT_UInt32	i_max_data_packet_size;
	BLT_UInt32	i_max_bitrate;
} Wma_object_file_properties_t;
	
typedef struct Wma_object_stream_properties_s
{
	guid_t	i_stream_type;
	guid_t	i_error_correction_type;
	BLT_UInt64	i_time_offset;
	BLT_UInt32	i_type_specific_data_length;
	BLT_UInt32	i_error_correction_data_length;
	BLT_UInt16	i_flags;
	BLT_UInt32	i_reserved;
} Wma_object_stream_properties_t;
	
typedef struct Wma_object_header_extension_s
{
	BLT_UInt16	  i_reserved1;
	BLT_UInt16	  i_reserved2;
	BLT_UInt32	  i_header_extension_size;
} Wma_object_header_extension_t ;
	
	
typedef struct Wma_objec_content_description_s
{
	BLT_UInt16 i_title_length;
	BLT_UInt16 i_author_length;
	BLT_UInt16 i_copyright_length;
	BLT_UInt16 i_description_length;
	BLT_UInt16 i_rating_length;
} Wma_object_content_description_t;
	
typedef struct Wma_codec_entry_s
{
	BLT_UInt16	  i_type;
	BLT_UInt16	  i_codec_name_length;
	BLT_UInt8	   *pwc_name;
	BLT_UInt16	  i_codec_des_length;
	BLT_UInt8	   *pwc_des;
	BLT_UInt16	  i_information_length;
	BLT_UInt8	  *p_information;
} Wma_codec_entry_t;
	
typedef struct Wma_object_codec_list_s
{
	BLT_UInt32	  i_reserved;
	BLT_UInt32	  i_codec_entries_count;
} Wma_object_codec_list_t;
	
typedef struct Wma_object_s
{
	/*此处如何修改*/
	//Wma_object_common_t com GNU_PACK;
	union
	{
	Wma_object_header_t 			 header;
	Wma_object_data_t				 data;
	Wma_object_index_t				 index;
	Wma_object_file_properties_t	 file_properties;
	Wma_object_stream_properties_t	 stream_properties;
	Wma_object_header_extension_t	 header_extension;
	Wma_object_codec_list_t 		 codec_list;
	Wma_object_content_description_t content;
	}un;
} Wma_object_t;
	
#define SIZE_PACKET_PPI 11

typedef struct packet_ppi_s
{
	BLT_UInt8  lenTypeFlags;
	BLT_UInt8  propertyFlags;
	BLT_UInt16 paddingLen;
	BLT_UInt32 sendTime;
	BLT_UInt16 duration;
	BLT_UInt8  payloadFlags;
} packet_ppi_t; /* Payload parsing information */
	
typedef struct packet_single_payload_s 
{
	BLT_UInt8  streamNr;
	BLT_UInt8  mediaObjNr;
	BLT_UInt32 offsetIntoMediaObj;
	BLT_UInt8  replicatedDataLen;
	BLT_UInt32 repDataMediaObjLen;
	BLT_UInt32 repDataPresentTime;
} packet_single_payload_t;
	
typedef struct packet_multi_compress_payload_s 
{
	BLT_UInt8  streamNr;
	BLT_UInt8  mediaObjNr;
	BLT_UInt32 offsetIntoMediaObj;
	BLT_UInt8  replicatedDataLen;
	BLT_UInt32 repDataMediaObjLen;
	BLT_UInt32 repDataPresentTime;
	BLT_UInt16 payloadLen;
} packet_multi_compress_payload_t;
	
typedef struct packet_multi_payload_s 
{
	BLT_UInt8  streamNr;
	BLT_UInt8  mediaObjNr;
	BLT_UInt32 offsetIntoMediaObj;
	BLT_UInt8  replicatedDataLen;
	BLT_UInt64 replicatedData;
	BLT_UInt16 payloadLen;
} packet_multi_payload_t;
	
typedef struct index_entry_s
{
	BLT_UInt32 packetNumber;
	BLT_UInt16 packetCount;
}index_entry_t;
	
typedef struct sequence_layer_data_s
{
	BLT_UInt32 uiNumFrames;
	BLT_UInt32 uiFlag1; /*0x00000004*/
	BLT_UInt8  uiExtraData[4]; 
	BLT_UInt32 uiHeight;
	BLT_UInt32 uiWidth;
	BLT_UInt32 uiFlag2; /*0x0000000C*/
	BLT_UInt32 uiAlterBufSize;
	BLT_UInt32 uiAverageBitrate;
	BLT_UInt32 uiFrameRate;
}sequence_layer_data_t;
	
typedef struct Wma_object_extended_stream_properties_s
{
	BLT_UInt64	ui64StartTime;
	BLT_UInt64	ui64EndTime;
	BLT_UInt32	uiDataBitrate;
	BLT_UInt32	uiBufferSize;
	BLT_UInt32	uiIniBufFull;
	BLT_UInt32	uiAlterDataBitrate;
	BLT_UInt32	uiAlterBufSize;
	BLT_UInt32	uiALterIniBufFull;
	BLT_UInt32	uiMaxobjSize;
	BLT_UInt32	uiFlags;
	BLT_UInt16	usStreamNum;
	BLT_UInt16	usStreamLangID;
	BLT_UInt64	ui64AverTimePerFrame;
	BLT_UInt16	usStreamNameCount;
	BLT_UInt16	usPayloadExtSysCount;
}Wma_object_extended_stream_properties_t;
	
typedef struct Wma_rcv_frame_s
{
	BLT_UInt32	uiframesize; //31bit keyframe
	BLT_UInt32	uiframetime;
}Wma_rcv_frame_t;


/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define UNKNOW_ID       			0
#define OBJ_HDR_ID      			1
#define OBJ_DATA_ID     			2
#define OBJ_IDX_ID      			3
#define FILE_PRO_ID     			4
#define STREAM_PRO_ID   			5
#define CONTENT_DEC_ID  			6
#define HDR_EXT_ID      			7
#define CODEC_LIST_ID   			8
#define PADDING_ID      			9
#define STR_BIT_PROPER_ID  			10
#define EXT_STREAM_PRO_ID  			11
#define CONTENT_ENCRYPTION_ID  		12

#define OBJ_HEADER_SIZE  			6  	
#define OBJ_FILE_PROPER_SIZE 		80  	
#define OBJ_EX_STREAM_PROPER_SIZE 	64
#define OBJ_STREAM_PROPER_SIZE 		54
#define OBJ_DATA_SIZE 				26
#define OBJ_INDEX_SIZE 				32
#define INDEX_ENTRY_SIZE 			6 


/*Wma file fixed packet size*/
#define WMA_PACKET_SIZE    8*1024
#define WMA_PREROLL_TIME  3000 /*unit ms*/

/*Wma file stream number*/
#define WMA_AUDIO_STREAM 1
#define WMA_VIDEO_STREAM 2

#define WMA_VID_I_FRM  0 
#define WMA_VID_P_FRM 1 

/*memory size needed by Wma packer*/
#define WMA_DATA_SIZE 16*1024
#define WMA_INDEX_SIZE 8*1024

/*memory size needed by Wma parser*/
#define WMA_VID_DATA_SIZE 256*1024 /*you can charge this define ,for performance*/
#define WMA_AUD_DATA_SIZE 64*1024

#define WMA_VID_CHECK_SIZE (Wma_VID_DATA_SIZE >> 1) 
#define WMA_AUD_CHECK_SIZE (Wma_AUD_DATA_SIZE >> 1)

#define HEADER_BASE_OBJECT 5


#define USE_LIBAVCODEC		1
#define FRAMERATE_MULTIPLE   1000

#define MP43_VOL_START_CODE	0x00000120

#define VID_PLAY_ERR015          0x4250    /* End of read data */

#define FOUR_CC( a, b, c, d )\
	( ((BLT_UInt32)a) | ( ((BLT_UInt32)b) << 8 )\         
				| ( ((BLT_UInt32)c) << 16 ) | ( ((BLT_UInt32)d) << 24 ) )

#define VID_UNKNOWN      0x0000

#define VID_WMV9         FOUR_CC('W','M','V','3')
#define VID_MP41         FOUR_CC('M','P','4','1')
#define VID_DIV1         FOUR_CC('D','I','V','1')
#define VID_MPG4         FOUR_CC('M','P','G','4')
#define VID_MP42         FOUR_CC('M','P','4','2')
#define VID_DIV2         FOUR_CC('D','I','V','2')
#define VID_MP43         FOUR_CC('M','P','4','3')
#define VID_DIV3         FOUR_CC('D','I','V','3')
#define VID_DIV4         FOUR_CC('D','I','V','4')
#define VID_DIV5         FOUR_CC('D','I','V','5')
#define VID_DIV6         FOUR_CC('D','I','V','6')
#define VID_AP41         FOUR_CC('A','P','4','1')
#define VID_COL1         FOUR_CC('C','O','L','1')
#define VID_DVX3         FOUR_CC('D','V','X','3')
#define VID_MPG3         FOUR_CC('M','P','G','3')
#define VID_DIVX3        (0x1111)
#define VID_WMV7         FOUR_CC('W','M','V','1')
#define VID_WMV8         FOUR_CC('W','M','V','2')


enum CodecID {
    CODEC_ID_NONE,

    /* video codecs */
    CODEC_ID_MPEG1VIDEO,
    CODEC_ID_MPEG2VIDEO,
    CODEC_ID_MPEG2VIDEO_XVMC,
    CODEC_ID_H261,
    CODEC_ID_H263,
    CODEC_ID_RV10,
    CODEC_ID_RV20,
    CODEC_ID_MJPEG,
    CODEC_ID_MJPEGB,
    CODEC_ID_LJPEG,
    CODEC_ID_SP5X,
    CODEC_ID_JPEGLS,
    CODEC_ID_MPEG4,
    CODEC_ID_RAWVIDEO,
    CODEC_ID_MSMPEG4V1,
    CODEC_ID_MSMPEG4V2,
    CODEC_ID_MSMPEG4V3,
    CODEC_ID_WMV1,
    CODEC_ID_WMV2,
    CODEC_ID_H263P,
    CODEC_ID_H263I,
    CODEC_ID_FLV1,
    CODEC_ID_SVQ1,
    CODEC_ID_SVQ3,
    CODEC_ID_DVVIDEO,
    CODEC_ID_HUFFYUV,
    CODEC_ID_CYUV,
    CODEC_ID_H264,
    CODEC_ID_INDEO3,
    CODEC_ID_VP3,
    CODEC_ID_THEORA,
    CODEC_ID_ASV1,
    CODEC_ID_ASV2,
    CODEC_ID_FFV1,
    CODEC_ID_4XM,
    CODEC_ID_VCR1,
    CODEC_ID_CLJR,
    CODEC_ID_MDEC,
    CODEC_ID_ROQ,
    CODEC_ID_INTERPLAY_VIDEO,
    CODEC_ID_XAN_WC3,
    CODEC_ID_XAN_WC4,
    CODEC_ID_RPZA,
    CODEC_ID_CINEPAK,
    CODEC_ID_WS_VQA,
    CODEC_ID_MSRLE,
    CODEC_ID_MSVIDEO1,
    CODEC_ID_IDCIN,
    CODEC_ID_8BPS,
    CODEC_ID_SMC,
    CODEC_ID_FLIC,
    CODEC_ID_TRUEMOTION1,
    CODEC_ID_VMDVIDEO,
    CODEC_ID_MSZH,
    CODEC_ID_ZLIB,
    CODEC_ID_QTRLE,
    CODEC_ID_SNOW,
    CODEC_ID_TSCC,
    CODEC_ID_ULTI,
    CODEC_ID_QDRAW,
    CODEC_ID_VIXL,
    CODEC_ID_QPEG,
    CODEC_ID_XVID,
    CODEC_ID_PNG,
    CODEC_ID_PPM,
    CODEC_ID_PBM,
    CODEC_ID_PGM,
    CODEC_ID_PGMYUV,
    CODEC_ID_PAM,
    CODEC_ID_FFVHUFF,
    CODEC_ID_RV30,
    CODEC_ID_RV40,
    CODEC_ID_VC1,
    CODEC_ID_WMV3,
    CODEC_ID_LOCO,
    CODEC_ID_WNV1,
    CODEC_ID_AASC,
    CODEC_ID_INDEO2,
    CODEC_ID_FRAPS,
    CODEC_ID_TRUEMOTION2,
    CODEC_ID_BMP,
    CODEC_ID_CSCD,
    CODEC_ID_MMVIDEO,
    CODEC_ID_ZMBV,
    CODEC_ID_AVS,
    CODEC_ID_SMACKVIDEO,
    CODEC_ID_NUV,
    CODEC_ID_KMVC,
    CODEC_ID_FLASHSV,
    CODEC_ID_CAVS,
    CODEC_ID_JPEG2000,
    CODEC_ID_VMNC,
    CODEC_ID_VP5,
    CODEC_ID_VP6,
    CODEC_ID_VP6F,
    CODEC_ID_TARGA,
    CODEC_ID_DSICINVIDEO,
    CODEC_ID_TIERTEXSEQVIDEO,
    CODEC_ID_TIFF,
    CODEC_ID_GIF,
    CODEC_ID_FFH264,
    CODEC_ID_DXA,
    CODEC_ID_DNXHD,
    CODEC_ID_THP,
    CODEC_ID_SGI,
    CODEC_ID_C93,
    CODEC_ID_BETHSOFTVID,
    CODEC_ID_PTX,
    CODEC_ID_TXD,
    CODEC_ID_VP6A,
    CODEC_ID_AMV,
    CODEC_ID_VB,
    CODEC_ID_PCX,
    CODEC_ID_SUNRAST,
    CODEC_ID_INDEO4,
    CODEC_ID_INDEO5,
    CODEC_ID_MIMIC,
    CODEC_ID_RL2,
    CODEC_ID_8SVX_EXP,
    CODEC_ID_8SVX_FIB,
    CODEC_ID_ESCAPE124,
    CODEC_ID_DIRAC,
    CODEC_ID_BFI,
    CODEC_ID_CMV,
    CODEC_ID_MOTIONPIXELS,
    CODEC_ID_TGV,
    CODEC_ID_TGQ,
    CODEC_ID_TQI,

    /* various PCM "codecs" */
    CODEC_ID_PCM_S16LE= 0x10000,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_PCM_U16LE,
    CODEC_ID_PCM_U16BE,
    CODEC_ID_PCM_S8,
    CODEC_ID_PCM_U8,
    CODEC_ID_PCM_MULAW,
    CODEC_ID_PCM_ALAW,
    CODEC_ID_PCM_S32LE,
    CODEC_ID_PCM_S32BE,
    CODEC_ID_PCM_U32LE,
    CODEC_ID_PCM_U32BE,
    CODEC_ID_PCM_S24LE,
    CODEC_ID_PCM_S24BE,
    CODEC_ID_PCM_U24LE,
    CODEC_ID_PCM_U24BE,
    CODEC_ID_PCM_S24DAUD,
    CODEC_ID_PCM_ZORK,
    CODEC_ID_PCM_S16LE_PLANAR,
    CODEC_ID_PCM_DVD,
    CODEC_ID_PCM_F32BE,
    CODEC_ID_PCM_F32LE,
    CODEC_ID_PCM_F64BE,
    CODEC_ID_PCM_F64LE,

    /* various ADPCM codecs */
    CODEC_ID_ADPCM_IMA_QT= 0x11000,
    CODEC_ID_ADPCM_IMA_WAV,
    CODEC_ID_ADPCM_IMA_DK3,
    CODEC_ID_ADPCM_IMA_DK4,
    CODEC_ID_ADPCM_IMA_WS,
    CODEC_ID_ADPCM_IMA_SMJPEG,
    CODEC_ID_ADPCM_MS,
    CODEC_ID_ADPCM_4XM,
    CODEC_ID_ADPCM_XA,
    CODEC_ID_ADPCM_ADX,
    CODEC_ID_ADPCM_EA,
    CODEC_ID_ADPCM_G726,
    CODEC_ID_ADPCM_CT,
    CODEC_ID_ADPCM_SWF,
    CODEC_ID_ADPCM_YAMAHA,
    CODEC_ID_ADPCM_SBPRO_4,
    CODEC_ID_ADPCM_SBPRO_3,
    CODEC_ID_ADPCM_SBPRO_2,
    CODEC_ID_ADPCM_THP,
    CODEC_ID_ADPCM_IMA_AMV,
    CODEC_ID_ADPCM_EA_R1,
    CODEC_ID_ADPCM_EA_R3,
    CODEC_ID_ADPCM_EA_R2,
    CODEC_ID_ADPCM_IMA_EA_SEAD,
    CODEC_ID_ADPCM_IMA_EA_EACS,
    CODEC_ID_ADPCM_EA_XAS,
    CODEC_ID_ADPCM_EA_MAXIS_XA,
    CODEC_ID_ADPCM_IMA_ISS,

    /* AMR */
    CODEC_ID_AMR_NB= 0x12000,
    CODEC_ID_AMR_WB,

    /* RealAudio codecs*/
    CODEC_ID_RA_144= 0x13000,
    CODEC_ID_RA_288,

    /* various DPCM codecs */
    CODEC_ID_ROQ_DPCM= 0x14000,
    CODEC_ID_INTERPLAY_DPCM,
    CODEC_ID_XAN_DPCM,
    CODEC_ID_SOL_DPCM,

    /* audio codecs */
    CODEC_ID_MP2= 0x15000,
    CODEC_ID_MP3, 
    CODEC_ID_AAC,
    CODEC_ID_AC3,
    CODEC_ID_DTS,
    CODEC_ID_VORBIS,
    CODEC_ID_DVAUDIO,
    CODEC_ID_WMAV1,
    CODEC_ID_WMAV2,
    CODEC_ID_MACE3,
    
    CODEC_ID_MACE6,
    CODEC_ID_VMDAUDIO,
    CODEC_ID_SONIC,
    CODEC_ID_SONIC_LS,
    CODEC_ID_FLAC,
    CODEC_ID_MP3ADU,
    CODEC_ID_MP3ON4,
    CODEC_ID_SHORTEN,
    CODEC_ID_ALAC,
    CODEC_ID_WESTWOOD_SND1,
    
    CODEC_ID_GSM, ///< as in Berlin toast format
    CODEC_ID_QDM2,
    CODEC_ID_COOK,
    CODEC_ID_TRUESPEECH,
    CODEC_ID_TTA,
    CODEC_ID_SMACKAUDIO,
    CODEC_ID_QCELP,
    CODEC_ID_WAVPACK,
    CODEC_ID_DSICINAUDIO,
    CODEC_ID_IMC,
    CODEC_ID_MUSEPACK7,
    CODEC_ID_MLP,
    CODEC_ID_GSM_MS, /* as found in WAV */
    CODEC_ID_ATRAC3,
    CODEC_ID_VOXWARE,
    CODEC_ID_APE,
    CODEC_ID_NELLYMOSER,
    CODEC_ID_MUSEPACK8,
    CODEC_ID_SPEEX,
    CODEC_ID_WMAVOICE,
    CODEC_ID_WMAPRO,
    CODEC_ID_WMALOSSLESS,
    CODEC_ID_ATRAC3P,
    CODEC_ID_EAC3,
    CODEC_ID_SIPR,
    CODEC_ID_MP1,
	CODEC_ID_AAC_PLUS, /* for gp */
	CODEC_ID_MP2A,/* for gp */
	CODEC_ID_MP4A,/* for gp */
	CODEC_ID_A52,/* for gp */

    /* subtitle codecs */
    CODEC_ID_DVD_SUBTITLE= 0x17000,
    CODEC_ID_DVB_SUBTITLE,
    CODEC_ID_TEXT,  ///< raw UTF-8 text
    CODEC_ID_XSUB,
    CODEC_ID_SSA,
    CODEC_ID_MOV_TEXT,

    /* other specific kind of codecs (generally used for attachments) */
    CODEC_ID_TTF= 0x18000,

    CODEC_ID_PROBE= 0x19000, ///< codec_id is not known (like CODEC_ID_NONE) but lavf should attempt to identify it

    CODEC_ID_MPEG2TS= 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                * stream (only used by libavformat) */
};

#define AUDIO_TYPE_NONE     CODEC_ID_NONE
#define AUDIO_TYPE_AC3      CODEC_ID_AC3
#define AUDIO_TYPE_A52      CODEC_ID_A52
#define AUDIO_TYPE_MP2      CODEC_ID_MP2
#define AUDIO_TYPE_MP3      CODEC_ID_MP3
#define AUDIO_TYPE_MP2A     CODEC_ID_MP2A
#define AUDIO_TYPE_MP4A     CODEC_ID_MP4A
#define AUDIO_TYPE_AAC      CODEC_ID_AAC
#define AUDIO_TYPE_AAC_PLUS CODEC_ID_AAC_PLUS
#define AUDIO_TYPE_AMR_NB   CODEC_ID_AMR_NB
#define AUDIO_TYPE_AMR_WB   CODEC_ID_AMR_WB
#define AUDIO_TYPE_FLAC     CODEC_ID_FLAC
#define AUDIO_TYPE_APE      CODEC_ID_APE
#define AUDIO_TYPE_COOK     CODEC_ID_COOK
#define AUDIO_TYPE_WMA      CODEC_ID_WMAV1
#define AUDIO_TYPE_WMA2     CODEC_ID_WMAV2
#define AUDIO_TYPE_WMAPRO   CODEC_ID_WMAPRO	
#define AUDIO_TYPE_PCM      CODEC_ID_PCM_S16LE 


/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
	/* WAVE format wFormatTag IDs */
#define AUD_FORMAT_UNKNOWN             0x0000 /* Microsoft Corporation */
#define AUD_FORMAT_PCM                 0x0001 /* Microsoft Corporation */
#define AUD_FORMAT_ADPCM               0x0002 /* Microsoft Corporation */
#define AUD_FORMAT_IEEE_FLOAT          0x0003 /* Microsoft Corporation */
#define AUD_FORMAT_ALAW                0x0006 /* Microsoft Corporation */
#define AUD_FORMAT_MULAW               0x0007 /* Microsoft Corporation */
#define AUD_FORMAT_DTS_MS              0x0008 /* Microsoft Corporation */
#define AUD_FORMAT_IMA_ADPCM           0x0011 /* Intel Corporation */
#define AUD_FORMAT_GSM610              0x0031 /* Microsoft Corporation */
#define AUD_FORMAT_MSNAUDIO            0x0032 /* Microsoft Corporation */
#define AUD_FORMAT_G726                0x0045 /* ITU-T standard  */
#define AUD_FORMAT_MPEG                0x0050 /* Microsoft Corporation */
#define AUD_FORMAT_MPEGLAYER3          0x0055 /* ISO/MPEG Layer3 Format Tag */
#define AUD_FORMAT_DOLBY_AC3_SPDIF     0x0092 /* Sonic Foundry */
	
#define AUD_FORMAT_A52                 0x2000
#define AUD_FORMAT_DTS                 0x2001
#define AUD_FORMAT_WMA1                0x0160
#define AUD_FORMAT_WMA2                0x0161
#define AUD_FORMAT_WMA3                0x0162
#define AUD_FORMAT_DIVIO_AAC           0x4143
#define AUD_FORMAT_AMR_CBR             0x7A21
#define AUD_FORMAT_AMR_VBR             0x7A22
	
/* Need to check these */
#define AUD_FORMAT_DK3                 0x0061
#define AUD_FORMAT_DK4                 0x0062

static const guid_t Wma_object_header_guid =
{0x75B22630, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t Wma_object_data_guid =
{0x75B22636, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t Wma_object_index_guid =
{0x33000890, 0xE5B1, 0x11CF, {0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB}};

static const guid_t Wma_object_file_properties_guid =
{0x8cabdca1, 0xa947, 0x11cf, {0x8e, 0xe4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t Wma_object_stream_properties_guid =
{0xB7DC0791, 0xA9B7, 0x11CF, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t Wma_object_content_description_guid =
{0x75B22633, 0x668E, 0x11CF, {0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c}};

static const guid_t Wma_object_header_extension_guid =
{0x5FBF03B5, 0xA92E, 0x11CF, {0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t Wma_object_metadata_guid =
{0xC5F8CBEA, 0x5BAF, 0x4877, {0x84, 0x67, 0xAA, 0x8C, 0x44, 0xFA, 0x4C, 0xCA}};

static const guid_t Wma_object_codec_list_guid =
{0x86D15240, 0x311D, 0x11D0, {0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

static const guid_t Wma_object_marker_guid =
{0xF487CD01, 0xA951, 0x11CF, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t Wma_object_stream_type_audio_guid =
{0xF8699E40, 0x5B4D, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const guid_t Wma_object_stream_type_video_guid =
{0xbc19efc0, 0x5B4D, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const guid_t Wma_object_stream_bitrate_properties_guid =
{0x7BF875CE, 0x468D, 0x11D1, { 0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2}};
static const guid_t Wma_object_extended_stream_properties =
{0x14E6A5CB, 0xC672, 0x4332, { 0x83, 0x99, 0xA9, 0x69, 0x52, 0x06, 0x5B, 0x5A}};
static const guid_t Wma_object_Content_Encryption_guid = 
{0x2211B3FB,0xBD23,0x11D2,{0xB4, 0xB7, 0x00, 0xA0, 0xC9, 0x55, 0xFC, 0x6E}};

static const guid_t Wma_object_stream_type_command_guid =
{0x59DACFC0, 0x59E6, 0x11D0, {0xA3, 0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};
static const guid_t Wma_object_reversed_1_guid =
{0xabd3d211,0xa9ba,0x11cf,{0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65}};
static const guid_t Wma_object_no_errcorrect_guid= 
{0x20fb5700,0x5b55,0x11cf,{0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b}};
static const guid_t Wma_object_audio_spread_obj_guid = 
{0xbfc3cd50,0x618f,0x11cf,{0x8b, 0xb2, 0x00, 0xaa, 0x00, 0xb4, 0xe2, 0x20}};
static const guid_t  Wma_object_codec_reserved_guid = {
0x86d15241,0x311d,0x11d0,{0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6}
};
static const guid_t Wma_object_padding_guid = {
0x1806d474,0xcadf,0x4509,{0xa4, 0xba, 0x9a, 0xab, 0xcb, 0x96, 0xaa, 0xe8}
};

static const guid_t Wma_object_file_guid = {
0x12345678,0x0000,0x1111,{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}
};

/*----------------------------------------------------------------------
|   Global 
+---------------------------------------------------------------------*/
WmaInfo_S 	g_stInfo;
BLT_UInt32  g_bFrame;
BLT_UInt32 	g_uiLastOffset;
BLT_UInt32 	g_uiPayLoadSize;
BLT_UInt8  	*g_pWmaHeaderData;
BLT_UInt8  	g_KeyFrame;

static codec_spec_data_t stAudCodec;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(WmaParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(WmaParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(WmaParser, ATX_Referenceable)

/*----------------------------------------------------------------------
|   WmaReInit
+---------------------------------------------------------------------*/
static void WmaReInit(WmaParser *pParser)
{
	
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, 0);

	//for Compressed payload
	ATX_SetMemory(&pParser->stAudObj.stCPayload, 0, sizeof(ComPressedPay_s));
	pParser->stAudObj.uiNowPayLoad = 0 ;
	pParser->stAudObj.uiNowPkt = 0 ;

	g_bFrame = 1;
}

/*----------------------------------------------------------------------
|   FlagBreakaway
+---------------------------------------------------------------------*/
static BLT_UInt32 FlagBreakaway( BLT_UInt32 uiFlag,	BLT_UInt32 uiMask)
{
	BLT_UInt8 uiMove = 0;
	BLT_UInt32 uiTemp;
	
	uiTemp = uiMask;
	
	if( uiTemp == 0 )
	{	/*the mask should been nonzero*/
		return 0;
	}
	while( ( uiTemp&0x01) == 0 )
	{	uiTemp = uiTemp >>1;
		uiMove ++;
	}
	return ( uiFlag&uiMask )>>uiMove;
}
/*----------------------------------------------------------------------
|   LengthConvert
+---------------------------------------------------------------------*/
static BLT_UInt8 LengthConvert( BLT_UInt32 uiLength )
{
	BLT_UInt8 uiLen = (BLT_UInt8)uiLength;
	if(uiLen == 3)
	{	
		uiLen = 4; 
	}
	return uiLen ;
}

/*----------------------------------------------------------------------
|   ParsePPI
+---------------------------------------------------------------------*/
static BLT_UInt8 ParsePPI(WmaParser* pParser, BLT_UInt32 ucStrNum,BLT_UInt32* pSendTime,BLT_UInt16* pDurTime)
{
	BLT_UInt8 	ucFlag; 
	BLT_UInt8 	ucSeqType;
	BLT_UInt8 	ucPadType;
	BLT_UInt8 	ucPackLenType;
	BLT_UInt8 	ucMultiPayload = 0;
	BLT_UInt32 	uiPackLen = 0;
	BLT_UInt32 	uiSeqLen = 0;
	BLT_UInt32	uiPadLen = 0;
	BLT_UInt32	bytes_read = 0;
	BLT_Result  result = BLT_SUCCESS;

	packet_ppi_t 	stParseInfo;
	PktParser_S* 	pPkt;

	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&stParseInfo, 2, &bytes_read);/*we just fetch first two bytes*/
	/*we parse length type flag*/
	ucFlag = stParseInfo.lenTypeFlags;
	ucMultiPayload = (BLT_UInt8)FlagBreakaway(ucFlag,0x01);
	ucSeqType = LengthConvert(FlagBreakaway(ucFlag,0x06));/* Sequence Type*/
	ucPadType = LengthConvert(FlagBreakaway(ucFlag,0x18));	/*Padding Length Type*/
	ucPackLenType = LengthConvert(FlagBreakaway(ucFlag,0x60));/*Packet Length Type*/
	/*we parse property flags*/
	ucFlag = stParseInfo.propertyFlags;
	


	
	if(ucStrNum == pParser->stAudObj.ucStrNum)
	{	

		pPkt = & (pParser->stAudObj);
	}
	pPkt->stPpi.ucRepType      = LengthConvert( FlagBreakaway( ucFlag,0x03));
	pPkt->stPpi.ucMediaOffType = LengthConvert( FlagBreakaway( ucFlag,0x03<<2 ));
	pPkt->stPpi.ucMediaObjType = LengthConvert( FlagBreakaway( ucFlag,0x03<<4));
	pPkt->stPpi.ucStrNrType    = LengthConvert( FlagBreakaway( ucFlag,0x03<<6));
	
	
	if (ucPackLenType == 0)
	{
		uiPackLen = pParser->stFile.uiPktSize;
	
	} 
	else
	{
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiPackLen, ucPackLenType, &bytes_read);

	}
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiSeqLen, ucSeqType, &bytes_read);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiPadLen, ucPadType, &bytes_read);
	
	
	if (uiPackLen < pParser->stFile.uiPktSize)
	{
		uiPadLen += pParser->stFile.uiPktSize - uiPackLen;
	
	}
	pPkt->stPpi.usPadLen = (BLT_UInt16)uiPadLen ;
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)pSendTime, 4, &bytes_read);
	result = ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)pDurTime, 2, &bytes_read) ;
	
	
	if(BLT_FAILED(result))
	{
		return (BLT_UInt8)result;	
	}
	if(ucMultiPayload == 1)
	{	

		/*Multi payload*/
		result = ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream,&ucFlag, 1, &bytes_read);

		if(BLT_FAILED(result))
		{
	
			return (BLT_UInt8)result;
		}
		pPkt->stPpi.ucPayType = LengthConvert( FlagBreakaway( ucFlag,0x03<<6));
		pPkt->uiPayLoadType = 1;
		pPkt->uiNowPayLoad = ucFlag&0x3F;
	
	}
	else
	{	/*single payload*/
		pPkt->uiPayLoadType = 0;
		pPkt->uiNowPayLoad = 1;

	}
	pPkt->uiNowPkt ++;


	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   ParseAudCodec
+---------------------------------------------------------------------*/
static BLT_UInt32 ParseAudCodec( codec_spec_data_t* pCodec )
{

	WmaInfo_S * pInfo = &g_stInfo;
	
	pInfo->uiAudType    = pCodec->wav.wFormatTag;
	pInfo->uiAudBit     = (BLT_UInt8)(pCodec->wav.wBitsPerSample);
	pInfo->uiAudBitRate = pCodec->wav.nAvgBytesPerSec*8;
	pInfo->uiAudChannel = (BLT_UInt8)(pCodec->wav.nChannels);
	pInfo->uiAudSR      = (BLT_UInt16)(pCodec->wav.nSamplesPerSec);
	pInfo->uiBlockAlign = pCodec->wav.nBlockAlign;
#if 0
	DEBUG0("***pInfo->uiAudType = %u***\n",pInfo->uiAudType);
	DEBUG0("***pInfo->uiAudBit = %02x***\n",pInfo->uiAudBit);
	DEBUG0("***pInfo->uiAudBitRate = %u***\n",pInfo->uiAudBitRate);
	DEBUG0("***pInfo->uiAudChannel = %02x***\n",pInfo->uiAudChannel);
	DEBUG0("***pInfo->uiAudSR = %u***\n",pInfo->uiAudSR);
	DEBUG0("***pInfo->uiBlockAlign = %u***\n",pInfo->uiBlockAlign);
	
	DEBUG0("***pInfo->uiAudType = %u****\n",pInfo->uiAudType);
	
	int i = 0;
	DEBUG0("**start**print memory cell*****\n");
	DEBUG0("sizeof(codec_spec_data_t) = %d\n",sizeof(codec_spec_data_t));
	for(i=0;i<sizeof(codec_spec_data_t);i++){
		if((i%8 == 0) && (i!=0)){
			DEBUG0("\n");	
		}
		DEBUG0("%02x ", ((BLT_UInt8 *)pCodec)[i]);
	}
	DEBUG0("\n");
	
	DEBUG0("**end**print memory cell*****\n");
#endif
	if (pInfo->uiAudType == 353 ) {/*WMA 2 channel*/
		BLT_UInt8 *pABuf;

		pABuf  = (BLT_UInt8 *)pCodec;
		pABuf += 18;
		
		pInfo->uiSamplesPerBlock  = *pABuf++;
		pInfo->uiSamplesPerBlock |= (*pABuf++) << 8;
		pInfo->uiSamplesPerBlock |= (*pABuf++) << 16;
		pInfo->uiSamplesPerBlock |= (*pABuf++) << 24;
		pInfo->uiEncodeOpt        = *pABuf++;
		pInfo->uiEncodeOpt       |= (*pABuf++) << 8;
			
	}

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PktHdrParser
+---------------------------------------------------------------------*/
static BLT_UInt32 PktHdrParser(WmaParser *pParser, BLT_UInt32 uiStreamNum, BLT_UInt32* pSendTime, BLT_UInt16* pDurTime)
{
	BLT_UInt8 	ucFlag;
	BLT_UInt32  bytes_read;
	BLT_Result	result= BLT_SUCCESS;
	ATX_Position  where;
	

	result = ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, &ucFlag, 1/*sizeof( BLT_UInt8)*/, &bytes_read);
	if( result != BLT_SUCCESS )
	{
		return result;
	}

	if( (ucFlag & 0x80) == 0x80 )
	{	/*we skip error conrection data field*/

		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream,&where);
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, (ucFlag & 0x0F) + (BLT_UInt32)where);
	}
	else
	{		

		/* fall back the pParser->stAudObj.pIdxStr,so we can retrieve the  entirely PPI structure*/
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream,&where);
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream,-1 + (BLT_UInt32)where);
	}
	
	result = ParsePPI(pParser,uiStreamNum,pSendTime,pDurTime);

	return result ;
}

/*----------------------------------------------------------------------
|   MP_TestObj
+---------------------------------------------------------------------*/
static BLT_UInt32 MP_TestObj(WmaParser* pParser, BLT_UInt8 ucStrNum, BLT_UInt32* pSkipSize, BLT_UInt32*pTime, BLT_UInt32*pSize, BLT_UInt8* pFrmType)
{
	
	BLT_UInt32 		uiSkipSize = 0;
	BLT_UInt32 		uiRepLen = 0;
	BLT_UInt32 		uiOffset = 0;
	BLT_UInt32 		uiPayLen = 0;
	ATX_Position 	where;
	BLT_Size		bytes_read;
	BLT_UInt8 		ucSubPayLen = 0;
	BLT_UInt8 		ucNum = 0;
	BLT_UInt8 		ucFlag;
	BLT_Result		result;
	PktParser_S*	pPkt;

	*pSize 		= 0;
	*pTime		= 0;
	*pFrmType 	= 1;

	if(ucStrNum == pParser->stAudObj.ucStrNum)
	{	
		pPkt = &( pParser->stAudObj);
	}	

	result	= ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
	if(BLT_FAILED(result))
	{
		return BLT_FAILURE;
	}
	uiSkipSize = (BLT_UInt32)where;

	if (pPkt->stCPayload.uiCurCPayNum > 0)
	{//for Compressed payload
		pPkt->stCPayload.uiReGetPay = 1;
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&ucSubPayLen, 1, &bytes_read);
		*pSize = ucSubPayLen;
		pPkt->stCPayload.uiSendTime += pPkt->stCPayload.uiTimeFlag;
		*pTime = pPkt->stCPayload.uiSendTime;

		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);

		uiSkipSize =  (BLT_UInt32)where- uiSkipSize;
		
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
		
		return BLT_SUCCESS;
	}
	
	//for Compressed payload
	ATX_SetMemory(&pPkt->stCPayload, 0, sizeof(ComPressedPay_s));
	
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, &ucFlag, 1, &bytes_read);
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, pPkt->stPpi.ucMediaObjType + (BLT_UInt32)where);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiOffset, pPkt->stPpi.ucMediaOffType, &bytes_read);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiRepLen, pPkt->stPpi.ucRepType, &bytes_read);
	
	if(uiRepLen == 1 && ( (ucFlag & 0x7F) == ucStrNum ))
	{//for Compressed payload
		pPkt->stCPayload.uiSendTime = uiOffset;
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&pPkt->stCPayload.uiTimeFlag, 1, &bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiPayLen, pPkt->stPpi.ucPayType, &bytes_read);
		do
		{
			ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&ucSubPayLen, 1, &bytes_read);

			if (ucSubPayLen > 0)
			{
				ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
				ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, ucSubPayLen + (BLT_UInt32)where);
				ucNum++;
			}

			if (uiPayLen < ucSubPayLen)
			{
				ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
				uiSkipSize = (BLT_UInt32)where - uiSkipSize;
				ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
				return BLT_FAILURE;
			} 
			else
			{
				if (*pSize == 0)
				{
					*pSize = ucSubPayLen;
				}
				uiPayLen -= (ucSubPayLen + 1);
			}

			if (uiPayLen == 0)
			{
				break;
			}
			else if (ucNum == 255)
			{
				return BLT_FAILURE;
			}
		}while(ucNum < 255);
		
		pPkt->stCPayload.uiCurCPayNum = ucNum;
		pPkt->uiNowPayLoad += (pPkt->stCPayload.uiCurCPayNum - 1);
		*pTime = pPkt->stCPayload.uiSendTime;

		if((ucFlag&0x80)>>7)
		{
			*pFrmType = 0 ;
		}
		else
		{
			*pFrmType = 1 ;
		}
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream , &where); 
		uiSkipSize = (BLT_UInt32)where - uiSkipSize;
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);

		return BLT_SUCCESS;
	}

	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)pSize, 4, &bytes_read);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)pTime, 4, &bytes_read);	
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream , &where); 
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiRepLen - 8 + (BLT_UInt32)where);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiPayLen, pPkt->stPpi.ucPayType, &bytes_read);
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
	uiSkipSize = (BLT_UInt32)where - uiSkipSize;
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
	*pSkipSize = uiSkipSize + uiPayLen;

	if( ( uiOffset == 0) &&( (ucFlag&0x7F) == ucStrNum )&&(uiRepLen != 1))
	{	
		if((ucFlag&0x80)>>7)
		{
			*pFrmType = 0 ;
		}
		else
		{
			*pFrmType = 1 ;
		}
		return BLT_SUCCESS;
	}
	else
	{	
		return BLT_FAILURE;
	}	
}
/*----------------------------------------------------------------------
|   SP_TestObj
+---------------------------------------------------------------------*/
BLT_UInt32 SP_TestObj(WmaParser* pParser, BLT_UInt8 ucStrNum, BLT_UInt32* pTime, BLT_UInt32* pSize, BLT_UInt8*  pFrmType)
{

	
	BLT_UInt8 	ucFlag;
	BLT_UInt8	ucNum;
	BLT_UInt8	ucSubPayLen;
	
	BLT_UInt32 	uiSkipSize;
	BLT_UInt32	uiPayLen;
	BLT_UInt32 	uiMediaOffset = 0;
	BLT_UInt32	uiRepLen = 0;
	ATX_Position  where;
	BLT_UInt32  bytes_read;
	
	BLT_Result	result;
	PktParser_S *pPkt;

	*pSize 		= 0;
	*pTime 		= 0;
	*pFrmType 	= 1;

	if( ucStrNum == pParser->stAudObj.ucStrNum )
	{	
		pPkt = &( pParser->stAudObj) ;
	}
	
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
	uiSkipSize = (BLT_UInt32)where; /*store the stream pos*/

	if (pPkt->stCPayload.uiCurCPayNum > 0)
	{//for Compressed payload
		pPkt->stCPayload.uiReGetPay = 1;
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&ucSubPayLen, 1, &bytes_read);
		*pSize = ucSubPayLen;
		pPkt->stCPayload.uiSendTime += pPkt->stCPayload.uiTimeFlag;
		*pTime = pPkt->stCPayload.uiSendTime;
		
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
		uiSkipSize = (BLT_UInt32)where - uiSkipSize;
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
		
		return BLT_SUCCESS;
	}

	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, &ucFlag, 1, &bytes_read);

	if((ucFlag & 0x7F) == ucStrNum )
	{
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, pPkt->stPpi.ucMediaObjType + (BLT_UInt32)where);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiMediaOffset,pPkt->stPpi.ucMediaOffType, &bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&uiRepLen, pPkt->stPpi.ucRepType, &bytes_read);
		
		if(uiRepLen == 1)
		{//for Compressed payload			
			ucNum = 0;
			uiPayLen = pParser->stFile.uiPktSize - pPkt->stPpi.usPadLen - 21;
			pPkt->stCPayload.uiSendTime = uiMediaOffset;
			ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&pPkt->stCPayload.uiTimeFlag, 1, &bytes_read);			
			do
			{
				ucSubPayLen = 0;
				ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)&ucSubPayLen, 1, &bytes_read);
				
				if (ucSubPayLen > 0)
				{
					ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
					ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, ucSubPayLen + (BLT_UInt32)where);
					ucNum++;
				}
				
				if (uiPayLen < ucSubPayLen)
				{
					ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
					uiSkipSize = (BLT_UInt32)where - uiSkipSize;
					ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
					return BLT_FAILURE;
				} 
				else
				{
					if (*pSize == 0)
					{
						*pSize = ucSubPayLen;
					}
					uiPayLen -= (ucSubPayLen + 1);
				}
				
				if (uiPayLen == 0)
				{
					break;
				}
				else if (ucNum == 255)
				{
					return BLT_FAILURE;
				}
			}while(ucNum < 255);
			
			pPkt->stCPayload.uiCurCPayNum = ucNum;
			pPkt->uiNowPayLoad += (pPkt->stCPayload.uiCurCPayNum - 1);
			*pTime = pPkt->stCPayload.uiSendTime;
			
			if((ucFlag&0x80)>>7)
			{
				*pFrmType = 0 ;
			}
			else
			{
				*pFrmType = 1 ;
		}
			ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
			uiSkipSize =  - uiSkipSize - 8;			
			ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
			
			return BLT_SUCCESS;
		}
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)pSize, 4, &bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, (BLT_UInt8*)pTime, 4, &bytes_read);
		
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
		uiSkipSize = (BLT_UInt32)where - uiSkipSize;
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, -uiSkipSize + (BLT_UInt32)where);
		
		if( (uiMediaOffset == 0)&&(uiRepLen != 1))
		{
			if((ucFlag&0x80)>>7)
			{
				*pFrmType = 0 ;
			}
			else
			{
				*pFrmType = 1 ;
			}
			return BLT_SUCCESS;
		}
		else
		{	
			return BLT_FAILURE;
		}
	}
	else
	{	
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream,-1 + (BLT_UInt32)where);
		return BLT_FAILURE;
	}
}

/*----------------------------------------------------------------------
|   WmaAudNextGet
+---------------------------------------------------------------------*/
static BLT_UInt32 WmaAudNextGet(WmaParser* pParser, BLT_UInt32* size, BLT_UInt32* time)
{

#if 0
	DEBUG0("*** check pParser ****\n");
	int i = 0;
		DEBUG0("\n");
	for(i=0;i<sizeof(WmaParser); i++){
		if(i%8 == 0 &&i != 0){
				DEBUG0("\n");
		}

		DEBUG0("%02x ", ((BLT_UInt8 *)pParser)[i]);
	}
	priDEBUG0ntf("\n");
	
	DEBUG0("*** end check pParser ****\n");
#endif
	
	
	BLT_UInt32 		uiNextPos;
	BLT_UInt32		uiSendTime;
	BLT_UInt32		uiSkipSize;
	BLT_UInt32		uiTime;
	BLT_UInt32		uiSize;
	BLT_UInt16 		uiDurTime;
	BLT_UInt8 		ucStrNum,ucFrmType;
	BLT_UInt32 		uiSeekFlag =0;
	ATX_Position 	where;
	BLT_Result 		result = BLT_SUCCESS;
	PktParser_S* 	pPkt = &(pParser->stAudObj);
	
	uiSize = 0;
	uiTime = 0;
	ucFrmType = 0;
	ucStrNum = pPkt->ucStrNum;
	
	while( uiSeekFlag == 0 )
	{	

		if( pPkt->uiNowPayLoad != 0 )
		{

			if(pPkt->uiPayLoadType == 1)
			{/*multi payload*/

				result =MP_TestObj(pParser,ucStrNum,&uiSkipSize,&uiTime,&uiSize,&ucFrmType);
				if(BLT_FAILED(result))
				{
					pPkt->uiNowPayLoad--;
					ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
					ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiSkipSize + (BLT_UInt32)where);
				}
				else if(BLT_SUCCEEDED(result))
				{
					uiSeekFlag = 1;
				}
				else
				{
					return result;
				}
			}
			else
			{/*single payload*/

				result =SP_TestObj(pParser,ucStrNum,&uiTime,&uiSize,&ucFrmType);
				if(BLT_FAILED(result))
				{
					pPkt->uiNowPayLoad--;
				}
				else if(BLT_SUCCEEDED(result))
				{
					uiSeekFlag = 1;
				}
				else
				{
					return result;
				}
			}
		}
		else
		{	//Seek to next Packet;

			
			
			if(pPkt->uiNowPkt >= pParser->stFile.uiPktNum )
			{
				/*reach the end of file*/
				DEBUG0("pPkt->uiNowPkt = %d\n", pPkt->uiNowPkt);
				DEBUG0("pParser->stFile.uiPktNum = %d\n", pParser->stFile.uiPktNum);
			//	while(1);

				return VID_PLAY_ERR015;
			}
			uiNextPos = pParser->stFile.uiPktSize * pPkt->uiNowPkt;

			ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
			uiNextPos -= (BLT_UInt32)where;
			

			result = ATX_InputStream_Seek( pParser->stAudObj.pIdxStr->stream, uiNextPos + (BLT_UInt32)where);
			if(BLT_FAILED(result))	
			{
				return result;
			}

			result = PktHdrParser(pParser,ucStrNum,&uiSendTime,&uiDurTime);

			if(BLT_FAILED(result))	
			{
				return result;
			}
		}
	}
	*size = uiSize ;
	*time = uiTime - pParser->stFile.uiPreroll;
	pPkt->uiObjSize = uiSize;
	pParser->stPlayer.uiAudms = uiTime;


	
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmaInfoOut
+---------------------------------------------------------------------*/ 
static BLT_UInt32 WmaInfoOut(WmaParser* pParser, ContainBitsInfo_t * pVidCa)
{
	BLT_Result 	result = BLT_SUCCESS;
	BLT_UInt32  ui32Size; 
	BLT_UInt32	ui32Time; 
	BLT_UInt32	ui32FType;
	BLT_UInt8   *pVData;
	WmaInfo_S*  pInfo = &g_stInfo;

	pVData = NULL;
	ui32Size = 0;
	ui32Time = 0;
	ui32FType = 0;
	


	ATX_SetMemory( ( void* )pVidCa, 0, sizeof( ContainBitsInfo_t ));
	pVidCa->bSeekable = pInfo->uiSeekable ;

		

	if( pInfo->uiAudType != AUD_FORMAT_UNKNOWN )
	{

		result = WmaAudNextGet(pParser, &ui32Size, &ui32Time);
		
		if (BLT_FAILED(result)) {
			pVidCa->audType = AUDIO_TYPE_NONE;
		}
		else
		{
#ifdef USE_LIBAVCODEC

			pVidCa->audExtraLen = pInfo->audExtraLen;
			pVidCa->pAudExtraData = pInfo->pAudExtraData;


			
#endif
			switch( pInfo->uiAudType )
			{
#ifdef USE_LIBAVCODEC

            case AUD_FORMAT_WMA1:

                pVidCa->audType = CODEC_ID_WMAV1;
				pVidCa->uiBlockAlign = pInfo->uiBlockAlign;
                break;
            case AUD_FORMAT_WMA2:

                pVidCa->audType = CODEC_ID_WMAV2;
				pVidCa->uiBlockAlign = pInfo->uiBlockAlign;
                break;
            case AUD_FORMAT_WMA3:

                pVidCa->audType = CODEC_ID_WMAPRO;
				pVidCa->uiBlockAlign = pInfo->uiBlockAlign;
               break;
#else
			case AUD_FORMAT_WMA1:
			case AUD_FORMAT_WMA2:
			case AUD_FORMAT_WMA3:
				pVidCa->audType = AUDIO_TYPE_WMA;
				pVidCa->uiBlockAlign = pInfo->uiBlockAlign;
				//pVidCa->audStreamNum = pInfo->audStreamNum;
				
				break;
#endif
			case AUD_FORMAT_IMA_ADPCM :
#ifdef USE_LIBAVCODEC

				if (pInfo->uiAudBit == 8){
					pVidCa->audType = CODEC_ID_PCM_ZORK;
}
				else
				{					pVidCa->audType = CODEC_ID_ADPCM_IMA_WAV;}
#else
				pVidCa->audType = AUDIO_TYPE_ADPCM;
#endif
				pVidCa->uiBlockAlign=pInfo->uiBlockAlign;			
				break;
			case AUD_FORMAT_MPEGLAYER3 :

				pVidCa->audType = AUDIO_TYPE_MP3;			
				break;
			case AUD_FORMAT_PCM :

				pVidCa->audType = AUDIO_TYPE_PCM ;			
				break;
			case AUD_FORMAT_AMR_VBR :

				pVidCa->audType = AUDIO_TYPE_AMR_NB ;			
				break;
			default :

				pVidCa->audType = AUDIO_TYPE_NONE;			
				break;
			}

			if (pInfo->uiAudBitRate > 0)
			{
				pVidCa->audBits = pInfo->uiAudBit;
				pVidCa->audBitrate = pInfo->uiAudBitRate;
				pVidCa->audChannel = pInfo->uiAudChannel;
				pVidCa->audLen = pInfo->uiAudLen;
				pVidCa->audSR = (BLT_UInt16)pInfo->uiAudSR;
				pVidCa->isVBR = pInfo->isVBR;

			}		
			else
			{
				pVidCa->audType = AUDIO_TYPE_NONE;
		
			}
		}		
	}
	else 
	{
		pVidCa->audType = AUDIO_TYPE_NONE ;

	}

	return BLT_SUCCESS ;
}


/*----------------------------------------------------------------------
|   int32_swap
+---------------------------------------------------------------------*/

static BLT_UInt32 int32_swap(BLT_UInt32 val)
{
	return (((val & 0xff) << 24) | ((val & 0xff00) << 8) | ((val >> 8) & 0xff00) | ((val >> 24) & 0xff));
}

/*----------------------------------------------------------------------
|   Wma_CmpGUID
+---------------------------------------------------------------------*/
static BLT_UInt32 Wma_CmpGUID( const guid_t *pGuid1, const guid_t *pGuid2 )
{
	if( (pGuid1->v1 != pGuid2->v1 )||
		(pGuid1->v2 != pGuid2->v2 )||
		(pGuid1->v3 != pGuid2->v3 )||
		(ATX_CompareMemory( pGuid1->v4,  pGuid2->v4, 8)) )
	{
		return BLT_FAILURE;/*not match*/
	}
	return BLT_SUCCESS; /* match */
}
/*----------------------------------------------------------------------
|   ComPareGUID
+---------------------------------------------------------------------*/
static BLT_UInt32 ComPareGUID(guid_t* pGuid)
{	
	if(Wma_CmpGUID(pGuid, &Wma_object_header_guid) == BLT_SUCCESS)
	{
		return OBJ_HDR_ID;
	}
	else if(Wma_CmpGUID(pGuid, &Wma_object_data_guid) == BLT_SUCCESS)
	{
		return OBJ_DATA_ID;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_index_guid) == BLT_SUCCESS)
	{
		return OBJ_IDX_ID ;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_file_properties_guid) == BLT_SUCCESS)
	{
		return FILE_PRO_ID;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_stream_properties_guid) == BLT_SUCCESS)
	{
		return STREAM_PRO_ID;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_content_description_guid) == BLT_SUCCESS)
	{
		return CONTENT_DEC_ID;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_header_extension_guid) == BLT_SUCCESS )
	{
		return HDR_EXT_ID ;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_codec_list_guid) == BLT_SUCCESS)
	{
		return CODEC_LIST_ID ;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_stream_bitrate_properties_guid) == BLT_SUCCESS)
	{
		return STR_BIT_PROPER_ID;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_padding_guid) == BLT_SUCCESS)
	{
		return PADDING_ID ;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_extended_stream_properties) == BLT_SUCCESS)
	{
		return EXT_STREAM_PRO_ID ;
	}
	else if (Wma_CmpGUID(pGuid, &Wma_object_Content_Encryption_guid) == BLT_SUCCESS)
	{
		return CONTENT_ENCRYPTION_ID ;
	}	
	else 
	{
		return UNKNOW_ID ;
	}		
}
/*----------------------------------------------------------------------
|   WmaHdrParse
+---------------------------------------------------------------------*/
static BLT_UInt32  WmaHdrParse(WmaParser* pParser, ATX_LargeSize uiFileSize)
 {
	WmaInfo_S* pInfo = &g_stInfo;
	Wma_object_common_t     stCom;
	Wma_object_common_t     stExtCom;
	Wma_object_t 			stUn;
	sequence_layer_data_t  	stSLD;
	MP43VideoObjectLayer_s  stDIVX3;
	WMV78VideoHeader_s      stWMV7Video;
	Wma_object_extended_stream_properties_t  stExtStrPro[2];
	guid_t 		stMediaType;
	BLT_UInt32 	uiEndSize;
	BLT_UInt32 	uiIDType;
	BLT_UInt32 	uiFlag;
	BLT_UInt32 	uiSeekPos;
	BLT_UInt16	usBPFlags = 0;
	BLT_UInt32 	uiaveragebitrate = 0;
	BLT_UInt32 	uiSeekedSize = 0;
	BLT_UInt32 	uiExtSeekedSize = 0;
	BLT_UInt32 	uiLastPassedSize = 0xFFFFFFFF;
	BLT_UInt32 	uiExtLastPassedSize = 0xFFFFFFFF;
	BLT_Result  result = BLT_SUCCESS;
	BLT_UInt64 	ui64Duration = 0;
	BLT_UInt8 	uiStop = 0 ;
	BLT_UInt8 	uiHasData = 0 ;
	BLT_UInt8 	uiHasIdx = 0 ;
	BLT_UInt8 	ucExtStop = 0;
	BLT_UInt8 	ucExtStrNum = 0;
	BLT_Size	bytes_read;
	ATX_Position	where;



	pInfo->vidExtraLen = 0;
	pInfo->pVidExtraData = NULL;
	pInfo->audExtraLen = 0;
	pInfo->pAudExtraData = NULL;
	pInfo->vidStreamNum = 0;
	pInfo->audStreamNum = 0;
	pInfo->isVBR = 2; /*Unknown*/
	g_pWmaHeaderData = 0;
		
	pParser->stAudObj.ucStrNum = 0;
	
	ATX_SetMemory(&stUn, 0, sizeof(Wma_object_t));
	ATX_SetMemory(&stSLD, 0, sizeof(sequence_layer_data_t));
	ATX_SetMemory(&stDIVX3, 0, sizeof(MP43VideoObjectLayer_s));
	ATX_SetMemory(&stWMV7Video, 0, sizeof(WMV78VideoHeader_s));
	ATX_SetMemory(&stWMV7Video, 0, sizeof(WMV78VideoHeader_s));
	ATX_SetMemory(&stExtStrPro[0], 0, OBJ_EX_STREAM_PROPER_SIZE/*sizeof(Wma_object_extended_stream_properties_t)*/);
	ATX_SetMemory(&stExtStrPro[1], 0, OBJ_EX_STREAM_PROPER_SIZE/*sizeof(Wma_object_extended_stream_properties_t)*/);
	
	stSLD.uiFlag1 = 0x00000004;
	stSLD.uiFlag2 = 0x0000000C;
		
	uiEndSize = uiFileSize;/*the whole Wma file size*/
	 
	 while(uiSeekedSize < uiEndSize)
	 {
	 	if(uiLastPassedSize != uiSeekedSize)
		{
			uiLastPassedSize = uiSeekedSize;
		}
		else
		{
			uiStop = 1 ;
			DEBUG0("[WmaHdrParse] break from a loop,the file is not unabridged\n");
			break ;
		}
	
	#if 0
		char buf[4096];
         int i = 0;
         DEBUG0("\n");
        ATX_InputStream_Read(pParser->input.stream, buf, sizeof(buf), &bytes_read);
         for(i=0;i<4096;i++){ 
            if((i%16 == 0) && (i !=0)){
                    DEBUG0("\n");
            }
            DEBUG0("%02x ", buf[i]);

         }
          DEBUG0("\n");
	#endif
	#if 0	
		ATX_SetMemory(&where, 0, sizeof(ATX_Position));
		DEBUG0("where = %llu\n",where);
		ATX_InputStream_Tell(pParser->input.stream, &where);
		
		char buf[4096];
         int i = 0;
         DEBUG0("\n");
		

		result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8*) &stCom, sizeof(stCom), &bytes_read);
		for(i=0;i<bytes_read;i++){ 
            if((i%8 == 0) && (i !=0)){
                    DEBUG0("\n");
            }
            DEBUG0("%02x ", ((BLT_UInt8*) &stCom)[i]);

         }
         //while(1);
		
		
		ATX_SetMemory(&where, 0, sizeof(where));
		ATX_InputStream_Tell(pParser->input.stream, &where);
		DEBUG0("where = %llu\n",where);
		DEBUG0("sizeof(stCom) = %0x\n",sizeof(stCom));
		DEBUG0("bytes_read = %0x\n",bytes_read);
		DEBUG0("\n");
		DEBUG0("**** print first read data*******\n");
		DEBUG0("stCom.i_object_id.v1 = %0x\n",		stCom.i_object_id.v1);
		DEBUG0("stCom.i_object_id.v2 = %0x\n",		stCom.i_object_id.v2);
		DEBUG0("stCom.i_object_id.v3 = %0x\n",		stCom.i_object_id.v3);
		DEBUG0("(stCom.i_object_id.v4)[0] = %0x\n",(stCom.i_object_id.v4)[0]);
		DEBUG0("(stCom.i_object_id.v4)[1] = %0x\n",(stCom.i_object_id.v4)[1]);
		DEBUG0("(stCom.i_object_id.v4)[2] = %0x\n",(stCom.i_object_id.v4)[2]);
		DEBUG0("(stCom.i_object_id.v4)[3] = %0x\n",(stCom.i_object_id.v4)[3]);
		DEBUG0("(stCom.i_object_id.v4)[4] = %0x\n",(stCom.i_object_id.v4)[4]);
		DEBUG0("(stCom.i_object_id.v4)[5] = %0x\n",(stCom.i_object_id.v4)[5]);
		DEBUG0("(stCom.i_object_id.v4)[6] = %0x\n",(stCom.i_object_id.v4)[6]);
		DEBUG0("(stCom.i_object_id.v4)[7] = %0x\n",(stCom.i_object_id.v4)[7]);
		DEBUG0("(stCom.i_object_size) = %llu\n", stCom.i_object_size);
		DEBUG0("\n");

	#endif
	
	
		result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8*) &stCom, sizeof(stCom), &bytes_read);                                                                                                         
		/*比较读到的ID*/
		uiIDType = ComPareGUID( &(stCom.i_object_id) );

		/*根据不同的ID做不同的动作*/
		switch( uiIDType )
		{
		case OBJ_HDR_ID :
	
			result = ATX_InputStream_Read(pParser->input.stream,(BLT_UInt8*)&(stUn.un.header),OBJ_HEADER_SIZE/*sizeof( Wma_object_header_t )*/,&bytes_read);
			if (BLT_FAILED(result)) {
				break;
			}
			uiSeekedSize += OBJ_HEADER_SIZE/*sizeof( Wma_object_header_t )*/ + 24;

			
			
			break;
			
		case OBJ_DATA_ID :

			result = ATX_InputStream_Read(pParser->input.stream,(BLT_UInt8*)&(stUn.un.data),OBJ_DATA_SIZE/*sizeof( Wma_object_data_t )*/, &bytes_read);
			if (BLT_FAILED(result)) {
				break;
			}
			result = ATX_InputStream_Tell(pParser->input.stream, &where);
			if (BLT_FAILED(result)) {
				break;
			}
			pParser->uiDataPos = (BLT_UInt32)where;/*data packet start postion  in file */

			
			
			uiSeekPos =(BLT_UInt32)stCom.i_object_size - 24 - OBJ_DATA_SIZE/*sizeof(Wma_object_data_t)*/; 

			result = ATX_InputStream_Seek(pParser->input.stream,uiSeekPos + (BLT_UInt32)where);
			
			uiHasData = 1;
			
			pParser->stFile.uiPktNum = ( BLT_UInt32 )stUn.un.data.i_total_data_packets;
			
		

			uiSeekedSize += (BLT_UInt32)stCom.i_object_size ;
			if (BLT_FAILED(result)) {
				result = BLT_SUCCESS ;
				uiStop = 1 ; /*you can play the file ,enven it is broken*/
				break;
			}

			break;
		case OBJ_IDX_ID :

			result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8*)&(stUn.un.index), OBJ_INDEX_SIZE/*sizeof( Wma_object_index_t )*/,
																			&bytes_read);
			if (BLT_FAILED(result)) {
				break;
			}
			/* the simple index entry start addr in the Wma simple index object */
			result = ATX_InputStream_Tell(pParser->input.stream, &where);
			if (BLT_FAILED(result)) {
				break;
			}
			pParser->uiIdxPos = (BLT_UInt32)where; 

			
			uiSeekPos = (BLT_UInt32)stCom.i_object_size - 24 - OBJ_INDEX_SIZE/*sizeof(Wma_object_index_t)*/;


			result = ATX_InputStream_Seek(pParser->input.stream,uiSeekPos + (BLT_UInt32)where);
			if (BLT_FAILED(result)) {	
				result = BLT_SUCCESS;
				uiStop = 1;
				break; 
			}
			uiHasIdx = 1;
			/*the number of index entry in the simple index object*/
			pParser->stFile.uiIDXNum = stUn.un.index.i_index_entry_count;
			pParser->stFile.uiTimeInterval = (BLT_UInt32)(stUn.un.index.i_index_entry_time_interval/10000);/*100ns to ms */
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;

			
			break;
			
		case HDR_EXT_ID :
	
			uiExtSeekedSize = 22;
	
			result = ATX_InputStream_Tell(pParser->input.stream, &where);
			if (BLT_FAILED(result)) {

				break;
			}
			result = ATX_InputStream_Seek(pParser->input.stream, uiExtSeekedSize + (BLT_UInt32)where);
			
	
			
			while (uiExtSeekedSize < (BLT_UInt32)(stCom.i_object_size - 24))
			{

				if(uiExtLastPassedSize != uiExtSeekedSize)
				{
					uiExtLastPassedSize = uiExtSeekedSize;
	
				}
				else
				{
					ucExtStop = 1 ;

					break ;
				}
				result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8*)&stExtCom, sizeof( stExtCom), &bytes_read);
				if (BLT_FAILED(result)) 
				{
					break;
				}
				
				/*获得ext id，进行比较*/
				uiIDType = ComPareGUID( &(stExtCom.i_object_id) );


				switch(uiIDType)
				{
				case EXT_STREAM_PRO_ID:

					if (ucExtStrNum > 1)
					{
						uiExtSeekedSize += (BLT_UInt32)stExtCom.i_object_size;
						if( (uiExtSeekedSize <= (BLT_UInt32)(stCom.i_object_size - 24))&&(stExtCom.i_object_size != 0) )
						{
							result = ATX_InputStream_Tell(pParser->input.stream, &where);
							if (BLT_FAILED(result)) 
							{
								break;
							}
							result = ATX_InputStream_Seek(pParser->input.stream,(BLT_UInt32)stExtCom.i_object_size -24 + (BLT_UInt32)where);
							if (BLT_FAILED(result)) 
							{
								break;
							}
						}
						else
						{
							ucExtStop = 1;
						}

						break;
					}
					result = ATX_InputStream_Read(pParser->input.stream,
				                (BLT_UInt8*)&stExtStrPro[ucExtStrNum],
							    OBJ_EX_STREAM_PROPER_SIZE/*sizeof(Wma_object_extended_stream_properties_t)*/,
								&bytes_read
							  );
					if (BLT_FAILED(result)) 
					{
						break;
					}
					else if (ucExtStrNum < 2)
					{
						ucExtStrNum++;
					}
					
					result = ATX_InputStream_Tell(pParser->input.stream, &where);
					if (BLT_FAILED(result)) 
					{
						break;
					}
					result = ATX_InputStream_Seek(pParser->input.stream,
						              (BLT_UInt32)stExtCom.i_object_size - 24 -OBJ_EX_STREAM_PROPER_SIZE/*sizeof(Wma_object_extended_stream_properties_t)*/ + (BLT_UInt32)where);
					if (BLT_FAILED(result)) 
					{
						break;
					}
					uiExtSeekedSize += (BLT_UInt32)stExtCom.i_object_size;
					break;
				case PADDING_ID:

					result = ATX_InputStream_Tell(pParser->input.stream, &where);
					if (BLT_FAILED(result)) 
					{
						break;
					}
					result = ATX_InputStream_Seek(pParser->input.stream,(BLT_UInt32)stExtCom.i_object_size - 24 + (BLT_UInt32)where);
					if (BLT_FAILED(result)) 
					{
						break;
					}
					uiExtSeekedSize += (BLT_UInt32)stExtCom.i_object_size ;	
					break;
				case UNKNOW_ID:

					uiExtSeekedSize += (BLT_UInt32)stExtCom.i_object_size;
					if( (uiExtSeekedSize <= (BLT_UInt32)(stCom.i_object_size - 24))&&(stExtCom.i_object_size != 0) )
					{
						result = ATX_InputStream_Tell(pParser->input.stream, &where);
						if (BLT_FAILED(result)) 
						{
							break;
						}
						result = ATX_InputStream_Seek(pParser->input.stream,(BLT_UInt32)stExtCom.i_object_size -24 + (BLT_UInt32)where);
						if (BLT_FAILED(result)) 
						{
							break;
						}
					}
					else
					{
						ucExtStop = 1;
					}
				    break;
				default:
				    break;
				}
				
				if (BLT_FAILED(result)) 
				{
					return result;
				}
				
				if( ucExtStop == 1)
				{	/*the file format maybe not right*/
					break;
				}
			}
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;
		
			break; 
		case CODEC_LIST_ID :

			result = ATX_InputStream_Tell(pParser->input.stream, &where);
			if (BLT_FAILED(result)) 
			{
				break;
			}		
			result = ATX_InputStream_Seek(pParser->input.stream,(BLT_UInt32)stCom.i_object_size - 24 + (BLT_UInt32)where);
			if (BLT_FAILED(result)) 
			{
				break;
			}	
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;
	
			break;
		case STREAM_PRO_ID :

			result = ATX_InputStream_Read(pParser->input.stream,
								(BLT_UInt8*)&(stUn.un.stream_properties),
								OBJ_STREAM_PROPER_SIZE/*sizeof(Wma_object_stream_properties_t)*/,
								&bytes_read
								);
			if (BLT_FAILED(result)) 
			{
				break;
			}	
			stMediaType = stUn.un.stream_properties.i_stream_type;
			

			
			if( !Wma_CmpGUID(&stMediaType, &Wma_object_stream_type_video_guid) 
				|| !Wma_CmpGUID(&stMediaType, &Wma_object_stream_type_audio_guid) )
			{

				#if 0
				if( !Wma_CmpGUID(&stMediaType, &Wma_object_stream_type_video_guid) )
				{	
					DEBUG0("***** !Wma_CmpGUID *****\n");
					if (stUn.un.stream_properties.i_type_specific_data_length > 51)
					{
						
						pInfo->vidExtraLen = stUn.un.stream_properties.i_type_specific_data_length - 51;
						if (g_pWmaHeaderData == NULL)
						{
	
							g_pWmaHeaderData = (BLT_UInt8 *)ATX_AllocateZeroMemory(pInfo->vidExtraLen);
							if (g_pWmaHeaderData == NULL)
							{
								return BLT_ERROR_OUT_OF_MEMORY;
								//return BLT_FAILURE;
							}
						}
					}

					result = ATX_InputStream_Read( pParser->input.stream,
										(BLT_UInt8*)&stVidCodec,
										51//stUn.un.stream_properties.i_type_specific_data_length,
										&bytes_read
										);
					if (BLT_BLT_FAILUREED(result)) 
					{
						break;
					}	
					else
					{
						stSLD.uiHeight = stVidCodec.dwEncodedImageHeight;
						stSLD.uiWidth  = stVidCodec.dwEncodedImageWidth;
					}
					if (pInfo->vidExtraLen)
					{
						result = ATX_InputStream_Read(pParser->input.stream,
										    g_pWmaHeaderData,
										    pInfo->vidExtraLen,
											&bytes_read
										  );
						if (BLT_BLT_FAILUREED(result)) 
						{
							break;
						}	
						else if (pInfo->vidExtraLen >= 4)
						{
							stSLD.uiExtraData[0] = *g_pWmaHeaderData;
							stSLD.uiExtraData[1] = *(g_pWmaHeaderData + 1);
							stSLD.uiExtraData[2] = *(g_pWmaHeaderData + 2);
							stSLD.uiExtraData[3] = *(g_pWmaHeaderData + 3);
						}
					}
					pParser->stVidObj.ucStrNum = (BLT_UInt8)(stUn.un.stream_properties.i_flags & 0x7F);
					pParser->stPlayer.uiStrNum ++;
					result = ParseVidCodec(&stVidCodec);
					if (BLT_BLT_FAILUREED(result)) 
					{
							return result;
					}			
				}
				else 
				{
				#endif
			
					result = ATX_InputStream_Read(pParser->input.stream,
									(BLT_UInt8*)&stAudCodec,
									stUn.un.stream_properties.i_type_specific_data_length,
									&bytes_read
									);
					if (BLT_FAILED(result)) 
					{
						return result;
					}
					if (pInfo->pAudExtraData == NULL) {
		
						pInfo->audExtraLen = stUn.un.stream_properties.i_type_specific_data_length;
						pInfo->pAudExtraData = ATX_AllocateZeroMemory(pInfo->audExtraLen);
				
						if (pInfo->pAudExtraData == NULL){
					
							break;
						}
						ATX_CopyMemory(pInfo->pAudExtraData, (BLT_UInt8*)&stAudCodec, pInfo->audExtraLen);
						int i = 0;
						
					}
					pParser->stAudObj.ucStrNum = (BLT_UInt8)(stUn.un.stream_properties.i_flags & 0x7F);
					pParser->stPlayer.uiStrNum ++;
					
					
					
							
					
					
					
					
				
					
					ParseAudCodec( &stAudCodec );
					/* ++ jerry : for Wma payload decoding ++ */
					pInfo->audStreamNum = pParser->stAudObj.ucStrNum;
			
				//}
				
				result = ATX_InputStream_Tell(pParser->input.stream, &where);
				if (BLT_FAILED(result)) 
				{
					break;
				}	
				
				result = ATX_InputStream_Seek(pParser->input.stream, stUn.un.stream_properties.i_error_correction_data_length + (BLT_UInt32)where);
				if (BLT_FAILED(result)) 
				{
					break;
				}
			}
			else
			{

				result = ATX_InputStream_Tell(pParser->input.stream, &where);
				if (BLT_FAILED(result)) 
				{
					break;
				}	
				result = ATX_InputStream_Seek(pParser->input.stream,(BLT_UInt32)stCom.i_object_size-24 + (BLT_UInt32)where);
				if (BLT_FAILED(result)) 
				{
					break;
				}	
			}
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;

			break;
		case FILE_PRO_ID :

			result = ATX_InputStream_Read(pParser->input.stream,
							(BLT_UInt8*)&(stUn.un.file_properties),
							OBJ_FILE_PROPER_SIZE/*sizeof( Wma_object_file_properties_t )*/,
							&bytes_read
							);
			if (BLT_FAILED(result)) 
			{
				break;
			}
			uiFlag = stUn.un.file_properties.i_flags;
			if( (uiFlag & 0x01)  == 0x01 )
			{//we don`t support broadcast mode
				result = BLT_FAILURE;
				break;
			}

			pParser->stPlayer.uiSeekable =  (BLT_UInt8)((uiFlag & 0x02) >> 1);
			pParser->stFile.uiPktSize = stUn.un.file_properties.i_min_data_packet_size ;/*Pkt size*/
			pParser->stFile.uiPreroll = (BLT_UInt32)(stUn.un.file_properties.i_preroll) ;/*millisecond unit*/
			
	
			
			if(stUn.un.file_properties.i_play_duration != 0)
			{
			
				ui64Duration = stUn.un.file_properties.i_play_duration - (stUn.un.file_properties.i_preroll * 10000);
				pInfo->uiAudLen = (BLT_UInt32)(ui64Duration/10000);/*convert to millisecond unit*/
		
			}
			else if(stUn.un.file_properties.i_send_duration != 0)
			{
		
				ui64Duration = stUn.un.file_properties.i_send_duration;
				pInfo->uiAudLen = (BLT_UInt32)(stUn.un.file_properties.i_send_duration/10000);/*convert to millisecond unit*/
		
			}
			else
			{
				//DEBUG0_0("file time is zero!!!\n");
				pInfo->uiAudLen = pInfo->uiVidLen = 0 ;
		
			}
			if (stDIVX3.uiBit_rate == 0)
			{
				stSLD.uiAverageBitrate = stUn.un.file_properties.i_max_bitrate;
				stDIVX3.uiBit_rate = int32_swap(stUn.un.file_properties.i_max_bitrate);
	
			}
	
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;

			//while(1);
			break;
		
		case CONTENT_DEC_ID :

			result = ATX_InputStream_Tell(pParser->input.stream, &where);
			if (BLT_FAILED(result)) 
			{
				break;
			}	
			
			result = ATX_InputStream_Seek(pParser->input.stream , (BLT_UInt32)stCom.i_object_size - 24 + (BLT_UInt32)where);
			if (BLT_FAILED(result)) 
			{
				break;
			}
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;

			break;
		case PADDING_ID :

			result = ATX_InputStream_Tell(pParser->input.stream, &where);
			if (BLT_FAILED(result)) 
			{
				break;
			}	
			result = ATX_InputStream_Seek(pParser->input.stream, (BLT_UInt32)stCom.i_object_size - 24 + (BLT_UInt32)where);
			if (BLT_FAILED(result)) 
			{
				break;
			}
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size ;	
	
			break;
		case CONTENT_ENCRYPTION_ID:

			return BLT_FAILURE;
			break;
		case UNKNOW_ID :/*unknow ID,but the block size must be nonzero and the total
			size can`t be larger than file size*/

			if (((stCom.i_object_size & 0x8000000000000000LL) == 1) || 
				(stCom.i_object_size > uiFileSize))
			{
				uiStop = 1;

				break;
			}
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size ;

			
			if( (uiSeekedSize <= uiFileSize)&&(stCom.i_object_size != 0) )
			{

				result = ATX_InputStream_Tell(pParser->input.stream, &where);
				if (BLT_FAILED(result)) 
				{
					break;
				}	
				result = ATX_InputStream_Seek(pParser->input.stream, (BLT_UInt32)stCom.i_object_size -24 + (BLT_UInt32)where);
				if (BLT_FAILED(result)) 
				{
					break;
				}	
			}
			else
			{
				uiStop = 1;
			 
			}			
			//DEBUG0("parsing as UNKNOW_ID! \n");
			break;
		case STR_BIT_PROPER_ID:	

			uiFlag = 0;
			result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8 *)&uiFlag, 2, &bytes_read);
			if (BLT_FAILED(result)) 
			{
				break;
			}
			for (;uiFlag > 0; uiFlag--)
			{
				usBPFlags = 0;
				uiaveragebitrate = 0;
				result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8 *)&usBPFlags, 2, &bytes_read);
				result = ATX_InputStream_Read(pParser->input.stream, (BLT_UInt8 *)&uiaveragebitrate, 4, &bytes_read);
				if (BLT_FAILED(result)) 
				{
					break;
				}
			}
			uiSeekedSize += (BLT_UInt32)stCom.i_object_size;

			break;
		default :			
			break;
		}
		if (BLT_FAILED(result)) 
		{
			/*a err occur */
			return result;
		}
		
		if( uiStop == 1)
		{	/*the file format maybe not right*/
			break;
		}
	 }
	 /*目前还不知道如何替换*/
	//StreamClose(pParser->stAudObj.pIdxStr);
	

	
	for (uiFlag = 0; uiFlag < 2; uiFlag++)
	{
		if (stExtStrPro[uiFlag].usStreamNum == pParser->stAudObj.ucStrNum)
		{
	
			pInfo->isVBR = (BLT_UInt8)(stExtStrPro[uiFlag].uiDataBitrate == stExtStrPro[uiFlag].uiAlterDataBitrate ? 0 : 1);
		}
	}

	
	if (pInfo->uiVidType == VID_WMV9)
	{	

		stSLD.uiFlag1 		= 	0x00000004; 
		stSLD.uiFlag2 		= 	0x0000000C;		
		stSLD.uiNumFrames 	&= 	0xFFFFFF;
		stSLD.uiNumFrames 	|= 	0xC5000000;
		
		if (uiaveragebitrate == 0)
		{
			stSLD.uiAlterBufSize = stSLD.uiAverageBitrate - (stSLD.uiAverageBitrate / 10);
			if ((stSLD.uiAverageBitrate % 10) > 4)
			{
				 stSLD.uiAlterBufSize -= 1;
			}		
		}
		
		if (g_pWmaHeaderData)
		{
			ATX_FreeMemory(g_pWmaHeaderData);
			g_pWmaHeaderData = NULL;
		}
		
		g_pWmaHeaderData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(sequence_layer_data_t));
		if (g_pWmaHeaderData == NULL)
		{
			return BLT_FAILURE;
		}
		ATX_CopyMemory(g_pWmaHeaderData, &stSLD, sizeof(sequence_layer_data_t));
		pInfo->pVidExtraData = g_pWmaHeaderData;
		pInfo->vidExtraLen = sizeof(sequence_layer_data_t);
	}
	else if (pInfo->uiVidType == VID_MP41 || pInfo->uiVidType == VID_DIV1 || pInfo->uiVidType == VID_MPG4 || /* MSMPEG4 v1 */
		     pInfo->uiVidType == VID_MP42 || pInfo->uiVidType == VID_DIV2 ||                                 /* MSMPEG4 v2 */
			 pInfo->uiVidType == VID_MP43 || pInfo->uiVidType == VID_DIV3 || pInfo->uiVidType == VID_DIV4 || /* MSMPEG4 v3 */
			 pInfo->uiVidType == VID_DIV5 || pInfo->uiVidType == VID_DIV6 || pInfo->uiVidType == VID_AP41 ||
			 pInfo->uiVidType == VID_COL1 || pInfo->uiVidType == VID_DVX3 || pInfo->uiVidType == VID_MPG3) 
	{

		stDIVX3.uiVol_start_code = int32_swap(MP43_VOL_START_CODE);
		stDIVX3.uiHeader_Length = int32_swap(sizeof(MP43VideoObjectLayer_s) - 8);
		stDIVX3.uiHandle = 0;
		stDIVX3.uiScale = int32_swap(pInfo->usScale);
		stDIVX3.uiRate = int32_swap(pInfo->uiFrmRate / FRAMERATE_MULTIPLE);
		stDIVX3.uiFourcc = pInfo->uiVidType;
		stDIVX3.uiWidth = int32_swap(pInfo->uiWidth);
		stDIVX3.uiHeight = int32_swap(pInfo->uiHeight); 
		stDIVX3.uiDepth = (BLT_UInt16)int32_swap(pInfo->usDepth << 16);

		if (g_pWmaHeaderData)
		{

			ATX_FreeMemory(g_pWmaHeaderData);
			g_pWmaHeaderData = NULL;
		}
		g_pWmaHeaderData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(MP43VideoObjectLayer_s));
		//g_pWmaHeaderData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(MP43VideoObjectLayer_s));
		if (g_pWmaHeaderData == NULL)
		{
		
			return BLT_FAILURE;
		}
		ATX_CopyMemory(g_pWmaHeaderData, &stDIVX3, sizeof(MP43VideoObjectLayer_s));
		//ATX_CopyMemory(g_pWmaHeaderData, &stDIVX3, sizeof(MP43VideoObjectLayer_s));
		pInfo->pVidExtraData = g_pWmaHeaderData;
		pInfo->vidExtraLen = sizeof(MP43VideoObjectLayer_s);

		pInfo->uiVidType = VID_DIVX3;
	}
	else if (pInfo->uiVidType == VID_WMV7 || pInfo->uiVidType == VID_WMV8)
	{

		stWMV7Video.uiFourCC = pInfo->uiVidType;
		stWMV7Video.uiWidth = pInfo->uiWidth;
		stWMV7Video.uiHeight = pInfo->uiHeight; 
		if (pInfo->vidExtraLen > 3 && pInfo->uiVidType == VID_WMV8)
		{

			stWMV7Video.uiexDataLen = pInfo->vidExtraLen;
			pInfo->pVidExtraData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(WMV78VideoHeader_s) + pInfo->vidExtraLen);
			//pInfo->pVidExtraData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(WMV78VideoHeader_s) + pInfo->vidExtraLen);
			if (pInfo->pVidExtraData == NULL)
			{
				return BLT_FAILURE;
			}
			
			ATX_CopyMemory(pInfo->pVidExtraData, &stWMV7Video, sizeof(WMV78VideoHeader_s));
			ATX_CopyMemory(pInfo->pVidExtraData + sizeof(WMV78VideoHeader_s), g_pWmaHeaderData, stWMV7Video.uiexDataLen);
			
			//ATX_CopyMemory(pInfo->pVidExtraData, &stWMV7Video, sizeof(WMV78VideoHeader_s));
			//ATX_CopyMemory(pInfo->pVidExtraData + sizeof(WMV78VideoHeader_s), g_pWmaHeaderData, stWMV7Video.uiexDataLen);
			pInfo->vidExtraLen += sizeof(WMV78VideoHeader_s);
		}
		else 
		{

			pInfo->pVidExtraData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(WMV78VideoHeader_s));
			//pInfo->pVidExtraData = (BLT_UInt8 *)ATX_AllocateZeroMemory(sizeof(WMV78VideoHeader_s));
			if (pInfo->pVidExtraData == NULL)
			{
				return BLT_FAILURE;
			}
			ATX_CopyMemory(pInfo->pVidExtraData, &stWMV7Video, sizeof(WMV78VideoHeader_s));
			//ATX_CopyMemory(pInfo->pVidExtraData, &stWMV7Video, sizeof(WMV78VideoHeader_s));		
			pInfo->vidExtraLen = sizeof(WMV78VideoHeader_s);
		}

		if (g_pWmaHeaderData)
		{
			ATX_FreeMemory(g_pWmaHeaderData);
			g_pWmaHeaderData = NULL;
			//ATX_FreeMemory(g_pWmaHeaderData);			
		}
		g_pWmaHeaderData = pInfo->pVidExtraData;
	}

	if(uiHasIdx&uiHasData)
	{	/*has index and data ,can seek*/

		pParser->stPlayer.uiSeekable = 1;
		pInfo->uiSeekable = 1 ;
		return BLT_SUCCESS;
	}
	else if(uiHasData )
	{	/*just has data , can not seek*/  

		pParser->stPlayer.uiSeekable =0;
		pInfo->uiSeekable = 0 ;
		return BLT_SUCCESS ;
	}
	else
	{ /* no data ,alse no index,this file cann`t been played*/

		return BLT_FAILURE;
	}
 }

/*----------------------------------------------------------------------
|   WmaheaderParse
+---------------------------------------------------------------------*/
static BLT_UInt32 WmaheaderParse(WmaParser* pParser, ContainBitsInfo_t* pInfo)
{
	ATX_LargeSize	fileTotalSize;
	BLT_UInt32		uiFileSize;
	BLT_UInt8*		pMem;
	BLT_UInt8*		pMem1;
	BLT_Result  	result;

	pMem  = (BLT_UInt8*)pParser->uiMemAddr;/*for cache audio data*/
	pMem1 = (BLT_UInt8*)(pMem + WMA_AUD_DATA_SIZE);/*for cache video data*/
	pParser->pAudSeekBuf = pMem1 + WMA_AUD_DATA_SIZE + WMA_VID_DATA_SIZE;
	

	/*获得文件的大小*/
	ATX_InputStream_GetSize(pParser->input.stream, &fileTotalSize);
	if (BLT_FAILED(result)) {
		return result;
	}
	DEBUG0("fileTotalSize = %llu\n", fileTotalSize);
	uiFileSize = (BLT_UInt32)fileTotalSize;

	DEBUG0("uiFileSize = %u\n", uiFileSize);

	result = WmaHdrParse(pParser, uiFileSize);
	if (BLT_FAILED(result)) {
		return result;
	}


	/* create a substream */
	pParser->stAudObj.pIdxStr = ATX_AllocateZeroMemory(sizeof(WmaParserInput));
	if (pParser->stAudObj.pIdxStr == NULL) {
        return BLT_ERROR_OUT_OF_MEMORY;
    }
	
    ATX_SubInputStream_Create(pParser->input.stream, 
                                     pParser->uiDataPos, 
                                     WMA_AUD_DATA_SIZE,
                                     NULL,
                                     &(pParser->stAudObj.pIdxStr->stream));
									 
	pParser->stAudObj.pIdxStr->uiStreamEnd = uiFileSize;
	

	WmaInfoOut(pParser, pInfo);
                       
	WmaReInit(pParser);

	/*test code end */
	return  BLT_SUCCESS;
}
/*----------------------------------------------------------------------
|   WmaParser_UpdateMediaType
+---------------------------------------------------------------------*/
static void
WmaParser_UpdateMediaType(WmaParser* self)
{
	
}



/*----------------------------------------------------------------------
|   WmaParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParserInput_SetStream(BLT_InputStreamUser* _self,
                          ATX_InputStream*     stream,
                          const BLT_MediaType* media_type)
{
    WmaParser*    		self = ATX_SELF_M(input, WmaParser, BLT_InputStreamUser);
    BLT_Size       		header_size;
    BLT_StreamInfo 		stream_info;
    ContainBitsInfo_t 	*av_info;
    ContainBitsInfo_t 	st_av_info;
    BLT_Result     		result;
    
    av_info = &st_av_info;

    /* check media type */
    if (media_type == NULL || media_type->id != self->input.media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);
    

    self->input.stream = stream;
	result = WmaheaderParse(self, av_info);

	
	if (av_info->audType != AUDIO_TYPE_NONE && av_info->audSR)
	{
#ifdef USE_LIBAVCODEC
		if (av_info->audType == CODEC_ID_WMAV1
		 || av_info->audType == CODEC_ID_WMAV2
		 || av_info->audType == CODEC_ID_WMAPRO)
#else
		if (av_info->audType == AUDIO_TYPE_WMA
		 || av_info->audType == AUDIO_TYPE_WMA2
		 || av_info->audType == AUDIO_TYPE_WMAPRO)
#endif
		{
			self->output.media_type->wma_audio_info.wAudioStreamId = g_stInfo.audStreamNum;
			self->output.media_type->wma_audio_info.wFormatTag	= (short)g_stInfo.uiAudType;	
			self->output.media_type->wma_audio_info.nSamplesPerSec = g_stInfo.uiAudSR;
			self->output.media_type->wma_audio_info.wBitsPerSample = g_stInfo.uiAudBit;
			self->output.media_type->wma_audio_info.nChannels = g_stInfo.uiAudChannel;
			self->output.media_type->wma_audio_info.nBlockAlign = g_stInfo.uiBlockAlign;
			self->output.media_type->wma_audio_info.nAvgBytesPerSec = g_stInfo.uiAudBitRate / 8;
			self->output.media_type->wma_audio_info.nSamplesPerBlock = g_stInfo.uiSamplesPerBlock;
			self->output.media_type->wma_audio_info.nEncodeOpt = g_stInfo.uiEncodeOpt;
			self->output.media_type->wma_audio_info.bStreamId = self->output.media_type->wma_audio_info.wAudioStreamId;
			self->output.media_type->wma_audio_info.cbRepData = 8;
			self->output.media_type->wma_audio_info.cbPayloadSize = self->output.media_type->wma_audio_info.nBlockAlign;
			self->output.media_type->extra_data_size = sizeof(wma_audio_t);
			self->output.media_type->extra_data = (BLT_UInt8 *)&(self->output.media_type->wma_audio_info);
			

#if 0
			DEBUG0("self->output.media_type->wma_audio_info.wAudioStreamId = %x\n", self->output.media_type->wma_audio_info.wAudioStreamId);
			DEBUG0("self->output.media_type->wma_audio_info.wFormatTag = %x\n", self->output.media_type->wma_audio_info.wFormatTag);
			DEBUG0("self->output.media_type->wma_audio_info.nChannels = %x\n",self->output.media_type->wma_audio_info.nChannels);
			DEBUG0("self->output.media_type->wma_audio_info.nSamplesPerSec = %x\n", self->output.media_type->wma_audio_info.nSamplesPerSec);
			DEBUG0("self->output.media_type->wma_audio_info.nAvgBytesPerSec= %x\n", self->output.media_type->wma_audio_info.nAvgBytesPerSec);
			DEBUG0("self->output.media_type->wma_audio_info.nBlockAlign= %x\n", self->output.media_type->wma_audio_info.nBlockAlign);
			DEBUG0("self->output.media_type->wma_audio_info.wBitsPerSample = %x\n", self->output.media_type->wma_audio_info.wBitsPerSample);
			DEBUG0("self->output.media_type->wma_audio_info.nSamplesPerBlock= %x\n", self->output.media_type->wma_audio_info.nSamplesPerBlock);
			DEBUG0("self->output.media_type->wma_audio_info.nEncodeOpt = %x\n", self->output.media_type->wma_audio_info.nEncodeOpt);
			DEBUG0("g_asf_parse->asf_header.bStreamId =%x\n", self->output.media_type->wma_audio_info.bStreamId);
			DEBUG0("g_asf_parse->asf_header.cbRepData =%x\n", self->output.media_type->wma_audio_info.cbRepData);
			DEBUG0("g_asf_parse->asf_header.cbPayloadSize =%x\n", self->output.media_type->wma_audio_info.cbPayloadSize);





			int i =0;
			for(i=0;i<self->output.media_type->extra_data_size;i++)
			{
				if(i%8==0 && i!=0){
					DEBUG0("\n");
				}
				DEBUG0("%02x ", ((BLT_UInt8*)self->output.media_type->extra_data)[i]);
			}
			DEBUG0("\n");
while(1);
#endif

#if 1
		
			DEBUG0("g_stInfo.audExtraLen = %x\n", g_stInfo.audExtraLen);
			DEBUG0("g_stInfo.audStreamNum = %x\n", g_stInfo.audStreamNum);
			DEBUG0("g_stInfo.isVBR = %x\n",g_stInfo.isVBR);
			DEBUG0("g_stInfo.uiAudBit = %x\n", g_stInfo.uiAudBit);
			DEBUG0("g_stInfo.uiAudBitRate= %x\n", g_stInfo.uiAudBitRate);
			DEBUG0("g_stInfo.uiAudChannel= %x\n", g_stInfo.uiAudChannel);
			DEBUG0("g_stInfo.uiAudLen = %x\n", g_stInfo.uiAudLen);
			DEBUG0("g_stInfo.uiAudSR= %x\n", g_stInfo.uiAudSR);
			DEBUG0("g_stInfo.uiAudType = %x\n", g_stInfo.uiAudType);
			DEBUG0("g_stInfo.uiBlockAlign =%x\n", g_stInfo.uiBlockAlign);
			DEBUG0("g_stInfo.uiSeekable =%x\n", g_stInfo.uiSeekable);
			DEBUG0("g_stInfo.uiSamplesPerBlock =%x\n", g_stInfo.uiSamplesPerBlock);
		//	while(1);
#endif

			
#if 0
			typedef struct {
				BLT_Mask	   mask;			 /**< Mask indicating which fields are valid */
				BLT_StreamType type;			 /**< Stream Type							 */
				BLT_UInt32	   id;				 /**< Stream ID 							 */
				BLT_UInt32	   nominal_bitrate;  /**< Nominal bitrate						 */
				BLT_UInt32	   average_bitrate;  /**< Average bitrate						 */
				BLT_UInt32	   instant_bitrate;  /**< Instant bitrate						 */
				BLT_UInt64	   size;			 /**< Size in bytes 						 */
				BLT_UInt64	   duration;		 /**< Duration in milliseconds				 */
				BLT_UInt32	   sample_rate; 	 /**< Sample rate in Hz 					 */
				BLT_UInt16	   channel_count;	 /**< Number of channels					 */
				BLT_UInt16	   width;			 /**< Picture Width 						 */
				BLT_UInt16	   height;			 /**< Picture Height						 */
				BLT_Flags	   flags;			 /**< Stream Flags							 */
				BLT_CString    data_type;		 /**< Human-readable data type				 */
			} BLT_StreamInfo;

#define BLT_STREAM_INFO_MASK_ALL             0x1FFF

#define BLT_STREAM_INFO_MASK_TYPE            0x0001
#define BLT_STREAM_INFO_MASK_ID              0x0002
#define BLT_STREAM_INFO_MASK_NOMINAL_BITRATE 0x0004
#define BLT_STREAM_INFO_MASK_AVERAGE_BITRATE 0x0008
#define BLT_STREAM_INFO_MASK_INSTANT_BITRATE 0x0010
#define BLT_STREAM_INFO_MASK_SIZE            0x0020
#define BLT_STREAM_INFO_MASK_DURATION        0x0040
#define BLT_STREAM_INFO_MASK_SAMPLE_RATE     0x0080
#define BLT_STREAM_INFO_MASK_CHANNEL_COUNT   0x0100
#define BLT_STREAM_INFO_MASK_WIDTH           0x0200
#define BLT_STREAM_INFO_MASK_HEIGHT          0x0400
#define BLT_STREAM_INFO_MASK_FLAGS           0x0800
#define BLT_STREAM_INFO_MASK_DATA_TYPE       0x1000

#endif



			stream_info.mask = 0;
			stream_info.sample_rate   	= g_stInfo.uiAudSR;
    		stream_info.channel_count 	= g_stInfo.uiAudChannel;
			stream_info.duration		= g_stInfo.uiAudLen;
#if 1
			if(CODEC_ID_WMAV1 == av_info->audType ){
				stream_info.data_type = "WMAV1";				
			}if(CODEC_ID_WMAV2 == av_info->audType){
				stream_info.data_type = "WMAV2";
			}else{
				stream_info.data_type = "UNKOWN";
			}
#endif

    		stream_info.mask |=
                      BLT_STREAM_INFO_MASK_SAMPLE_RATE   |
                      BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
                      BLT_STREAM_INFO_MASK_DATA_TYPE	 |
                      BLT_STREAM_INFO_MASK_DURATION;
			 /* update the stream info */
    		if (stream_info.mask && ATX_BASE(self, BLT_BaseMediaNode).context) {
        		BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, 
                           &stream_info);
   			 }
			
			WmaParser_UpdateMediaType(self);

		}
	}
	DEBUG0("*********************return from WmaParserInput_SetStream**********************\n");
		
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmaParser_MakeEosPacket
+---------------------------------------------------------------------*/
static BLT_Result
WmaParser_MakeEosPacket(WmaParser* self, BLT_MediaPacket** packet)
{
	BLT_Result result;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        0,
                                        (BLT_MediaType*)self->output.media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;

    BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
    return BLT_SUCCESS;
}


/*----------------------------------------------------------------------
|   WmaParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParserInput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    WmaParser* self = ATX_SELF_M(input, WmaParser, BLT_MediaPort);

    if (index == 0) {
        *media_type = &self->input.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmaParserInput)
    ATX_GET_INTERFACE_ACCEPT(WmaParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WmaParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WmaParserInput, BLT_InputStreamUser)
    WmaParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WmaParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(WmaParserInput, BLT_MediaPort)
    WmaParserInput_GetName,
    WmaParserInput_GetProtocol,
    WmaParserInput_GetDirection,
    WmaParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   WmaParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    WmaParser* self = ATX_SELF_M(output, WmaParser, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = self->output.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}


/*----------------------------------------------------------------------
|   MP_ObjFetch
+---------------------------------------------------------------------*/  
BLT_UInt32 MP_ObjFetch(WmaParser* pParser,BLT_UInt8 ucStrNum, BLT_UInt8* pBuf, BLT_UInt32* puiSize)
{
	BLT_UInt32 uiRepLen = 0;
	BLT_UInt32 uiPayLen = 0;
	BLT_UInt32 uiObjSize = 0;
	BLT_UInt32 uiOffset = 0;
	BLT_UInt32 uiCorrect = 0;
	BLT_UInt8 ucFlag;
	BLT_UInt8 ucSubPayLen;
	ATX_Size bytes_read = 0;
	BLT_Position where;
	BLT_Result result = BLT_SUCCESS;
	PktParser_S *pPkt;

	if( ucStrNum == pParser->stAudObj.ucStrNum )
	{	
		pPkt = &(pParser->stAudObj) ;
	}

	if (pPkt->stCPayload.uiCurCPayNum > 0 && pPkt->stCPayload.uiReGetPay == 1)
	{//for Compressed payload	
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
								(BLT_UInt8*)&ucSubPayLen,
								1,
								&bytes_read);
		
		pPkt->stCPayload.uiCurCPayNum--;
		*puiSize = ucSubPayLen;
		
		return BLT_SUCCESS;
	}

	uiObjSize = pPkt->uiObjSize;
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
								&ucFlag,
								1,
								&bytes_read);
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, pPkt->stPpi.ucMediaObjType + (BLT_UInt32)where);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&uiOffset,
											pPkt->stPpi.ucMediaOffType,
											&bytes_read);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&uiRepLen,
											pPkt->stPpi.ucRepType,
											&bytes_read);

	if (uiRepLen == 1 && (ucFlag & 0x7F) == ucStrNum)
	{//for Compressed payload
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, 1 + (BLT_UInt32)where);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&uiPayLen,
											pPkt->stPpi.ucPayType,
											&bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&ucSubPayLen,
											1,
											&bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											pBuf,
											ucSubPayLen,
											&bytes_read);
				
		pPkt->stCPayload.uiCurCPayNum--;
		*puiSize = ucSubPayLen;
		
		return BLT_SUCCESS;
	}

	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiRepLen + (BLT_UInt32)where);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&uiPayLen,
											pPkt->stPpi.ucPayType,
											&bytes_read);

	if((ucFlag & 0x7F) == ucStrNum)
	{
		if(uiOffset != 0)
		{
			uiCorrect= g_uiPayLoadSize -(uiOffset - g_uiLastOffset);
		}
		pBuf -= uiCorrect;
		if(	uiPayLen >(uiObjSize - uiOffset))
		{
			result = ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											pBuf,
											uiObjSize - uiOffset,
											&bytes_read);
			if(BLT_FAILED(result)){
				return result;
			}
			
			ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
			ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiPayLen -uiObjSize +uiOffset + (BLT_UInt32)where);

			g_uiPayLoadSize = uiObjSize - uiOffset;
			*puiSize = g_uiPayLoadSize-uiCorrect ;
		}
		else
		{
			result = ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											pBuf,
											uiPayLen,
											&bytes_read);
			if(BLT_FAILED(result)){
				return result;
			}

			g_uiPayLoadSize = uiPayLen;
			*puiSize = g_uiPayLoadSize - uiCorrect ;
		}
		g_uiLastOffset = uiOffset;
		
	}
	else
	{
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiPayLen + (BLT_UInt32)where);
		if(BLT_FAILED(result)){
				return result;
		}
		*puiSize = 0;
	}
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   SP_ObjFetch
+---------------------------------------------------------------------*/  
BLT_UInt32 SP_ObjFetch(WmaParser* pParser ,BLT_UInt8 ucStrNum, BLT_UInt8* pBuf, BLT_UInt32* puiSize)
{
	BLT_UInt32 	uiRepLen = 0;
	BLT_UInt32	uiCorrect = 0;
	BLT_UInt32 	uiSize;
	BLT_UInt32 	uiObjSize = 0;
	BLT_UInt32	uiOffset = 0;
	BLT_UInt8 	ucFlag;
	BLT_UInt8 	ucSubPayLen;
	ATX_Size 	bytes_read;
	BLT_Position where;
	BLT_Result	result = BLT_SUCCESS;
	PktParser_S *pPkt;

	if(ucStrNum == pParser->stAudObj.ucStrNum )
	{
		pPkt = &(pParser->stAudObj) ;
	}


	if (pPkt->stCPayload.uiCurCPayNum > 0 /*&& pPkt->stCPayload.uiReGetPay == 1*/)
	{//for Compressed payload				
		
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8 *)&ucSubPayLen,
											1,
											&bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											pBuf,
											ucSubPayLen,
											&bytes_read);									
	
		pPkt->stCPayload.uiCurCPayNum--;
		*puiSize = ucSubPayLen;
		
		return BLT_SUCCESS;
	}

	uiObjSize = pPkt->uiObjSize;

	
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&ucFlag,
											1,
											&bytes_read);
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, pPkt->stPpi.ucMediaObjType + (BLT_UInt32)where);											
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&uiOffset,
											pPkt->stPpi.ucMediaOffType,
											&bytes_read);
	ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&uiRepLen,
											pPkt->stPpi.ucRepType,
											&bytes_read);
	
	
	if (uiRepLen == 1 && (ucFlag & 0x7F) == ucStrNum)
	{//for Compressed payload
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, 1 + (BLT_UInt32)where);											
	
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											(BLT_UInt8*)&ucSubPayLen,
											1,
											&bytes_read);
		ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											pBuf,
											ucSubPayLen,
											&bytes_read);
															
		pPkt->stCPayload.uiCurCPayNum--;
		*puiSize = ucSubPayLen;
		
		return BLT_SUCCESS;
	}
	
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);							
	ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiRepLen + (BLT_UInt32)where);									

	if((ucFlag & 0x7F) == ucStrNum )
	{	/*emendation for last data fill*/
		if(uiOffset != 0)
		{
			uiCorrect= g_uiPayLoadSize -(uiOffset - g_uiLastOffset);
		}
		pBuf -=uiCorrect;
		ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);	
		uiSize =pParser->stFile.uiPktSize * pPkt->uiNowPkt - (BLT_UInt32)where - pPkt->stPpi.usPadLen;
		if(uiSize > uiObjSize - uiOffset )
		{
			uiSize = uiObjSize - uiOffset;
		}
		result = ATX_InputStream_Read(pParser->stAudObj.pIdxStr->stream, 
											pBuf,
											uiSize,
											&bytes_read);
		if(BLT_FAILED(result)){
				return result;
		}
		
		g_uiPayLoadSize = uiSize;
		*puiSize = g_uiPayLoadSize - uiCorrect;
		g_uiLastOffset = uiOffset;
		
	
	}
	else
	{
		*puiSize = 0;
	}
	
	return BLT_SUCCESS ;
}

/*----------------------------------------------------------------------
|   WmaAudDataFill
+---------------------------------------------------------------------*/  
BLT_UInt32 WmaAudDataFill(WmaParser* pParser, BLT_UInt8* pBuf)
{
	BLT_UInt32 	uiRetSize;
	BLT_UInt32 	uiNextPos;
	BLT_UInt32 	uiSendTime;
	BLT_UInt16 	uiDurTime;
	BLT_Int32  	siSize;
	BLT_UInt8*	pMovBuf;
	BLT_UInt8	ucStrNum;
	BLT_Position where;
	BLT_Result result = BLT_SUCCESS;
	
	PktParser_S* pPkt = &(pParser->stAudObj);
	ucStrNum = pPkt->ucStrNum;
	siSize = pPkt->uiObjSize;
	pMovBuf = pBuf;

	
	while(siSize > 0)
	{
		
		
		if( pPkt->uiNowPayLoad != 0 )
		{
			if(pPkt->uiPayLoadType == 1)
			{/*multi payload*/
				result =MP_ObjFetch(pParser, ucStrNum, pMovBuf, &uiRetSize);
				pPkt->uiNowPayLoad--;
				if(BLT_SUCCEEDED(result)){
					pMovBuf += uiRetSize;
					siSize -= uiRetSize;
				}else{
					return result;
				}
			}else{/*single payload*/
				result =SP_ObjFetch(pParser,ucStrNum,pMovBuf,&uiRetSize);
				pPkt->uiNowPayLoad--;
				if(BLT_SUCCEEDED(result)){
					pMovBuf += uiRetSize;
					siSize -= uiRetSize;
			
				}else{
					return result;
				}
			}
		}else{	
				//Seek to next Packet;
				if(pPkt->uiNowPkt >=  pParser->stFile.uiPktNum ){
					/*reach the end of file*/
					return VID_PLAY_ERR015;
				}
				uiNextPos = pParser->stFile.uiPktSize * pPkt->uiNowPkt;
				
				ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);		
				result = ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, uiNextPos + (BLT_UInt32)where);
				if (BLT_FAILED(result)) {
					return result;
				}	
				result = PktHdrParser(pParser, ucStrNum, &uiSendTime, &uiDurTime);
				if (BLT_FAILED(result)) {
					return result;
				}	
			}

	}
	
	if(siSize != 0){	
		return BLT_FAILURE;
	}

	return BLT_SUCCESS;
}
/*----------------------------------------------------------------------
|   WmaParserOutput_GetStream
+---------------------------------------------------------------------*/                         
BLT_METHOD
WmaParserOutput_GetPacket(BLT_PacketProducer*  _self,
                          BLT_MediaPacket**   packet)
{

   WmaParser* self = ATX_SELF_M(output, WmaParser, BLT_PacketProducer);
   /*asf_frame_t *af = &g_asf_parse->a_frame;*/
   BLT_UInt8*   tempbuf = NULL;
   BLT_UInt32 	size;
   BLT_UInt32 	pts;
   BLT_Result  	result = BLT_SUCCESS;
   BLT_UInt8	buf[20 * 1024];
   BLT_UInt32 	total_size = 0;
   
	#if 1
	*packet = NULL;
	ATX_SetMemory(buf, 0, 20 * 1024);

	do{		
		result = WmaAudNextGet(self, &size, &pts);
			

		if (BLT_FAILED(result)) {		
				if(result != VID_PLAY_ERR015 ){			
						return BLT_ERROR_PORT_HAS_NO_DATA;		
				}else{
						if(total_size > 0){
							DEBUG0("&&&& zui hou yige packet &&&&&\n");	
							goto pack_packet;
						} else {
							DEBUG0("&&&& at end of the file &&&&&\n");	
							self->input.eos = BLT_TRUE;			
							return WmaParser_MakeEosPacket(self, packet);		
						}		
				}
		}
		DEBUG0("############ total_size = %d\n##########", total_size);
		tempbuf = (BLT_UInt8*)ATX_AllocateZeroMemory(size);    
		if (tempbuf == NULL) {		
			DEBUG0("**** alloc tmpbuf fail ****\n");        
			return BLT_ERROR_OUT_OF_MEMORY;    
		}

		ATX_SetMemory(tempbuf, 0, size);	

		result = WmaAudDataFill(self, tempbuf);
		if (BLT_FAILED(result)) {		
			ATX_FreeMemory(tempbuf);	
			return BLT_FAILURE;	
		}
		
		ATX_CopyMemory(buf + total_size, tempbuf, size);
		total_size += size;
		ATX_FreeMemory(tempbuf);
		tempbuf = NULL;
		
	}while(total_size < 8 * 1024);
	


pack_packet:
	result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
											total_size,
											(BLT_MediaType*)self->output.media_type,
											packet);	
		if (BLT_FAILED(result)) {		
			DEBUG0("**** create packet fail ****\n");		
			return result;	
		}	   
		ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(*packet),
						buf,
						total_size);	   
		BLT_MediaPacket_SetPayloadSize(*packet, total_size);

		if(tempbuf != NULL){
			ATX_FreeMemory(tempbuf);
			tempbuf = NULL;
		}
		ATX_SetMemory(buf, 0, 20 * 1024);
		total_size = 0;
		return BLT_SUCCESS;
	#endif

#if 0	
	/* default value */
	*packet = NULL;
	   
	DEBUG0("**** into WmaParserOutput_GetPacket ****\n");	
	/*获得文件的大小和时间错*/
	result = WmaAudNextGet(self, &size, &pts);

	if (BLT_FAILED(result)) {
		DEBUG0("**** result = %d****\n", result);
		DEBUG0("**** WmaAudNextGet fail ****\n");
		//while(1);
		if(result != VID_PLAY_ERR015 ){
			return BLT_ERROR_PORT_HAS_NO_DATA;
		}else{
			/*到了文件末尾*/
			DEBUG0("&&&& at end of the file &&&&&\n");
			self->input.eos = BLT_TRUE;
			return WmaParser_MakeEosPacket(self, packet);	
		}
	}		

	tempbuf = ATX_AllocateZeroMemory(size);
    if (tempbuf == NULL) {
		DEBUG0("**** alloc tmpbuf fail ****\n");
        return BLT_ERROR_OUT_OF_MEMORY;
    }

	ATX_SetMemory(tempbuf, 0, size);
	
	result = WmaAudDataFill(self, tempbuf);
	if (BLT_FAILED(result)) {
		DEBUG0("**** WmaAudDataFill fail ****\n");
		return BLT_FAILURE;
	}
	result = BLT_Core_CreateMediaPacket(
                        ATX_BASE(self, BLT_BaseMediaNode).core,
                        size,
                        (BLT_MediaType*)self->output.media_type,
                        packet);


    if (BLT_FAILED(result)) {
		DEBUG0("**** create packet fail ****\n");
		return result;
	}

	
   ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(*packet),
                                  	tempbuf,
                                   	size);
	

   BLT_MediaPacket_SetPayloadSize(*packet, size);

    return BLT_SUCCESS;
#endif
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmaParserOutput)
    ATX_GET_INTERFACE_ACCEPT(WmaParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WmaParserOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WmaParserOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(WmaParserOutput, BLT_MediaPort)
    WmaParserOutput_GetName,
    WmaParserOutput_GetProtocol,
    WmaParserOutput_GetDirection,
    WmaParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WmaParserOutput, BLT_PacketProducer)
    WmaParserOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   WmaParserInit
+---------------------------------------------------------------------*/

static void WmaParserInit(void)
{

	WmaInfo_S *pstInfo = &g_stInfo ;
	
	ATX_SetMemory(pstInfo,0,sizeof(WmaInfo_S));
	g_bFrame = 1;
}

/*----------------------------------------------------------------------
|   WmaParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
WmaParser_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    WmaParser* self;

    ATX_LOG_FINE("WmaParser::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(WmaParser));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
	/* construct the object */
    BLT_MediaType_Init(&self->input.media_type,
                       ((WmaParserModule*)module)->wma_type_id);
	self->input.stream = NULL;
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);
	self->output.media_type = (BLT_WmaAudioMediaType*)ATX_AllocateZeroMemory(sizeof(BLT_WmaAudioMediaType)+1);
	BLT_MediaType_InitEx(&self->output.media_type->base, ((WmaParserModule*)module)->wma_type_id, sizeof(BLT_WmaAudioMediaType)+1);
    
	/* setup interfaces */
    ATX_SET_INTERFACE_EX(self, WmaParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, WmaParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  WmaParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  WmaParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, WmaParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, WmaParserOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

	
	WmaParserInit();
   
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmaParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
WmaParser_Destroy(WmaParser* self)
{
    ATX_LOG_FINE("WmaParser::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* free the media type extensions */
    BLT_MediaType_Free(self->output.media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WmaParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParser_Deactivate(BLT_MediaNode* _self)
{
    WmaParser* self = ATX_SELF_EX(WmaParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("WmaParser::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

	/* release the stream */
    ATX_RELEASE_OBJECT(self->input.stream);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmaParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    WmaParser* self = ATX_SELF_EX(WmaParser, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(&self->input, BLT_MediaPort);
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "output")) {
        *port = &ATX_BASE(&self->output, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}
/*----------------------------------------------------------------------
|   AsfAudSeek
+---------------------------------------------------------------------*/

static BLT_UInt32 AsfAudSeek(WmaParser* pParser, BLT_UInt32 ms)
{
	BLT_UInt32		uiSeekPos = ms;
	BLT_UInt32 		uiNowPkt;
	BLT_UInt32		uiPayLoadType;
	BLT_UInt32		uiNowPayLoad;
	BLT_UInt32 		uiAudms;
	BLT_UInt32 		uiseektime;
	BLT_UInt32		uisize;
	BLT_UInt8*		pAudTBuf;
	BLT_Position	where;
	BLT_Result		result;
	BLT_UInt8*		pAudBuf;
	BLT_UInt32 		uimaxaud;
	WmaInfo_S* 		pInfo = &g_stInfo;	

	result = BLT_SUCCESS;
	pAudTBuf = NULL;
	DEBUG0("**** into AsfAudSeek ****\n");

	DEBUG0("*** pParser->stPlayer.uiAudms = %d\n", pParser->stPlayer.uiAudms);

	DEBUG0("pParser->stFile.uiPreroll = %d\n", pParser->stFile.uiPreroll);
	DEBUG0("uiSeekPos = %d\n", uiSeekPos);
	if ((uiSeekPos + pParser->stFile.uiPreroll) < pParser->stPlayer.uiAudms)
	{
		ATX_InputStream_Seek(pParser->stAudObj.pIdxStr->stream, 0);			
		ATX_SetMemory(&(pParser->stAudObj.stCPayload), 0, sizeof(ComPressedPay_s));		
		pParser->stAudObj.uiNowPayLoad = 0 ;
		pParser->stAudObj.uiNowPkt = 0 ;
	}

	if (uiSeekPos > 500)
	{
		uiSeekPos -= 500;
	}

	uimaxaud = 10*1024;

	pAudBuf = ATX_AllocateZeroMemory(uimaxaud);
	if (pAudBuf == NULL){
		return BLT_FAILURE;
	}

	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
	DEBUG0("^^^^^^^^^ where = %llu ^^^^^^^^\n", where);

	do { 
		WmaAudNextGet(pParser, &uisize, &uiseektime);
		DEBUG0("uiSeekPos = %d\n", uiSeekPos);
		DEBUG0("uiseektime =  %d\n", uiseektime);
		if (uisize > uimaxaud){
			uimaxaud = uisize;
			ATX_FreeMemory(pAudBuf);
			pAudBuf = ATX_AllocateZeroMemory(uimaxaud);
			if (pAudBuf == NULL)
			{
				return BLT_FAILURE;
			}
		}
		WmaAudDataFill(pParser, pAudBuf);
	} while (uiSeekPos > uiseektime);

	ATX_FreeMemory(pAudBuf);
	pAudBuf = NULL;
	DEBUG0("***** parser seek end *****\n");
	ATX_InputStream_Tell(pParser->stAudObj.pIdxStr->stream, &where);
	DEBUG0("^^^^^^^^^ where = %llu ^^^^^^^^\n", where);

	return BLT_SUCCESS;

}

/*----------------------------------------------------------------------
|   WmaParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParser_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    WmaParser* self = ATX_SELF_EX(WmaParser, BLT_BaseMediaNode, BLT_MediaNode);
	BLT_Result 	result;
	BLT_UInt32	ms;

	#if 0
    /* estimate the seek point */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
        return BLT_FAILURE;
    }
	#endif
    /* seek to the estimated offset */
    /* seek into the input stream (ignore return value) */
	DEBUG0("point->time_stamp.seconds = %d\n", point->time_stamp.seconds);
	DEBUG0("point->time_stamp.nanoseconds = %d\n", point->time_stamp.nanoseconds);
	ms = point->time_stamp.seconds * 1000 + point->time_stamp.nanoseconds /1000000;
	DEBUG0("ms = %d\n", ms);
	
	result =  	AsfAudSeek(self, ms);
	if(BLT_FAILED(result)){
		return BLT_FAILURE;
	}
    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                  */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmaParser)
    ATX_GET_INTERFACE_ACCEPT_EX(WmaParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(WmaParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WmaParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    WmaParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    WmaParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    WmaParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WmaParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   WmaParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    WmaParserModule* self = ATX_SELF_EX(WmaParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".wma" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wma",
                                            "audio/wma");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/wma" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/wma",
        &self->wma_type_id);
    if (BLT_FAILED(result)) return result;

	
	/* register an alias for the same mime type */
	BLT_Registry_RegisterNameForId(registry, 
									  BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
									  "audio/wma", self->wma_type_id);
    


    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmaParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
WmaParserModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    WmaParserModule* self = ATX_SELF_EX(WmaParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need the input protocol to be STREAM_PULL and the output */
            /* protocol to be STREAM_PULL                                  */
             if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* we need the input media type to be 'audio/wav' */
            if (constructor->spec.input.media_type->id != self->wma_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unknown at this point */
            if (constructor->spec.output.media_type->id != 
                BLT_MEDIA_TYPE_ID_UNKNOWN) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "WmaParser")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not out name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("WmaParserModule::Probe - Ok [%d]", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmaParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(WmaParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(WmaParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(WmaParserModule, WmaParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WmaParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    WmaParserModule_Attach,
    WmaParserModule_CreateInstance,
    WmaParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define WmaParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WmaParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(WmaParserModule,
                                         "WMA Parser",
                                         "com.axiosys.parser.wma",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)

