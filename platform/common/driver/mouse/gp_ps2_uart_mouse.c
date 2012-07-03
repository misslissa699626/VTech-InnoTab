/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
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
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_mouse.c
 * @brief ps2 mouse driver interface 
 * @author zaimingmeng
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/gp_gpio.h>
#include <mach/gp_timer.h>
#include <mach/gp_mouse.h>
//#include <mach/spmp_sram.h>
#include <mach/hal/hal_uart.h>
#include <linux/serial_reg.h>	/*regs like UART_IER_THRI...*/
//#include <mach/gp_clock.h>
#include <linux/serial_core.h>
//#include <sys/time.h>
//#include <time.h>
#include <mach/gp_board.h>

typedef struct gp_mouse_s {	
	int pin_clk;
	int pin_data;
	
	struct timer_list 		timer;        
	struct input_dev *input;
} gp_mouse_t;


static struct input_dev *input;//20110310

#define BUFNUMMAX		4
#define MOUSEMSGMAX	         4
#define LeftBtn			0x01
#define RightBtn			0x02

static struct gp_mouse_s gp_mouse_data;
static int gpMouseIrqFlag;
static int gpMouseSystemFlag;
static int gpMouseRestartFlag;
static int gpMouseDataReg;
static int gpMouseBitCount;
static int gpMouseDataBufNum;
static int gpMouseReceiveBuffer[BUFNUMMAX];
static int gpMousetype;
static int gpMouseErrDataFlag;
static int gpMouseTimeCount;
static int gpMouseCheckFlag;
static int gpMousejishu;
static int gpMouseInitDelay;

static int gpMouseUartFlag = 0;

static gp_board_ps2_uart_mouse_t	*p_gp_board_ps2_uart_mouse_t = NULL;


/* define the clk data mcroe */

#define MouseClk_Input_No			gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_clk, GPIO_PULL_FLOATING)	//Clk, input gpio mode floating
									
#define MouseClk_Input_Up			gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_clk, GPIO_PULL_HIGH)	//Clk, input gpio mode pull high
	
#define MouseClk_OutputLow		gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_clk, 0);	//Clk, output gpio mode
	
#define MouseClk_OutputHigh		gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_clk, 1);	//Clk, output gpio mode

#define MouseData_Input_No			gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_data, GPIO_PULL_FLOATING)		//Data, input gpio mode floating
	
#define MouseData_Input_Up			gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_data, GPIO_PULL_HIGH)		//Data, input gpio mode pull high
	
#define MouseData_OutputLow		gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_data, 0);		//Data, output gpio mode
	
#define MouseData_OutputHigh		gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_data, 1);		//Data, output gpio mode

#define MouseClk_SetLow_Int			gp_gpio_irq_property(gp_mouse_data.pin_clk, GPIO_IRQ_LEVEL_LOW, NULL)

#define MouseClk_Disable_Int			gp_gpio_enable_irq(gp_mouse_data.pin_clk, 0)

#define MouseClk_Enable_Int			gp_gpio_enable_irq(gp_mouse_data.pin_clk, 1)
		
void gp_mouse_report_key(int *buffer);
signed int timerID;
int handle,handle_clk;
int callback_ID;
uart_cfg_t* mouse_uart_config = NULL;

static unsigned int MOUSE_CLK_GPIO;
static unsigned int MOUSE_DATA_GPIO;


//struct timeval time_start;
//struct timeval time_end;
signed int Global_PS2mouse_clk_halfperiod = 0;
int R_PS2mouse_clk_test_flag = 0;
int R_PS2mouse_uart_request_flag = 0;
static int gpMousejishu1;
//static int temp_mouse_sysflag;


static int gp_mouse_request_gpio(void);
//static void gp_mouse_gpioIrq_test_clk_freq(void);
static void gp_mouse_gpioIrq_test_clk_freq_1(void);
static void reset_insmod_mouse(void);//20110310
static void gp_mouse_gpioIrq(void);// int value, void *data);

//extern int gettimeofday(struct timeval *tv, struct timezone *tz);

//extern long GetTimeTick();
//{ 
//     struct timeval
//	 {
//		 long tv_sec, tv_usec;
//	 }tv;
//
//     gettimeofday(&tv, NULL);
//
//     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
//}


struct gp_mouse_uart_port {
	struct uart_port	port;
	unsigned char		ier;	/*interrupt enable register*/
	unsigned char		lcr;	/*line control register*/
	unsigned char		mcr;	/*modem	control	register*/
	unsigned int		lsr_break_flag;
	struct clk		*clk;
	char			*name;
};

static struct gp_mouse_uart_port gp_mouse_uart = {
	.name	= "UARTC0",
	.port	= {
		.type		= PORT_SPMP8000,
		.iotype		= UPIO_MEM,
		.membase	= (void	*)&(*((volatile	u32 *)IO2_ADDRESS(0xB04000))),
		.mapbase	= 0x92B4000,
		.irq		= IRQ_UART_C0,		
		.uartclk	= 0,
		.fifosize	= 16,
		//.ops		= &gp_uart_pops,
		.line		= 1,
	},
};

int last_uart_data=0x0;
int cur_uart_data=0x0;
static inline irqreturn_t gp_mouse_uart_irq(int irq, void *dev_id)
{
	struct gp_mouse_uart_port *up	= dev_id;
	unsigned int iir;
	int temp1 = 0;

//	printk("gp_mouse_uart_irq() entrance\n"); 

	iir = gpHalUartGetIntFlags(up->port.line);
	if (iir & UART_IIR_NO_INT)
	{
		//printk("(iir & UART_IIR_NO_INT)\n");		
		return IRQ_NONE;
	}
		
	do{
		temp1 = *((volatile unsigned int *)(0xfdb04014));           //Line statue Reg
		if( ((temp1 & 0x04) == 0) && ((temp1 & 0x01) == 1) )
		{
			cur_uart_data = *((volatile unsigned int *)(0xfdb04000));//Received buffer Reg
			gpMouseReceiveBuffer[gpMouseDataBufNum] =  cur_uart_data;        //Received buffer Reg
//			printk("0x%x\n",gpMouseReceiveBuffer[gpMouseDataBufNum]);
			if((gpMouseDataBufNum == 0) && ((cur_uart_data & 0x08) == 0x00))//not first byte of a data stream
			{
			}
			else
			{
		    		gpMouseDataBufNum += 1 ;    
			}
			if((last_uart_data == 0xaa) && (cur_uart_data == 0x00))//mouse plug\power on signal
			{
				printk("mouse plug\n");
				reset_insmod_mouse();
				break;
			}
			last_uart_data = cur_uart_data;                           
		}
//		if(gpMouseDataBufNum >= 2)//power on
//		{
//			if(((gpMouseReceiveBuffer[0] == 0xaa) && (gpMouseReceiveBuffer[1] == 0x00)) 
//			|| ((gpMouseReceiveBuffer[1] == 0xaa) && (gpMouseReceiveBuffer[2] == 0x00))
//			|| ((gpMouseReceiveBuffer[2] == 0xaa) && (gpMouseReceiveBuffer[3] == 0x00)))//mouse power on signal
//			{
//				printk("mouse power on\n");
//				printk("reset_insmod_mouse()\n");
//				reset_insmod_mouse();
//				break;
//			}
//		}
		if(gpMouseDataBufNum >= 4)
		{
		    gpMouseDataBufNum = 0;
//			printk("gpMouseDataBufNum >= 4\n");
			//printk("g\n");
		    gp_mouse_report_key(gpMouseReceiveBuffer);
		}
	}while(temp1 & 0x01);

	//printk("gp_mouse_uart_irq() exit\n");
	return 0;
}

static void gp_mouse_init(void)
{
	unsigned int apbHz;
	struct clk *apbClk;
	unsigned int temp,i;

	printk("gp_mouse_init start\n");
	if(R_PS2mouse_clk_test_flag == 0)
	{
		/* apply timer service, get the apb clock value */
		apbClk = clk_get(NULL,"clk_arm_apb");
		if(apbClk){
			apbHz = clk_get_rate(apbClk);
			printk (KERN_NOTICE "apbHz = %d\n",apbHz);
			clk_put(apbClk);
		}
		else{
			apbHz = 21000000;
			printk("apb use default clock 21M\n");
		}
	
		temp = apbHz/10000000 - 1 ;
	
		timerID = 0;
		for (i=0;i<5 && timerID==0;i++) {
//			printk("ready to request timer %d\n",i);
//			printk("-------------------------------\n");
			timerID = gp_tc_request(i,"ps2-mouse");
//			printk("//-------------------------------//\n");
		}
///		if(i >= 5)
//		{
//			printk("request timer fail\n");
//		}
		printk("ps2mouse_timerID is %d\n",timerID);
		/* setup timer */
		//gp_tc_set_prescale(timerID,temp);
		//gp_tc_set_operation_mode(timerID,1); /* period timer mode */
		//gp_tc_set_count_mode(timerID,1);
		//gp_tc_enable_int(timerID,1);
		gp_tc_set_prescale(timerID,temp);
		gp_tc_set_operation_mode(timerID,0);// 0:free running time mode; 1:period timer mode; 
	 						//2:free running counter mode; 3:period counter mode
		gp_tc_set_count_mode(timerID,1);//up count
		gp_tc_enable_int(timerID,0);//disable interrupt
		gp_tc_enable(timerID);//20110309
	}

	MouseClk_OutputLow;
	for(i = 0; i < 0x200; i++)//20110314
	{
//		asm{"nop"};
		MouseClk_OutputLow;
		
	}
	MouseData_OutputLow;	
	MouseData_Input_Up;
	MouseClk_Input_Up;
	MouseClk_SetLow_Int;
	MouseClk_Disable_Int;

	gpMouseSystemFlag = 0;
	gpMousetype = 3;
	gpMouseIrqFlag = ReceiveFlag;
	gpMouseBitCount = 0;
	gpMouseDataBufNum = 0 ;
	gpMouseTimeCount = 0;
	gpMouseErrDataFlag = 0;
	gpMouseCheckFlag = 0;
	gpMouseInitDelay = 0;

	MouseClk_Enable_Int;
	printk("gp_mouse_init exit\n");
}

void gpMouseInitUart(void)
{
	int retval;
	int temp1;
	
	struct gp_mouse_uart_port *up = &gp_mouse_uart;

//uart init
	temp1 = *((volatile unsigned int *)(0xfc005144));
	//printk("0xfc005144 = 0x%x\n", temp1); 
	temp1 |= 0x01;
	*((volatile unsigned int *)(0xfc005144)) = temp1;                 //disale ARM Jtag for using uart0
	
	*((volatile unsigned int *)(0xfc807094)) = 0x00050100;      //SCUA uart clock config: Set UART0 clock to 27MHz

	if(mouse_uart_config == NULL)
	{
		mouse_uart_config = kmalloc(sizeof (uart_cfg_t), GFP_KERNEL);
        }
        mouse_uart_config->bits_per_word = 8;   
        mouse_uart_config->parity = 4;      //ODD parity
        //mouse_uart_config->parity = 0;      //No parity
        mouse_uart_config->stop_bits = 0;
        mouse_uart_config->auto_flow_ctrl = 0;
       //mouse_uart_config->baudrate = 13890;//play boy pad
//        mouse_uart_config->baudrate = 12200;//play boy ps2mouse
       // mouse_uart_config->baudrate = 13158;
        //mouse_uart_config->baudrate = 115200;
	
//	DIAG_ERROR("Global_PS2mouse_clk_halfperiod = %d\n", Global_PS2mouse_clk_halfperiod);
	//Global_PS2mouse_clk_halfperiod *= 2;
	//Global_PS2mouse_clk_halfperiod /= 1000000;
	mouse_uart_config->baudrate = 10000000/Global_PS2mouse_clk_halfperiod;
	DIAG_ERROR("mouse_uart_config->baudrate = %d\n", mouse_uart_config->baudrate);

	/*
	 * Ensure that no interrupts are enabled otherwise when
	 * request_irq() is called we could get stuck trying to
	 * handle an unexpected interrupt
	 */        
	//gpHalUartSetIntEn(up->port.line, 0);

	/*
	 * Allocate the	IRQ
	 */
	//if(R_PS2mouse_clk_test_flag == 0)
	if(R_PS2mouse_uart_request_flag == 0)
	{
		retval = request_irq(up->port.irq, gp_mouse_uart_irq, 0, up->name, up);
		if(retval)
		{
		    printk("request_irq Fail !!\n"); 
		}
		else
		{
			printk("request_irq uart successful\n");
		}
		R_PS2mouse_uart_request_flag = 1;
	}
	clk_enable(up->clk);
	/*
	 * Clear the FIFO buffers and disable them.
	 * (they will be reenabled in set_termios())
	 */
	gpHalUartSetFifoCtrl(up->port.line, UART_FCR_ENABLE_FIFO);
	gpHalUartSetFifoCtrl(up->port.line, UART_FCR_ENABLE_FIFO | UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	gpHalUartSetFifoCtrl(up->port.line, 0);        
	/*
	 * Clear the interrupt registers.
	 */
	(void) gpHalUartClrIntFlags(up->port.line);

	/*
	 * Now,	initialize the UART
	 */        
        gpHalUartInit(up->port.line, mouse_uart_config);
        *((volatile unsigned int *)(0xfdb04004)) = 0x5; 	//Enable received data INT
	*((volatile unsigned int *)(0xfdb04008)) = 0x1; 	//FIFO enable
	/*
	 * And clear the interrupt registers again for luck.
	 */	
	(void) gpHalUartClrIntFlags(up->port.line);
//uart end    
}

//===================================================================
void SendDatatoDevice(int tdata)
{
	int temp;
	int t1,i,tt;		
	MouseClk_Disable_Int;			//disable INT
	gpMouseDataBufNum = 0;
	tdata&=0xff;		
	temp=tdata;
	t1=1;
	for(i=0;i<8;i++)	
	{
		tt = temp&(1<<i);
		if(tt)
		{	t1^=1;	}
	}	
	t1|= 0x2;
	t1<<=8;
	temp=tdata+t1;
	gpMouseDataReg=temp;
	//----------------
	MouseClk_OutputLow;		//Set clock send low
	
	for(i = 0; i < 0x200; i++)
	{
//		asm{"nop"};
		MouseClk_OutputLow;
		
	}
	MouseClk_Input_Up;
		
	MouseData_OutputLow;		//set send data 0
			
	gpMouseIrqFlag = ReceiveFlag|SendFlag;
	gpMouseBitCount = 0;
	
	MouseClk_SetLow_Int;		//Set High to Low trigger
	MouseClk_Enable_Int;	 	//Enable INT
}

	

static int gp_mouse_request_gpio(void)
{
	int ret = 0;

//	MOUSE_CLK_GPIO = board_config_get_ps2mouse_clk_gpio();
//	MOUSE_DATA_GPIO = board_config_get_ps2mouse_data_gpio();
	
	gp_mouse_data.pin_clk = gp_gpio_request(MOUSE_CLK_GPIO, "mouse");
	if(IS_ERR_VALUE(gp_mouse_data.pin_clk)){
		DIAG_ERROR("mouse request gpio clk = %d\n", gp_mouse_data.pin_clk);
		return -EINVAL;
	}
	gp_gpio_set_function(gp_mouse_data.pin_clk, GPIO_FUNC_GPIO);
	gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_OUTPUT);
	//gp_gpio_set_value(gp_mouse_data.pin_clk, 1);
	
	gp_mouse_data.pin_data = gp_gpio_request(MOUSE_DATA_GPIO, "mouse");
	if(IS_ERR_VALUE(gp_mouse_data.pin_data)){
		DIAG_ERROR("mouse request gpio data = %d\n", gp_mouse_data.pin_data);
		goto error;
	}
	gp_gpio_set_function(gp_mouse_data.pin_data, GPIO_FUNC_GPIO);
	//gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_INPUT);
	//gp_gpio_set_pullfunction(gp_mouse_data.pin_data, GPIO_PULL_HIGH);

	return ret;

error:
	gp_gpio_release(gp_mouse_data.pin_clk);

	return -EINVAL;
}


void MouseTimerReturnPro(void)
{
	gpMouseBitCount = 0;
	//MouseWriteCount = 0;
	gpMouseDataBufNum = 0 ;
	gpMouseTimeCount = 0;
	gpMouseErrDataFlag = 0;
	gpMouseCheckFlag = 0;
	//MouseClk_Clear_Flag;		//Clear flag
	MouseClk_SetLow_Int;		//Set High to Low trigger
	MouseClk_Enable_Int;		//Enable INT
}

void mouserest(void)
{
	gpMouseDataBufNum = 0;

	gpMouseBitCount = 0;
	gpMouseSystemFlag = 0;		
	gpMouseErrDataFlag=0;
	gpMousejishu = 0;
	SendDatatoDevice(0xff);	
//	if(R_PS2mouse_clk_test_flag == 0)
//	{
//		gpMouseInitDelay = 5;
//	}
//	else
//	{
		gpMouseInitDelay = 3;//10;
//	}
	gpMousejishu1 = 0;
	//temp_mouse_sysflag = 0;
}

//===================================================================
int TestMouseData(int data)     //verify data 
{
	//int temp ,t1,i,tt;
	//temp = data ;
	//printk("data = 0x%x\n",data);
	
	if(data < 0x400 || data >= 0x800)
	{
	    //printk("data < 0x400 || data >= 0x800, data = 0x%x\n",data);
	    return 1;
	}
	if(data&0x1)
	{
	    //printk("data%2, data = 0x%x\n",data);	
	    return 1;
	}
/*	t1=1;
	for(i=0;i < 11;i++)	
	{
		tt = temp&(1<<i);
		if(tt)
		{	t1^=1;	}
	}
	if(!t1)
	{
	    //printk("parity error, data = 0x%x\n",data);	
	    return 1;
	}
*/
	return 0;
}

int MouseTimerCheck(int *PacketReg)
{	
	int temph = PacketReg[0];
	int tempm = PacketReg[1];
	int tempe = PacketReg[2];
	int tempf = PacketReg[3];
//	if(MouseDataBufNum>=Mousetype)
//	{ 
//		MouseDataBufNum -= Mousetype;
//	}
//	else return 1;
	if((!(temph&0x010))|| ((temph&0x100)) || ((temph&0x80)))
	{											//Allways1,YOverFlow ,XOverFlow		
		return 1;
	}
	else
	{
		PacketReg[0] = (temph>>1)&0xff ;
		PacketReg[1] = (tempm>>1)&0xff ;
		PacketReg[2] = (tempe>>1)&0xff ;
		if( gpMousetype==4 ) PacketReg[3] = (tempf>>1)&0xff ;		
	} 
	return 0;	
}

void MouseTimerErrPro(void)
{
	MouseClk_Disable_Int;	
	
	//MouseClk_Clear_Flag;	
	MouseClk_SetLow_Int;

	gpMouseBitCount = 0;
	//MouseWriteCount = 0;
	gpMouseDataBufNum = 0 ;
	//mousetimenum = 0;
	gpMouseErrDataFlag = 1;
}


//-------------------------------------------------
void MouseSendDataInIrq(void)
{

	switch(gpMouseBitCount)
	{	
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			if(gpMouseDataReg&0x1)
			{
				MouseData_Input_Up;//data high
			//	MouseData_OutputHigh;
			}
			else
			{
				MouseData_OutputLow;//data low
			}
			gpMouseBitCount += 1 ;	
			gpMouseDataReg = gpMouseDataReg>>1;
			break;
		case 9:			//mouse send stop bit	
			MouseData_Input_Up;//data high
			gpMouseBitCount += 1 ;		
			break;
/*		case 10:
			MouseData_Input_Up;//data no	
			
			MouseClk_Dienable_IntAll;
			MouseClk_SetRise_Int;
			MouseClk_Enable_IntAll;	
			MouseBitCount += 1;
			break;*/
		case 10:		//SendOverss
			MouseData_Input_No;
			gpMouseBitCount = 0;
			gpMouseIrqFlag &= ~0x2 ;
			gpMouseDataReg = 0;		
			break;
	}
}


//-----------------------------------------------------------

void MouseReceiveDataInIrq(void)
{
	int Bit;
	gp_gpio_get_value(gp_mouse_data.pin_data, &Bit);
	
	if(Bit)
	{
		gpMouseDataReg |= (1<<gpMouseBitCount);
		gpMouseBitCount += 1;
	}
	else
	{
		gpMouseDataReg &= ~(1<<gpMouseBitCount);
		gpMouseBitCount += 1;
	}
	if( gpMouseBitCount >= 11 )
	{
		gpMouseBitCount = 0 ;
		if( gpMouseDataBufNum<BUFNUMMAX )
		{
			gpMouseReceiveBuffer[gpMouseDataBufNum] = gpMouseDataReg ;
			gpMouseDataBufNum += 1 ;
			gpMouseDataReg = 0;
		}
	}
}

void gp_mouse_report_key(int *buffer)
{
	int datax;
	int datay;
	
	if(buffer[0]&0x10){
		datax = buffer[1]&0xff;
		datax += 0xffffff00;
	}
	else{
		datax = buffer[1]&0xff;
	}
         input_report_rel((gp_mouse_data.input), REL_X, datax);
	if(buffer[0]&0x20){
		datay = buffer[2]&0xff;
		datay += 0xffffff00;
	}
	else{
		datay = buffer[2]&0xff;
	}
	//----------------------------------------------------------
	//buffer[0] ^= 0x20;
	//buffer[2] = -buffer[2];
	datay = -datay;//20110315
	//----------------------------------------------------------
	input_report_rel((gp_mouse_data.input), REL_Y, datay);

	if(buffer[0]&LeftBtn){
		input_report_key(gp_mouse_data.input, BTN_LEFT,   1);
	}
	else{
		input_report_key(gp_mouse_data.input, BTN_LEFT,   0);
	}
	
	if(buffer[0]&RightBtn){
		input_report_key(gp_mouse_data.input, BTN_RIGHT,   1);
	}
	else{
		input_report_key(gp_mouse_data.input, BTN_RIGHT,   0);
	}

	if(gpMousetype == 4){
		if(buffer[3] == 0x01){
			input_report_rel((gp_mouse_data.input), REL_WHEEL, 1);
		}
		else if(buffer[3] == 0xff){
			input_report_rel((gp_mouse_data.input), REL_WHEEL, -1);
		}
	}

	input_sync(gp_mouse_data.input);
}

void MouseReceiveData_Uart(void)
{
         int Ret,temp3;
         char Receive_buffer;
         
         struct gp_mouse_uart_port *up = &gp_mouse_uart;
         
         temp3 = *((volatile unsigned int *)(0xfdb0400C));
         
         temp3 &= 0x7;
         *((volatile unsigned int *)(0xfdb0400C)) = temp3;      //make sure the DLAB is disable
	//printk("0xfdb04000 = 0x%x\n",temp3);         
         
         Ret = gpHalUartRead(up->port.line, &Receive_buffer, 1);
         printk("received data = 0x%x\n",Receive_buffer);   
}

void MouseReceiveDataInIrqOK(void)
{
	int Bit;
	int erro=0;

	gp_gpio_get_value(gp_mouse_data.pin_data, &Bit);

	if(gpMouseBitCount==0&&gpMouseDataBufNum==0)
	{
		gpMouseTimeCount = 0;
		gpMouseCheckFlag = 1;
	}
	if(Bit)
	{
		gpMouseDataReg |= (1<<gpMouseBitCount);
		gpMouseBitCount += 1;
	}
	else
	{
		gpMouseDataReg &= ~(1<<gpMouseBitCount);
		gpMouseBitCount += 1;
	}
	
	if( gpMouseBitCount >= 11 )
	{
		gpMouseBitCount = 0 ;
		gpMouseReceiveBuffer[gpMouseDataBufNum] = gpMouseDataReg ;
		gpMouseDataBufNum += 1 ;

		erro = TestMouseData(gpMouseDataReg);//del by erichan
		if(erro)
		{
			gpMouseBitCount = 0;
			gpMouseDataBufNum = 0;
			//MouseTimerErrPro();	
		}
		else
		{
			if((gpMouseDataReg&0x1ff)==0x154)
			{
				gpMouseDataBufNum = 0;
				gpMouseSystemFlag = 0;
			}
		}		
		if( gpMouseDataBufNum>=4 )
		{
			gpMouseCheckFlag = 0 ;
			gpMouseDataBufNum = 0 ;
			erro = MouseTimerCheck((int *)gpMouseReceiveBuffer);
			if(erro)
			{
				MouseTimerErrPro();
				//printk("mouse_other_erro\n");
			}
			else
			{
				    gp_mouse_report_key(gpMouseReceiveBuffer);
			}
		}
	}
}


void gp_init_step(void)
{
	int tdata;			
	if( gpMouseRestartFlag == 0 )
	{	
		printk("mouse reset()\n");
		mouserest();
		gpMouseRestartFlag = 1;
	}
	if( gpMouseDataBufNum==0)
	{		
		gpMousejishu ++;
		//printk("gpMousejishu == %d\n",gpMousejishu);
		if(gpMousejishu >=1000)//0x400000
		{			
			gpMouseRestartFlag = 0;
			gpMousejishu = 0 ;
			//mouseInitisrflag = 0 ;
		}
		//if(gpMouseSystemFlag < 0x8000)
		//{
		//	if(temp_mouse_sysflag == gpMouseSystemFlag)
		//	{	
		//		if(++gpMousejishu1 > 20)
		//		{
		//			mouserest();
		//		}
		//	}
		//	else
		//	{
		//		temp_mouse_sysflag = gpMouseSystemFlag;
		//		gpMousejishu1 = 0;
		//	}
		//}
		//else	
		//{
		//	gpMousejishu1 = 0;
		//}
	}
 	if(gpMouseDataBufNum>0)
	{
		//Mousejishu = 0;
		if(TestMouseData(gpMouseReceiveBuffer[gpMouseDataBufNum-1]))
		{
			gpMouseRestartFlag = 1;
			mouserest();
			return;	
		}
		tdata = ((gpMouseReceiveBuffer[0]>>1)&0xff);
		printk("mousedata = %x ,gpMouseSystemFlag = %x\n",tdata,gpMouseSystemFlag);
		//printk("%x %x\n",tdata,gpMouseSystemFlag);
		gpMouseReceiveBuffer[0] = gpMouseReceiveBuffer[1];
		gpMouseReceiveBuffer[1] = gpMouseReceiveBuffer[2];
		gpMouseReceiveBuffer[2] = gpMouseReceiveBuffer[3];
		gpMouseDataBufNum -=1;
		switch(tdata)
		{			
			case 0xaa:
				if(gpMouseSystemFlag == 0)
				{		
					gpMouseSystemFlag = 0x01;
				}
				else
				{
					mouserest();
				}
				break;
			case 0x00:		
				if( gpMouseSystemFlag==0x01 )
				{
					SendDatatoDevice(0xf3);
					gpMouseSystemFlag = 0x2;
					//-------------------------------------------------------------------------------------------20110509
					if(R_PS2mouse_clk_test_flag == 0)
					{
						gp_gpio_register_isr(gp_mouse_data.pin_clk, (void (*)(void *))gp_mouse_gpioIrq_test_clk_freq_1, ( void * )(gp_mouse_data.pin_clk)) ;
						gpMousejishu1 = 0;
					}
					//-------------------------------------------------------------------------------------------
				}
//				else if( gpMouseSystemFlag==0x100 )
//				{
//					SendDatatoDevice(0xf3);
//					gpMouseSystemFlag = 0x200;		
//					Mousetype = 3 ;
//				}
				else
				{
					mouserest();
				}
				break;
			case 0x03:
				if( gpMouseSystemFlag==0x100 )
				{
					gpMousetype = 4 ;
				//	SendDatatoDevice(0xf3);
					SendDatatoDevice(0xe8);
					gpMouseSystemFlag = 0x200;
					
				}
				else
				{
					mouserest();
				}
				break;							
			case 0xfa:
				if( gpMouseSystemFlag == 0 )
				{
					gpMouseSystemFlag = 0;
					//SendDatatoDevice(0xf3);             //command set sampling rate
					//gpMouseSystemFlag = 0x2;								
				}
				else if( gpMouseSystemFlag==0x02 )
				{
					SendDatatoDevice(0xc8);             //200
					gpMouseSystemFlag = 0x04;
				}
				else if( gpMouseSystemFlag==0x04 )
				{
					SendDatatoDevice(0xf3);             //command set sampling rate
					gpMouseSystemFlag = 0x08;
				}
				else if( gpMouseSystemFlag==0x08 )
				{
					SendDatatoDevice(0x64);             //100
					gpMouseSystemFlag = 0x10;
				}
				else if( gpMouseSystemFlag==0x10 )
				{
					SendDatatoDevice(0xf3);             //command set sampling rate
					gpMouseSystemFlag = 0x20;
				}
				else if( gpMouseSystemFlag==0x20 )
				{
					SendDatatoDevice(0x50);             //80   
					gpMouseSystemFlag = 0x40;
				}
				else if( gpMouseSystemFlag==0x40 )
				{
					//printk("Test 1\n");
					SendDatatoDevice(0xf2);                 //Get device ID
					//printk("Test 2\n");					
					gpMouseSystemFlag = 0x80;
				}
				else if (gpMouseSystemFlag == 0x80)
				{
					gpMouseSystemFlag = 0x100;
				}
				else if (gpMouseSystemFlag == 0x200)
				{
					SendDatatoDevice(0x03);
					//SendDatatoDevice(0x14);
					gpMouseSystemFlag = 0x400;
				}
				else if (gpMouseSystemFlag == 0x400)//set sample rate
				{
					SendDatatoDevice(0xf3);             //command set sampling rate
					gpMouseSystemFlag = 0x800;
				}
				else if (gpMouseSystemFlag == 0x800)
				{
					SendDatatoDevice(0xc8);               //actually use this function to set sample rate
					gpMouseSystemFlag = 0x1000;
				}				
				else if (gpMouseSystemFlag == 0x1000)
				{
					SendDatatoDevice(0xf4);             //Enable data reporting
					gpMouseSystemFlag = 0x2000;
				}
				else if (gpMouseSystemFlag == 0x2000)
				{
					gpMouseSystemFlag = 0x8000;
					gpMouseDataBufNum = 0;
					gpMouseErrDataFlag=0;
					//Mousejishu = 0;

					//MouseNum = 0;
					//MouseRCount = 0;
					//MouseCount = 0;					
					printk("ps2_uart_mouse_InitGood_data\n");
				}
				else
				{
					mouserest();
				}							
				break;
			default:				
				mouserest();//20110314 mask
				//printk("mouse re init\n");//20110314
				//gp_mouse_init();//20110314
				//gpMouseRestartFlag = 0;//20110314
				//gpMouseUartFlag = 0;//20110314
				//last_uart_data = 0;//20110314
				//cur_uart_data = 0;//20110314
				break;
		}
	}

	if(gpMouseSystemFlag < 0x8000)//20110315
	{
		if(++gpMousejishu1 > 15)
		{
			gpMousejishu1 = 0; 
			gpMouseRestartFlag = 0;
		}		
	}
}

//int gpMouseUartFlag = 0;
static void gp_mouse_time(unsigned long arg)
{
	int temp1;
	
	gp_mouse_data.timer.expires = jiffies + HZ/5;

	add_timer(&gp_mouse_data.timer);

	//printk("gp_mouse_time()\n");

	if(gpMouseSystemFlag&MOUSEStepFlag)	
	{
		if(gpMouseUartFlag == 0)
		{            	        
			gpMouseUartFlag = 1;
			//printk("gp_mouse_time() / gpMouseUartFlag == 0\n");
			//gp_gpio_release_irq(0, callback_ID); //del by erihan
			gp_gpio_unregister_isr (gp_mouse_data.pin_clk) ;
//			if(R_PS2mouse_clk_test_flag == 0)
//			{
//				gp_gpio_register_isr (gp_mouse_data.pin_clk, gp_mouse_gpioIrq_test_clk_freq, ( void * )(gp_mouse_data.pin_clk)) ; //20110309
// 			}
//			else
//			{
				gp_gpio_release(gp_mouse_data.pin_clk);//20110309mask
				gp_gpio_release(gp_mouse_data.pin_data);
				temp1 = *((volatile unsigned int *)(0xfc005084));      //change pad group of uart clock from 2 to 0
				temp1 &= ~0x80;
				*((volatile unsigned int *)(0xfc005084)) = temp1;
				temp1 = *((volatile unsigned int *)(0xfc005088));
				temp1 &= ~0x02;
				*((volatile unsigned int *)(0xfc005088)) = temp1;      //change pad group of uart data from 2 to 0            
				gpMouseInitUart();
//			}
		}		
		
		if(gpMouseErrDataFlag)
		{
			gpMouseTimeCount += 1;
			if(gpMouseTimeCount>6)
			{
				MouseTimerReturnPro();
			}
		}
		else if(gpMouseCheckFlag)
		{
			gpMouseTimeCount += 1;
			if(gpMouseTimeCount>6)
			{
				gpMouseTimeCount = 0;
				MouseTimerErrPro();
			}		
		}
	}	
	else if(!gpMouseInitDelay){
			do{
				gp_init_step();
			}while(gpMouseDataBufNum);			
	}
	else{
			gpMouseInitDelay --;
	}

	//if((gpMouseSystemFlag >=1) && (gpMouseSystemFlag < 0x8000))
	//{
	//	gp_mouse_data.timer.expires = jiffies + HZ/20;
	//}
	//else
	//{
	//	gp_mouse_data.timer.expires = jiffies + HZ/3;
	//}
	//add_timer(&gp_mouse_data.timer);
}

static void gp_mouse_gpioIrq_test_clk_freq_1(void)
{
	int Bit,Bit1;
	unsigned int start_timerload,end_timerload;
	//int timer_haldle;
	//unsigned int apbHz;
	//struct clk *apbClk;
	unsigned int temp;

//		printk("freq\n");

//	if(R_PS2mouse_clk_test_flag == 0)
//	{
		//gp_tc_set_load(timerID, 0);
		gp_tc_get_load(timerID, (int*)&start_timerload);
		gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit);		
		while(1)
		{
			gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit1);
			if(Bit1 != Bit)
			{
				gp_tc_get_load(timerID, (int*)&start_timerload);
				Bit = Bit1;
				break;
			}
			else
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				temp = end_timerload - start_timerload;
				if(temp < 0)
				{
					temp += 65536;//16bit register
				}
				if(temp >= 20000)//2mS overtime
				{
					printk("2mS overtime = %d\n",temp);					
					return;
				}
			}
		}
		while(1)
		{
			gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit1);
			if(Bit1 != Bit)
			{
				//gp_tc_get_load(timerID, (int*)&end_timerload);
				Bit = Bit1;
				break;
			}
			else
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				temp = end_timerload - start_timerload;
				if(temp < 0)
				{
					temp += 65536;//16bit register
				}
				if(temp >= 20000)//2mS overtime
				{
					printk("2mS overtime1 = %d\n",temp);						
					return;
				}
			}
		}
		while(1)
		{
			gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit1);
			if(Bit1 != Bit)
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				break;
			}
			else
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				temp = end_timerload - start_timerload;
				if(temp < 0)
				{
					temp += 65536;//16bit register
				}
				if(temp >= 20000)//2mS overtime
				{
					printk("2mS overtime2 = %d\n",temp);						
					return;
				}
			}
		}
		Global_PS2mouse_clk_halfperiod = end_timerload - start_timerload;
		if(Global_PS2mouse_clk_halfperiod < 0)
		{
			Global_PS2mouse_clk_halfperiod += 65536;//16bit register
		}
		//----------------------------------------------------------------------------------------------2011030
//		gp_gpio_unregister_isr (gp_mouse_data.pin_clk) ;
//		gp_gpio_release(gp_mouse_data.pin_clk);
//		gp_gpio_release(gp_mouse_data.pin_data);
//		temp = *((volatile unsigned int *)(0xfc005084));      //change pad group of uart clock from 2 to 0
//		temp &= ~0x80;
//		*((volatile unsigned int *)(0xfc005084)) = temp;
//		temp = *((volatile unsigned int *)(0xfc005088));
//		temp &= ~0x02;
//		*((volatile unsigned int *)(0xfc005088)) = temp;      //change pad group of uart data from 2 to 0            
//		gpMouseInitUart();
		//gpMouseUartFlag = 1;
		//----------------------------------------------------------------------------------------------
		R_PS2mouse_clk_test_flag = 1;
		printk("timerID = %d\n",timerID);
		gp_tc_disable(timerID);
		gp_tc_release(timerID);
		printk("gp_tc_release timerID = %d\n",timerID);
		
		gp_gpio_register_isr(gp_mouse_data.pin_clk, (void (*)(void *))gp_mouse_gpioIrq, ( void * )(gp_mouse_data.pin_clk)) ;
		mouserest();//20110509
//	}	
}

/*
static void gp_mouse_gpioIrq_test_clk_freq()
{
	int Bit,Bit1;
	unsigned int start_timerload,end_timerload;
	//int timer_haldle;
	//unsigned int apbHz;
	//struct clk *apbClk;
	unsigned int temp;

//		printk("freq\n");

//	if(R_PS2mouse_clk_test_flag == 0)
//	{
		//gp_tc_set_load(timerID, 0);
		gp_tc_get_load(timerID, (int*)&start_timerload);
		gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit);		
		while(1)
		{
			gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit1);
			if(Bit1 != Bit)
			{
				gp_tc_get_load(timerID, (int*)&start_timerload);
				Bit = Bit1;
				break;
			}
			else
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				temp = end_timerload - start_timerload;
				if(temp < 0)
				{
					temp += 65536;//16bit register
				}
				if(temp >= 20000)//2mS overtime
				{
					printk("2mS overtime = %d\n",temp);					
					return;
				}
			}
		}
		while(1)
		{
			gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit1);
			if(Bit1 != Bit)
			{
				//gp_tc_get_load(timerID, (int*)&end_timerload);
				Bit = Bit1;
				break;
			}
			else
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				temp = end_timerload - start_timerload;
				if(temp < 0)
				{
					temp += 65536;//16bit register
				}
				if(temp >= 20000)//2mS overtime
				{
					printk("2mS overtime1 = %d\n",temp);						
					return;
				}
			}
		}
		while(1)
		{
			gp_gpio_get_value(gp_mouse_data.pin_clk, &Bit1);
			if(Bit1 != Bit)
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				break;
			}
			else
			{
				gp_tc_get_load(timerID, (int*)&end_timerload);
				temp = end_timerload - start_timerload;
				if(temp < 0)
				{
					temp += 65536;//16bit register
				}
				if(temp >= 20000)//2mS overtime
				{
					printk("2mS overtime2 = %d\n",temp);						
					return;
				}
			}
		}
		Global_PS2mouse_clk_halfperiod = end_timerload - start_timerload;
		if(Global_PS2mouse_clk_halfperiod < 0)
		{
			Global_PS2mouse_clk_halfperiod += 65536;//16bit register
		}
		//----------------------------------------------------------------------------------------------2011030
		gp_gpio_unregister_isr (gp_mouse_data.pin_clk) ;
		gp_gpio_release(gp_mouse_data.pin_clk);
		gp_gpio_release(gp_mouse_data.pin_data);
		temp = *((volatile unsigned int *)(0xfc005084));      //change pad group of uart clock from 2 to 0
		temp &= ~0x80;
		*((volatile unsigned int *)(0xfc005084)) = temp;
		temp = *((volatile unsigned int *)(0xfc005088));
		temp &= ~0x02;
		*((volatile unsigned int *)(0xfc005088)) = temp;      //change pad group of uart data from 2 to 0            
		gpMouseInitUart();
		//gpMouseUartFlag = 1;
		//----------------------------------------------------------------------------------------------
//		printk("start_timerload = %d\n",start_timerload);
//		printk("end_timerload = %d\n",end_timerload);
//		printk("1  Global_PS2mouse_clk_halfperiod = %d\n",Global_PS2mouse_clk_halfperiod);
//		if(Global_PS2mouse_clk_halfperiod < 0)
//		{
//			printk("ps2-mouse timer overflow\n");
//		}
		R_PS2mouse_clk_test_flag = 1;
		printk("timerID = %d\n",timerID);
		gp_tc_disable(timerID);
		gp_tc_release(timerID);
		printk("gp_tc_release timerID = %d\n",timerID);
		
//	}	
}
*/

static void gp_mouse_gpioIrq(void)// int value, void *data)
{
	gpMousejishu1 = 0;//20110315
	if(gpMouseIrqFlag&ReceiveFlag)
	{
		if(gpMouseIrqFlag&SendFlag)
		{	
			//	printk("M\n");
			MouseSendDataInIrq();
		}
		else
		{	
			if(gpMouseSystemFlag == MOUSEStepFlag)
			{
				//MouseReceiveDataInIrqOK();                //We don't need to USE this function to receive data after we switch to UART
			}
			else
			{
				//printk("N\n");
				MouseReceiveDataInIrq(); 
			}		
		}	
	}
	//printk("gpioIrq\n");
}

static void reset_insmod_mouse()//20110310
{
	int ret = 0;

//	struct input_dev *input;
	
	*((volatile unsigned int *)(0xfdb04004)) &= ~0x5; 	//disable received data INT
	//*((volatile unsigned int *)(0xfdb04008)) &= ~0x1; 	//FIFO disable
	ret = gp_mouse_request_gpio();
	if(ret){
		printk("gp_mouse_request_gpio() == !0\n");
		return;// ret;
	}
	//gp_gpio_request_irq(0, "mouse", gp_mouse_gpioIrq, NULL);
	
	//callback_ID = gp_gpio_request_irq(0, "mouse", gp_mouse_gpioIrq, NULL);//del by erichan
	gp_gpio_register_isr(gp_mouse_data.pin_clk, (void (*)(void *))gp_mouse_gpioIrq, ( void * )(gp_mouse_data.pin_clk)) ; 
//	input = input_allocate_device();
//	if (!input) {
//		ret = -ENOMEM;
//		printk("input alloc erro\n");
//		goto erro_req;
//	}
	   
//	input->name = pdev->name;
//	input->phys = "mouse/input0";
//	input->dev.parent = &pdev->dev;
       
//	input->id.bustype = BUS_HOST;
//	input->id.vendor = 0x0001;
//	input->id.product = 0x0001;
//	input->id.version = 0x0100;
//	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
//	input->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) |BIT_MASK(BTN_RIGHT);
//	input->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

//	gp_mouse_data.input = input;

//	input_set_drvdata(input, &gp_mouse_data);
    
//	ret = input_register_device(input);
//	if (ret){
//		goto erro_input;
//	}


//	platform_set_drvdata(pdev, &gp_mouse_data);

	gp_mouse_init();
	gpMouseRestartFlag = 0;//20110310
	gpMouseUartFlag = 0;//20110310
	last_uart_data = 0;
	cur_uart_data = 0;
	/*enable the kb*/
//	init_timer(&gp_mouse_data.timer);//20110311mask, do not reinit_timer,it would make "INTERNAL ERROR"
//	gp_mouse_data.timer.data = (unsigned long) &gp_mouse_data;
//	gp_mouse_data.timer.function = gp_mouse_time;
//	gp_mouse_data.timer.expires = jiffies + HZ/5;
//	add_timer(&gp_mouse_data.timer);
	printk("reset_insmod_mouse exit\n");
	return;

//erro_input:
//	input_free_device(input);

//erro_timer:
	//gp_gpio_release(gp_mouse_data.pin_clk);
	//gp_gpio_release(gp_mouse_data.pin_data);
//erro_req:
	
//	return ret;
}

static int gp_mouse_probe(struct platform_device *pdev)
{
	int ret = 0;

//	struct input_dev *input;
	//printk("gp_mouse_probe()\n");
	
	p_gp_board_ps2_uart_mouse_t = gp_board_get_config("ps2_uart_mouse",gp_board_ps2_uart_mouse_t);
	if(p_gp_board_ps2_uart_mouse_t == NULL){
		//kfree(WorkArea);
		//DIAG_ERROR("[%s:%d] Error!\n", __FUNCTION__, __LINE__);
		//goto out_unregister;
		printk("gp_board_get_config of ps2_uart_mouse Error !!!\n");
	}
	MOUSE_CLK_GPIO = p_gp_board_ps2_uart_mouse_t->get_clk_gpio();
	MOUSE_DATA_GPIO = p_gp_board_ps2_uart_mouse_t->get_data_gpio();
	//printk("MOUSE_CLK_GPIO = 0x%x\n",MOUSE_CLK_GPIO);
	//printk("MOUSE_DATA_GPIO = 0x%x\n",MOUSE_DATA_GPIO);

	ret = gp_mouse_request_gpio();
	if(ret){
		return ret;
	}
	//gp_gpio_request_irq(0, "mouse", gp_mouse_gpioIrq, NULL);
	
	//callback_ID = gp_gpio_request_irq(0, "mouse", gp_mouse_gpioIrq, NULL);//del by erichan
	gp_gpio_register_isr(gp_mouse_data.pin_clk, (void (*)(void *))gp_mouse_gpioIrq, ( void * )(gp_mouse_data.pin_clk)) ; 
	input = input_allocate_device();
	//printk("input = input_allocate_device();\n");
	if (!input) {
		ret = -ENOMEM;
		printk("input alloc erro\n");
		goto erro_req;
	}
	   
	input->name = pdev->name;
	input->phys = "mouse/input0";
	input->dev.parent = &pdev->dev;
       
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) |BIT_MASK(BTN_RIGHT);
	input->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

	gp_mouse_data.input = input;

	input_set_drvdata(input, &gp_mouse_data);
	//printk("input_set_drvdata(input, &gp_mouse_data);\n");
    
	ret = input_register_device(input);
	//printk("ret = input_register_device(input);\n");
	if (ret){
		goto erro_input;
	}


	platform_set_drvdata(pdev, &gp_mouse_data);
	//printk("platform_set_drvdata(pdev, &gp_mouse_data);\n");
	gp_mouse_init();
	gpMouseRestartFlag = 0;//20110310
	gpMouseUartFlag = 0;//20110310
	/*enable the kb*/

	init_timer(&gp_mouse_data.timer);
	gp_mouse_data.timer.data = (unsigned long) &gp_mouse_data;
	gp_mouse_data.timer.function = gp_mouse_time;
	gp_mouse_data.timer.expires = jiffies + HZ/5;
	add_timer(&gp_mouse_data.timer);
	//printk("gp_mouse_probe() exit\n");
	return 0;

erro_input:
	input_free_device(input);

//erro_timer:
	//gp_gpio_release(gp_mouse_data.pin_clk);
	//gp_gpio_release(gp_mouse_data.pin_data);
erro_req:
	
	return ret;
}

static int gp_mouse_remove(struct platform_device *pdev)
{
	input_free_device(gp_mouse_data.input);

	gp_gpio_release(gp_mouse_data.pin_clk);
	gp_gpio_release(gp_mouse_data.pin_data);

	return 0;
}



static struct platform_device gp_mouse_device = {
	.name	= "gp-mouse",
	.id	= -1,
};

static struct platform_driver gp_mouse_driver = {
	.probe		= gp_mouse_probe,
	.remove		= __devexit_p(gp_mouse_remove),
	.driver		= {
		.name	= "gp-mouse",
		.owner	= THIS_MODULE,
	}
};


static int gp_mouse_drv_init(void)
{
	platform_device_register(&gp_mouse_device);
	return platform_driver_register(&gp_mouse_driver);
}

static void gp_mouse_drv_free(void)
{
	platform_device_unregister(&gp_mouse_device);
	platform_driver_unregister(&gp_mouse_driver);	
}

module_init(gp_mouse_drv_init);
module_exit(gp_mouse_drv_free);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP ps2 mouse driver");
MODULE_LICENSE_GP;
