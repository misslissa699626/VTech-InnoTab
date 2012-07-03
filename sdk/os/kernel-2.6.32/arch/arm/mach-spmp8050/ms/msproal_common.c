/*=============================================================================
* Copyright 2003-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_common.c
*
* DESCRIPTION   : Memory Stick common API
*
* FUNCTION LIST
*                   msproal_drv_common_clear_error_info
*                   msproal_drv_common_init
*                   msproal_drv_common_media_identification
*                   msproal_drv_common_write_error_info
=============================================================================*/
#include <mach/ms/msproal_common.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>

#if         (1 == MSPROAL_ACQUIRE_ERROR)
/******************************************************************************
*   FUNCTION    : msproal_drv_common_clear_error_info
*   DESCRIPTION : Clear error-trace info
*------------------------------------------------------------------------------
*   void msproal_drv_common_clear_error_info(MSIFHNDL *msifhndl, UWORD num,
*       UWORD *count, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       num         : Number to be able to store error infos
*       count       : Count to occur errors
*       data        : Buffer to store error info
******************************************************************************/
void msproal_drv_common_clear_error_info(MSIFHNDL *msifhndl, UWORD num,
         ULONG *count, ULONG *data)
{
    msproal_user_memset((UBYTE *)data, 0x00, num * MSPROAL_SIZEOF_ERROR_INFO);
    *count = 0;

    return;
}
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */

/******************************************************************************
*   FUNCTION    : msproal_drv_common_init
*   DESCRIPTION : Initialization (setting interrupt and variable).
*------------------------------------------------------------------------------
*   SINT msproal_drv_common_init(MSIFHNDL *msifhndl, void *adrs)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       adrs        : Base address in the IP Core corresponding to the slot
*                   to be initialized
******************************************************************************/
SINT msproal_drv_common_init(MSIFHNDL *msifhndl, void *adrs)
{
    msifhndl->BaseAdrs      = (unsigned long)adrs;
    msifhndl->Rw            = MSPROAL_READ_WRITE;
    msifhndl->Wp            = MSPROAL_WP_OFF;
    msifhndl->IfMode        = MSPROAL_SERIAL_MODE;
    msifhndl->IfModeMax     = MSPROAL_SERIAL_MODE;
    msifhndl->PowerClass    = 0;
    msifhndl->Status        = 0;
    /* Clear "IntState" */
    msifhndl->IntState      = MSPROAL_CLEAR_INTSTATE;

    msproal_user_clear_flg(0);

    /* 20 MHz */
    msproal_user_change_clock(MSPROAL_SERIAL_MODE);

    /* Memory Stick I/F is reset */
    return msproal_msif_reset_host(msifhndl);
}

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_common_media_identification
*   DESCRIPTION : Media identification procedure.
*------------------------------------------------------------------------------
*   SINT msproal_drv_common_media_identification(MSIFHNDL *msifhndl,
*           ULONG *stick, SINT *rw, SINT *wp)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       stick       : Area to store the media type and other information
*                   about the Memory Stick media
*       rw          : Area to store the status of a read/write operation
*       wp          : Area to store he information about the write-protect
*                   switch
******************************************************************************/
SINT msproal_drv_common_media_identification(MSIFHNDL *msifhndl, ULONG *stick,
        SINT *rw, SINT *wp)
{
    ULONG   work, tmp[2];
    SINT    result;
#if         (3 == MSPROAL_SUPPORT_IFMODE)
    UWORD   hsttsreg;
#endif  /*  (3 == MSPROAL_SUPPORT_IFMODE)   */
    UBYTE   *stts_regs;
    UBYTE   type_reg, category_reg, class_reg, ifmode_reg;

    /* Read Status Registers */
    stts_regs = (UBYTE *)tmp;
    result = msproal_msif_read_reg(msifhndl, 0, 8, stts_regs);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MS_STTS0_WP & stts_regs[MS_STATUS_REG0_ADRS]) {
        *wp = MSPROAL_WP_ON;
        *rw = MSPROAL_READ_ONLY;
    }

    type_reg        = stts_regs[MS_TYPE_REG_ADRS];
    category_reg    = stts_regs[MS_CATEGORY_REG_ADRS];
    class_reg       = stts_regs[MS_CLASS_REG_ADRS];
    ifmode_reg      = stts_regs[MS2_IF_MODE_REG_ADRS];
    /* Media type of Group1 is distinguished */
    if(0x02 > (UBYTE)(type_reg + 1)) {
        if(type_reg == category_reg) {
            if((category_reg == class_reg)
            || (0x03 > (UBYTE)(class_reg - 1))) {
                work    = MSPROAL_STICK_V1;
                /* Media type is Supporting Serial Interface    */
                /* and Parallel Interface                       */
                if(0x00 == type_reg) {
                    work |= MSPROAL_STICK_S4P;
                }

                /* Media type is determined */
                if(0xFF == class_reg) {
                    work |= MSPROAL_STICK_RW;
                } else {
                    work |= (1L << class_reg);
                }
            } else {
                work    = MSPROAL_STICK_UNKNOWN;
            }
        /* Media type of Memory Stick I/O Expansion Module */
        } else if(0x7F > (UBYTE)(category_reg - 1)) {
            work    = MSPROAL_STICK_IOEX;
        } else {
            work    = MSPROAL_STICK_UNKNOWN;
        }
    /* Media type of Group2 is distinguished */
    } else if(0x01 == type_reg) {
        if(0x00 == category_reg) {
            /* Media type is determined */
            if(0x04 > class_reg) {
                work        = (MSPROAL_STICK_PRO | MSPROAL_STICK_S4P);

                /* Serial, 4bit parallel and 8bit parallel  */
                /* interface mode are supported ?           */
                if(0x00 == ifmode_reg) {
                    /* Media type is determined */
                    work    |= (1L << class_reg);
                } else if(MS2_IFMODE_IFMOD_PROHG == ifmode_reg) {
                    work    |= (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK);
#if         (3 == MSPROAL_SUPPORT_IFMODE)
                    /* Get Status Register */
                    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                            &hsttsreg);
                    if(!(hsttsreg & MSIF_STTS_NO8P)) {
                        /* Supporting 8bit Parallel */
                        work    |= MSPROAL_STICK_S8P_COM;
                    }
#endif  /*  (3 == MSPROAL_SUPPORT_IFMODE)   */
                    /* Media type is determined */
                    work    |= (1L << class_reg);
                } else {
                    work    = MSPROAL_STICK_UNKNOWN;
                }
            } else {
                work    = MSPROAL_STICK_UNKNOWN;
            }
        /* Media type of Memory Stick I/O Expansion Module */
        } else if(0x10 == category_reg) {
            work        = (MSPROAL_STICK_PRO_IOEX | MSPROAL_STICK_S4P);
            /* Serial, 4bit parallel and 8bit parallel  */
            /* interface mode are supported ?           */
            if(MS2_IFMODE_IFMOD_PROHG == ifmode_reg) {
                work    |= (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK);
#if         (3 == MSPROAL_SUPPORT_IFMODE)
                /* Get Status Register */
                msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                        &hsttsreg);
                if(!(hsttsreg & MSIF_STTS_NO8P)) {
                    /* Supporting 8bit Parallel */
                    work    |= MSPROAL_STICK_S8P_COM;
                }
#endif  /*  (3 == MSPROAL_SUPPORT_IFMODE)   */
            } else if(0x00 != ifmode_reg) {
                work    = MSPROAL_STICK_UNKNOWN;
            }
        } else {
            work    = MSPROAL_STICK_UNKNOWN;
        }
    } else {
        work    = MSPROAL_STICK_UNKNOWN;
    }

    *stick = work;

    /* The processing which judges Class Register and sets  */
    /* MSPROAL_READ_ONLY as Rw is performed by Device Type  */
    /* judging processing.                                  */
    /**if(!(msifhndl->Stick & MSPROAL_STICK_RW)) {          */
    /******msifhndl->Rw = MSPROAL_READ_ONLY;                */
    /**}                                                    */

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_common_media_identification
*   DESCRIPTION : Media identification procedure.
*------------------------------------------------------------------------------
*   SINT msproal_drv_common_media_identification(MSIFHNDL *msifhndl,
*           ULONG *stick, SINT *rw, SINT *wp)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       stick       : Area to store the media type and other information
*                   about the Memory Stick media
*       rw          : Area to store the status of a read/write operation
*       wp          : Area to store he information about the write-protect
*                   switch
******************************************************************************/
SINT msproal_drv_common_media_identification(MSIFHNDL *msifhndl, ULONG *stick,
        SINT *rw, SINT *wp)
{
    ULONG   work, tmp[2];
    SINT    result;
    UBYTE   *stts_regs;
    UBYTE   type_reg, category_reg, class_reg, ifmode_reg;

    /* Read Status Registers */
    stts_regs = (UBYTE *)tmp;
    
    #if 1
    result = msproal_msif_read_reg(msifhndl, 0, 8, stts_regs);
    if(MSPROAL_OK != result) {
        return result;
    }
    #else
    
    result = msproal_msif_read_reg(msifhndl, 0, 4, &stts_regs[0]);
    if(MSPROAL_OK != result) {
        return result;
    }

	result = msproal_msif_read_reg(msifhndl, 4, 4, &stts_regs[4]);
	if(MSPROAL_OK != result) {
		return result;
	}
	#endif

    if(MS_STTS0_WP & stts_regs[MS_STATUS_REG0_ADRS]) {
        *wp = MSPROAL_WP_ON;
        *rw = MSPROAL_READ_ONLY;
    }

    type_reg        = stts_regs[MS_TYPE_REG_ADRS];
    category_reg    = stts_regs[MS_CATEGORY_REG_ADRS];
    class_reg       = stts_regs[MS_CLASS_REG_ADRS];
    ifmode_reg      = stts_regs[MS2_IF_MODE_REG_ADRS];
    /* Media type of Group1 is distinguished */
    if(0x02 > (UBYTE)(type_reg + 1)) {
        if(type_reg == category_reg) {
            if((category_reg == class_reg)
            || (0x03 > (UBYTE)(class_reg - 1))) {
                work    = MSPROAL_STICK_V1;
                /* Media type is Supporting Serial Interface    */
                /* and Parallel Interface                       */
                if(0x00 == type_reg) {
                    work |= MSPROAL_STICK_S4P;
                }

                /* Media type is determined */
                if(0xFF == class_reg) {
                    work |= MSPROAL_STICK_RW;
                } else {
                    work |= (1L << class_reg);
                }
            } else {
                work    = MSPROAL_STICK_UNKNOWN;
            }
        /* Media type of Memory Stick I/O Expansion Module */
        } else if(0x7F > (UBYTE)(category_reg - 1)) {
            work    = MSPROAL_STICK_IOEX;
        } else {
            work    = MSPROAL_STICK_UNKNOWN;
        }
    /* Media type of Group2 is distinguished */
    } else if(0x01 == type_reg) {
        if(0x00 == category_reg) {
            /* Media type is determined */
            if(0x04 > class_reg) {
                work        = (MSPROAL_STICK_PRO | MSPROAL_STICK_S4P);

                /* Serial, 4bit parallel and 8bit parallel  */
                /* interface mode are supported ?           */
                if(0x00 == ifmode_reg) {
                    /* Media type is determined */
                    work    |= (1L << class_reg);
                } else if(MS2_IFMODE_IFMOD_PROHG == ifmode_reg) {
                    work    |= (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK);
                    /* Media type is determined */
                    work    |= (1L << class_reg);
                } else {
                    work    = MSPROAL_STICK_UNKNOWN;
                }
            } else {
                work    = MSPROAL_STICK_UNKNOWN;
            }
        /* Media type of Memory Stick I/O Expansion Module */
        } else if(0x10 == category_reg) {
            work        = (MSPROAL_STICK_PRO_IOEX | MSPROAL_STICK_S4P);
            /* Serial, 4bit parallel and 8bit parallel  */
            /* interface mode are supported ?           */
            if(MS2_IFMODE_IFMOD_PROHG == ifmode_reg) {
                work    |= (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK);
            } else if(0x00 != ifmode_reg) {
                work    = MSPROAL_STICK_UNKNOWN;
            }
        } else {
            work    = MSPROAL_STICK_UNKNOWN;
        }
    } else {
        work    = MSPROAL_STICK_UNKNOWN;
    }

    *stick = work;

    /* The processing which judges Class Register and sets  */
    /* MSPROAL_READ_ONLY as Rw is performed by Device Type  */
    /* judging processing.                                  */
    /**if(!(msifhndl->Stick & MSPROAL_STICK_RW)) {          */
    /******msifhndl->Rw = MSPROAL_READ_ONLY;                */
    /**}                                                    */

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         (1 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_common_write_error_info
*   DESCRIPTION : Set error informations.
*------------------------------------------------------------------------------
*   void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
*           SINT result, ULONG *count, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       id          : Function ID
*       result      : Result value
*       count       : Count to occur errors
*       data        : Area to store error information
******************************************************************************/
void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
        SINT result, ULONG *count, ULONG *data)
{
#if         (1 == MSPROAL_ACQUIRE_ERROR)
    ULONG   *error_info;
    ULONG   err_cnt;
    ULONG   hadrs;
    SINT    end_adrs, h_reg_cnt;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    SINT    ifmode;

#if         (1 == MSPROAL_ACQUIRE_ERROR)
    err_cnt     = *count;
    error_info  = data;

    /* Set Function ID */
    *error_info ++  = id;
    /* Set Result value */
    *error_info ++  = (ULONG)result;
    /* Set Error Count */
    *error_info ++  = err_cnt;

    /* Set the register of Host Controller(0x30) */
    msproal_user_read_mem32(MSIF_CMD_ADRS(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the registers of Host Controller(0x38 - 0x44) */
    hadrs       = MSIF_STTS_ADRS(msifhndl->BaseAdrs);
    end_adrs    = 0xC;
    for(h_reg_cnt = 0; end_adrs >= h_reg_cnt; h_reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(hadrs + h_reg_cnt, error_info);
        error_info ++;
    }

    /* Set the register of Host Controller(0x4C) */
    msproal_user_read_mem32(MSIF_DMA_CTRL_ADRS(msifhndl->BaseAdrs),
                            error_info);

    /* Increment error count */
    *count = err_cnt + 1;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */

    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return;
    }

    ifmode = msifhndl->IfMode;
    if(MSPROAL_4PARALLEL_MODE == ifmode) {
        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            ifmode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            ifmode = MSPROAL_V1_4PARALLEL_MODE;
        }
    }

tpc_ex_set_cmd    result = msproal_tpc_change_ifmode(msifhndl, ifmode);
    if(MSPROAL_OK != result) {
        return;
    }

    return;
}
#endif  /*  (1 == MSPROAL_SUPPORT_IP)   */

#if         (2 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_common_write_error_info
*   DESCRIPTION : Set error informations.
*------------------------------------------------------------------------------
*   void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
*           SINT result, ULONG *count, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       id          : Function ID
*       result      : Result value
*       count       : Count to occur errors
*       data        : Area to store error information
******************************************************************************/
void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
        SINT result, ULONG *count, ULONG *data)
{
#if         (1 == MSPROAL_ACQUIRE_ERROR)
    ULONG   *error_info;
    ULONG   err_cnt;
    ULONG   hadrs, iadrs;
    SINT    end_adrs, reg_cnt, h_reg_cnt;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    SINT    ifmode;

#if         (1 == MSPROAL_ACQUIRE_ERROR)
    err_cnt     = *count;
    error_info  = data;

    /* Set Function ID */
    *error_info ++  = id;
    /* Set Result value */
    *error_info ++  = (ULONG)result;
    /* Set Error Count */
    *error_info ++  = err_cnt;

    /* Set the registers of I-CON(0x00 - 0x1C) */
    iadrs       = ICON_CTRL_REG(msifhndl->BaseAdrs);
    end_adrs    = 0x1C;
    for(reg_cnt = 0; end_adrs >= reg_cnt; reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(iadrs + reg_cnt, error_info);
        error_info ++;
    }

    /* Set the register of I-CON(0x2C) */
    msproal_user_read_mem32(ICON_VER_REG(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the register of Host Controller(0x30) */
    msproal_user_read_mem32(MSIF_CMD_ADRS(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the registers of Host Controller(0x38 - 0x44) */
    hadrs       = MSIF_STTS_ADRS(msifhndl->BaseAdrs);
    end_adrs    = 0xC;
    for(h_reg_cnt = 0; end_adrs >= h_reg_cnt; h_reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(hadrs + h_reg_cnt, error_info);
        error_info ++;
    }

    /* Set the register of Host Controller(0x4C) */
    msproal_user_read_mem32(MSIF_DMA_CTRL_ADRS(msifhndl->BaseAdrs),
                            error_info);

    /* Increment error count */
    *count = err_cnt + 1;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */

    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return;
    }

    ifmode = msifhndl->IfMode;
    if(MSPROAL_4PARALLEL_MODE == ifmode) {
        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            ifmode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            ifmode = MSPROAL_V1_4PARALLEL_MODE;
        }
    }

    result = msproal_tpc_change_ifmode(msifhndl, ifmode);
    if(MSPROAL_OK != result) {
        return;
    }

    return;
}
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (3 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_common_write_error_info
*   DESCRIPTION : Set error informations.
*------------------------------------------------------------------------------
*   void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
*           SINT result, ULONG *count, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       id          : Function ID
*       result      : Result value
*       count       : Count to occur errors
*       data        : Area to store error information
******************************************************************************/
void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
        SINT result, ULONG *count, ULONG *data)
{
#if         (1 == MSPROAL_ACQUIRE_ERROR)
    ULONG   *error_info;
    ULONG   err_cnt;
    ULONG   hadrs;
    SINT    end_adrs, h_reg_cnt;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    SINT    ifmode;

#if         (1 == MSPROAL_ACQUIRE_ERROR)
    err_cnt     = *count;
    error_info  = data;

    /* Set Function ID */
    *error_info ++  = id;
    /* Set Result value */
    *error_info ++  = (ULONG)result;
    /* Set Error Count */
    *error_info ++  = err_cnt;

    /* Set the register of Host Controller(0x30) */
    msproal_user_read_mem32(MSIF_CMD_ADRS(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the registers of Host Controller(0x38 - 0x3C) */
    hadrs       = MSIF_STTS_ADRS(msifhndl->BaseAdrs);
    end_adrs    = 0x4;
    for(h_reg_cnt = 0; end_adrs >= h_reg_cnt; h_reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(hadrs + h_reg_cnt, error_info);
        error_info ++;
    }

    /* Increment error count */
    *count = err_cnt + 1;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */

    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return;
    }

    ifmode = msifhndl->IfMode;
    if(MSPROAL_4PARALLEL_MODE == ifmode) {
        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            ifmode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            ifmode = MSPROAL_V1_4PARALLEL_MODE;
        }
    }

    result = msproal_tpc_change_ifmode(msifhndl, ifmode);
    if(MSPROAL_OK != result) {
        return;
    }

    return;
}
#endif  /*  (3 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_common_write_error_info
*   DESCRIPTION : Set error informations.
*------------------------------------------------------------------------------
*   void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
*           SINT result, ULONG *count, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       id          : Function ID
*       result      : Result value
*       count       : Count to occur errors
*       data        : Area to store error information
******************************************************************************/
void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
        SINT result, ULONG *count, ULONG *data)
{
#if         (1 == MSPROAL_ACQUIRE_ERROR)
    ULONG   *error_info;
    ULONG   err_cnt;
    ULONG   hadrs, iadrs;
    SINT    end_adrs, reg_cnt, h_reg_cnt;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    SINT    ifmode;

#if         (1 == MSPROAL_ACQUIRE_ERROR)
    err_cnt     = *count;
    error_info  = data;

    /* Set Function ID */
    *error_info ++  = id;
    /* Set Result value */
    *error_info ++  = (ULONG)result;
    /* Set Error Count */
    *error_info ++  = err_cnt;

    /* Set the registers of I-CON(0x00 - 0x1C) */
    iadrs       = ICON_CTRL_REG(msifhndl->BaseAdrs);
    end_adrs    = 0x1C;
    for(reg_cnt = 0; end_adrs >= reg_cnt; reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(iadrs + reg_cnt, error_info);
        error_info ++;
    }

    /* Set the register of I-CON(0x2C) */
    msproal_user_read_mem32(ICON_VER_REG(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the register of Host Controller(0x30) */
    msproal_user_read_mem32(MSIF_CMD_ADRS(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the registers of Host Controller(0x38 - 0x3C) */
    hadrs       = MSIF_STTS_ADRS(msifhndl->BaseAdrs);
    end_adrs    = 0x4;
    for(h_reg_cnt = 0; end_adrs >= h_reg_cnt; h_reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(hadrs + h_reg_cnt, error_info);
        error_info ++;
    }

    /* Increment error count */
    *count = err_cnt + 1;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */

    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return;
    }

    ifmode = msifhndl->IfMode;
    if(MSPROAL_4PARALLEL_MODE == ifmode) {
        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            ifmode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            ifmode = MSPROAL_V1_4PARALLEL_MODE;
        }
    }

    result = msproal_tpc_change_ifmode(msifhndl, ifmode);
    if(MSPROAL_OK != result) {
        return;
    }

    return;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_common_write_error_info
*   DESCRIPTION : Set error informations.
*------------------------------------------------------------------------------
*   void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
*           SINT result, ULONG *count, ULONG *data)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       id          : Function ID
*       result      : Result value
*       count       : Count to occur errors
*       data        : Area to store error information
******************************************************************************/
void msproal_drv_common_write_error_info(MSIFHNDL *msifhndl, ULONG id,
        SINT result, ULONG *count, ULONG *data)
{
#if         (1 == MSPROAL_ACQUIRE_ERROR)
    ULONG   *error_info;
    ULONG   err_cnt;
    ULONG   hadrs, iadrs;
    SINT    end_adrs, reg_cnt, h_reg_cnt;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    SINT    ifmode;

#if         (1 == MSPROAL_ACQUIRE_ERROR)
    err_cnt     = *count;
    error_info  = data;

    /* Set Function ID */
    *error_info ++  = id;
    /* Set Result value */
    *error_info ++  = (ULONG)result;
    /* Set Error Count */
    *error_info ++  = err_cnt;

    /* Set the registers of I-CON(0x00 - 0x1C) */
    iadrs       = ICON_CTRL_REG(msifhndl->BaseAdrs);
    end_adrs    = 0x1C;
    for(reg_cnt = 0; end_adrs >= reg_cnt; reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(iadrs + reg_cnt, error_info);
        error_info ++;
    }

    /* Set the register of I-CON(0x2C) */
    msproal_user_read_mem32(ICON_VER_REG(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the register of Host Controller(0x30) */
    msproal_user_read_mem32(MSIF_CMD_ADRS(msifhndl->BaseAdrs), error_info);
    error_info ++;

    /* Set the registers of Host Controller(0x38 - 0x44) */
    hadrs       = MSIF_STTS_ADRS(msifhndl->BaseAdrs);
    end_adrs    = 0xC;
    for(h_reg_cnt = 0; end_adrs >= h_reg_cnt; h_reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(hadrs + h_reg_cnt, error_info);
        error_info ++;
    }

    /* Set the registers of I-CON(0x80 - 0xA0) */
    iadrs       = ICON_DMA0_TADR_REG(msifhndl->BaseAdrs);
    end_adrs    = 0x20;
    for(reg_cnt = 0; end_adrs >= reg_cnt; reg_cnt += sizeof(ULONG)) {
        msproal_user_read_mem32(iadrs + reg_cnt, error_info);
        error_info ++;
    }

    /* Increment error count */
    *count = err_cnt + 1;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */

    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return;
    }

    ifmode = msifhndl->IfMode;
    if(MSPROAL_4PARALLEL_MODE == ifmode) {
        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            ifmode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            ifmode = MSPROAL_V1_4PARALLEL_MODE;
        }
    }

    result = msproal_tpc_change_ifmode(msifhndl, ifmode);
    if(MSPROAL_OK != result) {
        return;
    }

    return;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */
