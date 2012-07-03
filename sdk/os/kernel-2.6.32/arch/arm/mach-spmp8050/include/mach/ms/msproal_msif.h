/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_msif.h
*
* DESCRIPTION   : MSIF layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_MSIF_H
#define     __MSPROAL_MSIF_H

#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_port.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
    Declaration prototype
******************************************************************************/
extern SINT msproal_msif_change_ifmode(MSIFHNDL *, SINT);
extern SINT msproal_msif_control_power(MSIFHNDL *, SINT);
extern SINT msproal_msif_ex_set_cmd(MSIFHNDL *, SINT, ULONG, UWORD, SINT);
extern SINT msproal_msif_get_ifmode(MSIFHNDL *, SINT *);
extern SINT msproal_msif_get_int(MSIFHNDL *, SINT, UBYTE *);
extern SINT msproal_msif_read_reg(MSIFHNDL *, SINT, SINT, UBYTE *);
extern SINT msproal_msif_reset_host(MSIFHNDL *);
extern SINT msproal_msif_set_cmd(MSIFHNDL *, SINT, SINT);
#if         (1 == MSPROAL_SUPPORT_V1)
extern SINT msproal_msif_set_para_extra(MSIFHNDL *, SINT, SINT, SINT, UBYTE *,
                SINT);
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */
extern SINT msproal_msif_write_reg(MSIFHNDL *, SINT, SINT, UBYTE *);

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_MSIF_H    */
