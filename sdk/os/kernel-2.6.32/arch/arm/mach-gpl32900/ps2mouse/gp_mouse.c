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


typedef struct gp_mouse_s {
	int pin_clk;
	int pin_data;

	struct timer_list 		timer;
	struct input_dev *input;
} gp_mouse_t;

#define BUFNUMMAX		4
#define MOUSEMSGMAX	4
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


/* define the clk data mcroe */

#define MouseClk_Input_No			gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_clk, GPIO_PULL_FLOATING)	//Clk, input gpio mode 悬浮

#define MouseClk_Input_Up			gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_clk, GPIO_PULL_HIGH)	//Clk, input gpio mode 上拉

#define MouseClk_OutputLow		gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_clk, 0);	//Clk, output gpio mode

#define MouseClk_OutputHigh		gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_clk, 1);	//Clk, output gpio mode

#define MouseData_Input_No			gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_data, GPIO_PULL_FLOATING)		//Data, input gpio mode 悬浮

#define MouseData_Input_Up			gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_INPUT);\
	gp_gpio_set_pullfunction(gp_mouse_data.pin_data, GPIO_PULL_HIGH)		//Data, input gpio mode 上拉

#define MouseData_OutputLow		gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_data, 0);		//Data, output gpio mode

#define MouseData_OutputHigh		gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_OUTPUT);\
	gp_gpio_set_value(gp_mouse_data.pin_data, 1);		//Data, output gpio mode

#define MouseClk_SetLow_Int			gp_gpio_irq_property(gp_mouse_data.pin_clk, GPIO_IRQ_LEVEL_LOW, NULL)

#define MouseClk_Disable_Int			gp_gpio_enable_irq(gp_mouse_data.pin_clk, 0)

#define MouseClk_Enable_Int			gp_gpio_enable_irq(gp_mouse_data.pin_clk, 1)


static void gp_mouse_init(void)
{
	MouseClk_OutputLow;
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

}

//===================================================================
void SendDatatoDevice(int tdata)
{
	int temp;
	int t1,i,tt;
	MouseClk_Disable_Int;			//关中断
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

	MouseClk_SetLow_Int;		//高触发
	MouseClk_Enable_Int;	 	//开中断
}



static int gp_mouse_request_gpio(void)
{
	int ret = 0;

	gp_mouse_data.pin_clk = gp_gpio_request(MOUSE_CLK_GPIO, "mouse");
	if(IS_ERR_VALUE(gp_mouse_data.pin_clk)){
		DIAG_ERROR("mouse request gpio clk = %d\n", gp_mouse_data.pin_clk);
		return -EINVAL;
	}
	gp_gpio_set_function(gp_mouse_data.pin_clk, GPIO_FUNC_GPIO);
	//gp_gpio_set_direction(gp_mouse_data.pin_clk, GPIO_DIR_OUTPUT);
	//gp_gpio_set_value(gp_mouse_data.pin_clk, 1);

	gp_mouse_data.pin_data = gp_gpio_request(MOUSE_DATA_GPIO, "mouse");
	if(IS_ERR_VALUE(gp_mouse_data.pin_data)){
		DIAG_ERROR("mouse request gpio data = %d\n", gp_mouse_data.pin_data);
		goto error;
	}
	gp_gpio_set_function(gp_mouse_data.pin_data, GPIO_FUNC_GPIO);
	//gp_gpio_set_direction(gp_mouse_data.pin_data, GPIO_DIR_INPUT);
	//gp_gpio_set_pullfunction(gp_mouse_data.pin_data, GPIO_PULL_HIGH);

	gp_gpio_register_isr(gp_mouse_data.pin_clk, gp_mouse_gpioIrq, NULL);

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
	//MouseClk_Clear_Flag;		//清标志
	MouseClk_SetLow_Int;		//设置下降触发
	MouseClk_Enable_Int;		//开中断
}

void mouserest(void)
{
	gpMouseDataBufNum = 0;

	gpMouseBitCount = 0;
	gpMouseSystemFlag = 0;
	gpMouseErrDataFlag=0;
	gpMousejishu = 0;
	SendDatatoDevice(0xff);
	gpMouseInitDelay = 10;
}

//===================================================================
int TestMouseData(int data)//检测数据
{
	int temp ,t1,i,tt;
	temp = data ;
	if(data < 0x400 || data >= 0x800) return 1;
	if((data%2)==1)	return 1;
	t1=1;
	for(i=0;i < 11;i++)
	{
		tt = temp&(1<<i);
		if(tt)
		{	t1^=1;	}
	}
	if(!t1) return 1;
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
	MouseClk_Disable_Int;		//关中断

	//MouseClk_Clear_Flag;		//清标志
	MouseClk_SetLow_Int;		//设置下降触发

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
			MouseClk_SetRise_Int;	//发数据是设置成 上升触发响应
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
        //printk("datax = %x\n",datax);
	if(buffer[0]&0x20){
		datay = buffer[2]&0xff;
		datay += 0xffffff00;
	}
	else{
		datay = buffer[2]&0xff;
	}
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

void MouseReceiveDataInIrqOK(void)
{
	int Bit;int erro;

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
		erro = TestMouseData(gpMouseDataReg);
		if(erro)
		{
			MouseTimerErrPro();
	//		printf("mouse_test_erro %d\n",MouseDataReg);
		}
		else
		{
			if((gpMouseDataReg&0x1ff)==0x154)
			{
				gpMouseDataBufNum = 0;
				gpMouseSystemFlag = 0;
			}/**/
		}
		if( gpMouseDataBufNum>=4 )
		{
			gpMouseCheckFlag = 0 ;
			gpMouseDataBufNum = 0 ;
			erro = MouseTimerCheck((int *)gpMouseReceiveBuffer);
			if(erro)
			{
				MouseTimerErrPro();
				printk("mouse_other_erro\n");
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
		mouserest();
		gpMouseRestartFlag = 1;
	}
	if( gpMouseDataBufNum==0)
	{
		gpMousejishu ++;
		if(gpMousejishu >=1000)//0x400000
		{
			gpMouseRestartFlag = 0;
			gpMousejishu = 0 ;
			//mouseInitisrflag = 0 ;
		}
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
		//printk("mousedata = %x ,%x\n",tdata,gpMouseSystemFlag);
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
				//	SendDatatoDevice(0xf3);//采样
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
		//			SendDatatoDevice(0xf3);
		//			gpMouseSystemFlag = 0x2;
				}
				else if( gpMouseSystemFlag==0x02 )
				{
					SendDatatoDevice(0xc8);
					gpMouseSystemFlag = 0x04;
				}
				else if( gpMouseSystemFlag==0x04 )
				{
					SendDatatoDevice(0xf3);
					gpMouseSystemFlag = 0x08;
				}
				else if( gpMouseSystemFlag==0x08 )
				{
					SendDatatoDevice(0x64);
					gpMouseSystemFlag = 0x10;
				}
				else if( gpMouseSystemFlag==0x10 )
				{
					SendDatatoDevice(0xf3);
					gpMouseSystemFlag = 0x20;
				}
				else if( gpMouseSystemFlag==0x20 )
				{
					SendDatatoDevice(0x50);
					gpMouseSystemFlag = 0x40;
				}
				else if( gpMouseSystemFlag==0x40 )
				{
					SendDatatoDevice(0xf2);
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
				else if (gpMouseSystemFlag == 0x400)//设置采样率
				{
					SendDatatoDevice(0xf3);
					gpMouseSystemFlag = 0x800;
				}
				else if (gpMouseSystemFlag == 0x800)
				{
					SendDatatoDevice(40);//设置采样率的值 改这个就可以
					gpMouseSystemFlag = 0x1000;
				}
				else if (gpMouseSystemFlag == 0x1000)
				{
					SendDatatoDevice(0xf4);
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
					//printk("zfq_InitGood_data\n");
				}
				else
				{
					mouserest();
				}
				break;
			default:
				mouserest();
				break;
		}
	}
}


static void gp_mouse_time(unsigned long arg)
{
	gp_mouse_data.timer.expires = jiffies + HZ/10;

	add_timer(&gp_mouse_data.timer);

	if(gpMouseSystemFlag&MOUSEStepFlag)
	{
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
}

static void gp_mouse_gpioIrq(void *data)
{
	//if(!(value&0x1000)){
	//	return 0;
	//}

	if(gpMouseIrqFlag&ReceiveFlag)
	{
		if(gpMouseIrqFlag&SendFlag)
		{
			//mouseInitisrflag = MOUSERESETNEM;
			MouseSendDataInIrq();
		}
		else
		{
			if(gpMouseSystemFlag == MOUSEStepFlag)
			{
				MouseReceiveDataInIrqOK();
			}
			else
			{
				MouseReceiveDataInIrq();
			}
		}
	}
}

static int gp_mouse_probe(struct platform_device *pdev)
{
	int ret = 0;

	struct input_dev *input;

	ret = gp_mouse_request_gpio();
	if(ret){
		return ret;
	}
	//gp_gpio_request_irq(0, "mouse", gp_mouse_gpioIrq, NULL);

	input = input_allocate_device();
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

	ret = input_register_device(input);
	if (ret){
		goto erro_input;
	}


	platform_set_drvdata(pdev, &gp_mouse_data);

	gp_mouse_init();

	/*enable the kb*/

	init_timer(&gp_mouse_data.timer);
	gp_mouse_data.timer.data = (unsigned long) &gp_mouse_data;
	gp_mouse_data.timer.function = gp_mouse_time;
	gp_mouse_data.timer.expires = jiffies + HZ/10;
	add_timer(&gp_mouse_data.timer);

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

