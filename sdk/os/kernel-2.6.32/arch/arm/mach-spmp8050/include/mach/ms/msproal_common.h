/*=============================================================================
* Copyright 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_common.h
*
* DESCRIPTION   : MSPROAL_COMMON layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_COMMON_H
#define     __MSPROAL_COMMON_H

#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_defs.h>
#include <mach/ms/ms_defs.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
    Declaration prototype
******************************************************************************/
extern SINT msproal_drv_common_init(MSIFHNDL *,void *);
extern SINT msproal_drv_common_media_identification(MSIFHNDL *, ULONG *,
                SINT *, SINT *);
#if         (1 == MSPROAL_ACQUIRE_ERROR)
extern void msproal_drv_common_clear_error_info(MSIFHNDL *, UWORD, ULONG *,
                ULONG *);
#endif  /*  (1 == MSPROAL_ACQUIRE_ERROR)  */
extern void msproal_drv_common_write_error_info(MSIFHNDL *, ULONG, SINT,
                ULONG *, ULONG *);

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_COMMON_H  */
