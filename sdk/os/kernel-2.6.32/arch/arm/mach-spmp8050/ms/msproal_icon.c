/*=============================================================================
* Copyright 2003-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_icon.c
*
* DESCRIPTION   : Memory Stick common API
*
* FUNCTION LIST
*                   msproal_icon_end_dma
*                   msproal_icon_exec_mc
*                   msproal_icon_load_mc
*                   msproal_icon_reset_icon
*                   msproal_icon_start_dma
*                   msproal_icon_wait_int
*                   msproal_icon_write_pout_reg
=============================================================================*/
#include <mach/ms/msproal_common.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_icon_end_dma
*   DESCRIPTION : End DMA.
*------------------------------------------------------------------------------
*   void msproal_icon_end_dma(MSIFHNDL *msifhndl)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
void msproal_icon_end_dma(MSIFHNDL *msifhndl)
{
    msproal_user_write_mem32(ICON_DMA0_CNF_REG(msifhndl->BaseAdrs), 0);
    msproal_user_write_mem32(ICON_DMA1_CNF_REG(msifhndl->BaseAdrs), 0);

    return;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_icon_exec_mc
*   DESCRIPTION : Execute I-CON microcode
*------------------------------------------------------------------------------
*   SINT msproal_icon_exec_mc(MSIFHNDL *msifhndl, SINT pc)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Meida error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_MC_FLASH_READ_ERR   : Flash read error
*       MSPROAL_MC_FLASH_WRITE_ERR  : Flash write error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pc          : The starting address of microcode to be executed
******************************************************************************/
SINT msproal_icon_exec_mc(MSIFHNDL *msifhndl, SINT pc)
{
    ULONG   ptn;
    SINT    result;
    UWORD   ictrlreg;

    /* Set PC */
    msproal_user_write_mem16(ICON_PC_REG(msifhndl->BaseAdrs), (UWORD)pc);

    msifhndl->IntState = MSPROAL_CLEAR_INTSTATE;

    ptn = MSPROAL_FLG_EXTRACT;
    result = msproal_user_check_flg(ptn);
    if(MSPROAL_OK == result) {
        msproal_user_clear_flg(~ptn);

        return MSPROAL_EXTRACT_ERR;
    }

    msproal_user_clear_flg(~(MSPROAL_FLG_ICON | MSPROAL_FLG_DMA));

    /* Set START bit */
    msproal_user_read_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), &ictrlreg);
    ictrlreg |= ICON_CTRL_STRT;
    msproal_user_write_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), ictrlreg);

    return msproal_icon_wait_int(   msifhndl,
                                    MSPROAL_TIMEOUT_ICON,
                                    MSPROAL_WICON);
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         ((2 == MSPROAL_SUPPORT_IP) || (5 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_icon_load_mc
*   DESCRIPTION : Load I-CON microcode to instruction queue
*------------------------------------------------------------------------------
*   SINT msproal_icon_load_mc(MSIFHNDL *msifhndl, SINT pc, SINT size,
*           UWORD *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Abnormal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pc          : PC number microcode begun to be written in
*       size        : Write data size
*       data        : Address to the microcode to be written
******************************************************************************/
SINT msproal_icon_load_mc(MSIFHNDL *msifhndl, SINT pc, SINT size, UWORD *data)
{
    if((1 > size) || (0 > pc)) {
        return MSPROAL_PARAM_ERR;
    }

    if(256 < (pc + size)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Set PC */
    msproal_user_write_mem16(ICON_PC_REG(msifhndl->BaseAdrs), (UWORD)pc);

    /* Write microcode */
    while(size --) {
        msproal_user_write_mem16(ICON_INST_QUEUE(msifhndl->BaseAdrs), *data);
        data ++;
    }

    return MSPROAL_OK;
}
#endif  /*  ((2 == MSPROAL_SUPPORT_IP) || (5 == MSPROAL_SUPPORT_IP))    */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_icon_load_mc
*   DESCRIPTION : Load I-CON microcode to instruction queue
*------------------------------------------------------------------------------
*   SINT msproal_icon_load_mc(MSIFHNDL *msifhndl, SINT pc, SINT size,
*           UWORD *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Abnormal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pc          : PC number microcode begun to be written in
*       size        : Write data size
*       data        : Address to the microcode to be written
******************************************************************************/
SINT msproal_icon_load_mc(MSIFHNDL *msifhndl, SINT pc, SINT size, UWORD *data)
{
    if((1 > size) || (0 > pc)) {
        return MSPROAL_PARAM_ERR;
    }

    if(128 < (pc + size)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Set PC */
    msproal_user_write_mem16(ICON_PC_REG(msifhndl->BaseAdrs), (UWORD)pc);

    /* Write microcode */
    while(size --) {
        msproal_user_write_mem16(ICON_INST_QUEUE(msifhndl->BaseAdrs), *data);
        data ++;
    }

    return MSPROAL_OK;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (2 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_icon_reset_icon
*   DESCRIPTION : Reset I-CON
*------------------------------------------------------------------------------
*   SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
{
    SINT    result;
    UWORD   isysreg;

    /* Set SRST to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_SRST;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    msproal_user_start_timer(1);

    /* Wait until SRST becomes 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    while(isysreg & ICON_SYS_SRST) {
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    }

    msproal_user_end_timer();

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= (ICON_SYS_INTE
            | ICON_SYS_DMACH
            | ICON_SYS_DMAE0
            | ICON_SYS_DMAE1
            | ((MSPROAL_DMA_SLICE_SIZE - 1) << 4));
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    return MSPROAL_OK;
}
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_icon_reset_icon
*   DESCRIPTION : Reset I-CON
*------------------------------------------------------------------------------
*   SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
{
    SINT    result;
    UWORD   isysreg;

    /* Set SRST to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_SRST;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    msproal_user_start_timer(1);

    /* Wait until SRST becomes 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    while(isysreg & ICON_SYS_SRST) {
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    }

    msproal_user_end_timer();

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= (ICON_SYS_INTE
            | ICON_SYS_DMAE0
            | ICON_SYS_DMAE1
            | ((MSPROAL_DMA_SLICE_SIZE - 1) << 4));
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
#if         (2 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_icon_reset_icon
*   DESCRIPTION : Reset I-CON
*------------------------------------------------------------------------------
*   SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
{
    SINT    result;
    UWORD   isysreg;

    /* Set SRST to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_SRST;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    msproal_user_start_timer(1);

    /* Wait until SRST becomes 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    while(isysreg & ICON_SYS_SRST) {
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    }

    msproal_user_end_timer();

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= (ICON_SYS_INTE
            | ICON_SYS_DMACH
            | ICON_SYS_DMAE0
            | ICON_SYS_DMAE1
            | ((MSPROAL_DMA_SLICE_SIZE - 1) << 6));
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    return MSPROAL_OK;
}
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_icon_reset_icon
*   DESCRIPTION : Reset I-CON
*------------------------------------------------------------------------------
*   SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
{
    SINT    result;
    UWORD   isysreg;

    /* Set SRST to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_SRST;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    msproal_user_start_timer(1);

    /* Wait until SRST becomes 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    while(isysreg & ICON_SYS_SRST) {
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    }

    msproal_user_end_timer();

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= (ICON_SYS_INTE
            | ICON_SYS_DMAE0
            | ICON_SYS_DMAE1
            | ((MSPROAL_DMA_SLICE_SIZE - 1) << 6));
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_icon_reset_icon
*   DESCRIPTION : Reset I-CON
*------------------------------------------------------------------------------
*   SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_icon_reset_icon(MSIFHNDL *msifhndl)
{
    ULONG   iporeg;
    SINT    result;
    UWORD   isysreg;

    msproal_user_read_mem32(ICON_PO_REG(msifhndl->BaseAdrs), &iporeg);

    /* Set SRST to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_SRST;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    msproal_user_start_timer(1);

    /* Wait until SRST becomes 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    while(isysreg & ICON_SYS_SRST) {
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    }

    msproal_user_end_timer();

    msproal_user_write_mem32(ICON_PO_REG(msifhndl->BaseAdrs), iporeg);

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_INTE;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_icon_start_dma
*   DESCRIPTION : Start DMA.
*------------------------------------------------------------------------------
*   SINT msproal_icon_start_dma(MSIFHNDL *msifhndl, ULONG *lltbl, SINT select)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lltbl       : Address to link list table
*       select      : DMA target select
*                   (MSPROAL_SELECT_GDFIFO/MSPROAL_SELECT_PBUFF)
******************************************************************************/
SINT msproal_icon_start_dma(MSIFHNDL *msifhndl, ULONG *lltbl, SINT select)
{
    if(MSPROAL_SELECT_GDFIFO == select) {
        msproal_user_write_mem32(   ICON_DMA0_TADR_REG(msifhndl->BaseAdrs),
                                    lltbl[0]);
        msproal_user_write_mem32(   ICON_DMA0_LADR_REG(msifhndl->BaseAdrs),
                                    lltbl[1]);
        msproal_user_write_mem32(   ICON_DMA0_CNF_REG(msifhndl->BaseAdrs),
                                    lltbl[2]);
    } else if(MSPROAL_SELECT_PBUFF == select) {
        msproal_user_write_mem32(   ICON_DMA1_TADR_REG(msifhndl->BaseAdrs),
                                    lltbl[0]);
        msproal_user_write_mem32(   ICON_DMA1_LADR_REG(msifhndl->BaseAdrs),
                                    lltbl[1]);
        msproal_user_write_mem32(   ICON_DMA1_CNF_REG(msifhndl->BaseAdrs),
                                    lltbl[2]);
    } else {
        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         ((2 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_icon_wait_int
*   DESCRIPTION : Wait interrupt and judge the state after microcode stopped
*------------------------------------------------------------------------------
*   SINT msproal_icon_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Meida error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_MC_FLASH_READ_ERR   : Flash read error
*       MSPROAL_MC_FLASH_WRITE_ERR  : Flash write error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       time        : Time until Timeout(ms)
*       mode        : Wait Mode(MSPROAL_WICON/MSPROAL_WDMA)
******************************************************************************/
SINT msproal_icon_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
{
    ULONG   ptn, intstat;
    SINT    result;
    UWORD   iflgreg, hsttsreg;

    switch(mode) {
    case MSPROAL_WICON:
        ptn = MSPROAL_FLG_ICON | MSPROAL_FLG_EXTRACT;
        result = msproal_user_wait_flg(ptn, time);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        intstat = msifhndl->IntState;

        msproal_user_clear_flg(~ptn);

        if(intstat & MSPROAL_FLG_EXTRACT) {
            if(!(intstat & MSPROAL_FLG_ICON)) {
                result = msproal_user_wait_flg(MSPROAL_FLG_ICON, time);
                if(MSPROAL_OK != result) {
                    return MSPROAL_SYSTEM_ERR;
                }

                msproal_user_clear_flg(~MSPROAL_FLG_ICON);
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Get Flag Register */
        msproal_user_read_mem16(ICON_FLG_REG(msifhndl->BaseAdrs), &iflgreg);
        if(iflgreg & ICON_FLAG_HLTF) {
            result = -(iflgreg & ICON_FLAG_EXTS_MASK);
        } else if(iflgreg & ICON_FLAG_ITOF) {
            /* Get Status Register */
            msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                    &hsttsreg);
            if(hsttsreg & MSIF_STTS_RDY) {
                result = MSPROAL_ACCESS_ERR;
            } else {
                result = MSPROAL_SYSTEM_ERR;
            }
        } else if(iflgreg & ICON_FLAG_FLG) {
            result = MSPROAL_ACCESS_ERR;
        } else {
            result = MSPROAL_SYSTEM_ERR;
        }
        break;
    case MSPROAL_WDMA:
        ptn = MSPROAL_FLG_DMA;
        result = msproal_user_wait_flg(ptn, time);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_clear_flg(~ptn);
        break;
    default :
        result = MSPROAL_PARAM_ERR;
        break;
    }

    return result;
}
#endif  /*  ((2 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_icon_wait_int
*   DESCRIPTION : Wait interrupt and judge the state after microcode stopped
*------------------------------------------------------------------------------
*   SINT msproal_icon_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Meida error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_MC_FLASH_READ_ERR   : Flash read error
*       MSPROAL_MC_FLASH_WRITE_ERR  : Flash write error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       time        : Time until Timeout(ms)
*       mode        : Wait Mode(MSPROAL_WICON)
******************************************************************************/
SINT msproal_icon_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
{
    ULONG   ptn, intstat;
    SINT    result;
    UWORD   ipc, ir3, iflgreg, hsttsreg;

    switch(mode) {
    case MSPROAL_WICON:
        ptn = MSPROAL_FLG_ICON | MSPROAL_FLG_EXTRACT;
        result = msproal_user_wait_flg(ptn, time);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        intstat = msifhndl->IntState;

        msproal_user_clear_flg(~ptn);

        if(intstat & MSPROAL_FLG_EXTRACT) {
            if(!(intstat & MSPROAL_FLG_ICON)) {
                result = msproal_user_wait_flg(MSPROAL_FLG_ICON, time);
                if(MSPROAL_OK != result) {
                    return MSPROAL_SYSTEM_ERR;
                }

                msproal_user_clear_flg(~MSPROAL_FLG_ICON);
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Get Flag Register */
        msproal_user_read_mem16(ICON_FLG_REG(msifhndl->BaseAdrs), &iflgreg);
        if(iflgreg & ICON_FLAG_HLTF) {
            result = -(iflgreg & ICON_FLAG_EXTS_MASK);
        } else if(iflgreg & ICON_FLAG_ITOF) {
            /* Get PC Register */
            msproal_user_read_mem16(ICON_PC_REG(msifhndl->BaseAdrs), &ipc);
            if(2 == (ipc & ICON_PC_PRGC_MASK)) {
                /* Get General Register3 */
                msproal_user_read_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs),
                                        &ir3);
                result = -ir3;
            } else {
                /* Get Status Register */
                msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                        &hsttsreg);
                if(hsttsreg & MSIF_STTS_RDY) {
                    result = MSPROAL_ACCESS_ERR;
                } else {
                    result = MSPROAL_SYSTEM_ERR;
                }
            }
        } else if(iflgreg & ICON_FLAG_FLG) {
            result = MSPROAL_ACCESS_ERR;
        } else {
            result = MSPROAL_SYSTEM_ERR;
        }
        break;
    default :
        result = MSPROAL_PARAM_ERR;
        break;
    }

    return result;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_icon_write_pout_reg
*   DESCRIPTION : Host Controller is reset.
*------------------------------------------------------------------------------
*   SINT msproal_icon_write_pout_reg(MSIFHNDL *msifhndl, ULONG data,
            ULONG mask)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl        : Address to initialized MSIFHNDL type
*       data            : Data to write
*       mask            : Mask to set value
******************************************************************************/
SINT msproal_icon_write_pout_reg(MSIFHNDL *msifhndl, ULONG data, ULONG mask)
{
    ULONG   iporeg;

    msproal_user_read_mem32(ICON_PO_REG(msifhndl->BaseAdrs), &iporeg);

    iporeg = (ULONG)((iporeg & mask) | (data & ~mask));

    msproal_user_write_mem32(ICON_PO_REG(msifhndl->BaseAdrs), iporeg);

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */
