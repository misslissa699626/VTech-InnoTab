/*=============================================================================
* Copyright 2002-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_mgralif.h
*
* DESCRIPTION   : MSPROAL_MGRALI layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_MGRALIF_H
#define     __MSPROAL_MGRALIF_H

#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
extern int msproal_mgralif_init(void);
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_MGRALIF_H */
