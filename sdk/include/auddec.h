#ifndef __AUD_DEC_H__
#define __AUD_DEC_H__
	
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************
 * Header Files
 ***************************************************************************/

/***************************************************************************
 * Constants
 ***************************************************************************/
 
#define AUDEC_OK              0
#define AUDEC_ERR             -1 /* general error */
#define AUDEC_ERR_INPUT       -2 /* function input parameter invalid */
#define AUDEC_ERR_HEAD        -3 /* find header, but it's some parameters error */
#define AUDEC_ERR_SYNC        -4 /* not find sync code for sync code bitstream */
#define AUDEC_ERR_SUPPORT     -5 /* header is ok, but not support decoding the bitstream*/
#define AUDEC_ERR_MEM         -6 /* out buffer not enough for current decoded pcm */
#define AUDEC_ERR_DOWNFLOW    -7 /* bitstream not enough, need more data */
#define AUDEC_ERR_FRAME       -8 /* current frame bitstream error */
#define AUDEC_ERR_FATAL       -9 /* fatal error, need init to decode again */



/***************************************************************************
 * Macros
 ***************************************************************************/

/***************************************************************************
 * Data Types
 ***************************************************************************/
 
/*
 * audio decode parameter
 */
typedef struct {
    /* follow from parser */
    int					codec_id; /* see CODEC_ID_xxx in gp_avcodec.h */
    int					sample_rate;
    int					channels;
    int					bit_rate;
    int					block_align; 
	const char			*extradata; /* some audio head data*/
    int					extradata_size; /* head data size */
    	
    /* follow from decode control part */ 
    void 				*hDecoder; /* audio decoder instance, MUST be 4-byte alignment */ 

    /* follow from decoder */
    int					out_sample_rate;
    int					out_channels;
    int					out_bit_rate;
    int					out_fmt;
    int					delay_size; /* delay data size in decoder*/
    
    // Bitstream Ring Buffer
    char	 			*Ring;
    int					RingSize;
    int					RingRI;
    int					RingWI;
    
    const char *es_data; /* element stream data */
    int es_size;  /* element stream data size, unit in byte*/ 
	
    /* follow from decode control part */ 
    void *audec_buf; /* audio decoder instance buffer, align to 4 bytes boundary */ 
} audec_param_t;
/*
 * audio decode interface
 */
typedef struct audec_interface_s
{
    /* @description: get audio decoder instrance size
        * @return: audio decoder instrance size.
       */ 
    unsigned int (*instance_size)(void);
    /* @description: initialize audio decoder 
        * @param: *adp[in] the pointer of audio decode parameter struct 
        * @return: On error a negative value is returned, otherwise otherwise the number 
        * of bytes used. 
        */ 
    int (*init)(audec_param_t *adp);
    /* @description: decode one audio frame 
        * @param: *adp[in] the pointer of audio es infomation 
        * @param: out_buf[out] the pointer of the output buffer 
        * @param: out_size[in\out] input the output buffer size, return the output pcm size 
        * @return: On error a negative value is returned, otherwise the number of bytes 
        * used or zero if no frame could be decompressed. 
        */ 
    int (*dec)(audec_param_t *adp, unsigned char *out_buf, int *out_size); 
    /* @description: uninitialize audio decoder 
        * @param: *adp[in] the pointer of audio decode parameter struct
        * @return: On error a negative value is returned, otherwise 0. 
        */ 
    int (*uninit)(audec_param_t *adp);	
    const char *description;
} audec_interface_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AUD_DEC_H__ */ 
