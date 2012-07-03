#include <mach/os/ros_api.h>
#include <linux/spinlock.h>

static spinlock_t gRosSpinLock = SPIN_LOCK_UNLOCKED;

void
ros_interrupt_init(
	void
)
{
	spin_lock_init(&gRosSpinLock);
}

ros_uint32
ros_enter_critical(
	void
)
{
	unsigned long flags;

	spin_lock_irqsave(&gRosSpinLock, flags);

	return flags;
}

void
ros_exit_critical(
	ros_uint32 state
)
{
	spin_unlock_irqrestore(&gRosSpinLock, (unsigned long)state);
}
