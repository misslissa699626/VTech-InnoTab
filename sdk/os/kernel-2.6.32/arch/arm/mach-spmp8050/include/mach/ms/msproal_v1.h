/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_v1.h
*
* DESCRIPTION   : MSPROAL_V1 layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_V1_H
#define     __MSPROAL_V1_H

#include <mach/ms/msproal_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
    Declaration prototype
******************************************************************************/
#if         (1 == MSPROAL_ACQUIRE_ERROR)
extern void msproal_drv_common_clear_error_info(MSIFHNDL *, UWORD, ULONG *,
                ULONG *);
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)  */
extern void msproal_drv_common_write_error_info(MSIFHNDL *, ULONG, SINT,
                ULONG *, ULONG *);

#if         (1 == MSPROAL_SUPPORT_V1)
extern SINT msproal_drv_v1_bootblock_confirmation(MSIFHNDL *);
extern SINT msproal_drv_v1_change_power(MSIFHNDL *, SINT);
extern SINT msproal_drv_v1_check_mpbr(MSIFHNDL *);
extern SINT msproal_drv_v1_complete_lptbl(MSIFHNDL *, SINT, SINT);
extern SINT msproal_drv_v1_generate_lptbl(MSIFHNDL *, SINT, UWORD *, SINT *);
extern SINT msproal_drv_v1_get_model_name(MSIFHNDL *, SBYTE *);
extern SINT msproal_drv_v1_ladrs_confirmation(MSIFHNDL *, SINT, UWORD *,
                SINT *);
extern SINT msproal_drv_v1_mount(MSIFHNDL *, SINT);
extern SINT msproal_drv_v1_protect_bootarea(MSIFHNDL *, UWORD *, SINT *);
extern SINT msproal_drv_v1_read_atrb_info(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_drv_v1_recovery(MSIFHNDL *);
extern SINT msproal_drv_v1_stop(MSIFHNDL *);
extern SINT msproal_drv_v1_wakeup(MSIFHNDL *);
extern SINT msproal_seq_v1_clear_buffer(MSIFHNDL *);
extern SINT msproal_seq_v1_copy_block(MSIFHNDL *, SINT, SINT, SINT, SINT);
extern SINT msproal_seq_v1_erase_block(MSIFHNDL *, SINT);
extern SINT msproal_seq_v1_overwrite_extradata(MSIFHNDL *, SINT, SINT,
                UBYTE *);
extern SINT msproal_seq_v1_read_block(MSIFHNDL *, SINT, SINT, SINT, UBYTE *,
                UBYTE *);
extern SINT msproal_seq_v1_read_bootblock(MSIFHNDL *);
extern SINT msproal_seq_v1_read_extradata(MSIFHNDL *, SINT, SINT, UBYTE *);
extern SINT msproal_seq_v1_reset(MSIFHNDL *);
extern SINT msproal_seq_v1_sleep(MSIFHNDL *);
extern SINT msproal_seq_v1_stop(MSIFHNDL *);
extern SINT msproal_seq_v1_write_block(MSIFHNDL *, SINT, SINT, SINT, UBYTE *,
                UBYTE *);
extern SINT msproal_seq_v1_write_extradata(MSIFHNDL *, SINT, SINT, UBYTE *);
extern SINT msproal_tbl_check_useblock(MSIFHNDL *, SINT);
extern SINT msproal_tbl_get_freeblock(MSIFHNDL *, SINT *, SINT);
extern SINT msproal_tbl_init_tbl(MSIFHNDL *, SINT);
extern SINT msproal_tbl_log_to_phy(MSIFHNDL *, SINT, UWORD, SINT *);
extern SINT msproal_tbl_update_freeblock(MSIFHNDL *, SINT, UWORD *, SINT);
extern SINT msproal_tbl_update_lptbl(MSIFHNDL *, SINT, UWORD, SINT);
extern SINT msproal_trans_copy_block(MSIFHNDL *, SINT, SINT, SINT, SINT,
                UBYTE *);
extern SINT msproal_trans_format(MSIFHNDL *, SINT);
extern SINT msproal_trans_read_lba(MSIFHNDL *, ULONG, SINT, UBYTE *);
extern SINT msproal_trans_update_block(MSIFHNDL *, UWORD, SINT, SINT, UBYTE *);
extern SINT msproal_trans_write_lba(MSIFHNDL *, ULONG, SINT, UBYTE *);
#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
extern SINT msproal_drv_v1_read_1seg_extradata(MSIFHNDL *, SINT, UBYTE *);
extern SINT msproal_drv_v1_read_nextradata(MSIFHNDL *, SINT, SINT, UBYTE *);
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_V1_H  */
