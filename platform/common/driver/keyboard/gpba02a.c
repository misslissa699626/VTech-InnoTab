/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2011 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  antion @2011.02.28                                                    *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_keyboard.c
 * @brief used SPI driver interface 
 * @author antion
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/hal/hal_spi1.h>
#include <mach/gp_spi.h>
#include <mach/gp_board.h>
//----------------------------------
#include <linux/init.h>

#include <linux/fs.h> /* everything... */



#include <mach/gp_display.h>

#include <mach/general.h>



#include <mach/gp_pwm.h>

#include <mach/gp_gpio.h>

#include <linux/delay.h> 	/* udelay/mdelay */

#include <mach/gp_adc.h>

#include <linux/input.h>

#include <mach/hal/hal_gpio.h>

#include <mach/gp_timer.h>
//**************************************************************************************************
#include <mach/hal/hal_gpio.h>
//#define MK_GPIO_INDEX(channel,func,gid,pin) (((channel)<<24)|((func)<<16)|((gid)<<8)|(pin))
/**************************************************************************

 *                              M A C R O S                               *

 **************************************************************************/

#define GPBA02_BASE_BUFFER     	0x00
#define GPBA02_BASE_DIR			0x04
#define GPBA02_BASE_ATTR		0x08
#define GPBA02_BASE_DATA		0x0c

#define GPBA02_OFFSET_PORT_A 	0x00
#define GPBA02_OFFSET_PORT_B	0x01
#define GPBA02_OFFSET_PORT_C	0x02

#define GPBA02_PORTA_BUFFER		(GPBA02_BASE_BUFFER	+GPBA02_OFFSET_PORT_A)
#define GPBA02_PORTA_DIR		(GPBA02_BASE_DIR	+GPBA02_OFFSET_PORT_A)
#define GPBA02_PORTA_ATTR		(GPBA02_BASE_ATTR	+GPBA02_OFFSET_PORT_A)
#define GPBA02_PORTA_DATA		(GPBA02_BASE_DATA	+GPBA02_OFFSET_PORT_A)

#define GPBA02_PORTB_BUFFER		(GPBA02_BASE_BUFFER+GPBA02_OFFSET_PORT_B)
#define GPBA02_PORTB_DIR		(GPBA02_BASE_DIR	+GPBA02_OFFSET_PORT_B)
#define GPBA02_PORTB_ATTR		(GPBA02_BASE_ATTR	+GPBA02_OFFSET_PORT_B)
#define GPBA02_PORTB_DATA		(GPBA02_BASE_DATA	+GPBA02_OFFSET_PORT_B)

#define GPBA02_PORTC_BUFFER		(GPBA02_BASE_BUFFER	+GPBA02_OFFSET_PORT_C)
#define GPBA02_PORTC_DIR		(GPBA02_BASE_DIR	+GPBA02_OFFSET_PORT_C)
#define GPBA02_PORTC_ATTR		(GPBA02_BASE_ATTR	+GPBA02_OFFSET_PORT_C)
#define GPBA02_PORTC_DATA		(GPBA02_BASE_DATA	+GPBA02_OFFSET_PORT_C)

#define KEYSCAN_OUT_BUFFER		GPBA02_PORTA_BUFFER          
#define KEYSCAN_OUT_DIR			GPBA02_PORTA_DIR          
#define KEYSCAN_OUT_ATTR		GPBA02_PORTA_ATTR
#define KEYSCAN_OUT_DATA		GPBA02_PORTA_DATA          

#define KEYSCAN_IN_BUFFER		GPBA02_PORTB_BUFFER          
#define KEYSCAN_IN_DIR			GPBA02_PORTB_DIR          
#define KEYSCAN_IN_ATTR			GPBA02_PORTB_ATTR
#define KEYSCAN_IN_LOW_DATA		GPBA02_PORTB_DATA  

#define KEYSCAN_IN1_BUFFER		GPBA02_PORTC_BUFFER          
#define KEYSCAN_IN1_DIR			GPBA02_PORTC_DIR          
#define KEYSCAN_IN1_ATTR		GPBA02_PORTC_ATTR
#define KEYSCAN_IN_HIGH_DATA	GPBA02_PORTC_DATA  
/*
#define	VK_LCONTROL		0x80
#define VK_LWIN			0x81
#define VK_LSHIFT		0x82
#define VK_LALT			0x83
#define VK_APPS			0x84
#define VK_RIGHT		0x85
#define VK_FINAL		0x86
#define	VK_SPACE		0x87
#define	VK_LEFT			0x88
#define VK_DOWN			0x89
#define	VK_RETURN		0x8a
#define VK_NEXT			0x8b
#define	VK_CAPITAL		0x8c
#define VK_UP			0x8d
#define VK_FRONT		0x8e
#define VK_TAB			0x8f
#define VK_BACK			0x90
#define VK_PAUSE		0x91
#define VK_LEDUP		0x92
#define VK_VOLUMEUP		0x93
#define VK_SPEEDUP		0x94
#define VK_NEW			0x95
#define	VK_COPY			0x96
#define VK_DELETE		0x97
#define VK_PLAY			0x98
#define VK_ESCAPE		0x99
#define VK_HELP			0x9a
#define VK_LEDDOWN		0x9b
#define VK_VOLUMEDOWN	0x9c
#define VK_SPEEDDOWN	0x9d
#define	VK_SAVE			0X9e
#define VK_REPEAT		0x9f
#define VK_INSERT		0xa0

#define MWKMOD_SHIFT 0x10
*/
#if 0
#define	VK_LCONTROL		29
#define VK_LWIN			78	/*+*/
#define VK_LALT			74	/*-*/
#define VK_APPS			55	/***/
#define VK_FINAL		98	/*/*/

#define VK_FRONT		104	/*PAGEUP*/
#define VK_NEXT			109	/*PAGEDN*/

#define VK_BACK			14
#define	VK_SPACE		57
#define VK_LSHIFT		42
#define VK_TAB			15
#define	VK_CAPITAL		58
#define VK_ESCAPE		1
#define	VK_RETURN		28
#define VK_UP			103
#define VK_DOWN			108
#define	VK_LEFT			105
#define VK_RIGHT		106

#define VK_HELP			59   	/*F1*/
#define VK_LEDUP		60   	/*F2*/	
#define VK_LEDDOWN		61	/*F3*/
#define VK_VOLUMEUP		62	/*F4*/
#define VK_VOLUMEDOWN		63	/*F5*/
#define VK_SPEEDUP		64	/*F6*/
#define VK_SPEEDDOWN		65	/*F7*/
#define VK_NEW			66	/*F8*/
#define	VK_SAVE			67	/*F9*/
#define	VK_COPY			68	/*F10*/
#define VK_PLAY			87	/*F11*/
#define VK_PAUSE		88	/*F12*/
#define VK_REPEAT		102	/*HOME*/
#define VK_INSERT		110	
#define VK_DELETE		111
#endif
#define MWKMOD_SHIFT 0x10
#define KEYBOARD_NAME	"keyboardx"

#define keyprint	1//print key state define
#define keygpio		0//set gpio state to test wave

#if keygpio
#define	spi_set_rx_channel		0

#define	spi_set_rx_func			0//1

#define	spi_set_rx_gid			23

#define	spi_set_rx_pin			6

#define	spi_set_rx_level		1
#endif
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
#if keygpio
static gpio_content_t cmd_rx = {0};
#endif
typedef struct gp_keyboard_s{
	struct miscdevice dev; 	 /*!< @brief spi device */
	struct semaphore keyboard_sem;
	int pin_handle;
}gp_keyboard_t;

typedef struct gp_keyboard_info_s 
{
	UINT32 id;		/*!< @brief spi channel index */
	UINT8 isOpened;
	UINT32 freq;		/*!< @brief spi freq value */
	UINT8 dam_mode;		/*!< @brief spi enable/disable dam mode */
	UINT8 clk_pol;		/*!< @brief spi clock polarity at idles state */
	UINT8 clk_pha;		/*!< @brief spi clock phase,data occur at odd/even edge of SCLK clock */
	UINT8 lsbf;		/*!< @brief spi LSB or MSB first */
} gp_keyboard_info_t;

typedef struct gp_keyboard_timer_s {	
	int pin_clk;
	int pin_data;
	struct timer_list 	timer;        
	struct input_dev *input;
} gp_keyboard_timer_t;
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct gp_keyboard_s *gp_keyboard_data;     	 /*!< @brief spi device */
static struct gp_keyboard_info_s keyboard_spi_port;
static struct gp_keyboard_timer_s gp_keyboard_timer_data;

static int usLastKey = 0;
static int usRepeat;
static int lastausScanCode[2];
static int usKeyPress;

static int testct = 0;
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
static const int out_data [] = {1,2,4,8,16,32,64,128};
static const int in_data [] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
#define VK_LCDUP	KEY_F13
#define VK_LCDDOWN	KEY_F14
#define VK_NEW		KEY_F15
#define VK_INPUT	KEY_F16
#define VK_ZMIN		KEY_F17
#define VK_ZMOUT	KEY_F18
static int key_table[8][16] = 
{
{KEY_ESC ,KEY_HELP ,KEY_F1 ,KEY_F2 ,KEY_F3 ,KEY_F4 ,KEY_F5 ,VK_LCDDOWN ,VK_LCDUP ,KEY_VOLUMEDOWN,KEY_VOLUMEUP,VK_NEW,KEY_SAVE,KEY_COPY,KEY_DELETE,0},
{KEY_GRAVE,KEY_1,KEY_2,0,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_0,KEY_MINUS,KEY_EQUAL,KEY_INSERT,0},
{KEY_TAB,KEY_Q,KEY_W,0,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,KEY_O,KEY_P,KEY_LEFTBRACE,KEY_RIGHTBRACE,KEY_BACKSPACE,0},
{KEY_CAPSLOCK,KEY_A,KEY_S,0,KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,0,KEY_L,KEY_SEMICOLON,KEY_APOSTROPHE,KEY_PAGEUP,0},
{KEY_LEFTSHIFT,0,0,0,0,0,0,KEY_F6,0,0,0,0,0,0,KEY_ENTER,KEY_PAGEDOWN},
{KEY_LEFTCTRL,0,0,0,0,0,0,0,0,0,0,0,0,0,KEY_LEFTSHIFT,KEY_LEFT},
{KEY_HOME,KEY_Z,0,0,KEY_X,KEY_C,KEY_V,KEY_B,KEY_N,KEY_M,0,KEY_COMMA,KEY_DOT,KEY_SLASH,KEY_UP,KEY_DOWN},
{VK_INPUT,0,0,KEY_LEFTALT,0,0,0,0,0,KEY_SPACE,0,0,VK_ZMIN,KEY_BACKSLASH,KEY_RIGHT,VK_ZMOUT}
};
#if 0
static int key_table[8][16] = 
{
    {VK_LCONTROL, 52/*'.'*/, VK_LWIN, VK_LSHIFT, VK_LALT, 0, 0, 0, 
    33/*'f'*/, 47/*'v'*/, 0x00, 38/*'l'*/, 51/*','*/, VK_APPS, 43/*'\\'*/, VK_RIGHT}, 
    
    {0x00, VK_FINAL, 0x00, 0x00, 0x00, 44/*'z'*/, 45/*'x'*/, 46/*'c'*/, 
    34/*'g'*/, 35/*'h'*/, 48/*'b'*/, 37/*'k'*/, 50/*'m'*/, VK_SPACE, 0x00, 0x00},
     
    {0x00, 40/*'\''*/, 0x00, 0x00, 0x00, 0x00, 0x00, 32/*'d'*/, 
    19/*'r'*/, 21/*'y'*/, 49/*'n'*/, 24/*'o'*/, 39/*';'*/, VK_LEFT, 0x00, VK_DOWN},
     
    {0x00, 27/*']'*/, 0x00, 0x00, 0x00, 30/*'a'*/, 31/*'s'*/, 17/*'w'*/, 
    18/*'e'*/, 20/*'t'*/, 36/*'j'*/, 23/*'i'*/, 25/*'p'*/, 53/*'/'*/, 0x00, VK_RETURN},
    
    {0x00, 26/*'['*/,0x00, VK_NEXT, 0x00, VK_CAPITAL, 16/*'q'*/, 3/*'2'*/, 
    5/*'4'*/, 7/*'6'*/, 22/*'u'*/, 10/*'9'*/, 12/*'-'*/, VK_UP, 0x00, VK_FRONT},
    
    {0x00, 13/*'='*/, 0x00, 0x00, 0x00, 0x00, VK_TAB, 2/*'1'*/, 
    4/*'3'*/, 6/*'5'*/, 8/*'7'*/, 9/*'8'*/, 11/*'0'*/, 0x00, 0x00, VK_BACK},
     
    {0x00, VK_PAUSE, 0x00, 0x00, 0x00, 0x00, 0x00, 41/*'`'*/, 
    VK_LEDUP, VK_VOLUMEUP, VK_SPEEDUP, VK_NEW, VK_COPY, 0x00, 0x00, VK_DELETE},
     
    {0x00, VK_PLAY, 0x00, 0x00, 0x00, 0x00, 0x00, VK_ESCAPE, 
    VK_HELP, VK_LEDDOWN, VK_VOLUMEDOWN, VK_SPEEDDOWN, VK_SAVE, VK_REPEAT, 0x00, VK_INSERT}
};
#endif
/*
static int key_table[8][16] = 
{
    {VK_LCONTROL, '.', VK_LWIN, VK_LSHIFT, VK_LALT, 0, 0, 0,
    'f', 'v', 0x00, 'l', ',', VK_APPS, '\\', VK_RIGHT},
    
    {0x00, VK_FINAL, 0x00, 0x00, 0x00, 'z', 'x', 'c',
    'g', 'h', 'b', 'k', 'm', VK_SPACE, 0x00, 0x00},
     
    {0x00, '\'', 0x00, 0x00, 0x00, 0x00, 0x00, 'd',
    'r', 'y', 'n', 'o', ';', VK_LEFT, 0x00, VK_DOWN},
     
    {0x00, ']', 0x00, 0x00, 0x00, 'a', 's', 'w',
    'e', 't', 'j', 'i', 'p', '/', 0x00, VK_RETURN},
    
    {0x00, '[',0x00, VK_NEXT, 0x00, VK_CAPITAL, 'q', '2',
    '4', '6', 'u', '9', '-', VK_UP, 0x00, VK_FRONT},
    
    {0x00, '=', 0x00, 0x00, 0x00, 0x00, VK_TAB, '1',
    '3', '5', '7', '8', '0', 0x00, 0x00, VK_BACK},
     
    {0x00, VK_PAUSE, 0x00, 0x00, 0x00, 0x00, 0x00, '`',
    VK_LEDUP, VK_VOLUMEUP, VK_SPEEDUP, VK_NEW, VK_COPY, 0x00, 0x00, VK_DELETE},
     
    {0x00, VK_PLAY, 0x00, 0x00, 0x00, 0x00, 0x00, VK_ESCAPE,
    VK_HELP, VK_LEDDOWN, VK_VOLUMEDOWN, VK_SPEEDDOWN, VK_SAVE, VK_REPEAT, 0x00, VK_INSERT}
};
*/
/*
static int key_table_sh[8][16] = {
    {VK_LCONTROL, '>', VK_LWIN, VK_LSHIFT, VK_LALT, 0, 0, 0,
    'F', 'V', 0x00, 'L', '<', VK_APPS, '|', VK_RIGHT},
    
    {0x00, VK_FINAL, 0x00, 0x00, 0x00, 'Z', 'X', 'C',
    'G', 'H', 'B', 'K', 'M', VK_SPACE, 0x00, 0x00},
     
    {0x00, '"', 0x00, 0x00, 0x00, 0x00, 0x00, 'D',
    'R', 'Y', 'N', 'O', ':', VK_LEFT, 0x00, VK_DOWN},
     
    {0x00, '}', 0x00, 0x00, 0x00, 'A', 'S', 'W',
    'E', 'T', 'J', 'I', 'P', '?', 0x00, VK_RETURN},
    
    {0x00, '{',0x00, VK_NEXT, 0x00, VK_CAPITAL, 'Q', '@',
    '$', '^', 'U', '(', '_', VK_UP, 0x00, VK_FRONT},
    
    {0x00, '+', 0x00, 0x00, 0x00, 0x00, VK_TAB, '!',
    '#', '%', '&', '*', ')', 0x00, 0x00, VK_BACK},
     
    {0x00, VK_PAUSE, 0x00, 0x00, 0x00, 0x00, 0x00, '~',
    VK_LEDUP, VK_VOLUMEUP, VK_SPEEDUP, VK_NEW, VK_COPY, 0x00, 0x00, VK_DELETE},
     
    {0x00, VK_PLAY, 0x00, 0x00, 0x00, 0x00, 0x00, VK_ESCAPE,
    VK_HELP, VK_LEDDOWN, VK_VOLUMEDOWN, VK_SPEEDDOWN, VK_SAVE, VK_REPEAT, 0x00, VK_INSERT}
};
*/
#if keygpio
static int gpiocfgOut(

	int channel,

	int func,

	int gid,

	int pin,

	int level

)

{

	int handle;

	gpio_content_t ctx;



	if ( channel == 0xff ) {

		return -1;

	}



	ctx.pin_index = MK_GPIO_INDEX( channel, func, gid, pin );

	handle = gp_gpio_request( ctx.pin_index, NULL );



	gp_gpio_set_function( handle, func );

	gp_gpio_set_direction( handle, GPIO_DIR_OUTPUT );

	gp_gpio_set_output( handle, level, 0 );

	

	gp_gpio_release( handle );

	return	0;

}
#endif
/*@brief   keyboard device open*/
static int gp_keyboard_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*@brief   keyboard device close*/
static int gp_keyboard_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

/*@brief   keyboard device ioctl*/
static long gp_keyboard_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

/*@brief   keyboard device read*/
static ssize_t gp_keyboard_dev_read(struct file *file, char __user *buf,size_t count, loff_t *oppos)
{
	return 0;
}

/*@brief   keyboard device write*/
static ssize_t gp_keyboard_dev_write(struct file *file, const char __user *buf,size_t count, loff_t *oppos)
{
	return 0;
}
static void SPI_WriteReg(UINT8 addr, UINT8 value) 
{
	char buffer[3];
	buffer[0] = addr|0x80;
	buffer[1] = value;
	gp_spi1_cs_enable((int)&keyboard_spi_port);
	gp_spi1_write((int)&keyboard_spi_port, buffer, 2);
	gp_spi1_cs_disable((int)&keyboard_spi_port);
}

static UINT8  SPI_ReadReg(UINT8 addr)
{
	char buffer[2];
	char bufferr[2];
	buffer[0] = addr&0x7f;
	gp_spi1_cs_enable((int)&keyboard_spi_port);
	gp_spi1_write((int)&keyboard_spi_port, buffer, 1);
	gp_spi1_read((int)&keyboard_spi_port, bufferr, 1);
	gp_spi1_cs_disable((int)&keyboard_spi_port);
	return bufferr[0];
}


static UINT16 readKeyboardPort(void)
{
	UINT16 usLowByte, usHighByte;
	usLowByte = SPI_ReadReg(KEYSCAN_IN_LOW_DATA) & 0xff;
	usHighByte = SPI_ReadReg(KEYSCAN_IN_HIGH_DATA) &0xff;
	return usLowByte + (usHighByte << 8); 
}

static UINT32 setKeyboardPort(UINT8 value)
{
	SPI_WriteReg(KEYSCAN_OUT_DIR, value);
	return 0;
}
struct input_dev *input;
static void gp_keyboard_isr(unsigned long arg)
{
	UINT8 ucScanCol, ucScanRow, ucPressedKeyCount;
	UINT16 usCurKey;

	int ausScanCode[2];

	ucPressedKeyCount = 0;
	gp_keyboard_timer_data.timer.expires = jiffies + 3;

	add_timer(&gp_keyboard_timer_data.timer);
	
	setKeyboardPort(0xff);
	usCurKey = readKeyboardPort();
	if ( 0 != (usCurKey & 0xffff))
	{
		for(ucScanCol = 0; ucScanCol < 8; ucScanCol ++)
		{
			setKeyboardPort(out_data[ucScanCol]);
			usCurKey = readKeyboardPort();
			for(ucScanRow = 0; ucScanRow < 16; ucScanRow ++)	
			{
				if (usCurKey & in_data[ucScanRow])
				{
					ausScanCode[ucPressedKeyCount] = key_table[ucScanCol][ucScanRow];
					ucPressedKeyCount+=1;
					//printk("ucScanCol=%d,ucScanRow=%d\n",ucScanCol,ucScanRow);
					if(ucPressedKeyCount >= 2)
					{
						goto KeyEnd;
					}
				}
			}	
		}
	}
	else
	{
	}
KeyEnd:	
	setKeyboardPort(0x00);	
#if keygpio
	cmd_rx.value = 1;
	gp_gpio_set_value((int)&cmd_rx,1);
#endif
	if((usLastKey == 0)&&(ucPressedKeyCount > 0))
	{//key down
		usRepeat = 0;
		usLastKey = 1;
		usKeyPress = ucPressedKeyCount;
		input_event(input, EV_KEY, ausScanCode[0], 1);//key down
#if keyprint
		printk("1-%c-down\n",ausScanCode[0]);
#endif
		lastausScanCode[0]=ausScanCode[0];
		if(ucPressedKeyCount == 2)
		{
			input_event(input, EV_KEY, ausScanCode[1], 1);//key down
#if keyprint
		printk("2-%c-down\n",ausScanCode[1]);
#endif
			lastausScanCode[1]=ausScanCode[1];
		}
		input_sync(input);
	}
	else if(usLastKey == 1)
	{//key repeat
		if(usKeyPress == 1)
		{//last one key
			if(ucPressedKeyCount == 2)
			{//now tow key
				if(lastausScanCode[0] == ausScanCode[0])
				{
					input_event(input, EV_KEY, ausScanCode[1], 1);//key down
#if keyprint
		printk("3-%c-down\n",ausScanCode[1]);
#endif
					lastausScanCode[1] = ausScanCode[1];
					usKeyPress = 2;
					usRepeat = 0;
				}
				else if(lastausScanCode[0] == ausScanCode[1])
				{
					input_event(input, EV_KEY, ausScanCode[0], 1);//key down
#if keyprint
		printk("4-%c-down\n",ausScanCode[0]);
#endif
					lastausScanCode[1] = ausScanCode[0];
					usKeyPress = 2;
					usRepeat = 0;
				}
			}
			else if(ucPressedKeyCount == 1)
			{//now one key
				if(lastausScanCode[0] != ausScanCode[0])
				{
					input_event(input, EV_KEY, lastausScanCode[0], 0);//key up
#if keyprint
		printk("5-%c-up\n",lastausScanCode[0]);
#endif
					input_sync(input);
					input_event(input, EV_KEY, ausScanCode[0], 1);//key down
#if keyprint
		printk("6-%c-down\n",ausScanCode[0]);
#endif
					lastausScanCode[0] = ausScanCode[0];
					usRepeat = 0;
					input_sync(input);
				}
				else
				{
					if(usRepeat >= 30)
					{
						input_event(input, EV_KEY, ausScanCode[0], 2);//key repeat
#if keyprint
		printk("7x-%c-repeat\n",ausScanCode[0]);
#endif
						input_sync(input);
					}
					else
					{
						if(usRepeat >= 0)
						{
							usRepeat += 1;
						}
					}
				}
			}
			else
			{//now no key
				input_event(input, EV_KEY, lastausScanCode[0], 0);//key up
#if keyprint
		printk("8-%c-up\n",lastausScanCode[0]);
#endif
				input_sync(input);
				usRepeat = 0;
				lastausScanCode[0] = 0;
				usKeyPress = 0;
				usLastKey = 0;
			}
		}
		else //if(usKeyPress == 2)
		{//last tow key
			if(ucPressedKeyCount == 2)
			{//now tow key
				if(lastausScanCode[0] == ausScanCode[0])
				{
					if(lastausScanCode[1] == ausScanCode[1])
					{
						if(usRepeat >= 30)
						{
							input_event(input, EV_KEY, ausScanCode[1], 2);//key repeat
#if keyprint
		printk("9-%c-repeat\n",ausScanCode[1]);
#endif
							input_sync(input);
						}
						else
						{
							if(usRepeat >= 0)
							{
								usRepeat += 1;
							}
						}
					}
					else
					{
						input_event(input, EV_KEY, lastausScanCode[1], 0);//key up
#if keyprint
		printk("a-%c-up\n",lastausScanCode[1]);
#endif
						input_sync(input);
						input_event(input, EV_KEY, ausScanCode[1], 1);//key down
#if keyprint
		printk("b-%c-down\n",ausScanCode[1]);
#endif
						lastausScanCode[1] = ausScanCode[1];
						usRepeat = 0;
						input_sync(input);
					}
				}
				else if(lastausScanCode[1] == ausScanCode[0])
				{
					if(lastausScanCode[0] == ausScanCode[1])
					{
						if(usRepeat >= 30)
						{
							input_event(input, EV_KEY, lastausScanCode[1], 2);//key repeat
#if keyprint
		printk("c-%c-repeat\n",lastausScanCode[1]);
#endif
							input_sync(input);
						}
						else
						{
							if(usRepeat >= 0)
							{
								usRepeat += 1;
							}
						}
					}
					else
					{
						
						input_event(input, EV_KEY, lastausScanCode[0], 0);//key up
#if keyprint
		printk("d-%c-up\n",lastausScanCode[0]);
#endif
						lastausScanCode[0] = ausScanCode[0];
						lastausScanCode[1] = ausScanCode[1];
						input_sync(input);
						input_event(input, EV_KEY, ausScanCode[1], 1);//key down
#if keyprint
		printk("e-%c-down\n", ausScanCode[1]);
#endif
						input_sync(input);
						usRepeat = 0;
					}
				}
				else if(lastausScanCode[0] == ausScanCode[1])
				{
					if(lastausScanCode[1] == ausScanCode[0])
					{
						//input_event(input, EV_KEY, ausScanCode[1], 2);//key repeat*/
					}
					else
					{
						input_event(input, EV_KEY, lastausScanCode[1], 0);//key up
#if keyprint
		printk("f-%c-up\n",lastausScanCode[1]);
#endif
						input_sync(input);
						lastausScanCode[1] = ausScanCode[0];
						input_event(input, EV_KEY, ausScanCode[0], 2);//key repeat
#if keyprint
		printk("g-%c-repeat\n",ausScanCode[0]);
#endif
						input_sync(input);
						usRepeat = 0;
					}
				}
				else
				{
					
					input_event(input, EV_KEY, lastausScanCode[0], 0);//key up
					input_event(input, EV_KEY, lastausScanCode[1], 0);//key up
#if keyprint
		printk("h-%c-up\ni-%c-up\n",lastausScanCode[0],lastausScanCode[1]);
#endif
					input_sync(input);
					input_event(input, EV_KEY, ausScanCode[0], 1);//key down
					input_event(input, EV_KEY, ausScanCode[1], 1);//key down
#if keyprint
		printk("j-%c-down\nk-%c-down\n",ausScanCode[0],ausScanCode[1]);
#endif
					input_sync(input);
					lastausScanCode[0] = ausScanCode[0];
					lastausScanCode[1] = ausScanCode[1];
					usRepeat = 0;
				}
			}
			else if(ucPressedKeyCount == 1)
			{//now one key
				usKeyPress = 1;
				if(lastausScanCode[0] == ausScanCode[0])
				{
					input_event(input, EV_KEY, lastausScanCode[1], 0);//key up
#if keyprint
		printk("l-%c-up\n",lastausScanCode[1]);
#endif
					input_sync(input);
					lastausScanCode[1] = 0;
					usRepeat = 0;
					usRepeat = -1;
				}
				else if(lastausScanCode[1] == ausScanCode[0])
				{
					input_event(input, EV_KEY, lastausScanCode[0], 0);//key up
#if keyprint
		printk("m-%c-up\n",lastausScanCode[0]);
#endif
					input_sync(input);
					lastausScanCode[0] = ausScanCode[0];
					lastausScanCode[1] = 0;
				}
			}
			else //ucPressedKeyCount == 0
			{
				input_event(input, EV_KEY, lastausScanCode[0], 0);//key up
				input_event(input, EV_KEY, lastausScanCode[1], 0);//key up
#if keyprint
		printk("n-%c-up\no-%c-up\n",lastausScanCode[0],lastausScanCode[1]);
#endif
				input_sync(input);
				lastausScanCode[0] = 0;
				lastausScanCode[1] = 0;
				usRepeat = 0;
				usLastKey = 0;
				usKeyPress = 0;
			}
		}
	}
#if keygpio
	cmd_rx.value = 0;
	gp_gpio_set_value((int)&cmd_rx,0);
#endif
	/*if((testct%30) == 0)
	{
		printk("kb tick = %d,kct = %d\n",testct/30,ucPressedKeyCount);
	}
	testct += 1;*/
	return;
}

static void keyboard_spigpba02a_init(struct platform_device *pdev)
{
	//spi port and GPBA02A init
	char bufferw[3];
	int i,j;
	int ret = 0;
	//int dly;
#if keygpio
	cmd_rx.pin_index = MK_GPIO_INDEX(spi_set_rx_channel, spi_set_rx_func, spi_set_rx_gid, spi_set_rx_pin);
	gpiocfgOut(spi_set_rx_channel,spi_set_rx_func,spi_set_rx_gid,spi_set_rx_pin,spi_set_rx_level);
#endif	
	input = input_allocate_device();
	if (!input) {
		ret = -ENOMEM;
		printk("input alloc erro\n");
		return;
	}
	   
	input->name = pdev->name;
	input->phys = "keyboard/input0";
	input->dev.parent = &pdev->dev;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	input->evbit[0] = BIT_MASK(EV_KEY);//| BIT_MASK(EV_REL);
	input->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) |BIT_MASK(BTN_RIGHT);
	//input->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

	gp_keyboard_timer_data.input = input;

	input_set_drvdata(input, &gp_keyboard_timer_data);
	for(i = 0;i < 8;i++)
	{
		for(j = 0;j < 16;j++)
		{
			if(key_table[i][j] != 0)
			{
				input_set_capability(input, EV_KEY, key_table[i][j]);
			}
		}
	}
	input_set_capability(input, EV_KEY, 0);
    
	ret = input_register_device(input);
	if (ret){
		goto erro_input;
	}

	platform_set_drvdata(pdev, &gp_keyboard_timer_data);
	
	keyboard_spi_port.id		= 0;
	keyboard_spi_port.isOpened	= 1;
	keyboard_spi_port.freq		= 6000000;
	keyboard_spi_port.dam_mode	= 0;
	keyboard_spi_port.clk_pol	= 0;
	keyboard_spi_port.clk_pha	= 0;
	keyboard_spi_port.lsbf		= 0;

	//reset GPBA02A;
	gp_spi1_cs_enable((int)&keyboard_spi_port);
	bufferw[0]=0xff;

	gp_spi1_write((int)&keyboard_spi_port, bufferw, 1);
	printk("reset GPBA02A\n");

	gp_spi1_cs_disable((int)&keyboard_spi_port);

	//timer setting
	init_timer(&gp_keyboard_timer_data.timer);
	gp_keyboard_timer_data.timer.data = (unsigned long) &gp_keyboard_timer_data;
	gp_keyboard_timer_data.timer.function = gp_keyboard_isr;
	gp_keyboard_timer_data.timer.expires = jiffies + 3;
	add_timer(&gp_keyboard_timer_data.timer);
	printk("timer init finish.\n");
erro_input:
	return;
}

static void KeyScan_Initial(void)
{	
	SPI_WriteReg(0xFF, 0);

	SPI_WriteReg(KEYSCAN_OUT_ATTR, 0x00);//
	SPI_WriteReg(KEYSCAN_OUT_DIR, 	0x00);//0xff 
	SPI_WriteReg(KEYSCAN_OUT_BUFFER,0xff );//pull low 
	
	SPI_WriteReg(KEYSCAN_IN_ATTR, 0x00);//
	SPI_WriteReg(KEYSCAN_IN_DIR, 	0x00);//0xff 
	SPI_WriteReg(KEYSCAN_IN_BUFFER,0xff );//pull low 

	SPI_WriteReg(KEYSCAN_IN1_ATTR, 0x00);//
	SPI_WriteReg(KEYSCAN_IN1_DIR, 	0x00);//0xff 
	SPI_WriteReg(KEYSCAN_IN1_BUFFER,0xff );//pull low 
	printk("init key board finished!\n");
	return;
}

struct file_operations keyboard_fops = 
{
	.owner          = THIS_MODULE,
	.open           = gp_keyboard_dev_open,
	.release        = gp_keyboard_dev_release,
	.unlocked_ioctl = gp_keyboard_dev_ioctl,
	.read			= gp_keyboard_dev_read,
	.write			= gp_keyboard_dev_write,
};

/**
 * @brief   keyboard device resource
 */
static struct platform_device gp_keyboard_device = {
	.name	= "gp-keyboardx",
	.id	= 0,
};

/**
 * @brief   keyboard device probe
 * spi1 dev be used,so spi dev must be opened
 */
static int gp_keyboard_probe(struct platform_device *pdev)
{
	int ret = 0;
	printk("gp_keyboard_probe....\n");
	gp_keyboard_data = kzalloc(sizeof(gp_keyboard_t), GFP_KERNEL);
	if(!gp_keyboard_data){
		return -ENOMEM;
	}
	memset(gp_keyboard_data, 0, sizeof(gp_keyboard_t));

	keyboard_spigpba02a_init(pdev);
	KeyScan_Initial();

	gp_keyboard_data->dev.name = KEYBOARD_NAME;
	gp_keyboard_data->dev.minor  = MISC_DYNAMIC_MINOR;
	gp_keyboard_data->dev.fops  = &keyboard_fops;

	ret = misc_register(&gp_keyboard_data->dev);
	if(ret != 0){
		DIAG_ERROR("keyboard probe register fail\n");
		return -1;
	}
	
	platform_set_drvdata(pdev,&gp_keyboard_data);

	init_MUTEX(&gp_keyboard_data->keyboard_sem);
	return 0;
}

/**
 * @brief   keyboard device remove
 */
static int gp_keyboard_remove(struct platform_device *pdev)
{
	misc_deregister(&gp_keyboard_data->dev);
	kfree(gp_keyboard_data);
	return 0;
}

static struct platform_driver gp_keyboard_driver = {
	.probe	= gp_keyboard_probe,
	.remove	= gp_keyboard_remove,
	.driver	= 
	{
		.owner	= THIS_MODULE,
		.name	= "gp-keyboardx"
	},
};

/**
 * @brief   keyboard driver init
 */
static int __init gp_keyboard_drv_init(void)
{
	printk("gp_keyboard_drv_init\n");
	platform_device_register(&gp_keyboard_device);
	return platform_driver_register(&gp_keyboard_driver);
}

/**
 * @brief   keyboard driver exit
 */
static void __exit gp_keyboard_drv_exit(void)
{
	printk("gp_keyboard_drv_exit\n");
	platform_device_unregister(&gp_keyboard_device);
	platform_driver_unregister(&gp_keyboard_driver);
}

module_init(gp_keyboard_drv_init);
module_exit(gp_keyboard_drv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP KeyBoardx Driver");
MODULE_LICENSE_GP;