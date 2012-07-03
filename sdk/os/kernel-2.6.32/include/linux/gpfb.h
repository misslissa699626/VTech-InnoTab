/*
 * Gpfb!! common define  Rev. 3.1.0
 *
 */

#ifndef _LINUX_GPFB_H
#define _LINUX_GPFB_H

#define GPFB_STATE_NORMAL       0
#define GPFB_STATE_SUSPEND      1
#define GPFB_STATE_RESUME       2

#define GPFB_SHRINK_NONE        0
#define GPFB_SHRINK_ALL         1
#define GPFB_SHRINK_LIMIT1      2
#define GPFB_SHRINK_LIMIT2      3

#ifndef __ASSEMBLY__

extern int pm_device_down;
extern int gpfb_shrink;
extern int gpfb_swapout_disable;
extern int gpfb_separate_pass;
extern int gpfb_canceled;

int gpfb_set_savearea(unsigned long start, unsigned long end);
void gpfb_save_cancel(void);

#ifdef CONFIG_PM_GPFB_DEBUG

int gpfb_printf(const char *fmt, ...);
void gpfb_putc(char c);

#else

#define gpfb_printf(fmt...)
#define gpfb_putc(c)

#endif

#endif  /* __ASSEMBLY__ */

#endif  /* _LINUX_GPFB_H */
