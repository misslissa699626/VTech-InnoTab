#include <mach/os/ros_api.h>


ros_uint32 
ros_sem_create(
	ros_sem *sem,
	ros_char *name,
	ros_uint32 initial_value
)
{
	sema_init(&sem->impl, initial_value);
	sem->name = name;

	return ROS_OK;
}
EXPORT_SYMBOL(ros_sem_create);

ros_uint32 
ros_sem_destroy(
	ros_sem *sem
)
{
	memset(&sem->impl, 0, sizeof(sem->impl));
	sem->name = NULL;

	return ROS_OK;
}
EXPORT_SYMBOL(ros_sem_destroy);

ros_uint32 
ros_sem_wait(
	ros_sem *sem
)
{
	down(&sem->impl);

	return ROS_OK;
}
EXPORT_SYMBOL(ros_sem_wait);

ros_uint32 
ros_sem_trywait(
	ros_sem *sem
)
{
	int result;

	result = down_trylock(&sem->impl);
	if (result == 0)
		return ROS_OK;
	else
		return ROS_ERR_NOT_AVAILABLE;
}
EXPORT_SYMBOL(ros_sem_trywait);

ros_uint32 
ros_sem_timedwait(
	ros_sem *sem, 
	ros_uint32 timeout
)
{
	int result;
	
	if (timeout == ROS_WAIT_FOREVER) {
		down(&sem->impl);
		result = 0;
	}
	else if (timeout == ROS_NO_WAIT) {
		result = down_trylock(&sem->impl);
	}
	else {
		result = down_timeout(&sem->impl, timeout * HZ / 1000);
	}

	if (result == 0)
		return ROS_OK;
	else if (result == -ETIME)
		return ROS_ERR_NOT_AVAILABLE;
	else
		return ROS_ERR_NOT_AVAILABLE;
}
EXPORT_SYMBOL(ros_sem_timedwait);

ros_uint32 
ros_sem_post(
	ros_sem *sem
)
{
	up(&sem->impl);

	return ROS_OK;
}
EXPORT_SYMBOL(ros_sem_post);
