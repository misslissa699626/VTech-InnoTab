#include <mach/os/ros_api.h>
#include <linux/sched.h>

/* Thread */
static int
__ros_thread_run(
	void *data
)
{
	ros_thread *thread = (ros_thread *)data;
	
	thread->entry_function(thread->entry_data);

	while (!kthread_should_stop()) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(ROS_MSEC_TO_TICKS(200));	
	}

	return 0;
}

ros_uint32
ros_thread_create(
	ros_thread *thread,
	ros_char *name,
	void (*entry_function)(void *),
	void *entry_data,
	void *stack_start,
	ros_uint32 stack_size,
	ros_uint32 priority,
	ros_uint32 time_slice,
	ros_uint32 auto_start
)
{
	thread->name = name;
	thread->entry_function = entry_function;
	thread->entry_data = entry_data;
	
	thread->ref = kthread_create(__ros_thread_run, thread, name);

	if (auto_start == ROS_AUTO_ACTIVATE) {
		wake_up_process(thread->ref);
	}

	return ROS_OK;
}
EXPORT_SYMBOL(ros_thread_create);

ros_uint32
ros_thread_destroy(
	ros_thread *thread
)
{
	int thread_result;

	thread_result = kthread_stop(thread->ref);
	
	return ROS_OK;
}
EXPORT_SYMBOL(ros_thread_destroy);

ros_uint32
ros_thread_resume(
	ros_thread *thread
)
{
	wake_up_process(thread->ref);

	return ROS_OK;
}
EXPORT_SYMBOL(ros_thread_resume);

ros_uint32
ros_thread_sleep(
	ros_uint32 timeout
)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(ROS_MSEC_TO_TICKS(timeout));

	return ROS_OK;
}
EXPORT_SYMBOL(ros_thread_sleep);
