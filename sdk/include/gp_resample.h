/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file gp_resample.h
 * @brief
 * @author
 */

#ifndef _GP_RESAMPLE_H
#define  _GP_RESAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define GP_RESAMPLE_MAX_VOLUME 255

enum {
	GP_RESAMPLE_FORMAT_8BIT,
	GP_RESAMPLE_FORMAT_16BIT,
	GP_RESAMPLE_FORMAT_32BIT,
} gp_resample_format;

enum {
	GP_RESAMPLE_LINEAR = 1,
	GP_RESAMPLE_CUBIC = 2,
};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_resample_tag gp_resample_state_t ;

typedef struct gp_resample_config_s {
	int input_format;
	int output_format;
	int input_sample_rate;
	int output_sample_rate;
	int converter_type;
	int input_channels;
	int output_channels;
} gp_resample_config_t;

typedef struct gp_resample_data_s {
	void *input;
	void *output;
	int input_frames;	/* bytes of one channel */
	int output_frames;	/* bytes of one channel */
	int input_frames_used;
	int output_frames_gen;
	int end_of_input;
	int volume;
} gp_resample_data_t;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
gp_resample_state_t* gp_resample_new(gp_resample_config_t config, int* error);
gp_resample_state_t* gp_resample_delete (gp_resample_state_t *state);
int gp_resample_process (gp_resample_state_t *state, gp_resample_data_t *data);
int gp_resample_reset (gp_resample_state_t *state);


#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#endif	/* _GP_RESAMPLE_H */

