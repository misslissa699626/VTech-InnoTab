#include <mach/hal/hal_timer.h>
#include <mach/hal/regmap/reg_scu.h>

/*
 *@brief the pwm enable/disable function
 *@param enable[in]:0:disable; 1:enable
 */
void gpHalPwmEn(int id,int enable)
{
	if(enable){
        /*GPL32900 Walk around for duty cycle problem.*/
        // disable first 
        gpHalTimerSetCtrl(id,0);
		gpHalTimerSetCtrl(id,0x41d); /* up counting */
	}
	else{
		gpHalTimerSetCtrl(id,0);
	}
}
