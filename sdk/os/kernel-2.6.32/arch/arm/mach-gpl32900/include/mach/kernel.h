
#include <linux/types.h> /* size_t */
#include <linux/errno.h> /* error codes */
#include <linux/string.h>
#include <linux/kernel.h> /* printk() */

#include <linux/kthread.h>

#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/delay.h>

#include <linux/rtc.h>

#include <linux/slab.h> /* kmalloc() */
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/clk.h>

#include <linux/fs.h> /* everything... */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/ioctl.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/configfs.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/list.h>

#include <asm/io.h>
#include <asm/div64.h>
#include <asm/mach/map.h>
#include <asm/mman.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

