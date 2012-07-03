#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/regs-i2c.h>
#include <mach/regs-scu.h>
/* i2c controller state */

//#define I2C_POLLING_MODE
enum spmp_i2c_state {
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP
};

struct spmp_platform_i2c {
	int		bus_num;	/* bus number to use */
	unsigned int	 flags;
	unsigned int slave_addr;	/* slave address for controller */
	unsigned long	bus_freq;	/* standard bus frequency */
	unsigned long	max_freq;	/* max frequency for the bus */
	unsigned long	min_freq;	/* min frequency for the bus */
	unsigned int  sda_delay;	
	void	(*cfg_gpio)(struct platform_device *dev);
};

struct spmp_i2c {
	spinlock_t		lock;
	wait_queue_head_t	wait;
	unsigned int		suspended:1;

	struct i2c_msg		*msg;
	unsigned int		msg_num;
	unsigned int		msg_idx;
	unsigned int		msg_ptr;
	
	enum spmp_i2c_state	state;
	unsigned long		clkrate;

	unsigned int		irq;
	void __iomem		*regs;
	struct clk		*clk;
	struct device		*dev;
	struct resource		*ioarea;
	struct i2c_adapter	adap;
};


#ifdef I2C_POLLING_MODE
/* spmp_i2cB_master_complete
 *
 * complete the message and wake up the caller, using the given return code,
 * or zero to mean ok.
*/

static inline void spmp_i2cB_master_complete(struct spmp_i2c *i2c, int ret)
{
	dev_dbg(i2c->dev, "master_complete %d\n", ret);

	i2c->msg_ptr = 0;
	i2c->msg = NULL;
	i2c->msg_idx++;
	i2c->msg_num = 0;
	if (ret)
		i2c->msg_idx = ret;
}

int I2C_P0_P034_mstart(struct spmp_i2c *i2c, unsigned int  aAck, unsigned int  aCmd, unsigned int  aData)
{

	unsigned int  ctrl = 0;
	unsigned int  iccr = 0;	
	unsigned int  delay_count = 25000;
	int   ret = 0;
	
	iccr =readl(i2c->regs + I2C_C_ICCR_OFST);
	
	iccr &= 0x2F;
	switch(aCmd){
		case 1: // write
			iccr |= 0x00;
			ctrl = 0xD0;
			break;

		case 2: // Read
			iccr |= 0x80;
			ctrl = 0x90;
			break;

		default:
			iccr |= 0x00;
			ctrl = 0xD0;
			break;
	}
	
	writel(iccr, i2c->regs + I2C_C_ICCR_OFST);
	writel(ctrl, i2c->regs + I2C_C_ICSR_OFST);	

	if(2 == aCmd){ //READ CMD
		writel((aData|0x01), i2c->regs + I2C_C_IDSR_OFST);			
	}
	else{
		writel(aData, i2c->regs + I2C_C_IDSR_OFST);			
	}
	
	writel(ctrl | 0x20, i2c->regs + I2C_C_ICSR_OFST);	

	do{
		iccr =readl(i2c->regs + I2C_C_ICCR_OFST);
		ctrl =readl(i2c->regs + I2C_C_ICSR_OFST);		
		udelay(20);

		if(--delay_count <= 0)
		{
			ret = -1;
			break;
		}
	}
	while(  !(iccr & 0x10) | // Interrupt Pending
	(ctrl & 0x01 & aAck) // ACK Received
	);
	return ret;
}

//----------------------------------------------------------------------------
int I2C_P0_P034_mmid(struct spmp_i2c *i2c,unsigned int  aAck, unsigned int  aCmd, unsigned int  aData)
{
	unsigned int   iccr, ctrl, data0;
	unsigned int   delay_count = 25000;
	int ret = 0;

	iccr =readl( i2c->regs + I2C_C_ICCR_OFST);
	iccr &= 0x2F; 

	switch(aCmd){
		case 1: // write
		writel(aData, i2c->regs + I2C_C_IDSR_OFST);		
			iccr = 0x10|iccr;
			break;

		case 2: // Read
			if(aAck == 1){    
				iccr |= 0x90;
			}
			else{      
				iccr |= 0x10;
			}
			break;

		default:
			iccr |= 0x00;
			break;
		//=======================================================
	}
	
	writel(iccr, i2c->regs + I2C_C_ICCR_OFST);

	do{
		iccr =readl( i2c->regs + I2C_C_ICCR_OFST);
		ctrl =readl( i2c->regs + I2C_C_ICSR_OFST);		
       udelay(20);

		if(--delay_count <= 0)
		{
			ret = -1;
			return ret;
		}
	}
	while(  !(iccr & 0x10) | // Interrupt Pending
	(ctrl & 0x01 & aAck) // ACK Received
	);
    data0 =readl( i2c->regs + I2C_C_IDSR_OFST);		
	return data0;
}


//----------------------------------------------------------------------------
void I2C_P0_P034_stop(struct spmp_i2c *i2c,unsigned int  aAck, unsigned int  aCmd, unsigned int  aData)
{
    unsigned int   ctrl;
    
    switch(aCmd){
        case 1: // write
            ctrl = 0xD0;
			 writel(ctrl, i2c->regs + I2C_C_ICSR_OFST);	
            do{
				ctrl =readl(i2c->regs + I2C_C_ICSR_OFST);		
            }
            while(ctrl & 0x20); // wait non-busy
        
            break;
        case 2:
            ctrl = 0x90;
			  writel(ctrl, i2c->regs + I2C_C_ICSR_OFST);	
            do{
		        ctrl =readl(i2c->regs + I2C_C_ICSR_OFST);		
            }
            while(ctrl & 0x20); // wait non-busy
            break;
        
        default:
            ctrl = 0xD0;
            break;
    }

}
//----------------------------------------------------------------------------
int I2C_P0_P034_WR(struct spmp_i2c *i2c,unsigned int  aId)
{
	int ret = 0;
    int i;
	ret = I2C_P0_P034_mstart(i2c,1, 1, aId);
	if (ret == 0){
		I2C_P0_P034_mmid(i2c,1, 1, (unsigned int) i2c->msg->buf[0]);
		for(i = 1 ; i< i2c->msg->len; i++)			
		{
    		I2C_P0_P034_mmid(i2c,1, 1, (unsigned int) i2c->msg->buf[i]);
		}		
	}
	I2C_P0_P034_stop(i2c,0, 1, 0x00);	
	return ret;
}
//----------------------------------------------------------------------------
int I2C_P0_P034_RD(struct spmp_i2c *i2c,unsigned int  aId)
{
	int ret = 0;
    int i;
	ret = I2C_P0_P034_mstart(i2c,1, 2, aId);  // Read Device ID
	if (ret == 0){
		for(i = 0 ; i< i2c->msg->len ; i++){
			ret = I2C_P0_P034_mmid  (i2c,0, 2, 0x00);    // Read Data
			if (ret != (-1)){
				i2c->msg->buf[i] = (unsigned char) ret;
			}else{
	    		break;
			}
		}
	}
	I2C_P0_P034_stop(i2c,0, 2, 0x00);
	return ret;
}

/* no support sequence write msg & sequence read msg */
static void spmp_i2cB_message_start(struct spmp_i2c *i2c,
				      struct i2c_msg *msg)
{
	unsigned int addr;
   unsigned int i,j;
	 int ret = -1;
	 for (i = 0; i < i2c->msg_num; i++)
	 {	    
		addr = (i2c->msg->addr & 0xFF);
		if (i2c->msg->flags & I2C_M_RD) {
			ret = I2C_P0_P034_RD(i2c, addr);						
		}
		else{		
			ret = I2C_P0_P034_WR(i2c, addr);			
		}				
		if(ret) break;
		 i2c->msg++;
 	    i2c->msg_idx++;		 
	 }
   if(!ret)
		spmp_i2cB_master_complete(i2c, 0);	 
	else
		spmp_i2cB_master_complete(i2c, EINVAL);	 				
}

/* spmp_i2cB_irq
 *
 * top level IRQ servicing routine
*/

static irqreturn_t spmp_i2cB_irq(int irqno, void *dev_id)
{
	return IRQ_HANDLED;
}


/* spmp_i2cB_set_master
 *
 * get the i2c bus for a master transaction
*/



/* spmp_i2cB_doxfer
 *
 * this starts an i2c transfer
*/

static int spmp_i2cB_doxfer(struct spmp_i2c *i2c,
			      struct i2c_msg *msgs, int num)
{
	unsigned long timeout;
	int ret;

	if (i2c->suspended)
		return -EIO;
	spin_lock_irq(&i2c->lock);
	i2c->msg     = msgs;
	i2c->msg_num = num;
	i2c->msg_ptr = 0;
	i2c->msg_idx = 0;
	i2c->state   = STATE_START;
	spmp_i2cB_message_start(i2c, msgs);
	spin_unlock_irq(&i2c->lock);

	ret = i2c->msg_idx;

 out:
	return ret;
}

/* spmp_i2cB_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
*/

static int spmp_i2cB_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	struct spmp_i2c *i2c = (struct spmp_i2c *)adap->algo_data;
	int retry;
	int ret;

	for (retry = 0; retry < adap->retries; retry++) {

		ret = spmp_i2cB_doxfer(i2c, msgs, num);

		if (ret != -EAGAIN)
			return ret;

		dev_dbg(i2c->dev, "Retrying transmission (%d)\n", retry);

		udelay(100);
	}

	return -EREMOTEIO;
}

/* declare our i2c functionality */
static u32 spmp_i2cB_func(struct i2c_adapter *adap)
{

	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

/* i2c bus registration info */

static const struct i2c_algorithm spmp_i2cB_algorithm = {
	.master_xfer		= spmp_i2cB_xfer,
	.functionality		= spmp_i2cB_func,
};

#else
/* spmp_i2cB_master_complete
 *
 * complete the message and wake up the caller, using the given return code,
 * or zero to mean ok.
*/

static inline void spmp_i2cB_master_complete(struct spmp_i2c *i2c, int ret)
{
	dev_dbg(i2c->dev, "master_complete %d\n", ret);

	i2c->msg_ptr = 0;
	i2c->msg = NULL;
	i2c->msg_idx++;
	i2c->msg_num = 0;
	if (ret)
		i2c->msg_idx = ret;

	wake_up(&i2c->wait);
}

static inline void spmp_i2cB_disable_ack(struct spmp_i2c *i2c)
{
	unsigned long tmp;

	tmp = readl(i2c->regs + I2C_C_ICCR_OFST);
	writel(tmp & ~ICCR_ACKEN, i2c->regs + I2C_C_ICCR_OFST);
}

static inline void spmp_i2cB_enable_ack(struct spmp_i2c *i2c)
{
	unsigned long tmp;

	tmp = readl(i2c->regs + I2C_C_ICCR_OFST);
	writel(tmp | ICCR_ACKEN, i2c->regs + I2C_C_ICCR_OFST);
}

/* irq enable/disable functions */

static inline void spmp_i2cB_disable_irq(struct spmp_i2c *i2c)
{
	unsigned long tmp;

	tmp = readl(i2c->regs + I2C_C_ICCR_OFST);
	writel(tmp & ~ICCR_INTREN, i2c->regs + I2C_C_ICCR_OFST);
}

static inline void spmp_i2cB_enable_irq(struct spmp_i2c *i2c)
{
	unsigned long tmp;

	tmp = readl(i2c->regs + I2C_C_ICCR_OFST);
	writel(tmp | ICCR_INTREN, i2c->regs + I2C_C_ICCR_OFST);
}


/* spmp_i2cB_message_start
 *
 * put the start of a message onto the bus
*/

static void spmp_i2cB_message_start(struct spmp_i2c *i2c,
				      struct i2c_msg *msg)
{
	unsigned int addr = (msg->addr & 0xFF);
	unsigned long stat;
	unsigned long iiccon;

	stat = 0;
	stat |=  ICSR_TXRX_ENABLE;

	if (msg->flags & I2C_M_RD) {
		stat |= ICSR_MASTER_RX;
		addr |= 1;	
	} else
		stat |= ICSR_MASTER_TX;

	if (msg->flags & I2C_M_REV_DIR_ADDR)
		addr ^= 1;

	/* todo - check for wether ack wanted or not */
//	if (~(msg->flags & I2C_M_NO_RD_ACK))
	   	spmp_i2cB_enable_ack(i2c);	
	iiccon = readl(i2c->regs + I2C_C_ICCR_OFST);
	writel(stat, i2c->regs + I2C_C_ICSR_OFST);

	dev_dbg(i2c->dev, "START: %08lx to IICSTAT, %02x to DS\n", stat, addr);
	writeb(addr, i2c->regs + I2C_C_IDSR_OFST);

	/* delay here to ensure the data byte has gotten onto the bus
	 * before the transaction is started */

	dev_dbg(i2c->dev, "iiccon, %08lx\n", iiccon);
//	writel(iiccon, i2c->regs + I2C_C_ICCR_OFST);

	stat |= ICSR_START;
	writel(stat, i2c->regs + I2C_C_ICSR_OFST);
}

static inline void spmp_i2cB_stoptransfer(struct spmp_i2c *i2c)
{  
    unsigned int ctrl;
	ctrl =readl(i2c->regs + I2C_C_ICSR_OFST);		
	writel(ctrl & ~(ICSR_START), i2c->regs + I2C_C_ICSR_OFST);	
}

static void spmp_i2cB_stop(struct spmp_i2c *i2c, int ret)
{	
	spmp_i2cB_stoptransfer(i2c);
	spmp_i2cB_master_complete(i2c, ret);
	spmp_i2cB_disable_irq(i2c);		
}

/* helper functions to determine the current state in the set of
 * messages we are sending */

/* is_lastmsg()
 *
 * returns TRUE if the current message is the last in the set
*/

static inline int is_lastmsg(struct spmp_i2c *i2c)
{
	return i2c->msg_idx >= (i2c->msg_num - 1);
}

/* is_msglast
 *
 * returns TRUE if we this is the last byte in the current message
*/

static inline int is_msglast(struct spmp_i2c *i2c)
{
	return i2c->msg_ptr == i2c->msg->len-1;
}

/* is_msgend
 *
 * returns TRUE if we reached the end of the current message
*/

static inline int is_msgend(struct spmp_i2c *i2c)
{
	return i2c->msg_ptr >= i2c->msg->len;
}

/* i2s_s3c_irq_nextbyte
 *
 * process an interrupt and work out what to do
 */

static int i2s_s3c_irq_nextbyte(struct spmp_i2c *i2c, unsigned long iicstat)
{
	unsigned long tmp;
	unsigned char byte;
	int ret = 0;
//   printk("i2c state = %d \n",i2c->state);
	switch (i2c->state) {

	case STATE_IDLE:
		dev_err(i2c->dev, "%s: called in STATE_IDLE\n", __func__);
		goto out;
		break;

	case STATE_STOP:
		dev_err(i2c->dev, "%s: called in STATE_STOP\n", __func__);
		spmp_i2cB_disable_irq(i2c);
		goto out_ack;
	
	case STATE_START:
		/* last thing we did was send a start condition on the
		 * bus, or started a new i2c message
		 */

		if (iicstat & ICSR_LASTBIT &&
		    !(i2c->msg->flags & I2C_M_IGNORE_NAK)) {
			/* ack was not received... */

			dev_dbg(i2c->dev, "ack was not received\n");
			spmp_i2cB_stop(i2c, -ENXIO);
			goto out_ack;
		}

		if (i2c->msg->flags & I2C_M_RD)
			i2c->state = STATE_READ;
		else
			i2c->state = STATE_WRITE;

		/* terminate the transfer if there is nothing to do
		 * as this is used by the i2c probe to find devices. */

		if (is_lastmsg(i2c) && i2c->msg->len == 0) {
			spmp_i2cB_stop(i2c, 0);
			goto out_ack;
		}

		if (i2c->state == STATE_READ)
			goto prepare_read;

		/* fall through to the write state, as we will need to
		 * send a byte as well */

	case STATE_WRITE:
		/* we are writing data to the device... check for the
		 * end of the message, and if so, work out what to do
		 */

		if (!(i2c->msg->flags & I2C_M_IGNORE_NAK)) {
			if (iicstat & ICSR_LASTBIT) {
				dev_dbg(i2c->dev, "WRITE: No Ack\n");

				spmp_i2cB_stop(i2c, -ECONNREFUSED);
				goto out_ack;
			}
		}

 retry_write:

		if (!is_msgend(i2c)) {
			byte = i2c->msg->buf[i2c->msg_ptr++];
			writeb(byte, i2c->regs + I2C_C_IDSR_OFST);

			/* delay after writing the byte to allow the
			 * data setup time on the bus, as writing the
			 * data to the register causes the first bit
			 * to appear on SDA, and SCL will change as
			 * soon as the interrupt is acknowledged */

		} else if (!is_lastmsg(i2c)) {
			/* we need to go to the next i2c message */

			dev_dbg(i2c->dev, "WRITE: Next Message\n");
			i2c->msg_ptr = 0;
			i2c->msg_idx++;
			i2c->msg++;

			/* check to see if we need to do another message */
			if (i2c->msg->flags & I2C_M_NOSTART) {

				if (i2c->msg->flags & I2C_M_RD) {
					/* cannot do this, the controller
					 * forces us to send a new START
					 * when we change direction */

					spmp_i2cB_stop(i2c, -EINVAL);
				}

				goto retry_write;
			} else {
				/* send the new start */
				spmp_i2cB_stoptransfer(i2c);
				udelay(10);
				spmp_i2cB_message_start(i2c, i2c->msg);
				i2c->state = STATE_START;
			}

		} else {
			/* send stop */

			spmp_i2cB_stop(i2c, 0);
		}
		break;

	case STATE_READ:
		/* we have a byte of data in the data register, do
		 * something with it, and then work out wether we are
		 * going to do any more read/write
		 */

		byte = readb(i2c->regs + I2C_C_IDSR_OFST);
		i2c->msg->buf[i2c->msg_ptr++] = byte;

 prepare_read:
		if (is_msglast(i2c)) {
			/* last byte of buffer */

			if (is_lastmsg(i2c))
				spmp_i2cB_disable_ack(i2c);

		} else if (is_msgend(i2c)) {
			/* ok, we've read the entire buffer, see if there
			 * is anything else we need to do */

			if (is_lastmsg(i2c)) {
				/* last message, send stop and complete */
				dev_dbg(i2c->dev, "READ: Send Stop\n");

				spmp_i2cB_stop(i2c, 0);
			} else {
				/* go to the next transfer */
				dev_dbg(i2c->dev, "READ: Next Transfer\n");

				i2c->msg_ptr = 0;
				i2c->msg_idx++;
				i2c->msg++;
			}
		}

		break;
	}

	/* acknowlegde the IRQ and get back on with the work */

 out_ack:
	tmp = readl(i2c->regs + I2C_C_ICCR_OFST);
	tmp |= ICCR_INTPREND;
	writel(tmp, i2c->regs + I2C_C_ICCR_OFST);
//	spmp_i2cB_enable_irq(i2c);
 out:
	return ret;
}

/* spmp_i2cB_irq
 *
 * top level IRQ servicing routine
*/

static irqreturn_t spmp_i2cB_irq(int irqno, void *dev_id)
{
	struct spmp_i2c *i2c = dev_id;
	unsigned int i2c_status;
	unsigned int tmp;

	i2c_status = readl(i2c->regs + I2C_C_ICSR_OFST);

	if (i2c_status & ICSR_ARB_FAIL) {
		/* deal with arbitration loss */
		printk("deal with arbitration loss\n");
	}
		
	if (i2c->state == STATE_IDLE) {
		dev_dbg(i2c->dev, "IRQ: error i2c->state == IDLE\n");
		tmp = readl(i2c->regs + I2C_C_ICCR_OFST);
		tmp |= ICCR_INTPREND;
		writel(tmp, i2c->regs +  I2C_C_ICCR_OFST);			
		goto out;
	}

	/* pretty much this leaves us with the fact that we've
	 * transmitted or received whatever byte we last sent */
	i2s_s3c_irq_nextbyte(i2c, i2c_status);
 out:
	return IRQ_HANDLED;
}


/* spmp_i2cB_set_master
 *
 * get the i2c bus for a master transaction
*/

static int spmp_i2cB_set_master(struct spmp_i2c *i2c)
{
	unsigned long iicstat;
	int timeout = 400;

	while (timeout-- > 0) {
		iicstat = readl(i2c->regs + I2C_C_ICSR_OFST);

		if (!(iicstat & ICSR_BUSY_STS))
			return 0;

		msleep(1);
	}

	return -ETIMEDOUT;
}

/* spmp_i2cB_doxfer
 *
 * this starts an i2c transfer
*/

static int spmp_i2cB_doxfer(struct spmp_i2c *i2c,
			      struct i2c_msg *msgs, int num)
{
	unsigned long timeout;
	int ret;

	if (i2c->suspended)
		return -EIO;

	ret = spmp_i2cB_set_master(i2c);
	if (ret != 0) {
		dev_err(i2c->dev, "cannot get bus (error %d)\n", ret);
		ret = -EAGAIN;
		goto out;
	}

	spin_lock_irq(&i2c->lock);
//    printk("spmp_i2cB_doxfer\n");
	i2c->msg     = msgs;
	i2c->msg_num = num;
	i2c->msg_ptr = 0;
	i2c->msg_idx = 0;
	i2c->state   = STATE_START;

	spmp_i2cB_enable_irq(i2c);
	spmp_i2cB_message_start(i2c, msgs);
	spin_unlock_irq(&i2c->lock);

	timeout = wait_event_timeout(i2c->wait, i2c->msg_num == 0, HZ * 5);

	ret = i2c->msg_idx;

	/* having these next two as dev_err() makes life very
	 * noisy when doing an i2cdetect */

	if (timeout == 0)
		dev_dbg(i2c->dev, "timeout\n");
	else if (ret != num)
		dev_dbg(i2c->dev, "incomplete xfer (%d)\n", ret);

	/* ensure the stop has been through the bus */

 out:
	return ret;
}

/* spmp_i2cB_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
*/

static int spmp_i2cB_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	struct spmp_i2c *i2c = (struct spmp_i2c *)adap->algo_data;
	int retry;
	int ret;
	for (retry = 0; retry < adap->retries; retry++) {

		ret = spmp_i2cB_doxfer(i2c, msgs, num);

		if (ret != -EAGAIN)
			return ret;

		dev_dbg(i2c->dev, "Retrying transmission (%d)\n", retry);

		udelay(100);
	}

	return -EREMOTEIO;
}

/* declare our i2c functionality */
static u32 spmp_i2cB_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

/* i2c bus registration info */

static const struct i2c_algorithm spmp_i2cB_algorithm = {
	.master_xfer		= spmp_i2cB_xfer,
	.functionality		= spmp_i2cB_func,
};

#endif

/* spmp_i2cB_init
 *
 * initialise the controller, set the IO lines and frequency
*/

static int spmp_i2cB_init(struct spmp_i2c *i2c)
{
	unsigned long iicon = 0;
	struct spmp_platform_i2c *pdata;

	/* get the plafrom data */

	pdata = i2c->dev->platform_data;

	writel(pdata->slave_addr, i2c->regs + I2C_C_IAR_OFST);

	dev_info(i2c->dev, "slave address 0x%02x\n", pdata->slave_addr);

	writel(iicon, i2c->regs + I2C_C_ICCR_OFST);
	
	writel(0x40, i2c->regs + I2C_C_TXLCLK_OFST);  //set tx clock 
    
	writel(0x04, i2c->regs + I2C_C_IDEBCLK_OFST); //iec de-bounce clock 

	return 0;
}

/* spmp_i2cB_probe
 *
 * called by the bus driver when a suitable device is found
*/

static int spmp_i2cB_probe(struct platform_device *pdev)
{
	struct spmp_i2c *i2c;
	struct spmp_platform_i2c *pdata;
	struct resource *res;
	int ret;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data\n");
		return -EINVAL;
	}

	i2c = kzalloc(sizeof(struct spmp_i2c), GFP_KERNEL);
	if (!i2c) {
		dev_err(&pdev->dev, "no memory for state\n");
		return -ENOMEM;
	}

	strlcpy(i2c->adap.name, "spmp-i2cB", sizeof(i2c->adap.name));
	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.algo    = &spmp_i2cB_algorithm;
	i2c->adap.retries = 2;
	i2c->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	spin_lock_init(&i2c->lock);
	init_waitqueue_head(&i2c->wait);

	/* find the clock and enable it */

	i2c->dev = &pdev->dev;
	i2c->clk = clk_get(&pdev->dev, "SYS_I2C");
	if (IS_ERR(i2c->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		ret = -ENOENT;
		goto err_noclk;
	}

	dev_dbg(&pdev->dev, "clock source %p\n", i2c->clk);

	clk_enable(i2c->clk);

	/* map the registers */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto err_clk;
	}
	i2c->ioarea = request_mem_region(res->start, (res->end-res->start)+1,
					 pdev->name);

	if (i2c->ioarea == NULL) {
		dev_err(&pdev->dev, "cannot request IO\n");
		ret = -ENXIO;
		goto err_clk;
	}

	i2c->regs = ioremap(res->start, (res->end-res->start)+1);

	if (i2c->regs == NULL) {
		dev_err(&pdev->dev, "cannot map IO\n");
		ret = -ENXIO;
		goto err_ioarea;
	}

	dev_dbg(&pdev->dev, "registers %p (%p, %p)\n",
		i2c->regs, i2c->ioarea, res);

	/* setup info block for the i2c core */

	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	/* initialise the i2c controller */

	ret = spmp_i2cB_init(i2c);
	if (ret != 0)
		goto err_iomap;

	/* find the IRQ for this unit (note, this relies on the init call to
	 * ensure no current IRQs pending
	 */
	i2c->irq = ret = platform_get_irq(pdev, 0);
	if (ret <= 0) {
		dev_err(&pdev->dev, "cannot find IRQ\n");
		goto err_iomap;
	}

	ret = request_irq(i2c->irq, spmp_i2cB_irq, IRQF_DISABLED,
			  dev_name(&pdev->dev), i2c);

	if (ret != 0) {
		dev_err(&pdev->dev, "cannot claim IRQ %d\n", i2c->irq);
		goto err_iomap;
	}

	/* Note, previous versions of the driver used i2c_add_adapter()
	 * to add the bus at any number. We now pass the bus number via
	 * the platform data, so if unset it will now default to always
	 * being bus 0.
	 */

	i2c->adap.nr = pdata->bus_num;
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add bus to i2c core\n");
		goto err_irq;
	}
	platform_set_drvdata(pdev, i2c);
	printk("spmp i2c probe\n");
	dev_info(&pdev->dev, "%s: I2C adapter\n", dev_name(&i2c->adap.dev));
	return 0;

 err_irq:
	free_irq(i2c->irq, i2c);

 err_iomap:
	iounmap(i2c->regs);

 err_ioarea:
	release_resource(i2c->ioarea);

 err_clk:
	clk_disable(i2c->clk);

 err_noclk:
	kfree(i2c);
	return ret;
}

/* spmp_i2cB_remove
 *
 * called when device is removed from the bus
*/

static int spmp_i2cB_remove(struct platform_device *pdev)
{
	struct spmp_i2c *i2c = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c->adap);
	free_irq(i2c->irq, i2c);
	clk_disable(i2c->clk);
	iounmap(i2c->regs);
	release_resource(i2c->ioarea);
	kfree(i2c);

	return 0;
}

#ifdef CONFIG_PM
static int spmp_i2cB_suspend_late(struct platform_device *dev,
				    pm_message_t msg)
{
	struct spmp_i2c *i2c = platform_get_drvdata(dev);
	i2c->suspended = 1;
	clk_disable(i2c->clk);	
	return 0;
}

static int spmp_i2cB_resume(struct platform_device *dev)
{
	struct spmp_i2c *i2c = platform_get_drvdata(dev);

	i2c->suspended = 0;
	clk_enable(i2c->clk);	
	spmp_i2cB_init(i2c);

	return 0;
}

#else
#define spmp_i2cB_suspend_late NULL
#define spmp_i2cB_resume NULL
#endif

/* device driver for platform bus bits */

static struct platform_driver spmp_i2c_driver = {
	.probe		= spmp_i2cB_probe,
	.remove		= spmp_i2cB_remove,
	.suspend_late	= spmp_i2cB_suspend_late,
	.resume		=spmp_i2cB_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "spmp-i2cB",
	},
};

static int __init i2cB_adap_spmp_init(void)
{
	int ret;

	ret = platform_driver_register(&spmp_i2c_driver);
	return ret;
}

static void __exit i2cB_adap_spmp_exit(void)
{
	platform_driver_unregister(&spmp_i2c_driver);
}

module_init(i2cB_adap_spmp_init);
module_exit(i2cB_adap_spmp_exit);

MODULE_DESCRIPTION("SPMP8000 I2C_C Bus driver");
MODULE_AUTHOR("Generalplus");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:spmp8000-i2c");


