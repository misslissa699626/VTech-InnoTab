#include <mach/os/ros_api.h>
#include <linux/jiffies.h>


static void
__ros_timer_timeout(
	unsigned long data
)
{
	ros_timer *timer = (ros_timer*)data;
	
	timer->func(timer->data);

	if (timer->interVal > 0) {
		timer->impl.expires = jiffies + ROS_MSEC_TO_TICKS(timer->interVal);
		add_timer(&timer->impl);
	}
}

ros_uint32 
ros_timer_create(
	ros_timer *timer, 
	ros_char *name,
	void (*func)(void *), 
	void *data,
	ros_uint32 initial, 
	ros_uint32 interval,
	ros_uint32 auto_activate
)
{
	timer->name = name;
	timer->func = func;
	timer->data = data;
	timer->initVal = initial;
	timer->interVal = interval;

	init_timer(&timer->impl);
	timer->impl.function = __ros_timer_timeout;
	timer->impl.data = (unsigned long)timer;
	timer->impl.expires = jiffies + ROS_MSEC_TO_TICKS(initial);
	
	if (auto_activate == ROS_AUTO_ACTIVATE) {
		add_timer(&timer->impl);
	}

	return ROS_OK;
}
EXPORT_SYMBOL(ros_timer_create);

ros_uint32 
ros_timer_destroy(
	ros_timer *timer
)
{
	del_timer(&timer->impl);

	return ROS_OK;
}
EXPORT_SYMBOL(ros_timer_destroy);

ros_uint32 
ros_timer_activate(
	ros_timer *timer
)
{
	timer->impl.expires = jiffies + ROS_MSEC_TO_TICKS(timer->initVal);
	add_timer(&timer->impl);

	return ROS_OK;
}
EXPORT_SYMBOL(ros_timer_activate);

ros_uint32 
ros_timer_deactivate(
	ros_timer *timer
)
{
	del_timer(&timer->impl);

	return ROS_OK;
}
EXPORT_SYMBOL(ros_timer_deactivate);

ros_uint32 
ros_timer_set_times(
	ros_timer *timer,
	ros_uint32 initial, 
	ros_uint32 interval
)
{
	timer->initVal = initial;
	timer->interVal = interval;

	return ROS_OK;
}
EXPORT_SYMBOL(ros_timer_set_times);

ros_uint32 
ros_timer_get_times(
	ros_timer *timer,
	ros_uint32 *initial, 
	ros_uint32 *interval
)
{
	*initial = timer->initVal;
	*interval = timer->interVal;

	return ROS_OK;
}
EXPORT_SYMBOL(ros_timer_get_times);
