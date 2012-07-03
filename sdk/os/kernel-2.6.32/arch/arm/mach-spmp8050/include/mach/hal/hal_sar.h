#ifndef __HAL_SAR_H__
#define __HAL_SAR_H__
#include <mach/common.h>	/*for data types*/

#define MODE_TP_AUTO	0
#define MODE_TP_MANUAL	1
#define MODE_AUX	2

#define SAR_INTPENDN	1
#define SAR_INTPENUP	2
#define SAR_INTPNL	4
#define SAR_INTAUX	8
#define SAR_INTALL (SAR_INTPENDN | SAR_INTPENUP | SAR_INTPNL | SAR_INTAUX)

typedef struct sar_init_s {
	UINT8	clk_tog_en;	/*1-always toggling clock,0-toggling only in measurement*/
	UINT8	conv_dly;
	UINT8	chkdly;
	UINT8	x2y_dly;
	UINT16	interval_dly;
	UINT16	debounce_dly;
	UINT32	clock_rate;
}sar_init_t;
/*export functions*/
void gpHalSarSetIntEn(UINT32 mask,UINT32 newstate);
UINT32 gpHalSarGetIntEn(UINT32 mask);
UINT32 gpHalSarGetIntFlag(UINT32 mask);
void gpHalSarClrIntFlag(UINT32 mask);
void gpHalSarStartConv(UINT32 mode,UINT32 arg);
void gpHalSarStopConv(UINT32 mode);
void gpHalSarInit(sar_init_t* psarInit);
void gpHalSarSetClkRate(UINT32 clk_rate);
UINT32 gpHalSarGetPNL(void);
UINT32 gpHalSarGetAUX(void);
#endif
