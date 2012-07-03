/*=============================================================================
* Copyright 2002-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_tpc.h
*
* DESCRIPTION   : MSPROAL_TPC layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_TPC_H
#define     __MSPROAL_TPC_H

#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_user.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

extern SINT msproal_tpc_change_ifmode(MSIFHNDL *, SINT);
extern SINT msproal_tpc_ex_set_cmd(MSIFHNDL *,SINT, ULONG, UWORD);
extern SINT msproal_tpc_get_int(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_get_int1(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_read_fifo(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_read_nfifo(MSIFHNDL *, SINT, UBYTE *);
extern SINT msproal_tpc_read_page(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_read_reg(MSIFHNDL *, SINT, UBYTE *);
extern SINT msproal_tpc_read_short_data(MSIFHNDL *, SINT, UBYTE *);
extern SINT msproal_tpc_reset_host(MSIFHNDL *);
extern void msproal_tpc_set_bsycnt(MSIFHNDL *, SINT);
extern SINT msproal_tpc_set_cmd(MSIFHNDL *, SINT);
extern SINT msproal_tpc_set_rw_reg_adrs(MSIFHNDL *, SINT, SINT, SINT, SINT);
extern SINT msproal_tpc_set_tpc(MSIFHNDL *, SINT, SINT);
extern SINT msproal_tpc_wait_int(MSIFHNDL *, SINT, SINT);
extern SINT msproal_tpc_write_fifo(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_write_nfifo(MSIFHNDL *, SINT , UBYTE *);
extern SINT msproal_tpc_write_page(MSIFHNDL *,UBYTE *);
extern SINT msproal_tpc_write_reg(MSIFHNDL *, SINT , UBYTE *);
extern SINT msproal_tpc_write_short_data(MSIFHNDL *, SINT , UBYTE *);
#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
extern SINT msproal_tpc_read_mgd_reg(MSIFHNDL *, UBYTE *, UBYTE *, UBYTE *);
extern SINT msproal_tpc_read_mg_stts_reg(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_write_mgd_reg(MSIFHNDL *, UBYTE *, UBYTE *, UBYTE *,
                UBYTE *);
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */
#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_PROHG)
extern SINT msproal_tpc_read_quad_long_data(MSIFHNDL *, UBYTE *);
extern SINT msproal_tpc_write_quad_long_data(MSIFHNDL *, UBYTE *);
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_TPC_H     */
