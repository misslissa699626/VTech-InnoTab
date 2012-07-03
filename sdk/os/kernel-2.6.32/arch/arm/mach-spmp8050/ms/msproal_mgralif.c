/*=============================================================================
* Copyright 2002-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_mgralif.c
*
* DESCRIPTION   : MGRAL I/F API
*
* FUNCTION LIST
*                   msal_read_block
*                   msal_read_extradata
*                   msproal_mgralif_init
*                   tpc_read_mg_stts_reg
*                   tpc_read_mgd_reg
*                   tpc_set_cmd
*                   tpc_write_mgd_reg
=============================================================================*/
#include <mach/ms/msal.h>
#include <mach/ms/msproal_config.h>

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
MSHNDL  mshndl;
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : msal_read_block
*   DESCRIPTION : Reads PageData and ExtraData from the page within the range
*               of instruction in the instructed block.
*------------------------------------------------------------------------------
*   int msal_read_block(int pblk, int spage, int epage, unsigned char *data,
*           unsigned short *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       pblk        : Physical block number being read
*       spage       : Start page number being read
*       epage       : End page number being read
*       data        : Address to area where read Data is stored
*       extradata   : Address to area where read ExtraData is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
*                       Offset4 : OverwriteFlag
*                           :       :
******************************************************************************/
int msal_read_block(int pblk, int spage, int epage, unsigned char *data,
        unsigned short *extradata)
{
    SINT    result, cnt;
    UBYTE   *extra;

    extra = &msifhndl->DataBuf[0];
    result = msproal_seq_v1_read_block( msifhndl,
                                        pblk,
                                        spage,
                                        epage,
                                        data,
                                        extra);

    for(cnt = 0; (epage - spage) >= cnt; cnt ++) {
        *extradata ++ = MAKEWORD(extra[cnt * 4], extra[cnt * 4 + 1]);
        *extradata ++ = MAKEWORD(extra[cnt * 4 + 2], extra[cnt * 4 + 3]);
    }

    return result;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : msal_read_extradata
*   DESCRIPTION : Reads ExtraData from the instructed page of the instructed
*               block.
*------------------------------------------------------------------------------
*   int msal_read_extradata(int pblk, int page, unsigned short *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       pblk        : Physical block number being read
*       page        : Page number being read
*       extradata   : Address to area where read ExtraData is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
int msal_read_extradata(int pblk, int page, unsigned short *extradata)
{
    SINT    result;
    UBYTE   extra[4];

    result = msproal_seq_v1_read_extradata( msifhndl,
                                            pblk,
                                            page,
                                            extra);

    extradata[0] = MAKEWORD(extra[0], extra[1]);
    extradata[1] = MAKEWORD(extra[2], extra[3]);

    return result;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : msproal_mgralif_init
*   DESCRIPTION : Initialization (setting MGRAL).
*------------------------------------------------------------------------------
*   SINT msproal_mgralif_init(void)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       None
******************************************************************************/
SINT msproal_mgralif_init(void)
{
    return MSPROAL_OK;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))  */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : tpc_read_mg_stts_reg
*   DESCRIPTION : Reads data from the MG STTS Register of Memory Stick
*               and stores it in data.
*------------------------------------------------------------------------------
*   int tpc_read_mg_stts_reg(MSHNDL *mshndl, unsigned char *status)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       mshndl      : Address to initialized MSHNDL type
*       status      : Address to area(2 bytes) where read Status is stored
******************************************************************************/
int tpc_read_mg_stts_reg(MSHNDL *mshndl, unsigned char *status)
{
    int     result, retry;

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
*   FUNCTION    : tpc_read_mgd_reg
*   DESCRIPTION : Reads data from the MGD READ Register of Memory Stick
*               and stores it in data.
*------------------------------------------------------------------------------
*   int tpc_read_mgd_reg(MSHNDL *mshndl, unsigned char *data0,
*           unsigned char *data1, unsigned char *data2)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       mshndl      : Address to initialized MSHNDL type
*       data0       : Address to area(8 bytes) where read Data(00h-07h) is
*                   stored
*       data1       : Address to area(8 bytes) where read Data(08h-0fh) is
*                   stored
*       data2       : Address to area(8 bytes) where read Data(10h-17h) is
*                   stored
******************************************************************************/
int tpc_read_mgd_reg(MSHNDL *mshndl, unsigned char *data0,
        unsigned char *data1, unsigned char *data2)
{
    int             result, retry;
    unsigned char   *mgd_reg[3], **pend, **pread;

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

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : tpc_set_cmd
*   DESCRIPTION : Issue processing of SET_CMD command.
*------------------------------------------------------------------------------
*   int tpc_set_cmd(MSHNDL *mshndl, int cmd, int mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_ERR           : FlashError(Read,Write,Erase)
*       MSPROAL_CMDNK_ERR           : Command nack
*   ARGUMENT
*       mshndl      : Address to initialized MSHNDL type
*       cmd         : Command
*       mode        : MSIF_NOINTWAIT/MSIF_GETINT
******************************************************************************/
int tpc_set_cmd(MSHNDL *mshndl, int cmd, int mode)
{
    return msproal_msif_set_cmd(msifhndl, cmd, MS_TIMEOUT_MG_COMMAND);
}
#endif  /*  ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))    */

#if         ((1 == MSPROAL_SUPPORT_V1) && (1 == MSPROAL_SUPPORT_MG))
/******************************************************************************
*   FUNCTION    : tpc_write_mgd_reg
*   DESCRIPTION : Write data to the MGD WRITE Register of the Memory Stick.
*------------------------------------------------------------------------------
*   int tpc_write_mgd_reg(MSHNDL *mshndl, unsigned char *data0,
*           unsigned char *data1, unsigned char *data2, unsigned char *J)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       mshndl      : Address to initialized MSHNDL type
*       data0       : Address to area(8 bytes) where Data written in 00h-07h
*                   is stored
*       data1       : Address to area(8 bytes) where Data written in 08h-0fh
*                   is stored
*       data2       : Address to area(8 bytes) where Data written in 10h-17h
*                   is stored
*       J           : Address to area(2 bytes) where Data(attestation number)
*                   written in 18h-19h is stored
******************************************************************************/
int tpc_write_mgd_reg(MSHNDL *mshndl, unsigned char *data0,
        unsigned char *data1, unsigned char *data2, unsigned char *J)
{
    int             result, retry;
    unsigned char   *mgd_reg[3], **pend, **pread;

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
                return result;
            }

            pread ++;
        }

        /* Write data to MGD WRITE Register(18h-19h) */
        result = msproal_tpc_write_nfifo(msifhndl, 2, J);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
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
