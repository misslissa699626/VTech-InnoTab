/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal.c
*
* DESCRIPTION   : FileSystem I/F API
*
* FUNCTION LIST
*                   msproal_init
*                   msproal_recovery
*                   msproal_mount
*                   msproal_start
*                   msproal_unmount
*                   msproal_check_media
*                   msproal_check_stick
*                   msproal_check_write_status
*                   msproal_clear_error_info
*                   msproal_get_ifmode
*                   msproal_get_info
*                   msproal_get_model_name
*                   msproal_get_progress
*                   msproal_get_system_info
*                   msproal_read_atrb_info
*                   msproal_read_lba
*                   msproal_read_sect
*                   msproal_write_lba
*                   msproal_write_sect
*                   msproal_control_ifmode
*                   msproal_control_power_class
*                   msproal_control_power_supply
*                   msproal_format
*                   msproal_sleep
*                   msproal_wakeup
*                   msproal_write_pout_reg
=============================================================================*/
#include <mach/ms/msproal.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>

FUNC msproal_pro_func = {
    msproal_seq_pro_read_atrb,
    msproal_drv_pro_read_data,
    msproal_drv_pro_write_data,
    msproal_drv_pro_get_model_name,
    msproal_seq_pro_change_power,
    msproal_seq_pro_format,
    msproal_drv_pro_mount,
    msproal_seq_pro_sleep,
    msproal_drv_pro_wakeup,
    msproal_drv_pro_recovery,
    msproal_drv_pro_stop
};

#if         (1 == MSPROAL_SUPPORT_V1)
FUNC msproal_v1_func = {
    msproal_drv_v1_read_atrb_info,
    msproal_trans_read_lba,
    msproal_trans_write_lba,
    msproal_drv_v1_get_model_name,
    msproal_drv_v1_change_power,
    msproal_trans_format,
    msproal_drv_v1_mount,
    msproal_seq_v1_sleep,
    msproal_drv_v1_wakeup,
    msproal_drv_v1_recovery,
    msproal_drv_v1_stop
};
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

MSPROAL_ERROR   msproal_error;

MSIFHNDL    *msifhndl;
FUNC        *msproal_func;

/******************************************************************************
*   FUNCTION    : msproal_init
*   DESCRIPTION : Initialize the system.
*------------------------------------------------------------------------------
*   SINT msproal_init(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_init(void)
{
#if         (1 == MSPROAL_ACQUIRE_ERROR)
    ULONG   *data;
    UWORD   num;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    SINT    result;

    msifhndl            = (MSIFHNDL *)MSPROAL_NULL;
    msproal_func        = (FUNC *)MSPROAL_NULL;

    result = msproal_user_init_system();

    if(MSPROAL_OK == result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        msproal_user_get_error_info(&data, &num);

        msproal_error.ErrInfo  = data;
        msproal_error.ErrNum   = num;

        msproal_drv_common_clear_error_info(msifhndl,
                                            num,
                                            &msproal_error.ErrCnt,
                                            data);
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    }

    return result;
}

/******************************************************************************
*   FUNCTION    : msproal_recovery
*   DESCRIPTION : Execute recovery procedure for Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_recovery(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_recovery(void)
{
    SINT    result;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    result = (*(msproal_func->recovery))(msifhndl);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_RECOVERY,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_mount
*   DESCRIPTION : Mount Memory Stick
*------------------------------------------------------------------------------
*   SINT msproal_mount(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_FORMAT_WARN         : Data format warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_LOCKED_ERR          : Read/Write protect error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE/MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_mount(SINT mode)
{
    SINT    result, result_stop;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         (1 == MSPROAL_SUPPORT_V1)
    if(MSPROAL_STICK_PRO & msifhndl->Stick) {
        msproal_func = &msproal_pro_func;
    } else if(MSPROAL_STICK_V1 & msifhndl->Stick) {
        msproal_func = &msproal_v1_func;
    } else {
        return MSPROAL_UNSUPPORT_ERR;
    }
#else   /*  (0 == MSPROAL_SUPPORT_V1)   */
    if(MSPROAL_STICK_PRO & msifhndl->Stick) {
        msproal_func = &msproal_pro_func;
    } else {
        return MSPROAL_UNSUPPORT_ERR;
    }
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

    result = (*(msproal_func->mount))(msifhndl, mode);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_MOUNT,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            data    = msproal_error.ErrInfo
                    + (msproal_error.ErrCnt % msproal_error.ErrNum)
                    * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                data);

            return result_stop;
        }

        if((MSPROAL_FORMAT_WARN == result) || (MSPROAL_FORMAT_ERR == result)) {
            msifhndl->Status |= MSPROAL_STTS_MOU_STORAGE;
        }

        return result;
    }

    msifhndl->Status |= MSPROAL_STTS_MOU_STORAGE;

    return result;
}

/******************************************************************************
*   FUNCTION    : msproal_start
*   DESCRIPTION : Initialize Memory Stick I/F and circumference circuits and
*               execute Media type identification process.
*------------------------------------------------------------------------------
*   SINT msproal_start(UBYTE *workarea)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       workarea    : Area to store the data to be used inside.
******************************************************************************/
SINT msproal_start(UBYTE *workarea)
{
    ULONG   adrs;
    SINT    result;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

#if 0
    result = msproal_user_get_base_adrs(&adrs);
    if(MSPROAL_OK != result) {
        return result;
    }
#endif

    msifhndl = (MSIFHNDL *)workarea;
    adrs = 0;
    result = msproal_drv_common_init(msifhndl, (void *)adrs);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_drv_common_media_identification(   msifhndl,
                                                        &msifhndl->Stick,
                                                        &msifhndl->Rw,
                                                        &msifhndl->Wp);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_START,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_unmount
*   DESCRIPTION : Unmount Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_unmount(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_unmount(void)
{
    SINT    result;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_OK;
    }

    msproal_user_change_clock(MSPROAL_SERIAL_MODE);

    /* Memory Stick I/F is reset */
    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl            = (MSIFHNDL *)MSPROAL_NULL;
    msproal_func        = (FUNC *)MSPROAL_NULL;

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_check_media
*   DESCRIPTION : Report media type.
*------------------------------------------------------------------------------
*   SINT msproal_check_media(UBYTE *media)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       media       : Area to store the media type of Memory Stick
******************************************************************************/
SINT msproal_check_media(UBYTE *media)
{
    ULONG   stick;
    SINT    result;
    SINT    rw, wp;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    result = msproal_drv_common_media_identification(   msifhndl,
                                                        &stick,
                                                        &rw,
                                                        &wp);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_CHECK_MEDIA,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

    *media = (UBYTE)stick;

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_check_stick
*   DESCRIPTION : Confirm whether Memory Stick is inserted.
*------------------------------------------------------------------------------
*   SINT msproal_check_stick(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_check_stick(void)
{
    return msproal_user_check_stick();
}

/******************************************************************************
*   FUNCTION    : msproal_check_write_status
*   DESCRIPTION : Confirm whether Memory Stick is in the write-protected
*                 status or not.
*------------------------------------------------------------------------------
*   SINT msproal_check_write_status(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_check_write_status(void)
{
    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_WRITE_ERR;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_clear_error_info
*   DESCRIPTION : Clear error-trace info
*------------------------------------------------------------------------------
*   SINT msproal_clear_error_info(void)
*   RETURN
*       MSPROAL_OK              : Normal
*       MSPROAL_SYSTEM_ERR      : System error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_clear_error_info(void)
{
#if  (1 == MSPROAL_ACQUIRE_ERROR)
    msproal_drv_common_clear_error_info(msifhndl,
                                        msproal_error.ErrNum,
                                        &msproal_error.ErrCnt,
                                        msproal_error.ErrInfo);

    return MSPROAL_OK;
#else   /*  (0 == MSPROAL_ACQUIRE_ERROR)  */
    return MSPROAL_SYSTEM_ERR;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)  */
}

/******************************************************************************
*   FUNCTION    : msproal_get_ifmode
*   DESCRIPTION : Get the interface mode of Memory Stick side.
*------------------------------------------------------------------------------
*   SINT msproal_get_ifmode(SINT *mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_IFMODE_WARN         : Interface mode warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       mode        : Area to store the Interface Mode
******************************************************************************/
SINT msproal_get_ifmode(SINT *mode)
{
    SINT    result;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    result = msproal_msif_get_ifmode(msifhndl, mode);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_GET_IFMODE,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_get_info
*   DESCRIPTION : Report informations of Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_get_info(MSPROAL_MSINFO *msinfo)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       msinfo      : Area to store the informations of Memory Stick
******************************************************************************/
SINT msproal_get_info(MSPROAL_MSINFO *msinfo)
{
    const static ULONG ms_capa[6] = {
        0x1EE0, 0x3DE0, 0x7BC0, 0xF7C0, 0x1EF80, 0x3DF00
    };
    ULONG   blk_size, effect_blk_num;
    SINT    ms_no, eseg;
    UWORD   blk_num, unit_size;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(msifhndl->Stick & MSPROAL_STICK_V1) {
        blk_size            = MAKEWORD( msifhndl->BootData[0x1A2],
                                        msifhndl->BootData[0x1A3]);
        blk_num             = MAKEWORD( msifhndl->BootData[0x1A4],
                                        msifhndl->BootData[0x1A5]);
        eseg                = blk_num >> 9;
        unit_size           = 1024;

        msinfo->PhyCapa     = (494 + 496 * (eseg - 1)) * (blk_size * 2);
        /* ms_no =  0   4MB     */
        /*          1   8MB     */
        /*          2   16MB    */
        /*          3   32MB    */
        /*          4   64MB    */
        /*          5   128MB   */
        ms_no = 0;
        while(eseg >>= 1) {
            ms_no ++;
        }
        if(0x10 == blk_size) {
            ms_no ++;
        }
        msinfo->LogiCapa    = ms_capa[ms_no];
    } else if(msifhndl->Stick & MSPROAL_STICK_PRO) {
        blk_size        = MAKEWORD( msifhndl->BootData[0x1A2],
                                    msifhndl->BootData[0x1A3]);
        effect_blk_num  = MAKEWORD( msifhndl->BootData[0x1A6],
                                    msifhndl->BootData[0x1A7]);
        unit_size       = MAKEWORD( msifhndl->BootData[0x1CC],
                                    msifhndl->BootData[0x1CD]);

        msinfo->PhyCapa     = blk_size * effect_blk_num * (unit_size / 512);
        msinfo->LogiCapa    = msinfo->PhyCapa;
    } else {
        blk_size        = MAKELONG( MAKEWORD(   msifhndl->BootData[0x1C2],
                                                msifhndl->BootData[0x1C3]),
                                    MAKEWORD(   msifhndl->BootData[0x1C0],
                                                msifhndl->BootData[0x1C1]));
        effect_blk_num  = MAKELONG( MAKEWORD(   msifhndl->BootData[0x1A8],
                                                msifhndl->BootData[0x1A9]),
                                    MAKEWORD(   msifhndl->BootData[0x1A6],
                                                msifhndl->BootData[0x1A7]));
        unit_size       = MAKEWORD( msifhndl->BootData[0x1CC],
                                    msifhndl->BootData[0x1CD]);

        msinfo->PhyCapa     = blk_size * effect_blk_num * (unit_size / 512);
        msinfo->LogiCapa    = msinfo->PhyCapa;
    }
    msinfo->BlockSize   = blk_size * (unit_size / 512);
    msinfo->Stick       = msifhndl->Stick;
    msinfo->Wp          = msifhndl->Wp;

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_get_model_name
*   DESCRIPTION : Get the Model name.
*------------------------------------------------------------------------------
*   SINT msproal_get_model_name(SBYTE *modelname)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       modelname   : Address to area where the information is stored
******************************************************************************/
SINT msproal_get_model_name(SBYTE *modelname)
{
    SINT    result, result_stop;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    /* Get Model name */
    result = (*(msproal_func->get_name))(msifhndl, modelname);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_GET_MODEL_NAME,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            data    = msproal_error.ErrInfo
                    + (msproal_error.ErrCnt % msproal_error.ErrNum)
                    * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                data);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return result;
}

/******************************************************************************
*   FUNCTION    : msproal_get_progress
*   DESCRIPTION : Report progress of a format process.
*------------------------------------------------------------------------------
*   SINT msproal_get_progress(SINT *progress)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       progress    :  Area to store the progress
******************************************************************************/
SINT msproal_get_progress(SINT *progress)
{
    ULONG   all_data, proc_data;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    all_data    = MAKELONG( MAKEWORD(   msifhndl->WorkArea[2],
                                        msifhndl->WorkArea[3]),
                            MAKEWORD(   msifhndl->WorkArea[0],
                                        msifhndl->WorkArea[1]));
    proc_data   = MAKELONG( MAKEWORD(   msifhndl->WorkArea[6],
                                        msifhndl->WorkArea[7]),
                            MAKEWORD(   msifhndl->WorkArea[4],
                                        msifhndl->WorkArea[5]));

    if(0 != all_data) {
        *progress   = (proc_data * 100) / all_data;
    } else {
        *progress   = -1;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_get_sytem_info
*   DESCRIPTION : In case of Memory Stick, get the boot & attribute
*               information. In case of Memory Stick PRO, get the system
*               information.
*------------------------------------------------------------------------------
*   SINT msproal_get_system_info(UBYTE *sysinfo)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       sysinfo     : Address to area where the information is stored
******************************************************************************/
SINT msproal_get_system_info(UBYTE *sysinfo)
{
    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    msproal_user_memcpy(sysinfo,
                        &msifhndl->BootData[MSPROAL_BOOT_ATTR_ADRS],
                        96);

    return MSPROAL_OK;
}

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_read_atrb_info
*   DESCRIPTION : Read data of specified attribute information.
*------------------------------------------------------------------------------
*   SINT msproal_read_atrb_info(ULONG lsct, ULONG size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lsct        : Start logical sector number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_read_atrb_info(ULONG lsct, ULONG size, UBYTE *data)
{
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    /* Read Attribute Information */
    result = (*(msproal_func->read_atrb))(msifhndl, lsct, (SINT)size, data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_READ_ATRB_INFO,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return result;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_read_atrb_info
*   DESCRIPTION : Read data of specified attribute information.
*------------------------------------------------------------------------------
*   SINT msproal_read_atrb_info(ULONG lsct, ULONG size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lsct        : Start logical sector number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_read_atrb_info(ULONG lsct, ULONG size, UBYTE *data)
{
#if         (0 == MSPROAL_SUPPORT_VMEM)
    ULONG   *lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    trans_size;
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

#if         (0 == MSPROAL_SUPPORT_VMEM)
    idadrsreg   = (ULONG)data;
    idcnfreg    = (ICON_DMA_CNF_DMAEN
                | ICON_DMA_CNF_LLTEN
                | ICON_DMA_CNF_BSZ_64);
    data        = msifhndl->WorkArea;
    lltbl       = (ULONG *)data;
    trans_size  = size * 512;
    while(0 < trans_size) {
        lltbl[0]        = idadrsreg;
        if(MSPROAL_MAX_TRANS_NUM < trans_size) {
            lltbl[1]    = (ULONG)&lltbl[3];
            lltbl[2]    = idcnfreg;
            trans_size  -= MSPROAL_MAX_TRANS_NUM;
            idadrsreg   += MSPROAL_MAX_TRANS_NUM;
        } else {
            lltbl[1]    = 0;
            lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                        | ((trans_size >> 2) & ICON_DMA_CNF_TRCNT_MASK));
            trans_size  = 0;
        }

        lltbl       += 3;
    }
    msproal_user_flush_cache((void *)data, 512 * 4);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */

    /* Read Attribute Information */
    result = (*(msproal_func->read_atrb))(msifhndl, lsct, (SINT)size, data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_READ_ATRB_INFO,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return result;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_read_lba
*   DESCRIPTION : Read data of specified sector size from specified LBA number.
*------------------------------------------------------------------------------
*   SINT msproal_read_lba(ULONG lba, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lba         : Start LBA number
*       size        : The number of read-out LBAs
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_read_lba(ULONG lba, SINT size, UBYTE *data)
{
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = (*(msproal_func->read))(msifhndl, lba, size, data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_READ_LBA,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_read_lba
*   DESCRIPTION : Read data of specified sector size from specified LBA number.
*------------------------------------------------------------------------------
*   SINT msproal_read_lba(ULONG lba, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lba         : Start LBA number
*       size        : The number of read-out LBAs
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_read_lba(ULONG lba, SINT size, UBYTE *data)
{
#if         (0 == MSPROAL_SUPPORT_VMEM)
    ULONG   *lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    trans_size;
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

#if         (0 == MSPROAL_SUPPORT_VMEM)
    idadrsreg   = (ULONG)data;
    idcnfreg    = (ICON_DMA_CNF_DMAEN
                | ICON_DMA_CNF_LLTEN
                | ICON_DMA_CNF_BSZ_64);
    data        = msifhndl->WorkArea;
    lltbl       = (ULONG *)data;
    trans_size  = size * 512;
    while(0 < trans_size) {
        lltbl[0]        = idadrsreg;
        if(MSPROAL_MAX_TRANS_NUM < trans_size) {
            lltbl[1]    = (ULONG)&lltbl[3];
            lltbl[2]    = idcnfreg;
            trans_size  -= MSPROAL_MAX_TRANS_NUM;
            idadrsreg   += MSPROAL_MAX_TRANS_NUM;
        } else {
            lltbl[1]    = 0;
            lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                        | ((trans_size >> 2) & ICON_DMA_CNF_TRCNT_MASK));
            trans_size  = 0;
        }

        lltbl       += 3;
    }
    msproal_user_flush_cache((void *)data, 512 * 4);
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    result = (*(msproal_func->read))(msifhndl, lba, size, data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_READ_LBA,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_read_sect
*   DESCRIPTION : Read data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_read_sect(ULONG lsct, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lsct        : Start logical sector number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_read_sect(ULONG lsct, SINT size, UBYTE *data)
{
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = (*(msproal_func->read))(   msifhndl,
                                        lsct ,//+ msifhndl->HidSct,
                                        size,
                                        data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_READ_SECT,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_read_sect
*   DESCRIPTION : Read data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_read_sect(ULONG lsct, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lsct        : Start logical sector number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_read_sect(ULONG lsct, SINT size, UBYTE *data)
{
#if         (0 == MSPROAL_SUPPORT_VMEM)
    ULONG   *lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    trans_size;
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

#if         (0 == MSPROAL_SUPPORT_VMEM)
    idadrsreg   = (ULONG)data;
    idcnfreg    = (ICON_DMA_CNF_DMAEN
                | ICON_DMA_CNF_LLTEN
                | ICON_DMA_CNF_BSZ_64);
    data        = msifhndl->WorkArea;
    lltbl       = (ULONG *)data;
    trans_size  = size * 512;
    while(0 < trans_size) {
        lltbl[0]        = idadrsreg;
        if(MSPROAL_MAX_TRANS_NUM < trans_size) {
            lltbl[1]    = (ULONG)&lltbl[3];
            lltbl[2]    = idcnfreg;
            trans_size  -= MSPROAL_MAX_TRANS_NUM;
            idadrsreg   += MSPROAL_MAX_TRANS_NUM;
        } else {
            lltbl[1]    = 0;
            lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                        | ((trans_size >> 2) & ICON_DMA_CNF_TRCNT_MASK));
            trans_size  = 0;
        }

        lltbl       += 3;
    }
    msproal_user_flush_cache((void *)data, 512 * 4);
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    result = (*(msproal_func->read))(   msifhndl,
                                        lsct + msifhndl->HidSct,
                                        size,
                                        data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_READ_SECT,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_write_lba
*   DESCRIPTION : Write data of specified sector size from specified LBA
*               number.
*------------------------------------------------------------------------------
*   SINT msproal_write_lba(ULONG lba, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lba         : Start LBA number
*       size        : The number of write-in LBAs
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_write_lba(ULONG lba, SINT size, UBYTE *data)
{
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = (*(msproal_func->write))(msifhndl, lba, size, data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_WRITE_LBA,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_write_lba
*   DESCRIPTION : Write data of specified sector size from specified LBA
*               number.
*------------------------------------------------------------------------------
*   SINT msproal_write_lba(ULONG lba, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lba         : Start LBA number
*       size        : The number of write-in LBAs
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_write_lba(ULONG lba, SINT size, UBYTE *data)
{
#if         (0 == MSPROAL_SUPPORT_VMEM)
    ULONG   *lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    trans_size;
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

#if         (0 == MSPROAL_SUPPORT_VMEM)
    idadrsreg   = (ULONG)data;
    idcnfreg    = (ICON_DMA_CNF_DMAEN
                | ICON_DMA_CNF_LLTEN
                | ICON_DMA_CNF_BSZ_64);
    data        = msifhndl->WorkArea;
    lltbl       = (ULONG *)data;
    trans_size  = size * 512;
    while(0 < trans_size) {
        lltbl[0]        = idadrsreg;
        if(MSPROAL_MAX_TRANS_NUM < trans_size) {
            lltbl[1]    = (ULONG)&lltbl[3];
            lltbl[2]    = idcnfreg;
            trans_size  -= MSPROAL_MAX_TRANS_NUM;
            idadrsreg   += MSPROAL_MAX_TRANS_NUM;
        } else {
            lltbl[1]    = 0;
            lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                        | ((trans_size >> 2) & ICON_DMA_CNF_TRCNT_MASK));
            trans_size  = 0;
        }

        lltbl       += 3;
    }
    msproal_user_flush_cache((void *)data, 512 * 4);
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    result = (*(msproal_func->write))(msifhndl, lba, size, data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_WRITE_LBA,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_write_sect
*   DESCRIPTION : Write data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_write_sect(ULONG lsct, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lsct        : Start logical sector number
*       size        : The number of write-in sectors
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_write_sect(ULONG lsct, SINT size, UBYTE *data)
{
    SINT    result, result_stop,retry;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */
	
	retry = MSPROAL_RETRY_COUNT;
    
    do {
    	result = (*(msproal_func->write))(  msifhndl,
    	                                    lsct ,//+ msifhndl->HidSct,
    	                                    size,
    	                                    data);
    	if(MSPROAL_OK != result) {
#if 	        (1 == MSPROAL_ACQUIRE_ERROR)
    	    dat = msproal_error.ErrInfo
    	        + (msproal_error.ErrCnt % msproal_error.ErrNum)
    	        * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    	    msproal_drv_common_write_error_info(msifhndl,
    	                                        MSPROAL_ID_WRITE_SECT,
    	                                        result,
    	                                        &msproal_error.ErrCnt,
    	                                        dat);
    	
    	    if(MSPROAL_EXTRACT_ERR == result) {
    	        return MSPROAL_EXTRACT_ERR;
    	    }

    	    result_stop = (*(msproal_func->stop))(msifhndl);
    	    if(MSPROAL_OK != result_stop) {
#if 	        (1 == MSPROAL_ACQUIRE_ERROR)
    	        dat = msproal_error.ErrInfo
    	            + (msproal_error.ErrCnt % msproal_error.ErrNum)
    	            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
    	        msproal_drv_common_write_error_info(msifhndl,
    	                                            MSPROAL_ID_STOP,
    	                                            result_stop,
    	                                            &msproal_error.ErrCnt,
    	                                            dat);
    	
    	        return result_stop;
    	    }
			
			if (MSPROAL_CMDNK_ERR == result) { //========== TEST# : T9-002 ==========
				msifhndl->Rw = MSPROAL_READ_ONLY;
				return MSPROAL_WRITE_ERR;
			}
    	    //return result;
    	    continue;
    	}
    	return MSPROAL_OK;
    }while(retry--); //========== TEST# : T9-001 ==========

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */
	return result;
    //return MSPROAL_OK;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_write_sect
*   DESCRIPTION : Write data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_write_sect(ULONG lsct, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       lsct        : Start logical sector number
*       size        : The number of write-in sectors
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_write_sect(ULONG lsct, SINT size, UBYTE *data)
{
#if         (0 == MSPROAL_SUPPORT_VMEM)
    ULONG   *lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    trans_size;
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    SINT    result, result_stop;
    ULONG   *dat;

    dat = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

#if         (0 == MSPROAL_SUPPORT_VMEM)
    idadrsreg   = (ULONG)data;
    idcnfreg    = (ICON_DMA_CNF_DMAEN
                | ICON_DMA_CNF_LLTEN
                | ICON_DMA_CNF_BSZ_64);
    data        = msifhndl->WorkArea;
    lltbl       = (ULONG *)data;
    trans_size  = size * 512;
    while(0 < trans_size) {
        lltbl[0]        = idadrsreg;
        if(MSPROAL_MAX_TRANS_NUM < trans_size) {
            lltbl[1]    = (ULONG)&lltbl[3];
            lltbl[2]    = idcnfreg;
            trans_size  -= MSPROAL_MAX_TRANS_NUM;
            idadrsreg   += MSPROAL_MAX_TRANS_NUM;
        } else {
            lltbl[1]    = 0;
            lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                        | ((trans_size >> 2) & ICON_DMA_CNF_TRCNT_MASK));
            trans_size  = 0;
        }

        lltbl       += 3;
    }
    msproal_user_flush_cache((void *)data, 512 * 4);
#endif  /*  (0 == MSPROAL_SUPPORT_VMEM) */
    result = (*(msproal_func->write))(  msifhndl,
                                        lsct + msifhndl->HidSct,
                                        size,
                                        data);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        dat = msproal_error.ErrInfo
            + (msproal_error.ErrCnt % msproal_error.ErrNum)
            * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_WRITE_SECT,
                                            result,
                                            &msproal_error.ErrCnt,
                                            dat);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            dat = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                dat);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

/******************************************************************************
*   FUNCTION    : msproal_control_ifmode
*   DESCRIPTION : Control the interface mode of Memory Stick side
*               and Host Controller side.
*------------------------------------------------------------------------------
*   SINT msproal_control_ifmode(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE/MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_control_ifmode(SINT mode)
{
#if         (1 != MSPROAL_SUPPORT_IFMODE)
    SINT    result;
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE)   */
    SINT    ifmode;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    ifmode = msifhndl->IfMode;
    if(ifmode == mode) {
        return MSPROAL_OK;
    }

#if         (1 == MSPROAL_SUPPORT_IFMODE)
    return MSPROAL_PARAM_ERR;
#elif       ((1 == MSPROAL_SUPPORT_PROHG) && (3 == MSPROAL_SUPPORT_IFMODE))
    switch(mode) {
    case MSPROAL_SERIAL_MODE:
        break;
    case MSPROAL_4PARALLEL_MODE:
        if(!(msifhndl->Stick & MSPROAL_STICK_S4P)) {
            return MSPROAL_PARAM_ERR;
        }

        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            mode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            mode = MSPROAL_V1_4PARALLEL_MODE;
        }
        break;
    case MSPROAL_8PARALLEL_MODE:
        if(!(msifhndl->Stick & MSPROAL_STICK_S8P_COM)) {
            return MSPROAL_PARAM_ERR;
        }

        if(MSPROAL_SERIAL_MODE == ifmode) {
            result = msproal_msif_change_ifmode(msifhndl,
                                                MSPROAL_PRO_4PARALLEL_MODE);
            if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
                data    = msproal_error.ErrInfo
                        + (msproal_error.ErrCnt % msproal_error.ErrNum)
                        * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
                msproal_drv_common_write_error_info(msifhndl,
                                                    MSPROAL_ID_CONTROL_IFMODE,
                                                    result,
                                                    &msproal_error.ErrCnt,
                                                    data);

                return result;
            }
        }
        break;
    default:
        return MSPROAL_PARAM_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = msproal_msif_change_ifmode(msifhndl, mode);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_CONTROL_IFMODE,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
#else   /*  (2 == MSPROAL_SUPPORT_IFMODE)   */
    switch(mode) {
    case MSPROAL_SERIAL_MODE:
        break;
    case MSPROAL_4PARALLEL_MODE:
        if(!(msifhndl->Stick & MSPROAL_STICK_S4P)) {
            return MSPROAL_PARAM_ERR;
        }

        if(msifhndl->Stick & (MSPROAL_STICK_PRO | MSPROAL_STICK_XC)) {
            mode = MSPROAL_PRO_4PARALLEL_MODE;
        } else {
            mode = MSPROAL_V1_4PARALLEL_MODE;
        }
        break;
    default:
        return MSPROAL_PARAM_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = msproal_msif_change_ifmode(msifhndl, mode);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_CONTROL_IFMODE,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
#endif  /*  (1 == MSPROAL_SUPPORT_IFMODE)   */
}

/******************************************************************************
*   FUNCTION    : msproal_control_power_class
*   DESCRIPTION : Control the Power class of Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_control_power_class(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       mode        : Power class mode
******************************************************************************/
SINT msproal_control_power_class(SINT mode)
{
#if         (1 == MSPROAL_SUPPORT_XC)
    SINT    result;
    SINT    pwrcls;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(msifhndl->PowerClass == mode) {
        return MSPROAL_OK;
    }

#if         (1 == MSPROAL_SUPPORT_XC)
    pwrcls = (msifhndl->Stick & MSPROAL_STICK_PC_MASK) >> 9;
    if(pwrcls < mode) {
        return MSPROAL_PARAM_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = (*(msproal_func->change_power))(msifhndl, mode);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_CONTROL_POWER_CLASS,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
    return MSPROAL_PARAM_ERR;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
}

/******************************************************************************
*   FUNCTION    : msproal_control_power_supply
*   DESCRIPTION : Control Memory Stick power supply.
*------------------------------------------------------------------------------
*   SINT msproal_control_power_supply(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       mode        : Mode(MSPROAL_POWER_ON/MSPROAL_POWER_OFF)
******************************************************************************/
SINT msproal_control_power_supply(SINT mode)
{
    return msproal_msif_control_power(msifhndl, mode);
}

/******************************************************************************
*   FUNCTION    : msproal_format
*   DESCRIPTION : Format Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_format(SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       mode        : Mode(MSPROAL_QUICK_FORMAT/MSPROAL_FULL_FORMAT)
******************************************************************************/
SINT msproal_format(SINT mode)
{
    SINT    result, result_stop;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

#if         ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))
    result = msproal_wakeup();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  ((1 == MSPROAL_AUTO_WAKEUP) || (1 == MSPROAL_AUTO_SLEEP))   */

    result = (*(msproal_func->format))(msifhndl, mode);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_FORMAT,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        if(MSPROAL_EXTRACT_ERR == result) {
            return MSPROAL_EXTRACT_ERR;
        }

        result_stop = (*(msproal_func->stop))(msifhndl);
        if(MSPROAL_OK != result_stop) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
            data    = msproal_error.ErrInfo
                    + (msproal_error.ErrCnt % msproal_error.ErrNum)
                    * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
            msproal_drv_common_write_error_info(msifhndl,
                                                MSPROAL_ID_STOP,
                                                result_stop,
                                                &msproal_error.ErrCnt,
                                                data);

            return result_stop;
        }

        return result;
    }

#if         (1 == MSPROAL_AUTO_SLEEP)
    result = msproal_sleep();
    if(MSPROAL_OK != result) {
        return result;
    }
#endif  /*  (1 == MSPROAL_AUTO_SLEEP)   */

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_sleep
*   DESCRIPTION : Memory Stick is put to sleep status.
*------------------------------------------------------------------------------
*   SINT msproal_sleep(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_sleep(void)
{
    SINT    result;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    result = (*(msproal_func->sleep))(msifhndl);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_SLEEP,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_wakeup
*   DESCRIPTION : Memory Stick wakes up from sleep status.
*------------------------------------------------------------------------------
*   SINT msproal_wakeup(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_wakeup(void)
{
    SINT    result;
    ULONG   *data;

    data = (ULONG *)MSPROAL_NULL;

    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    if(!(msifhndl->Status & MSPROAL_STTS_MOU_STORAGE)) {
        return MSPROAL_UNMOUNT_ERR;
    }

    result = (*(msproal_func->wakeup))(msifhndl);
    if(MSPROAL_OK != result) {
#if         (1 == MSPROAL_ACQUIRE_ERROR)
        data    = msproal_error.ErrInfo
                + (msproal_error.ErrCnt % msproal_error.ErrNum)
                * MSPROAL_OFFSET_ERROR_INFO;
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)    */
        msproal_drv_common_write_error_info(msifhndl,
                                            MSPROAL_ID_WAKEUP,
                                            result,
                                            &msproal_error.ErrCnt,
                                            data);

        return result;
    }

    return MSPROAL_OK;
}

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_write_pout_reg
*   DESCRIPTION : Writes data in the Parallel Output Register
*------------------------------------------------------------------------------
*   SINT msproal_write_pout_reg(ULONG data, ULONG mask)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_UNMOUNT_ERR         : Unmount error
*   ARGUMENT
*       data            : Data to write
*       mask            : Mask to set value
******************************************************************************/
SINT msproal_write_pout_reg(ULONG data, ULONG mask)
{
    if(MSPROAL_NULL == msifhndl) {
        return MSPROAL_UNMOUNT_ERR;
    }

    return msproal_icon_write_pout_reg(msifhndl, data, mask);
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

void msproal_set_xfr_mode(UBYTE mode)
{
	msifhndl->XfrMode = mode;
}
