/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_defs.h
*
* DESCRIPTION   : Definition, Macro definition
=============================================================================*/
#ifndef     __MSPROAL_DEFS_H
#define     __MSPROAL_DEFS_H

/******************************************************************************
*   DEFINE
******************************************************************************/
#define MSPROAL_READ                0
#define MSPROAL_WRITE               1
#define MSPROAL_WMSINT              0
#define MSPROAL_WRDY                1
#define MSPROAL_WTIME               2
#define MSPROAL_WDMA                3
#define MSPROAL_WMSINT_SERIAL       4
#define MSPROAL_WMSINT_PARALLEL     5
#define MSPROAL_SERIAL_MODE         1
#define MSPROAL_4PARALLEL_MODE      2
#define MSPROAL_V1_4PARALLEL_MODE   3
#define MSPROAL_PRO_4PARALLEL_MODE  4
#define MSPROAL_8PARALLEL_MODE      5
#define MSPROAL_READ_ONLY           0
#define MSPROAL_READ_WRITE          1
#define MSPROAL_QUICK_FORMAT        0
#define MSPROAL_FULL_FORMAT         1
#define MSPROAL_INC_SADR            0   /* Increment to source address      */
#define MSPROAL_INC_DADR            1   /* Increment to destination address */
#define MSPROAL_SELECT_GDFIFO       1   /* Channel to be selected is GDFIFO */
#define MSPROAL_SELECT_PBUFF        2   /* Channel to be selected is PBuff  */
#define MSPROAL_SELECT_DATA         3   /* Channel to be selected is DataReg*/

#define MSPROAL_WICON               1

#define MSPROAL_CLEAR_INTSTATE      0x0
#define MSPROAL_SIZEOF_INIT         sizeof(MSIFHNDL)
#define MSPROAL_SIZEOF_ERROR_INFO   128
#define MSPROAL_OFFSET_ERROR_INFO   32
#define MSPROAL_MAX_TRANS_NUM       32 * 512 * 16
#define MSPROAL_NULL                (void *)0

/******************************************************************************
    MSPROAL STICK DISCRIMINATION
******************************************************************************/
#define MSPROAL_STICK_V1            0x00000010  /* Memory Stick             */
#define MSPROAL_STICK_PRO           0x00000020  /* Memory Stick PRO         */
#define MSPROAL_STICK_IOEX          0x00000040  /* Memory Stick I/O         */
                                                /* Expansion                */
#define MSPROAL_STICK_PRO_IOEX      0x00000080  /* Memory Stick PRO I/O     */
                                                /* Expansion                */
#define MSPROAL_STICK_XC            0x00000100  /* Memory Stick XC          */
#define MSPROAL_STICK_UNKNOWN       0x00000000  /* Other Sticks out of      */
                                                /* Memory Stick Standard    */
#define MSPROAL_STICK_RW            0x00000001  /* Memory Stick Read/Write  */
#define MSPROAL_STICK_ROM           0x00000002  /* Memory Stick ROM         */
#define MSPROAL_STICK_R             0x00000004  /* Memory Stick -R          */
#define MSPROAL_STICK_ROM_ETC       0x00000008  /* Memory Stick ROM_ETC     */
#define MSPROAL_STICK_DEVICE_MASK   0x0000000F  /* Mask Data                */
#define MSPROAL_STICK_S8P_STICK     0x00008000  /* Supporting 8bit Parallel */
                                                /* Interface Memory Stick   */
#define MSPROAL_STICK_S8P_COM       0x00004000  /* Supporting 8bit Parallel */
                                                /* Interface Communication  */
#define MSPROAL_STICK_S4P           0x00002000  /* Supporting 4bit Parallel */
                                                /* Interface                */
#define MSPROAL_STICK_2K            0x00000800  /* Supporting 2K Command    */
#define MSPROAL_STICK_PC_MASK       0x00000600  /* Mask Data                */

/******************************************************************************
    MSPROAL FLAG DISCRIMINATION
******************************************************************************/
#define MSPROAL_FLG_EXTRACT         0x80000000
#define MSPROAL_FLG_DMA             0x40000000
#define MSPROAL_FLG_ICON            0x20000000

/******************************************************************************
    MSPROAL STATUS DISCRIMINATION
******************************************************************************/
#define MSPROAL_STTS_MOU_STORAGE    0x00000001

/******************************************************************************
    MSPROAL RETURN CODE DEFINE
******************************************************************************/
#define MSPROAL_OK                  0       /* Normal */
#define MSPROAL_FORMAT_WARN         1       /* Format warning */
#define MSPROAL_IFMODE_WARN         2       /* Interface mode warning */
#define MSPROAL_PARAM_ERR           -1      /* Parameter error */
#define MSPROAL_RO_ERR              -2      /* Read Only error */
#define MSPROAL_ACCESS_ERR          -3      /* Access error */
#define MSPROAL_SYSTEM_ERR          -4      /* System error */
#define MSPROAL_MEDIA_ERR           -5      /* Media error */
#define MSPROAL_FORMAT_ERR          -6      /* Format error */
#define MSPROAL_UNSUPPORT_ERR       -7      /* Unsupport error */
#define MSPROAL_READ_PROTECTED_ERR  -8      /* Read Protected error */
#define MSPROAL_LOCKED_ERR          -9      /* Read/Write protect error */
#define MSPROAL_WRITE_ERR           -10     /* Write error */
#define MSPROAL_READ_ERR            -11     /* Read error */
#define MSPROAL_EXTRACT_ERR         -12     /* Media extract */
#define MSPROAL_UNMOUNT_ERR         -13     /* Media extract */
#define MSPROAL_ERR                 -101    /* Abnormal */
#define MSPROAL_HOST_CRC_ERR        -102    /* CRC error */
#define MSPROAL_HOST_TOE_ERR        -103    /* TOE(Time Out Error) error */
#define MSPROAL_TIMEOUT_ERR         -104    /* Timeout error */
#define MSPROAL_CMDNK_ERR           -105    /* Command nack */
#define MSPROAL_FLASH_ERR           -106    /* FlashError(Read,Write,Erase) */
#define MSPROAL_FLASH_READ_ERR      -107    /* FlashReadError */
#define MSPROAL_FLASH_WRITE_ERR     -110    /* FlashWriteError */
#define MSPROAL_FLASH_ERASE_ERR     -111    /* FlashEraseError */
#define MSPROAL_MC_FLASH_READ_ERR   -13     /* FlashReadError(MicroCode) */
#define MSPROAL_MC_FLASH_WRITE_ERR  -14     /* FlashWriteError(MicroCode) */

/******************************************************************************
    MSPROAL FUNCTION ID DEFINE
******************************************************************************/
#define MSPROAL_ID_START                    0x0001
#define MSPROAL_ID_MOUNT                    0x0002
#define MSPROAL_ID_READ_SECT                0x0003
#define MSPROAL_ID_WRITE_SECT               0x0004
#define MSPROAL_ID_FORMAT                   0x0005
#define MSPROAL_ID_RECOVERY                 0x0006
#define MSPROAL_ID_WAKEUP                   0x0007
#define MSPROAL_ID_SLEEP                    0x0008
#define MSPROAL_ID_CONTROL_IFMODE           0x0009
#define MSPROAL_ID_READ_LBA                 0x000B
#define MSPROAL_ID_WRITE_LBA                0x000C
#define MSPROAL_ID_STOP                     0x000D
#define MSPROAL_ID_CHECK_MEDIA              0x0012
#define MSPROAL_ID_GET_MODEL_NAME           0x0013
#define MSPROAL_ID_READ_ATRB_INFO           0x0014
#define MSPROAL_ID_GET_IFMODE               0x0015
#define MSPROAL_ID_CONTROL_POWER_CLASS      0x0016

/******************************************************************************
    TRUE/FALSE Flag
******************************************************************************/
#define MSPROAL_TRUE                1
#define MSPROAL_FALSE               0

/******************************************************************************
    Power Control
******************************************************************************/
#define MSPROAL_POWER_ON            1
#define MSPROAL_POWER_OFF           0

/******************************************************************************
    Extra Data
******************************************************************************/
#define MSPROAL_OVFLG_ADRS          0
#define MSPROAL_MANAFLG_ADRS        1
#define MSPROAL_LADRS1_ADRS         2
#define MSPROAL_LADRS0_ADRS         3

/******************************************************************************
    Write Protect Switch
******************************************************************************/
#define MSPROAL_WP_ON               1
#define MSPROAL_WP_OFF              0

/******************************************************************************
    MEMORY STICK PRO DEFINE
******************************************************************************/
#define MSPROAL_BOOT_ATTR_ADRS      0x1A0

/******************************************************************************
    Logical/Physical transformation table define
******************************************************************************/
#define MSPROAL_BLOCK_NOT_EXIST     0xFFFF

/******************************************************************************
    Macro definition
******************************************************************************/
/* Swap high byte, low byte */
#undef SWAPWORD
#define SWAPWORD(data)          (((UBYTE)(data) << 8) | ((UWORD)(data) >> 8))
/* Make long value from low word value and high word value */
#undef MAKELONG
#define MAKELONG(wlo, whi)      ((ULONG)(((UWORD)(wlo)) | ((ULONG)(((UWORD)(whi)) << 16))))
/* Make word value from high byte value and low byte value */
#undef MAKEWORD
#define MAKEWORD(hi, lo)        ((UWORD)(((UBYTE)(lo)) | ((UWORD)(((UBYTE)(hi)) << 8))))
/* Get low byte value from word value */
#undef LOBYTE
#define LOBYTE(w)               ((UBYTE)(w))
/* Get high byte value from word value */
#undef HIBYTE
#define HIBYTE(w)               ((UBYTE)(((UWORD)(w) >> 8) & 0xFF))
/* Get low word value from double word value */
#undef LOWORD
#define LOWORD(l)               ((UWORD)(l))
/* Get high word value from double word value */
#undef HIWORD
#define HIWORD(l)               ((UWORD)(((ULONG)(l) >> 16) & 0xFFFF))

#endif  /*  __MSPROAL_DEFS_H    */
