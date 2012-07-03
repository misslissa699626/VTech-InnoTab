typedef enum {
	GPIO_ENABLE=0,
	GPIO_DATA,
	GPIO_DIR,
	GPIO_POLARITY,
	GPIO_STICKY,
	GPIO_PULLUP_EN,
	GPIO_PULLDOWN_EN,
	GPIO_INT_EN,
	GPIO_INT_PEND,
	GPIO_DEBOUNCE_EN,
	GPIO_WKUEN,
	GPIO_STATUS,	
} gpio_func_type_t;

int halGpioInit(void);
int halGpioSetPGS(unsigned int gid,unsigned int writevalue);
int halGpioSetPGC(unsigned int gid,unsigned int writevalue, int ext);
int halGpioSetFuncData(gpio_func_type_t functype,unsigned int gpio_channel,unsigned int gpiopin,unsigned int writebit);
int halGpioGetPGS(unsigned int gid,unsigned int *readvalue);
int halGpioGetPGC(unsigned int gid,unsigned int *readvalue, int ext);
int halGpioGetFuncData(gpio_func_type_t functype,unsigned int gpio_channel,unsigned int gpiopin,unsigned int *readvalue);
// Special for GPIO 2 & 3
int halGpio23SetDir(unsigned int gpio_channel,unsigned int gpiopin,unsigned int writebit);
int halGpio23SetOutData(unsigned int gpio_channel,unsigned int gpiopin,unsigned int writebit);
int halGpio23GetInData(unsigned int gpio_channel,unsigned int gpiopin,unsigned int *readvalue);


// ext function
void spmp_free_gpio_irq (int gpio_id,int gpio_ch);
int spmp_request_gpio_irq (int gpio_id, char *name,  void (*irq_handler)(int, void *), void *data);
int spmp_get_gpio_irq (int gpio_id);