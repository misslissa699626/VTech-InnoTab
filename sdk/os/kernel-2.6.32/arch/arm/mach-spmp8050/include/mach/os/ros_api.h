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
* @file ros_api.h
* @brief The include file for developing ros interfaces
* @author billyshieh@generalplus.com
*/

#ifndef _ROS_API_H_
#define _ROS_API_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* Define minimum stack size */
#define ROS_MIN_STACK_SIZE 0

/* Define API constants.  */
#define ROS_TRUE             1
#define ROS_FALSE            0

/* Constant for thread priority */
#define ROS_MAX_PRIORITIES   32

/* Constant for thread time slice */
#define ROS_NO_TIME_SLICE        0
#define ROS_DEFAULT_TIME_SLICE   5

/* Constant for thread auto start */
#define ROS_AUTO_START       1
#define ROS_DONT_START       0

/* Constant for timer activate */
#define ROS_AUTO_ACTIVATE    1
#define ROS_NO_ACTIVATE      0

/* Constant for mutex inherit */
#define ROS_NO_INHERIT       0
#define ROS_INHERIT          1

/* Constant for timeout */
/* timeout parameter is msec */
#define ROS_NO_WAIT      0
#define ROS_WAIT_FOREVER 0xFFFFFFFFUL

/* Constant for event flags */
#define ROS_FLAG_AND      0
#define ROS_FLAG_AND_CLR  1
#define ROS_FLAG_OR       2
#define ROS_FLAG_OR_CLR   3

/* Constant for return values */
#define ROS_OK                 0x00
#define ROS_ERR_PARAM          0x02
#define ROS_ERR_QUEUE_EMPTY    0x03
#define ROS_ERR_QUEUE_FULL     0x04
#define ROS_ERR_WAIT_ABORTED   0x05
#define ROS_ERR_NOT_AVAILABLE  0x06
#define ROS_ERR_NOT_OWNED      0x07
#define ROS_ERR_RESUME         0x08
#define ROS_ERR_SUSPEND        0x09
#define ROS_ERR_DESTROY        0x10


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define ROS_TICKS_TO_MSEC(x)  ((x) * 1000 / HZ)
#define ROS_MSEC_TO_TICKS(x)  ((x) * HZ / 1000)
 
#define ENTER_CRITICAL(x) x = ros_enter_critical()
#define EXIT_CRITICAL(x)  ros_exit_critical(x)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef signed char        ros_int8;
typedef unsigned char      ros_uint8;
typedef signed short       ros_int16;
typedef unsigned short     ros_uint16;
typedef signed int         ros_int32;
typedef unsigned int       ros_uint32;
typedef signed long long   ros_int64;
typedef unsigned long long ros_uint64;

typedef char               ros_char;
typedef ros_uint32         ros_bool;


typedef struct ros_thread_s {
	struct task_struct *ref;
	ros_char *name;
	void (*entry_function)(void *);
	void *entry_data;
} ros_thread;

typedef struct ros_sem_s {
	struct semaphore impl;
	ros_char *name;
} ros_sem;

typedef struct ros_timer_s {
	struct timer_list impl;
	ros_char *name;
	void (*func)(void *);
	void *data;
	ros_uint32 initVal;
	ros_uint32 interVal;
} ros_timer;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/* Thread */
/*!
 * @breif Create a thread.
 * @param thread The pointer to thread control block.
 * @param name The pointer to the name of thread.
 * @param entry_function Specifies the function for thread execution.
 * @param entry_data The data value passed to the thread entry routine.
 * @param stack_start The start address of stack.
 * @param stack_size The size of stack in byte.
 * @param priority The priority of thread.
 * @param time_slice The time slice value of thread in ticks.
 * @param auto_start Specifies whether the thread starts immediately or is placed in a suspended state.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_create(ros_thread *thread, ros_char *name,
				void (*entry_function)(void *), void *entry_data,
				void *stack_start, ros_uint32 stack_size,
				ros_uint32 priority,
				ros_uint32 time_slice, ros_uint32 auto_start);

/*!
 * @breif Destroy a thread.
 * @param thread The pointer to thread control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_DESTROY Fail.
 */
ros_uint32 ros_thread_destroy(ros_thread *thread);

/*!
 * @breif Resume specified thread.
 * @param thread The pointer to thread control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_resume(ros_thread *thread);

/*!
 * @breif Suspend current thread for specified time period.
 * @param timeout The ticks for thread to sleep.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_sleep(ros_uint32 timeout);


/* Semaphore */
/*!
 * @breif Create a semaphore.
 * @param sem The pointer to semaphore control block.
 * @param name The pointer to the name of semaphore.
 * @param initial_value The value to initial semaphore count.
 * @return ROS_OK Success.
 */
ros_uint32 ros_sem_create(ros_sem *sem, ros_char *name,
				ros_uint32 initial_value);

/*!
 * @breif Destroy a semaphore.
 * @param sem The pointer to semaphore control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_sem_destroy(ros_sem *sem);

/*!
 * @breif wait on a counting semaphore.
 * @param sem The pointer to semaphore control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_sem_wait(ros_sem *sem);

/*!
 * @breif get a semaphore if available.
 * @param sem The pointer to semaphore control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_sem_trywait(ros_sem *sem);

/*!
 * @breif Wait on a semaphore with timeout.
 * @param sem The pointer to semaphore control block.
 * @param timeout The value to set the absolute timeout(tick).
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_sem_timedwait(ros_sem *sem, ros_uint32 timeout);

/*!
 * @breif Increment semaphore count.
 * @param sem The pointer to semaphore control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_sem_post(ros_sem *sem);


/* Timers */
/*!
 * @breif Create a timer.
 * @param timer The pointer to timer control block.
 * @param pname The pointer to the name of timer.
 * @param func The pointer to the call back function of timer.
 * @param data The pointer to the data to be passed to callback of timer.
 * @param initial The value to set trigger time.
 * @param interval The value to set re-trigger interval.
 * @param auto_activate The flag for enable/disable timer auto-active function.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_create(ros_timer *timer, ros_char *pname,
				void (*func)(void *), void *data,
				ros_uint32 initial, ros_uint32 interval,
				ros_uint32 auto_activate);

/*!
 * @breif Destroy a timer.
 * @param timer The pointer to timer control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_destroy(ros_timer *timer);

/*!
 * @breif Enable a timer.
 * @param timer The pointer to timer control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_activate(ros_timer *timer);

/*!
 * @breif Disable a timer.
 * @param timer The pointer to timer control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_deactivate(ros_timer *timer);

/*!
 * @breif Modify interval value of time.
 * @param timer The pointer to timer control block.
 * @param initial The value to set trigger time.
 * @param interval The value to set re-trigger interval.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_set_times(ros_timer *timer,
				ros_uint32 initial, ros_uint32 interval);

/*!
 * @breif Return the next trigger time for the timer and its re-trigger interval.
 * @param timer The pointer to timer control block.
 * @param initial The pointer to trigger time of timer.
 * @param interval The pointer to current re-trigger interval of timer.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_get_times(ros_timer *timer,
				ros_uint32 *initial, ros_uint32 *interval);


/* System Clock */
ros_uint64 ros_clock_get(void);


/* Interrupt Control */
ros_uint32 ros_enter_critical(void);
void ros_exit_critical(ros_uint32 state);

#if 0
int ros_irq_request(unsigned int irq,
		int (*handler)(int, void *),
		unsigned long irqflags,
		const char * dev_name,
		void *dev_id);
void ros_irq_release(unsigned int irq, void *dev_id);
#endif

#ifdef  __cplusplus
}
#endif

#endif /* _ROS_API_H_ */
