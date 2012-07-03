/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_config.h
*
* DESCRIPTION   : Configuration of MSPROAL Environment
=============================================================================*/
#ifndef     __MSPROAL_CONFIG_H
#define     __MSPROAL_CONFIG_H

#define PLATFORM_GPL32900 0
#define PLATFORM          PLATFORM_GPL32900

/* Supporting MG (Not=0, Supporting=1) */
#define MSPROAL_SUPPORT_MG      0

/* Data bus width (2bytes/16bits = 2, 4bytes/32bits = 4) */
#define MSPROAL_ALIGN_BYTES     4

#define MSPROAL_RETRY_COUNT     3   /* 0            : Not retry             */
                                    /* More than 1  : Retry number of times */

/* Memory Stick automatically wakeup */
#define MSPROAL_AUTO_WAKEUP     0   /* 0 : Not wakeup                       */
                                    /* 1 : Wakeup                           */

/* Memory Stick automatically sleep (And wakeup)                        */
/* MSPROAL_AUTO_WAKEUP setting value invalid when MSPROAL_AUTO_SLEEP is */
/* setted to 1.                                                         */
#define MSPROAL_AUTO_SLEEP      0   /* 0 : Not sleep                        */
                                    /* 1 : Sleep                            */

/* Check MBR and PBR of Memory Stick V1.X for Matching required item */
#define MSPROAL_CHECK_MPBR      0   /* 0 : Not check                        */
                                    /* 1 : Check                            */

/* Supporting Memory Stick V1.X (Not=0, Supporting=1) */
#define MSPROAL_SUPPORT_V1      1

/* Supporting DMA Controller(Not=0, Supporting=1) */
#define MSPROAL_SUPPORT_DMA     0

/* DMA 1ch (2ch=2, 1ch=1) */
#define MSPROAL_DMA_CHANNELS    2

#define MSPROAL_DMA_SLICE_SIZE  6   /* 1 : 4 Byte                           */
                                    /* 2 : 16 Byte                          */
                                    /* 3 : 32 Byte                          */
                                    /* 4 : 64 Byte                          */
                                    /* 5 : 128 Byte                         */
                                    /* 6 : 256 Byte                         */

/* Supporting Memory Stick PRO Expansion (Not=0, Supporting=1) */
#define MSPROAL_SUPPORT_PROHG   0

/* Supporting Memory Stick XC (Not=0, Supporting=1) */
#define MSPROAL_SUPPORT_XC      1

/* Supporting Memory Stick Controller */
#define MSPROAL_SUPPORT_IP      3   /* 1 : smshc                            */
                                    /* 2 : smshc_i                          */
                                    /* 3 : sms2ip                           */
                                    /* 4 : sms2ip + I-CON                   */
                                    /* 5 : smshc_id                         */

/* Memory Stick interface mode */
#define MSPROAL_SUPPORT_IFMODE  2   /* 1 : Serial interface mode            */
                                    /* 2 : 4bit parallel interface mode     */
                                    /* 3 : 8bit parallel interface mode     */

#define MSPROAL_ACCESS_TIME     10

#define MSPROAL_RESET_TIME      10

/* Acquire error when it occurs in MSPROAL Standard Function */
#define MSPROAL_ACQUIRE_ERROR   0   /* 0 : Not acquire                      */
                                    /* 1 : Acquire                          */

/* Supporting Virtual Memory (Not=0, Supporting=1) */
#define MSPROAL_SUPPORT_VMEM    1

#endif  /*  __MSPROAL_CONFIG_H  */
