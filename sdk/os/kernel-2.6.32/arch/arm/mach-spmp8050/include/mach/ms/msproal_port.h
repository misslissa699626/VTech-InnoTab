/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_port.h
*
* DESCRIPTION   : MS Host Controller - Definition
=============================================================================*/
#ifndef     __MSPROAL_PORT_H
#define     __MSPROAL_PORT_H

#include <mach/ms/msproal_config.h>

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
    MSIF Port definition
******************************************************************************/
#if         (4 == MSPROAL_ALIGN_BYTES)
/* align 4 bytes boundary */
/*--    Command Register            --*/
#define MSIF_CMD_ADRS(adrs)         ((adrs) + 0x00)
/*--    Data Register               --*/
#define MSIF_DATA_ADRS(adrs)        ((adrs) + 0x04)
/*--    Status Register             --*/
#define MSIF_STTS_ADRS(adrs)        ((adrs) + 0x14)
/*--    System Register             --*/
#define MSIF_SYS_ADRS(adrs)         ((adrs) + 0x0C)
#else   /*  (4 == MSPROAL_ALIGN_BYTES)  */
/* align 2 bytes boundary */
/*--    Command Register            --*/
#define MSIF_CMD_ADRS(adrs)         ((adrs) + 0x00)
/*--    Data Register               --*/
#define MSIF_DATA_ADRS(adrs)        ((adrs) + 0x04)
/*--    Status Register             --*/
#define MSIF_STTS_ADRS(adrs)        ((adrs) + 0x08)
/*--    System Register             --*/
#define MSIF_SYS_ADRS(adrs)         ((adrs) + 0x0C)
#endif  /*  (4 == MSPROAL_ALIGN_BYTES)  */

/******************************************************************************
    HOST CONTROLER REGISTER OF BIT FIELD
******************************************************************************/
/*--    Command Register            --*/
#define MSIF_CMD_DSL                0x0400

/*--    Status Register             --*/
#define MSIF_STTS_DRQ               0x4000
#define MSIF_STTS_MSINT             0x2000
#define MSIF_STTS_RDY               0x1000
#define MSIF_STTS_CRC               0x0200
#define MSIF_STTS_TOE               0x0100
#define MSIF_STTS_EMP               0x0020
#define MSIF_STTS_FUL               0x0010
#define MSIF_STTS_CED               0x0008
#define MSIF_STTS_ERR               0x0004
#define MSIF_STTS_BRQ               0x0002
#define MSIF_STTS_CNK               0x0001

/*--    System Register             --*/
#define MSIF_SYS_RST                0x8000
#define MSIF_SYS_SRAC               0x4000
#define MSIF_SYS_INTEN              0x2000
#define MSIF_SYS_NOCRC              0x1000
#define MSIF_SYS_INTCLR             0x0800
#define MSIF_SYS_MSIEN              0x0400
#define MSIF_SYS_FCLR               0x0200
#define MSIF_SYS_FDIR               0x0100
#define MSIF_SYS_DAM                0x0080
#define MSIF_SYS_DRM                0x0040
#define MSIF_SYS_DRQSL              0x0020
#define MSIF_SYS_REI                0x0010
#define MSIF_SYS_REO                0x0008
#define MSIF_SYS_BSY0               0x0000      /* BSYCNT=0 */
#define MSIF_SYS_BSY1               0x0001      /* BSYCNT=1 */
#define MSIF_SYS_BSY2               0x0002      /* BSYCNT=2 */
#define MSIF_SYS_BSY3               0x0003      /* BSYCNT=3 */
#define MSIF_SYS_BSY4               0x0004      /* BSYCNT=4 */
#define MSIF_SYS_BSY5               0x0005      /* BSYCNT=5 */
#define MSIF_SYS_BSY6               0x0006      /* BSYCNT=6 */
#define MSIF_SYS_BSY7               0x0007      /* BSYCNT=7 */

#define MSIF_SYS_BSYCNT_MASK        0xFFF8
#define MSIF_SYS_BSYCNT_USUAL       MSIF_SYS_BSY5

/******************************************************************************
    Model size & Align byte
******************************************************************************/
#if         (4 == MSPROAL_ALIGN_BYTES)
/* align 4 bytes boundary */
#define MSIF_MODEL_SIZE             unsigned long
#define MSIF_ALIGN_BYTE             4
#else   /*  (4 == MSPROAL_ALIGN_BYTES)  */
/* align 2 bytes boundary */
#define MSIF_MODEL_SIZE             unsigned short
#define MSIF_ALIGN_BYTE             2
#endif  /*  (4 == MSPROAL_ALIGN_BYTES)  */
#else   /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */
/******************************************************************************
    MSIF Port definition
******************************************************************************/
/*--    Command Register            --*/
#define MSIF_CMD_ADRS(adrs)         ((adrs) + 0x00)
/*--    Data Register               --*/
#define MSIF_DATA_ADRS(adrs)        ((adrs) + 0x04)
/*--    Status Register             --*/
#define MSIF_STTS_ADRS(adrs)        ((adrs) + 0x08)
/*--    System Register             --*/
#define MSIF_SYS_ADRS(adrs)         ((adrs) + 0x0C)
/*--    User Custom Register        --*/
#define MSIF_USR_CSTM_ADRS(adrs)    ((adrs) + 0x10)
/*--    FIFO Control Register       --*/
#define MSIF_FIFO_CTRL_ADRS(adrs)   ((adrs) + 0x14)
/*--    DMA Request Control Register    --*/
#define MSIF_DMA_CTRL_ADRS(adrs)    ((adrs) + 0x1C)

/******************************************************************************
    HOST CONTROLER REGISTER OF BIT FIELD
******************************************************************************/
/*--    Command Register            --*/
#define MSIF_CMD_DSL                0x0800

/*--    Status Register             --*/
#define MSIF_STTS_DRQ               0x4000
#define MSIF_STTS_MSINT             0x2000
#define MSIF_STTS_RDY               0x1000
#define MSIF_STTS_NO8P              0x0400
#define MSIF_STTS_CRC               0x0200
#define MSIF_STTS_TOE               0x0100
#define MSIF_STTS_CED               0x0080
#define MSIF_STTS_ERR               0x0040
#define MSIF_STTS_BRQ               0x0020
#define MSIF_STTS_CNK               0x0001

/*--    System Register             --*/
#define MSIF_SYS_RST                0x8000
#define MSIF_SYS_INTEN              0x4000
#define MSIF_SYS_MSIEN              0x2000
#define MSIF_SYS_DRQSL              0x1000
#define MSIF_SYS_INTCLR             0x0800
#define MSIF_SYS_SRAC               0x0080
#define MSIF_SYS_EIGHT              0x0040
#define MSIF_SYS_MSPIO1             0x0020
#define MSIF_SYS_MSPIO0             0x0010
#define MSIF_SYS_NOCRC              0x0008
#define MSIF_SYS_BSY0               0x0000      /* BSYCNT=0 */
#define MSIF_SYS_BSY1               0x0001      /* BSYCNT=1 */
#define MSIF_SYS_BSY2               0x0002      /* BSYCNT=2 */
#define MSIF_SYS_BSY3               0x0003      /* BSYCNT=3 */
#define MSIF_SYS_BSY4               0x0004      /* BSYCNT=4 */
#define MSIF_SYS_BSY5               0x0005      /* BSYCNT=5 */
#define MSIF_SYS_BSY6               0x0006      /* BSYCNT=6 */
#define MSIF_SYS_BSY7               0x0007      /* BSYCNT=7 */
#define MSIF_SYS_BSYCNT_MASK        0xFFF8
#define MSIF_SYS_BSYCNT_USUAL       MSIF_SYS_BSY5

/*--    User Custom Register        --*/
#define MSIF_USR_CSTM_MSPIO_MASK    0xFF00
#define MSIF_USR_CSTM_EMP           0x0020
#define MSIF_USR_CSTM_FUL           0x0010
#define MSIF_USR_CSTM_NDLT          0x0001

/*--    FIFO Control Register       --*/
#define MSIF_FIFO_CTRL_AFCLR        0x0400
#define MSIF_FIFO_CTRL_FCLR         0x0200
#define MSIF_FIFO_CTRL_FDIR         0x0100
#define MSIF_FIFO_CTRL_DRSZ_4       0x0000
#define MSIF_FIFO_CTRL_DRSZ_8       0x0001
#define MSIF_FIFO_CTRL_DRSZ_16      0x0002
#define MSIF_FIFO_CTRL_DRSZ_MASK    0x0003

/*--    DMA Control Register        --*/
#define MSIF_DMA_CTRL_DMAON_0       0x0001  /* READ_MG_STTS_REG             */
#define MSIF_DMA_CTRL_DMAON_1       0x0002
#define MSIF_DMA_CTRL_DMAON_2       0x0004  /* READ_PAGE_DATA/READ_LONG_DATA*/
#define MSIF_DMA_CTRL_DMAON_3       0x0008  /* READ_MGD_REG/READ_SHORT_DATA */
#define MSIF_DMA_CTRL_DMAON_4       0x0010  /* READ_REG                     */
#define MSIF_DMA_CTRL_DMAON_5       0x0020  /* READ_QUAD_LONG_DATA          */
#define MSIF_DMA_CTRL_DMAON_6       0x0040
#define MSIF_DMA_CTRL_DMAON_7       0x0080  /* GET_INT                      */
#define MSIF_DMA_CTRL_DMAON_8       0x0100  /* SET_RW_REG_ADRS              */
#define MSIF_DMA_CTRL_DMAON_9       0x0200  /* EX_SET_CMD                   */
#define MSIF_DMA_CTRL_DMAON_10      0x0400  /* WRITE_QUAD_LONG_DATA         */
#define MSIF_DMA_CTRL_DMAON_11      0x0800  /* WRITE_REG                    */
#define MSIF_DMA_CTRL_DMAON_12      0x1000  /* WRITE_MGD_REG/WRITE_SHORT_DATA*/
#define MSIF_DMA_CTRL_DMAON_13      0x2000  /* WRITE_PAGE_DATA/             */
                                            /* WRITE_LONG_DATA              */
#define MSIF_DMA_CTRL_DMAON_14      0x4000  /* SET_CMD                      */
#define MSIF_DMA_CTRL_DMAON_15      0x8000
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#endif  /*  __MSPROAL_PORT_H    */
