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
#include <mach/gp_cache.h>
#include <mach/gp_chunkmem.h>
#include <mach/hal/hal_ceva.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_vic.h>
#include <mach/hal/regmap/reg_scu.h>

#include "ceva_nop.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define CEVA_CMD_NEED_REPLY         0
#define CEVA_CMD_NO_REPLY           1
#define CEVA_CMD_DATA_MAX_SIZE      256
#define CEVA_REPLY_DATA_MAX_SIZE    2048

/* Ceva timeouts (ms) */
#define CEVA_READY_TIMEOUT          1000
#define CEVA_BOOTED_TIMEOUT         2000
#define CEVA_COMMAND_TIMEOUT        3000
#define CEVA_REPLY_TIMEOUT          8000

#define VFLAG_FRAMEOUT  0x10000000  /* [out] output frame available */
#define VFLAG_YUV422OUT 0x20000000  /* [out] output YUV422 instead of YUV420 */
#define VFLAG_YUV420IN  0x00000001  /* [in] input YUV420 instead of YUV422 */
#define VFLAG_ARGBOUT   0x00000040  /* [in] output ARGB */
#define VFLAG_RGB565OUT 0x00000100  /* [in] output RGB565 */

#define VFLAG_SP_OUT    (VFLAG_ARGBOUT|VFLAG_RGB565OUT) /* special output */

// #define AUTO_LOAD_IMAGE

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

/* Ceva codec sub types */
enum {
	CEVA_CODEC_TYPE_NONE = 0,
	CEVA_CODEC_TYPE_MPEG4_SP,       // decoder
	CEVA_CODEC_TYPE_MPEG4_ASP,
	CEVA_CODEC_TYPE_MJPEG,
	CEVA_CODEC_TYPE_H263,
	CEVA_CODEC_TYPE_S263,
	CEVA_CODEC_TYPE_H264_BP,
	CEVA_CODEC_TYPE_H264_MP,
	CEVA_CODEC_TYPE_H264_HP,
	CEVA_CODEC_TYPE_WMV7,
	CEVA_CODEC_TYPE_WMV8,           // 10
	CEVA_CODEC_TYPE_VC1_SP,
	CEVA_CODEC_TYPE_VC1_AP,
	CEVA_CODEC_TYPE_MPEG2,
	CEVA_CODEC_TYPE_MPEG1,
	CEVA_CODEC_TYPE_RV10,
	CEVA_CODEC_TYPE_RV20,
	CEVA_CODEC_TYPE_RV30,
	CEVA_CODEC_TYPE_RV40,
	CEVA_CODEC_TYPE_DIV3,
	CEVA_CODEC_TYPE_JPG,            // 20
	CEVA_CODEC_TYPE_JPG_ENCODE,     // encoder
	CEVA_CODEC_TYPE_MPEG4_ENCODE,
	CEVA_CODEC_TYPE_H264_ENCODE,
	CEVA_CODEC_TYPE_MJPG_ENCODE,
	CEVA_CODEC_TYPE_VP6,
	CEVA_CODEC_TYPE_MPEG4_HD,
	CEVA_CODEC_TYPE_THEORA,
	CEVA_CODEC_TYPE_KGB             // game
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
	bool resumed;                   /*!< @brief ceva device resumed flag */
	unsigned int codec_main_type;   /*!< @brief ceva codec main type: DECODE/ENCODE/GAME/etc. */
	unsigned int codec_sub_type;    /*!< @brief ceva codec sub type: CEVA_CODEC_TYPE_XXX */
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
 * @brief   Ceva register dump
 */
static void ceva_reg_dump(void)
{
	scuaReg_t *scuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	scucReg_t *scucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	DIAG_PRINTF("=== CEVA CLOCK REGS ===\n");
	DIAG_PRINTF("scuaReg->scuaPeriClkEn2 = %08X\n", scuaReg->scuaPeriClkEn2);
	DIAG_PRINTF("scucReg->scucPeriClkEn  = %08X\n", scucReg->scucPeriClkEn);
	DIAG_PRINTF("scucReg->scucCevaCntEn  = %08X\n", scucReg->scucCevaCntEn);
}

/**
 * @brief   Ceva debug command process function
 */
static void ceva_dbg_cmd(ceva_cmd_t *cmd)
{
#ifndef GP_SYNC_OPTION
	gp_invalidate_dcache_range((unsigned int)cmd, sizeof(ceva_cmd_t) - CEVA_CMD_DATA_MAX_SIZE + cmd->data_len);
#else
	GP_SYNC_CACHE();
#endif
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
#ifndef GP_SYNC_OPTION
		gp_clean_dcache_range((unsigned int)&ceva->dbg_reply, sizeof(ceva_reply_t) - CEVA_REPLY_DATA_MAX_SIZE + ceva->dbg_reply.data_len);
#else
	GP_SYNC_CACHE();
#endif
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

#ifndef GP_SYNC_OPTION
	gp_clean_dcache_range((unsigned int)cmd, sizeof(ceva_cmd_t) - CEVA_CMD_DATA_MAX_SIZE + cmd->data_len);
#else
	GP_SYNC_CACHE();
#endif
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
			ceva_reg_dump();
		}

		gpHalCevaEnableIrq(0, PIU_INTMSK_R0WRHIE_EN);
#else
		gpHalCevaSetCmd(0, __pa(cmd));
		/* wait reply */
		if (gpHalCevaWaitStatus(PIU_STATUS_R0WRS, CEVA_REPLY_TIMEOUT) >= 0) {
			reply = (ceva_reply_t *)__va(gpHalCevaGetReply(0));
		}
#endif
#ifndef GP_SYNC_OPTION
		if (reply != NULL) {
			gp_invalidate_dcache_range((unsigned int)reply, sizeof(ceva_reply_t) - CEVA_REPLY_DATA_MAX_SIZE + reply->data_len);
		}
#else
	GP_SYNC_CACHE();
#endif
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
 * @brief   Ceva command: codec exec
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
#endif

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

/**
 * @brief   Ceva clock enable/disable
 * @param   enable [in] 0:disable, 1:enable
 * @return  None
 * @see
 */
static void ceva_clock_enable(int enable)
{
	gpHalScuClkEnable(SCU_C_PERI_DMAC1 | SCU_C_PERI_CXMP_SL | SCU_C_PERI_CXMD_SL, SCU_C, enable);
	gpHalScuClkEnable(SCU_A_PERI_CEVA_L2RAM , SCU_A2, enable);
	gpHalCevaClkEnable(enable);
#if 1
	DIAG_DEBUG(">>> ceva_clock_enable(%d)\n", enable);
	ceva_reg_dump();
#endif
}

/**
 * @brief   Ceva close (shutdown)
 * @return  None
 * @see
 */
static void ceva_close(void)
{
	if (ceva->ready) {
#ifndef AUTO_LOAD_IMAGE
		ceva_cmd_notify_to_close();
#endif
		ceva->ready = false;
	}
#ifndef AUTO_LOAD_IMAGE
	gpHalCevaLock();
#endif
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

#ifdef GP_SYNC_OPTION
	GP_SYNC_CACHE();
#endif
	gpHalCevaReset(entry_addr);
	ceva->ready = (gpHalCevaWaitReady(CEVA_READY_TIMEOUT) >= 0);
	DIAG_DEBUG("CEVA %s\n", ceva->ready ? "READY" : "NOT READY");
	if (!ceva->ready) {
		ceva_reg_dump();
	}

	return (ceva->ready ? 0 : -ERESTARTSYS);
}

#include <mach/gp_version.h>
#include <mach/hardware.h>
typedef void (*check_xdma_finish_t)(void);
check_xdma_finish_t check_xdma_finish_do = NULL;

void check_xdma_finish(void)
{
	#define		MMP_DMAC1_M410_BASE 		0x1000		// DMA 1 base address
	int pending_transfers = 0;
	int loop = 0;
	volatile UINT32 *ptr = (volatile UINT32 *)(IO2_ADDRESS(MMP_DMAC1_M410_BASE + 0x38));
    do
    {
        pending_transfers = (*ptr & 0x00ff0000) >> 16;
		if(pending_transfers && loop > 1000){
			printk("pending_transfers=%x loop=%d \n",pending_transfers,loop);
		}
		loop++;
		udelay(10);
	} while (pending_transfers && loop < 100000);   //no more data transfers
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
			ceva_reg_dump();
			ret = -ERESTARTSYS; /* waiting boot time out */
		}
		else {
			DIAG_DEBUG("CEVA BOOTED\n");
		}
	}

	if(gp_version_get() == GPL329XXA_VER_A){
		check_xdma_finish_do = check_xdma_finish;
	}
	else{
		check_xdma_finish_do = NULL;
	}

	return ret;
}

/**
 * @brief   Ceva run NOP code
 */
int gp_ceva_nop(void)
{
	if (ceva->opened) {
		return -EBUSY;
	}

	ceva_clock_enable(1);
	gpHalCevaLock();
	memcpy(ceva->ext_vbase + 0x10000, cevaNopCodec, sizeof(cevaNopCodec));
	return ceva_reset(ceva->ext_base + 0x10000);
}
EXPORT_SYMBOL(gp_ceva_nop);


/**
 * @brief   Ceva device ioctl function
 */
static long ceva_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	static ceva_video_decode_t *decode = NULL;
	static ceva_video_encode_t *encode = NULL;
	static ceva_game_kgb_t *kgb = NULL;
	static unsigned int last_cmd;
	static unsigned char *save;
	static int offset;
	int ret = 0;

	if (ceva->resumed) { /* ceva is resumed from suspend state */
		ceva->resumed = false;
		if (cmd == CEVA_IOCTL_INIT
			|| cmd == CEVA_IOCTL_TRIGGER
			|| cmd == CEVA_IOCTL_FREE) { /* these cmd need reload ceva codec */
			return CEVA_ERR_RESUMED; /* notify libceva do re-init action */
		}
	}

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
#ifndef GP_SYNC_OPTION
				gp_clean_dcache_range(load.src_addr, load.size);
#endif
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
#if 1
				udelay(1000); /* TODO: need fix it */
				udelay(1000); /* TODO: need fix it */
				udelay(1000); /* TODO: need fix it */
#endif
				copy_from_user(ceva->ext_vbase + (load.dst_addr - ceva->ext_base),
							   (void __user*)load.src_addr,
							   load.size);
#ifndef GP_SYNC_OPTION
				gp_clean_dcache_range((unsigned int)ceva->ext_vbase + (load.dst_addr - ceva->ext_base), load.size);
#endif
			}
		}
		break;

	case CEVA_IOCTL_BOOT:
		offset = -1;
		ret = ceva_boot();
		break;

	case CEVA_IOCTL_INIT:
		if (ceva->codec_main_type == CEVA_DECODE) {
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
			DIAG_VERB(">>> CEVA_IOCTL_INIT: %p - %p\n", save, save + decode->frame_buf_size);

#ifndef GP_SYNC_OPTION
			gp_clean_dcache_range((unsigned int)decode, sizeof(ceva_video_decode_t));
#endif
			ret = ceva_cmd_codec_init(decode);
			if (ret != CEVA_ERR) {
#ifndef GP_SYNC_OPTION
				gp_invalidate_dcache_range((unsigned int)decode, sizeof(ceva_video_decode_t));
#endif
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
		else if (ceva->codec_main_type == CEVA_ENCODE) {
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

#ifndef GP_SYNC_OPTION
			gp_clean_dcache_range((unsigned int)encode, sizeof(ceva_video_encode_t));
#endif
			ret = ceva_cmd_codec_init(encode);
			if (ret != CEVA_ERR) {
#ifndef GP_SYNC_OPTION
				gp_invalidate_dcache_range((unsigned int)encode, sizeof(ceva_video_encode_t));
#endif
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
		else if (ceva->codec_main_type == CEVA_GAME) {
			switch (ceva->codec_sub_type) {
			case CEVA_CODEC_TYPE_KGB:
				if (kgb != NULL) {
					kfree(kgb);
				}
				kgb = (ceva_game_kgb_t *)kmalloc(sizeof(ceva_game_kgb_t), GFP_KERNEL);
				if (kgb == NULL) {
					ret = -ENOMEM;
					DIAG_ERROR("CEVA_IOCTL_INIT: out of memory!\n");
					break;
				}
				ret = ceva_command(CX_CODEC_INIT, 4, NULL);
				break;
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
		if ((ceva->codec_main_type == CEVA_DECODE) && (decode != NULL)) {
			ceva_video_decode_t tmp;
			unsigned char *save_out_buf_va = NULL;

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

			if (((tmp.flags & VFLAG_SP_OUT) != 0) && (ceva->codec_sub_type != CEVA_CODEC_TYPE_JPG)) {
				/* Using 'version' as a pointer of special output(ARGB/RGB565) temporarlity !! */
				/* TODO: This need to dicuss with internel memeber for createing a new buffer pointer */
				save_out_buf_va = tmp.version;
				if (save_out_buf_va == NULL) {
					DIAG_ERROR("ARGB/RGB565 output buffer address(vdt.version) can't be NULL!\n");
					ret = -EINVAL;
					break;
				}
				decode->version = (unsigned char *)gp_user_va_to_pa(save_out_buf_va);
			}

#ifndef GP_SYNC_OPTION
			gp_clean_dcache_range((unsigned int)tmp.in_buf, tmp.useful_bytes);
			gp_clean_dcache_range((unsigned int)decode, sizeof(ceva_video_decode_t));
#endif
			ret = ceva_cmd_codec_trigger(decode);
			if (ret != CEVA_ERR) {
#ifndef GP_SYNC_OPTION
				gp_invalidate_dcache_range((unsigned int)decode, sizeof(ceva_video_decode_t));
#endif
				/* copy decode result to tmp */
				memcpy(&tmp, decode, sizeof(ceva_video_decode_t));
				/* restore in_buf */
				tmp.in_buf = save;
				/* translate out_buf from pa to user_va */
				tmp.frame_buf += offset;
				DIAG_VERB(">>> CEVA_IOCTL_TRIGGER: %d %d %d %d\n", tmp.width, tmp.height, tmp.stride, tmp.stride_chroma);
				if ((tmp.flags & VFLAG_FRAMEOUT) && tmp.height) {
					if (save_out_buf_va != NULL) {
						tmp.out_buf[0] = save_out_buf_va; /* restore out_buf */
					}
					else {
						tmp.out_buf[0] += offset;
						if (tmp.out_buf[1] != NULL && (tmp.flags & VFLAG_SP_OUT) == 0) { /* have uv data ? */
							tmp.out_buf[1] += offset;
							tmp.out_buf[2] += offset;
						}
					}
#ifndef GP_SYNC_OPTION
					DIAG_VERB(">>> CEVA_IOCTL_TRIGGER: (Y) %p - %p\n", tmp.out_buf[0], tmp.out_buf[0] + tmp.height * tmp.stride);
					gp_invalidate_dcache_range((unsigned int)tmp.out_buf[0], tmp.height * tmp.stride);
					if (tmp.out_buf[1] != NULL && (tmp.flags & VFLAG_SP_OUT) == 0) { /* have uv data ? */
						unsigned int uv_size = tmp.height / ((tmp.flags & VFLAG_YUV422OUT)?1:2) * tmp.stride_chroma;
						DIAG_VERB("                        (U) %p - %p\n", tmp.out_buf[1], tmp.out_buf[1] + uv_size);
						DIAG_VERB("                        (V) %p - %p\n", tmp.out_buf[2], tmp.out_buf[2] + uv_size);
						gp_invalidate_dcache_range((unsigned int)tmp.out_buf[1], uv_size);
						gp_invalidate_dcache_range((unsigned int)tmp.out_buf[2], uv_size);
					}
#endif
				}

				if (copy_to_user((void __user *)arg, &tmp, sizeof(ceva_video_decode_t))) {
					ret = -EFAULT;
				}
			}
		}
		else if ((ceva->codec_main_type == CEVA_ENCODE) && (encode != NULL)) {
			ceva_video_encode_t tmp;
			unsigned char *user_in_buf[3];
			if (copy_from_user(&tmp, (void __user*)arg, sizeof(ceva_video_encode_t))) {
				ret = -EFAULT;
				break;
			}

			/* codec sub type special process */
			switch (ceva->codec_sub_type) {
			case CEVA_CODEC_TYPE_MPEG4_ENCODE:
				/* process out_buf */
				if (tmp.out_buf != NULL) {
					/* translate out_buf from user_va to pa */
					encode->out_buf = (unsigned char *)gp_user_va_to_pa(tmp.out_buf);
					/* get offset of VA - PA */
					offset = tmp.out_buf - encode->out_buf;
				}
				break;
			}

			/* save in_buf user_va*/
			user_in_buf[0] = tmp.in_buf[0];
			user_in_buf[1] = tmp.in_buf[1];
			user_in_buf[2] = tmp.in_buf[2];

			/* translate in_buf from user_va to pa */
			encode->in_buf[0] = (unsigned char *)gp_user_va_to_pa(tmp.in_buf[0]);
			if (tmp.in_buf[1] != NULL) { /* have uv data ? */
				encode->in_buf[1] = (unsigned char *)gp_user_va_to_pa(tmp.in_buf[1]);
				encode->in_buf[2] = (unsigned char *)gp_user_va_to_pa(tmp.in_buf[2]);
			}
			encode->time_stamp_in = tmp.time_stamp_in;
			memcpy(&encode->width, &tmp.width, (&encode->rc_mode) - (&encode->width));
			encode->flags         = tmp.flags;

#ifndef GP_SYNC_OPTION
			gp_clean_dcache_range((unsigned int)tmp.in_buf[0], tmp.height * tmp.width);
			if (tmp.in_buf[1] != NULL) { /* have uv data ? */
				gp_clean_dcache_range((unsigned int)tmp.in_buf[1], tmp.height * tmp.width / ((tmp.flags & VFLAG_YUV420IN)?4:2));
				gp_clean_dcache_range((unsigned int)tmp.in_buf[2], tmp.height * tmp.width / ((tmp.flags & VFLAG_YUV420IN)?4:2));
			}
			gp_clean_dcache_range((unsigned int)encode, sizeof(ceva_video_encode_t));
#endif
			ret = ceva_cmd_codec_trigger(encode);
			if (ret != CEVA_ERR) {
#ifndef GP_SYNC_OPTION
				gp_invalidate_dcache_range((unsigned int)encode, sizeof(ceva_video_encode_t));
#endif
				/* copy encode result to tmp */
				memcpy(&tmp, encode, sizeof(ceva_video_encode_t));
				/* restore in_buf */
				tmp.in_buf[0] = user_in_buf[0];
				tmp.in_buf[1] = user_in_buf[1];
				tmp.in_buf[2] = user_in_buf[2];

				if (tmp.out_buf != NULL && tmp.out_bytes != 0) {
					/* translate out_buf from pa to user_va */
					tmp.out_buf += offset;
#ifndef GP_SYNC_OPTION
					gp_invalidate_dcache_range((unsigned int)tmp.out_buf, tmp.out_bytes);
#endif
				}

				if (copy_to_user((void __user *)arg, &tmp, sizeof(ceva_video_encode_t))) {
					ret = -EFAULT;
				}
			}
		}
		else if (ceva->codec_main_type == CEVA_GAME) {
			switch (ceva->codec_sub_type) {
			case CEVA_CODEC_TYPE_KGB:
				{
					if (copy_from_user(kgb, (void __user*)arg, sizeof(ceva_game_kgb_t))) {
						ret = -EFAULT;
						break;
					}

					/* translate from VA to PA */
					if (offset == -1) {
						offset = (unsigned char *)gp_user_va_to_pa(kgb->pOutBuf) - kgb->pOutBuf;
					}
					kgb->pOutBuf  += offset;
					kgb->pVramBuf += offset;
					kgb->pOamBuf = (unsigned short *)(((unsigned char *)kgb->pOamBuf) + offset);
					kgb->pRegBuf = (unsigned short *)(((unsigned char *)kgb->pRegBuf) + offset);

					*(unsigned int *)(ceva->cmd.data) = __pa(kgb);
					ret = ceva_command(CX_CODEC_TRIGGER, 4, NULL);
				}
				break;
			}
		}

		if(check_xdma_finish_do)
			check_xdma_finish_do();

		break;

	case CEVA_IOCTL_FREE:
		if ((ceva->codec_main_type == CEVA_DECODE) && (decode != NULL)) {
			ret = ceva_cmd_codec_free(decode);
			kfree(decode);
			decode = NULL;
		}
		else if ((ceva->codec_main_type == CEVA_ENCODE) && (encode != NULL)) {
			ret = ceva_cmd_codec_free(encode);
			kfree(encode);
			encode = NULL;
		}
		else if (ceva->codec_main_type == CEVA_GAME) {
			switch (ceva->codec_sub_type) {
			case CEVA_CODEC_TYPE_KGB:
				ret = ceva_command(CX_CODEC_FREE, 4, NULL);
				kfree(kgb);
				kgb = NULL;
				break;
			}
		}
		break;

	case CEVA_IOCTL_CODEC_TYPE:
#ifdef AUTO_LOAD_IMAGE
		if (ceva->codec_sub_type != CEVA_CODEC_SUB_TYPE(arg)) {
			ceva->codec_main_type = CEVA_CODEC_MAIN_TYPE(arg);
			ceva->codec_sub_type = CEVA_CODEC_SUB_TYPE(arg);
			DIAG_DEBUG(">>> CEVA Codec: main_type=%d  sub_type=%d\n", ceva->codec_main_type, ceva->codec_sub_type);
		}
		else { /* alreay loaded same codec, notify libceva no need load codec again */
			ret = CEVA_ERR_LOADED;
		}
#else
		ceva->codec_main_type = CEVA_CODEC_MAIN_TYPE(arg);
		ceva->codec_sub_type = CEVA_CODEC_SUB_TYPE(arg);
		DIAG_DEBUG(">>> CEVA Codec: main_type=%d  sub_type=%d\n", ceva->codec_main_type, ceva->codec_sub_type);
#endif
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

	case CEVA_IOCTL_SET_PRIORITY:
		ret = gpHalVicSetPriority(VIC0, IRQ_PIU_CM, arg);
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
		//ceva->resumed = true; /* for test ceva resume */
		return -EBUSY;
	}
	ceva->opened = true;
	ceva_clock_enable(1);
#ifndef AUTO_LOAD_IMAGE
	gpHalCevaLock();

	/* set default codec type */
	ceva->codec_main_type = CEVA_DECODE;
	ceva->codec_sub_type = CEVA_CODEC_TYPE_NONE;
#endif

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

static void gp_ceva_device_release(struct device *dev)
{
	DIAG_INFO("[%s][%d] OK\n", __FUNCTION__, __LINE__);
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
#ifndef AUTO_LOAD_IMAGE
		gpHalCevaLock();
#endif
		ceva->resumed = true; /* need re-init ceva */
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
	.name = "gp-ceva",
	.id   = -1,
	.dev  = {
		.release = gp_ceva_device_release,
	},
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

	/* set default codec type */
	ceva->codec_main_type = CEVA_DECODE;
	ceva->codec_sub_type = CEVA_CODEC_TYPE_NONE;

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
