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
 * @file gp_timer.c
 * @brief timer/count driver interface 
 * @author zaimingmeng
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>

#include <mach/hal/hal_timer.h>
#include <mach/gp_timer.h>

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_tc_s {
	struct miscdevice dev;     	 /*!< @brief tc device */
	char *name;
	int id;				/*!< @brief device id*/
	unsigned int irq;
	void (*irq_handler)(int,void *);
	spinlock_t lock;
	int isOpened;
	struct list_head list;
} gp_tc_t;


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static LIST_HEAD(gp_timer_list);
static int g_tc_irq[5] = {IRQ_TIMERINT0,IRQ_TIMERINT1,IRQ_TIMERINTX,IRQ_TIMERINTX,IRQ_TIMERINTX};

/**
 * @brief timer/counter enable function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_enable(int handle)
{	
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerEn(timer->id, 1);
	return 0;
}
EXPORT_SYMBOL(gp_tc_enable);

/**
 * @brief timer/counter disable function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_disable(int handle)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerEn(timer->id, 0);
	return 0;
}
EXPORT_SYMBOL(gp_tc_disable);

/**
 * @brief timer/counter enable/disable interrupter
 * @param handle [in] timer/counter handle
 * @param enable [in] 0:disable interrupter; 1:enable interrupter
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_enable_int(int handle, int enable)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerIntEn(timer->id, enable);
	return 0;
}
EXPORT_SYMBOL(gp_tc_enable_int);

/**
 * @brief timer/counter enable/disable output
 * @param handle [in] timer/counter handle
 * @param enable [in] 0:disable timer output; 1:enable timer output
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_enable_output(int handle, int enable)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerOeEnSet(timer->id, enable);
	gpHalTimerOeSet(timer->id, 1);
	return 0;
}
EXPORT_SYMBOL(gp_tc_enable_output);

/**
 * @brief timer/counter count up or down selection
 * @param handle [in] timer/counter handle
 * @param dir [in] dir value: 0:down counting; 1:up counting;
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_count_mode(int handle, int dir)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerUdSet(timer->id, dir);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_count_mode);

/**
 * @brief up/down counting control selection set
 * @param handle [in] timer/counter handle
 * @param mode [in] dir value: 0:up/down control by bit4 TxCTR;
 *                 1:up/down control by EXTUDx input;
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_count_selection(int handle, int mode)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerUdsSet(timer->id, mode);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_count_selection);

/**
 * @brief timer/counter output mode set,only effect when timer output is in normal mode.
 * @param handle [in] timer/counter handle
 * @param mode [in] 0:toggle mode; 1:pulse mode
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_pulse_mode(int handle, int mode)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerOmSet(timer->id, mode);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_pulse_mode);

/**
 * @brief external input active edge set
 * @param handle [in] timer/counter handle
 * @param edge [in] dir value: 0:positive edge; 1:negative edge;
 *                 2:both edge;
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_active_edge(int handle, int edge)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerEsSet(timer->id, edge);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_active_edge);

/**
 * @brief timer operating mode set
 * @param handle [in] timer/counter handle
 * @param mode [in]  mode value: 0:free running time mode; 1:period timer mode; 
 *		2:free running counter mode; 3:period counter mode
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_operation_mode(int handle, int mode)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	if(mode > 3)
		return -1;
	gpHalTimerMSet(timer->id, mode);
	return 0;
	
}
EXPORT_SYMBOL(gp_tc_set_operation_mode);

/**
 * @brief load value set
 * @param handle [in] timer/counter handle
 * @param value [in] timer/counter load value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_load(int handle, int value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerLoadSet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_load);

/**
 * @brief load value get
 * @param handle [in] timer/counter handle
 * @param value [out] timer/counter load value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_load(int handle, int *value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerLoadGet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_get_load);

/**
 * @brief timer compare value set(used only in pwm mode)
 * @param handle [in] timer/counter handle
 * @param value [in] timer/counter compare value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_compare(int handle, int value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerCmpSet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_compare);

/**
 * @brief timer compare value get;used only in pwm mode
 * @param handle [in] timer/counter handle
 * @param value [out] timer/counter compare value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_compare(int handle, int *value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerCmpGet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_get_compare);

/**
 * @brief timer/counter clock prescale set
 * @param handle [in] timer/counter handle
 * @param value [in] timer/counter prescale value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_prescale(int handle, int value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerPrescaleSet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_prescale);

/**
 * @brief timer/counter clock prescale get
 * @param handle [in] timer/counter handle
 * @param value [out] timer/counter prescale value
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_prescale(int handle, int *value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerPrescaleGet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_get_prescale);

/**
 * @brief use to clear interrupt flags
 * @param handle [in] timer/counter handle
 * @param value [in] 0:clear interrupter flag
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_set_int_state(int handle, int value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerInterruptSet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_set_int_state);

/**
 * @brief get interrupt flags
 * @param handle [in] timer/counter handle
 * @param value [out]  interrupter flag
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_get_int_state(int handle, int *value)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;

	gpHalTimerInterruptGet(timer->id, value);
	return 0;
}
EXPORT_SYMBOL(gp_tc_get_int_state);

/**
 * @brief the counter register of timer/counter init function;init load/prescale/cmp register
 * @param freq[in]:timer value of the counter reload(hz)
 * @param duty[in]:int timer mode is 0;in PWM mode is(0~100)
 */
void gp_tc_set_freq_duty(int handle, UINT32 freq, UINT32 duty)
{
	UINT32 apbHz;
	UINT32 temp;

	apbHz = gpHalTimerGetBaseClk();

	if(freq <= 500){
		gp_tc_set_prescale(handle, 1000-1);
		freq = freq*1000;
	}
	else{
		gp_tc_set_prescale(handle, 0);
	}
	temp = apbHz/freq-1;

	gp_tc_set_load(handle,temp);
	

	if((0 < duty)&&(duty < 100)){
		temp = (100 - duty)*temp/100 - 1;
	}
	else if(0 == duty){
		temp--;
	}
	else{
		temp = 1;
	}

	gp_tc_set_compare(handle, temp);

	gp_tc_set_operation_mode(handle, 1);		//period mode
}
EXPORT_SYMBOL(gp_tc_set_freq_duty);

/**
 * @brief request the timer/counter resource function
 * @param timer_id [in] timer/counter device id
 * @param name [in] caller name
 * @return success: 0,  erro: NULL
 */
int gp_tc_request(int timer_id, char *name)
{
	int found = 0;
	struct gp_tc_s *timer = NULL;

	list_for_each_entry(timer,&gp_timer_list,list){
		if((timer->id==timer_id)&&(timer->name == NULL)){
			found = 1;
			break;
		}
	}
	
	if(found){
		if(spin_trylock(&timer->lock)&&(timer->isOpened == 0)){
			DIAG_VERB("request tc %s ok,id=%d\n",name,timer->id);
			timer->name = name;
			timer->isOpened = 1;
			spin_unlock(&timer->lock);
		}
		else{
			timer = NULL;
			DIAG_ERROR("request tc %s error\n",name);
		}
	}
	else{
		timer = NULL;
		DIAG_ERROR("request tc %s error\n",name);
	}
	return (int)timer;
}
EXPORT_SYMBOL(gp_tc_request);

/**
 * @brief release the timer/counter resource function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_release(int handle)
{
	struct gp_tc_s *timer = NULL;

	unsigned long flags;

	if(handle == 0)
		return -EINVAL;

	timer = (struct gp_tc_s *)handle;
	
	spin_lock(&timer->lock);
	local_irq_save(flags);

	gpHalTimerInit(timer->id);		//clear the regsiter
	timer->name = NULL;
	timer->isOpened = 0;

	if(timer->irq_handler){
		free_irq(timer->irq, timer);
		timer->irq_handler = NULL;	
		DIAG_VERB("free tc irq=%d\n",timer->irq);
	}

	local_irq_restore(flags);
	spin_unlock(&timer->lock);
	return 0;
}
EXPORT_SYMBOL(gp_tc_release);

/**
 * @brief the timer/counter irq callback function handle
 * @param irq [in] irq index
 * @param dev_id [in] timer handle
 * @return IRQ_HANDLED
 */
static irqreturn_t gp_tc_irq_handler(int irq, void *dev_id)
{
	int temp=0;
	struct gp_tc_s *timer = (struct gp_tc_s *)dev_id;

	gp_tc_get_int_state((int)timer, &temp);
	if(temp){
		gp_tc_set_int_state((int)timer,0);

		if(timer->irq_handler)
			timer->irq_handler(timer->id, timer);
	}

	return IRQ_HANDLED;
}

/**
 * @brief the timer/counter interrupt register function
 * @param handle [in] timer/counter handle
 * @param irq_handler [in] interrupt function handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_irq_register(int handle, void(*irq_handler)(int,void *))
{
	struct gp_tc_s *timer;
	unsigned long flags;
	int ret = 0;

	if(handle == 0)
		return -EINVAL;
	timer = (struct gp_tc_s *)handle;

	local_irq_save(flags);
		
	ret = request_irq(timer->irq, gp_tc_irq_handler, IRQF_SHARED, timer->name, timer);

	if(!ret){		
		timer->irq_handler = irq_handler;
		DIAG_VERB("register irq=%d ok\n",timer->irq);
		gp_tc_enable_int(handle, 1);	//enable interrupt
	}
	else{
		DIAG_ERROR("register irq=%d error\n",timer->irq);
	}

	local_irq_restore(flags);

	return ret;
}
EXPORT_SYMBOL(gp_tc_irq_register);

/**
 * @brief the timer/counter device init function
 * @param handle [in] timer/counter handle
 * @return success: 0,  erro: ERROR_ID
 */
int gp_tc_init(int handle)
{
	struct gp_tc_s *timer = NULL;

	if(0 == handle){
		return -EINVAL;
	}
	timer = (struct gp_tc_s *)handle;
	
	gpHalTimerInit(timer->id);
	timer->name = NULL;
	timer->irq_handler = NULL;

	return 0; 
}
EXPORT_SYMBOL(gp_tc_init);

/**
 * @brief   tc driver open
 */
static int gp_tc_dev_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	int handle = 1;
	int found = 0;
	int minor = iminor(inode);
	struct gp_tc_s *timer = NULL;

	list_for_each_entry(timer,&gp_timer_list,list){
		if(minor == timer->dev.minor){
			found = 1;
			break;
		}
	}
	if(0 == found){
		ret = -EBUSY;
		goto out;
	}

	handle = gp_tc_request(timer->id, (char *)timer->dev.name);
	if(handle){
		file->private_data = timer;
	}
	else{
		ret = -EBUSY;
		goto out;
	}
out:	
	return ret;
}

/**
 * @brief   tc driver release
 */
static int gp_tc_dev_release(struct inode *inode, struct file *file)
{
	struct gp_tc_s *timer = (struct gp_tc_s *)file->private_data;

	file->private_data = NULL;
	gp_tc_release((int)timer);
	return 0;
}

/**
 * @brief   tc driver ioctl
 */
static long gp_tc_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int temp;
	struct gp_tc_s *timer = (struct gp_tc_s *)file->private_data;

	if(timer == NULL){
		return -ENOTTY;
	}
	
	spin_lock(&timer->lock);
	switch(cmd){
	case TC_IOCTL_ENABLE:
		gp_tc_enable((int)timer);		
		break;

	case TC_IOCTL_DISABLE:
		gp_tc_disable((int)timer);
		break;	

	case TC_IOCTL_ENABLE_OUTPUT:
		gp_tc_enable_output((int)timer, arg);
		break;				

	case TC_IOCTL_SET_COUNT_MODE:
		gp_tc_set_count_mode((int)timer, arg);
		break;		

	case TC_IOCTL_SET_COUNT_SELECTION:
		gp_tc_set_count_selection((int)timer, arg);
		break;		

	case TC_IOCTL_SET_PULSE_MODE:
		gp_tc_set_pulse_mode((int)timer, arg);	
		break;

	case TC_IOCTL_SET_ACTIVE_EDGE:	
		gp_tc_set_active_edge((int)timer, arg);
		break;

	case TC_IOCTL_SET_OPERATION_MODE:	
		gp_tc_set_operation_mode((int)timer, arg);
		break;

	case TC_IOCTL_SET_RELOAD:
		gp_tc_set_load((int)timer, arg);		
		break;

	case TC_IOCTL_GET_CURRENT_VALUE:
		gp_tc_get_load((int)timer, &temp);
		if(copy_to_user((void __user *)arg, &temp, sizeof(int))){
			ret = -EFAULT;
		}
		break;

	case TC_IOCTL_SET_PRESCALE:
		gp_tc_set_prescale((int)timer, arg);		
		break;

	case TC_IOCTL_GET_PRESCALE:
		gp_tc_get_prescale((int)timer, &temp);
		if(copy_to_user((void __user *)arg, &temp, sizeof(int))){
			ret = -EFAULT;
		}
		break;

	case TC_IOCTL_SET_COMPARE:
		gp_tc_set_compare((int)timer, arg);		
		break;

	case TC_IOCTL_GET_COMPARE:
		gp_tc_get_compare((int)timer, &temp);
		if(copy_to_user((void __user *)arg, &temp, sizeof(int))){
			ret = -EFAULT;
		}
		break;			
	}
	spin_unlock(&timer->lock);

	return ret;
}

struct file_operations gp_tc_fops = {
	.owner          = THIS_MODULE,
	.open           = gp_tc_dev_open,
	.release        = gp_tc_dev_release,
	.unlocked_ioctl = gp_tc_dev_ioctl,
};

/**
 * @brief   tc driver probe
 */
static int gp_tc_probe(struct platform_device *pdev)
{

	int ret = -1;
	char *name = NULL;
	struct gp_tc_s *timer = NULL;

	timer = kzalloc(sizeof(gp_tc_t), GFP_KERNEL);
	if(!timer)
		return -ENOMEM;

	INIT_LIST_HEAD(&timer->list);

	name = kzalloc(7, GFP_KERNEL);
	if(!name)
		goto err_alloc;
	sprintf(name, "timer%d", pdev->id);


	timer->dev.name  = name;
	timer->dev.fops  = &gp_tc_fops;
	timer->dev.minor  = MISC_DYNAMIC_MINOR;
	timer->id = pdev->id;
	timer->irq = g_tc_irq[pdev->id];

	ret = misc_register(&timer->dev);
	if(ret != 0){
		DIAG_ERROR("tc probe register fail\n");
		goto err_alloc;
	}

	list_add(&timer->list, &gp_timer_list);
	spin_lock_init(&timer->lock);
	gp_tc_init((int)timer);
	
	platform_set_drvdata(pdev, timer);
	
	DIAG_INFO("tc probe ok\n");
	
	return 0;

err_alloc:
	DIAG_ERROR("tc probe erro\n");
	if(name != NULL){
		kfree(name);
	}
	if(timer != NULL){
		kfree(timer);
	}
		
	return -1;
}

/**
 * @brief   tc driver remove
 */
static int gp_tc_remove(struct platform_device *pdev)
{
	int found = 0;
	struct gp_tc_s *timer = NULL;

	list_for_each_entry(timer,&gp_timer_list,list){
		if(timer->id == pdev->id){
			found = 1;
			break;
		}
	}
	if(1 == found){
		misc_deregister(&timer->dev);
		
		if(timer->dev.name)
			kfree(timer->dev.name);
		list_del(&timer->list);
		kfree(timer);
	}
	return 0;
}

/**
 * @brief   tc driver define
 */
static struct platform_driver gp_tc_driver = {
	.probe  = gp_tc_probe,
	.remove = gp_tc_remove,
	.driver = {
		.name  = "gp-timer",
		.owner = THIS_MODULE,
	}
};

/**
 * @brief   wdt device release
 */
static void gp_timer_device_release(struct device *dev)
{
}

/**
 * @brief   timer device infomation define
 */
static struct platform_device gp_tc_device0 = {
	.name	= "gp-timer",
	.id	= 0,
	.dev	= {
		.release = gp_timer_device_release,
	},
};

static struct platform_device gp_tc_device1 = {
	.name	= "gp-timer",
	.id	= 1,
	.dev	= {
		.release = gp_timer_device_release,
	},
};

static struct platform_device gp_tc_device2 = {
	.name	= "gp-timer",
	.id	= 2,
	.dev	= {
		.release = gp_timer_device_release,
	},
};

static struct platform_device gp_tc_device3 = {
	.name	= "gp-timer",
	.id	= 3,
	.dev	= {
		.release = gp_timer_device_release,
	},
};

static struct platform_device gp_tc_device4 = {
	.name	= "gp-timer",
	.id	= 4,
	.dev	= {
		.release = gp_timer_device_release,
	},
};

static struct platform_device *gp_tc_devices[] = {
	&gp_tc_device0,
	&gp_tc_device1,
	&gp_tc_device2,
	&gp_tc_device3,
	&gp_tc_device4,
};

/**
 * @brief   tc driver init
 */
static int __init gp_tc_drv_init(void)
{	
	platform_add_devices(gp_tc_devices, ARRAY_SIZE(gp_tc_devices));
	return platform_driver_register(&gp_tc_driver);
}

static void gp_platform_remove_devices(struct platform_device **dev, int num)
{
	int i;
	
	for(i = 0; i < num; i++){
		platform_device_unregister(dev[i]);
	}
}

/**
 * @brief   tc driver exit
 */
static void __exit gp_tc_drv_exit(void)
{
	gp_platform_remove_devices(gp_tc_devices, ARRAY_SIZE(gp_tc_devices));
	platform_driver_unregister(&gp_tc_driver);
}

module_init(gp_tc_drv_init);
module_exit(gp_tc_drv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP timer/counter Driver");
MODULE_LICENSE_GP;
