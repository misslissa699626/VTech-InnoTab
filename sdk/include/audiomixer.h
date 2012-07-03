#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <sys/soundcard.h>

/* length of the audio buffer size */
#define AUDIOMIXER_BUF_SIZE (4 * 1024)

/* length of the authorization key, octets */
#define AUDIOMIXER_KEY_LEN (16)

/* default port for the audiomixer server */
#define AUDIOMIXER_DEFAULT_PORT (5002)

/* default sample rate for the EsounD server */
#define AUDIOMIXER_DEFAULT_RATE (44100)

/* maximum length of a stream/sample name */
#define AUDIOMIXER_NAME_MAX (128)

/* a magic number to identify the relative endianness of a client */
#define AUDIOMIXER_ENDIAN_KEY \
	( (unsigned int) ( ('E' << 24) + ('N' << 16) + ('D' << 8) + ('N') ) )

#define AUDIOMIXER_VOLUME_MAX (255)



/*************************************/
/* what can we do to/with the EsounD */
enum audiomixer_proto { 
    AUDIOMIXER_PROTO_CONNECT,      /* implied on inital client connection */

    /* stream functionality: play, record, monitor */
    AUDIOMIXER_PROTO_STREAM_PLAY,  /* play all following data as a stream */

	/* stream volume */
    AUDIOMIXER_PROTO_STREAM_VOLUME,  /* set stream volume */

	/* stream output buffer info */
    AUDIOMIXER_PROTO_STREAM_OUTPUT_INFO,  /* stream output buffer info */

	/* stream pause/resume */
    AUDIOMIXER_PROTO_STREAM_PAUSE,  /* stream pause */
	AUDIOMIXER_PROTO_STREAM_RESUME,  /* stream resume */

	/* stream flush */
	AUDIOMIXER_PROTO_STREAM_FLUSH,  /* stream flush */

	/* stream clear input buffer */
	AUDIOMIXER_PROTO_CLEAR_INPUT_BUFFER,  /* stream clear input buffer */

    /* free and reclaim /dev/dsp functionality */
    AUDIOMIXER_PROTO_STANDBY,	    /* release /dev/dsp and ignore all data */
    AUDIOMIXER_PROTO_RESUME,	    /* reclaim /dev/dsp and play sounds again */
    
    /* remote management */
    AUDIOMIXER_PROTO_SERVER_INFO,  /* get server info (ver, sample rate, format) */
    
    /* status */
    AUDIOMIXER_PROTO_STANDBY_MODE, /* see if server is in standby, autostandby, etc */

	/* ctrl */
    AUDIOMIXER_PROTO_CTRL_S2M, /* ctrl */

    AUDIOMIXER_PROTO_MAX           /* for bounds checking */
};
    

/******************/
/* The EsounD api */

/* the properties of a sound buffer are logically or'd */

/* bits of stream/sample data */
#define AUDIOMIXER_MASK_BITS	( 0x000F )
#define AUDIOMIXER_BITS8 	( 0x0000 )
#define AUDIOMIXER_BITS16	( 0x0001 )

/* how many interleaved channels of data */
#define AUDIOMIXER_MASK_CHAN	( 0x00F0 )
#define AUDIOMIXER_MONO	( 0x0010 )
#define AUDIOMIXER_STEREO	( 0x0020 )

/* whether it's a stream or a sample */
#define AUDIOMIXER_MASK_MODE	( 0x0F00 )
#define AUDIOMIXER_STREAM	( 0x0000 )

/* the function of the stream/sample, and common functions */
#define AUDIOMIXER_MASK_FUNC	( 0xF000 )
#define AUDIOMIXER_PLAY	( 0x1000 )

/* share memory */
#define AUDIO_SHM "audio_shm"
#define AUDIO_SEM_EMPTY "audio_sem_ept"

typedef int audiomixer_format_t;
typedef int audiomixer_proto_t;

/*******************************************************************/
/* client side API for playing sounds */

typedef unsigned char octet;

enum audiomixer_standby_mode { 
    ESM_ERROR, ESM_ON_STANDBY, ESM_ON_AUTOSTANDBY, ESM_RUNNING
};

typedef struct audiomixer_client_buffer_s {
	unsigned int id;
	unsigned int wp;
	unsigned int rp;
	unsigned int sem_client_cnt;
	unsigned int sem_server_cnt;
	unsigned int total;
	void* buffer;
} audiomixer_client_buffer_t;

typedef struct audiomixer_info_s {
	int format;
	int input_channels;
	int output_channels;
	int rate;
	int bufferSize;
	int triggerSize;
	int volume;
	const char *host;
	const char *name;
} audiomixer_info_t;

typedef struct audiomixer_handle_s {
	int socket;
	int emptySize;
	int bufferSize;
	int triggerSize;
	int id;
	int shm_id;
	int volume;
	audiomixer_client_buffer_t* pBufferInfo;
	sem_t* sem;	/* for accessing emptySize */
} audiomixer_handle_t;

typedef struct audiomixer_server_info_s {
	int format;		// AUDIOMIXER_BITS8, AUDIOMIXER_BITS16
	int channels;	// AUDIOMIXER_MONO, AUDIOMIXER_STEREO
	int rate;
} audiomixer_server_info_t;

typedef struct audiomixer_ctrl_s {
	int socket;
} audiomixer_ctrl_t;

/*******************************************************************/
/* basic audiomixer client interface functions */
audiomixer_handle_t* audiomixer_play(audiomixer_info_t info);
int audiomixer_write(audiomixer_handle_t *handle, void *buffer, int size);
int audiomixer_close(audiomixer_handle_t *handle);
int audiomixer_pause(audiomixer_handle_t *handle);
int audiomixer_resume(audiomixer_handle_t *handle);
int audiomixer_get_free_size(audiomixer_handle_t *handle);
int audiomixer_flush(audiomixer_handle_t *handle);
int audiomixer_set_volume(audiomixer_handle_t *handle, int volume);
int audiomixer_get_output_info(audiomixer_handle_t *handle, audio_buf_info *info);
int audiomixer_clear_input_buffer(audiomixer_handle_t *handle);
int audiomixer_get_server_info(audiomixer_handle_t *handle, audiomixer_server_info_t *info);

/* control functions */
audiomixer_ctrl_t* audiomixer_ctrl_open(void);
int audiomixer_ctrl_s2m(audiomixer_ctrl_t* handle, int enable);
void audiomixer_ctrl_close(audiomixer_ctrl_t *handle);

#ifdef __cplusplus
}
#endif


#endif /* #ifndef AUDIOMIXER_H */
