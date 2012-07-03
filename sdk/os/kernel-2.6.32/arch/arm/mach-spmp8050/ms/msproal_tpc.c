/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_tpc.c
*
* DESCRIPTION   : TPC API
*
* FUNCTION LIST
*                   msproal_tpc_change_ifmode
*                   msproal_tpc_ex_set_cmd
*                   msproal_tpc_get_int
*                   msproal_tpc_read_fifo
*                   msproal_tpc_read_mg_stts_reg
*                   msproal_tpc_read_mgd_reg
*                   msproal_tpc_read_nfifo
*                   msproal_tpc_read_page
*                   msproal_tpc_read_quad_long_data
*                   msproal_tpc_read_reg
*                   msproal_tpc_read_short_data
*                   msproal_tpc_reset_host
*                   msproal_tpc_set_bsycnt
*                   msproal_tpc_set_cmd
*                   msproal_tpc_set_rw_reg_adrs
*                   msproal_tpc_set_tpc
*                   msproal_tpc_wait_int
*                   msproal_tpc_write_fifo
*                   msproal_tpc_write_mgd_reg
*                   msproal_tpc_write_nfifo
*                   msproal_tpc_write_page
*                   msproal_tpc_write_quad_long_data
*                   msproal_tpc_write_reg
*                   msproal_tpc_write_short_data
=============================================================================*/
#include <mach/ms/msproal_common.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_config.h>
#include <mach/hal/hal_ms.h>
#include <mach/regs-ms.h>
#include <linux/coda.h>
#include <mach/gp_apbdma0.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>


#if         ((1 == MSPROAL_SUPPORT_IP) || (2 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_change_ifmode
*   DESCRIPTION : Change the interface mode of host controller side.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARALLEL_MODE/MSPROAL_PRO_4PARALLEL_MODE/
*                   MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_tpc_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
{
    UWORD   hsysreg;

    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs),
                            &hsysreg);
    if(MSPROAL_SERIAL_MODE == mode) {
        hsysreg |= (MSIF_SYS_SRAC | MSIF_SYS_MSPIO1);
        hsysreg &= ~MSIF_SYS_EIGHT;
    } else if(MSPROAL_8PARALLEL_MODE == mode) {
        hsysreg |= MSIF_SYS_EIGHT;
        hsysreg &= ~(MSIF_SYS_SRAC | MSIF_SYS_MSPIO1);
    } else {
        hsysreg &= ~(MSIF_SYS_SRAC | MSIF_SYS_MSPIO1 | MSIF_SYS_EIGHT);
    }
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

    msproal_user_change_clock(mode);

    return MSPROAL_OK;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (2 == MSPROAL_SUPPORT_IP))    */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_change_ifmode
*   DESCRIPTION : Change the interface mode of host controller side.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARALLEL_MODE/MSPROAL_PRO_4PARALLEL_MODE)
******************************************************************************/
SINT msproal_tpc_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
{
    gpHalMsChangeIfMode(mode);
    
    msproal_user_change_clock(mode);

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_tpc_change_ifmode
*   DESCRIPTION : Change the interface mode of host controller side.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_V1_4PARALLEL_MODE/MSPROAL_PRO_4PARALLEL_MODE/
*                   MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_tpc_change_ifmode(MSIFHNDL *msifhndl, SINT mode)
{
    UWORD   hsysreg;

    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs),
                            &hsysreg);
    if(MSPROAL_SERIAL_MODE == mode) {
        hsysreg |= MSIF_SYS_SRAC;
        hsysreg &= ~MSIF_SYS_EIGHT;
    } else if(MSPROAL_8PARALLEL_MODE == mode) {
        hsysreg |= MSIF_SYS_EIGHT;
        hsysreg &= ~MSIF_SYS_SRAC;
    } else {
        hsysreg &= ~(MSIF_SYS_SRAC | MSIF_SYS_EIGHT);
    }
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

    msproal_user_change_clock(mode);

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

/******************************************************************************
*   FUNCTION    : msproal_tpc_ex_set_cmd
*   DESCRIPTION : Execute the EX_SET_CMD command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_ex_set_cmd(MSIFHNDL *msifhndl, SINT cmd, ULONG adrs,
*           UWORD count)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       cmd         : Command
*       adrs        : Data Address Register
*       count       : Data Count Register
******************************************************************************/
SINT msproal_tpc_ex_set_cmd(MSIFHNDL *msifhndl, SINT cmd, ULONG adrs,
        UWORD count)
{
    SINT    result, retry;
    UINT    status;
    
    retry = MSPROAL_RETRY_COUNT;
    do {
        gpHalMsSendCmd((RUNCMD|TPC_EX_SET_CMD) | ((cmd<<8)&0xFF00) | ((count<<16)&0xFFFF0000), adrs, 0);
        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_EX_SET_CMD);
        
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        
        /* End Timer */
        msproal_user_end_timer();
        status = gpHalMsGetStatus();
       
        if (status & TIMEOUT) {
            continue;
        }
        if (status & CRCERR) {
            return MSPROAL_SYSTEM_ERR;
        }
      	
        return MSPROAL_OK;
        
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

/******************************************************************************
*   FUNCTION    : msproal_tpc_get_int
*   DESCRIPTION : Execute the GET_INT command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_get_int(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : INT Register
******************************************************************************/
SINT msproal_tpc_get_int(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;
    UINT    status;

    retry = MSPROAL_RETRY_COUNT;
    do {
        
    	gpHalMsSendCmd((RUNCMD|TPC_GET_INT), 0, 1);
    	
        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_GET_INT);
        
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        
        /* End Timer */
        msproal_user_end_timer();
        
        status = gpHalMsGetStatus();
        if (status & TIMEOUT || status & CRCERR) {
            continue;
        }
        
        *data = (UBYTE) gpHalMsReadReg();
        
        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

SINT msproal_tpc_get_int1(MSIFHNDL *msifhndl, UBYTE *data)
{

    SINT    result = MSPROAL_OK;
    UBYTE       datareg[4];
    /* Setting READ_ADRS and READ_SIZE */
    result = msproal_tpc_set_rw_reg_adrs(   msifhndl,
                                            0x01,
                                            4,
                                            MS_DEFAULT_WRITE_ADRS,
                                            MS_DEFAULT_WRITE_SIZE);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_tpc_read_reg(msifhndl, 4, datareg);

    if(MSPROAL_OK != result) {
        return result;
    }
    *data =  datareg[0] ;
    
    return result;

}

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_fifo
*   DESCRIPTION : Read data from the Data register and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_fifo(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_HOST_TOE_ERR        : Host TOE(Time Out Error) error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area(8 bytes) where read data is stored
******************************************************************************/
SINT msproal_tpc_read_fifo(MSIFHNDL *msifhndl, UBYTE *data)
{
    ULONG   ptn;
    SINT    result;
    UWORD   hsttsreg;

    ptn = MSIF_STTS_TOE | MSPROAL_FLG_EXTRACT;

    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    /* Waiting for interrupt by DRQ */
    while(!(hsttsreg & MSIF_STTS_DRQ)) {
        /* Did interrupt occur by TOE */
        result = msproal_user_check_flg(ptn);
        if(MSPROAL_OK == result) {
            msproal_user_clear_flg(~(ptn | MSIF_STTS_RDY));

            if(!(msifhndl->IntState & MSPROAL_FLG_EXTRACT)) {
                return MSPROAL_HOST_TOE_ERR;
            }

            result = msproal_tpc_reset_host(msifhndl);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Did interrupt occur by timer */
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    }

    msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (ULONG *)(data));
    msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (ULONG *)(data + sizeof(ULONG)));

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_fifo
*   DESCRIPTION : Read data from the Data register and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_fifo(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_HOST_TOE_ERR        : Host TOE(Time Out Error) error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area(8 bytes) where read data is stored
******************************************************************************/
SINT msproal_tpc_read_fifo(MSIFHNDL *msifhndl, UBYTE *data)
{
    ULONG   ptn;
    SINT    result;
    UWORD   hsttsreg;

    ptn = MSIF_STTS_TOE | MSPROAL_FLG_EXTRACT;

    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    /* Waiting for interrupt by DRQ */
    while(!(hsttsreg & MSIF_STTS_DRQ)) {
        /* Did interrupt occur by TOE */
        result = msproal_user_check_flg(ptn);
        if(MSPROAL_OK == result) {
            msproal_user_clear_flg(~(ptn | MSIF_STTS_RDY));

            if(!(msifhndl->IntState & MSPROAL_FLG_EXTRACT)) {
                return MSPROAL_HOST_TOE_ERR;
            }

            result = msproal_tpc_reset_host(msifhndl);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Did interrupt occur by timer */
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    }

#if         (4 == MSPROAL_ALIGN_BYTES)
    msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 0));
    msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 1));
#else   /*  (2 == MSPROAL_ALIGN_BYTES)  */
    msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 0));
    msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 1));
    msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 2));
    msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 3));
#endif  /*  (4 == MSPROAL_ALIGN_BYTES)  */

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_mg_stts_reg
*   DESCRIPTION : Read data from the MG STTS Register of Memory Stick
*               and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_mg_stts_reg(MSIFHNDL *msifhndl, UBYTE *status)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       status      : Address to area(2 bytes) where read Status is stored
******************************************************************************/
SINT msproal_tpc_read_mg_stts_reg(MSIFHNDL *msifhndl, UBYTE *status)
{
    SINT    result, retry;

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* Issuing of READ_MG_STTS_REG TPC */
        result = msproal_tpc_set_tpc(msifhndl, TPC_READ_MG_STTS_REG, 2);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_READ_MG_STTS_REG);

        /* Read data from MG STTS Register */
        result = msproal_tpc_read_nfifo(msifhndl, 2, status);

        /* End Timer */
        msproal_user_end_timer();

        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        /* Wating for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_RDY,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_CRC_ERR != result) {
                return result;
            }

            continue;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_mgd_reg
*   DESCRIPTION : Read data from the MGD READ Register of Memory Stick
*               and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_mgd_reg(MSIFHNDL *msifhndl, UBYTE *data0,
*           UBYTE *data1, UBYTE *data2)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data0       : Address to area(8 bytes) where read Data(00h-07h) is
*                   stored
*       data1       : Address to area(8 bytes) where read Data(08h-0fh) is
*                   stored
*       data2       : Address to area(8 bytes) where read Data(10h-17h) is
*                   stored
******************************************************************************/
SINT msproal_tpc_read_mgd_reg(MSIFHNDL *msifhndl, UBYTE *data0, UBYTE *data1,
        UBYTE *data2)
{
    SINT    result, retry;
    UBYTE   *mgd_reg[3], **pend, **pread;

    retry       = MSPROAL_RETRY_COUNT;
    mgd_reg[0]  = data0;
    mgd_reg[1]  = data1;
    mgd_reg[2]  = data2;
    pend        = mgd_reg + 3;
    do {
        /* Issuing of READ_MGD_REG TPC */
        result = msproal_tpc_set_tpc(msifhndl, TPC_READ_MGD_REG, 24);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_READ_MGD_REG);

        /* Read data from MGD Register */
        pread = mgd_reg;
        while(pend > pread) {
            result = msproal_tpc_read_fifo(msifhndl, *pread);
            if(MSPROAL_OK != result) {
                break;
            }

            pread ++;
        }

        /* End Timer */
        msproal_user_end_timer();

        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_RDY,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_CRC_ERR != result) {
                return result;
            }

            continue;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_nfifo
*   DESCRIPTION : Read size bytes of data from the Data register
*               and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_HOST_TOE_ERR        : Host TOE(Time Out Error) error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : The numbers of bytes to read
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_tpc_read_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    ULONG   ptn;
    ULONG   work[2];
    SINT    result, cnt, rem;
    UWORD   hsttsreg;
    UBYTE   *pend, *pdata, *pwork;

    ptn = MSIF_STTS_TOE | MSPROAL_FLG_EXTRACT;

    pdata   = data;
    pend    = data + (size & ~0x7);
    while(pdata < pend) {
        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                &hsttsreg);
        while(!(hsttsreg & MSIF_STTS_DRQ)) {
            result = msproal_user_check_flg(ptn);
            if(MSPROAL_OK == result) {
                msproal_user_clear_flg(~(ptn | MSIF_STTS_RDY));

                if(!(msifhndl->IntState & MSPROAL_FLG_EXTRACT)) {
                    return MSPROAL_HOST_TOE_ERR;
                }

                result = msproal_tpc_reset_host(msifhndl);
                if(MSPROAL_OK != result) {
                    return MSPROAL_SYSTEM_ERR;
                }

                return MSPROAL_EXTRACT_ERR;
            }

            /* Did interrupt occur by timer */
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                    &hsttsreg);
        }


        msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                (ULONG *)(pdata));
        msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                (ULONG *)(pdata + sizeof(ULONG)));
        pdata += 8;
    }

    rem = size & 0x7;
    if(rem) {
        pwork = (UBYTE *)work;
        result = msproal_tpc_read_fifo(msifhndl, pwork);
        if(MSPROAL_OK != result) {
            return result;
        }

        for(cnt = 0; cnt < rem; cnt ++) {
            pdata[cnt] = pwork[cnt];
        }
    }

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_nfifo
*   DESCRIPTION : Read size bytes of data from the Data register
*               and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_HOST_TOE_ERR        : Host TOE(Time Out Error) error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : The numbers of bytes to read
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_tpc_read_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    ULONG   ptn;
    ULONG   work[2];
    SINT    result, cnt, rem;
    UWORD   hsttsreg;
    UBYTE   *pend, *pdata, *pwork;

    ptn = MSIF_STTS_TOE | MSPROAL_FLG_EXTRACT;

    pdata   = data;
    pend    = data + (size & ~0x7);
    while(pdata < pend) {
        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                &hsttsreg);
        while(!(hsttsreg & MSIF_STTS_DRQ)) {
            result = msproal_user_check_flg(ptn);
            if(MSPROAL_OK == result) {
                msproal_user_clear_flg(~(ptn | MSIF_STTS_RDY));

                if(!(msifhndl->IntState & MSPROAL_FLG_EXTRACT)) {
                    return MSPROAL_HOST_TOE_ERR;
                }

                result = msproal_tpc_reset_host(msifhndl);
                if(MSPROAL_OK != result) {
                    return MSPROAL_SYSTEM_ERR;
                }

                return MSPROAL_EXTRACT_ERR;
            }

            /* Did interrupt occur by timer */
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                    &hsttsreg);
        }

#if         (4 == MSPROAL_ALIGN_BYTES)
        msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 0));
        msproal_user_read_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 1));
#else   /*  (2 == MSPROAL_ALIGN_BYTES)  */
        msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 0));
        msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 1));
        msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 2));
        msproal_user_read_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            (MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 3));
#endif  /*  (4 == MSPROAL_ALIGN_BYTES)  */
        pdata += 8;
    }

    rem = size & 0x7;
    if(rem) {
        pwork = (UBYTE *)work;
        result = msproal_tpc_read_fifo(msifhndl, pwork);
        if(MSPROAL_OK != result) {
            return result;
        }

        for(cnt = 0; cnt < rem; cnt ++) {
            pdata[cnt] = pwork[cnt];
        }
    }

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         !(((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))  \
            && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_page
*   DESCRIPTION : Read one page(512 bytes) of data from the Memory Stick and
*               stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_page(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where read data is stored
******************************************************************************/
/* TPC READ_LONG_DATA */
SINT msproal_tpc_read_page(MSIFHNDL *msifhndl, UBYTE *data)
{  
    SINT    result, retry;
    UINT    status;
    UINT  	*ptr;
    UINT  	i;
    gpApbdma0Param_t	dma_param;
    
    retry = MSPROAL_RETRY_COUNT;
    do {
    	if (msifhndl->XfrMode == MS_CPU_MODE) {
    		gpHalMsSetFifoLevel(TX_TRI_L1,RX_TRI_L8);
    		ptr = (UINT *) data;
    		gpHalMsSendCmd(RUNCMD|TPC_READ_PAGE_DATA,0,0);
    		for (i=0;i<16;i++) {	
    			status = 0;
    			msproal_user_start_timer(MSPROAL_TIMEOUT_READ_PAGE_DATA);
        		while (!(gpHalMsGetStatus() & BUFFULL)) {
            		result = msproal_user_check_timer();
            		if(MSPROAL_OK != result) {
            	    	status |= TIMEOUT;
            	    	break;
            		}
        		}
        		msproal_user_end_timer();
    			gpHalMsReadData(ptr,8);
    			
    			status |= gpHalMsGetStatus();
    			if ((status & TIMEOUT) || (status & CRCERR)) {
            		break;
        		}
    			ptr += 8;
    		}
    		
    		msproal_user_start_timer(MSPROAL_TIMEOUT_READ_PAGE_DATA);
        	while (!(gpHalMsGetStatus() & CMDCOM)) {
        	    result = msproal_user_check_timer();
        	    if(MSPROAL_OK != result) {
        	        return MSPROAL_SYSTEM_ERR;
        	    }
        	}
        	msproal_user_end_timer();
    		
    		if (status & TIMEOUT) {
            	continue;
        	}
        	if (status & CRCERR) {
            	return MSPROAL_ACCESS_ERR; 
        	}
        }
	    else {
	    	gpHalMsClearStatus();
	    	gpHalMsSetFifoLevel(TX_TRI_L1,RX_TRI_L1);
	    	
	    	dma_param.module = MS;
	    	dma_param.buf0 = (unsigned char*)virt_to_phys(data);
			dma_param.ln0 = 512;
			dma_param.dir = 0;
			dma_param.buf1 = 0;
			dma_param.ln1 = 0;
			
			gp_apbdma0_en(msifhndl->handle_dma,dma_param);
			
        	gpHalMsSendCmd(RUNCMD|TPC_READ_PAGE_DATA,0,0);
        	
        	msproal_user_start_timer(MSPROAL_TIMEOUT_READ_PAGE_DATA+10);
        	while (!(gpHalMsGetStatus() & CMDCOM)) {
        	    result = msproal_user_check_timer();
        	    if(MSPROAL_OK != result) {
        	        return MSPROAL_SYSTEM_ERR;
        	    }
        	}
        	msproal_user_end_timer();
        	
        	msproal_user_start_timer(MSPROAL_TIMEOUT_READ_PAGE_DATA);
        	status = 0;
        	while(gp_apbdma0_trywait(msifhndl->handle_dma)!= 0) {
        	    result = msproal_user_check_timer();
        	    if(MSPROAL_OK != result) {
        	        status |= TIMEOUT;
        	        break;
        	    }
        	}
        	msproal_user_end_timer();
        	
        	status |= gpHalMsGetStatus();
        	            
        	if (status & TIMEOUT) {
        	    continue;
        	}
        	if (status & CRCERR) {
        	    return MSPROAL_ACCESS_ERR; //If want to retry for crc erorr in read page,it should be re-send SET_EXT_CMD.
        	                               //TEST# : T3-011 passed by can't judgement
        	}
        }   
        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  !(((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))  */
        /*  && (1 == MSPROAL_SUPPORT_DMA))                              */

#if         (((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   \
            && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_page
*   DESCRIPTION : Read one page(512 bytes) of data from the Memory Stick and
*               stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_page(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_tpc_read_page(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;
    UBYTE   *dst, *src;

    msproal_user_virt_to_bus(   (void *)MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)data, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)data, 0x200);

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* set DMAC for sms2ip */
        result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                        (void *)src,
                                        (void *)dst,
                                        0x200,
                                        MSPROAL_SELECT_DATA);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_tpc_set_tpc(msifhndl, TPC_READ_PAGE_DATA, 0x200);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_READ_PAGE_DATA,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            msproal_user_end_dma();

            if(MSPROAL_HOST_TOE_ERR != result) {
                if(MSPROAL_HOST_CRC_ERR != result) {
                    return result;
                }

                break;
            }

            continue;
        }

        /* Waiting DMA finish */
        return msproal_tpc_wait_int(msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */
        /*  && (1 == MSPROAL_SUPPORT_DMA))                              */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
#if         !((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_quad_long_data
*   DESCRIPTION : Read four page(2048 bytes) of data from the Memory Stick and
*               stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_quad_long_data(MSIFHNDL *msifhndl,UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_tpc_read_quad_long_data(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* Issuing of READ_QUAD_LONG_DATA TPC */
        result = msproal_tpc_set_tpc(msifhndl, TPC_READ_QUAD_LONG_DATA, 0x800);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_READ_QUAD_LONG_DATA);

        /* Read data from FIFO in units of page( = 2048 bytes) */
        result = msproal_tpc_read_nfifo(msifhndl, 0x800, data);

        /* End Timer */
        msproal_user_end_timer();

        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_RDY,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_CRC_ERR != result) {
                return result;
            }

            break;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))  */
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_tpc_read_quad_long_data
*   DESCRIPTION : Read four page(2048 bytes) of data from the Memory Stick and
*               stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_quad_long_data(MSIFHNDL *msifhndl,UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_tpc_read_quad_long_data(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;
    UBYTE   *dst, *src;

    msproal_user_virt_to_bus(   (void *)MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)data, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)data, 0x800);

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* set DMAC for Host Controller */
        result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                        (void *)src,
                                        (void *)dst,
                                        0x800,
                                        MSPROAL_SELECT_DATA);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Issuing of READ_QUAD_LONG_DATA TPC */
        result = msproal_tpc_set_tpc(msifhndl, TPC_READ_QUAD_LONG_DATA, 0x800);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_READ_QUAD_LONG_DATA,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            msproal_user_end_dma();

            if(MSPROAL_HOST_TOE_ERR != result) {
                if(MSPROAL_HOST_CRC_ERR != result) {
                    return result;
                }

                break;
            }

            continue;
        }

        /* Waiting DMA finish */
        return msproal_tpc_wait_int(msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))   */

/******************************************************************************
*   FUNCTION    : msproal_tpc_read_reg
*   DESCRIPTION : Read the size amount of data from the Memory Stick register
*               and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_reg(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : Consecutive size for READ_REG(READ_SIZE)
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_tpc_read_reg(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    SINT    result, retry;
    UINT    i,status;
    UINT    *ptr;

    retry = MSPROAL_RETRY_COUNT;
    do {
        ptr = (UINT *)data;
        
        gpHalMsSendCmd(RUNCMD|TPC_READ_REG,0,size);
        status = 0;
        for (i=0;i<(size+3)/4;i++) {
            msproal_user_start_timer(MSPROAL_TIMEOUT_READ_REG);
            while (!(gpHalMsGetStatus() & REGBUFFULL)) {
                result = msproal_user_check_timer();
                if(MSPROAL_OK != result) {
                    status |= TIMEOUT;
                    break;
                }
            }
            msproal_user_end_timer();
            
            status |= gpHalMsGetStatus();
            if ((status & TIMEOUT) || (status & CRCERR)) {
                break;
            }
            *ptr++ = gpHalMsReadReg();  
        }
        
        msproal_user_start_timer(MSPROAL_TIMEOUT_READ_REG);
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        msproal_user_end_timer();
        
        if ((status & TIMEOUT) || (status & CRCERR)) {
            continue;
        }
        
        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

/******************************************************************************
*   FUNCTION    : msproal_tpc_read_short_data
*   DESCRIPTION : Read the size amount of data from the Data Buffer of
*               the Memory Stick and stores it in data.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_read_short_data(MSIFHNDL *msifhndl, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : The numbers of bytes to read
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_tpc_read_short_data(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    SINT    result, retry;
    UINT    i;
    UINT    status;
    UINT    *ptr;
    
    retry = MSPROAL_RETRY_COUNT;
    do {
        ptr = (UINT *)data;
        
        gpHalMsSetFifoLevel(TX_TRI_L1,RX_TRI_L8);
        gpHalMsSendCmd((RUNCMD|TPC_READ_SHORT_DATA),0,size);
        
        status = 0;
        for (i=0;i<(size/4);i+=8) {
            msproal_user_start_timer(MSPROAL_TIMEOUT_READ_SHORT_DATA);
            while (!(gpHalMsGetStatus()&BUFFULL)) {
                result = msproal_user_check_timer();
                if(MSPROAL_OK != result) {
                    status |= TIMEOUT;
                    break;
                }
            }   
            msproal_user_end_timer();
            status |= gpHalMsGetStatus();
            
            if ((status & TIMEOUT) || (status & CRCERR)) {
                break;
            }
            
            gpHalMsReadData(ptr, 8);
            ptr += 8;
        }
        
        msproal_user_start_timer(MSPROAL_TIMEOUT_READ_SHORT_DATA);
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        msproal_user_end_timer();
        
        if ((status & TIMEOUT) || (status & CRCERR)) {
                continue;
        }
        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
#if         !((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_reset_host
*   DESCRIPTION : Host Controller is reset.
*------------------------------------------------------------------------------
*   void msproal_tpc_reset_host(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl        : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_tpc_reset_host(MSIFHNDL *msifhndl)
{
    UWORD   hsysreg;

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    /* Setting RST=1 */
    hsysreg |= MSIF_SYS_RST;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

    msproal_user_start_timer(1);

    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    while(hsysreg & MSIF_SYS_RST) {
        if(MSPROAL_OK != msproal_user_check_timer()) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    }

    msproal_user_end_timer();

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    /* Serial I/F Interrupt enable */
    hsysreg |= MSIF_SYS_INTEN;
    /* Setting BSYCNT=<usual> */
    hsysreg &= MSIF_SYS_BSYCNT_MASK;
    hsysreg |= MSIF_SYS_BSYCNT_USUAL;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

    return MSPROAL_OK;
}
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))  */
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_reset_host
*   DESCRIPTION : Host Controller is reset.
*------------------------------------------------------------------------------
*   void msproal_tpc_reset_host(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl        : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_tpc_reset_host(MSIFHNDL *msifhndl)
{
    UWORD   hsysreg, hfifoctrlreg, hdmactrlreg;

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    /* Setting RST=1 */
    hsysreg |= MSIF_SYS_RST;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

    msproal_user_start_timer(1);

    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    while(hsysreg & MSIF_SYS_RST) {
        if(MSPROAL_OK != msproal_user_check_timer()) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    }

    msproal_user_end_timer();

    /*----      System Register access          -----------------------------*/
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &hsysreg);
    /* Serial I/F Interrupt enable */
    hsysreg |= MSIF_SYS_INTEN;
    /* Setting BSYCNT=<usual> */
    hsysreg &= MSIF_SYS_BSYCNT_MASK;
    hsysreg |= MSIF_SYS_BSYCNT_USUAL;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), hsysreg);

    /*----      FIFO Control Register access    -----------------------------*/
    hfifoctrlreg = MSIF_FIFO_CTRL_DRSZ_4;
    msproal_user_write_mem16(   MSIF_FIFO_CTRL_ADRS(msifhndl->BaseAdrs),
                                hfifoctrlreg);

    /*----      DMA Control Register access     -----------------------------*/
    hdmactrlreg = (MSIF_DMA_CTRL_DMAON_2
                | MSIF_DMA_CTRL_DMAON_5
                | MSIF_DMA_CTRL_DMAON_10
                | MSIF_DMA_CTRL_DMAON_13);
    msproal_user_write_mem16(   MSIF_DMA_CTRL_ADRS(msifhndl->BaseAdrs),
                                hdmactrlreg);

    return MSPROAL_OK;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_reset_host
*   DESCRIPTION : Host Controller is reset.
*------------------------------------------------------------------------------
*   void msproal_tpc_reset_host(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*   ARGUMENT
*       msifhndl        : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_tpc_reset_host(MSIFHNDL *msifhndl)
{
    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

/******************************************************************************
*   FUNCTION    : msproal_tpc_set_bsycnt
*   DESCRIPTION : Set bsycnt to BSY of Host Controller.
*------------------------------------------------------------------------------
*   Void msproal_tpc_set_bsycnt(MSIFHNDL *msifhndl, SINT bsycnt)
*   RETURN
*       None
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       bsycnt      : Busy count
******************************************************************************/
void msproal_tpc_set_bsycnt(MSIFHNDL *msifhndl, SINT bsycnt)
{
    return;
}

/******************************************************************************
*   FUNCTION    : msproal_tpc_set_cmd
*   DESCRIPTION : Execute the SET_CMD command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_set_cmd(MSIFHNDL *msifhndl, SINT cmd)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       cmd         : Command
******************************************************************************/
SINT msproal_tpc_set_cmd(MSIFHNDL *msifhndl, SINT cmd)
{
    SINT    result, retry;
    UINT    status;

    retry = MSPROAL_RETRY_COUNT;
    do {
		gpHalMsSendCmd((RUNCMD|TPC_SET_CMD) | ((cmd<<8)&0xFF00),0,0);
        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_SET_CMD);
        
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        
        /* End Timer */
        msproal_user_end_timer();
        status = gpHalMsGetStatus();
       
        if (status & TIMEOUT) {
            continue;
        }
        
        if (status & CRCERR) {
            return MSPROAL_SYSTEM_ERR;
        }
        
        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

/******************************************************************************
*   FUNCTION    : msproal_tpc_set_rw_reg_adrs
*   DESCRIPTION : Set the start addresses(radrs, wadrs) and
*               data size(rsize, wsize) of the Memory Stick to be read or
*               written.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_set_rw_reg_adrs(MSIFHNDL *msifhndl, SINT radrs,
*           SINT rsize, SINT wadrs, SINT wsize)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       radrs       : Starting address for READ_REG(READ_ADRS)
*       rsize       : Read data size for READ_REG(READ_SIZE)
*       wadrs       : Starting address for WRITE_REG(WRITE_ADRS)
*       wsize       : Write data size for WRITE_REG(WRITE_SIZE)
******************************************************************************/
SINT msproal_tpc_set_rw_reg_adrs(MSIFHNDL *msifhndl, SINT radrs, SINT rsize,
        SINT wadrs, SINT wsize)
{
    SINT    result, retry;
    UINT    argument,status;

    retry = MSPROAL_RETRY_COUNT;
    
    do {
        argument = ((wsize<<24)&0xFF000000) | ((wadrs<<16)&0xFF0000) | ((rsize<<8)&0xFF00) | (radrs&0xFF);
        
        gpHalMsSendCmd((RUNCMD|TPC_SET_RW_REG_ADRS),argument,0);
        /* Start Timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_SET_RW_REG_ADRS);
        
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        
        /* End Timer */
        msproal_user_end_timer();
        status = gpHalMsGetStatus();
        
        if (status & TIMEOUT) {
            continue;
        }
        if (status & CRCERR) {
            return MSPROAL_SYSTEM_ERR;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_set_tpc
*   DESCRIPTION : Execute the TPC command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_set_tpc(MSIFHNDL *msifhndl, SINT cmd, SINT size)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       cmd         : Command
*       size        : Read/Write Data Size
******************************************************************************/
SINT msproal_tpc_set_tpc(MSIFHNDL *msifhndl, SINT cmd, SINT size)
{
    ULONG   ptn;
    SINT    result;

    switch(cmd) {
    case TPC_READ_QUAD_LONG_DATA:
    case TPC_WRITE_QUAD_LONG_DATA:
        size = 0x0000;
        break;
    case TPC_READ_PAGE_DATA:
    case TPC_WRITE_PAGE_DATA:
        size = 0x0200;
        break;
    case TPC_READ_REG:
    case TPC_WRITE_REG:
        size &= 0x07FF;
        break;
    case TPC_READ_SHORT_DATA:
    case TPC_WRITE_SHORT_DATA:
        size &= 0x07FF;
        break;
    case TPC_GET_INT:
        size = 0x0001;
        break;
    case TPC_SET_RW_REG_ADRS:
        size = 0x0004;
        break;
    case TPC_SET_CMD:
        size = 0x001;
        break;
    case TPC_EX_SET_CMD:
        size = 0x0007;
        break;
#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
    case TPC_READ_MG_STTS_REG:
        size = 0x0002;
        break;
    case TPC_READ_MGD_REG:
        size = 0x0018;
        break;
    case TPC_WRITE_MGD_REG:
        size = 0x001A;
        break;
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */
    default :
        return MSPROAL_PARAM_ERR;
    }

    /* Clear "IntState" */
    msifhndl->IntState = MSPROAL_CLEAR_INTSTATE;

    ptn = MSPROAL_FLG_EXTRACT;
    result = msproal_user_check_flg(ptn);
    if(MSPROAL_OK == result) {
        msproal_user_clear_flg(~ptn);

        return MSPROAL_EXTRACT_ERR;
    }

    ptn = 0xFFFF | MSPROAL_FLG_DMA;
    msproal_user_clear_flg(~ptn);

    cmd = (cmd << 12) | size;
    msproal_user_write_mem16(MSIF_CMD_ADRS(msifhndl->BaseAdrs), (UWORD)cmd);

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_set_tpc
*   DESCRIPTION : Execute the TPC command issue process.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_set_tpc(MSIFHNDL *msifhndl, SINT cmd, SINT size)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       cmd         : Command
*       size        : Read/Write Data Size
******************************************************************************/
SINT msproal_tpc_set_tpc(MSIFHNDL *msifhndl, SINT cmd, SINT size)
{
    ULONG   ptn;
    SINT    result;

    switch(cmd) {
    case TPC_READ_PAGE_DATA:
    case TPC_WRITE_PAGE_DATA:
        size = 0x0200;
        break;
    case TPC_READ_REG:
    case TPC_WRITE_REG:
        size &= 0x03FF;
        break;
    case TPC_READ_SHORT_DATA:
    case TPC_WRITE_SHORT_DATA:
        size &= 0x03FF;
        break;
    case TPC_GET_INT:
        size = 0x0001;
        break;
    case TPC_SET_RW_REG_ADRS:
        size = 0x0004;
        break;
    case TPC_SET_CMD:
        size = 0x001;
        break;
    case TPC_EX_SET_CMD:
        size = 0x0007;
        break;
#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
    case TPC_READ_MG_STTS_REG:
        size = 0x0002;
        break;
    case TPC_READ_MGD_REG:
        size = 0x0018;
        break;
    case TPC_WRITE_MGD_REG:
        size = 0x001A;
        break;
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */
    default :
        return MSPROAL_PARAM_ERR;
    }

    /* Clear "IntState" */
    msifhndl->IntState = MSPROAL_CLEAR_INTSTATE;

    ptn = MSPROAL_FLG_EXTRACT;
    result = msproal_user_check_flg(ptn);
    if(MSPROAL_OK == result) {
        msproal_user_clear_flg(~ptn);

        return MSPROAL_EXTRACT_ERR;
    }

    ptn = 0xFFFF | MSPROAL_FLG_DMA;
    msproal_user_clear_flg(~ptn);

    cmd = (cmd << 12) | size;
    msproal_user_write_mem16(MSIF_CMD_ADRS(msifhndl->BaseAdrs), (UWORD)cmd);

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         !(((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))  \
            && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_wait_int
*   DESCRIPTION : Wait for an interrupt factor.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_HOST_CRC_ERR        : Host CRC error
*       MSPROAL_HOST_TOE_ERR        : Host TOE(Time Out Error) error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       time        : Time until Timeout(ms)
*       mode        : Wait Mode(MSPROAL_WMSINT/MSPROAL_WRDY/MSPROAL_WTIME)
******************************************************************************/
SINT msproal_tpc_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
{
    ULONG   ptn;
    SINT    result;

    result = MSPROAL_OK;

    switch(mode) {
    case MSPROAL_WRDY:
        ptn = MSIF_STTS_RDY
            | MSIF_STTS_TOE
            | MSIF_STTS_CRC
            | MSPROAL_FLG_EXTRACT;
        result = msproal_user_wait_flg(ptn, time);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_clear_flg(~(ptn | MSIF_STTS_DRQ));

        if(msifhndl->IntState & MSPROAL_FLG_EXTRACT) {
            result = msproal_tpc_reset_host(msifhndl);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Did interrupt occur by TOE or CRC? */
        if(msifhndl->IntState & (MSIF_STTS_CRC | MSIF_STTS_TOE)) {
            result = (msifhndl->IntState & MSIF_STTS_TOE) ?
                        MSPROAL_HOST_TOE_ERR : MSPROAL_HOST_CRC_ERR;
        }
        break;
    case MSPROAL_WMSINT_SERIAL:
        msproal_user_start_timer(time);
        while(!(gpHalMsGetStatus() & MSINT_CED)) {
            /* Did interrupt occur by timer? */
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_TIMEOUT_ERR;
            }
        }
        /* End timer */
        msproal_user_end_timer();
        break;
    case MSPROAL_WMSINT_PARALLEL:
        msproal_user_start_timer(time);
        while(!(gpHalMsGetStatus() & (MSINT_CED|MSINT_ERR|MSINT_BREQ|MSINT_CMDNK))) {
            /* Did interrupt occur by timer? */
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_TIMEOUT_ERR;
            }
        }
        /* End timer */
        msproal_user_end_timer();
        break;
    case MSPROAL_WTIME:
        msproal_user_wait_time(time);

        break;
    default :
        result = MSPROAL_PARAM_ERR;
        break;
    }

    return result;
}
#endif  /*  !(((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))  */
        /*  && (1 == MSPROAL_SUPPORT_DMA))                              */

#if         (((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   \
            && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_wait_int
*   DESCRIPTION : Wait for an interrupt factor.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_HOST_CRC_ERR        : Host CRC error
*       MSPROAL_HOST_TOE_ERR        : Host TOE(Time Out Error) error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       time        : Time until Timeout(ms)
*       mode        : Wait Mode(MSPROAL_WMSINT/MSPROAL_WRDY/MSPROAL_WTIME/
*                   MSPROAL_WDMA)
******************************************************************************/
SINT msproal_tpc_wait_int(MSIFHNDL *msifhndl, SINT time, SINT mode)
{
    ULONG   ptn;
    SINT    result;

    result = MSPROAL_OK;

    switch(mode) {
    case MSPROAL_WRDY:
        ptn = MSIF_STTS_RDY
            | MSIF_STTS_TOE
            | MSIF_STTS_CRC
            | MSPROAL_FLG_EXTRACT;
        result = msproal_user_wait_flg(ptn, time);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_clear_flg(~(ptn | MSIF_STTS_DRQ));

        if(msifhndl->IntState & MSPROAL_FLG_EXTRACT) {
            result = msproal_tpc_reset_host(msifhndl);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Did interrupt occur by TOE or CRC? */
        if(msifhndl->IntState & (MSIF_STTS_CRC | MSIF_STTS_TOE)) {
            result = (msifhndl->IntState & MSIF_STTS_TOE) ?
                        MSPROAL_HOST_TOE_ERR : MSPROAL_HOST_CRC_ERR;
        }
        break;
    case MSPROAL_WMSINT:
        ptn = MSIF_STTS_MSINT | MSPROAL_FLG_EXTRACT;

        result = msproal_user_wait_flg(ptn, time);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_clear_flg(~(ptn | 0xFF));

        if(msifhndl->IntState & MSPROAL_FLG_EXTRACT) {
            return MSPROAL_EXTRACT_ERR;
        }
        break;
    case MSPROAL_WTIME:
        msproal_user_wait_time(time);

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
#endif  /*  (((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */
        /*  && (1 == MSPROAL_SUPPORT_DMA))                              */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_fifo
*   DESCRIPTION : Write data to the DATA Register of the Host Controller.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_fifo(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area(8 bytes) where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_fifo(MSIFHNDL *msifhndl, UBYTE *data)
{
    ULONG   ptn;
    SINT    result;
    UWORD   hsttsreg;

    ptn = MSPROAL_FLG_EXTRACT;

    /* Waiting for interrupt by DRQ */
    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    while(!(hsttsreg & MSIF_STTS_DRQ)) {
        result = msproal_user_check_flg(ptn);
        if(MSPROAL_OK == result) {
            msproal_user_clear_flg(~ptn);

            result = msproal_tpc_reset_host(msifhndl);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Did interrupt occur by timer */
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    }

    /* Write data to DATA Register */
    msproal_user_write_mem32(   MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                *(ULONG *)(data));
    msproal_user_write_mem32(   MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                *(ULONG *)(data + sizeof(ULONG)));

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_fifo
*   DESCRIPTION : Write data to the DATA Register of the Host Controller.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_fifo(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area(8 bytes) where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_fifo(MSIFHNDL *msifhndl, UBYTE *data)
{
    ULONG   ptn;
    SINT    result;
    UWORD   hsttsreg;

    ptn = MSPROAL_FLG_EXTRACT;

    /* Waiting for interrupt by DRQ */
    msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    while(!(hsttsreg & MSIF_STTS_DRQ)) {
        result = msproal_user_check_flg(ptn);
        if(MSPROAL_OK == result) {
            msproal_user_clear_flg(~ptn);

            result = msproal_tpc_reset_host(msifhndl);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            return MSPROAL_EXTRACT_ERR;
        }

        /* Did interrupt occur by timer */
        result = msproal_user_check_timer();
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
    }

    /* Write data to DATA Register */
#if         (4 == MSPROAL_ALIGN_BYTES)
    msproal_user_write_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                             *(MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 0));
    msproal_user_write_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                             *(MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 1));
#else   /*  (2 == MSPROAL_ALIGN_BYTES)  */
    msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                             *(MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 0));
    msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                             *(MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 1));
    msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                             *(MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 2));
    msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                             *(MSIF_MODEL_SIZE *)(data + MSIF_ALIGN_BYTE * 3));
#endif  /*  (4 == MSPROAL_ALIGN_BYTES)  */

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_mgd_reg
*   DESCRIPTION : Write data to the MGD WRITE Register of the Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_mgd_reg(MSIFHNDL *msifhndl, UBYTE *data0,
*           UBYTE *data1, UBYTE *data2, UBYTE *J)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data0       : Address to area(8 bytes) where Data written in 00h-07h
*                   is stored
*       data1       : Address to area(8 bytes) where Data written in 08h-0fh
*                   is stored
*       data2       : Address to area(8 bytes) where Data written in 10h-17h
*                   is stored
*       J           : Address to area(2 bytes) where Data(attestation number)
*                   written in 18h-19h is stored
******************************************************************************/
SINT msproal_tpc_write_mgd_reg(MSIFHNDL *msifhndl, UBYTE *data0, UBYTE *data1,
        UBYTE *data2, UBYTE *J)
{
    SINT    result, retry;
    UBYTE   *mgd_reg[3], **pend, **pread;

    retry       = MSPROAL_RETRY_COUNT;
    mgd_reg[0]  = data0;
    mgd_reg[1]  = data1;
    mgd_reg[2]  = data2;
    pend        = mgd_reg + 3;
    do {
        /* Issuing of WRITE_MGD_REG TPC */
        result = msproal_tpc_set_tpc(msifhndl, TPC_WRITE_MGD_REG, 26);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Start timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_REG);

        /* Write data to MGD WRITE Register(00h-17h) */
        pread = mgd_reg;
        while(pend > pread) {
            result = msproal_tpc_write_fifo(msifhndl, *pread);
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            pread ++;
        }

        /* Write data to MGD WRITE Register(18h-19h) */
        result = msproal_tpc_write_nfifo(msifhndl, 2, J);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* End Timer */
        msproal_user_end_timer();

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_RDY,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_nfifo
*   DESCRIPTION : Write size byte of data to the Data register.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : The numbers of bytes to write
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    ULONG   ptn;
    ULONG   work[2];
    SINT    result, cnt, rem;
    UWORD   hsttsreg;
    UBYTE   *pend, *pdata, *pwork;

    ptn = MSPROAL_FLG_EXTRACT;

    pdata   = data;
    pend    = pdata + (size & ~0x7);
    while(pdata < pend) {
        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
        while(!(hsttsreg & MSIF_STTS_DRQ)) {
            result = msproal_user_check_flg(ptn);
            if(MSPROAL_OK == result) {
                msproal_user_clear_flg(~ptn);

                result = msproal_tpc_reset_host(msifhndl);
                if(MSPROAL_OK != result) {
                    return MSPROAL_SYSTEM_ERR;
                }

                return MSPROAL_EXTRACT_ERR;
            }

            /* Did interrupt occur by timer */
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                    &hsttsreg);
        }

        msproal_user_write_mem32(   MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                    *(ULONG *)(pdata));
        msproal_user_write_mem32(   MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                    *(ULONG *)(pdata + sizeof(ULONG)));
        pdata += 8;
    }

    rem = size & 0x7;
    if(rem) {
        pwork = (UBYTE *)work;
        for(cnt = 0; cnt < rem; cnt ++) {
            pwork[cnt] = pdata[cnt];
        }

        for(; cnt < 8; cnt ++) {
            pwork[cnt] = 0;
        }

        result = msproal_tpc_write_fifo(msifhndl, pwork);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_nfifo
*   DESCRIPTION : Write size byte of data to the Data register.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : The numbers of bytes to write
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_nfifo(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    ULONG   ptn;
    ULONG   work[2];
    SINT    result, cnt, rem;
    UWORD   hsttsreg;
    UBYTE   *pend, *pdata, *pwork;

    ptn = MSPROAL_FLG_EXTRACT;

    pdata   = data;
    pend    = pdata + (size & ~0x7);
    while(pdata < pend) {
        msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs), &hsttsreg);
        while(!(hsttsreg & MSIF_STTS_DRQ)) {
            result = msproal_user_check_flg(ptn);
            if(MSPROAL_OK == result) {
                msproal_user_clear_flg(~ptn);

                result = msproal_tpc_reset_host(msifhndl);
                if(MSPROAL_OK != result) {
                    return MSPROAL_SYSTEM_ERR;
                }

                return MSPROAL_EXTRACT_ERR;
            }

            /* Did interrupt occur by timer */
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }

            msproal_user_read_mem16(MSIF_STTS_ADRS(msifhndl->BaseAdrs),
                                    &hsttsreg);
        }

#if         (4 == MSPROAL_ALIGN_BYTES)
        msproal_user_write_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            *(MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 0));
        msproal_user_write_mem32(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            *(MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 1));
#else   /*  (2 == MSPROAL_ALIGN_BYTES)  */
        msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            *(MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 0));
        msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            *(MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 1));
        msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            *(MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 2));
        msproal_user_write_mem16(MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                            *(MSIF_MODEL_SIZE *)(pdata + MSIF_ALIGN_BYTE * 3));
#endif  /*  (4 == MSPROAL_ALIGN_BYTES)  */
        pdata += 8;
    }

    rem = size & 0x7;
    if(rem) {
        pwork = (UBYTE *)work;
        for(cnt = 0; cnt < rem; cnt ++) {
            pwork[cnt] = pdata[cnt];
        }

        for(; cnt < 8; cnt ++) {
            pwork[cnt] = 0;
        }

        result = msproal_tpc_write_fifo(msifhndl, pwork);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         !(((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))  \
            && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_page
*   DESCRIPTION : Write one page(512 bytes) of data stored in data to the
*               Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_page(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_page(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;
    UINT    status;
    UINT    *ptr;
    UINT    i;
    gpApbdma0Param_t	dma_param;
    
    retry = MSPROAL_RETRY_COUNT;
    do {
    	if (msifhndl->XfrMode == MS_CPU_MODE) {
    		gpHalMsSetFifoLevel(TX_TRI_L8,RX_TRI_L1);
    		ptr = (UINT *) data;
    		gpHalMsSendCmd(RUNCMD|TPC_WRITE_PAGE_DATA,0,0);
    		for (i=0;i<16;i++) {	
    			status = 0;
    			msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_PAGE_DATA);
        		while (!(gpHalMsGetStatus() & BUFEMPTY)) {
            		result = msproal_user_check_timer();
            		if(MSPROAL_OK != result) {
            	    	status |= TIMEOUT;
            	    	break;
            		}
        		}
        		msproal_user_end_timer();
    			gpHalMsWriteData(ptr,8);
    			
    			status |= gpHalMsGetStatus();
    			if ((status & TIMEOUT) || (status & CRCERR)) {
            		break;
        		}
    			ptr += 8;
    		}
    		
    		msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_PAGE_DATA);
        	while (!(gpHalMsGetStatus() & CMDCOM)) {
        	    result = msproal_user_check_timer();
        	    if(MSPROAL_OK != result) {
        	        return MSPROAL_SYSTEM_ERR;
        	    }
        	}
        	msproal_user_end_timer();
    		
    		if (status & TIMEOUT) {
            	continue;
        	}
        	if (status & CRCERR) {
            	return MSPROAL_ACCESS_ERR; 
        	}
    	}
    	else {
    		gpHalMsClearStatus();
    		gpHalMsSetFifoLevel(TX_TRI_L1,RX_TRI_L1);
    		
        	dma_param.module = MS;
        	dma_param.buf0 = (unsigned char*)virt_to_phys(data);
			dma_param.ln0 = 512;
			dma_param.dir = 1;
			dma_param.buf1 = 0;
			dma_param.ln1 = 0;
			
			gpHalMsSendCmd(RUNCMD|TPC_WRITE_PAGE_DATA,0,0);
			
			gp_apbdma0_en(msifhndl->handle_dma,dma_param);
        	
        	msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_PAGE_DATA+10);
        	while (!(gpHalMsGetStatus() & CMDCOM)) {
        	    result = msproal_user_check_timer();
        	    if(MSPROAL_OK != result) {
        	        return MSPROAL_SYSTEM_ERR;
        	    }
        	}
        	msproal_user_end_timer();
        	
        	msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_PAGE_DATA);
        	status = 0;
        	while(gp_apbdma0_trywait(msifhndl->handle_dma)!= 0) {
        	    result = msproal_user_check_timer();
        	    if(MSPROAL_OK != result) {
        	        status |= TIMEOUT;
        	        break;
        	    }
        	}
        	msproal_user_end_timer();
        	
        	status |= gpHalMsGetStatus();
        	            
        	if (status & TIMEOUT) {
        	    continue;
        	}
        	if (status & CRCERR) {
        	    return MSPROAL_SYSTEM_ERR;
        	}   
		}
        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  !(((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))  */
        /*  && (1 == MSPROAL_SUPPORT_DMA))                              */

#if         (((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   \
            && (1 == MSPROAL_SUPPORT_DMA))
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_page
*   DESCRIPTION : Write one page(512 bytes) of data stored in data to the
*               Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_page(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_page(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;
    UBYTE   *dst, *src;

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, 0x200);

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* set DMAC for sms2ip */
        result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                        (void *)src,
                                        (void *)dst,
                                        0x200,
                                        MSPROAL_SELECT_DATA);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Issuing of WRITE_PAGE_DATA TPC */
        result = msproal_tpc_set_tpc(msifhndl, TPC_WRITE_PAGE_DATA, 0x200);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_WRITE_PAGE_DATA,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            msproal_user_end_dma();
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        /* Waiting DMA finish */
        return msproal_tpc_wait_int(msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */
        /*  && (1 == MSPROAL_SUPPORT_DMA))                              */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
#if         !((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_quad_long_data
*   DESCRIPTION : Write four page(2048 bytes) of data stored in data to the
*               Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_quad_long_data(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_quad_long_data(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* Issuing of WRITE_QUAD_LONG_DATA TPC */
        result = msproal_tpc_set_tpc(   msifhndl,
                                        TPC_WRITE_QUAD_LONG_DATA,
                                        0x800);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Start timer */
        msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_QUAD_LONG_DATA);

        result = msproal_tpc_write_nfifo(msifhndl, 0x800, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* End timer */
        msproal_user_end_timer();

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        MSPROAL_TIMEOUT_RDY,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))  */
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_tpc_write_quad_long_data
*   DESCRIPTION : Write four page(2048 bytes) of data stored in data to the
*               Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_quad_long_data(MSIFHNDL *msifhndl, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_quad_long_data(MSIFHNDL *msifhndl, UBYTE *data)
{
    SINT    result, retry;
    UBYTE   *dst, *src;

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)MSIF_DATA_ADRS(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, 0x800);

    retry = MSPROAL_RETRY_COUNT;
    do {
        /* set DMAC for Host Controller */
        result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                        (void *)src,
                                        (void *)dst,
                                        0x800,
                                        MSPROAL_SELECT_DATA);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Issuing of WRITE_QUAD_LONG_DATA TPC */
        result = msproal_tpc_set_tpc(   msifhndl,
                                        TPC_WRITE_QUAD_LONG_DATA,
                                        0x800);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Waiting for interrupt by RDY */
        result = msproal_tpc_wait_int(  msifhndl,
                                        TPC_WRITE_QUAD_LONG_DATA,
                                        MSPROAL_WRDY);
        if(MSPROAL_OK != result) {
            msproal_user_end_dma();
            if(MSPROAL_HOST_TOE_ERR != result) {
                return result;
            }

            continue;
        }

        /* Waiting DMA finish */
        return msproal_tpc_wait_int(msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) && (1 == MSPROAL_SUPPORT_DMA))   */

/******************************************************************************
*   FUNCTION    : msproal_tpc_write_reg
*   DESCRIPTION : Write the size amount of data to the Memory Stick register.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_reg(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : Consecutive size for WRITE_REG(WRITE_SIZE)
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_reg(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    SINT    result, retry;
    UINT    i,status;
    UINT    *ptr;
    
    retry = MSPROAL_RETRY_COUNT;
    do {
        ptr = (UINT *)data;
		gpHalMsSendCmd((RUNCMD|TPC_WRITE_REG),*ptr++,size);
		
        status = 0; 
        for (i=1;i<(size+3)/4;i++) {
            msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_REG);
            while (!(gpHalMsGetStatus() & REGBUFEMPTY)) {
                result = msproal_user_check_timer();
                if(MSPROAL_OK != result) {
                    status |= TIMEOUT;
                    break;
                }
            }
            msproal_user_end_timer();
            
            gpHalMsWriteReg(*ptr++);
        }
        
        msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_REG);
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        msproal_user_end_timer();
        status |= gpHalMsGetStatus();
        
        if (status & TIMEOUT) {
            continue;
        }
        
        if (status & CRCERR) {
            return MSPROAL_SYSTEM_ERR;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}

/******************************************************************************
*   FUNCTION    : msproal_tpc_write_short_data
*   DESCRIPTION : Write the size amount of data to the Data Buffer of the
*               Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_tpc_write_short_data(MSIFHNDL *msifhndl, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       size        : The numbers of bytes to write
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_tpc_write_short_data(MSIFHNDL *msifhndl, SINT size, UBYTE *data)
{
    SINT    result, retry;
    UINT    status,i;
    UINT    *ptr;

    retry = MSPROAL_RETRY_COUNT;
    do {
        ptr = (UINT *)data;
        
        gpHalMsSendCmd((RUNCMD|TPC_WRITE_SHORT_DATA),0,0);
        
        status = 0;
        for (i=0;i<(size/4);i=i+8) {
            msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_SHORT_DATA);
            while (!(gpHalMsGetStatus()&BUFEMPTY)) {
                result = msproal_user_check_timer();
                if(MSPROAL_OK != result) {
                    status |= TIMEOUT;
                    break;
                }
            }   
            msproal_user_end_timer();
            status |= gpHalMsGetStatus();
            
            if ((status & TIMEOUT) || (status & CRCERR)) {
                break;
            }
            
            gpHalMsWriteData(ptr,8);
            ptr += 8;
        }
		
		msproal_user_start_timer(MSPROAL_TIMEOUT_WRITE_SHORT_DATA);
        while (!(gpHalMsGetStatus() & CMDCOM)) {
            result = msproal_user_check_timer();
            if(MSPROAL_OK != result) {
                return MSPROAL_SYSTEM_ERR;
            }
        }
        msproal_user_end_timer();
        
        if ((status & TIMEOUT) || (status & CRCERR)) {
                continue;
        }

        return MSPROAL_OK;
    } while(retry --);

    return MSPROAL_ACCESS_ERR;
}
