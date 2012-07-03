/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_msif.c
*
* DESCRIPTION   : MS API
*
* FUNCTION LIST
*                   msproal_msif_change_ifmode
*                   msproal_msif_control_power
*                   msproal_msif_ex_set_cmd
*                   msproal_msif_get_ifmode
*                   msproal_msif_get_int
*                   msproal_msif_read_reg
*                   msproal_msif_reset_host
*                   msproal_msif_set_cmd
*                   msproal_msif_set_para_extra
*                   msproal_msif_write_reg
=============================================================================*/
#include <mach/ms/msproal_common.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>
#include <mach/hal/hal_ms.h>


#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_change_ifmode
*   DESCRIPTION : Change the interface mode of Memory Stick side
*               and Host Controller side.
*------------------------------------------------------------------------------
*   SINT msproal_msif_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARALLEL_MODE/MSPROAL_PRO_4PARALLEL_MODE/
*                   MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_msif_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result;
    SINT    ifmode;
    UBYTE   sys_para;

    switch(mode) {
    case MSPROAL_SERIAL_MODE:
        ifmode      = mode;
        sys_para    = MS_SYSPARA_BAMD;  /* == MS2_SYSPARA_SRC */
        break;
    case MSPROAL_V1_4PARALLEL_MODE:
        ifmode      = MSPROAL_4PARALLEL_MODE;
        sys_para    = (MS_SYSPARA_BAMD | MS_SYSPARA_PAM);
        break;
    case MSPROAL_PRO_4PARALLEL_MODE:
        ifmode      = MSPROAL_4PARALLEL_MODE;
        sys_para    = 0x00;
        break;
    default:
        ifmode      = mode;
        sys_para    = 0x40;
        break;
    }

    result = msproal_msif_write_reg(msifhndl,
                                    MS_SYSTEM_PARAM_ADRS,
                                    1,
                                    &sys_para);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_tpc_change_ifmode(msifhndl, mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl->IfMode = ifmode;

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_change_ifmode
*   DESCRIPTION : Change the interface mode of Memory Stick side
*               and Host Controller side.
*------------------------------------------------------------------------------
*   SINT msproal_msif_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARALLEL_MODE/MSPROAL_PRO_4PARALLEL_MODE)
******************************************************************************/
SINT msproal_msif_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result;
    SINT    ifmode;
    UBYTE   sys_para;

    switch(mode) {
    case MSPROAL_SERIAL_MODE:
        ifmode      = mode;
        sys_para    = MS_SYSPARA_BAMD;  /* == MS2_SYSPARA_SRC */
        break;
    case MSPROAL_V1_4PARALLEL_MODE:
        ifmode      = MSPROAL_4PARALLEL_MODE;
        sys_para    = (MS_SYSPARA_BAMD | MS_SYSPARA_PAM);
        break;
    default:  /* MSPROAL_PRO_4PARALLEL_MODE */
        ifmode      = MSPROAL_4PARALLEL_MODE;
        sys_para    = 0x00;
        break;
    }

    result = msproal_msif_write_reg(msifhndl,
                                    MS_SYSTEM_PARAM_ADRS,
                                    1,
                                    &sys_para);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_tpc_change_ifmode(msifhndl, mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl->IfMode = ifmode;

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

/******************************************************************************
*   FUNCTION    : msproal_msif_control_power
*   DESCRIPTION : Control Memory Stick power supply.
*------------------------------------------------------------------------------
*   SINT msproal_msif_control_power(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Mode(MSPROAL_POWER_ON/MSPROAL_POWER_OFF)
******************************************************************************/
SINT msproal_msif_control_power(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result, time;

    result = msproal_user_control_power(mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MSPROAL_POWER_ON == mode) {
        time = MSPROAL_ACCESS_TIME;
    } else {
        time = MSPROAL_RESET_TIME;
    }

    msproal_user_wait_time(time);

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_msif_ex_set_cmd
*   DESCRIPTION : Execute the EX_SET_CMD command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_msif_ex_set_cmd(MSIFHNDL *msifhndl, SINT cmd, ULONG adrs,
*           UWORD count, SINT time)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_ERR           : FlashError(Read,Write,Erase)
*       MSPROAL_CMDNK_ERR           : Command nack
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       cmd         : Command
*       adrs        : Data Address Register
*       count       : Data Count Register
*       time        : Time until Timeout(ms)
******************************************************************************/
SINT msproal_msif_ex_set_cmd(MSIFHNDL *msifhndl, SINT cmd, ULONG adrs,
        UWORD count, SINT time)
{
    SINT    result;
    UBYTE   dummy_intreg;

    /* Issuing of EX_SET_CMD TPC */
    result = msproal_tpc_ex_set_cmd(msifhndl, cmd, adrs, count);
    if(MSPROAL_OK != result) {
        return result;
    }

    return msproal_msif_get_int(msifhndl, time, &dummy_intreg);
}

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_get_ifmode
*   DESCRIPTION : Get the interface mode of Memory Stick side.
*------------------------------------------------------------------------------
*   SINT msproal_msif_get_ifmode(MSIFHNDL *msifhndl, SINT *mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_IFMODE_WARN         : Interface mode warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Area to store the Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE/MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_msif_get_ifmode(MSIFHNDL *msifhndl, SINT *mode)
{
    SINT    result;
    SINT    clk_mode;
    UBYTE   intreg;

    result = msproal_tpc_get_int(msifhndl, &intreg);
    if(MSPROAL_ACCESS_ERR != result) {
        if(MSPROAL_OK == result) {
            *mode = msifhndl->IfMode;
            return MSPROAL_OK;
        }

        return result;
    }

#if         (1 != MSPROAL_SUPPORT_IFMODE)
#if         ((1 == MSPROAL_SUPPORT_PROHG) && (3 == MSPROAL_SUPPORT_IFMODE))
    if(msifhndl->Stick & MSPROAL_STICK_S8P_COM) {
        clk_mode    = MSPROAL_8PARALLEL_MODE;
        result = msproal_tpc_change_ifmode(msifhndl, clk_mode);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_tpc_get_int(msifhndl, &intreg);
        if(MSPROAL_ACCESS_ERR != result) {
            if(MSPROAL_OK == result) {
                *mode = MSPROAL_8PARALLEL_MODE;
                return MSPROAL_IFMODE_WARN;
            }

            return result;
        }
    }
#endif  /*  ((1 == MSPROAL_SUPPORT_PROHG) && (3 == MSPROAL_SUPPORT_IFMODE)) */

    if(msifhndl->Stick & MSPROAL_STICK_S4P) {
        if(msifhndl->Stick & MSPROAL_STICK_V1) {
            clk_mode    = MSPROAL_V1_4PARALLEL_MODE;
        } else {
            clk_mode    = MSPROAL_PRO_4PARALLEL_MODE;
        }
        result = msproal_tpc_change_ifmode(msifhndl, clk_mode);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_tpc_get_int(msifhndl, &intreg);
        if(MSPROAL_ACCESS_ERR != result) {
            if(MSPROAL_OK == result) {
                *mode = MSPROAL_4PARALLEL_MODE;
                return MSPROAL_IFMODE_WARN;
            }

            return result;
        }
    }
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE) */

    clk_mode    = MSPROAL_SERIAL_MODE;
    result = msproal_tpc_change_ifmode(msifhndl, clk_mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_tpc_get_int(msifhndl, &intreg);
    if(MSPROAL_ACCESS_ERR != result) {
        if(MSPROAL_OK == result) {
            *mode = MSPROAL_SERIAL_MODE;
            return MSPROAL_IFMODE_WARN;
        }

        return result;
    }

    return MSPROAL_SYSTEM_ERR;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_get_ifmode
*   DESCRIPTION : Get the interface mode of Memory Stick side.
*------------------------------------------------------------------------------
*   SINT msproal_msif_get_ifmode(MSIFHNDL *msifhndl, SINT *mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_IFMODE_WARN         : Interface mode warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Area to store the Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE)
******************************************************************************/
SINT msproal_msif_get_ifmode(MSIFHNDL *msifhndl, SINT *mode)
{
    SINT    result;
    SINT    clk_mode;
    UBYTE   intreg;

    result = msproal_tpc_get_int(msifhndl, &intreg);
    if(MSPROAL_ACCESS_ERR != result) {
        if(MSPROAL_OK == result) {
            *mode = msifhndl->IfMode;
            return MSPROAL_OK;
        }

        return result;
    }

#if         (1 != MSPROAL_SUPPORT_IFMODE)
    if(msifhndl->Stick & MSPROAL_STICK_S4P) {
        if(msifhndl->Stick & MSPROAL_STICK_V1) {
            clk_mode    = MSPROAL_V1_4PARALLEL_MODE;
        } else {
            clk_mode    = MSPROAL_PRO_4PARALLEL_MODE;
        }
        result = msproal_tpc_change_ifmode(msifhndl, clk_mode);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_tpc_get_int(msifhndl, &intreg);
        if(MSPROAL_ACCESS_ERR != result) {
            if(MSPROAL_OK == result) {
                *mode = MSPROAL_4PARALLEL_MODE;
                return MSPROAL_IFMODE_WARN;
            }

            return result;
        }
    }
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE) */

    clk_mode    = MSPROAL_SERIAL_MODE;
    result = msproal_tpc_change_ifmode(msifhndl, clk_mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_tpc_get_int(msifhndl, &intreg);
    if(MSPROAL_ACCESS_ERR != result) {
        if(MSPROAL_OK == result) {
            *mode = MSPROAL_SERIAL_MODE;
            return MSPROAL_IFMODE_WARN;
        }

        return result;
    }

    return MSPROAL_SYSTEM_ERR;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_get_int
*   DESCRIPTION : Execute the GET_INT command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_msif_get_int(MSIFHNDL *msifhndl, SINT time, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_ERR           : FlashError(Read,Write,Erase)
*       MSPROAL_CMDNK_ERR           : Command nack
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       time        : Time until Timeout(ms)
*       data        : INT Register
******************************************************************************/
SINT msproal_msif_get_int(MSIFHNDL *msifhndl, SINT time, UBYTE *data)
{
    SINT    result;
    UBYTE   intreg;

    /* Waiting for interrupt by MSINT */
    result = msproal_tpc_wait_int(msifhndl, time, MSPROAL_WMSINT);
    if(MSPROAL_OK != result) {
        if(MSPROAL_SYSTEM_ERR == result) {
            return MSPROAL_ACCESS_ERR;
        }

        return result;
    }

    /* Serial Mode ? */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        /* Issuing of GET_INT TPC */
        result = msproal_tpc_get_int(msifhndl, &intreg);
        if(MSPROAL_OK != result) {
            return result;
        }
    } else {
        intreg  = (UBYTE)msifhndl->IntState;
    }

    *data = intreg;
    /* ERR=1 or CMDNK=1? */
    if(intreg & (MS_INT_CMDNK | MS_INT_ERR)) {
        return (intreg & MS_INT_CMDNK) ? MSPROAL_CMDNK_ERR : MSPROAL_FLASH_ERR;
    }

    /* CED=1 or BREQ=1? */
    if(intreg & (MS_INT_CED | MS_INT_BREQ)) {
        return MSPROAL_OK;
    }

    return MSPROAL_SYSTEM_ERR;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_get_int
*   DESCRIPTION : Execute the GET_INT command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_msif_get_int(MSIFHNDL *msifhndl, SINT time, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_ERR           : FlashError(Read,Write,Erase)
*       MSPROAL_CMDNK_ERR           : Command nack
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       time        : Time until Timeout(ms)
*       data        : INT Register
******************************************************************************/
SINT msproal_msif_get_int(MSIFHNDL *msifhndl, SINT time, UBYTE *data)
{
    SINT    result;
    UWORD   hsttsreg;
    UBYTE   intreg;
	
    /* Waiting for interrupt by MSINT */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
    	result = msproal_tpc_wait_int(msifhndl, time, MSPROAL_WMSINT_SERIAL);
    }
    else {
    	result = msproal_tpc_wait_int(msifhndl, time, MSPROAL_WMSINT_PARALLEL);
    }
    
    if(MSPROAL_OK != result) {
        if(MSPROAL_SYSTEM_ERR == result) {
            return MSPROAL_ACCESS_ERR;
        }

        return result;
    }

    /* Serial Mode ? */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        /* Issuing of GET_INT TPC */
        result = msproal_tpc_get_int1(msifhndl, &intreg);
        if(MSPROAL_OK != result) {
            return result;
        }
    } else {
        hsttsreg    = (UWORD)gpHalMsGetStatus();
        intreg      = ((hsttsreg & MSINT_CED)>>3) 
                    | ((hsttsreg & MSINT_ERR)>>5) 
                    | ((hsttsreg & MSINT_BREQ)>>7) 
                    | ((hsttsreg & MSINT_CMDNK)>>13) ;
    }

    *data = intreg;
    /* ERR=1 or CMDNK=1? */
    if(intreg & (MS_INT_CMDNK | MS_INT_ERR)) {
        return (intreg & MS_INT_CMDNK) ? MSPROAL_CMDNK_ERR : MSPROAL_FLASH_ERR;
    }

    /* CED=1 or BREQ=1? */
    if(intreg & (MS_INT_CED | MS_INT_BREQ)) {
        return MSPROAL_OK;
    }
	
    return MSPROAL_SYSTEM_ERR;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

/******************************************************************************
*   FUNCTION    : msproal_msif_read_reg
*   DESCRIPTION : Set Address(adrs) and Data length(size) by SET_R/W_REG_ADRS,
*               and read Register of Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_msif_read_reg(MSIFHNDL *msifhndl, SINT adrs, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       adrs        : Starting address for READ_REG(READ_ADRS)
*       size        : Read data size for READ_REG(READ_SIZE)
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_msif_read_reg(MSIFHNDL *msifhndl, SINT adrs, SINT size,
        UBYTE *data)
{
    SINT    result;

    /* Is size less than 1, or larger than 256? */
    if(255 < (UINT)(size - 1)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Setting READ_ADRS and READ_SIZE */
    result = msproal_tpc_set_rw_reg_adrs(   msifhndl,
                                            adrs,
                                            size,
                                            MS_DEFAULT_WRITE_ADRS,
                                            MS_DEFAULT_WRITE_SIZE);
    if(MSPROAL_OK != result) {
        return result;
    }

    return msproal_tpc_read_reg(msifhndl, size, data);
}

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_reset_host
*   DESCRIPTION : Reset Host Controller.
*------------------------------------------------------------------------------
*   SINT msproal_msif_reset_host(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_msif_reset_host(MSIFHNDL *msifhndl)
{
    return msproal_tpc_reset_host(msifhndl);
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_msif_reset_host
*   DESCRIPTION : Reset Host Controller and I-CON.
*------------------------------------------------------------------------------
*   SINT msproal_msif_reset_host(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_msif_reset_host(MSIFHNDL *msifhndl)
{
    SINT    result;

    result = msproal_icon_reset_icon(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    return msproal_tpc_reset_host(msifhndl);
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

/******************************************************************************
*   FUNCTION    : msproal_msif_set_cmd
*   DESCRIPTION : Execute the SET_CMD command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_msif_set_cmd(MSIFHNDL *msifhndl, SINT cmd, SINT time)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_ERR           : FlashError(Read,Write,Erase)
*       MSPROAL_CMDNK_ERR           : Command nack
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       cmd         : Command
*       time        : Time until Timeout(ms)
******************************************************************************/
SINT msproal_msif_set_cmd(MSIFHNDL *msifhndl, SINT cmd, SINT time)
{
    SINT    result;
    UBYTE   intreg;

    /* Issuing of SET_CMD TPC */
    result = msproal_tpc_set_cmd(msifhndl, cmd);
    if(MSPROAL_OK != result) {
        return result;
    }
	
    if(MS_CMD_RESET != cmd) {
        return msproal_msif_get_int(msifhndl, time, &intreg);
    }

    return MSPROAL_OK;
}

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_msif_set_para_extra
*   DESCRIPTION : Write pblk, page and atype to the Parameter Register, and
*               write extradata to the ExtraData Register of the Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_msif_set_para_extra(MSIFHNDL *msifhndl, SINT pblk, SINT page,
*           SINT atype, UBYTE *extradata, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number
*       page        : Page number
*       atype       : Access type(MS_CMDPARA_SINGLE_MODE/MS_CMDPARA_BLOCK_MODE
*               /MS_CMDPARA_EXTRA_MODE/MS_CMDPARA_OVERWRITE_MODE)
*       extradata   : Address to area where extradata for write is stored
*                   (effective only to specify MSIF_WRITE)
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
*       mode        : MSPROAL_READ/MSPROAL_WRITE
******************************************************************************/
SINT msproal_msif_set_para_extra(MSIFHNDL *msifhndl, SINT pblk, SINT page,
        SINT atype, UBYTE *extradata, SINT mode)
{
    ULONG   work[3];
    SINT    size;
    UWORD   lwdata;
    UBYTE   *data;

    data = (UBYTE *)work;

    /* write-mode ? */
    if(MSPROAL_WRITE == mode) {
        /* Not OVERWRITE ? */
        if(MS_CMDPARA_OVERWRITE != atype) {
            size    = 9;
            /* The 1st byte of ExtraData(LogicalAddress) is stored */
            data[8] = extradata[3];
            /* The 2nd byte of ExtraData(LogicalAddress) is stored */
            data[7] = extradata[2];
        } else {
            size = 6;
        }
        /* ExtraData(ManagementFlag) is stored */
        data[6] = extradata[1];
        /* ExtraData(OverwriteFlag) is stored */
        data[5] = extradata[0];
    } else {
        size = 5;
    }

    /* Page number is stored */
    data[4] = page;
    /* Command Parameter is stored */
    data[3] = atype;

    lwdata  = LOWORD(pblk);
    /* The 1st byte of Physical block number is stored */
    data[2] = LOBYTE(lwdata);
    /* The 2nd byte of Physical block number is stored */
    data[1] = HIBYTE(lwdata);
    /* The 3rd byte of Physical block number is stored */
    data[0] = LOBYTE(HIWORD(pblk));

    return msproal_msif_write_reg(  msifhndl,
                                    MS_BLOCK_ADDRESS2_ADRS,
                                    size,
                                    data);
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

/******************************************************************************
*   FUNCTION    : msproal_msif_write_reg
*   DESCRIPTION : Write the (size) amount of data to the Memory Stick Register
*               (adrs).
*------------------------------------------------------------------------------
*   SINT msproal_msif_write_reg(MSIFHNDL *msifhndl, SINT adrs, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       adrs        : Starting address for WRITE_REG(WRITE_ADRS)
*       size        : Write data size for WRITE_REG(WRITE_SIZE)
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_msif_write_reg(MSIFHNDL *msifhndl, SINT adrs, SINT size,
        UBYTE *data)
{
    SINT    result;

    /* Is size less than 1, or larger than 256? */
    if(255 < (UINT)(size - 1)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Setting WRITE_ADRS and WRITE_SIZE */
    result = msproal_tpc_set_rw_reg_adrs(   msifhndl,
                                            MS_STATUS_REG0_ADRS,
                                            1,
                                            adrs,
                                            size);
    if(MSPROAL_OK != result) {
        return result;
    }

    return msproal_tpc_write_reg(msifhndl, size, data);
}
