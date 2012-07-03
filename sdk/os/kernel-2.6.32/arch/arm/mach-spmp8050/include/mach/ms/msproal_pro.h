/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_pro.h
*
* DESCRIPTION   : MSPROAL_PRO layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_PRO_H
#define     __MSPROAL_PRO_H

#include <mach/ms/msproal_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
    Declaration prototype
******************************************************************************/
extern SINT msproal_drv_pro_attribute_confirmation(MSIFHNDL *);
extern SINT msproal_drv_pro_check_mpbr(MSIFHNDL *);
extern SINT msproal_drv_pro_check_mbsr(MSIFHNDL *);
extern SINT msproal_drv_pro_get_model_name(MSIFHNDL *, SBYTE *);
extern SINT msproal_drv_pro_mount(MSIFHNDL *, SINT);
extern SINT msproal_drv_pro_read_data(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_drv_pro_recovery(MSIFHNDL *);
extern SINT msproal_drv_pro_stop(MSIFHNDL *);
extern SINT msproal_drv_pro_wakeup(MSIFHNDL *);
extern SINT msproal_drv_pro_write_data(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_seq_pro_change_power(MSIFHNDL *, SINT);
extern SINT msproal_seq_pro_erase(MSIFHNDL *, ULONG, SINT);
extern SINT msproal_seq_pro_format(MSIFHNDL *, SINT);
extern SINT msproal_seq_pro_read_atrb(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_seq_pro_read_data(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_seq_pro_startup(MSIFHNDL *);
extern SINT msproal_seq_pro_sleep(MSIFHNDL *);
extern SINT msproal_seq_pro_write_data(MSIFHNDL *, ULONG, SINT, UBYTE *);
#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_PROHG)
extern SINT msproal_seq_pro_read_2k_data(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_seq_pro_write_2k_data(MSIFHNDL *, ULONG, SINT, UBYTE *);
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */
#if         (1 == MSPROAL_SUPPORT_XC)
extern SINT msproal_drv_pro_check_mbsr(MSIFHNDL *);
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_PRO_H */
