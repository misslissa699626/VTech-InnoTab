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
 
#ifndef _ROS_IMPL_H_
#define _ROS_IMPL_H_

#ifdef  __cplusplus
extern  "C" {
#endif

#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/resource.h>


#define DEBUG_LEVEL 3
#define ROS_MAX_THREAD_DATA 6
#define ROS_SET_NAME(dstName, srcName) do{\
	struct fast_copy_s { char name[ROS_MAX_NAME]; };\
	struct fast_copy_s* dst = (struct fast_copy_s*)(dstName);\
	struct fast_copy_s* src = (struct fast_copy_s*)(srcName);\
	*dst = *src;\
	(dstName)[ROS_MAX_NAME-1]='\0';}while(0)

#if DEBUG_LEVEL < 1
#define ROS_DBG_ERR(args...) 
#define ROS_DBG_WRN(args...)
#define ROS_DBG_LOG(args...)  
#define ROS_DBG_INF(args...) 
#elif DEBUG_LEVEL < 2
#define ROS_DBG_ERR(args...) printf(args)
#define ROS_DBG_WRN(args...) 
#define ROS_DBG_LOG(args...)  
#define ROS_DBG_INF(args...) 
#elif DEBUG_LEVEL < 3
#define ROS_DBG_ERR(args...) printf(args)
#define ROS_DBG_WRN(args...) printf(args)
#define ROS_DBG_LOG(args...)  
#define ROS_DBG_INF(args...) 
#elif DEBUG_LEVEL < 4
#define ROS_DBG_ERR(args...) printf(args)
#define ROS_DBG_WRN(args...) printf(args)
#define ROS_DBG_LOG(args...) printf(args)
#define ROS_DBG_INF(args...) 
#else
#define ROS_DBG_ERR(args...) printf(args)
#define ROS_DBG_WRN(args...) printf(args)
#define ROS_DBG_LOG(args...) printf(args)
#define ROS_DBG_INF(args...) printf(args)
#endif
/**************************************************************************
 * Data Structure
 **************************************************************************/
typedef struct
{
	ros_uint16          id;
	/* TBD */
} ros_thread_info;

typedef struct
{
	pthread_t impl;    
	ros_char name[ROS_MAX_NAME];
	ros_uint32 thdata[ROS_MAX_THREAD_DATA];
    ros_uint32 active;
    void *(*entry)(void*);
    ros_uint32 entry_data;
    ros_uint32 priority;
    ros_uint32 auto_start;
} ros_thread;

typedef struct ros_mutex_t
{
	pthread_mutex_t impl;			/*<! The mutex structure of eCos */
	ros_char name[ROS_MAX_NAME];				/*<! The name of mutex */
	ros_thread *threadHandle;	/*<! The handle of thread that occupied the mutex */
	ros_uint16 nestLevel;		/*<! The nest level of this mutex, used for recursive mode */
	/*struct ros_mutex_t *prevMutex;
	struct ros_mutex_t *nextMutex;*/
} ros_mutex;

typedef struct ros_cond_t
{
	pthread_cond_t impl;
	ros_mutex *mutex;
	ros_char name[ROS_MAX_NAME];
	/*struct ros_cond_t *prevCond;
	struct ros_cond_t *nextCond;*/
} ros_cond;

typedef struct ros_sem_t
{
	sem_t impl;
	ros_char name[ROS_MAX_NAME];
	/*struct ros_sem_t *prevSem;
	struct ros_sem_t *nextSem;*/
} ros_sem;


typedef struct ros_queue_t
{
	ros_char *name;			/*<! The name of queue */
	ros_mutex mutex;		/*<! The mutex handle */
	ros_sem sem_write;		/*<! The semaphore handle used for queue write */
	ros_sem sem_read;		/*<! The semaphore handle used for queue read */
	ros_uint32 *queue_start;/*<! The start address of queue */
	ros_uint32 queue_size;	/*<! The size of this queue in byte */
	ros_uint32 capacity;	/*<! The capacity of this queue, it depends on queue_size & msg_size */
	ros_uint32 msg_size;	/*<! The size of msg in byte, it can only be 1/2/4/8/16 32-bit size */
	ros_uint32 *start;		/*<! The start address of queue */
	ros_uint32 *end;		/*<! The end address of queue */
	ros_uint32 *write;		/*<! The address of write pointer */
	ros_uint32 *read;		/*<! The address of read pointer */
	struct ros_queue_t *prevQueue;
	struct ros_queue_t *nextQueue;	
} ros_queue;


typedef struct ros_timer_t
{  
   timer_t impl;
   void (*func)(void *);
   void *data;
   ros_uint32 initVal;
   ros_uint32 interVal;
   ros_char *name;
   /*struct ros_timer_t *prevTimer;
   struct ros_timer_t *nextTimer;	*/
} ros_timer;


typedef struct ros_flag_t
{
	ros_uint32 value;
	ros_char name[ROS_MAX_NAME];
	pthread_mutex_t *flock;
    pthread_cond_t  *fcond;
    /*struct ros_flag_t *prevFlag;
	struct ros_flag_t *nextFlag;*/
} ros_flag;

typedef pthread_barrier_t ros_barrier;

#ifdef  __cplusplus
       }
#endif

#endif /* _ROS_IMPL_H_ */
