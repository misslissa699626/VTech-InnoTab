#include <mach/hal/hal_timer.h>


/*
 *@brief the pwm enable/disable function
 *@param enable[in]:0:disable; 1:enable
 */
void gpHalPwmEn(int id,int enable)
{
	if(enable){
		gpHalTimerSetCtrl(id,0x41d);	
		gpHalTimerSetCtrl(id,0x40d);
	}
	else{
		gpHalTimerSetCtrl(id,0);	
	}

}
