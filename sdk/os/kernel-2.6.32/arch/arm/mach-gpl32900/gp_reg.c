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
 * @file    gp_reg.c
 * @brief   Implement of register dump/fill driver
 * @author  Daolong Li
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/hardware.h>
#include <mach/gp_reg.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define DUMP_CHARS_PER_LINE 16
 
#define DEVICE_NAME "reg"

/* error code*/ 
#define ERR_REGISTER_ADDR	1
#define ERR_REGISTER_LEN	2

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
#define DEBUG0 DIAG_INFO
#else
#define DEBUG0(...)
#endif

#define REG_PRINT DIAG_INFO

 /**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
 typedef struct gp_reg_s {
	struct miscdevice dev;     				
	spinlock_t lock;
} gp_reg_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
 static gp_reg_t* gp_reg_info = NULL;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
*@brief Check register address or lenght valid or not
*@return 0 valida; other invalid
*@note This function depend on platform
*/
static int check_register(unsigned int *addr, unsigned int len)
{
	unsigned int temp_addr = *addr;
	unsigned int temp_start;
	unsigned int temp_base;
	unsigned int temp_size;

	unsigned int offset = 0;
	if ( temp_addr >= IO3_START ) {
		temp_start = IO3_START;
		temp_base = IO3_BASE;
		temp_size = IO3_SIZE;		
	}
	else if  (temp_addr >=IO2_START ) {
		temp_start = IO2_START;
		temp_base = IO2_BASE;
		temp_size = IO2_SIZE;	
	}
	else if ( temp_addr >= IO0_START ) {
		temp_start = IO0_START;
		temp_base = IO0_BASE;
		temp_size = IO0_SIZE;
	}
	else {
		return ERR_REGISTER_ADDR;	/*out of range of register address*/
	}
	
	temp_addr -= temp_start;
	temp_addr += temp_base;
	offset = temp_addr - temp_base;
	
	/*check range*/
	if ( (offset + len) > temp_size ) {
		return ERR_REGISTER_LEN;
	}

	*addr = temp_addr;
	return 0;
}

static unsigned int safe_8bit_write(unsigned int addr, unsigned char data)
{
	*((volatile unsigned char *)addr) = data;
	return 1;
}

static unsigned int safe_8bit_read(unsigned int addr, unsigned char *data)
{
	*data = *((volatile unsigned char *)addr);
	return 1;
}

static unsigned int safe_16bit_write(unsigned int addr,unsigned short data)
{
	if ( addr & 0x1 ) {
		return 0;
	}

	*((volatile unsigned short *)addr) = data;
	return 1;
}

static unsigned int safe_16bit_read(unsigned int addr, unsigned short *data)
{
	if ( addr & 0x1 ) {
		return 0;
	}
	
	*data = *((volatile unsigned short *)addr);
	return 1;
}

static unsigned int safe_32bit_write(unsigned int addr, unsigned int data)
{
	if ( addr & 0x3 ) {
		return 0;
	}
	
	*((volatile unsigned int *)addr) = data;
	return 1;
}

static unsigned int safe_32bit_read(unsigned int addr,	unsigned int *data)
{
	if ( addr & 0x3 ) {
		return 0;
	}
	
	*data = *((volatile unsigned int *)addr);
	return 1;
}

/**
*@brief Dump register 
*@param addr Start address of register
*@param len Length of register to be dump
*@param format How many bytes to be dump, accept REG_FORMAT_BYTE, REG_FORMAT_WORD,  REG_FORMAT_DWORD
 *@return 0 Success; Other fail
*/
int gp_reg_dump(unsigned int addr, unsigned int len, unsigned int format)
{

	unsigned int dump_addr = addr;
	unsigned int dump_len = len;
	unsigned int dump_fmt = format;
	unsigned int size;
	unsigned int value;
	unsigned int char_value;
	unsigned char  read_memory[DUMP_CHARS_PER_LINE];
	unsigned char  data_8;
	unsigned short data_16;
	unsigned int data_32;
	unsigned int bus_error = 0;
	unsigned int display_addr;
	int ret = 0;

	DEBUG0("%s: addr, len, format = 0x%08x, %d, %d \n", __FUNCTION__, addr, len, format);
	
	if ( dump_len == 0 ) {
		dump_len = 32;
	}

	if ( dump_fmt == 0 ) {
		dump_fmt = REG_FORMAT_DWORD;
	}

	size = dump_fmt;
	switch ( dump_fmt ) {
	case REG_FORMAT_DWORD:
		dump_addr &= (~0x3);
		dump_len += (4 - (dump_len & 0x3));
		break;
	case REG_FORMAT_WORD:
		dump_addr &= (~0x1);
		dump_len += (2 - (dump_len & 0x1));
		break;
	default:
		break;
	}
	
	display_addr = dump_addr;
	ret = check_register(&dump_addr, dump_len * size);
	if ( ret != 0 ) {
		if ( ret == ERR_REGISTER_ADDR )
			DIAG_ERROR("Invalid register address(0x%08x)!\n", addr);
		else if ( ret == ERR_REGISTER_LEN )
			DIAG_ERROR("Out of range of register(0x%08x + %d)!\n", addr, dump_len*size);

		return (-EFAULT);
	}
	value = 0;
	while ( 1 ) {
		if ( (value % DUMP_CHARS_PER_LINE) == 0 ) {
			if ( value ) {
				REG_PRINT(" | ");

				for ( char_value = 0; char_value < DUMP_CHARS_PER_LINE; char_value++ ) {
					if ( (read_memory[char_value] < ' ') || (read_memory[char_value] > 127) )
						REG_PRINT(".");
					else
						REG_PRINT("%c", read_memory[char_value]);
				}

				if ( (char_value + value) >= (dump_len + DUMP_CHARS_PER_LINE) ) {
					/* dump job is complete */
					REG_PRINT("\n");
					dump_addr += dump_len;
					if ( bus_error ){
						REG_PRINT ("\x7 warning bus (or alignment) errors occurred.\n");
						return (-EFAULT);
					}
					return 0;
				}
				REG_PRINT("\n");
			}
			/* Print the line offset label */
			REG_PRINT("%08x", display_addr + value);
		}

		/* Print an 8 byte boundary */
		if ( (value & (DUMP_CHARS_PER_LINE - 1)) == (DUMP_CHARS_PER_LINE >> 1) )
			REG_PRINT("-");
		else
			REG_PRINT(" ");

		switch ( dump_fmt ) {
		case REG_FORMAT_BYTE:
			if ( safe_8bit_read(dump_addr + value, &data_8) ) {
				REG_PRINT("%02x", data_8);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 0] = data_8;
			}
			else {
				bus_error = 1;
				REG_PRINT("**");
				read_memory[(value % DUMP_CHARS_PER_LINE) + 0] = '*';
			}
			break;

		case REG_FORMAT_WORD:
			if ( safe_16bit_read(dump_addr + value, &data_16) ) {
				REG_PRINT("%04x", data_16);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 0] = (unsigned char) (data_16 >> 8);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 1] = (unsigned char) data_16;
			}
			else {
				bus_error = 1;
				REG_PRINT("****");
				read_memory[(value % DUMP_CHARS_PER_LINE) + 0] = '*';
				read_memory[(value % DUMP_CHARS_PER_LINE) + 1] = '*';
			}
			break;

		case REG_FORMAT_DWORD:
		default:
			if ( safe_32bit_read(dump_addr + value, &data_32) ) {
				REG_PRINT("%08x", data_32);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 0] = (unsigned char) (data_32 >> 24);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 1] = (unsigned char) (data_32 >> 16);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 2] = (unsigned char) (data_32 >>  8);
				read_memory[(value % DUMP_CHARS_PER_LINE) + 3] = (unsigned char) data_32;
			}
			else {
				bus_error = 1;
				REG_PRINT("********");
				read_memory[(value % DUMP_CHARS_PER_LINE) + 0] = '*';
				read_memory[(value % DUMP_CHARS_PER_LINE) + 1] = '*';
				read_memory[(value % DUMP_CHARS_PER_LINE) + 2] = '*';
				read_memory[(value % DUMP_CHARS_PER_LINE) + 3] = '*';
			}
			break;
		}
		value += size;
	}

	return 0;
}
EXPORT_SYMBOL(gp_reg_dump);

/**
*@brief Fill register
*@param addr Start address of register
*@param len Length of register to be filled
*@param format How many bytes to be dump, accept REG_FORMAT_BYTE, REG_FORMAT_WORD,  REG_FORMAT_DWORD
*@param value data to be filled
 *@return 0 Success; Other fail
*/
int gp_reg_fill(unsigned int addr, unsigned int len, unsigned int format, unsigned int value)
{
	unsigned int fill_addr = addr;
	unsigned int i = 0;
	unsigned int bus_error = 0;
	int ret = 0;

	DEBUG0("%s: addr, len, format , value = 0x%08x, %d, %d, 0x%x \n", __FUNCTION__, addr, len, format, value);
	if ( len == 0 )
		len = 1;

	ret = check_register(&fill_addr, len * format);
	if ( ret != 0 )	{
		if ( ret == ERR_REGISTER_ADDR )
			DIAG_INFO("Invalid register address(0x%08x)!\n", addr);
		else if ( ret == ERR_REGISTER_LEN )
			DIAG_INFO("Out of range of register(0x%08x + %d)!\n", addr, len * format);

		return (-EFAULT);
	}

	while( i < len ){
		switch ( format ) {
		case REG_FORMAT_BYTE:
			bus_error = !safe_8bit_write(fill_addr, value);
			break;
		case REG_FORMAT_WORD:
			bus_error = !safe_16bit_write(fill_addr, value);
			break;
		case REG_FORMAT_DWORD:
			bus_error = !safe_32bit_write(fill_addr, value);
			break;
		}
		if ( bus_error ) {
			DIAG_ERROR("bus error (or alignment) error at 0x%08x, operation terminated\n", fill_addr);
			return (-EFAULT);
		}
		DEBUG0("fill_addr = 0x%08x\n", fill_addr);
		fill_addr += format;
		i++;
	}

	return 0;
}
EXPORT_SYMBOL(gp_reg_fill);


static int gp_reg_open(struct inode *inode, struct file *file)
{
	DEBUG0("%s:Enter!\n", __FUNCTION__);
	return 0;
}

static int gp_reg_release(struct inode *inode, struct file *file)
{
	DEBUG0("%s:Enter!\n", __FUNCTION__);
	return 0;
}

static long gp_reg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	gp_reg_cmd_t param = {0};
	int ret = 0;

	if ( NULL == gp_reg_info ) {
		return (-ENOIOCTLCMD);			
	}
	spin_lock(&gp_reg_info->lock);

	switch ( cmd ) {
	case REG_DUMP:
		if (copy_from_user(&param, (void __user *)arg, sizeof(param) ) )
			ret = -EFAULT;
		ret = gp_reg_dump(param.addr, param.len, param.format);
		break;
	case REG_FILL:
		if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
			ret = -EFAULT;
		ret = gp_reg_fill(param.addr, param.len, param.format, param.data);
		break;
	default:		
		ret = -ENOIOCTLCMD;	
		break;
	}
	
	spin_unlock(&gp_reg_info->lock);
	return ret;
}

static struct file_operations gp_reg_fops = {
	.owner          = THIS_MODULE,
	.open           = gp_reg_open,
	.release        = gp_reg_release,
	.unlocked_ioctl = gp_reg_ioctl,
};


static int __init init_gp_reg(void)
{	
	int ret = 0;
	
	gp_reg_info = kzalloc(sizeof(gp_reg_t),GFP_KERNEL);
	if ( NULL == gp_reg_info ) {
		return -ENOMEM;
	}	

	spin_lock_init(&gp_reg_info->lock);

	gp_reg_info->dev.name = DEVICE_NAME;
	gp_reg_info->dev.minor = MISC_DYNAMIC_MINOR;
	gp_reg_info->dev.fops  = &gp_reg_fops;
	
	
	ret = misc_register(&gp_reg_info->dev);
	if ( ret != 0 ) {
		DIAG_ERROR(KERN_ALERT "misc register fail\n");
		kfree(gp_reg_info);
		return ret;
	}
	
	return 0;
}

static void __exit free_gp_reg(void)
{
	if ( NULL == gp_reg_info )
		return;
	
	misc_deregister(&gp_reg_info->dev);
	kfree(gp_reg_info);
	gp_reg_info = NULL;
	
	return;
}

module_init(init_gp_reg);
module_exit(free_gp_reg);

MODULE_LICENSE_GP;
MODULE_AUTHOR("GeneralPlus");
MODULE_DESCRIPTION("gp register dump/fill driver!");


