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
 * @file    gp_line_buffer.c
 * @brief   Implement of line buffer.
 * @author  junp.zhang
 * @since   2010/11/10
 * @date    2010/11/10
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/gp_line_buffer.h>
#include <mach/hal/hal_line_buffer.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LINE_BUFFER_NUM							4
#define LINE_BUFFER_MODULE_NUM					5
#define LINE_BUFFER_MODE_NUM					0x10
#define LINE_BUFFER_MODULE_MSK					0x07

typedef struct mode_info_s {
	char status[5];
	unsigned int modules[LINE_BUFFER_MODULE_NUM];
} mode_info_t;

static mode_info_t mode_flags[LINE_BUFFER_MODE_NUM] = {
	{"DDSS", {0x0c, 0x03, 0x00, 0x00, 0x00}},
	{"DDSG", {0x0c, 0x02, 0x00, 0x01, 0x00}},
	{"DDGG", {0x0c, 0x00, 0x00, 0x03, 0x00}},
	{"DDVV", {0x0c, 0x00, 0x03, 0x00, 0x00}},
	{"DDSR", {0x0c, 0x02, 0x00, 0x00, 0x01}},
	{"DDGR", {0x0c, 0x00, 0x00, 0x02, 0x01}},
	{"DSSR", {0x08, 0x06, 0x00, 0x00, 0x01}},
	{"DSGR", {0x08, 0x04, 0x00, 0x02, 0x01}},
	{"DGGR", {0x08, 0x00, 0x00, 0x06, 0x01}},
	{"DVVR", {0x08, 0x00, 0x06, 0x00, 0x01}},
	{"DVVG", {0x08, 0x00, 0x06, 0x01, 0x00}},
	{"SSGG", {0x00, 0x06, 0x00, 0x03, 0x00}},
	{"SSGR", {0x00, 0x06, 0x00, 0x02, 0x01}},
	{"SGGR", {0x00, 0x08, 0x00, 0x06, 0x01}},
	{"VVGG", {0x00, 0x00, 0x0c, 0x06, 0x00}},
	{"VVGR", {0x00, 0x00, 0x0c, 0x02, 0x01}},
};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct line_buffer_info_s {
	struct semaphore sem;           	/*!< @brief mutex semaphore for line buffer ops */
	wait_queue_head_t request_wait;   	/*!< @brief line buffer request wait queue */
	bool idle;                      	/*!< @brief line buffer idle flag */
	char line_buffer_mask;				/*!< @brief line buffer mask value */
} line_buffer_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static line_buffer_info_t *line_buffer = NULL;

/**
* @brief   look up proper mode from array of mode_flags
*/
static int get_proper_mode(unsigned int module, unsigned int block)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int count = 0;
	unsigned int flag = 0;
	int mode = -1;
	int currMode = -1;

	for (i = 0; i < LINE_BUFFER_MODE_NUM; i++) {
		/*line buffer num is eq block*/
		unsigned int tmp = (mode_flags[i].modules)[module];
		if (tmp <= 0){
			continue;
		}
		count = 0;
		for (j = 0; j < LINE_BUFFER_NUM; j++) {
			if (1 == ((tmp >> j) & 0x01)){
				count++;
			}
		}
		if (block != count) {
			continue;
		}
		/*line buffer is request*/
		if ((line_buffer->line_buffer_mask & tmp) != 0) {
			continue;
		}
		/*no module use line_buffer*/
		if (line_buffer->line_buffer_mask == 0) {
			mode = i;
			DIAG_VERB("!!!no module use line_buffer, can change\n");
			return mode;
		}
		/*get current line buffer info*/
		currMode = gpHalLineBufferGetMode();
		/*switch contition*/
		count = 0;
		for (j = 0; j < LINE_BUFFER_NUM; j++) {
			if (1 == ((line_buffer->line_buffer_mask >> j) & 0x01)) {
				count++;
			}
		}
		flag = 0;
		for (j = 0; j < LINE_BUFFER_NUM; j++) {
			if (flag == count) {
				mode = i;
				return mode;
			}
			if (1 == ((line_buffer->line_buffer_mask >> j) & 0x01)) {
				if((mode_flags[currMode].status)[LINE_BUFFER_NUM-1-j] == \
									(mode_flags[i].status)[LINE_BUFFER_NUM-1-j])\
				{
					flag++;
					continue;
				} 
				else {
					break;
				}
			}
		}
	}
	return mode;
}

/**
* @brief   change line buffer mode
* @param   module [in] module id
* @param   block  [in] request number of line buffer
* @return  SP_OK(0)/ERROR_ID
*/
static int do_line_buffer_mode_change(unsigned int module, unsigned int block)
{
	int mode = 0;

	if (down_interruptible(&line_buffer->sem) != 0) {
		return -ERESTARTSYS;
	}

	while (1) {
		/*look up table*/
		mode = get_proper_mode(module, block);
		if (mode >= 0) {
			break;
		}
		line_buffer->idle = false;
		up(&line_buffer->sem);

		/*sleep*/
		if (wait_event_interruptible(line_buffer->request_wait, line_buffer->idle)) {
			return -ERESTARTSYS;
		}

		if (down_interruptible(&line_buffer->sem) != 0) {
			return -ERESTARTSYS;
		}
	}
	/*set mask & mode*/
	line_buffer->line_buffer_mask |= (mode_flags[mode].modules)[module];
	gpHalLineBufferSetMode(mode);
	up(&line_buffer->sem);

	return SP_OK;
}

/**
* @brief   line buffer request function
* @param   module [in] module id
* @param   length [in] line length in pixels
* @return  module id/-1
*/
static	int get_certain_module(unsigned int module)
{
	int ret = -1;

	switch(module & LINE_BUFFER_MODULE_MSK){
	case LINE_BUFFER_MODULE_DISPLAY:
		ret = LINE_BUFFER_MODULE_DISPLAY;
		break;
	case LINE_BUFFER_MODULE_SCALER:
		ret = LINE_BUFFER_MODULE_SCALER;
		break;
	case LINE_BUFFER_MODULE_VSCALER:
		ret = LINE_BUFFER_MODULE_VSCALER;
		break;
	case LINE_BUFFER_MODULE_2D:
		ret = LINE_BUFFER_MODULE_2D;
		break;
	case LINE_BUFFER_MODULE_ROTATOR:
		ret = LINE_BUFFER_MODULE_ROTATOR;
		break;
	default:
		break;
	}
	return ret;
}

/**
* @brief   line buffer request function
* @param   module [in] module id
* @param   length [in] line length in pixels
* @return  SP_OK(0)/ERROR_ID
*/
int gp_line_buffer_request(unsigned int module, unsigned int length)
{
	int ret = 0;
	unsigned int modu = 0;
	unsigned int num_line_buffer = 0;

	if (length <= 0) {
		DIAG_ERROR("!!!err, negative value of length\n");
		ret = -EINVAL;
		goto out;
	}

	num_line_buffer = (length + 511) / 512;
	if (num_line_buffer > 2) {
		num_line_buffer = 2;
	}

	modu = get_certain_module(module);
	if (modu < 0) {
		DIAG_ERROR("!!!err, module id\n");
		ret = -EINVAL;
		goto out;
	}

	ret = do_line_buffer_mode_change(modu, num_line_buffer);
	if (ret < 0) {
		DIAG_VERB("!!!err, do line buffer mode change\n");
		goto out;
	}

out:
	return ret;
}
EXPORT_SYMBOL(gp_line_buffer_request);

/**
* @brief   line buffer release function
* @param   module [in] module id
* @return  SP_OK(0)/ERROC_ID
*/
int gp_line_buffer_release(unsigned int module)
{
	unsigned int mode = 0;
	unsigned int modu = 0;
	unsigned int tmp = 0;
	int ret = SP_OK;

	/*locate curr mode/module*/
	modu = get_certain_module(module);
	if (modu <= 0) {
		DIAG_ERROR("!!!err, module id\n");
		ret = -EINVAL;
		goto out;
	}

	mode = gpHalLineBufferGetMode();

	if (down_interruptible(&line_buffer->sem) != 0) {
		return -ERESTARTSYS;
	}

	if ((mode_flags[mode].modules)[modu]) {
		tmp = (mode_flags[mode].modules)[modu] & line_buffer->line_buffer_mask;
		if (tmp > 0) {
			line_buffer->line_buffer_mask &= ~tmp;
			/*wake up wait queue*/
			line_buffer->idle = true;
			wake_up_interruptible(&line_buffer->request_wait);
		} 
		else {
			DIAG_ERROR("!!!err, release module not request\n");
			ret = -EINVAL;
			goto out;
		}
	} 
	else {
		DIAG_ERROR("!!!err, module id\n");
		ret = -EINVAL;
		goto out;
	}

out:
	up(&line_buffer->sem);
	return ret;
}
EXPORT_SYMBOL(gp_line_buffer_release);

static int __init line_buffer_init(void)
{
	int ret = -ENXIO;

	line_buffer = (line_buffer_info_t *)kmalloc(sizeof(line_buffer_info_t),  GFP_KERNEL);
	if (line_buffer == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("!!!kmalloc fail\n");
		goto fail_kmalloc;
	}
	memset(line_buffer, 0, sizeof(line_buffer_info_t));

	/* initialize */
	init_MUTEX(&line_buffer->sem);
	init_waitqueue_head(&line_buffer->request_wait);
	gpHalLineBufferClkEnable(1);

	return 0;

/* error rollback */
fail_kmalloc:
	return ret;
}

static void __exit line_buffer_exit(void)
{
	gpHalLineBufferClkEnable(0);
	kfree(line_buffer);
	line_buffer = NULL;
}

module_init(line_buffer_init);
module_exit(line_buffer_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus Line Buffer Module");
MODULE_LICENSE_GP;