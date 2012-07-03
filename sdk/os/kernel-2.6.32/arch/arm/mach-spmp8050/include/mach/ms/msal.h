/*=============================================================================
* Copyright 2002-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msal.h
*
* DESCRIPTION   : USER API header
=============================================================================*/
#ifndef     __MSAL_H
#define     __MSAL_H

#include <mach/ms/msproal.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/* dummy */
typedef unsigned char   MSHNDL;

/******************************************************************************
*   DEFINE
******************************************************************************/
#define MSIF_GETINT             1

extern int msal_read_block(int, int, int, unsigned char *,
            unsigned short *);
extern int msal_read_extradata(int, int, unsigned short *);
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSAL_H    */
