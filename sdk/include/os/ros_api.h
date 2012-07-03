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
* @author kthuang@generalplus.com
*/
 
#ifndef _ROS_API_H_
#define _ROS_API_H_

#ifdef  __cplusplus
extern  "C" {
#endif

/**************************************************************************
 * Base Type
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

/**************************************************************************
 * Constant
 **************************************************************************/
/* Define API constants.  */
#define ROS_TRUE             1
#define ROS_FALSE            0
#define ROS_MAX_NAME		32

/* Constant for thread priority */
#define ROS_MAX_PRIORITIES   32

/* Constant for thread time slice */
#define ROS_NO_TIME_SLICE        0
#define ROS_DEFAULT_TIME_SLICE   5

/* Constant for thread auto start */
#define ROS_AUTO_START       1
#define ROS_DONT_START       0

/* Constant for per-thread data */
#define ROS_THREAD_DATA_KERNEL 0
#define ROS_THREAD_DATA_USER   1
#define ROS_THREAD_DATA_ERRNO  2
#define ROS_THREAD_DATA_VFS    3
#define ROS_THREAD_DATA_MAX    6

/* Constant for message size */
#define ROS_MSG_1_UINT32          1
#define ROS_MSG_2_UINT32          2
#define ROS_MSG_4_UINT32          4
#define ROS_MSG_8_UINT32          8
#define ROS_MSG_16_UINT32         16

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
#define ROS_ERR                0x01
#define ROS_ERR_PARAM          0x02
#define ROS_ERR_QUEUE_EMPTY    0x03
#define ROS_ERR_QUEUE_FULL     0x04
#define ROS_ERR_WAIT_ABORTED   0x05
#define ROS_ERR_NOT_AVAILABLE  0x06
#define ROS_ERR_NOT_OWNED      0x07
#define ROS_ERR_RESUME         0x08
#define ROS_ERR_SUSPEND        0x09
#define ROS_ERR_DESTROY        0x10
#define ROS_ERR_ETIMEDOUT       0x11

/*
 * These flags used only by the kernel as part of the
 * irq handling routines.
 *
 * IRQF_DISABLED - keep irqs disabled when calling the action handler
 * IRQF_SAMPLE_RANDOM - irq is used to feed the random generator
 * IRQF_SHARED - allow sharing the irq among several devices
 * IRQF_PROBE_SHARED - set by callers when they expect sharing mismatches to occur
 * IRQF_TIMER - Flag to mark this interrupt as timer interrupt
 * IRQF_PERCPU - Interrupt is per cpu
 * IRQF_NOBALANCING - Flag to exclude this interrupt from irq balancing
 * IRQF_IRQPOLL - Interrupt is used for polling (only the interrupt that is
 *                registered first in an shared interrupt is considered for
 *                performance reasons)
 */
#define IRQF_DISABLED		0x00000020
#define IRQF_SAMPLE_RANDOM	0x00000040
#define IRQF_SHARED		0x00000080
#define IRQF_PROBE_SHARED	0x00000100
#define IRQF_TIMER		0x00000200
#define IRQF_PERCPU		0x00000400
#define IRQF_NOBALANCING	0x00000800
#define IRQF_IRQPOLL		0x00001000

/*
 * SA_FLAGS values:
 *
 * SA_INTERRUPT is a no-op, but left due to historical reasons. Use the
 */
#define SA_INTERRUPT          IRQF_DISABLED 
#define SA_SAMPLE_RANDOM      IRQF_SAMPLE_RANDOM 
#define SA_SHIRQ              IRQF_SHARED 
#define SA_PROBEIRQ            IRQF_PROBE_SHARED 

/**************************************************************************
 * Macro
 **************************************************************************/
#define ROS_TICKS_TO_MSEC(x)  ((x) * 10)
#define ROS_MSEC_TO_TICKS(x)  (((x) + 9) / 10)
 
#define ENTER_CRITICAL(x) x = ros_enter_critical()
#define EXIT_CRITICAL(x)  ros_exit_critical(x)

#define CYG_OFFSETOF(type, field) ((int)&(((type*)0)->field))
#define CYG_DNODE_TO_THREAD(x) ((cyg_thread*)(((char*)x) - CYG_OFFSETOF(cyg_thread, next)))

#define DO_MALLOC(x)     malloc( x ) /*!< Macro for doing dynamic memory manageemnt. */
#define DO_FREE(x)       free( ( void * )( x ) ) /*!< Macro for freeing memory. */

/**************************************************************************
 * Data Structure
 **************************************************************************/
#include "os/ros_impl.h"

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
 * @breif Get the handle of current thread.
 * @return Return the handle of current thread.
 */
ros_thread *ros_thread_self(void);

/*!
 * @breif Relinquish control to other application threads.
 */
void ros_thread_yield(void);

/*!
 * @breif Suspend current thread for specified time period.
 * @param timeout The ticks for thread to sleep.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_sleep(ros_uint32 timeout);

/*!
 * @breif Suspend specified thread.
 * @param thread The pointer to thread control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_suspend(ros_thread *thread);

/*!
 * @breif Resume specified thread.
 * @param thread The pointer to thread control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_resume(ros_thread *thread);

/*!
 * @breif Kill specified thread.
 * @param thread The pointer to thread control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_kill(ros_thread *thread);

/*!
 * @breif Exit the current thread.
 */
void ros_thread_exit(void);

/*!
 * @breif Abort suspension of specified thread.
 * @param thread The pointer to thread control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_abort_wait(ros_thread *thread);

/*!
 * @breif Get priority of specified thread.
 * @param thread The pointer to thread control block.
 * @param priority The pointer to store the priority value.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_get_priority(ros_thread *thread, ros_uint32 *priority);

/*!
 * @breif Set priority of specified thread.
 * @param thread The pointer to thread control block.
 * @param priority The value to set the thread's priority.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_set_priority(ros_thread *thread, ros_uint32 priority);

/*!
 * @breif Get data of specified thread.
 * @param index The offset of the per-thread data.
 * @param data The pointer of the per-thread data at the specified index for the current thread.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_get_data(ros_uint32 index, ros_uint32 *data);

/*!
 * @breif Set entry data of specified thread.
 * @param index The offset of the per-thread data.
 * @param data The value to set at per-thread data index.
 * @return ROS_OK Success.
 */
ros_uint32 ros_thread_set_data(ros_uint32 index, ros_uint32 data);
/*ros_uint32 ros_thread_get_info(ros_thread *thread, ros_thread_info *info);*/





ros_uint32 ros_thread_wait_end(ros_thread *thread);


/* Mutex */
/*!
 * @breif Create a mutex.
 * @param mutex The pointer to mutex control block.
 * @param name The pointer to the name of mutex.
 * @param inherit The flag for enable/disable priority inherit.
 * @return ROS_OK Success.
 */
ros_uint32 ros_mutex_create(ros_mutex *mutex, ros_char *name,
				ros_uint32 inherit);

/*!
 * @breif Destroy a mutex.
 * @param mutex The pointer to mutex control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_mutex_destroy(ros_mutex *mutex);

/*!
 * @breif Obtain the ownership of mutex.
 * @param mutex The pointer to mutex control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_mutex_lock(ros_mutex *mutex);

/*!
 * @breif Try to Obtain the ownership of mutex.
 * @param mutex The pointer to mutex control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_mutex_trylock(ros_mutex *mutex);

/*!
 * @breif Release ownership of mutex.
 * @param mutex The pointer to mutex control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_mutex_unlock(ros_mutex *mutex);

/* Condition */
/*!
 * @breif Create a condition.
 * @param cond The pointer to condition control block.
 * @param name The pointer to the name of condition.
 * @param mutex The pointer to the mutex of condition.
 * @return ROS_OK Success.
 */
ros_uint32 ros_cond_create(ros_cond *cond, ros_char *name, ros_mutex *mutex);

/*!
 * @breif Destroy a condition.
 * @param cond The pointer to condition control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_cond_destroy(ros_cond *cond);

/*!
 * @breif Wait on a condition variable.
 * @param cond The pointer to condition control block.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_cond_wait(ros_cond *cond);

/*!
 * @breif Wait on a condition variable with timeout.
 * @param cond The pointer to condition control block.
 * @param timeout The value to set the absolute timeout(msec).
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_cond_timedwait(ros_cond *cond, ros_uint32 timeout);

/*!
 * @breif Wake one thread waiting on a condition variable
 * @param cond The pointer to condition control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_cond_signal(ros_cond *cond);

/*!
 * @breif Wake all thread waiting on a condition variable
 * @param cond The pointer to condition control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_cond_broadcast(ros_cond *cond);

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

/*!
 * @breif Get current semaphore count.
 * @param sem The pointer to semaphore control block.
 * @param value The pointer to receive count of semaphore.
 * @return ROS_OK Success.
 */
ros_uint32 ros_sem_get_value(ros_sem *sem, ros_uint32 *value);

/* Event Flags */
/*!
 * @breif Create a event flag.
 * @param flag The pointer to flag control block.
 * @param name The pointer to the name of flag.
 * @return ROS_OK Success.
 */
ros_uint32 ros_flag_create(ros_flag *flag, ros_char *name);

/*!
 * @breif Destroy a event flag.
 * @param flag The pointer to flag control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_flag_destroy(ros_flag *flag);

/*!
 * @breif Wait for the flag is set as given pattern.
 * @param flag The pointer to flag control block.
 * @param pattern The 32-bits variable the represent the requested flags.
 * @param wait_mode Specify the conditons for wake up.
 * @param actual_value The pointer to the variable to store the actual flag when the thread is woken up.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_flag_wait(ros_flag *flag, ros_uint32 pattern,
				ros_uint32 wait_mode, ros_uint32 *actual_value);

/*!
 * @breif Try to wait for the flag is set as given pattern in specified time period.
 * @param flag The pointer to flag control block.
 * @param pattern The 32-bits variable the represent the requested flags.
 * @param wait_mode Specify the conditons for wake up.
 * @param actual_value The pointer to the variable to store the actual flag when the thread is woken up.
 * @param timeout The timeout value to wait for flag conditions to be met.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_flag_timedwait(ros_flag *flag, ros_uint32 pattern,
				ros_uint32 wait_mode, ros_uint32 *actual_value,
				ros_uint32 timeout);

/*!
 * @breif Set flag's value.
 * @param flag The pointer to flag control block.
 * @param value The bits that are set to one in this parameter are set along with the current bits set in the flag.
 * @return ROS_OK Success.
 */
ros_uint32 ros_flag_setbits(ros_flag *flag, ros_uint32 value);

/*!
 * @breif Mask flag's value.
 * @param flag The pointer to flag control block.
 * @param value The bits that are set to zero in this parameter are cleared in the flag.
 * @return ROS_OK Success.
 */
ros_uint32 ros_flag_maskbits(ros_flag *flag, ros_uint32 value);

/*!
 * @breif Get flag's value.
 * @param flag The pointer to flag control block.
 * @param value The pointer of the variable to store the current flag's value.
 * @return ROS_OK Success.
 */
ros_uint32 ros_flag_get_value(ros_flag *flag, ros_uint32 *value);

/* Message Queue */
/*!
 * @breif Create a message queue.
 * @param queue The pointer to queue control block.
 * @param name The pointer to the name of message queue.
 * @param msg_size The size of each message. It can be ROS_MSG_1_UINT32/ROS_MSG_2_UINT32/ROS_MSG_4_UINT32/ROS_MSG_8_UINT32/ROS_MSG_16_UINT32.
 * @param queue_start The start address of message queue.
 * @param queue_size The size of message queue in bytes.
 * @return ROS_OK Success.
 */
ros_uint32 ros_queue_create(ros_queue *queue, ros_char *name,
				ros_uint32 msg_size, void *queue_start,
				ros_uint32 queue_size);

/*!
 * @breif Destroy a message queue.
 * @param queue The pointer to queue control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_queue_destroy(ros_queue *queue);

/*!
 * @breif Send a message to the message queue.
 * @param queue The pointer to queue control block.
 * @param msg The pointer of the message to be sent
 * @param timeout The timeout value to wait when trying to send a message in the message queue.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_queue_send(ros_queue *queue, void *msg, ros_uint32 timeout);

/*!
 * @breif Send a message to the front of message queue.
 * @param queue The pointer to queue control block.
 * @param msg The pointer of the message to be sent
 * @param timeout The timeout value to wait when trying to send a message in the message queue.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_queue_front_send(ros_queue *queue, void *msg,
				ros_uint32 timeout);

/*!
 * @breif Receive a message from the message queue.
 * @param queue The pointer to queue control block.
 * @param msg The pointer to store the received message.
 * @param timeout The timeout value to wait when trying to receive a message from the message queue.
 * @return ROS_OK Success.
 * @return ROS_ERR_NOT_AVAILABLE Error.
 */
ros_uint32 ros_queue_receive(ros_queue *queue, void *msg, ros_uint32 timeout);

/*!
 * @breif Receive a message from the message queue without removing it from message queue
 * @param queue The pointer to queue control block.
 * @param msg The pointer to store received message.
 * @return ROS_OK Success.
 * @return ROS_ERR_QUEUE_EMPTY Message queue is empty.
 */
ros_uint32 ros_queue_peek(ros_queue *queue, void *msg);

/*!
 * @breif Empty messages of the message queue.
 * @param queue The pointer to queue control block.
 * @return ROS_OK Success.
 */
ros_uint32 ros_queue_flush(ros_queue *queue);

/*!
 * @breif Get free size of the message queue.
 * @param queue The pointer to queue control block.
 * @param count The pointer to store the free size count.
 * @return ROS_OK Success.
 */
ros_uint32 ros_queue_get_free_size(ros_queue *queue, ros_uint32 *count);

/*!
 * @breif Get used size of the message queue.
 * @param queue The pointer to queue control block.
 * @param count The pointer to store the used size count.
 * @return ROS_OK Success.
 */
ros_uint32 ros_queue_get_used_size(ros_queue *queue, ros_uint32 *count);

/* Timers */
/*!
 * @breif Create a timer.
 * @param timer The pointer to timer control block.
 * @param pname The pointer to the name of timer.
 * @param func The pointer to the call back function of timer.
 * @param data The pointer to the data to be passed to callback of timer.
 * @param initial The value to set absolute trigger time.
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
 * @breif Initialize or start a timer.
 * @param timer The pointer to timer control block.
 * @param initial The value to set absolute trigger time.
 * @param interval The value to set re-trigger interval.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_set_times(ros_timer *timer,
				ros_uint32 initial, ros_uint32 interval);

/*!
 * @breif Return the next absolute trigger time for the timer and its re-trigger interval.
 * @param timer The pointer to timer control block.
 * @param initial The pointer to next trigger time of timer.
 * @param interval The pointer to current re-trigger interval of timer.
 * @return ROS_OK Success.
 */
ros_uint32 ros_timer_get_times(ros_timer *timer,
				ros_uint32 *initial, ros_uint32 *interval);

/* System Clock */
ros_uint64 ros_clock_get(void);
ros_uint32 ros_clock_set(ros_uint64 new_time);


ros_uint32 ros_barrier_init(ros_barrier *barrier, int val);
ros_uint32 ros_barrier_wait(ros_barrier *barrier);
ros_uint32 ros_barrier_destroy(ros_barrier *barrier);



#ifdef  __cplusplus
       }
#endif

#endif /* _ROS_API_H_ */
