#ifndef __GP_SKEL_H__
#define __GP_SKEL_H__
#include <mach/common.h>

typedef struct
{
	UINT8	clk_tog_en;	/*!< Specifies whether continous toggle SARCK to SAR ADC 
				This parameter can be set to 1-ENABLE or 0-DISABLE */
	UINT8	x2y_dly;	/*!< auto conversion x to y conversion delay, cycles in SARCK */

	UINT16	interval_dly;	/*!< auto conversion delay between 2 conversions, cycles in SARCK */

	UINT8	chkdly;		/*!< pen status re-check time (cycles in SARCK) */

	UINT16	debounce_dly;	/*!<SAR ADC de-bounce time (cycles in SARCK) */
}TP_InitTypeDef;

/*pointer struct for touch panel*/
struct point {
	int x;
	int y;
};

int gp_tp_request(void);
int gp_tp_release(int handle);
int gp_tp_init(int handle, TP_InitTypeDef* initStruct);
int gp_tp_set_intEnState(int handle, int it_mask, int newState);
int gp_tp_get_intEnState(int handle, int it_mask, int* state);
int gp_tp_get_intState(int handle, int it_mask, int* state);
int gp_tp_get_xy(int handle, struct point *ptArray, int* num);
int gp_tp_startConvert(int handle);
int gp_tp_stopConvert(int handle);
int gp_tp_set_mode(int handle, int mode);

#endif
