/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_user.c
*
* DESCRIPTION   :
*
* FUNCTION LIST
*                   msproal_user_start_timer
*                   msproal_user_check_timer
*                   msproal_user_end_timer
*                   msproal_user_get_base_adrs
*                   msproal_user_check_stick
*                   msproal_user_extract_int
*                   msproal_user_control_power
*                   msproal_user_change_clock
*                   msproal_user_icon_int
*                   msproal_user_msif_int
*                   msproal_user_check_int
*                   msproal_user_init_system
*                   msproal_user_start_dma
*                   msproal_user_end_dma
*                   msproal_user_dma_int
*                   msproal_user_set_flg
*                   msproal_user_clear_flg
*                   msproal_user_check_flg
*                   msproal_user_wait_flg
*                   msproal_user_wait_time
*                   msproal_user_write_mem32
*                   msproal_user_write_mem16
*                   msproal_user_read_mem32
*                   msproal_user_read_mem16
*                   msproal_user_write_mem8
*                   msproal_user_get_error_info
*                   msproal_user_flush_cache
*                   msproal_user_invalidate_cache
*                   msproal_user_virt_to_bus
*                   msproal_user_bus_to_virt
*                   msproal_user_memset
*                   msproal_user_memcpy
*                   msproal_user_sort
=============================================================================*/
#include <mach/ms/msproal.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>
#include <mach/ms/msproal_user.h>
#include <mach/hal/hal_ms.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/coda.h>
#include <mach/gp_timer.h>
#include <mach/diag.h>

volatile ULONG  eventflg;
ULONG           error_info[MSPROAL_SIZEOF_ERROR_INFO * 4];
SINT      timerID;
/******************************************************************************
*   FUNCTION    : msproal_user_start_timer
*   DESCRIPTION : Start timer.
*------------------------------------------------------------------------------
*   void msproal_user_start_timer(int time)
*   RETURN
*       None
*   ARGUMENT
*       time        : Time until Timeout(ms)
******************************************************************************/
void msproal_user_start_timer(int time)
{
    /* Timer0 init */
	//R_T0CTR = 0x412;
	//R_T0LDR = 65536-time-1;
	//R_T0CTR |= 0x1;
 	gp_tc_set_load(timerID,65536-time-1);
	gp_tc_enable(timerID);
    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_check_timer
*   DESCRIPTION : Check if timer timed out or not.
*------------------------------------------------------------------------------
*   int msproal_user_check_timer(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_TIMEOUT_ERR         : Timeout
*   ARGUMENT
*       None
******************************************************************************/
int msproal_user_check_timer(void)
{
	SINT iflag;
	
	gp_tc_get_int_state(timerID,&iflag);
	if (iflag & 0x1) {
		gp_tc_set_int_state(timerID,0);
		return MSPROAL_TIMEOUT_ERR;
	}
	
    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_end_timer
*   DESCRIPTION : End timer.
*------------------------------------------------------------------------------
*   void msproal_user_end_timer(void)
*   RETURN
*       None
*   ARGUMENT
*       None
******************************************************************************/
void msproal_user_end_timer(void)
{
	gp_tc_disable(timerID);
    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_get_base_adrs
*   DESCRIPTION : Get the base address of Host Controller.
*------------------------------------------------------------------------------
*   SINT msproal_user_get_base_adrs(ULONG *adrs)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       adrs        : The base address for Host Controller
******************************************************************************/
SINT msproal_user_get_base_adrs(ULONG *adrs)
{
    //*adrs = MS_ADRS_BASE;

    return MSPROAL_OK;
}

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_check_stick
*   DESCRIPTION : Check if Memory Stick is inserted or not.
*------------------------------------------------------------------------------
*   SINT msproal_user_check_stick(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_check_stick(void)
{
    ULONG   gpiodatareg;

    /* Get GPIO Data Register */
    gpiodatareg = *GPIO_DATA(GPIO3_ADRS_BASE);
    if(gpiodatareg & DATA_INS) {
        return MSPROAL_EXTRACT_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_check_stick
*   DESCRIPTION : Check if Memory Stick is inserted or not.
*------------------------------------------------------------------------------
*   SINT msproal_user_check_stick(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_check_stick(void)
{
    ULONG   hppreg;

    /* Get Parallel Port Register */
    hppreg = *PCOOL_PP_DATA(PCOOL_PPORT_ADRS_BASE);

    if(hppreg & PCOOL_PP_INS) {
        return MSPROAL_EXTRACT_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_extract_int
*   DESCRIPTION : Handles Extract interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_extract_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_extract_int(MSIFHNDL *msifhndl)
{
    msifhndl->IntState |= MSPROAL_FLG_EXTRACT;

    msproal_user_set_flg(MSPROAL_FLG_EXTRACT);

    return;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_extract_int
*   DESCRIPTION : Handles Extract interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_extract_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_extract_int(MSIFHNDL *msifhndl)
{
    UWORD   ictrlreg;

    msproal_user_read_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), &ictrlreg);
    ictrlreg &= ~ICON_CTRL_STRT;
    msproal_user_write_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), ictrlreg);

    msifhndl->IntState |= MSPROAL_FLG_EXTRACT;

    msproal_user_set_flg(MSPROAL_FLG_EXTRACT);

    return;
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_control_power
*   DESCRIPTION : Control the power supply of Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_user_control_power(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       mode            : Power ON or OFF
******************************************************************************/
SINT msproal_user_control_power(SINT mode)
{
    ULONG   gpiodatareg;

    /* Get GPIO Data Register */
    gpiodatareg = *GPIO_DATA(GPIO3_ADRS_BASE);
    if(MSPROAL_POWER_ON == mode) {
        gpiodatareg |= DATA_MSPWR;
    } else {
        gpiodatareg &= ~DATA_MSPWR;
    }

    *GPIO_DATA(GPIO3_ADRS_BASE) = gpiodatareg;

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_change_clock
*   DESCRIPTION : Change the frequency of serial clock(SCLK).
*------------------------------------------------------------------------------
*   void msproal_user_change_clock(SINT mode)
*   RETURN
*       None
*   ARGUMENT
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARARELL_MODE/MSPROAL_PRO_4PARARELL_MODE/
*                   MSPROAL_8PARALLEL_MODE)
******************************************************************************/
void msproal_user_change_clock(SINT mode)
{
    ULONG   gpiodatareg;

    /* Get GPIO Data Register */
    gpiodatareg = *GPIO_DATA(GPIO3_ADRS_BASE);
    gpiodatareg &= ~DATA_CLOCK_MASK;
    if(MSPROAL_SERIAL_MODE == mode) {
        gpiodatareg |= DATA_CLOCK_SERIAL;
    } else {
        gpiodatareg |= DATA_CLOCK_PARALLEL;
    }

    *GPIO_DATA(GPIO3_ADRS_BASE) = gpiodatareg;

    return;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_control_power
*   DESCRIPTION : Control the power supply of Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_user_control_power(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       mode            : Power ON or OFF
******************************************************************************/
SINT msproal_user_control_power(SINT mode)
{
    
    return MSPROAL_OK;
}
/******************************************************************************
*   FUNCTION    : msproal_user_change_clock
*   DESCRIPTION : Change the frequency of serial clock(SCLK).
*------------------------------------------------------------------------------
*   void msproal_user_change_clock(SINT mode)
*   RETURN
*       None
*   ARGUMENT
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARARELL_MODE/MSPROAL_PRO_4PARARELL_MODE/
*                   MSPROAL_8PARALLEL_MODE)
******************************************************************************/
void msproal_user_change_clock(SINT mode)
{
    UINT	div,ms_clk;
    
    unsigned long clk_rate = 0;
	struct clk *pclk;

	pclk = clk_get(NULL,"clk_sys_apb");

	if(pclk){
		clk_rate = clk_get_rate(pclk);	
		clk_put(pclk);
	}
	else{
		clk_rate = 37000000;
	}

    if(MSPROAL_SERIAL_MODE == mode) {
        ms_clk = 20000000;
    } else {
        ms_clk = 40000000;
    }
    
	div = (UINT) ((clk_rate+(ms_clk<<1)-1) / (ms_clk<<1)) - 1;
	
	gpHalMsChangeClock(div);
	
    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_set_clock
*   DESCRIPTION : Set the frequency of serial clock(SCLK).
*------------------------------------------------------------------------------
*   void msproal_user_set_clock(UINT freq)
*   RETURN
*       None
*   ARGUMENT
*       freq        : clock frequency
******************************************************************************/
void msproal_user_set_clock(UINT freq)
{
    UINT	div;
    unsigned long clk_rate = 0;
	struct clk *pclk;

	pclk = clk_get(NULL,"clk_sys_apb");

	if(pclk){
		clk_rate = clk_get_rate(pclk);	
		clk_put(pclk);
	}
	else{
		clk_rate = 37000000;
	}

	div = (UINT) ((clk_rate+(freq<<1)-1) / (freq<<1)) - 1;
	
	gpHalMsChangeClock(div);
	
    return;
}

#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_icon_int
*   DESCRIPTION : Handle icon interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_icon_int(void)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_icon_int(MSIFHNDL *msifhndl)
{
    UWORD   ictrlreg;

    /* Get Control Register */
    msproal_user_read_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), &ictrlreg);

    /* Interrupt from ICON */
    ictrlreg            |= ICON_CTRL_INTC;
    msproal_user_write_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), ictrlreg);

    msifhndl->IntState |= MSPROAL_FLG_ICON;

    msproal_user_set_flg(MSPROAL_FLG_ICON);

    return;
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_msif_int
*   DESCRIPTION : Handle Memory Stick interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_msif_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_msif_int(MSIFHNDL *msifhndl)
{
    UWORD   hsttsreg;

    /* Get Status Register */
    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);

    /* Clear interrupt occurred by Host Controller */
    msproal_user_write_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), hsttsreg);

    msifhndl->IntState |= hsttsreg;

    msproal_user_set_flg(hsttsreg);

    return;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_msif_int
*   DESCRIPTION : Handle Memory Stick interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_msif_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_msif_int(MSIFHNDL *msifhndl)
{
    #if 0
    UWORD   hsttsreg, hsysreg;

    /* Get Status Register */
    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);

    /* Clear interrupt occurred by Host Controller */
    
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    hsysreg |= MSIF_SYS_INTCLR;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

	
    msifhndl->IntState |= hsttsreg;

    msproal_user_set_flg(hsttsreg);
	#endif
    return;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_check_int
*   DESCRIPTION : Check if interrupt is occured from Host Controller or I-CON.
*------------------------------------------------------------------------------
*   void msproal_user_check_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_check_int(MSIFHNDL *msifhndl)
{
    UWORD   ictrlreg;

    /* Get Control Register */
    msproal_user_read_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), &ictrlreg);
    if(ictrlreg & ICON_CTRL_INT) {
        msproal_user_icon_int(msifhndl);
    } else {
        msproal_user_msif_int(msifhndl);
    }

    return;
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_check_int
*   DESCRIPTION : Check if interrupt is occured from Host Controller or I-CON.
*------------------------------------------------------------------------------
*   void msproal_user_check_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_check_int(MSIFHNDL *msifhndl)
{
    msproal_user_msif_int(msifhndl);

    return;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (1 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_user_init_system
*   DESCRIPTION : Initialize the System.
*------------------------------------------------------------------------------
*   SINT msproal_user_init_system(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_init_system(void)
{
    /* Initialize the GPIO */
    /* DIR7-5 Outpit, DIR4-0 Input */
    *GPIO_DIR_CTRL(GPIO3_ADRS_BASE) = DIR_CTRL_MSPWR | DIR_CTRL_CLOCK;
    *GPIO_DATA(GPIO3_ADRS_BASE)     = DATA_CLOCK_SERIAL;
    /* GPIO INT(XINT_GPIO) Enable */
    *GPIO_CTRL0(GPIO3_ADRS_BASE)    = CTRL0_INTEN;
    /* Rise Edge Disable */
    *GPIO_CTRL1(GPIO3_ADRS_BASE)    = CTRL_INS;
    /* FEN4 Fall Edge Enable */
    *GPIO_CTRL2(GPIO3_ADRS_BASE)    &= ~CTRL_INS;

    /* Initialize INTC and Enable MS & Timer interrupt & DMA interrupt*/
    *IEIS(INT_ADRS_BASE) |= (IEIS_ENABLE_MSIF
                            | IEIS_ENABLE_DMA1
                            | IEIS_ENABLE_GPIO);
    /* DMA  */
    *IPR2(INT_ADRS_BASE) = IPR2_DMA1_PRI11;
    /* MSIF & PPORT  */
    *IPR5(INT_ADRS_BASE) = (IPR5_MSIF_PRI12 | IPR5_GPIO_PRI12);
    /* Priority 10 < 12(maximum) */
    *ICPR(INT_ADRS_BASE) = ICPR_DEFAULT_PRI;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_IP)   */

#if         (2 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_user_init_system
*   DESCRIPTION : Initialize the System.
*------------------------------------------------------------------------------
*   SINT msproal_user_init_system(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_init_system(void)
{
    /* Initialize the GPIO */
    /* DIR7-5 Outpit, DIR4-0 Input */
    *GPIO_DIR_CTRL(GPIO3_ADRS_BASE) = DIR_CTRL_MSPWR | DIR_CTRL_CLOCK;
    *GPIO_DATA(GPIO3_ADRS_BASE)     = DATA_CLOCK_SERIAL;
    /* GPIO INT(XINT_GPIO) Enable */
    *GPIO_CTRL0(GPIO3_ADRS_BASE)    = CTRL0_INTEN;
    /* Rise Edge Disable */
    *GPIO_CTRL1(GPIO3_ADRS_BASE)    = CTRL_INS;
    /* FEN4 Fall Edge Enable */
    *GPIO_CTRL2(GPIO3_ADRS_BASE)    &= ~CTRL_INS;

#if         (2 == MSPROAL_DMA_CHANNELS)
    /* Initialize INTC and Enable MS & Timer interrupt & DMA interrupt */
    *IEIS(INT_ADRS_BASE) |= (IEIS_ENABLE_ICON
                            | IEIS_ENABLE_DMA2
                            | IEIS_ENABLE_GPIO);
    /* DMA  */
    *IPR2(INT_ADRS_BASE) = IPR2_DMA2_PRI11;
#else   /*  (2 == MSPROAL_DMA_CHANNELS) */
    /* Initialize INTC and Enable MS & Timer interrupt & DMA interrupt*/
    *IEIS(INT_ADRS_BASE) |= (IEIS_ENABLE_ICON
                            | IEIS_ENABLE_DMA1
                            | IEIS_ENABLE_GPIO);
    /* DMA  */
    *IPR2(INT_ADRS_BASE) = IPR2_DMA1_PRI11;
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */
    /* ICON & PPORT  */
    *IPR5(INT_ADRS_BASE) = (IPR5_ICON_PRI12 | IPR5_GPIO_PRI12);
    /* Priority 10 < 12(maximum) */
    *ICPR(INT_ADRS_BASE) = ICPR_DEFAULT_PRI;

    return MSPROAL_OK;
}
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (3 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_user_init_system
*   DESCRIPTION : Initialize the System.
*------------------------------------------------------------------------------
*   SINT msproal_user_init_system(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_init_system(void)
{
    UINT apbHz;
	struct clk *apbClk;
	UINT temp,i;
	
	//get the apb clock value
	apbClk = clk_get(NULL,"clk_arm_apb");
	if(apbClk){
		apbHz = clk_get_rate(apbClk);
		clk_put(apbClk);
	}
	else{
		apbHz = 21000000;
		DIAG_INFO("apb use default clock 21M\n");
	}
	
	temp = apbHz/1000 - 1 ;
	
	timerID = 0;
	for (i=0;i<5 && timerID==0;i++) {
		timerID = gp_tc_request(i,"mscard");
	}
	
	gp_tc_set_prescale(timerID,temp);
	gp_tc_set_operation_mode(timerID,1); /* period timer mode */
	gp_tc_set_count_mode(timerID,1);
	gp_tc_enable_int(timerID,1);
	
    gpHalMsInit();
    
    return MSPROAL_OK;
}

void msproal_user_uninit_system(void)
{
	gp_tc_release(timerID);
}
#endif  /*  (3 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_user_init_system
*   DESCRIPTION : Initialize the System.
*------------------------------------------------------------------------------
*   SINT msproal_user_init_system(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_init_system(void)
{
    /* Initialize the Parallel Port */
    *PCOOL_PP_CTRL(PCOOL_PPORT_ADRS_BASE)   = (PCOOL_PP_PIEN
                                            | PCOOL_PP_INTEN
                                            | PCOOL_PP_POEN);
    *PCOOL_PP_DATA(PCOOL_PPORT_ADRS_BASE)   &= PCOOL_PP_ENABLE_ICON;

#if         (2 == MSPROAL_DMA_CHANNELS)
    /* Initialize INTC and Enable MS & Timer interrupt & DMA interrupt*/
    *IEIS(INT_ADRS_BASE) |= (IEIS_ENABLE_ICON
                            | IEIS_ENABLE_DMA2
                            | IEIS_ENABLE_PPORT);
    /* DMA  */
    *IPR2(INT_ADRS_BASE) = IPR2_DMA2_PRI11;
#else   /*  (2 == MSPROAL_DMA_CHANNELS) */
    /* Initialize INTC and Enable MS & Timer interrupt & DMA interrupt*/
    *IEIS(INT_ADRS_BASE) |= (IEIS_ENABLE_ICON
                            | IEIS_ENABLE_DMA1
                            | IEIS_ENABLE_PPORT);
    /* DMA  */
    *IPR2(INT_ADRS_BASE) = IPR2_DMA1_PRI11;
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */
    /* ICON & PPORT  */
    *IPR5(INT_ADRS_BASE) = (IPR5_ICON_PRI12 | IPR5_PPORT_PRI12);
    /* Priority 10 < 12(maximum) */
    *ICPR(INT_ADRS_BASE) = ICPR_DEFAULT_PRI;

    return MSPROAL_OK;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_user_init_system
*   DESCRIPTION : Initialize the System.
*------------------------------------------------------------------------------
*   SINT msproal_user_init_system(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_init_system(void)
{
    /* Initialize the GPIO */
    /* DIR7-5 Outpit, DIR4-0 Input */
    *GPIO_DIR_CTRL(GPIO3_ADRS_BASE) = DIR_CTRL_MSPWR | DIR_CTRL_CLOCK;
    *GPIO_DATA(GPIO3_ADRS_BASE)     = DATA_CLOCK_SERIAL;
    /* GPIO INT(XINT_GPIO) Enable */
    *GPIO_CTRL0(GPIO3_ADRS_BASE)    = CTRL0_INTEN;
    /* Rise Edge Disable */
    *GPIO_CTRL1(GPIO3_ADRS_BASE)    = CTRL_INS;
    /* FEN4 Fall Edge Enable */
    *GPIO_CTRL2(GPIO3_ADRS_BASE)    &= ~CTRL_INS;

    /* Initialize INTC and Enable MS & Timer interrupt & DMA interrupt */
    *IEIS(INT_ADRS_BASE) |= (IEIS_ENABLE_ICON | IEIS_ENABLE_GPIO);
    /* ICON & PPORT  */
    *IPR5(INT_ADRS_BASE) = (IPR5_ICON_PRI12 | IPR5_GPIO_PRI12);
    /* Priority 10 < 12(maximum) */
    *ICPR(INT_ADRS_BASE) = ICPR_DEFAULT_PRI;

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (1 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_DMA)
/******************************************************************************
*   FUNCTION    : msproal_user_start_dma
*   DESCRIPTION : Start DMA.
*------------------------------------------------------------------------------
*   SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
*           SINT select)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Abnormal
*   ARGUMENT
*       dinc        : Destination address increment flag
*       src         : DMA source address
*       dst         : DMA destination address
*       size        : DMA transfer size
*       select      : DMA target select
******************************************************************************/
SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
        SINT select)
{
    *DMACS0(DMAC_ADRS_BASE) = DMACS_CLEAR;  /* Disable DMA                  */
    *SADR0(DMAC_ADRS_BASE)  = (ULONG)src;   /* Set DMA Source Address       */
    *DADR0(DMAC_ADRS_BASE)  = (ULONG)dst;   /* Set DMA Destination Address  */
    *TCNT0(DMAC_ADRS_BASE)  = (ULONG)(size >> 2); /* TCNT = size/4byte      */

    if(MSPROAL_SELECT_DATA == select) {
        if(MSPROAL_INC_DADR == dinc) {
            *DMACS0(DMAC_ADRS_BASE) = DMACS_RS_IRQ0
                                    | DMACS_DAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        } else {
            *DMACS0(DMAC_ADRS_BASE) = DMACS_RS_IRQ0
                                    | DMACS_SAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        }
    } else {
        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_check_dma_int
*   DESCRIPTION : DMA is checked.
*------------------------------------------------------------------------------
*   SINT msproal_user_check_dma_int(void)
*   RETURN
*       MSPROAL_OK                 : Normal
*       MSPROAL_ERR                : Abnormal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_check_dma_int(void)
{
    /* check if interrupt occured from dma */
    if(*DMACS0(DMAC_ADRS_BASE) & DMACS_IF) {
        return MSPROAL_OK;
    }

    return MSPROAL_ERR;
}

/******************************************************************************
*   FUNCTION    : msproal_user_end_dma
*   DESCRIPTION : End DMA.
*------------------------------------------------------------------------------
*   void msproal_user_end_dma(void)
*   RETURN
*       None
*   ARGUMENT
*       None
******************************************************************************/
void msproal_user_end_dma(void)
{
    *DMACS0(DMAC_ADRS_BASE) = DMACS_CLEAR;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_dma_int
*   DESCRIPTION : Handle Memory Stick interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_dma_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_dma_int(MSIFHNDL *msifhndl)
{
    *DMACS0(DMAC_ADRS_BASE) = DMACS_CLEAR;

    msproal_user_set_flg(MSPROAL_FLG_DMA);

    return;
}
#endif  /*  (1 == MSPROAL_SUPPORT_DMA)  */
#endif  /*  (1 == MSPROAL_SUPPORT_IP)   */

#if         (3 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_DMA)
/******************************************************************************
*   FUNCTION    : msproal_user_start_dma
*   DESCRIPTION : Start DMA.
*------------------------------------------------------------------------------
*   SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
*           SINT select)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Abnormal
*   ARGUMENT
*       dinc        : Destination address increment flag
*       src         : DMA source address
*       dst         : DMA destination address
*       size        : DMA transfer size
*       select      : DMA target select
******************************************************************************/
SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
        SINT select)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;  /* Disable DMA                  */
    *SADR1(DMAC_ADRS_BASE)  = (ULONG)src;   /* Set DMA Source Address       */
    *DADR1(DMAC_ADRS_BASE)  = (ULONG)dst;   /* Set DMA Destination Address  */
    *TCNT1(DMAC_ADRS_BASE)  = (ULONG)(size >> 2); /* TCNT = size/4byte      */

    if(MSPROAL_SELECT_DATA == select) {
        if(MSPROAL_INC_DADR == dinc) {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_DAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        } else {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_SAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        }
    } else {
        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_check_dma_int
*   DESCRIPTION : DMA is checked.
*------------------------------------------------------------------------------
*   SINT msproal_user_check_dma_int(void)
*   RETURN
*       MSPROAL_OK                 : Normal
*       MSPROAL_ERR                : Abnormal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_user_check_dma_int(void)
{
    /* check if interrupt occured from dma */
    if(*DMACS1(DMAC_ADRS_BASE) & DMACS_IF) {
        return MSPROAL_OK;
    }

    return MSPROAL_ERR;
}

/******************************************************************************
*   FUNCTION    : msproal_user_end_dma
*   DESCRIPTION : End DMA.
*------------------------------------------------------------------------------
*   void msproal_user_end_dma(void)
*   RETURN
*       None
*   ARGUMENT
*       None
******************************************************************************/
void msproal_user_end_dma(void)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_dma_int
*   DESCRIPTION : Handle Memory Stick interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_dma_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_dma_int(MSIFHNDL *msifhndl)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;

    msproal_user_set_flg(MSPROAL_FLG_DMA);

    return;
}
#endif  /*  (1 == MSPROAL_SUPPORT_DMA)  */
#endif  /*  (3 == MSPROAL_SUPPORT_IP)   */

#if         ((2 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
#if         (2 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_user_start_dma
*   DESCRIPTION : Start DMA.
*------------------------------------------------------------------------------
*   SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
*           SINT select)
*   RETURN
*       MSPROAL_OK              : Normal
*       MSPROAL_PARAM_ERR       : Parameter error
*   ARGUMENT
*       dinc        : Destination address increment flag
*       src         : DMA source address
*       dst         : DMA destination address
*       size        : DMA transfer size
*       select      : DMA target select
******************************************************************************/
SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
        SINT select)
{
    if(MSPROAL_SELECT_GDFIFO == select) {
        /* DMA: GDFIFO */
        *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;  /* Disable DMA */
        *SADR1(DMAC_ADRS_BASE)  = (ULONG)src;   /* Set DMA Source Address */
        *DADR1(DMAC_ADRS_BASE)  = (ULONG)dst;   /* Set DMA Destination Address */
        *TCNT1(DMAC_ADRS_BASE)  = (ULONG)(size >> 2);    /* size / 4byte */
        if(MSPROAL_INC_DADR == dinc) {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_DAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        } else {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_SAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        }
    } else if(MSPROAL_SELECT_PBUFF == select) {
        /* DMA: PAGE_BUFFER */
        *DMACS2(DMAC_ADRS_BASE) = DMACS_CLEAR;  /* Disable DMA */
        *SADR2(DMAC_ADRS_BASE)  = (ULONG)src;   /* Set DMA Source Address */
        *DADR2(DMAC_ADRS_BASE)  = (ULONG)dst;   /* Set DMA Destination Address */
        *TCNT2(DMAC_ADRS_BASE)  = (ULONG)(size >> 4);    /* size / 16byte */
        if(MSPROAL_INC_DADR == dinc) {
            *DMACS2(DMAC_ADRS_BASE) = DMACS_RS_IRQ2
                                    | DMACS_DAR_INC
                                    | DMACS_TSZ_16BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        } else {
            *DMACS2(DMAC_ADRS_BASE) = DMACS_RS_IRQ2
                                    | DMACS_SAR_INC
                                    | DMACS_TSZ_16BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        }
    } else {
        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_end_dma
*   DESCRIPTION : End DMA.
*------------------------------------------------------------------------------
*   void msproal_user_end_dma(void)
*   RETURN
*       None
*   ARGUMENT
*       None
******************************************************************************/
void msproal_user_end_dma(void)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;
    *DMACS2(DMAC_ADRS_BASE) = DMACS_CLEAR;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_dma_int
*   DESCRIPTION : Handle Memory Stick interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_dma_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_dma_int(MSIFHNDL *msifhndl)
{
    *DMACS2(DMAC_ADRS_BASE) = DMACS_CLEAR;

    msproal_user_set_flg(MSPROAL_FLG_DMA);

    return;
}
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */

#if         (1 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_user_start_dma
*   DESCRIPTION : Start DMA.
*------------------------------------------------------------------------------
*   SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
*           SINT select)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Abnormal
*   ARGUMENT
*       dinc        : Destination address increment flag
*       src         : DMA source address
*       dst         : DMA destination address
*       size        : DMA transfer size
*       select      : DMA target select
******************************************************************************/
SINT msproal_user_start_dma(SINT dinc, void *src, void *dst, SINT size,
        SINT select)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;  /* Disable DMA                  */
    *SADR1(DMAC_ADRS_BASE)  = (ULONG)src;   /* Set DMA Source Address       */
    *DADR1(DMAC_ADRS_BASE)  = (ULONG)dst;   /* Set DMA Destination Address  */

    if(MSPROAL_SELECT_GDFIFO == select) {
        /* DMA: GDFIFO */
        *TCNT1(DMAC_ADRS_BASE) = (ULONG)(size >> 2);    /* size / 4byte */
        if(MSPROAL_INC_DADR == dinc) {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_DAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        } else {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_SAR_INC
                                    | DMACS_TSZ_4BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        }
    } else if(MSPROAL_SELECT_PBUFF == select) {
        /* DMA: PAGE_BUFFER */
        *TCNT1(DMAC_ADRS_BASE) = (ULONG)(size >> 4);    /* size / 16byte */
        if(MSPROAL_INC_DADR == dinc) {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_DAR_INC
                                    | DMACS_TSZ_16BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        } else {
            *DMACS1(DMAC_ADRS_BASE) = DMACS_RS_IRQ1
                                    | DMACS_SAR_INC
                                    | DMACS_TSZ_16BYTE
                                    | DMACS_RFCT
                                    | DMACS_CHAE;
        }
    } else {
        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_end_dma
*   DESCRIPTION : End DMA.
*------------------------------------------------------------------------------
*   void msproal_user_end_dma(void)
*   RETURN
*       None
*   ARGUMENT
*       None
******************************************************************************/
void msproal_user_end_dma(void)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_dma_int
*   DESCRIPTION : Handle Memory Stick interrupt.
*------------------------------------------------------------------------------
*   void msproal_user_dma_int(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_user_dma_int(MSIFHNDL *msifhndl)
{
    *DMACS1(DMAC_ADRS_BASE) = DMACS_CLEAR;

    msproal_user_set_flg(MSPROAL_FLG_DMA);

    return;
}
#endif  /*  (1 == MSPROAL_DMA_CHANNELS) */
#endif  /*  ((2 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

/******************************************************************************
*   FUNCTION    : msproal_user_set_flg
*   DESCRIPTION : Set the bit pattern of interrupt factors to the flag.
*------------------------------------------------------------------------------
*   void msproal_user_set_flg(ULONG ptn)
*   RETURN
*       None
*   ARGUMENT
*       ptn         : Bit pattern of interrupt factors
******************************************************************************/
void msproal_user_set_flg(ULONG ptn)
{
    eventflg |= ptn;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_clear_flg
*   DESCRIPTION : Clear the bit pattern of interrupt factors.
*------------------------------------------------------------------------------
*   void msproal_user_clear_flg(ULONG ptn)
*   RETURN
*       None
*   ARGUMENT
*       ptn         : Bit pattern of interrupt factors
******************************************************************************/
void msproal_user_clear_flg(ULONG ptn)
{
    eventflg &= ptn;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_check_flg
*   DESCRIPTION : Check if at least one bit in the bit pattern
*               of interrupt factors is set or not.
*------------------------------------------------------------------------------
*   SINT msproal_user_check_flg(ULONG ptn)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_ERR                 : Abnormal
*   ARGUMENT
*       ptn         : Bit pattern of interrupt factors
******************************************************************************/
SINT msproal_user_check_flg(ULONG ptn)
{
    if(eventflg & ptn) {
        return MSPROAL_OK;
    } else {
        return MSPROAL_ERR;
    }
}

/******************************************************************************
*   FUNCTION    : msproal_user_wait_flg
*   DESCRIPTION : Wait until at least one bit in the bit pattern
*               of interrupt factors is set within specified time.
*------------------------------------------------------------------------------
*   SINT msproal_user_wait_flg(ULONG ptn, SINT time)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_TIMEOUT_ERR         : Timeout
*   ARGUMENT
*       ptn         : Bit pattern of interrupt factors
*       time        : Time until Timeout(ms)
******************************************************************************/
SINT msproal_user_wait_flg(ULONG ptn, SINT time)
{
    SINT    result;

    /* Start timer */
    msproal_user_start_timer(time);

    while(!(eventflg & ptn)) {
        /* Did interrupt occur by timer? */
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_TIMEOUT_ERR;
        }
    }

    /* End timer */
    msproal_user_end_timer();

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_user_wait_time
*   DESCRIPTION : Wait during the specified time.
*------------------------------------------------------------------------------
*   void msproal_user_wait_time(SINT time)
*   RETURN
*       None
*   ARGUMENT
*       time        : Time until Timeout(ms)
******************************************************************************/
void msproal_user_wait_time(SINT time)
{
    SINT    result;

    if(0 == time) {
        return;
    }

    /* Start timer */
    msproal_user_start_timer(time);

    /* Loop is executed repeatedly until Timeout */
    while(MSPROAL_OK == (result = msproal_user_check_timer())) {
        ;
    }

    return;
}

#if         (0 == MSPROAL_DIRECT_ACCESS)
/******************************************************************************
*   FUNCTION    : msproal_user_write_mem32
*   DESCRIPTION : Write 4 bytes data to the specified address.
*------------------------------------------------------------------------------
*   void msproal_user_write_mem32(ULONG adrs, ULONG data)
*   RETURN
*       None
*   ARGUMENT
*       adrs        : Address to area where data is written
*       data        : Data to write
******************************************************************************/
void msproal_user_write_mem32(ULONG adrs, ULONG data)
{
    *(volatile ULONG *)adrs = data;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_write_mem16
*   DESCRIPTION : Write 2 bytes data to the specified address.
*------------------------------------------------------------------------------
*   void msproal_user_write_mem16(ULONG adrs, UWORD data)
*   RETURN
*       None
*   ARGUMENT
*       adrs        : Address to area where data is written
*       data        : Data to write
******************************************************************************/
void msproal_user_write_mem16(ULONG adrs, UWORD data)
{
    *(volatile UWORD *)adrs = data;

    return;
}

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_user_write_mem8
*   DESCRIPTION : Write 1 byte data to the specified address.
*------------------------------------------------------------------------------
*   void msproal_user_write_mem8(ULONG adrs, UBYTE data)
*   RETURN
*       None
*   ARGUMENT
*       adrs        : Address to area where data is written
*       data        : Data to write
******************************************************************************/
void msproal_user_write_mem8(ULONG adrs, UBYTE data)
{
    *(volatile UBYTE *)adrs = data;

    return;
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

/******************************************************************************
*   FUNCTION    : msproal_user_read_mem32
*   DESCRIPTION : Read 4 bytes data from the specified address
*               and store it to the specified address.
*------------------------------------------------------------------------------
*   void msproal_user_read_mem32(ULONG adrs, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       adrs        : Address to area where data is read
*       data        : Address to area where data is stored
******************************************************************************/
void msproal_user_read_mem32(ULONG adrs, ULONG *data)
{
    *data = *(volatile ULONG *)adrs;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_read_mem16
*   DESCRIPTION : Read 2 bytes data from the specified address
*               and store it to the specified address.
*------------------------------------------------------------------------------
*   void msproal_user_read_mem16(ULONG adrs, UWORD *data)
*   RETURN
*       None
*   ARGUMENT
*       adrs        : Address to area where data is read
*       data        : Address to area where data is stored
******************************************************************************/
void msproal_user_read_mem16(ULONG adrs, UWORD *data)
{
    *data = *(volatile UWORD *)adrs;

    return;
}
#endif  /*  (0 == MSPROAL_DIRECT_ACCESS)    */

/******************************************************************************
*   FUNCTION    : msproal_user_get_error_info
*   DESCRIPTION : Allocates the buffer to store error info.
*------------------------------------------------------------------------------
*   void msproal_user_get_error_info(ULONG **data, UWORD *num)
*   RETURN
*       None
*   ARGUMENT
*       data        : Buffer to store error info
*       num         : Number to be able to store error infos
******************************************************************************/
void msproal_user_get_error_info(ULONG **data, UWORD *num)
{
    *data   = error_info;
    *num    = sizeof(error_info) / MSPROAL_SIZEOF_ERROR_INFO;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_flush_cache
*   DESCRIPTION : .
*------------------------------------------------------------------------------
*   void msproal_user_flush_cache(void *adrs, ULONG size)
*   RETURN
*       None
*   ARGUMENT
*       adrs        :
*       size        :
******************************************************************************/
void msproal_user_flush_cache(void *adrs, ULONG size)
{
    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_invalidate_cache
*   DESCRIPTION : .
*------------------------------------------------------------------------------
*   void msproal_user_invalidate_cache(void *adrs, ULONG size)
*   RETURN
*       None
*   ARGUMENT
*       adrs        :
*       size        :
******************************************************************************/
void msproal_user_invalidate_cache(void *adrs, ULONG size)
{
    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_virt_to_bus
*   DESCRIPTION : .
*------------------------------------------------------------------------------
*   void msproal_user_virt_to_bus(void *vadrs, ULONG *padrs)
*   RETURN
*       None
*   ARGUMENT
*       vadrs       :
*       padrs       :
******************************************************************************/
void msproal_user_virt_to_bus(void *vadrs, ULONG *padrs)
{
    *padrs = (ULONG)vadrs;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_bus_to_virt
*   DESCRIPTION : .
*------------------------------------------------------------------------------
*   void msproal_user_bus_to_virt(ULONG padrs, void **vadrs)
*   RETURN
*       None
*   ARGUMENT
*       padrs       :
*       vadrs       :
******************************************************************************/
void msproal_user_bus_to_virt(ULONG padrs, void **vadrs)
{
    *vadrs = (void *)padrs;

    return;
}

/******************************************************************************
*   FUNCTION    : msproal_user_memset
*   DESCRIPTION : Like the standard memset() function, sets the first (count)
*                 bytes in memory area (data) to the value of (val).
*------------------------------------------------------------------------------
*   void msproal_user_memset(UBYTE *data, UBYTE val, SINT count)
*   RETURN
*       None
*   ARGUMENT
*       data        : pointer to memory to fill
*       val         : value to put
*       count       : number of bytes to fill
******************************************************************************/
void msproal_user_memset(UBYTE *data, UBYTE val, SINT count)
{
    while(count --) {
        *data ++ = val;
    }

    return ;
}

/******************************************************************************
*   FUNCTION    : msproal_user_memcpy
*   DESCRIPTION : Like the standard memcpy() function, copies (count) bytes
*               from memory area (srcdata) to (dstdata).
*------------------------------------------------------------------------------
*   void msproal_user_memcpy(UBYTE *dstdata, UBYTE *srcdata, SINT count)
*   RETURN
*       None
*   ARGUMENT
*       dstdata     : pointer to destination data
*       srcdata     : pointer to source data
*       count       : number of bytes to copy
******************************************************************************/
void msproal_user_memcpy(UBYTE *dstdata, UBYTE *srcdata, SINT count)
{
    while(count --) {
        *dstdata ++ = *srcdata ++;
    }

    return;
}

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_user_sort
*   DESCRIPTION : Ascending sort.
*------------------------------------------------------------------------------
*   void msproal_user_sort(UWORD *base, SINT sort_cnt)
*   RETURN
*       None
*   ARGUMENT
*       base        : Start address of sort array
*       sort_cnt    : Data number to sort
******************************************************************************/
void msproal_user_sort(UWORD *base, SINT sort_cnt)
{
    UWORD   base_data, next_data;
    SINT    set_cnt, array_cnt, set, array;

    set = sort_cnt - 1;
    for(set_cnt = 0; set_cnt < set; set_cnt ++) {
        array = set - set_cnt;
        /* Largest data is stored in the last of array */
        for(array_cnt = 0; array_cnt < array; array_cnt ++) {
            base_data = base[array_cnt];
            next_data = base[array_cnt + 1];

            if(base_data > next_data) {
                next_data           = base_data;
                base[array_cnt]     = base[array_cnt + 1];
                base[array_cnt + 1] = next_data;
            }
        }
    }

    return ;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
