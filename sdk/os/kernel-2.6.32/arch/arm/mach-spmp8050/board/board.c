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

/*!
 * @file board.c
 * @brief The configuration of board
 */

#include <mach/module.h>
#include <mach/kernel.h>
#include <mach/cdev.h>
#include <mach/gp_board.h>
#include <mach/diag.h>

MODULE_LICENSE_GP;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int32_t gp_board_open(struct inode *inode, struct file *filp);
int32_t gp_board_release(struct inode *inode, struct file *filp);
long gp_board_ioctl(struct file *filp, uint32_t cmd, unsigned long arg);


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static gp_board_config_t *gp_board_config;
static uint32_t g_board_pin_func[GP_PIN_MAX];
static struct semaphore g_board_pin_mutex;
static gp_board_pin_func_t *gp_pin_func_config = NULL;

static struct miscdevice g_board_dev;
/* access functions */
static struct file_operations g_board_fops = {
	open: gp_board_open,
	release: gp_board_release,
	unlocked_ioctl: gp_board_ioctl
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

void*
__gp_board_get_config(
	const char *tag,
	int len,
	int *len_out
)
{
	int i;

	for (i = 0; i < gp_board_config->count; i++) {
		gp_board_item_t *pItem = &gp_board_config->items[i];
		if (strcmp(pItem->tag, tag) == 0) {
			if (len != 0 && pItem->len != len) {
				continue;
			}
			if (len_out != NULL) {
				*len_out = pItem->len;
			}
			return pItem->data;
		}
	}
	if (len_out != NULL) {
		*len_out = 0;
	}
	return NULL;
}
EXPORT_SYMBOL(__gp_board_get_config);

int32_t
gp_board_malloc_pin_conf_entry(
	int32_t type
)
{
	int32_t ret;

	if (down_interruptible(&g_board_pin_mutex) != 0) {
		return -ERESTARTSYS;
	}

	if (type < 0 || type >= GP_PIN_MAX) {
		ret = -EIO;
	}
	else if (g_board_pin_func[type]) {
		ret = -EIO;
	}
	else {
		g_board_pin_func[type] = 1;
		ret = type;
	}

	up(&g_board_pin_mutex);

	return ret;
}
EXPORT_SYMBOL(gp_board_malloc_pin_conf_entry);

void
gp_board_free_pin_conf_entry(
	int32_t type
)
{
	if (down_interruptible(&g_board_pin_mutex) != 0) {
		return;
	}

	if (type >= 0 && type < GP_PIN_MAX) {
		g_board_pin_func[type] = 0;
	}

	up(&g_board_pin_mutex);
}
EXPORT_SYMBOL(gp_board_free_pin_conf_entry);

int32_t
gp_board_pin_func_request(
	int32_t type,
	uint32_t timeout
)
{
	if (!gp_pin_func_config) {
		gp_pin_func_config = gp_board_get_config("pin_func", gp_board_pin_func_t);
		if (!gp_pin_func_config) {
			printk("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}

	return gp_pin_func_config->pin_func_request(type, timeout);
}
EXPORT_SYMBOL(gp_board_pin_func_request);

void
gp_board_pin_func_release(
	uint32_t handle
)
{
	if (!gp_pin_func_config) {
		gp_pin_func_config = gp_board_get_config("pin_func", gp_board_pin_func_t);
		if (!gp_pin_func_config) {
			printk("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
			return;
		}
	}

	gp_pin_func_config->pin_func_release(handle);
}
EXPORT_SYMBOL(gp_board_pin_func_release);

void
gp_board_register(
	const gp_board_config_t *config
)
{
	gp_board_config = (gp_board_config_t *)config;
}
EXPORT_SYMBOL(gp_board_register);

/**
 * \brief Open board device
 */
int32_t
gp_board_open(
	struct inode *inode,
	struct file *filp
)
{
	/* Success */
	return 0;
}

/**
 * \brief Release board device
 */
int32_t
gp_board_release(
	struct inode *inode,
	struct file *filp
)
{
	/* Success */
	return 0;
}

/**
 * \brief Ioctl of board device
 */
long
gp_board_ioctl(
	struct file *filp,
	uint32_t cmd,
	unsigned long arg
)
{
	if (cmd == IOCTL_BOARD_INVOKE) {
		gp_board_rpc_t rpc;
		copy_from_user((void*)&rpc, (const void __user *)arg, sizeof(gp_board_rpc_t));
		//printk("RPC %d %p\n", rpc.type, rpc.func);
		switch (rpc.type) {
		case GP_BOARD_INVOKE_VV: {
				void (*func)(void) = (void (*)(void))rpc.func;
				func();
				rpc.ret = 0;
			}
			break;
		case GP_BOARD_INVOKE_IV: {
				int (*func)(void) = (int (*)(void))rpc.func;
				rpc.ret = func();
			}
			break;
		case GP_BOARD_INVOKE_VI: {
				void (*func)(int) = (void (*)(int))rpc.func;
				func(rpc.iparam[0]);
			}
			break;
		case GP_BOARD_INVOKE_II: {
				int (*func)(int) = (int (*)(int))rpc.func;
				rpc.ret = func(rpc.iparam[0]);
			}
			break;
		case GP_BOARD_INVOKE_IU: {
				int (*func)(uint32_t) = (int (*)(uint32_t))rpc.func;
				rpc.ret = func((uint32_t) rpc.iparam[0]);
			}
			break;	
		case GP_BOARD_INVOKE_IIU: {
				int (*func)(int, uint32_t) = (int (*)(int, uint32_t))rpc.func;
				rpc.ret = func(rpc.iparam[0], (uint32_t)rpc.iparam[1]);
			}
			break;
		case GP_BOARD_INVOKE_III: {
				int (*func)(int, int) = (int (*)(int, int))rpc.func;
				rpc.ret = func(rpc.iparam[0], rpc.iparam[1]);
			}
			break;
		case GP_BOARD_INVOKE_IIII: {
				int (*func)(int, int, int) = (int (*)(int, int, int))rpc.func;
				rpc.ret = func(rpc.iparam[0], rpc.iparam[1], rpc.iparam[2]);
			}
			break;
		case GP_BOARD_INVOKE_IUUU: {
				int (*func)(uint32_t, uint32_t, uint32_t) = (int (*)(uint32_t, uint32_t, uint32_t))rpc.func;
				rpc.ret = func((uint32_t)rpc.iparam[0], (uint32_t)rpc.iparam[1], (uint32_t)rpc.iparam[2]);
			}
			break;
		case GP_BOARD_INVOKE_IIIII: {
				int (*func)(int, int, int, int ) = (int (*)(int, int, int, int ))rpc.func;
				rpc.ret = func(rpc.iparam[0], rpc.iparam[1], rpc.iparam[2], rpc.iparam[3] );
			}
			break;			
		default:
			rpc.ret = -1;
			break;
		}
		copy_to_user ((void __user *)arg, (const void *)&rpc, sizeof(gp_board_rpc_t));
	}
	else if (cmd == IOCTL_BOARD_GET_CONFIG) {
		gp_board_remote_config_t proxy;
		void *config;
		copy_from_user((void*)&proxy, (const void __user *)arg, sizeof(gp_board_remote_config_t));
		config = __gp_board_get_config(proxy.tag, proxy.len, &proxy.len_out);
		if (config != 0 && proxy.len_out != 0) {
			copy_to_user ((void __user *)proxy.config, (const void *)config, proxy.len_out);
		} else {
			proxy.len_out = 0;
		}
		copy_to_user ((void __user *)arg, (const void *)&proxy, sizeof(gp_board_remote_config_t));
	}
	else if (gp_board_config && gp_board_config->ioctl) {
		return (gp_board_config->ioctl)(filp, cmd, arg);
	}
	return -EIO;
}

static int __init
gp_board_init(
	void
)
{
	int32_t ret;

	/* register board dev */
	g_board_dev.minor = MISC_DYNAMIC_MINOR;
	g_board_dev.name = "board";
	g_board_dev.fops = &g_board_fops;
	ret = misc_register(&g_board_dev);
	if (ret) {
		return ret;
	}

	printk("board dev minor : %i\n", g_board_dev.minor);

	/* init mutex */
	init_MUTEX(&g_board_pin_mutex);
	return 0;
}

static void
gp_board_exit(
	void
)
{
	misc_deregister(&g_board_dev);
}

module_init(gp_board_init);
module_exit(gp_board_exit);
