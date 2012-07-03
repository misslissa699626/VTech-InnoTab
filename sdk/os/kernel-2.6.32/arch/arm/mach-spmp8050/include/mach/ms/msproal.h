/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal.h
*
* DESCRIPTION   : MSPROAL layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_H
#define     __MSPROAL_H

#include <mach/ms/msproal_common.h>
#include <mach/ms/msproal_v1.h>
#include <mach/ms/msproal_pro.h>
#include <mach/ms/msproal_defs_old.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
    Declaration prototype
******************************************************************************/
extern SINT msproal_init(void);
extern SINT msproal_recovery(void);
extern SINT msproal_mount(SINT);
extern SINT msproal_start(UBYTE *);
extern SINT msproal_unmount(void);
extern SINT msproal_check_media(UBYTE *);
extern SINT msproal_check_stick(void);
extern SINT msproal_check_write_status(void);
extern SINT msproal_clear_error_info(void);
extern SINT msproal_get_ifmode(SINT *);
extern SINT msproal_get_info(MSPROAL_MSINFO *);
extern SINT msproal_get_model_name(SBYTE *);
extern SINT msproal_get_progress(SINT *);
extern SINT msproal_get_system_info(UBYTE *);
extern SINT msproal_read_atrb_info(ULONG, ULONG, UBYTE *);
extern SINT msproal_read_lba(ULONG, SINT, UBYTE *);
extern SINT msproal_read_sect(ULONG, SINT, UBYTE *);
extern SINT msproal_write_lba(ULONG, SINT, UBYTE *);
extern SINT msproal_write_sect(ULONG, SINT, UBYTE *);
extern SINT msproal_control_ifmode(SINT);
extern SINT msproal_control_power_class(SINT);
extern SINT msproal_control_power_supply(SINT);
extern SINT msproal_format(SINT);
extern SINT msproal_sleep(void);
extern SINT msproal_wakeup(void);
extern void msproal_set_xfr_mode(UBYTE mode);
#if         (5 == MSPROAL_SUPPORT_IP)
extern SINT msproal_write_pout_reg(ULONG, ULONG);
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

extern MSIFHNDL *msifhndl;
extern FUNC     *msproal_func;
extern FUNC     msproal_pro_func;
#if         (1 == MSPROAL_SUPPORT_V1)
extern FUNC     msproal_v1_func;
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
extern MSPROAL_ERROR    msproal_error;

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_H */
