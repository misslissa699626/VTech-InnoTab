/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_types.h
*
* DESCRIPTION   : Definition type
=============================================================================*/
#ifndef     __MSPROAL_TYPES_H
#define     __MSPROAL_TYPES_H

#include <mach/ms/msproal_config.h>

typedef char            SBYTE;
typedef short           SWORD;
typedef long            SLONG;
typedef int             SINT;
typedef unsigned char   UBYTE;
typedef unsigned short  UWORD;
typedef unsigned long   ULONG;
typedef unsigned int    UINT;

#if         (0 == MSPROAL_SUPPORT_V1)
typedef struct  __msifhndl {
    UBYTE           WorkArea[512 * 4];      /* Work Area                    */
    UBYTE           DataBuf[512];           /* Data Buffer                  */
    UBYTE           BootData[512];          /* Boot Block data              */
    UBYTE           XfrMode;
    ULONG           BaseAdrs;               /* IP Core base address         */
    ULONG           HidSct;                 /* Hidden sector number         */
    ULONG           Stick;                  /* Memory Stick media type      */
    ULONG           Status;                 /* Status                       */
    volatile ULONG  IntState;
    SINT            IfMode;
    SINT            IfModeMax;
    SINT            PowerClass;
    SINT            Rw;                     /* Read/Write : 1 Read Only : 0 */
    SINT            Wp;                     /* Write-Protect ON : 1 OFF : 0 */
} MSIFHNDL;
#else   /*  (1 == MSPROAL_SUPPORT_V1)   */
typedef struct  __msifhndl {
    UBYTE           WorkArea[512 * 4];      /* Work Area                    */
    UBYTE           DataBuf[512 * 2];       /* Data Buffer                  */
    UBYTE           BootData[512 * 2];      /* Boot Block data              */
    UBYTE           XfrMode;
    ULONG           BaseAdrs;               /* IP Core base address         */
    ULONG           HidSct;                 /* Hidden sector number         */
    ULONG           Stick;                  /* Memory Stick media type      */
    ULONG           Status;                 /* Status                       */
    volatile ULONG  IntState;
    SINT            BootBlk;                /* Boot Block number            */
    SINT            BkBootBlk;              /* Backup Boot Block number     */
    SINT            IfMode;
    SINT            IfModeMax;
    SINT            PowerClass;
    SINT            Rw;                     /* Read/Write : 1 Read Only : 0 */
    SINT            Wp;                     /* Write-Protect ON : 1 OFF : 0 */
    SINT            FreeBlkNum[16];         /* Free block number            */
    SINT            DisBlkNum[16];          /* Disable Block number         */
    UWORD           FreeBlkTbl[16 * 16];    /* Free block table             */
    UWORD           LogiPhyTbl[7934];       /* Logi/Phy translation table   */
    UWORD           DisBlkTbl[15 * 16];     /* Disable Block table          */
    SINT            UserAreaBlocks;         /* Number of user area block    */
    SINT            BlockSize;              /* Block size                   */
    SINT            PageSize;               /* Page size                    */
    SINT            UnitSize;               /* Unit size                    */
    SINT 			handle_dma;	
} MSIFHNDL;
#endif  /*  (0 == MSPROAL_SUPPORT_V1)   */

typedef struct  __msproal_msinfo {
    ULONG           Stick;                  /* Memory Stick media type      */
    ULONG           LogiCapa;
    ULONG           PhyCapa;
    SINT            BlockSize;
    SINT            Wp;                     /* Write-Protect ON : 1 OFF : 0 */
} MSPROAL_MSINFO;

typedef struct __msproal_error {
    ULONG           *ErrInfo;       /* Address to buffer to store error info */
    ULONG           ErrCnt;         /* Number of Errors  */
    UWORD           ErrNum;         /* Max number of stored error info */
} MSPROAL_ERROR;

typedef struct __func {
    SINT (*read_atrb)(MSIFHNDL *, ULONG, SINT, UBYTE *);
    SINT (*read)(MSIFHNDL *, ULONG, SINT, UBYTE *);
    SINT (*write)(MSIFHNDL *, ULONG, SINT, UBYTE *);
    SINT (*get_name)(MSIFHNDL *, SBYTE *);
    SINT (*change_power)(MSIFHNDL *, SINT);
    SINT (*format)(MSIFHNDL *, SINT);
    SINT (*mount)(MSIFHNDL *, SINT);
    SINT (*sleep)(MSIFHNDL *);
    SINT (*wakeup)(MSIFHNDL *);
    SINT (*recovery)(MSIFHNDL *);
    SINT (*stop)(MSIFHNDL *);
} FUNC;

#endif  /*  __MSPROAL_TYPES_H   */
