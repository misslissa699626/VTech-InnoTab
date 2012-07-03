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
 * @file    gp_ceva.c
 * @brief   Implement of Ceva driver.
 * @author  qinjian
 * @since   2010/10/13
 * @date    2010/10/13
 */
/*#define DIAG_LEVEL DIAG_LVL_VERB*/
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/gp_ceva.h>
#include <mach/gp_chunkmem.h>
#include <mach/hal/hal_ceva.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define CEVA_OK                     0
#define CEVA_ERR                    -1

#define CEVA_CMD_NEED_REPLY         0
#define CEVA_CMD_NO_REPLY           1
#define CEVA_CMD_DATA_MAX_SIZE      256
#define CEVA_REPLY_DATA_MAX_SIZE    2048

/* Ceva timeouts (ms) */
#define CEVA_READY_TIMEOUT          1000
#define CEVA_BOOTED_TIMEOUT         2000
#define CEVA_COMMAND_TIMEOUT        3000
#define CEVA_REPLY_TIMEOUT          8000

/* Ceva commands */
enum CMD_CEVA {
	CX_READ = 1,
	CX_WRITE = 2,
	CX_READ_CHUNK = 3,
	CX_WRITE_CHUNK = 4,
	CX_SHOW_DBGVAR = 5,

	CX_RESERVE_10 = 0x10,
	CX_READ_IO = 0x11,
	CX_WRITE_IO = 0x12,

	/* command group for display relevent drivers on ARM */
	CX_LCD_CMD = 0x40,
	CX_SCALE_ENGINE_CMD,
	CX_LCD_CLEAR_CMD,
	CX_MALLOC_CMD,
	CX_MFREE_CMD,
	CX_DBGMSG_CMD,
	CX_GET_DBGVAR,

	CX_LOAD_BIN = 0x80,
	CX_BOOT = 0x81,
	CX_WAIT_CLOCK_RDY,
	CX_NOTIFY_CLOCK_RDY,
	CX_NOTIFY_TO_CLOSE,

	CX_LOAD_BIN_EX = 0x90,

	CX_PLAY_VIDEO = 0xC0,
	CX_CODEC_TRIGGER = 0xC0,
	CX_CODEC_INIT,
	CX_CODEC_FREE,

	CX_DECODE_FRAME_PC = 0xD0,

	CX_RESET_CEVA_FREE_ARM = 0xF0,
	CX_RESET_CEVA = 0xF1
};

/* Ceva status */
enum CMD_CEVA_STATUS {
	CX_STS_OK = 0,
	CX_STS_ERROR = 1,
	CX_STS_EXE_FAIL = 2,
	CX_STS_INVALID_CMD = 3,

	CX_STS_NOT_AVAILABLE = 0x80,
	CX_STS_SYS_NOT_BOOT = 0xF0
};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define CEVA_DEBUG_THREAD   1
#define REPLY_USE_IRQ       1

#define CHECKSUM \
	({ \
		int i = ceva->ext_size; \
		unsigned int sum_all = 0; \
		unsigned int sum = 0; \
		unsigned char *va = ceva->ext_vbase; \
		unsigned int pa = ceva->ext_base; \
		unsigned int mask = (i >> 4) - 1; \
		/*gp_sync_cache();*/ \
		while (i--) { \
			sum_all += *va; \
			sum += *va; \
			if ((pa & mask) == mask) { \
				DIAG_DEBUG("%08X: %08X\n", pa & ~mask, sum); \
				sum = 0; \
			} \
			va++, pa++; \
		} \
		DIAG_DEBUG("[%s] checksum = %08X\n", __func__, sum_all); \
		sum_all; \
	})

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct ceva_cmd_s {
	unsigned char id;               /*!< @brief command id */
	unsigned char reply;            /*!< @brief need reply:0 / no reply:1 */
	unsigned short data_len;        /*!< @brief command data length */
	unsigned char data[CEVA_CMD_DATA_MAX_SIZE]; /*!< @brief command data */
} ceva_cmd_t;

typedef struct ceva_reply_s {
	unsigned char data_len;         /*!< @brief reply data length */
	unsigned char id;               /*!< @brief indicate which command is replied */
	unsigned char status;           /*!< @brief status for command execution */
	unsigned char reserved;
	unsigned char data[CEVA_REPLY_DATA_MAX_SIZE]; /*!< @brief reply data */
} ceva_reply_t;

typedef struct ceva_info_s {
	struct miscdevice dev;          /*!< @brief ceva device */
	void *ext_vbase;                /*!< @brief start address of ceva ext image region(ka) */
	unsigned int ext_base;          /*!< @brief start address of ceva ext image region(pa) */
	unsigned int ext_size;          /*!< @brief size of ceva ext image region */
	struct semaphore sem;           /*!< @brief mutex semaphore for ceva ops */
	bool opened;                    /*!< @brief ceva device opened flag */
	bool ready;                     /*!< @brief ceva device ready flag */
	bool done;                      /*!< @brief ceva command done flag */
	unsigned int codec_type;        /*!< @brief ceva codec type, 0: decode, 1: encode, 2: game */
	wait_queue_head_t done_wait;    /*!< @brief ceva command done wait queue */
	ceva_cmd_t cmd;                 /*!< @brief ceva command */

	ceva_cmd_t *dbg_cmd;            /*!< @brief ceva debug command */
	ceva_reply_t dbg_reply;         /*!< @brief ceva debug command reply */
#if CEVA_DEBUG_THREAD
	struct task_struct *dbg_task;   /*!< @brief ceva debug task */
	wait_queue_head_t dbg_wait;     /*!< @brief ceva debug command wait queue */
#endif
} ceva_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

static int ceva_open(struct inode *inode, struct file *file);
static int ceva_release(struct inode *inode, struct file *file);
static long ceva_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static const struct file_operations ceva_fops = {
	.owner          = THIS_MODULE,
	.open           = ceva_open,
	.release        = ceva_release,
    .unlocked_ioctl = ceva_ioctl,
};

static ceva_info_t *ceva = NULL;

static unsigned int dbg_flag = DBGFLAG_GLOBAL_MSG_OFF;
module_param(dbg_flag, uint, S_IRUGO);

/**
 * @brief   Ceva debug command process function
 */
static void ceva_dbg_cmd(ceva_cmd_t *cmd)
{
	gp_sync_cache();
	DIAG_VERB("[%s] cmd:%x reply:%d\n", __FUNCTION__, cmd->id, cmd->reply);

	/* prepare reply data */
	memset(&ceva->dbg_reply, 0, sizeof(ceva->dbg_reply));
	ceva->dbg_reply.id = cmd->id;

	/* process ceva debug command */
	switch (cmd->id) {
	case CX_GET_DBGVAR:
		{
			ceva->dbg_reply.data_len = 4;
			*(unsigned int *)ceva->dbg_reply.data = dbg_flag;
			DIAG_DEBUG("[%s] dbg_flag:%08X\n", __FUNCTION__, dbg_flag);
		}
		break;
	case CX_DBGMSG_CMD:
		{
			/* print out ceva debug message */
			DIAG_DEBUG("(CX)%s", (char *)__va(*(unsigned int *)cmd->data));
		}
		break;
	default:
		{
			ceva->dbg_reply.status = CX_STS_INVALID_CMD;
			DIAG_ERROR("[%s] unsupported cmd(%x)!\n", __FUNCTION__, cmd->id);
		}
		break;
	}

	if (cmd->reply == CEVA_CMD_NEED_REPLY) { /* need reply */
		gpHalCevaClearStatus(PIU_STATUS_C1RDS);
		gp_sync_cache();
		gpHalCevaSetCmd(1, __pa(&ceva->dbg_reply));
	}
}

#if CEVA_DEBUG_THREAD
/**
 * @brief   Ceva debug thread
 */
static int ceva_dbg_thread(void *data)
{
	while (!kthread_should_stop()) {
		if (wait_event_timeout(ceva->dbg_wait, ceva->dbg_cmd != NULL, HZ) == 0) {
			continue; /* time out */
		}
		ceva_dbg_cmd(ceva->dbg_cmd);
		ceva->dbg_cmd = NULL;
	}

	return 0;
}
#endif

/**
 * @brief   Ceva irq handler
 */
static irqreturn_t ceva_irq_handler(int irq, void *dev_id)
{
	unsigned int status = gpHalCevaGetStatus();

	DIAG_VERB("[%s] status=%08X\n", __FUNCTION__, status);
	if (status & PIU_STATUS_R0WRS) {
        gpHalCevaClearStatus(PIU_STATUS_R0WRS);
		ceva->done = true;
		wake_up(&ceva->done_wait);
    }
	if (status & PIU_STATUS_R1WRS) {
		gpHalCevaClearStatus(PIU_STATUS_R1WRS);
		ceva->dbg_cmd = (ceva_cmd_t *)__va(gpHalCevaGetReply(1));
#if CEVA_DEBUG_THREAD
		wake_up(&ceva->dbg_wait);
#else
		ceva_dbg_cmd(ceva->dbg_cmd);
#endif
    }

	return IRQ_HANDLED;
}

/**
 * @brief   Ceva execute command
 * @param   cmd [in] ceva command
 * @return  command reply
 * @see
 */
static ceva_reply_t* _ceva_command(ceva_cmd_t *cmd)
{
	ceva_reply_t *reply = NULL;

	gp_sync_cache();
	DIAG_VERB("[%s] %02X\n", __FUNCTION__, cmd->id);

	if (cmd->reply == CEVA_CMD_NEED_REPLY) { /* need reply */
#if REPLY_USE_IRQ
		gpHalCevaEnableIrq(1, PIU_INTMSK_R0WRHIE_EN);

		ceva->done = false;
		gpHalCevaSetCmd(0, __pa(cmd));
		if (wait_event_timeout(ceva->done_wait, ceva->done, CEVA_REPLY_TIMEOUT * HZ / 1000) != 0) {
			reply = (ceva_reply_t *)__va(gpHalCevaGetReply(0));
		}
		else {
			DIAG_ERROR("[%s] %02X fail!\n", __FUNCTION__, cmd->id);
		}

		gpHalCevaEnableIrq(0, PIU_INTMSK_R0WRHIE_EN);
#else
		gpHalCevaSetCmd(0, __pa(cmd));
		/* wait reply */
		if (gpHalCevaWaitStatus(PIU_STATUS_R0WRS, CEVA_REPLY_TIMEOUT) >= 0) {
			reply = (ceva_reply_t *)__va(gpHalCevaGetReply(0));
		}
#endif
		gp_sync_cache();
	}
	else { /* no reply */
		gpHalCevaSetCmd(0, __pa(cmd));
		/* wait ceva has read the command, but need not to wait for its reply */
		gpHalCevaWaitStatus(PIU_STATUS_C0RDS, CEVA_COMMAND_TIMEOUT);
	}

	return reply;
}

/**
 * @brief   Ceva execute command
 * @param   cmd_id [in] command id
 * @param   data_len [in] command data length
 * @param   reply_data [in,out] return reply data, NULL means no reply
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_command(unsigned char cmd_id, unsigned short data_len, unsigned char **reply_data)
{
	int ret = CEVA_OK;
	ceva_reply_t *reply;

	ceva->cmd.id = cmd_id;
	ceva->cmd.data_len = data_len;

	if (reply_data == NULL) {
		ceva->cmd.reply = CEVA_CMD_NO_REPLY;
		_ceva_command(&ceva->cmd);
	}
	else {
		ceva->cmd.reply = CEVA_CMD_NEED_REPLY;
		reply = _ceva_command(&ceva->cmd);
		if (reply != NULL && reply->status == CX_STS_OK) {
			*reply_data = reply->data;
		}
		else {
			ret = CEVA_ERR;
		}
	}

	return ret;
}

/**************************************************************************
 *                        C E V A    C O M M A N D S                      *
 **************************************************************************/

/**
 * @brief   Ceva command: loading binary data
 * @param   load [in] loading paramter
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_load_bin(ceva_load_bin_t *load)
{
	unsigned char *reply_data;
	unsigned int *cmd_data = (unsigned int *)ceva->cmd.data;

	cmd_data[0] = load->type ? 1 : 0;
	cmd_data[1] = load->dst_addr;
	cmd_data[2] = load->size;
	cmd_data[3] = load->src_addr;

	return ceva_command(CX_LOAD_BIN_EX, 16, load->need_reply ? &reply_data : NULL);
}

/**
 * @brief   Ceva command: boot
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_boot(void)
{
	return ceva_command(CX_BOOT, 0, 0);
}

/**
 * @brief   Ceva command: codec decode
 * @param   arg [in] command argument
 * @return  success: used bytes(>= 0),  fail: CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_codec_trigger(void *arg)
{
	int ret;
	unsigned char *reply_data;

	*(unsigned int *)(ceva->cmd.data) = __pa(arg);
	ret = ceva_command(CX_CODEC_TRIGGER, 4, &reply_data);
	if (ret == CEVA_OK) {
		ret = *(int *)reply_data;
	}

	return ret;
}

/**
 * @brief   Ceva command: codec initialize
 * @param   arg [in] command argument
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_codec_init(void *arg)
{
	int ret;
	unsigned char *reply_data;

	*(unsigned int *)(ceva->cmd.data) = __pa(arg);
	ret = ceva_command(CX_CODEC_INIT, 4, &reply_data);
	if (ret == CEVA_OK) {
		ret = *(int *)reply_data;
	}

	return ret;
}

/**
 * @brief   Ceva command: codec free
 * @param   arg [in] command argument
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_codec_free(void *arg)
{
	int ret;
	unsigned char *reply_data;

	*(unsigned int *)(ceva->cmd.data) = __pa(arg);
	ret = ceva_command(CX_CODEC_FREE, 4, &reply_data);
	if (ret == CEVA_OK) {
		ret = *(int *)reply_data;
	}

	return ret;
}

#if 0
/**
 * @brief   Ceva command: read value from io memory
 * @param   addr [in] io memory address
 * @param   value [out] return readed value
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_read_io(unsigned int addr, unsigned int *value)
{
	int ret;
	unsigned char *reply_data;

	((unsigned int *)ceva->cmd.data)[0] = addr;
	ret = ceva_command(CX_READ_IO, 4, &reply_data);
	if (ret == CEVA_OK) {
		*value = ((unsigned int *)reply_data)[0];
	}

	return ret;
}

/**
 * @brief   Ceva command: write value to io memory
 * @param   addr [in] io memory address
 * @param   value [in] write value
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_write_io(unsigned int addr, unsigned int value)
{
	unsigned char *reply_data;

	((unsigned int *)ceva->cmd.data)[0] = addr;
	((unsigned int *)ceva->cmd.data)[1] = value;

	return ceva_command(CX_WRITE_IO, 8, &reply_data);
}

/**
 * @brief   Ceva command: read value from memory
 * @param   addr [in] memory address
 * @param   value [out] return readed value
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_read(unsigned int addr, unsigned int *value)
{
	int ret;
	unsigned char *reply_data;

	((unsigned int *)ceva->cmd.data)[0] = addr;
	ret = ceva_command(CX_READ, 4, &reply_data);
	if (ret == CEVA_OK) {
		*value = ((unsigned int *)reply_data)[0];
	}

	return ret;
}

/**
 * @brief   Ceva command: write value to memory
 * @param   addr [in] memory address
 * @param   value [in] write value
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int ceva_cmd_write(unsigned int addr, unsigned int value)
{
	unsigned char *reply_data;

	((unsigned int *)ceva->cmd.data)[0] = addr;
	((unsigned int *)ceva->cmd.data)[1] = value;

	return ceva_command(CX_WRITE, 8, &reply_data);
}

/**
 * @brief   Ceva command: notify ceva to closing
 * @return  CEVA_OK(0)/CEVA_ERR(-1)
 * @see
 */
static int
ceva_cmd_notify_to_close(void)
{
	unsigned char *reply_data;

	return ceva_command(CX_NOTIFY_TO_CLOSE, 0, &reply_data);
}
#endif

/**
 * @brief   Ceva clock enable/disable
 * @param   enable [in] 0:disable, 1:enable
 * @return  None
 * @see
 */
static void ceva_clock_enable(int enable)
{
	gpHalCevaClkEnable(enable);
}

/**
 * @brief   Ceva close (shutdown)
 * @return  None
 * @see
 */
static void ceva_close(void)
{
	if (ceva->ready) {
		#if 0
		ceva_cmd_notify_to_close();
		#endif
		ceva->ready = false;
	}
	gpHalCevaLock();
}

/**
 * @brief   Ceva reset
 * @param   entry_addr [in] entry address
 * @return  success: 0,  fail: errno
 * @see
 */
static int ceva_reset(unsigned long entry_addr)
{
	gpHalCevaEnableIrq(0, PIU_INTMSK_R2WRHIE_EN |
					      PIU_INTMSK_R1WRHIE_EN |
					      PIU_INTMSK_R0WRHIE_EN);
    gpHalCevaClearStatus(PIU_STATUS_R2WRS |
						 PIU_STATUS_R1WRS |
						 PIU_STATUS_R0WRS);
    gpHalCevaEnableIrq(1, PIU_INTMSK_R1WRHIE_EN);

	gp_sync_cache();
	gpHalCevaReset(entry_addr);
	ceva->ready = (gpHalCevaWaitReady(CEVA_READY_TIMEOUT) >= 0);
	DIAG_DEBUG("CEVA %s\n", ceva->ready ? "READY" : "NOT READY");

	return (ceva->ready ? 0 : -ERESTARTSYS);
}

/**
 * @brief   Ceva boot internal os
 * @return  success: 0,  fail: errno
 * @see
 */
static int ceva_boot(void)
{
	int ret;

	ret = ceva_cmd_boot();
	if (ret == CEVA_OK)	{
		if (gpHalCevaWaitBooted(CEVA_BOOTED_TIMEOUT) < 0) {
			ret = -ERESTARTSYS; /* waiting boot time out */
		}
		else {
			DIAG_DEBUG("CEVA BOOTED\n");
		}
	}

	return ret;
}

/**
 * @brief   Ceva device ioctl function
 */
static long ceva_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	static ceva_video_decode_t *decode;
	static ceva_video_encode_t *encode;
	static unsigned int last_cmd;
	static unsigned char *save;
	static int offset;
	int ret = 0;

	if (down_interruptible(&ceva->sem) != 0) {
		return -ERESTARTSYS;
	}
	if (cmd != last_cmd || cmd == CEVA_IOCTL_TRIGGER) {
		DIAG_VERB("+ceva_ioctl: %08X\n", cmd);
	}

	switch (cmd) {
	case CEVA_IOCTL_RESET:
		{
			unsigned long entry_addr;

			if (copy_from_user(&entry_addr, (void __user*)arg, sizeof(entry_addr))) {
				ret = -EFAULT;
				break;
			}

			ret = ceva_reset(entry_addr);
		}
		break;

	case CEVA_IOCTL_LOAD:
		{
			ceva_load_bin_t load;

			if (copy_from_user(&load, (void __user*)arg, sizeof(ceva_load_bin_t))) {
                ret = -EFAULT;
				break;
            }
			DIAG_VERB("CEVA_IOCTL_LOAD: src addr %08x, dst addr %08x, size %d, type %d\n",
					  load.src_addr, load.dst_addr, load.size, load.type);

			if (load.type < 4) { /* internal codec image */
				/* translate src_addr from user_va to pa */
				load.src_addr = gp_user_va_to_pa((void *)load.src_addr);
				ret = ceva_cmd_load_bin(&load);
			}
			else { /* external codec image */
				if (load.dst_addr + load.size >= ceva->ext_base + ceva->ext_size) {
					DIAG_ERROR("data size too large, dst %08x, size %d\n", load.dst_addr, load.size);
					ret = -EFAULT;
					break;
				}
				copy_from_user(ceva->ext_vbase + (load.dst_addr - ceva->ext_base),
							   (void __user*)load.src_addr,
							   load.size);
			}
		}
		break;

	case CEVA_IOCTL_BOOT:
		ret = ceva_boot();
		break;

	case CEVA_IOCTL_INIT:
		if (ceva->codec_type == CEVA_DECODE) {
			ceva_video_decode_t tmp;

			if (decode != NULL) {
				kfree(decode);
			}
			decode = (ceva_video_decode_t *)kmalloc(sizeof(ceva_video_decode_t), GFP_KERNEL);
			if (decode == NULL) {
				ret = -ENOMEM;
				DIAG_ERROR("CEVA_IOCTL_INIT: out of memory!\n");
				break;
			}
			if (copy_from_user(decode, (void __user*)arg, sizeof(ceva_video_decode_t))) {
				ret = -EFAULT;
				break;
			}

			/* save frame_buf user_va*/
			save = decode->frame_buf;
			/* translate frame_buf from user_va to pa */
			decode->frame_buf = (unsigned char *)gp_user_va_to_pa(decode->frame_buf);
			/* get offset of VA - PA */
			offset = save - decode->frame_buf;

			ret = ceva_cmd_codec_init(decode);
			if (ret != CEVA_ERR) {
				/* copy decode result to tmp */
				memcpy(&tmp, decode, sizeof(ceva_video_decode_t));
				/* restore frame_buf */
				tmp.frame_buf = save;

				if (copy_to_user((void __user*)arg, &tmp, sizeof(ceva_video_decode_t))) {
					ret = -EFAULT;
					break;
				}
			}
		}
		else if (ceva->codec_type == CEVA_ENCODE) {
			ceva_video_encode_t tmp;

			if (encode != NULL) {
				kfree(encode);
			}
			encode = (ceva_video_encode_t *)kmalloc(sizeof(ceva_video_encode_t), GFP_KERNEL);
			if (encode == NULL) {
				ret = -ENOMEM;
				DIAG_ERROR("CEVA_IOCTL_INIT: out of memory!\n");
				break;
			}
			if (copy_from_user(encode, (void __user*)arg, sizeof(ceva_video_encode_t))) {
				ret = -EFAULT;
				break;
			}

			/* save frame_buf user_va*/
			save = encode->frame_buf;

			/* translate frame_buf from user_va to pa */
			encode->frame_buf = (unsigned char *)gp_user_va_to_pa(encode->frame_buf);

			/* get offset of VA - PA */
			offset = save - encode->frame_buf;

			ret = ceva_cmd_codec_init(encode);
			if (ret != CEVA_ERR) {
				/* copy encode result to tmp */
				memcpy(&tmp, encode, sizeof(ceva_video_encode_t));
				/* restore frame_buf */
				tmp.frame_buf = save;

				if (copy_to_user((void __user*)arg, &tmp, sizeof(ceva_video_encode_t))) {
					ret = -EFAULT;
					break;
				}
			}
		}
		break;

	case CEVA_IOCTL_DUMPCODE:
		if (copy_to_user((void __user *)arg, ceva->ext_vbase, 0x100000)) {
			ret = -EFAULT;
		}
		break;

	case CEVA_IOCTL_CHECKSUM:
		while (arg--) {
			CHECKSUM;
		}
		break;

	case CEVA_IOCTL_TRIGGER:
		if ((ceva->codec_type == CEVA_DECODE) && (decode != NULL)) {
			ceva_video_decode_t tmp;

			if (copy_from_user(&tmp, (void __user*)arg, sizeof(ceva_video_decode_t))) {
				ret = -EFAULT;
				break;
			}

			/* save in_buf user_va*/
			save = tmp.in_buf;
			/* translate in_buf from user_va to pa */
			decode->in_buf        = (unsigned char *)gp_user_va_to_pa(tmp.in_buf);
			decode->useful_bytes  = tmp.useful_bytes;
			decode->flags         = tmp.flags;
			decode->time_stamp_in = tmp.time_stamp_in;

			ret = ceva_cmd_codec_trigger(decode);
			if (ret != CEVA_ERR) {
				/* copy decode result to tmp */
				memcpy(&tmp, decode, sizeof(ceva_video_decode_t));
				/* restore in_buf */
				tmp.in_buf = save;
				/* translate out_buf from pa to user_va */
				tmp.frame_buf  += offset;
				tmp.out_buf[0] += offset;
				tmp.out_buf[1] += offset;
				tmp.out_buf[2] += offset;

				if (copy_to_user((void __user *)arg, &tmp, sizeof(ceva_video_decode_t))) {
					ret = -EFAULT;
				}
			}
		}
		else if ((ceva->codec_type == CEVA_ENCODE) && (encode != NULL)) {
			ceva_video_encode_t tmp;
			unsigned char *user_in_buf[3];
			if (copy_from_user(&tmp, (void __user*)arg, sizeof(ceva_video_encode_t))) {
				ret = -EFAULT;
				break;
			}

			/* save in_buf user_va*/
			user_in_buf[0] = tmp.in_buf[0];
			user_in_buf[1] = tmp.in_buf[1];
			user_in_buf[2] = tmp.in_buf[2];

			/* translate in_buf from user_va to pa */
			encode->in_buf[0]     = (unsigned char *)gp_user_va_to_pa(tmp.in_buf[0]);
			encode->in_buf[1]     = (unsigned char *)gp_user_va_to_pa(tmp.in_buf[1]);
			encode->in_buf[2]     = (unsigned char *)gp_user_va_to_pa(tmp.in_buf[2]);
			encode->time_stamp_in = tmp.time_stamp_in;
			memcpy(&encode->width, &tmp.width, (&encode->rc_mode) - (&encode->width));
			encode->flags         = tmp.flags;

			ret = ceva_cmd_codec_trigger(encode);
			if (ret != CEVA_ERR) {
				/* copy encode result to tmp */
				memcpy(&tmp, encode, sizeof(ceva_video_encode_t));
				/* restore in_buf */
				tmp.in_buf[0] = user_in_buf[0];
				tmp.in_buf[1] = user_in_buf[1];
				tmp.in_buf[2] = user_in_buf[2];
				/* translate out_buf from pa to user_va */
				tmp.out_buf += offset;

				if (copy_to_user((void __user *)arg, &tmp, sizeof(ceva_video_encode_t))) {
					ret = -EFAULT;
				}
			}
		}
		break;

	case CEVA_IOCTL_FREE:
		if ((ceva->codec_type == CEVA_DECODE) && (decode != NULL)) {
			ret = ceva_cmd_codec_free(decode);
			kfree(decode);
			decode = NULL;
		}
		else if ((ceva->codec_type == CEVA_ENCODE) && (encode != NULL)) {
			ret = ceva_cmd_codec_free(encode);
			kfree(encode);
			encode = NULL;
		}
		break;

	case CEVA_IOCTL_CODEC_TYPE:
		if (arg > CEVA_GAME) {
			ret = -EINVAL;
		}
		else {
			ceva->codec_type = arg;
		}
		break;

	case CEVA_IOCTL_SET_CLOCK:
		{
			ceva_clock_t clk;

			if (copy_from_user(&clk, (void __user*)arg, sizeof(ceva_clock_t))) {
				ret = -EFAULT;
				break;
			}

			gpHalCevaSetClk(clk.ceva_ratio, clk.ceva_ahb_ratio, clk.ceva_apb_ratio);
		}
		break;

	case CEVA_IOCTL_SET_DBGFLAG:
		dbg_flag = arg;
		break;

	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	if (cmd != last_cmd || cmd == CEVA_IOCTL_TRIGGER) {
		DIAG_VERB("-ceva_ioctl: %08X ret=%d\n", cmd, ret);
		last_cmd = cmd;
	}
	up(&ceva->sem);
	return ret;
}

/**
 * @brief   Ceva device open function
 */
static int ceva_open(struct inode *inode, struct file *file)
{
	if (ceva->opened) {
		DIAG_ERROR("[%s] CEVA already opened!\n", __FUNCTION__);
		return -EBUSY;
	}
	ceva->opened = true;
	ceva_clock_enable(1);
	gpHalCevaLock();

	/* set default codec type */
	ceva->codec_type = CEVA_DECODE;

	return 0;
}

/**
 * @brief   Ceva device release function
 */
static int ceva_release(struct inode *inode, struct file *file)
{
	ceva_close();
	ceva_clock_enable(0);
	ceva->opened = false;
	return 0;
}

#ifdef CONFIG_PM
static int gp_ceva_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (ceva->opened) {
		down(&ceva->sem);
		ceva_clock_enable(0);
	}

	return 0;
}

static int gp_ceva_resume(struct platform_device *pdev)
{
	if (ceva->opened) {
		ceva_clock_enable(1);
		up(&ceva->sem);
	}

	return 0;
}
#else
#define gp_ceva_suspend NULL
#define gp_ceva_resume  NULL
#endif

/**
 * @brief   Ceva device & driver define
 */
static struct platform_device gp_ceva_device = {
	.name	= "gp-ceva",
	.id	= -1,
};

static struct platform_driver gp_ceva_driver = {
	.suspend = gp_ceva_suspend,
	.resume  = gp_ceva_resume,
	.driver  = {
		.owner  = THIS_MODULE,
		.name   = "gp-ceva"
	},
};

/**
 * @brief   Ceva driver init function
 */
static int __init ceva_init(void)
{
	int ret = -ENXIO;

	ceva = (ceva_info_t *)kzalloc(sizeof(ceva_info_t), GFP_KERNEL);
	if (ceva == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("ceva kmalloc fail\n");
		goto fail_kmalloc;
	}

	ceva->ext_base = CEVA_EXT_BASE;
	ceva->ext_size = CEVA_EXT_SIZE;
	ceva->ext_vbase = __va(ceva->ext_base);

	/* request ceva irq */
	ret = request_irq(IRQ_PIU_CM,
					  ceva_irq_handler,
					  IRQF_DISABLED,
					  "CEVA_IRQ",
					  ceva);
	if (ret < 0) {
		DIAG_ERROR("ceva request irq fail\n");
		goto fail_request_irq;
	}

	/* initialize */
	init_MUTEX(&ceva->sem);
	init_waitqueue_head(&ceva->done_wait);
#if CEVA_DEBUG_THREAD
	init_waitqueue_head(&ceva->dbg_wait);
	ceva->dbg_task = kthread_run(ceva_dbg_thread, NULL, "ceva_dbg");
	if (IS_ERR(ceva->dbg_task)) {
		DIAG_ERROR("ceva dbg task create fail\n");
		goto fail_run_dbg_task;
	}
#endif

	/* register device */
	ceva->dev.name  = "ceva";
	ceva->dev.minor = MISC_DYNAMIC_MINOR;
	ceva->dev.fops  = &ceva_fops;
	ret = misc_register(&ceva->dev);
	if (ret != 0) {
		DIAG_ERROR("ceva device register fail\n");
		goto fail_device_register;
	}

	gpHalCevaSetClk(0, 1, 1); /* set ceva clock to 320MHz */

	platform_device_register(&gp_ceva_device);
	platform_driver_register(&gp_ceva_driver);
	return 0;

	/* error rollback */
fail_device_register:
#if CEVA_DEBUG_THREAD
	kthread_stop(ceva->dbg_task);
fail_run_dbg_task:
#endif
	free_irq(IRQ_PIU_CM, ceva);
fail_request_irq:
	kfree(ceva);
	ceva = NULL;
fail_kmalloc:
	return ret;
}

/**
 * @brief   Ceva driver exit function
 */
static void __exit ceva_exit(void)
{
	misc_deregister(&ceva->dev);
#if CEVA_DEBUG_THREAD
	kthread_stop(ceva->dbg_task);
#endif
	free_irq(IRQ_PIU_CM, ceva);
	kfree(ceva);
	ceva = NULL;

	platform_device_unregister(&gp_ceva_device);
	platform_driver_unregister(&gp_ceva_driver);
}

module_init(ceva_init);
module_exit(ceva_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus Ceva Driver");
MODULE_LICENSE_GP;
