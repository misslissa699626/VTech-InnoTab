#ifndef __FILTER2_H__
#define __FILTER2_H__

#include "mediasys.h"

#define PACKET_FLAG_LAST		1

typedef enum
{
	SYS_CMD_PLAY = 0,
	SYS_CMD_STOP,
	SYS_CMD_EXIT,
} FILTER_CMD;

////////////////////////////////////////
//
// Frame Info
//
////////////////////////////////////////
#define N_FRAME_INFO		16
typedef struct
{
	int timeStamp;
	int duration;
	int start;
	int end;
} FRAMEINFO;

typedef enum
{
	CLK_CHANNEL_AUDIO,
	CLK_CHANNEL_VIDEO,
	CLK_CHANNEL_SUBTITLE,
} CLK_CHANNEL;

typedef enum
{
	FILTER_MSG_TIMEOUT,
	FILTER_MSG_PACKET_RETURN,
	FILTER_MSG_PACKET_IN,
	FILTER_MSG_SYS_CMD,
	FILTER_MSG_CMD,
} FILTER_MSG;

// packet header
typedef struct
{
	int			flag;
	const char	*data;
	int			data_len;
	const char	*info;
	const char	*extra;
	int			extra_len;
	int			time;		// msec
	int			duration;	// msec
	int			seq;
} DATAPKTHDR;


typedef struct
{
	int cmd;
	int param;
	int ret;
} CMDPKTHDR;

typedef struct filterOp_s
{
	const char *pname;
	int		(*init)(void *hFilter ); /* return the allocated user data */
	int		(*deinit)(void *hFilter );
	int		(*process)(void *hFilter, FILTER_MSG msg, int cmd);

	// properties
	int		nPkt;	
} filterOp_t;


// packet2 ////////////////////////////////////////////////
/*!
 * @breif  Free packet
 * @param  hPacket2 : packet void *
 */
int packet_Free(void *hPkt);

/*!
 * @breif  Free packet
 * @param  hPacket2 : packet void *
 * @return packet header
 */
const DATAPKTHDR *packet_GetHdr(void *hPkt);
void packet_SetHdr(void *hPkt, const DATAPKTHDR *Hdr);

/*!
 * @breif  Set packet type
 * @param  hPacket2 :  packet void *
 * @return type of packet
 */
int packet_GetType(void *hPkt);

// filter ////////////////////////////////////////////////

/*!
 * @breif Set the private data of a filter.
 * @param hFilter The void *of filter.
 * @param pdata The private data.
 * @return SP_OK Success, else fail.
 */
int filter_SetPrivData(void *hFilter, void *pdata );

/*!
 * @breif Get the private data of a filter.
 * @param hFilter The void *of filter.
 * @return The pointer to the private data.
 */
void *filter_GetPrivData(void *hFilter );


/*!
 * @brief Put packet to a queue.
 * @param hFilter The void *of the filter.
 * @return SP_OK If the packet is inserted into the queue successfully, else fail.
 */
int filter_PushPacket(void *hFilter, void *packet);

/*!
 * @breif Game the name of filter
 * @param hFilter The void *of filter.
 */
const char *filter_GetName(void *hFilter );


/*!
 * @breif  Get input packet
 * @param  hFilter   : filter void *
 * @return SP_OK / SP_FAIL
 */
void *filter_GetInPacket(void *hFilter);
int filter_GetInPacketNumber(void *hFilter);

/*!
 * @breif  Get output packet
 * @param  hFilter   : filter void *
 * @return void *of packet
 */
void *filter_GetEmptyPacket(void *hFilter);
int filter_GetEmptyPacketNumber(void *hFilter);


int filter_GetDebugLevel(void *hFilter);

int filter_NotifyUser(void *hFilter, int msg);

void *filter_GetMediaSystem(void *hFilter);


int mediaSystem_ClkStart(void *hMedia, int ch);
int mediaSystem_ClkStop(void *hMedia, int ch);
int mediaSystem_SetExtClkSrc(void *hMedia, int (*timeExtGet)(void *param), void *param);

#endif // __FILTER2_H__
