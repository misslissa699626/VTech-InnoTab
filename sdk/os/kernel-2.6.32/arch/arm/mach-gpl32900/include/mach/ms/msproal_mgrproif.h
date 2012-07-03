/*=============================================================================
* Copyright 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_mgrproif.h
*
* DESCRIPTION   : MSPROAL_MGRPROIF layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_MGRPROIF_H
#define     __MSPROAL_MGRPROIF_H

#include <mach/ms/msproal_types.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if         (1 == MSPROAL_SUPPORT_MG)
extern SINT msproal_mgrproif_do_read_cmd(MSIFHNDL *, const UBYTE *,
                const UBYTE *, const UBYTE *, UBYTE  *, SINT);
extern SINT msproal_mgrproif_do_write_cmd(MSIFHNDL *, const UBYTE *,
                const UBYTE *, const UBYTE *, UBYTE  *, SINT);
#endif  /*  (1 == MSPROAL_SUPPORT_MG)   */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_MGRPROIF_H    */
