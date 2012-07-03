/*=============================================================================
* Copyright 2002-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : tpc.h
*
* DESCRIPTION   : TPC layer - Declaration prototype
=============================================================================*/
#ifndef     __TPC_H
#define     __TPC_H

#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
extern int tpc_set_cmd(MSHNDL *, int, int);
extern int tpc_read_mg_stts_reg(MSHNDL *, unsigned char *);
extern int tpc_read_mgd_reg(MSHNDL *, unsigned char *, unsigned char *,
            unsigned char *);
extern int tpc_write_mgd_reg(MSHNDL *, unsigned char *, unsigned char *,
            unsigned char *, unsigned char *);
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#ifdef __cplusplus
}
#endif

#endif  /*  __TPC_H     */
