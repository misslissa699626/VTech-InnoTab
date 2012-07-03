/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : ms_defs.h
*
* DESCRIPTION   : Definition, Macro definition
=============================================================================*/
#ifndef     __MS_DEFS_H
#define     __MS_DEFS_H

#include <mach/ms/msproal_config.h>
#include <mach/ms/msproal_user.h>

/******************************************************************************
    TPC CODE
******************************************************************************/
#define TPC_READ_PAGE_DATA          0x02
#define TPC_READ_REG                0x04
#define TPC_GET_INT                 0x07
#define TPC_WRITE_PAGE_DATA         0x0D
#define TPC_WRITE_REG               0x0B
#define TPC_SET_RW_REG_ADRS         0x08
#define TPC_SET_CMD                 0x0E
#define TPC_READ_QUAD_LONG_DATA     0x15
#define TPC_WRITE_QUAD_LONG_DATA    0x1A

#if PLATFORM == PLATFORM_GPL32900
#define TPC_EX_SET_CMD              0x09
#define TPC_READ_SHORT_DATA         0x03
#define TPC_WRITE_SHORT_DATA        0x0C
#else
#define TPC_EX_SET_CMD              0x19
#define TPC_READ_SHORT_DATA         0x13
#define TPC_WRITE_SHORT_DATA        0x1C
#endif

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
#define TPC_READ_MG_STTS_REG        0x01
#define TPC_READ_MGD_REG            0x03
#define TPC_WRITE_MGD_REG           0x0C
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */


/******************************************************************************
    MS V1.X REGISTER OF BIT FIELD DEFINE
******************************************************************************/
/*--    INT Register                --*/
#define MS_INT_CED              0x80        /* CED bit */
#define MS_INT_ERR              0x40        /* ERR bit */
#define MS_INT_BREQ             0x20        /* BREQ bit */
#define MS_INT_CMDNK            0x01        /* CMDNK bit */

/*--    Status Register0            --*/
#define MS_STTS0_MB             0x80        /* MB bit */
#define MS_STTS0_FB0            0x40        /* FB0 bit */
#define MS_STTS0_BE             0x20        /* BE bit */
#define MS_STTS0_BF             0x10        /* BF bit */
#define MS_STTS0_SL             0x02        /* SL bit */
#define MS_STTS0_WP             0x01        /* WP bit */

/*--    Status Register1            --*/
#define MS_STTS1_MB             0x80        /* MB bit */
#define MS_STTS1_FB1            0x40        /* FB1 bit */
#define MS_STTS1_DTER           0x20        /* DTER bit */
#define MS_STTS1_UCDT           0x10        /* UCDT bit */
#define MS_STTS1_EXER           0x08        /* EXER bit */
#define MS_STTS1_UCEX           0x04        /* UCEX bit */
#define MS_STTS1_FGER           0x02        /* FGER bit */
#define MS_STTS1_UCFG           0x01        /* UCFG bit */

/*--    System Parameter            --*/
#define MS_SYSPARA_ATEN         0xc0        /* ATEN bit */
#define MS_SYSPARA_BAMD         0x80        /* BAMD bit */
#define MS_SYSPARA_PAM          0x08        /* PAM bit */

/*--    Command Parameter           --*/
#define MS_CMDPARA_BLOCK        0x00
#define MS_CMDPARA_SINGLE       0x20
#define MS_CMDPARA_EXDATA       0x40
#define MS_CMDPARA_OVERWRITE    0x80

/*--    OverwriteFlag               --*/
#define MS_OVFLG_BKST           0x80        /* BKST bit */
#define MS_OVFLG_PGST0          0x40        /* PGST0 bit */
#define MS_OVFLG_PGST1          0x20        /* PGST1 bit */
#define MS_OVFLG_UDST           0x10        /* UDST bit */

/*--    ManagementFlag              --*/
#define MS_MANAFLG_SCMS0        0x20        /* SCMS0 bit */
#define MS_MANAFLG_SCMS1        0x10        /* SCMS1 bit */
#define MS_MANAFLG_ATFLG        0x08        /* ATFLG bit */
#define MS_MANAFLG_SYSFLG       0x04        /* SYSFLG bit */


/******************************************************************************
    MS PRO REGISTER OF BIT FIELD DEFINE
******************************************************************************/
/*--    INT Register                --*/
#define MS2_INT_CED             0x80        /* CED bit */
#define MS2_INT_ERR             0x40        /* ERR bit */
#define MS2_INT_BREQ            0x20        /* BREQ bit */
#define MS2_INT_CMDNK           0x01        /* CMDNK bit */

/*--    Status Register             --*/
#define MS2_STTS_SL             0x02        /* SL bit */
#define MS2_STTS_WP             0x01        /* WP bit */

/*--    IF Mode Register            --*/
#define MS2_IFMODE_IFMOD_PROHG  0x07        /* PRO with extended function */
#define MS2_IFMODE_IFMOD_MASK   0x07        /* Interface mode bit mask  */

/*--    System Parameter            --*/
#define MS2_SYSPARA_SRAC        0x80        /* SRAC bit */


/******************************************************************************
    MS V1.X SET_R/W_REG_ADRS TPC
******************************************************************************/
/*--    Default value           --*/
#define MS_DEFAULT_READ_ADRS    0x00        /* The starting address read by */
                                            /* REDA_REG                     */
#define MS_DEFAULT_READ_SIZE    0x1F        /* The number of bytes read by  */
                                            /* REDA_REG                     */
#define MS_DEFAULT_WRITE_ADRS   0x10        /* The starting address written */
                                            /* by WRITE_REG                 */
#define MS_DEFAULT_WRITE_SIZE   0x0F        /* The number of bytes written  */
                                            /* by WRITE_REG                 */

/******************************************************************************
    MS PRO SET_R/W_REG_ADRS TPC
******************************************************************************/
/*--    Default value           --*/
#define MS2_DEFAULT_READ_ADRS   0x00        /* The starting address read by */
                                            /* REDA_REG                     */
#define MS2_DEFAULT_READ_SIZE   0x08        /* The number of bytes read by  */
                                            /* REDA_REG                     */
#define MS2_DEFAULT_WRITE_ADRS  0x10        /* The starting address written */
                                            /* by WRITE_REG                 */
#define MS2_DEFAULT_WRITE_SIZE  0x10        /* The number of bytes written  */
                                            /* by WRITE_REG                 */

/*--    MS V1.X Register address        --*/
#define MS_STATUS_REG0_ADRS     0x02        /* Status Register0     */
#define MS_STATUS_REG1_ADRS     0x03        /* Status Register1     */
#define MS_TYPE_REG_ADRS        0x04        /* Tyep   Register      */
#define MS_CATEGORY_REG_ADRS    0x06        /* Category Register    */
#define MS_CLASS_REG_ADRS       0x07        /* Class  Register      */
#define MS_SYSTEM_PARAM_ADRS    0x10        /* System Parameter     */
#define MS_BLOCK_ADDRESS2_ADRS  0x11        /* Block Address2       */
#define MS_PAGE_ADDRESS_ADRS    0x15        /* Page Address         */
#define MS_OVERWRITE_FLAG_ADRS  0x16        /* OverwriteFlag        */

/*--    MS PRO Register address         --*/
#define MS2_STATUS_REG_ADRS     0x02        /* Status Register      */
#define MS2_IF_MODE_REG_ADRS    0x05        /* IF Mode Register     */
#define MS2_SYSTEM_PARAM_ADRS   0x10        /* System Parameter     */
#define MS2_TPC_PARAM_ADRS      0x17        /* TPC Parameter        */

/******************************************************************************
    MS V1.X CMD CODE for SET_CMD TPC
******************************************************************************/
/*--    Flash CMD                   --*/
#define MS_CMD_BLOCK_READ       0xAA
#define MS_CMD_BLOCK_WRITE      0x55
#define MS_CMD_BLOCK_END        0x33
#define MS_CMD_BLOCK_ERASE      0x99
#define MS_CMD_FLASH_STOP       0xCC

/*--    Function CMD                --*/
#define MS_CMD_SLEEP            0x5A
#define MS_CMD_CLEAR_BUF        0xC3
#define MS_CMD_RESET            0x3C

/******************************************************************************
    MS PRO CMD CODE for SET_CMD TPC
******************************************************************************/
/*--    Memory Access CMD           --*/
#define MS2_CMD_READ_DATA       0x20
#define MS2_CMD_WRITE_DATA      0x21
#define MS2_CMD_READ_ATRB       0x24
#define MS2_CMD_STOP            0x25
#define MS2_CMD_ERASE           0x26
#define MS2_CMD_READ_2K_DATA    0x27
#define MS2_CMD_WRITE_2K_DATA   0x28

/*--    Function CMD                --*/
#define MS2_CMD_FORMAT          0x10
#define MS2_CMD_SLEEP           0x11
#define MS2_CMD_CHG_POWER_CLS   0x16

/******************************************************************************
    MS PRO SYSTEM INFORMATION OF BIT FIELD DEFINE
******************************************************************************/
/*--    Memory Stick Sub Class      --*/
#define MS2_SUBCLS_EXP_LOCKED   0x40    /* Export Locked Status bit         */
#define MS2_SUBCLS_STD_LOCKED   0x80    /* Standard Locked Status bit       */
#define MS2_SUBCLS_LOCKED_MASK  0xC0    /* Locked Status bit mask           */
#define MS2_SUBCLS_PW_CLS_MASK  0x18    /* Number of Power Classes bit mask */

/******************************************************************************
    Memory Stick Format
******************************************************************************/
#define MS_BLOCKS_IN_SEG                512
#define MS_DS_BLOCKS_IN_SEG             15 /* Disabled blocks in segment */
#define MS_MAX_SEG_NUM                  16
#define MS_SEG0_LADRS_NUM               494
#define MS_SEGN_LADRS_NUM               496

#endif  /*  __MS_DEFS_H */
