#include <mach/os/ros_api.h>
#include <linux/jiffies.h>

/* System Clock */

ros_uint64
ros_clock_get(
	void
)
{
	return get_jiffies_64();
}
EXPORT_SYMBOL(ros_clock_get);
