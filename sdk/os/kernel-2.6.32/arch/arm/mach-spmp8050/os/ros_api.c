#include <linux/init.h>
#include <linux/module.h>
#include <mach/os/ros_api.h>

extern void ros_sem_init(void);
extern void ros_flag_init(void);
extern void ros_timer_init(void);
extern void ros_interrupt_init(void);

static int
ros_api_init(
	void
)
{
	//ros_sem_init();
	//ros_flag_init();
	//ros_timer_init();
	ros_interrupt_init();

	return 0;
}

static void
ros_api_exit(
	void
)
{
}

module_init(ros_api_init);
module_exit(ros_api_exit);
