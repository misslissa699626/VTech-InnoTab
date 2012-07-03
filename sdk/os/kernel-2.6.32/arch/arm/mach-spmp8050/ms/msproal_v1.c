/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_v1.c
*
* DESCRIPTION   : Memory Stick V1.X API
*
* FUNCTION LIST
*                   msproal_drv_v1_bootblock_confirmation
*                   msproal_drv_v1_change_power
*                   msproal_drv_v1_check_mpbr
*                   msproal_drv_v1_complete_lptbl
*                   msproal_drv_v1_generate_lptbl
*                   msproal_drv_v1_get_model_name
*                   msproal_drv_v1_ladrs_confirmation
*                   msproal_drv_v1_mount
*                   msproal_drv_v1_protect_bootarea
*                   msproal_drv_v1_read_1seg_extradata
*                   msproal_drv_v1_read_atrb_info
*                   msproal_drv_v1_read_nextradata
*                   msproal_drv_v1_recovery
*                   msproal_drv_v1_stop
*                   msproal_drv_v1_wakeup
*                   msproal_seq_v1_clear_buffer
*                   msproal_seq_v1_copy_block
*                   msproal_seq_v1_erase_block
*                   msproal_seq_v1_overwrite_extradata
*                   msproal_seq_v1_read_block
*                   msproal_seq_v1_read_bootblock
*                   msproal_seq_v1_read_extradata
*                   msproal_seq_v1_reset
*                   msproal_seq_v1_sleep
*                   msproal_seq_v1_stop
*                   msproal_seq_v1_write_block
*                   msproal_seq_v1_write_extradata
*                   msproal_tbl_check_useblock
*                   msproal_tbl_get_freeblock
*                   msproal_tbl_init_tbl
*                   msproal_tbl_log_to_phy
*                   msproal_tbl_update_lptbl
*                   msproal_tbl_update_freeblock
*                   msproal_trans_copy_block
*                   msproal_trans_format
*                   msproal_trans_read_lba
*                   msproal_trans_update_block
*                   msproal_trans_write_lba
=============================================================================*/
#include <mach/ms/msproal_v1.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>
#include <mach/ms/msproal_mc_v1.h>


#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_bootblock_confirmation
*   DESCRIPTION : Boot Block content confirmation procedure.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_bootblock_confirmation(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_bootblock_confirmation(MSIFHNDL *msifhndl)
{
    ULONG   sadrs, data_size;
    SINT    result;
    SINT    pblk, page, dblk_num, stick, eseg, ms_no;
    UWORD   blk_size, blk_num, effect_blk_num, dblk;
    const static UWORD  def_blk_size[6] = {
        0x08, 0x08, 0x10, 0x10, 0x10, 0x10
    };
    const static UWORD  def_blk_num[6] = {
        0x200, 0x400, 0x400, 0x800, 0x1000, 0x2000
    };
    const static UWORD  def_effect_blk_num[6] = {
        0x1F0, 0x3E0, 0x3E0, 0x7C0, 0xF80, 0x1F00
    };
    UBYTE   *btdt, *dblkdt;
    UBYTE   devtype, stts1, sup_para;

    pblk = msifhndl->BootBlk;
    btdt = msifhndl->BootData;
    while(-1 != pblk) {
        /*---- check boot&attribute information ----*/
        /* Setting Physical block number, Page number   */
        /* and Single Page Access Mode                  */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                0,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }
		
        /* SET_CMD[BLOCK_READ] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result){
                return result;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result){
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result){
                    return result;
                }

                /* BootBlk which cannot be used is changed into BkBootBlk */
                pblk                = msifhndl->BkBootBlk;
                msifhndl->BootBlk   = pblk;
                msifhndl->BkBootBlk = -1;
                continue;
            }
        }
		
        /* 1 PageData(= 512 bytes) is read */
        result = msproal_tpc_read_page(msifhndl, btdt);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Class of Memory Stick is not 1 */
        if(0x01 != btdt[0x1A0]) {
            msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
            return MSPROAL_UNSUPPORT_ERR;
        }

        devtype = btdt[0x1D8];
        /* Device type is not 0, 1, 2, or 3 */
        if(0x03 < devtype) {
            msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
            return MSPROAL_UNSUPPORT_ERR;
        }

        /* Media type determination processing is performed */
        stick   = msifhndl->Stick;
        /* Class Register is 0x00 or 0xFF */
        if(MSPROAL_STICK_RW & stick) {
            if(0x02 == devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }
        /* Class Register is 0x01 */
        } else if(MSPROAL_STICK_ROM & stick) {
            if((0x00 == devtype) || (0x02 == devtype)) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }
        /* Class Register is 0x02 */
        } else if(MSPROAL_STICK_R & stick) {
            if(0x00 == devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }
        /* Class Register is 0x03 */
        } else {
            if(0x00 == devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }

            devtype = 3;
        }

        /* Media type is determined */
        stick &= ~MSPROAL_STICK_DEVICE_MASK;
        stick |= (1L << devtype);

        /* In case of Read Only */
        if(!(MSPROAL_STICK_RW & stick)) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }

        /* Supporting Parallel determination processing is performed */
        sup_para = btdt[0x1D3];
        if(0 == sup_para) {
            /* Clear Supporting Parallel */
            stick &= ~MSPROAL_STICK_S4P;
        } else if(1 == sup_para) {
            /* Supporting Parallel is 0 */
            if(!(MSPROAL_STICK_S4P & stick)) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }

#if         (1 != MSPROAL_SUPPORT_IFMODE)
            if(MSPROAL_SERIAL_MODE != msifhndl->IfModeMax) {
                result = msproal_msif_change_ifmode(msifhndl,
                                                    MSPROAL_V1_4PARALLEL_MODE);
                if(MSPROAL_OK != result) {
                    return result;
                }
            }
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE)   */
        } else {
            msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
            return MSPROAL_UNSUPPORT_ERR;
        }

        msifhndl->Stick = stick;

        /* Format type is not 1 */
        if(0x01 != btdt[0x1D6]) {
            return MSPROAL_MEDIA_ERR;
        }

        /* Contents of Block size, Number of blocks and */
        /* Number of effective blocks are confirmed     */
        blk_size        = MAKEWORD(btdt[0x1A2], btdt[0x1A3]);
        blk_num         = MAKEWORD(btdt[0x1A4], btdt[0x1A5]);
        effect_blk_num  = MAKEWORD(btdt[0x1A6], btdt[0x1A7]);
        eseg            = blk_num >> 9;
        
        msifhndl->BlockSize = blk_size;
        msifhndl->UserAreaBlocks = effect_blk_num;
        msifhndl->PageSize = MAKEWORD(btdt[0x1A8], btdt[0x1A9]);
        
        /* ms_no =  0   4MB     */
        /*          1   8MB     */
        /*          2   16MB    */
        /*          3   32MB    */
        /*          4   64MB    */
        /*          5   128MB   */
        ms_no = 0;
        while(eseg >>= 1) {
            ms_no ++;
        }
        if(0x10 == blk_size) {
            ms_no ++;
        }
        if(5 < ms_no) {
            return MSPROAL_MEDIA_ERR;
        }
        /* confirm block size */
        if(def_blk_size[ms_no] != blk_size) {
            return MSPROAL_MEDIA_ERR;
        }
        /* confirm number of blocks */
        if(def_blk_num[ms_no] != blk_num) {
            return MSPROAL_MEDIA_ERR;
        }
        /* confirm number of effective blocks */
        if(def_effect_blk_num[ms_no] != effect_blk_num) {
            return MSPROAL_MEDIA_ERR;
        }

        /*---- check system entry ----*/
        /* data type ID is not 1 */
        if(0x01 != btdt[0x178]) {
            return MSPROAL_MEDIA_ERR;
        }

        /* data size is 0 */
        data_size = MAKELONG(   MAKEWORD(btdt[0x176], btdt[0x177]),
                                MAKEWORD(btdt[0x174], btdt[0x175]));
        
        #if 0                    
        __asm {
        	ADDS data_size,data_size,#0
        }
        #endif
                                                        
        if(data_size == 0) {
            return MSPROAL_MEDIA_ERR;
        }

        /* The result which carried out the mask of the start address   */
        /* by 0x1FF is not 0                                            */
        sadrs = MAKELONG(   MAKEWORD(btdt[0x172], btdt[0x173]),
                            MAKEWORD(btdt[0x170], btdt[0x171]));
        #if 0               
        __asm {
        	ADDS sadrs,sadrs,#0
        }
        #endif
        
        if(0x1FF & sadrs) {
            return MSPROAL_MEDIA_ERR;
        }

        /* disabled block data page is calculated */
        /* start address exceed one block size */
        page = (sadrs >> 9) + 1;
        if((blk_size << 1) <= page) {
            return MSPROAL_MEDIA_ERR;
        }

        /*---- check disabled block data ----*/
        /* Setting Physical block number, Page number   */
        /* and Single Page Access Mode                  */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);
        
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result){
                return result;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result){
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result){
                    return result;
                }

                /* BootBlk which cannot be used is changed into BkBootBlk */
                pblk                = msifhndl->BkBootBlk;
                msifhndl->BootBlk   = pblk;
                msifhndl->BkBootBlk = -1;
                continue;
            }
        }

        /* 1 PageData(= 512 bytes) is read */
        dblkdt = &btdt[512];
        result = msproal_tpc_read_page(msifhndl, dblkdt);
        if(MSPROAL_OK != result) {
            return result;
        }

        dblk_num = 0;
        /* While disabled block number is not in agreement with 0xFFFF */
        do {
            dblk = MAKEWORD(*dblkdt, *(dblkdt + 1));
            /* Disabled block number is within the limits of block number */
            if(blk_num > dblk) {
                dblk_num ++;
            }

            dblkdt += 2;
        } while(0xFFFF != dblk);

        /* Disabled block number is not in agreement with the number of */
        /* system entry disabled block data size                        */
        if((data_size >> 1) != (ULONG)dblk_num) {
            return MSPROAL_MEDIA_ERR;
        }

        return MSPROAL_OK;
    }

    return MSPROAL_MEDIA_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_bootblock_confirmation
*   DESCRIPTION : Boot Block content confirmation procedure.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_bootblock_confirmation(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_bootblock_confirmation(MSIFHNDL *msifhndl)
{
    ULONG   sadrs, data_size;
    SINT    result;
    SINT    pblk, page, stick, dblk_num, seg_num, eseg, ms_no, seg_cnt;
    SINT    init_cnt, *dstbl_cnt;
    UWORD   blk_size, blk_num, effect_blk_num, dblk, *dstbl;
    const static UWORD  def_blk_size[6] = {
        0x08, 0x08, 0x10, 0x10, 0x10, 0x10
    };
    const static UWORD  def_blk_num[6] = {
        0x200, 0x400, 0x400, 0x800, 0x1000, 0x2000
    };
    const static UWORD  def_effect_blk_num[6] = {
        0x1F0, 0x3E0, 0x3E0, 0x7C0, 0xF80, 0x1F00
    };
    UBYTE   *btdt, *dblkdt;
    UBYTE   devtype, stts1, sup_para;

    pblk = msifhndl->BootBlk;
    btdt = msifhndl->BootData;
    while(-1 != pblk) {
        /*---- check boot&attribute information ----*/
        /* Setting Physical block number, Page number   */
        /* and Single Page Access Mode                  */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                0,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result){
                return result;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result){
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result){
                    return result;
                }

                /* BootBlk which cannot be used is changed into BkBootBlk */
                pblk                = msifhndl->BkBootBlk;
                msifhndl->BootBlk   = pblk;
                msifhndl->BkBootBlk = -1;
                continue;
            }
        }

        /* 1 PageData(= 512 bytes) is read */
        result = msproal_tpc_read_page(msifhndl, btdt);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Class of Memory Stick is not 1 */
        if(0x01 != btdt[0x1A0]) {
            msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
            return MSPROAL_UNSUPPORT_ERR;
        }

        devtype = btdt[0x1D8];
        /* Device type is not 0, 1, 2, or 3 */
        if(0x03 < devtype) {
            msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
            return MSPROAL_UNSUPPORT_ERR;
        }

        /* Media type determination processing is performed */
        stick   = msifhndl->Stick;
        /* Class Register is 0x00 or 0xFF */
        if(MSPROAL_STICK_RW & stick) {
            if(0x02 == devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }
        /* Class Register is 0x01 */
        } else if(MSPROAL_STICK_ROM & stick) {
            if((0x00 == devtype) || (0x02 == devtype)) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }
        /* Class Register is 0x02 */
        } else if(MSPROAL_STICK_R & stick) {
            if(0x00 == devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }
        /* Class Register is 0x03 */
        } else {
            if(0x00 == devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }

            devtype = 3;
        }

        /* Media type is determined */
        stick &= ~MSPROAL_STICK_DEVICE_MASK;
        stick |= (1L << devtype);

        /* In case of Read Only */
        if(!(MSPROAL_STICK_RW & stick)) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }

        /* Supporting Parallel determination processing is performed */
        sup_para = btdt[0x1D3];
        if(0 == sup_para) {
            /* Clear Supporting Parallel */
            stick &= ~MSPROAL_STICK_S4P;
        } else if(1 == sup_para) {
            /* Supporting Parallel is 0 */
            if(!(MSPROAL_STICK_S4P & stick)) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }

#if         (1 != MSPROAL_SUPPORT_IFMODE)
            result = msproal_msif_change_ifmode(msifhndl,
                                                MSPROAL_V1_4PARALLEL_MODE);
            if(MSPROAL_OK != result) {
                return result;
            }
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE)   */
        } else {
            msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
            return MSPROAL_UNSUPPORT_ERR;
        }

        msifhndl->Stick = stick;

        /* Format type is not 1 */
        if(0x01 != btdt[0x1D6]) {
            return MSPROAL_MEDIA_ERR;
        }

        /* Contents of Block size, Number of blocks and */
        /* Number of effective blocks are confirmed     */
        blk_size        = MAKEWORD(btdt[0x1A2], btdt[0x1A3]);
        blk_num         = MAKEWORD(btdt[0x1A4], btdt[0x1A5]);
        effect_blk_num  = MAKEWORD(btdt[0x1A6], btdt[0x1A7]);
        seg_num         = blk_num >> 9;
        eseg            = seg_num;
        /* ms_no =  0   4MB     */
        /*          1   8MB     */
        /*          2   16MB    */
        /*          3   32MB    */
        /*          4   64MB    */
        /*          5   128MB   */
        ms_no = 0;
        while(eseg >>= 1) {
            ms_no ++;
        }
        if(0x10 == blk_size) {
            ms_no ++;
        }
        if(5 < ms_no) {
            return MSPROAL_MEDIA_ERR;
        }
        /* confirm block size */
        if(def_blk_size[ms_no] != blk_size) {
            return MSPROAL_MEDIA_ERR;
        }
        /* confirm number of blocks */
        if(def_blk_num[ms_no] != blk_num) {
            return MSPROAL_MEDIA_ERR;
        }
        /* confirm number of effective blocks */
        if(def_effect_blk_num[ms_no] != effect_blk_num) {
            return MSPROAL_MEDIA_ERR;
        }

        /*---- check system entry ----*/
        /* data type ID is not 1 */
        if(0x01 != btdt[0x178]) {
            return MSPROAL_MEDIA_ERR;
        }

        /* data size is 0 */
        data_size = MAKELONG(   MAKEWORD(btdt[0x176], btdt[0x177]),
                                MAKEWORD(btdt[0x174], btdt[0x175]));
        if(0 == data_size) {
            return MSPROAL_MEDIA_ERR;
        }

        /* The result which carried out the mask of the start address   */
        /* by 0x1FF is not 0                                            */
        sadrs = MAKELONG(   MAKEWORD(btdt[0x172], btdt[0x173]),
                            MAKEWORD(btdt[0x170], btdt[0x171]));
        if(0x1FF & sadrs) {
            return MSPROAL_MEDIA_ERR;
        }

        /* disabled block data page is calculated */
        /* start address exceed one block size */
        page = (sadrs >> 9) + 1;
        if((blk_size << 1) <= page) {
            return MSPROAL_MEDIA_ERR;
        }

        /*---- check disabled block data ----*/
        /* Setting Physical block number, Page number   */
        /* and Single Page Access Mode                  */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result){
                return result;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result){
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result){
                    return result;
                }

                /* BootBlk which cannot be used is changed into BkBootBlk */
                pblk                = msifhndl->BkBootBlk;
                msifhndl->BootBlk   = pblk;
                msifhndl->BkBootBlk = -1;
                continue;
            }
        }

        /* 1 PageData(= 512 bytes) is read */
        dblkdt = &btdt[512];
        result = msproal_tpc_read_page(msifhndl, dblkdt);
        if(MSPROAL_OK != result) {
            return result;
        }

        dblk_num = 0;
        /* While disabled block number is not in agreement with 0xFFFF */
        dstbl       = msifhndl->DisBlkTbl;
        dstbl_cnt   = msifhndl->DisBlkNum;
        msproal_user_memset(
            (UBYTE *)dstbl,
            0xFF,
            (MS_MAX_SEG_NUM * MS_DS_BLOCKS_IN_SEG * sizeof(UWORD))
        );
        for(init_cnt = 0; init_cnt < MS_MAX_SEG_NUM; init_cnt++) {
            dstbl_cnt[init_cnt] = 0;
        }

        do {
            dblk = MAKEWORD(*dblkdt, *(dblkdt + 1));
            /* Disabled block number is within the limits of block number */
            if(blk_num > dblk) {
                seg_cnt = dblk / MS_BLOCKS_IN_SEG;
                dstbl[seg_cnt * MS_DS_BLOCKS_IN_SEG + dstbl_cnt[seg_cnt]]
                = dblk;
                dstbl_cnt[seg_cnt] += 1;
                dblk_num ++;
            }

            dblkdt += 2;
        } while(0xFFFF != dblk);

        /* Disabled block number is not in agreement with the number of */
        /* system entry disabled block data size                        */
        if((data_size >> 1) != (ULONG)dblk_num) {
            return MSPROAL_MEDIA_ERR;
        }

        /* Physical block number in Disabled Block Data is  */
        /* sorted per segment                               */
        for(seg_cnt = 0; seg_cnt < seg_num; seg_cnt ++) {
            msproal_user_sort(  dstbl + (seg_cnt * MS_DS_BLOCKS_IN_SEG),
                                dstbl_cnt[seg_cnt]);
        }

        return MSPROAL_OK;
    }

    return MSPROAL_MEDIA_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_change_power
*   DESCRIPTION : Change the Power class of Memory Stick. This is dummy
*               function because the method of controlling the Power class
*               doesn't exist.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_change_power(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_PARAM_ERR           : Parameter error
*   ARGUMENT
*       mode        : Power class mode
******************************************************************************/
SINT msproal_drv_v1_change_power(MSIFHNDL *msifhndl, SINT mode)
{
    return MSPROAL_PARAM_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (5 != MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_check_mpbr
*   DESCRIPTION : Check MBR & PBR for Memory Stick V1.X.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_check_mpbr(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_FORMAT_WARN         : Data format warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_check_mpbr(MSIFHNDL *msifhndl)
{
    ULONG               lba;
    SINT                result, ms_no, ret_result;
    UWORD               blk_num, blk_size;
    UBYTE               *data;
#if         (1 == MSPROAL_CHECK_MPBR)
    ULONG               part, spu, spf, hsct;
    ULONG               defpart, deflba, def_spu, def_hsct;
    UWORD               rsv_sct, rdir, bps;
    const static UBYTE  def_spf[6] = {0x02, 0x03, 0x03, 0x06, 0x0C, 0x1F};
    const static UBYTE  mbr_part1[6][15] = {
        /* 4MB      */
        {   0x01, 0x0C, 0x00, 0x01, 0x01, 0x10, 0xF5,
            0x1B, 0x00, 0x00, 0x00, 0xA5, 0x1E, 0x00, 0x00},
        /* 8MB      */
        {   0x01, 0x0A, 0x00, 0x01, 0x01, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0xA7, 0x3D, 0x00, 0x00},
        /* 16MB     */
        {   0x01, 0x0A, 0x00, 0x01, 0x03, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0x67, 0x7B, 0x00, 0x00},
        /* 32MB     */
        {   0x01, 0x04, 0x00, 0x01, 0x03, 0xD0, 0xDD,
            0x13, 0x00, 0x00, 0x00, 0x6D, 0xF7, 0x00, 0x00},
        /* 64MB     */
        {   0x02, 0x08, 0x00, 0x01, 0x07, 0xD0, 0xDD,
            0x27, 0x00, 0x00, 0x00, 0xD9, 0xEE, 0x01, 0x00},
        /* 128MB    */
        {   0x02, 0x02, 0x00, 0x06, 0x0F, 0xD0, 0xDD,
            0x21, 0x00, 0x00, 0x00, 0xDF, 0xDD, 0x03, 0x00}
    };
#endif  /*  (1 == MSPROAL_CHECK_MPBR)   */

    data        = msifhndl->WorkArea;
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3]);
    blk_num     = MAKEWORD( msifhndl->BootData[0x1A4],
                            msifhndl->BootData[0x1A5])
                >> 9;
    /* ms_no =  0    4MB */
    /*          1    8MB */
    /*          2   16MB */
    /*          3   32MB */
    /*          4   64MB */
    /*          5  128MB */
    ms_no       = 0;
    while(blk_num >>= 1) {
        ms_no ++;
    }
    if(0x10 == blk_size) {
        ms_no ++;
    }

    result = msproal_trans_read_lba(msifhndl, 0, 1, data);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            if(MSPROAL_READ_PROTECTED_ERR != result) {
                return result;
            }
        }

        return MSPROAL_FORMAT_ERR;
    }

    ret_result = MSPROAL_OK;

    /*---- MBR? ----*/
    /* x86 default boot partition */
    /* Partition type */
    /* Signature word */
    if((0x80 == data[0x1BE])
    && (0x55 == data[0x1FE])
    && (0xAA == data[0x1FF])
    && (0x00 != data[0x1C2])
    && ((0x00 != data[0x1C6])
        || (0x00 != data[0x1C7])
        || (0x00 != data[0x1C8])
        || (0x00 != data[0x1C9]))
    && ((0x00 != data[0x1CA])
        || (0x00 != data[0x1CB])
        || (0x00 != data[0x1CC])
        || (0x00 != data[0x1CD]))) {

        lba     = MAKELONG( MAKEWORD(data[0x1C7], data[0x1C6]),
                            MAKEWORD(data[0x1C9], data[0x1C8]));

#if         (1 == MSPROAL_CHECK_MPBR)
        /* Start head No. */
        if(mbr_part1[ms_no][0] != data[0x1BF]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Start sector No. */
        if(mbr_part1[ms_no][1] != data[0x1C0]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Start cylinder No. */
        if(mbr_part1[ms_no][2] != data[0x1C1]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Partition type */
        if(mbr_part1[ms_no][3] != data[0x1C2]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* End head No. */
        if(mbr_part1[ms_no][4] != data[0x1C3]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* End sector No. */
        if(mbr_part1[ms_no][5] != data[0x1C4]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* End cylinder No. */
        if(mbr_part1[ms_no][6] != data[0x1C5]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Start Sector(LBA) */
        deflba  = MAKELONG( MAKEWORD(   mbr_part1[ms_no][8],
                                        mbr_part1[ms_no][7]),
                            MAKEWORD(   mbr_part1[ms_no][10],
                                        mbr_part1[ms_no][9]));
        if(deflba != lba) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Partition size */
        defpart = MAKELONG( MAKEWORD(   mbr_part1[ms_no][12],
                                        mbr_part1[ms_no][11]),
                            MAKEWORD(   mbr_part1[ms_no][14],
                                        mbr_part1[ms_no][13]));
        part    = MAKELONG( MAKEWORD(data[0x1CB], data[0x1CA]),
                            MAKEWORD(data[0x1CD], data[0x1CC]));
        if(defpart != part) {
            ret_result = MSPROAL_FORMAT_WARN;
        }
#endif  /*  (1 == MSPROAL_CHECK_MPBR)   */

        result = msproal_trans_read_lba(msifhndl, lba, 1, data);
        if(MSPROAL_OK != result) {
            if(MSPROAL_READ_ERR != result) {
                if(MSPROAL_READ_PROTECTED_ERR != result) {
                    return result;
                }
            }
            return MSPROAL_FORMAT_ERR;
        }
    } else {
#if         (0 == MSPROAL_CHECK_MPBR)
        lba = 0;
#else   /*  (0 == MSPROAL_CHECK_MPBR)   */
        return MSPROAL_FORMAT_ERR;
#endif  /*  (0 == MSPROAL_CHECK_MPBR)   */
    }

    /* PBR? */
    /* Check jump code and signature word */
    if(((0xE9 == data[0x00]) || (0xEB == data[0x00]))
    && (0x55 == data[0x1FE])
    && (0xAA == data[0x1FF])) {
        msifhndl->HidSct = lba;

#if         (1 == MSPROAL_CHECK_MPBR)
        /* Number of bytes per sector */
        bps = MAKEWORD(data[0x00C], data[0x00B]);
        if(512 != bps) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of sectors in a cluster */
        if((blk_size << 1) != data[0x00D]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of reserved sectors */
        rsv_sct = MAKEWORD(data[0x00F], data[0x00E]);
        if(1 != rsv_sct) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of FATs */
        if(2 != data[0x010]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of root directory entries */
        rdir = MAKEWORD(data[0x012], data[0x011]);
        if(512 != rdir) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Total sectors(< 65536) */
        spu = MAKEWORD(data[0x014], data[0x013]);
        if(4 > ms_no) {
            def_spu = MAKELONG( MAKEWORD(   mbr_part1[ms_no][12],
                                            mbr_part1[ms_no][11]),
                                MAKEWORD(   mbr_part1[ms_no][14],
                                            mbr_part1[ms_no][13]));
        } else {
            def_spu = 0;
        }
        if(def_spu != spu) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Media ID */
        if(0xF8 != data[0x015]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of sectors in a FAT */
        spf = MAKEWORD(data[0x017], data[0x016]);
        if(def_spf[ms_no] != spf) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of hidden sectors */
        def_hsct    = MAKELONG( MAKEWORD(   mbr_part1[ms_no][8],
                                            mbr_part1[ms_no][7]),
                                MAKEWORD(   mbr_part1[ms_no][10],
                                            mbr_part1[ms_no][9]));
        hsct        = MAKELONG( MAKEWORD(data[0x01D], data[0x01C]),
                                MAKEWORD(data[0x01F], data[0x01E]));
        if(def_hsct != hsct) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Total sectors(>= 65536) */
        spu = MAKELONG( MAKEWORD(data[0x021], data[0x020]),
                        MAKEWORD(data[0x023], data[0x022]));
        if(4 > ms_no) {
            def_spu = 0;
        } else {
            def_spu = MAKELONG( MAKEWORD(   mbr_part1[ms_no][12],
                                            mbr_part1[ms_no][11]),
                                MAKEWORD(   mbr_part1[ms_no][14],
                                            mbr_part1[ms_no][13]));
        }
        if(def_spu != spu) {
            ret_result = MSPROAL_FORMAT_WARN;
        }
#endif  /*  (1 == MSPROAL_CHECK_MPBR)   */

    } else {
        return MSPROAL_FORMAT_ERR;
    }

    return ret_result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_check_mpbr
*   DESCRIPTION : Check MBR & PBR for Memory Stick V1.X.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_check_mpbr(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_FORMAT_WARN         : Data format warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_check_mpbr(MSIFHNDL *msifhndl)
{
    ULONG               lba;
    ULONG               pbuf_lltbl[3];
    SINT                result, ms_no, ret_result;
    UWORD               blk_num, blk_size;
    UBYTE               *data;
#if         (1 == MSPROAL_CHECK_MPBR)
    ULONG               part, spu, spf, hsct;
    ULONG               defpart, deflba, def_spu, def_hsct;
    UWORD               rsv_sct, rdir, bps;
    const static UBYTE  def_spf[6] = {0x02, 0x03, 0x03, 0x06, 0x0C, 0x1F};
    const static UBYTE  mbr_part1[6][15] = {
        /* 4MB      */
        {   0x01, 0x0C, 0x00, 0x01, 0x01, 0x10, 0xF5,
            0x1B, 0x00, 0x00, 0x00, 0xA5, 0x1E, 0x00, 0x00},
        /* 8MB      */
        {   0x01, 0x0A, 0x00, 0x01, 0x01, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0xA7, 0x3D, 0x00, 0x00},
        /* 16MB     */
        {   0x01, 0x0A, 0x00, 0x01, 0x03, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0x67, 0x7B, 0x00, 0x00},
        /* 32MB     */
        {   0x01, 0x04, 0x00, 0x01, 0x03, 0xD0, 0xDD,
            0x13, 0x00, 0x00, 0x00, 0x6D, 0xF7, 0x00, 0x00},
        /* 64MB     */
        {   0x02, 0x08, 0x00, 0x01, 0x07, 0xD0, 0xDD,
            0x27, 0x00, 0x00, 0x00, 0xD9, 0xEE, 0x01, 0x00},
        /* 128MB    */
        {   0x02, 0x02, 0x00, 0x06, 0x0F, 0xD0, 0xDD,
            0x21, 0x00, 0x00, 0x00, 0xDF, 0xDD, 0x03, 0x00}
    };
#endif  /*  (1 == MSPROAL_CHECK_MPBR)   */

    data        = msifhndl->WorkArea;
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3]);
    blk_num     = MAKEWORD( msifhndl->BootData[0x1A4],
                            msifhndl->BootData[0x1A5])
                >> 9;
    /* ms_no =  0    4MB */
    /*          1    8MB */
    /*          2   16MB */
    /*          3   32MB */
    /*          4   64MB */
    /*          5  128MB */
    ms_no       = 0;
    while(blk_num >>= 1) {
        ms_no ++;
    }
    if(0x10 == blk_size) {
        ms_no ++;
    }

#if         (1 == MSPROAL_SUPPORT_VMEM)
    msproal_user_virt_to_bus((void *)data, pbuf_lltbl);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
    pbuf_lltbl[0]   = (ULONG)data;
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
    pbuf_lltbl[1]   = 0;
    pbuf_lltbl[2]   = (ICON_DMA_CNF_DMAEN
                    | ICON_DMA_CNF_BSZ_64
                    | (512 >> 2));
    msproal_user_invalidate_cache((void *)data, 512);
    result = msproal_trans_read_lba(msifhndl, 0, 1, (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            if(MSPROAL_READ_PROTECTED_ERR != result) {
                return result;
            }
        }

        return MSPROAL_FORMAT_ERR;
    }

    ret_result = MSPROAL_OK;

    /*---- MBR? ----*/
    /* x86 default boot partition */
    /* Partition type */
    /* Signature word */
    if((0x80 == data[0x1BE])
    && (0x55 == data[0x1FE])
    && (0xAA == data[0x1FF])
    && (0x00 != data[0x1C2])
    && ((0x00 != data[0x1C6])
        || (0x00 != data[0x1C7])
        || (0x00 != data[0x1C8])
        || (0x00 != data[0x1C9]))
    && ((0x00 != data[0x1CA])
        || (0x00 != data[0x1CB])
        || (0x00 != data[0x1CC])
        || (0x00 != data[0x1CD]))) {

        lba     = MAKELONG( MAKEWORD(data[0x1C7], data[0x1C6]),
                            MAKEWORD(data[0x1C9], data[0x1C8]));

#if         (1 == MSPROAL_CHECK_MPBR)
        /* Start head No. */
        if(mbr_part1[ms_no][0] != data[0x1BF]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Start sector No. */
        if(mbr_part1[ms_no][1] != data[0x1C0]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Start cylinder No. */
        if(mbr_part1[ms_no][2] != data[0x1C1]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Partition type */
        if(mbr_part1[ms_no][3] != data[0x1C2]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* End head No. */
        if(mbr_part1[ms_no][4] != data[0x1C3]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* End sector No. */
        if(mbr_part1[ms_no][5] != data[0x1C4]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* End cylinder No. */
        if(mbr_part1[ms_no][6] != data[0x1C5]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Start Sector(LBA) */
        deflba  = MAKELONG( MAKEWORD(   mbr_part1[ms_no][8],
                                        mbr_part1[ms_no][7]),
                            MAKEWORD(   mbr_part1[ms_no][10],
                                        mbr_part1[ms_no][9]));
        if(deflba != lba) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Partition size */
        defpart = MAKELONG( MAKEWORD(   mbr_part1[ms_no][12],
                                        mbr_part1[ms_no][11]),
                            MAKEWORD(   mbr_part1[ms_no][14],
                                        mbr_part1[ms_no][13]));
        part    = MAKELONG( MAKEWORD(data[0x1CB], data[0x1CA]),
                            MAKEWORD(data[0x1CD], data[0x1CC]));
        if(defpart != part) {
            ret_result = MSPROAL_FORMAT_WARN;
        }
#endif  /*  (1 == MSPROAL_CHECK_MPBR)   */

        msproal_user_invalidate_cache((void *)data, 512);
        result = msproal_trans_read_lba(msifhndl, lba, 1, (UBYTE *)pbuf_lltbl);
        if(MSPROAL_OK != result) {
            if(MSPROAL_READ_ERR != result) {
                if(MSPROAL_READ_PROTECTED_ERR != result) {
                    return result;
                }
            }
            return MSPROAL_FORMAT_ERR;
        }
    } else {
#if         (0 == MSPROAL_CHECK_MPBR)
        lba = 0;
#else   /*  (0 == MSPROAL_CHECK_MPBR)   */
        return MSPROAL_FORMAT_ERR;
#endif  /*  (0 == MSPROAL_CHECK_MPBR)   */
    }

    /* PBR? */
    /* Check jump code and signature word */
    if(((0xE9 == data[0x00]) || (0xEB == data[0x00]))
    && (0x55 == data[0x1FE])
    && (0xAA == data[0x1FF])) {
        msifhndl->HidSct = lba;

#if         (1 == MSPROAL_CHECK_MPBR)
        /* Number of bytes per sector */
        bps = MAKEWORD(data[0x00C], data[0x00B]);
        if(512 != bps) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of sectors in a cluster */
        if((blk_size << 1) != data[0x00D]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of reserved sectors */
        rsv_sct = MAKEWORD(data[0x00F], data[0x00E]);
        if(1 != rsv_sct) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of FATs */
        if(2 != data[0x010]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of root directory entries */
        rdir = MAKEWORD(data[0x012], data[0x011]);
        if(512 != rdir) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Total sectors(< 65536) */
        spu = MAKEWORD(data[0x014], data[0x013]);
        if(4 > ms_no) {
            def_spu = MAKELONG( MAKEWORD(   mbr_part1[ms_no][12],
                                            mbr_part1[ms_no][11]),
                                MAKEWORD(   mbr_part1[ms_no][14],
                                            mbr_part1[ms_no][13]));
        } else {
            def_spu = 0;
        }
        if(def_spu != spu) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Media ID */
        if(0xF8 != data[0x015]) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of sectors in a FAT */
        spf = MAKEWORD(data[0x017], data[0x016]);
        if(def_spf[ms_no] != spf) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Number of hidden sectors */
        def_hsct    = MAKELONG( MAKEWORD(   mbr_part1[ms_no][8],
                                            mbr_part1[ms_no][7]),
                                MAKEWORD(   mbr_part1[ms_no][10],
                                            mbr_part1[ms_no][9]));
        hsct        = MAKELONG( MAKEWORD(data[0x01D], data[0x01C]),
                                MAKEWORD(data[0x01F], data[0x01E]));
        if(def_hsct != hsct) {
            ret_result = MSPROAL_FORMAT_WARN;
        }

        /* Total sectors(>= 65536) */
        spu = MAKELONG( MAKEWORD(data[0x021], data[0x020]),
                        MAKEWORD(data[0x023], data[0x022]));
        if(4 > ms_no) {
            def_spu = 0;
        } else {
            def_spu = MAKELONG( MAKEWORD(   mbr_part1[ms_no][12],
                                            mbr_part1[ms_no][11]),
                                MAKEWORD(   mbr_part1[ms_no][14],
                                            mbr_part1[ms_no][13]));
        }
        if(def_spu != spu) {
            ret_result = MSPROAL_FORMAT_WARN;
        }
#endif  /*  (1 == MSPROAL_CHECK_MPBR)   */

    } else {
        return MSPROAL_FORMAT_ERR;
    }

    return ret_result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_complete_lptbl
*   DESCRIPTION : This function executes the following procedures per segment.
*               -Generate correspondence information between logical address
*               and physical block number
*               -Boot area protection procedure if segment number is 0
*               -Logical address confirmation procedure
*               -Update alternative block information and the number of
*               alternative blocks
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_complete_lptbl(MSIFHNDL *msifhndl, SINT start_seg,
*           SINT end_seg)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       start_seg   : Start segment number
*       end_seg     : End segment number
******************************************************************************/
SINT msproal_drv_v1_complete_lptbl(MSIFHNDL *msifhndl, SINT start_seg,
        SINT end_seg)
{
    SINT    result, seg, eseg;
    UWORD   *ftbl;
    SINT    fn;

    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;

    /* start_seg is invalid negative value ? */
    if(0 > start_seg) {
        return MSPROAL_PARAM_ERR;
    }

    /* end_seg is beyond the maximum number of segment? */
    if(eseg < end_seg) {
        return MSPROAL_PARAM_ERR;
    }

    /* start_seg is larger than end_seg ? */
    if(start_seg > end_seg) {
        return MSPROAL_PARAM_ERR;
    }

    ftbl = (UWORD *)msifhndl->DataBuf;

    /* Complete logical/physical translation table  */
    /* from start segment to end segment            */
    for(seg = start_seg; seg <= end_seg; seg ++) {
        fn = 0;
        /* Generate logical/physical translation table */
        result = msproal_drv_v1_generate_lptbl( msifhndl,
                                                seg,
                                                ftbl,
                                                &fn);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Protect boot area */
        if(0 == seg) {
            result = msproal_drv_v1_protect_bootarea(   msifhndl,
                                                        ftbl,
                                                        &fn);
            if(MSPROAL_OK != result) {
                return result;
            }
        }

        /* Execute logical address confirmation procedure */
        result = msproal_drv_v1_ladrs_confirmation( msifhndl,
                                                    seg,
                                                    ftbl,
                                                    &fn);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Update alternative block information and the number of   */
        /* alternative blocks                                       */
        result = msproal_tbl_update_freeblock(  msifhndl,
                                                seg,
                                                ftbl,
                                                fn);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_generate_lptbl
*   DESCRIPTION : Procedure to generate correspondence information between
*               logical address and physical block number.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_generate_lptbl(MSIFHNDL *msifhndl, SINT seg,
*           UWORD *ftbl, SINT *fn)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       ftbl        : Arrangement where alternative block numbers are stored
*       fn          : Variable where the number of alternative blocks is stored
******************************************************************************/
SINT msproal_drv_v1_generate_lptbl(MSIFHNDL *msifhndl, SINT seg, UWORD *ftbl,
        SINT *fn)
{
    SINT    result;
    SINT    cur_pblk, bef_pblk, spblk, epblk, eseg;
    SINT    fblk_num;
    UWORD   ladrs;
    UBYTE   extradata[4];
    UBYTE   ovflg, manaflg, pgst;

    if(0 == seg) {
        if(-1 == (spblk = msifhndl->BkBootBlk)) {
            spblk   = msifhndl->BootBlk;
        }
        spblk ++;
        epblk       = MS_BLOCKS_IN_SEG;
    } else {
        spblk       = MS_BLOCKS_IN_SEG * seg;
        epblk       = spblk + MS_BLOCKS_IN_SEG;
    }

    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;

    /* Initialize tables */
    if(MSPROAL_OK != (result = msproal_tbl_init_tbl(msifhndl, seg))) {
        return result;
    }

    pgst     = 0;
    fblk_num = *fn;
    for(cur_pblk = spblk; epblk > cur_pblk; cur_pblk ++) {
        /* Disabled block? */
        result = msproal_tbl_check_useblock(msifhndl, cur_pblk);
        if(MSPROAL_OK != result) {
            continue;
        }

        /* Read ExtraDataArea */
        result  = msproal_seq_v1_read_extradata(msifhndl,
                                                cur_pblk,
                                                0,
                                                extradata);

        ovflg   = extradata[MSPROAL_OVFLG_ADRS];
        if(MSPROAL_OK != result) {
            if((MSPROAL_FLASH_READ_ERR != result)) {
                return result;
            }

            /* BKST is 1? */
            if(ovflg & MS_OVFLG_BKST) {
                /* BKST is set to 0 */
                extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            cur_pblk,
                                                            0,
                                                            extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }
            continue;
        }

        manaflg = extradata[MSPROAL_MANAFLG_ADRS];
        pgst    = (ovflg & (MS_OVFLG_PGST0 | MS_OVFLG_PGST1)) >> 5;
        /* BKST is 0? */
        if(!(ovflg & MS_OVFLG_BKST)) {
            continue;
        }

        /* PGST is 1 or 2? */
        if(2 > (UINT)(pgst - 1)) {
            /* BKST is set to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        cur_pblk,
                                                        0,
                                                        extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
            continue;
        }

        /* Last segment ? */
        if(eseg == seg) {
            /* ATFLG is 0? */
            if(!(manaflg & MS_MANAFLG_ATFLG)) {
                result = msproal_seq_v1_erase_block(msifhndl, cur_pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        if(MSPROAL_FLASH_ERASE_ERR != result) {
                            return result;
                        }
                    }
                    continue;
                }

                /* Update alternative block information */
                *ftbl ++ = cur_pblk;
                /* Update the number of alternative blocks */
                fblk_num ++;
                continue;
            }
        }

        ladrs   = MAKEWORD( extradata[MSPROAL_LADRS1_ADRS],
                            extradata[MSPROAL_LADRS0_ADRS]);
        /* Retrieve physical block for logical address using */
        /* the Logical/Physical corresponding information.   */
        result = msproal_tbl_log_to_phy(msifhndl, seg, ladrs, &bef_pblk);
        if(MSPROAL_OK != result) {
            result = msproal_seq_v1_erase_block(msifhndl, cur_pblk);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    if(MSPROAL_FLASH_ERASE_ERR != result) {
                        return result;
                    }
                }
                continue;
            }

            /* Update alternative block information */
            *ftbl ++ = cur_pblk;
            /* Update the number of alternative blocks */
            fblk_num ++;
            continue;
        }

        /* There are two blocks with same logical address? */
        if(MSPROAL_BLOCK_NOT_EXIST == bef_pblk) {
            /* Update Logical/Physical corresponding information */
            result = msproal_tbl_update_lptbl(  msifhndl,
                                                seg,
                                                ladrs,
                                                cur_pblk);
            if(MSPROAL_OK != result) {
                return result;
            }

            continue;
        }

        /* UDST is 1? */
        if(ovflg & MS_OVFLG_UDST) {
            /* Read ExtraDataArea of the block already registered   */
            /* into the Logical/Physical corresponding information. */
            result = msproal_seq_v1_read_extradata( msifhndl,
                                                    bef_pblk,
                                                    0,
                                                    extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_READ_ERR != result) {
                    return result;
                }

                /* BKST is set to 0 */
                extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            bef_pblk,
                                                            0,
                                                            extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
                /* Update Logical/Physical corresponding information */
                result = msproal_tbl_update_lptbl(  msifhndl,
                                                    seg,
                                                    ladrs,
                                                    cur_pblk);
                if(MSPROAL_OK != result) {
                    return result;
                }
                continue;
            }

            ovflg   = extradata[MSPROAL_OVFLG_ADRS];
            /* UDST is 0? */
            if(!(ovflg & MS_OVFLG_UDST)){
                result = msproal_seq_v1_erase_block(msifhndl, cur_pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        if(MSPROAL_FLASH_ERASE_ERR != result) {
                            return result;
                        }
                    }

                    continue;
                }

                /* Update alternative block information */
                *ftbl ++ = cur_pblk;
                /* Update the number of alternative blocks */
                fblk_num ++;
                continue;
            }
        }

        /* Update Logical/Physical corresponding information */
        result = msproal_tbl_update_lptbl(  msifhndl,
                                            seg,
                                            ladrs,
                                            cur_pblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_seq_v1_erase_block(msifhndl, bef_pblk);
        if(MSPROAL_OK != result) {
            if(MSPROAL_RO_ERR != result) {
                if(MSPROAL_FLASH_ERASE_ERR != result) {
                    return result;
                }
            }
            continue;
        }

        /* Update alternative block information */
        *ftbl ++ = bef_pblk;
        /* Update the number of alternative blocks */
        fblk_num ++;
        continue;
    }

    /* Update the number of alternative blocks */
    *fn = fblk_num;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_generate_lptbl
*   DESCRIPTION : Procedure to generate correspondence information between
*               logical address and physical block number.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_generate_lptbl(MSIFHNDL *msifhndl, SINT seg,
*           UWORD *ftbl, SINT *fn)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       ftbl        : Arrangement where alternative block numbers are stored
*       fn          : Variable where the number of alternative blocks is stored
******************************************************************************/
SINT msproal_drv_v1_generate_lptbl(MSIFHNDL *msifhndl, SINT seg, UWORD *ftbl,
        SINT *fn)
{
    SINT    result;
    SINT    cur_pblk, bef_pblk, spblk, epblk, eseg;
    SINT    fblk_num;
    UWORD   ladrs;
    UBYTE   ovflg, manaflg, pgst;
    UBYTE   *extra, *extradata;

    if(0 == seg) {
        if(-1 == (spblk = msifhndl->BkBootBlk)) {
            spblk   = msifhndl->BootBlk;
        }
        spblk ++;
        epblk       = MS_BLOCKS_IN_SEG;
    } else {
        spblk       = MS_BLOCKS_IN_SEG * seg;
        epblk       = spblk + MS_BLOCKS_IN_SEG;
    }

    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;

    /* Initialize tables */
    result = msproal_tbl_init_tbl(msifhndl, seg);
    if(MSPROAL_OK != result) {
        return result;
    }

    extra = msifhndl->WorkArea;
    result = msproal_drv_v1_read_1seg_extradata(msifhndl, seg, extra);
    if(MSPROAL_OK != result) {
        return result;
    }

    pgst        = 0;
    fblk_num    = 0;

    /* Generate Logical/Physical corresponding information */
    for(cur_pblk = spblk; epblk > cur_pblk; cur_pblk ++) {
        extradata   = extra + ((cur_pblk & 0x1FF) << 2);

        /* Disabled block or late-developed defective block? */
        if((0xFF == extradata[MSPROAL_OVFLG_ADRS])
        && (0xFE == extradata[MSPROAL_MANAFLG_ADRS])
        && (0xFF == extradata[MSPROAL_LADRS1_ADRS])
        && (0xFE == extradata[MSPROAL_LADRS0_ADRS])) {
            continue;
        }

        ovflg       = extradata[MSPROAL_OVFLG_ADRS];
        manaflg     = extradata[MSPROAL_MANAFLG_ADRS];
        pgst        = (ovflg & (MS_OVFLG_PGST0 | MS_OVFLG_PGST1)) >> 5;

        /* BKST of pblk is 0? */
        if(!(ovflg & MS_OVFLG_BKST)) {
            continue;
        }

        /* PGST of cur_pblk is 1 or 2? */
        if(2 > (UINT)(pgst - 1)) {
            /* BKST of cur_pblk is set to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        cur_pblk,
                                                        0,
                                                        extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }

            continue;
        }

        /* Last segment ? */
        if(eseg == seg) {
            /* ATFLG is 0? */
            if(!(manaflg & MS_MANAFLG_ATFLG)) {
                result = msproal_seq_v1_erase_block(msifhndl, cur_pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        if(MSPROAL_FLASH_ERASE_ERR != result) {
                            return result;
                        }
                    }

                    continue;
                }

                /* Update alternative block information */
                *ftbl ++ = cur_pblk;
                /* Update the number of alternative blocks */
                fblk_num ++;
                continue;
            }
        }

        ladrs   = MAKEWORD( extradata[MSPROAL_LADRS1_ADRS],
                            extradata[MSPROAL_LADRS0_ADRS]);
        /* Retrieve physical block for logical address using        */
        /* logical/physical translation table.                      */
        result = msproal_tbl_log_to_phy(msifhndl, seg, ladrs, &bef_pblk);
        if(MSPROAL_OK != result) {
            result = msproal_seq_v1_erase_block(msifhndl, cur_pblk);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    if(MSPROAL_FLASH_ERASE_ERR != result) {
                        return result;
                    }
                }

                continue;
            }

            /* Update alternative block information */
            *ftbl ++ = cur_pblk;
            /* Update the number of alternative blocks */
            fblk_num ++;
            continue;
        }

        /* There are two blocks with same logical address? */
        if(MSPROAL_BLOCK_NOT_EXIST == bef_pblk) {
            /* Update logical/physical translation table */
            result = msproal_tbl_update_lptbl(  msifhndl,
                                                seg,
                                                ladrs,
                                                cur_pblk);
            if(MSPROAL_OK != result) {
                return result;
            }

            continue;
        }

        /* UDST is 1? */
        if(ovflg & MS_OVFLG_UDST) {
            extradata   = extra + ((bef_pblk & 0x1FF) << 2);
            ovflg       = extradata[MSPROAL_OVFLG_ADRS];

            /* UDST is 0? */
            if(!(ovflg & MS_OVFLG_UDST)){
                result = msproal_seq_v1_erase_block(msifhndl, cur_pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        if(MSPROAL_FLASH_ERASE_ERR != result) {
                            return result;
                        }
                    }

                    continue;
                }

                /* Update alternative block information */
                *ftbl ++ = cur_pblk;
                /* Update the number of alternative blocks */
                fblk_num ++;
                continue;
            }
        }

        /* Update Logical/Physical corresponding information */
        result = msproal_tbl_update_lptbl(  msifhndl,
                                            seg,
                                            ladrs,
                                            cur_pblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_seq_v1_erase_block(msifhndl, bef_pblk);
        if(MSPROAL_OK != result) {
            if(MSPROAL_RO_ERR != result) {
                if(MSPROAL_FLASH_ERASE_ERR != result) {
                    return result;
                }
            }
            continue;
        }

        /* Update alternative block information */
        *ftbl ++ = bef_pblk;
        /* Update the number of alternative blocks */
        fblk_num ++;
        continue;
    }

    /* Update the number of alternative blocks */
    *fn = fblk_num;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_get_model_name
*   DESCRIPTION : Create the Model name.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_get_model_name(MSIFHNDL *msifhndl, SBYTE *modelname)
*   RETURN
*       MSPROAL_OK                  : Normal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       modelname   : Address to area where the Model name is stored
******************************************************************************/
SINT msproal_drv_v1_get_model_name(MSIFHNDL *msifhndl, SBYTE *modelname)
{
    SINT    eseg, ms_no;
    UWORD   blk_size, blk_num;

    /* Memory Stick */
    modelname[0]    = 'M';
    modelname[1]    = 'e';
    modelname[2]    = 'm';
    modelname[3]    = 'o';
    modelname[4]    = 'r';
    modelname[5]    = 'y';
    modelname[6]    = ' ';
    modelname[7]    = 'S';
    modelname[8]    = 't';
    modelname[9]    = 'i';
    modelname[10]   = 'c';
    modelname[11]   = 'k';

    /* Media Type */
    if(msifhndl->Stick & MSPROAL_STICK_R) {
        modelname[12]   = '-';
        modelname[13]   = 'R';
        modelname[14]   = ' ';
        modelname[15]   = 'D';
        modelname[16]   = 'u';
        modelname[17]   = 'o';
        modelname[18]   = ' ';
        modelname[19]   = ' ';
    } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
        modelname[12]   = '-';
        modelname[13]   = 'R';
        modelname[14]   = 'O';
        modelname[15]   = 'M';
        modelname[16]   = ' ';
        modelname[17]   = 'D';
        modelname[18]   = 'u';
        modelname[19]   = 'o';
    } else {
        modelname[12]   = ' ';
        modelname[13]   = 'D';
        modelname[14]   = 'u';
        modelname[15]   = 'o';
        modelname[16]   = ' ';
        modelname[17]   = ' ';
        modelname[18]   = ' ';
        modelname[19]   = ' ';
    }
    modelname[20]   = ' ';
    modelname[21]   = ' ';
    modelname[22]   = ' ';
    modelname[23]   = ' ';
    modelname[24]   = ' ';

    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3]);
    blk_num     = MAKEWORD( msifhndl->BootData[0x1A4],
                            msifhndl->BootData[0x1A5]);
    eseg    = blk_num >> 9;
    ms_no   = 0;
    while(eseg >>= 1) {
        ms_no ++;
    }
    if(0x10 == blk_size) {
        ms_no ++;
    }

    /* Capacity */
    modelname[25]   = '(';
    modelname[26]   = ' ';
    switch(ms_no) {
    case 0: /*   4MB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '4';
        break;
    case 1: /*   8MB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '8';
        break;
    case 2: /*  16MB */
        modelname[27]   = ' ';
        modelname[28]   = '1';
        modelname[29]   = '6';
        break;
    case 3: /*  32MB */
        modelname[27]   = ' ';
        modelname[28]   = '3';
        modelname[29]   = '2';
        break;
    case 4: /*  64MB */
        modelname[27]   = ' ';
        modelname[28]   = '6';
        modelname[29]   = '4';
        break;
    default:/* 128MB */
        modelname[27]   = '1';
        modelname[28]   = '2';
        modelname[29]   = '8';
        break;
    }
    modelname[30]   = 'M';
    modelname[31]   = 'B';
    modelname[32]   = ')';

    /* Vender Specification */
    modelname[33]   = ' ';
    modelname[34]   = ' ';
    modelname[35]   = ' ';
    modelname[36]   = ' ';
    modelname[37]   = ' ';
    modelname[38]   = ' ';
    modelname[39]   = ' ';
    modelname[40]   = ' ';
    modelname[41]   = ' ';
    modelname[42]   = ' ';
    modelname[43]   = ' ';
    modelname[44]   = ' ';
    modelname[45]   = ' ';
    modelname[46]   = ' ';

    /* Null Code */
    modelname[47]   = 0x00;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_ladrs_confirmation
*   DESCRIPTION : Logical address confirmation procedure.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_ladrs_confirmation(MSIFHNDL *msifhndl, SINT seg,
*           UWORD *ftbl, SINT *fn)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       ftbl        : Arrangement where alternative block numbers are stored
*       fn          : Variable where the number of alternative blocks is stored
******************************************************************************/
SINT msproal_drv_v1_ladrs_confirmation(MSIFHNDL *msifhndl, SINT seg,
        UWORD *ftbl, SINT *fn)
{
    SINT    result;
    SINT    minfn, eseg, pblk, freeblk;
    SINT    fblk_num;
    UWORD   ladrs, sladrs, eladrs;
    UBYTE   extradata[4];

    fblk_num    = *fn;
    eseg        = (MAKEWORD(msifhndl->BootData[0x1A4],
                            msifhndl->BootData[0x1A5]) >> 9)
                - 1;

    if(eseg == seg) {
        minfn = 1;
    } else {
        minfn = 0;
    }

    if(0 == seg) {
        sladrs = 0;
        eladrs = MS_SEG0_LADRS_NUM;
    } else {
        sladrs = MS_SEG0_LADRS_NUM + (MS_SEGN_LADRS_NUM * (seg - 1));
        eladrs = sladrs + MS_SEGN_LADRS_NUM;
    }

    /* In case of read only Memory Stick, it is not necessary to perform    */
    /* Logical Addres Confirmation procedure                                */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_OK;
    }

    for(ladrs = sladrs; eladrs > ladrs; ladrs ++) {
        /* Retrieve physical block for logical address using    */
        /* logical/physical translation table                   */
        result = msproal_tbl_log_to_phy(msifhndl, seg, ladrs, &pblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        if(MSPROAL_BLOCK_NOT_EXIST != pblk) {
            continue;
        }

        /* Retrieve unused block */
        extradata[MSPROAL_MANAFLG_ADRS]     = 0xFF;
        extradata[MSPROAL_LADRS1_ADRS]      = HIBYTE(ladrs);
        extradata[MSPROAL_LADRS0_ADRS]      = LOBYTE(ladrs);
        while(minfn < fblk_num) {
            extradata[MSPROAL_OVFLG_ADRS]   = 0xF8;

            fblk_num --;
            freeblk = ftbl[fblk_num];
            result = msproal_seq_v1_write_extradata(msifhndl,
                                                    freeblk,
                                                    0,
                                                    extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_WRITE_ERR != result) {
                    return result;
                }

                continue;
            }

            /* Update logical/physical information table */
            pblk = freeblk;
            result = msproal_tbl_update_lptbl(  msifhndl,
                                                seg,
                                                ladrs,
                                                pblk);
            if(MSPROAL_OK != result) {
                return result;
            }
            break;
        }

        /* Update the number of alternative blocks */
        *fn = fblk_num;

        /* There is no alternative blocks which can be assigned */
        if(MSPROAL_BLOCK_NOT_EXIST == pblk) {
            return MSPROAL_OK;
        }
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_mount
*   DESCRIPTION : Mount Memory Stick V1.X.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_mount(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_FORMAT_WARN         : Data format warning
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE/MSPROAL_8PARALLEL_MODE)
******************************************************************************/

SINT msproal_drv_v1_mount(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result, eseg;

    msifhndl->IfModeMax = mode;
	
    /* Retrieve boot block */
    result = msproal_seq_v1_read_bootblock(msifhndl);
    
    if(MSPROAL_OK != result) {
        return result;
    }
	
    /* Boot block content confirmation */
    result = msproal_drv_v1_bootblock_confirmation(msifhndl);
  
    if(MSPROAL_OK != result) {
        return result;
    }
	
    /* Complete correspondence information between logical address  */
    /* and physical block and execute bootarea protection           */
    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;
    
    result = msproal_drv_v1_complete_lptbl(msifhndl, 0, eseg);
    
    if(MSPROAL_OK != result) {
        return result;
    }
    
    /* Check MBR and PBR */
    return msproal_drv_v1_check_mpbr(msifhndl);
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_protect_bootarea
*   DESCRIPTION : Boot area protection procedure
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_protect_bootarea(MSIFHNDL *msifhndl, UWORD *ftbl,
*           SINT *fn)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       ftbl        : Arrangement where alternative block numbers are stored
*       fn          : Variable where the number of alternative blocks is stored
******************************************************************************/
SINT msproal_drv_v1_protect_bootarea(MSIFHNDL *msifhndl, UWORD *ftbl, SINT *fn)
{
    SINT    result;
    SINT    btblk, bkbtblk, cur_pblk, bef_pblk, registblk;
    SINT    epage, eseg;
    SINT    fblk_num;
    UWORD   ladrs, freeblk;
    UBYTE   minfn, extradata[4], cur_ovflg, bef_ovflg, pgst;

    epage   = (MAKEWORD(msifhndl->BootData[0x1A2],
                        msifhndl->BootData[0x1A3]) << 1)
            - 1;
    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;
    if(0 == eseg) {
        minfn = 1;
    } else {
        minfn = 0;
    }
    btblk   = msifhndl->BootBlk;
    if(-1 == (bkbtblk = msifhndl->BkBootBlk)) {
        bkbtblk = btblk;
    }

    fblk_num    = *fn;
    /* Loop is executed repeatedly until User Area */
    for(cur_pblk = 0; cur_pblk < bkbtblk; cur_pblk ++) {
        /* Disabled block? */
        result = msproal_tbl_check_useblock(msifhndl, cur_pblk);
        if(MSPROAL_OK != result) {
            continue;
        }
        
        /* Boot Block? */
        if(btblk == cur_pblk) {
            continue;
        }
	
        /* Read ExtraDataArea */
        result  = msproal_seq_v1_read_extradata(msifhndl,
                                                cur_pblk,
                                                0,
                                                extradata);

        cur_ovflg   = extradata[MSPROAL_OVFLG_ADRS];
        if(MSPROAL_OK != result) {
            if((MSPROAL_FLASH_READ_ERR != result)) {
                return result;
            }

            /* BKST is 1? */
            if(cur_ovflg & MS_OVFLG_BKST) {
                /* BKST is set to 0 */
                extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            cur_pblk,
                                                            0,
                                                            extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }
            continue;
        }

        pgst    = (cur_ovflg & (MS_OVFLG_PGST0 | MS_OVFLG_PGST1)) >> 5;
        ladrs   = MAKEWORD( extradata[MSPROAL_LADRS1_ADRS],
                            extradata[MSPROAL_LADRS0_ADRS]);
        /* BKST is 0? */
        if(!(cur_ovflg & MS_OVFLG_BKST)) {
            continue;
        }

        /* PGST is 1 or 2? */
        if(2 > (UINT)(pgst - 1)) {
            /* BKST is set to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        cur_pblk,
                                                        0,
                                                        extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
            continue;
        }

        /* Retrieve physical block for logical address using */
        /* the Logical/Physical corresponding information.   */
        result  = msproal_tbl_log_to_phy(msifhndl, 0, ladrs, &bef_pblk);
        if(MSPROAL_OK != result) {
            /* BKST is set to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        cur_pblk,
                                                        0,
                                                        extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
            continue;
        }

        /* Another block was assigned to the same logical address */
        /* in the Logical/Physical corresponding information.     */
        if(MSPROAL_BLOCK_NOT_EXIST != bef_pblk) {
            /* UDST is 1? */
            if(cur_ovflg & MS_OVFLG_UDST) {
                /* BKST is set to 0 */
                extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            cur_pblk,
                                                            0,
                                                            extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
                continue;
            }

            /* Read ExtraDataArea of the block already registered   */
            /* into the Logical/Physical corresponding information. */
            result = msproal_seq_v1_read_extradata( msifhndl,
                                                    bef_pblk,
                                                    0,
                                                    extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_READ_ERR != result) {
                    return result;
                }

                /* BKST is set to 0 */
                extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            bef_pblk,
                                                            0,
                                                            extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            } else {
                bef_ovflg   = extradata[MSPROAL_OVFLG_ADRS];
                /* UDST is 0? */
                if(!(bef_ovflg & MS_OVFLG_UDST)) {
                    /* BKST is set to 0 */
                    extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
                    result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                cur_pblk,
                                                                0,
                                                                extradata);
                    if(MSPROAL_OK != result) {
                        if(MSPROAL_RO_ERR != result) {
                            return result;
                        }
                    }
                    continue;
                }

                /* Erase block */
                result = msproal_seq_v1_erase_block(msifhndl, bef_pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_FLASH_ERASE_ERR != result) {
                        if(MSPROAL_RO_ERR != result) {
                            return result;
                        }
                    }
                } else {
                    /* Update the value on alternative block information */
                    /* Update the number of alternative blocks */
                    ftbl[fblk_num ++] = bef_pblk;
                }
            }
        }

        /* UDST is 1? */
        if(cur_ovflg & MS_OVFLG_UDST) {
            /* UDST of cur_pblk is set to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_UDST));
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        cur_pblk,
                                                        0,
                                                        extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
        }

        /* Copy to User Area */
        registblk = cur_pblk;
        while(minfn < fblk_num) {
            fblk_num --;
            freeblk = ftbl[fblk_num];
            result  = msproal_seq_v1_copy_block(msifhndl,
                                                cur_pblk,
                                                freeblk,
                                                0,
                                                epage);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_READ_ERR != result) {
                    if(MSPROAL_FLASH_WRITE_ERR != result) {
                        return result;
                    }
                    continue;
                }
            }

            registblk = freeblk;
            break;
        }

        /* Update Logical/Physical corresponding information */
        result = msproal_tbl_update_lptbl(  msifhndl,
                                            0,
                                            ladrs,
                                            registblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Copy was not performed? */
        if(cur_pblk == registblk) {
            continue;
        }

        /* BKST is set to 0 */
        extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
        result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                    cur_pblk,
                                                    0,
                                                    extradata);
        if(MSPROAL_OK != result) {
            if(MSPROAL_RO_ERR != result) {
                return result;
            }
        }
    }

    /* Update the number of alternative blocks */
    *fn = fblk_num;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_read_1seg_extradata
*   DESCRIPTION : Read extradatas of all blocks in specified segment
*               except blocks in boot area and disabled blocks and
*               late-developed defective blocks.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_read_1seg_extradata(MSIFHNDL *msifhndl, SINT seg,
*           UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       extradata   : Address to area where read ExtraData is stored
******************************************************************************/
SINT msproal_drv_v1_read_1seg_extradata(MSIFHNDL *msifhndl, SINT seg,
        UBYTE *extradata)
{
    SINT    result;
    SINT    dscnt, size;
    SINT    eseg, spblk, epblk, dspblk;
    UWORD   *dstbl;

    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;

    if((UINT)seg > (UINT)eseg) {
        return MSPROAL_PARAM_ERR;
    }

    dstbl       = &msifhndl->DisBlkTbl[MS_DS_BLOCKS_IN_SEG * seg];
    dscnt       = msifhndl->DisBlkNum[seg];

    /* Calculate start block number in specified segment */
    if(0 == seg) {
        if(-1 == (spblk = msifhndl->BkBootBlk)) {
            spblk = msifhndl->BootBlk;
        }
        spblk ++;

        /* Skip disabled blocks in boot area */
        while(dscnt) {
            dspblk = (SINT)*dstbl;
            if(spblk <= dspblk) {
                break;
            }
            dscnt --;
            dstbl ++;
        }

        extradata   += (spblk << 2);
    } else {
        spblk       = seg * MS_BLOCKS_IN_SEG;
    }

    /* Repeat until extradata from whole segment is created */
    do {
        if(dscnt == 0) {
            epblk   = (seg + 1) * MS_BLOCKS_IN_SEG - 1;
            size    = epblk - spblk + 1;
        } else {
            dspblk  = (SINT)*dstbl;
            epblk   = dspblk - 1;
            size    = dspblk - spblk;
        }

        /* Fill extradata corresponding to disabled block with 0xFFFE_FFFE */
        if(0 == size) {
            *extradata ++ = 0xFF;
            *extradata ++ = 0xFE;
            *extradata ++ = 0xFF;
            *extradata ++ = 0xFE;
            spblk ++;
            dstbl ++;
            dscnt --;
        } else {
            /* Read extradatas between specified two blocks continuously */
            result  = msproal_drv_v1_read_nextradata(   msifhndl,
                                                        spblk,
                                                        epblk,
                                                        extradata);
            if(MSPROAL_OK != result) {
                return result;
            }

            spblk       += size;
            extradata   += (size << 2);
        }
    } while(spblk & 0x1FF);

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */

#if         (5 != MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_read_atrb_info
*   DESCRIPTION : Read boot block information of specified size from specified
*                logical sector number.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_read_atrb_info(MSIFHNDL *msifhndl, ULONG lsct,
*           SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lsct        : Start logical sector number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_drv_v1_read_atrb_info(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
        UBYTE *data)
{
    SINT    spage, epage, last_page, pblk, blk_size, page;
    SINT    result;
    UBYTE   stts1;

    /* Calculate logical address and start page number in a block */
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3])
                << 1;
    spage       = lsct;
    last_page   = blk_size - 1;

    /* Last page beyond the range of Start page ? */
    if(last_page < spage) {
        return MSPROAL_PARAM_ERR;
    }

    if(blk_size < (spage + size)) {
        epage = last_page;
    } else {
        epage = spage + size - 1;
    }

    pblk = msifhndl->BootBlk;
    page = spage;

    do {
        /* Setting Physical block number of the copy source, Page number    */
        /* and SINGLE access mode                                           */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & MS_STTS1_UCDT) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result) {
                    return result;
                }

                return MSPROAL_READ_ERR;
            }
        }

        /* 1 PageData(= 512 bytes) is read */
        result = msproal_tpc_read_page(msifhndl, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        data += 512;
    } while(page ++ < epage);

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_read_atrb_info
*   DESCRIPTION : Read boot block information of specified size from specified
*                logical sector number.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_read_atrb_info(MSIFHNDL *msifhndl, ULONG lsct,
*           SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lsct        : Start logical sector number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_drv_v1_read_atrb_info(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
        UBYTE *data)
{
    ULONG   *cp_lltbl;
    SINT    spage, epage, last_page, pblk, blk_size, page;
    SINT    result;
    UBYTE   stts1;

    /* Calculate logical address and start page number in a block */
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3])
                << 1;
    spage       = lsct;
    last_page   = blk_size - 1;

    /* Last page beyond the range of Start page ? */
    if(last_page < spage) {
        return MSPROAL_PARAM_ERR;
    }

    cp_lltbl    = (ULONG *)data;
#if         (1 == MSPROAL_SUPPORT_VMEM)
    msproal_user_bus_to_virt(cp_lltbl[0], (void **)&data);
#else   /*  (0 == MSPROAL_SUPPORT_VMEM) */
    data        = (UBYTE *)cp_lltbl[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
    if(blk_size < (spage + size)) {
        epage = last_page;
    } else {
        epage = spage + size - 1;
    }

    pblk = msifhndl->BootBlk;
    page = spage;

    do {
        /* Setting Physical block number of the copy source, Page number    */
        /* and SINGLE access mode                                           */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & MS_STTS1_UCDT) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result) {
                    return result;
                }

                return MSPROAL_READ_ERR;
            }
        }

        /* 1 PageData(= 512 bytes) is read */
        result = msproal_tpc_read_page(msifhndl, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        data += 512;
    } while(page ++ < epage);

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_read_nextradata
*   DESCRIPTION : Read extradatas from specified physical block(spblk)
*               to specified physical block(epblk) continuously.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_read_nextradata(MSIFHNDL *msifhndl, SINT spblk,
*           SINT epblk, UBYTE *extradata);
*   RETURN
*       MPROSAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       spblk       : Start physical block
*       epblk       : End physical block
*       extradata   : Address to area where read ExtraData is stored
******************************************************************************/
SINT msproal_drv_v1_read_nextradata(MSIFHNDL *msifhndl, SINT spblk,
        SINT epblk, UBYTE *extradata)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   pblk, size;
    UWORD   isysreg;
    UBYTE   ovflg;
    UBYTE   *dst, *src;

    if(spblk > epblk) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_DRV_V1_S_READ_NEXTRADATA_LEN;
        mcdata  = (UWORD *)msproal_mc_drv_v1_s_read_nextradata;
    } else {
        mcsize  = MSPROAL_MC_DRV_V1_P_READ_NEXTRADATA_LEN;
        mcdata  = (UWORD *)msproal_mc_drv_v1_p_read_nextradata;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                ICON_CTRL_GRPN_NOWRITE);

    /* Set DMASL to 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    size    = epblk - spblk + 1;

    /* Repeat until all extradatas are read */
    while(0 != size) {
        msproal_user_write_mem16(ICON_GEN_REG1(msifhndl->BaseAdrs), size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)spblk);
        msproal_user_write_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs), 0x4000);

        /* General Data FIFO */
        msproal_user_virt_to_bus(   (void *)ICON_GDFIFO(msifhndl->BaseAdrs),
                                    (ULONG *)&src);
        msproal_user_virt_to_bus((void *)extradata, (ULONG *)&dst);
        msproal_user_invalidate_cache((void *)extradata, size << 2);
        result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                        (void *)src,
                                        (void *)dst,
                                        size << 2,
                                        MSPROAL_SELECT_GDFIFO);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_icon_exec_mc(msifhndl, 0);
        msproal_user_end_dma();
        if(MSPROAL_OK == result) {
            return result;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG1(msifhndl->BaseAdrs), &size);
        msproal_user_read_mem16(ICON_GEN_REG2(msifhndl->BaseAdrs), &pblk);

        extradata += (pblk - spblk) << 2;

        /* Read ExtraData Register */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        extradata);
        if(MSPROAL_OK != result) {
            return result;
        }
        ovflg = extradata[MSPROAL_OVFLG_ADRS];

        /* BKST is 1? */
        if(ovflg & MS_OVFLG_BKST) {
            /* Set BKST to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result  = msproal_seq_v1_overwrite_extradata(   msifhndl,
                                                            pblk,
                                                            0,
                                                            extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
        }

        *extradata ++   = 0xFF;
        *extradata ++   = 0xFE;
        *extradata ++   = 0xFF;
        *extradata ++   = 0xFE;

        size --;
        spblk = pblk + 1;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_read_nextradata
*   DESCRIPTION : Read extradatas from specified physical block(spblk)
*               to specified physical block(epblk) continuously.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_read_nextradata(MSIFHNDL *msifhndl, SINT spblk,
*           SINT epblk, UBYTE *extradata);
*   RETURN
*       MPROSAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       spblk       : Start physical block
*       epblk       : End physical block
*       extradata   : Address to area where read ExtraData is stored
******************************************************************************/
SINT msproal_drv_v1_read_nextradata(MSIFHNDL *msifhndl, SINT spblk,
        SINT epblk, UBYTE *extradata)
{
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   pblk, size;
    UWORD   isysreg;
    UBYTE   ovflg;
    UBYTE   *dst, *src;

    if(spblk > epblk) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_DRV_V1_S_READ_NEXTRADATA_LEN;
        mcdata  = (UWORD *)msproal_mc_drv_v1_s_read_nextradata;
    } else {
        mcsize  = MSPROAL_MC_DRV_V1_P_READ_NEXTRADATA_LEN;
        mcdata  = (UWORD *)msproal_mc_drv_v1_p_read_nextradata;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                ICON_CTRL_GRPN_NOWRITE);

    /* Set DMASL to 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    size    = epblk - spblk + 1;

    /* Repeat until all extradatas are read */
    while(0 != size) {
        msproal_user_write_mem16(ICON_GEN_REG1(msifhndl->BaseAdrs), size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)spblk);
        msproal_user_write_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs), 0x4000);

        /* General Data FIFO */
        msproal_user_virt_to_bus(   (void *)ICON_GDFIFO(msifhndl->BaseAdrs),
                                    (ULONG *)&src);
        msproal_user_virt_to_bus((void *)extradata, (ULONG *)&dst);
        msproal_user_invalidate_cache((void *)extradata, size << 2);
        result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                        (void *)src,
                                        (void *)dst,
                                        size << 2,
                                        MSPROAL_SELECT_GDFIFO);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_icon_exec_mc(msifhndl, 0);
        msproal_user_end_dma();
        if(MSPROAL_OK == result) {
            return result;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG1(msifhndl->BaseAdrs), &size);
        msproal_user_read_mem16(ICON_GEN_REG2(msifhndl->BaseAdrs), &pblk);

        extradata += (pblk - spblk) << 2;

        /* Read ExtraData Register */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        extradata);
        if(MSPROAL_OK != result) {
            return result;
        }
        ovflg = extradata[MSPROAL_OVFLG_ADRS];

        /* BKST is 1? */
        if(ovflg & MS_OVFLG_BKST) {
            /* Set BKST to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result  = msproal_seq_v1_overwrite_extradata(   msifhndl,
                                                            pblk,
                                                            0,
                                                            extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
        }

        *extradata ++   = 0xFF;
        *extradata ++   = 0xFE;
        *extradata ++   = 0xFF;
        *extradata ++   = 0xFE;

        size --;
        spblk = pblk + 1;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_read_nextradata
*   DESCRIPTION : Read extradatas from specified physical block(spblk)
*               to specified physical block(epblk) continuously.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_read_nextradata(MSIFHNDL *msifhndl, SINT spblk,
*           SINT epblk, UBYTE *extradata);
*   RETURN
*       MPROSAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       spblk       : Start physical block
*       epblk       : End physical block
*       extradata   : Address to area where read ExtraData is stored
******************************************************************************/
SINT msproal_drv_v1_read_nextradata(MSIFHNDL *msifhndl, SINT spblk,
        SINT epblk, UBYTE *extradata)
{
    ULONG   imctlreg;
    ULONG   gdfifo_lltbl[3];
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   pblk, size;
    UBYTE   ovflg;

    if(spblk > epblk) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_DRV_V1_S_READ_NEXTRADATA_LEN;
        mcdata  = (UWORD *)msproal_mc_drv_v1_s_read_nextradata;
    } else {
        mcsize  = MSPROAL_MC_DRV_V1_P_READ_NEXTRADATA_LEN;
        mcdata  = (UWORD *)msproal_mc_drv_v1_p_read_nextradata;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                ICON_CTRL_GRPN_NOWRITE);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    size    = epblk - spblk + 1;

    /* Repeat until all extradatas are read */
    while(0 != size) {
        msproal_user_write_mem16(ICON_GEN_REG1(msifhndl->BaseAdrs), size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)spblk);
        msproal_user_write_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs), 0x4000);

        /* General Data FIFO */
#if         (1 == MSPROAL_SUPPORT_VMEM)
        msproal_user_virt_to_bus((void *)extradata, gdfifo_lltbl);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        gdfifo_lltbl[0] = (ULONG)extradata;
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        gdfifo_lltbl[1] = 0;
        gdfifo_lltbl[2] = (ICON_DMA_CNF_DMAEN | ICON_DMA_CNF_BSZ_4 | size);
        msproal_user_invalidate_cache((void *)extradata, size * 4);
        result = msproal_icon_start_dma(msifhndl,
                                        (void *)gdfifo_lltbl,
                                        MSPROAL_SELECT_GDFIFO);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_icon_exec_mc(msifhndl, 0);
        if(MSPROAL_OK == result) {
            return result;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG1(msifhndl->BaseAdrs), &size);
        msproal_user_read_mem16(ICON_GEN_REG2(msifhndl->BaseAdrs), &pblk);

        extradata += (pblk - spblk) << 2;

        result = msproal_icon_reset_icon(msifhndl);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        /* Read ExtraData Register */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        extradata);
        if(MSPROAL_OK != result) {
            return result;
        }
        ovflg = extradata[MSPROAL_OVFLG_ADRS];

        /* BKST is 1? */
        if(ovflg & MS_OVFLG_BKST) {
            /* Set BKST to 0 */
            extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result  = msproal_seq_v1_overwrite_extradata(   msifhndl,
                                                            pblk,
                                                            0,
                                                            extradata);
            if(MSPROAL_OK != result) {
                if(MSPROAL_RO_ERR != result) {
                    return result;
                }
            }
        }

        *extradata ++   = 0xFF;
        *extradata ++   = 0xFE;
        *extradata ++   = 0xFF;
        *extradata ++   = 0xFE;

        size --;
        spblk = pblk + 1;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_recovery
*   DESCRIPTION : Execute recovery procedure for Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_recovery(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_recovery(MSIFHNDL *msifhndl)
{
    SINT    result;

    result = msproal_msif_control_power(msifhndl, MSPROAL_POWER_OFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_msif_reset_host(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_msif_control_power(msifhndl, MSPROAL_POWER_ON);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        return MSPROAL_OK;
    }

    msproal_user_change_clock(MSPROAL_SERIAL_MODE);

    return msproal_msif_change_ifmode(msifhndl, MSPROAL_V1_4PARALLEL_MODE);
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_stop
*   DESCRIPTION : Stop the command under execution.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_stop(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_stop(MSIFHNDL *msifhndl)
{
    SINT    result;
    UBYTE   stts0;

    /* Read Status Register0 */
    result = msproal_msif_read_reg( msifhndl,
                                    MS_STATUS_REG0_ADRS,
                                    1,
                                    &stts0);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(stts0 & MS_STTS0_MB) {
        /* SET_CMD[BLOCK_END] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_END,
                                        MS_TIMEOUT_BLOCK_WRITE);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }
        }
    }

    if(stts0 & MS_STTS0_FB0) {
        /* Stop all action of Flash Memory */
        result = msproal_seq_v1_stop(msifhndl);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    /* Reset Memory Stick */
    result = msproal_seq_v1_reset(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_drv_v1_wakeup
*   DESCRIPTION : Memory Stick V1.X wakes up from sleep status.
*------------------------------------------------------------------------------
*   SINT msproal_drv_v1_wakeup(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_v1_wakeup(MSIFHNDL *msifhndl)
{
    SINT    result;
    UBYTE   data;

    /* Setting WRITE_ADRS and WRITE_SIZE */
    result = msproal_tpc_set_rw_reg_adrs(   msifhndl,
                                            MS_DEFAULT_READ_ADRS,
                                            MS_DEFAULT_READ_SIZE,
                                            MS_PAGE_ADDRESS_ADRS,
                                            1);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* BSYCNT is set to 0 because time from BSY output to RDY output    */
    /* is long at WakeUp, and Timeout is not detected                   */
    msproal_tpc_set_bsycnt(msifhndl, MSIF_SYS_BSY0);

    /* Issuing of WRITE_REG */
    data = 0x00;
    result = msproal_tpc_write_reg(msifhndl, 1, &data);

    /* Setting BSYCNT=<usual> of System Register */
    msproal_tpc_set_bsycnt(msifhndl, MSIF_SYS_BSYCNT_USUAL);

    return result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_clear_buffer
*   DESCRIPTION : Clear PageBuffer of Memory Stick.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_clear_buffer(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_seq_v1_clear_buffer(MSIFHNDL *msifhndl)
{
    SINT    result;

    /* SET_CMD[CLEAR_BUF] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_CLEAR_BUF,
                                    MS_TIMEOUT_CLEAR_BUF);
    if(MSPROAL_CMDNK_ERR != result) {
        return result;
    }

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_copy_block
*   DESCRIPTION : Copy data of the specified page in the specified physical
*               block number (rpblk) to the specified physical block
*               number(wpblk).
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_copy_block(MSIFHNDL *msifhndl, SINT rpblk, SINT wpblk,
*           SINT spage, SINT epage)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       rpblk       : Physical block number of the copy source
*       wpblk       : Physical block number of the copy destination
*       spage       : Start page number to copy
*       epage       : End page number to copy
******************************************************************************/
SINT msproal_seq_v1_copy_block(MSIFHNDL *msifhndl, SINT rpblk, SINT wpblk,
        SINT spage, SINT epage)
{
    SINT    result, result_br, page, uc_flg;
    UBYTE   extra[4], *data, stts1, ovflg;

    /* Physical block number of the copy source and Physical block number   */
    /* of the copy destination are the same ?                               */
    if(rpblk == wpblk) {
        return MSPROAL_OK;
    }

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Read Only Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    data    = msifhndl->WorkArea;
    uc_flg  = MSPROAL_FALSE;
    page    = spage;
    do {
        /* Setting Physical block number of the copy source, Page number    */
        /* and SINGLE access mode                                           */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                rpblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result_br = msproal_msif_set_cmd(   msifhndl,
                                            MS_CMD_BLOCK_READ,
                                            MS_TIMEOUT_BLOCK_READ);

        /* Read ExtraData Register */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        extra);
        if(MSPROAL_OK != result) {
            return result;
        }

        if(MSPROAL_OK != result_br) {
            if(MSPROAL_FLASH_ERR != result_br) {
                if(MSPROAL_CMDNK_ERR != result_br) {
                    return result_br;
                }

                return MSPROAL_PARAM_ERR;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                ovflg = extra[MSPROAL_OVFLG_ADRS];
                /* PGST is 3? */
                if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                    extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
                    result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                rpblk,
                                                                page,
                                                                extra);
                    if(MSPROAL_OK != result) {
                        return result;
                    }
                }
                extra[MSPROAL_OVFLG_ADRS]   &=
                    (UBYTE)(~(MS_OVFLG_PGST0 | MS_OVFLG_PGST1));
                uc_flg                      = MSPROAL_TRUE;
            }

            /* 1 PageData(= 512 bytes) is read */
            result = msproal_tpc_read_page(msifhndl, data);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* 1 PageData(= 512 bytes) is written */
            result = msproal_tpc_write_page(msifhndl, data);
            if(MSPROAL_OK != result) {
                return result;
            }
        }

        ovflg = extra[MSPROAL_OVFLG_ADRS];
        /* PGST is not 3? */
        if(!(ovflg & MS_OVFLG_PGST0) || !(ovflg & MS_OVFLG_PGST1)) {
            if(ovflg & (UBYTE)(MS_OVFLG_PGST0 | MS_OVFLG_PGST1)) {
                uc_flg  = MSPROAL_TRUE;
            }

            /* Set PGST to 0 */
            extra[MSPROAL_OVFLG_ADRS] &=
                (UBYTE)(~(MS_OVFLG_PGST0 | MS_OVFLG_PGST1));
        }

        /* Set UDST and BKST to 1 */
        extra[MSPROAL_OVFLG_ADRS] |= (UBYTE)(MS_OVFLG_UDST | MS_OVFLG_BKST);

        /* Setting Physical block number of the copy destination,           */
        /* Page number, SINGLE access mode and ExtraData(OverwriteFlag,     */
        /* ManagementFlag and LogicalAddress)                               */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                wpblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                extra,
                                                MSPROAL_WRITE);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_WRITE] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_WRITE,
                                        MS_TIMEOUT_BLOCK_WRITE);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result) {
                    return result;
                }

                return MSPROAL_PARAM_ERR;
            }

            extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        wpblk,
                                                        0,
                                                        extra);
            if(MSPROAL_OK != result) {
                return result;
            }

            return MSPROAL_FLASH_WRITE_ERR;
        }
    } while(page ++ < epage);

    if(MSPROAL_TRUE == uc_flg) {
        return MSPROAL_FLASH_READ_ERR;
    }
    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_erase_block
*   DESCRIPTION : Erases all pages in the specified physical block number.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_erase_block(MSIFHNDL *msifhndl, SINT pblk)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_ERASE_ERR     : FlashEraseError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number to be erased
******************************************************************************/
SINT msproal_seq_v1_erase_block(MSIFHNDL *msifhndl, SINT pblk)
{
    SINT    result;
    UBYTE   extradata[4];

    /* Read Only Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* Setting Physical block number, Page number(0) and SINGLE access mode */
    result = msproal_msif_set_para_extra(   msifhndl,
                                            pblk,
                                            0,
                                            MS_CMDPARA_SINGLE,
                                            0,
                                            MSPROAL_READ);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* SET_CMD[BLOCK_ERASE] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_BLOCK_ERASE,
                                    MS_TIMEOUT_BLOCK_ERASE);
    if(MSPROAL_OK != result){
        if(MSPROAL_FLASH_ERR != result ){
            if(MSPROAL_CMDNK_ERR != result) {
                return result;
            }

            return MSPROAL_PARAM_ERR;
        }

        /* BKST is set to 0 */
        extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
        result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                    pblk,
                                                    0,
                                                    extradata);
        if(MSPROAL_OK != result){
            return result;
        }

        return MSPROAL_FLASH_ERASE_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_overwrite_extradata
*   DESCRIPTION : Overwrites ExtraData(only OverwriteFlag) of the specified
*               page in the specified physical block number.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_overwrite_extradata(MSIFHNDL *msifhndl, SINT pblk,
*           SINT page, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being written
*       page        : Page number being written
*       extradata   : Address to area where written ExtraData is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*****************************************************************************/
SINT msproal_seq_v1_overwrite_extradata(MSIFHNDL *msifhndl, SINT pblk,
        SINT page, UBYTE *extradata)
{
    SINT    result;

    /* Read Only Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* Setting Physical block number, Page number, OVERWRITE access mode    */
    /* and ExtraData(OverwriteFlag, ManagementFlag and LogicalAddress)      */
    result = msproal_msif_set_para_extra(   msifhndl,
                                            pblk,
                                            page,
                                            MS_CMDPARA_OVERWRITE,
                                            extradata,
                                            MSPROAL_WRITE);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* SET_CMD[BLOCK_WRITE] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_BLOCK_WRITE,
                                    MS_TIMEOUT_BLOCK_WRITE);
    if(MSPROAL_OK != result) {
        if(MSPROAL_FLASH_ERR != result) {
            if(MSPROAL_CMDNK_ERR != result) {
                return result;
            }

            return MSPROAL_PARAM_ERR;
        }
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_block
*   DESCRIPTION : Read PageData and ExtraData from the page within the
*               specified range in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data,UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
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
SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    SINT    result, uc_flg, loop_flag, page;
    UBYTE   intreg, stts1, wextra[4], ovflg;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Set Physical block number, Start page number and BLOCK access mode */
    result = msproal_msif_set_para_extra(   msifhndl,
                                            pblk,
                                            spage,
                                            MS_CMDPARA_BLOCK,
                                            0,
                                            MSPROAL_READ);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* SET_CMD[BLOCK_READ] command transmission */
    result = msproal_tpc_set_cmd(msifhndl, MS_CMD_BLOCK_READ);
    if(MSPROAL_OK != result) {
        return result;
    }

    uc_flg          = MSPROAL_FALSE;
    page            = spage;
    loop_flag       = MSPROAL_TRUE;
    do {
        /* Wait for execution result */
        /* CMD_BLOCK_READ */
        result = msproal_msif_get_int(  msifhndl,
                                        MS_TIMEOUT_BLOCK_READ,
                                        &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_PARAM_ERR;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                if(!(intreg & MS_INT_CED)) {
                    /* SET_CMD[BLOCK_END] command transmission */
                    result = msproal_msif_set_cmd(  msifhndl,
                                                    MS_CMD_BLOCK_END,
                                                    MS_TIMEOUT_BLOCK_READ);
                    if(MSPROAL_OK != result) {
                        if(MSPROAL_FLASH_ERR != result) {
                            if(MSPROAL_CMDNK_ERR != result) {
                                return result;
                            }

                            return MSPROAL_ACCESS_ERR;
                        }
                    }
                }

                /* Read ExtraData(OverwriteFlag, ManagementFlag and  */
                /* LogicalAddress)                                   */
                result = msproal_msif_read_reg( msifhndl,
                                                MS_OVERWRITE_FLAG_ADRS,
                                                4,
                                                extradata);
                if(MSPROAL_OK != result) {
                    return result;
                }

                ovflg = extradata[MSPROAL_OVFLG_ADRS];
                /* PGST is 3? */
                if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                    wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
                    result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                pblk,
                                                                page,
                                                                wextra);
                    if(MSPROAL_OK != result) {
                        if(MSPROAL_RO_ERR != result) {
                            return result;
                        }
                    }

                }

                /* Set page status to 1 */
                extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_PGST0));
                uc_flg = MSPROAL_TRUE;

                /* 1 PageData(= 512 bytes) is read */
                result = msproal_tpc_read_page(msifhndl, data);
                if(MSPROAL_OK != result) {
                    return result;
                }

                /* offset 4 bytes */
                extradata += 4;
                /* offset 512 bytes */
                data += 512;

                /* End page number is read ? */
                if(page == epage) {
                    break;
                }

                page ++;
                /* Set page number and BLOCK access mode */
                result = msproal_msif_set_para_extra(   msifhndl,
                                                        pblk,
                                                        page,
                                                        MS_CMDPARA_BLOCK,
                                                        0,
                                                        MSPROAL_READ);
                if(MSPROAL_OK != result) {
                    return result;
                }

                /* SET_CMD[BLOCK_READ] command transmission */
                result = msproal_tpc_set_cmd(msifhndl, MS_CMD_BLOCK_READ);
                if(MSPROAL_OK != result) {
                    return result;
                }

                continue;
            }
        }

        /* Last page is read ? */
        if(intreg & MS_INT_CED) {
            loop_flag = MSPROAL_FALSE;
        } else {
            /* End page number is read ? */
            if(page == epage) {
                /* SET_CMD[BLOCK_END] command transmission */
                result = msproal_msif_set_cmd(  msifhndl,
                                                MS_CMD_BLOCK_END,
                                                MS_TIMEOUT_BLOCK_READ);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_FLASH_ERR != result) {
                        if(MSPROAL_CMDNK_ERR != result) {
                            return result;
                        }

                        return MSPROAL_ACCESS_ERR;
                    }
                }

                loop_flag = MSPROAL_FALSE;
            } else {
                page ++;
            }
        }

        /* Read ExtraData(OverwriteFlag, ManagementFlag and  */
        /* LogicalAddress)                                   */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        extradata);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* 1 PageData(= 512 bytes) is read */
        if(MSPROAL_OK != (result = msproal_tpc_read_page(msifhndl, data))) {
            return result;
        }

        /* offset 4 bytes */
        extradata += 4;
        /* offset 512 bytes */
        data += 512;
    } while(loop_flag);

    if(page != epage) {
        return MSPROAL_PARAM_ERR;
    }

    if(MSPROAL_TRUE == uc_flg) {
        return MSPROAL_FLASH_READ_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
#if         (2 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_block
*   DESCRIPTION : Read PageData and ExtraData from the page within the
*               specified range in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data,UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
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
SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   uc_flg, imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   size;
    UWORD   isysreg;
    UBYTE   *cp_extradata, wextra[4], page, ovflg;
    UBYTE   *dst, *src;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_read_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_read_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    cp_extradata    = extradata;
    size            = epage - spage + 1;
    page            = (UBYTE)spage;
    uc_flg          = 0L;

    /* Page Buffer */
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)data, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* General Data FIFO */
    msproal_user_virt_to_bus(   (void *)ICON_GDFIFO(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)extradata, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)extradata, size << 2);
    result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 2,
                                    MSPROAL_SELECT_GDFIFO);
    if(MSPROAL_OK != result) {
        return result;
    }

    do {
        msproal_user_write_mem16(   ICON_GEN_REG0(msifhndl->BaseAdrs),
                                    size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)pblk);
        msproal_user_write_mem16(   ICON_GEN_REG3(msifhndl->BaseAdrs),
                                    (UWORD)page);
        msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                    ICON_CTRL_GRPN_NOWRITE);

        /* Set DMASL to 1 */
        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
        isysreg &= ~ICON_SYS_DMASL_MASK;
        isysreg |= ICON_SYS_DMASL_PB;
        msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

        /* Set PBBC to 0 */
        msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                &imctlreg);
        imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
        msproal_user_write_mem32(   ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    imctlreg);

        result = msproal_icon_exec_mc(msifhndl, 0);
        if(MSPROAL_OK == result) {
            break;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            msproal_user_end_dma();
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), &size);

        page    = (UBYTE)(epage - size + 1);
        uc_flg  |= (0x1L << page);

        page ++;
        size --;
    } while(0 != size);

    result = msproal_icon_wait_int( msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_end_dma();

    if(0L == uc_flg) {
        return MSPROAL_OK;
    }

    /* Write 1 to PGST of the pages where UnCorrectableError occured as a   */
    /* result of BLOCK_READ CMD. And to notify this to upper level function,*/
    /* PGST in extradata read is set to 1.                                  */
    wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
    for(page = (UBYTE)spage; page <= (UBYTE)epage; page ++) {
        if(uc_flg & (0x1L << page)) {
            ovflg = cp_extradata[MSPROAL_OVFLG_ADRS];
            /* PGST is 3? */
            if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            pblk,
                                                            page,
                                                            wextra);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }

            /* Set PGST to 1 */
            cp_extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
        }

        cp_extradata += 4;
    }

    return MSPROAL_FLASH_READ_ERR;
}
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
#if         (1 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_block
*   DESCRIPTION : Read PageData and ExtraData from the page within the
*               specified range in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data,UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
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
SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   ptn;
    ULONG   uc_flg;
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   size;
    UWORD   ictrlreg, isysreg;
    UBYTE   *cp_extradata, wextra[4], page, ovflg, gfvd;
    UBYTE   *dst, *src;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_read_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_read_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    cp_extradata    = extradata;
    size            = epage - spage + 1;
    page            = (UBYTE)spage;
    uc_flg          = 0L;

    /* Page Buffer */
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)data, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    do {
        msproal_user_write_mem16(   ICON_GEN_REG0(msifhndl->BaseAdrs),
                                    size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)pblk);
        msproal_user_write_mem16(   ICON_GEN_REG3(msifhndl->BaseAdrs),
                                    (UWORD)page);
        msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                    ICON_CTRL_GRPN_NOWRITE);

        /* Set DMASL to 1 */
        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
        isysreg &= ~ICON_SYS_DMASL_MASK;
        isysreg |= ICON_SYS_DMASL_PB;
        msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

        /* Set PBBC to 0 */
        msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                &imctlreg);
        imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
        msproal_user_write_mem32(   ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    imctlreg);

        /* General Data FIFO */
        /* Set PC */
        msproal_user_write_mem16(ICON_PC_REG(msifhndl->BaseAdrs), 0);

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

        msproal_user_start_timer(MSPROAL_TIMEOUT_ICON);

        while(size) {
            msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    &imctlreg);
            gfvd = HIBYTE(HIWORD(imctlreg));
            if(4 > gfvd) {
                ptn = MSPROAL_FLG_ICON | MSPROAL_FLG_EXTRACT;
                result = msproal_user_check_flg(ptn);
                if(MSPROAL_OK == result) {
                    msproal_user_clear_flg(~MSPROAL_FLG_EXTRACT);

                    if(msifhndl->IntState & MSPROAL_FLG_EXTRACT) {
                        result = msproal_msif_reset_host(msifhndl);
                        if(MSPROAL_OK != result) {
                            return MSPROAL_SYSTEM_ERR;
                        }

                        msproal_user_end_dma();

                        return MSPROAL_EXTRACT_ERR;
                    }

                    break;
                }

                if(MSPROAL_OK != (result = msproal_user_check_timer())){
                    msproal_user_end_dma();
                    return MSPROAL_SYSTEM_ERR;
                }
            } else {
                msproal_user_read_mem32(ICON_GDFIFO(msifhndl->BaseAdrs),
                                        (ULONG *)extradata);
                extradata += 4;
                size --;
            }
        }
        msproal_user_end_timer();

        msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                &imctlreg);
        gfvd = HIBYTE(HIWORD(imctlreg));
        while(4 < gfvd) {
            msproal_user_read_mem32(ICON_GDFIFO(msifhndl->BaseAdrs),
                                    (ULONG *)extradata);
            extradata += 4;
            size --;

            msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    &imctlreg);
            gfvd = HIBYTE(HIWORD(imctlreg));
        }

        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_ICON,
                                        MSPROAL_WICON);

        if(MSPROAL_OK == result) {
            break;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            msproal_user_end_dma();
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), &size);

        page    = (UBYTE)(epage - size + 1);
        uc_flg  |= (0x1L << page);

        page ++;
        size --;
    } while(0 != size);

    result = msproal_icon_wait_int( msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_end_dma();

    if(0L == uc_flg) {
        return MSPROAL_OK;
    }

    /* Write 1 to PGST of the pages where UnCorrectableError occured as a   */
    /* result of BLOCK_READ CMD. And to notify this to upper level function,*/
    /* PGST in extradata read is set to 1.                                  */
    wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
    for(page = (UBYTE)spage; page <= (UBYTE)epage; page ++) {
        if(uc_flg & (0x1L << page)) {
            ovflg = cp_extradata[MSPROAL_OVFLG_ADRS];
            /* PGST is 3? */
            if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            pblk,
                                                            page,
                                                            wextra);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }

            /* Set PGST to 1 */
            cp_extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
        }

        cp_extradata += 4;
    }

    return MSPROAL_FLASH_READ_ERR;
}
#endif  /*  (1 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
#if         (2 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_block
*   DESCRIPTION : Read PageData and ExtraData from the page within the
*               specified range in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data,UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
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
SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   uc_flg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   size;
    UWORD   isysreg;
    UBYTE   *cp_extradata, wextra[4], page, ovflg;
    UBYTE   *dst, *src;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_read_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_read_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    cp_extradata    = extradata;
    size            = epage - spage + 1;
    page            = (UBYTE)spage;
    uc_flg          = 0L;

    /* Page Buffer */
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)data, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* General Data FIFO */
    msproal_user_virt_to_bus(   (void *)ICON_GDFIFO(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)extradata, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)extradata, size << 2);
    result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 2,
                                    MSPROAL_SELECT_GDFIFO);
    if(MSPROAL_OK != result) {
        return result;
    }

    do {
        msproal_user_write_mem16(   ICON_GEN_REG0(msifhndl->BaseAdrs),
                                    size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)pblk);
        msproal_user_write_mem16(   ICON_GEN_REG3(msifhndl->BaseAdrs),
                                    (UWORD)page);
        msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                    ICON_CTRL_GRPN_NOWRITE);

        /* Set DMASL to 1 */
        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
        isysreg |= ICON_SYS_DMASL;
        msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

        result = msproal_icon_exec_mc(msifhndl, 0);
        if(MSPROAL_OK == result) {
            break;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            msproal_user_end_dma();
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), &size);

        page    = (UBYTE)(epage - size + 1);
        uc_flg  |= (0x1L << page);

        page ++;
        size --;
    } while(0 != size);

    result = msproal_icon_wait_int( msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_end_dma();

    if(0L == uc_flg) {
        return MSPROAL_OK;
    }

    /* Write 1 to PGST of the pages where UnCorrectableError occured as a   */
    /* result of BLOCK_READ CMD. And to notify this to upper level function,*/
    /* PGST in extradata read is set to 1.                                  */
    wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
    for(page = (UBYTE)spage; page <= (UBYTE)epage; page ++) {
        if(uc_flg & (0x1L << page)) {
            ovflg = cp_extradata[MSPROAL_OVFLG_ADRS];
            /* PGST is 3? */
            if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            pblk,
                                                            page,
                                                            wextra);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }

            /* Set PGST to 1 */
            cp_extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
        }

        cp_extradata += 4;
    }

    return MSPROAL_FLASH_READ_ERR;
}
#endif  /*  (2 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
#if         (1 == MSPROAL_DMA_CHANNELS)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_block
*   DESCRIPTION : Read PageData and ExtraData from the page within the
*               specified range in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data,UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
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
SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   ptn;
    ULONG   uc_flg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   size;
    UWORD   ictrlreg, isysreg, imctlreg;
    UBYTE   *cp_extradata, wextra[4], page, ovflg, gfvd;
    UBYTE   *dst, *src;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_read_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_read_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    cp_extradata    = extradata;
    size            = epage - spage + 1;
    page            = (UBYTE)spage;
    uc_flg          = 0L;

    /* Page Buffer */
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&src);
    msproal_user_virt_to_bus((void *)data, (ULONG *)&dst);
    msproal_user_invalidate_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_DADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    do {
        msproal_user_write_mem16(   ICON_GEN_REG0(msifhndl->BaseAdrs),
                                    size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)pblk);
        msproal_user_write_mem16(   ICON_GEN_REG3(msifhndl->BaseAdrs),
                                    (UWORD)page);
        msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                    ICON_CTRL_GRPN_NOWRITE);

        /* Set DMASL to 1 */
        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
        isysreg |= ICON_SYS_DMASL;
        msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

        /* General Data FIFO */
        /* Set PC */
        msproal_user_write_mem16(ICON_PC_REG(msifhndl->BaseAdrs), 0);

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

        msproal_user_start_timer(MSPROAL_TIMEOUT_ICON);

        while(size) {
            msproal_user_read_mem16(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    &imctlreg);
            gfvd = HIBYTE(imctlreg);
            if(4 > gfvd) {
                ptn = MSPROAL_FLG_ICON | MSPROAL_FLG_EXTRACT;
                result = msproal_user_check_flg(ptn);
                if(MSPROAL_OK == result) {
                    msproal_user_clear_flg(~MSPROAL_FLG_EXTRACT);

                    if(msifhndl->IntState & MSPROAL_FLG_EXTRACT) {
                        result = msproal_msif_reset_host(msifhndl);
                        if(MSPROAL_OK != result) {
                            return MSPROAL_SYSTEM_ERR;
                        }

                        msproal_user_end_dma();

                        return MSPROAL_EXTRACT_ERR;
                    }

                    break;
                }

                if(MSPROAL_OK != (result = msproal_user_check_timer())){
                    msproal_user_end_dma();
                    return MSPROAL_SYSTEM_ERR;
                }
            } else {
                msproal_user_read_mem32(ICON_GDFIFO(msifhndl->BaseAdrs),
                                        (ULONG *)extradata);
                extradata += 4;
                size --;
            }
        }
        msproal_user_end_timer();

        msproal_user_read_mem16(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                &imctlreg);
        gfvd = HIBYTE(imctlreg);
        while(4 < gfvd) {
            msproal_user_read_mem32(ICON_GDFIFO(msifhndl->BaseAdrs),
                                    (ULONG *)extradata);
            extradata += 4;
            size --;

            msproal_user_read_mem16(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    &imctlreg);
            gfvd = HIBYTE(imctlreg);
        }

        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_ICON,
                                        MSPROAL_WICON);

        if(MSPROAL_OK == result) {
            break;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            msproal_user_end_dma();
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), &size);

        page    = (UBYTE)(epage - size + 1);
        uc_flg  |= (0x1L << page);

        page ++;
        size --;
    } while(0 != size);

    result = msproal_icon_wait_int( msifhndl,
                                    MSPROAL_TIMEOUT_DMA,
                                    MSPROAL_WDMA);
    if(MSPROAL_OK != result) {
        return result;
    }

    msproal_user_end_dma();

    if(0L == uc_flg) {
        return MSPROAL_OK;
    }

    /* Write 1 to PGST of the pages where UnCorrectableError occured as a   */
    /* result of BLOCK_READ CMD. And to notify this to upper level function,*/
    /* PGST in extradata read is set to 1.                                  */
    wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
    for(page = (UBYTE)spage; page <= (UBYTE)epage; page ++) {
        if(uc_flg & (0x1L << page)) {
            ovflg = cp_extradata[MSPROAL_OVFLG_ADRS];
            /* PGST is 3? */
            if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            pblk,
                                                            page,
                                                            wextra);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }

            /* Set PGST to 1 */
            cp_extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
        }

        cp_extradata += 4;
    }

    return MSPROAL_FLASH_READ_ERR;
}
#endif  /*  (1 == MSPROAL_DMA_CHANNELS) */
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_block
*   DESCRIPTION : Read PageData and ExtraData from the page within the
*               specified range in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data,UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError(UnCorrectable Error)
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
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
SINT msproal_seq_v1_read_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   uc_flg, imctlreg;
    ULONG   *lltbl, *cp_lltbl;
    ULONG   gdfifo_lltbl[3], pbuf_lltbl[3];
    SINT    result;
    SINT    trans_size, trans_cnt;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   size;
    UBYTE   *cp_extradata, wextra[4], page, ovflg;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_read_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_read_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    cp_lltbl        = (ULONG *)data;
    lltbl           = cp_lltbl;
    cp_extradata    = extradata;
    size            = epage - spage + 1;
    page            = (UBYTE)spage;
    uc_flg          = 0L;
    trans_size      = 0;
    trans_cnt       = 0;

    do {
        if(0 == uc_flg) {
            msproal_user_memcpy((UBYTE *)pbuf_lltbl,
                                (UBYTE *)lltbl,
                                sizeof(ULONG) * 3);
        } else {
            if(0 == trans_size) {
                msproal_user_memcpy((UBYTE *)pbuf_lltbl,
                                    (UBYTE *)lltbl,
                                    sizeof(ULONG) * 3);
            } else {
                pbuf_lltbl[0]   = *(lltbl - 3) + (trans_cnt - trans_size);
                pbuf_lltbl[1]   = *(lltbl - 2);
                pbuf_lltbl[2]   = ((*(lltbl - 1) & ICON_DMA_CNF_BSZ_MASK)
                                | ICON_DMA_CNF_DMAEN
                                | (trans_size >> 2));
            }
        }

        /* Page Buffer */
        result = msproal_icon_start_dma(msifhndl,
                                        (ULONG *)pbuf_lltbl,
                                        MSPROAL_SELECT_PBUFF);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* General Data FIFO */
#if         (1 == MSPROAL_SUPPORT_VMEM)
        msproal_user_virt_to_bus((void *)extradata, gdfifo_lltbl);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        gdfifo_lltbl[0] = (ULONG)extradata;
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        gdfifo_lltbl[1] = 0;
        gdfifo_lltbl[2] = (ICON_DMA_CNF_DMAEN | ICON_DMA_CNF_BSZ_4 | size);
        result = msproal_icon_start_dma(msifhndl,
                                        (ULONG *)gdfifo_lltbl,
                                        MSPROAL_SELECT_GDFIFO);
        if(MSPROAL_OK != result) {
            return result;
        }

        msproal_user_write_mem16(   ICON_GEN_REG0(msifhndl->BaseAdrs),
                                    size);
        msproal_user_write_mem16(   ICON_GEN_REG2(msifhndl->BaseAdrs),
                                    (UWORD)pblk);
        msproal_user_write_mem16(   ICON_GEN_REG3(msifhndl->BaseAdrs),
                                    (UWORD)page);
        msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                    ICON_CTRL_GRPN_NOWRITE);

        /* Set PBBC to 0 */
        msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                &imctlreg);
        imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
        msproal_user_write_mem32(   ICON_MEM_CTRL_REG(msifhndl->BaseAdrs),
                                    imctlreg);

        result = msproal_icon_exec_mc(msifhndl, 0);
        if(MSPROAL_OK == result) {
            break;
        }

        if(MSPROAL_MC_FLASH_READ_ERR != result) {
            msproal_icon_end_dma(msifhndl);
            return result;
        }

        msproal_user_read_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), &size);

        result = msproal_icon_reset_icon(msifhndl);
        if(MSPROAL_OK != result) {
            return MSPROAL_SYSTEM_ERR;
        }

        page        = (UBYTE)(epage - size + 1);
        uc_flg      |= (0x1L << page);

        page ++;
        extradata   += (page - spage) << 2;
        size --;
        trans_size  = (page - spage) * 512;
        lltbl       = cp_lltbl;
        while(0 < trans_size) {
            trans_cnt   = (lltbl[2] & ICON_DMA_CNF_TRCNT_MASK) * 4;
            trans_size  -= trans_cnt;
            lltbl       += 3;
        }

        trans_size = -trans_size;
    } while(0 != size);

    msproal_icon_end_dma(msifhndl);

    if(0L == uc_flg) {
        return MSPROAL_OK;
    }

    /* Write 1 to PGST of the pages where UnCorrectableError occured as a   */
    /* result of BLOCK_READ CMD. And to notify this to upper level function,*/
    /* PGST in extradata read is set to 1.                                  */
    wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
    for(page = (UBYTE)spage; page <= (UBYTE)epage; page ++) {
        if(uc_flg & (0x1L << page)) {
            ovflg = cp_extradata[MSPROAL_OVFLG_ADRS];
            /* PGST is 3? */
            if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                            pblk,
                                                            page,
                                                            wextra);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_RO_ERR != result) {
                        return result;
                    }
                }
            }

            /* Set PGST to 1 */
            cp_extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
        }

        cp_extradata += 4;
    }

    return MSPROAL_FLASH_READ_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_bootblock
*   DESCRIPTION : Read Boot Block data  Boot Block retrieval procedure.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_bootblock(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_seq_v1_read_bootblock(MSIFHNDL *msifhndl)
{
    SINT    result;
    SINT    pblk;
    UBYTE   *rdata, extradata[4];
    UBYTE   stts1, ovflg, manaflg;

    pblk                = 0;
    msifhndl->BootBlk   = -1;
    msifhndl->BkBootBlk = -1;
    rdata               = msifhndl->BootData;
    /* BootBlock retrieve the first 12 blocks of Physical Block(initial     */
    /* defect maximum blocks of segment 0 + 2)                              */
    do {
        /* Setting Physical block number of the copy source, Page number    */
        /* and SINGLE access mode                                           */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                pblk,
                                                0,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }
		
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_READ,
                                        MS_TIMEOUT_BLOCK_READ);                    
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result){
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result){
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result){
                    return result;
                }

                continue;
            }
        }

        /* Read ExtraData(OverwriteFlag, ManagementFlag and LogicalAddress) */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        extradata );
        if(MSPROAL_OK != result) {
            return result;
        }

        ovflg   = extradata[MSPROAL_OVFLG_ADRS];
        manaflg = extradata[MSPROAL_MANAFLG_ADRS];
        /* BKST is 0, SYSFLG is 1 */
        if(!(ovflg & MS_OVFLG_BKST) || (manaflg & MS_MANAFLG_SYSFLG)) {
            /* Page Buffer Clear */
            result = msproal_seq_v1_clear_buffer(msifhndl);
            if(MSPROAL_OK != result){
                return result;
            }

            continue;
        }

        /* 1 PageData(= 512 bytes) is read */
        result = msproal_tpc_read_page(msifhndl, rdata);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* BootBlock ID ? */
        if((0x00 != rdata[0]) || (0x01 != rdata[1])) {
            continue;
        }

        if(-1 == msifhndl->BootBlk) {
            msifhndl->BootBlk   = pblk;
        } else {
            msifhndl->BkBootBlk = pblk;

            return MSPROAL_OK;
        }
    } while(pblk ++ < 11);

    return (-1 == msifhndl->BootBlk) ? MSPROAL_MEDIA_ERR : MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_read_extradata
*   DESCRIPTION : Read ExtraData from the specified page in the specified
*               physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_read_extradata(MSIFHNDL *msifhndl, SINT pblk,
*           SINT page, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being read
*       page        : Page number being read
*       extradata   : Address to area where read ExtraData is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_seq_v1_read_extradata(MSIFHNDL *msifhndl, SINT pblk, SINT page,
        UBYTE *extradata)
{
    SINT    result, uc_flag;
    UBYTE   stts1;

    /* Start page number is a negative value ? */
    if(0 > page) {
        return MSPROAL_PARAM_ERR;
    }

    /* Setting Physical block number, Page number and EXTRA access mode */
    result = msproal_msif_set_para_extra(   msifhndl,
                                            pblk,
                                            page,
                                            MS_CMDPARA_EXDATA,
                                            0,
                                            MSPROAL_READ);
    if(MSPROAL_OK != result) {
        return result;
    }

    uc_flag = MSPROAL_FALSE;

    /* SET_CMD[BLOCK_READ] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_BLOCK_READ,
                                    MS_TIMEOUT_BLOCK_READ);
    if(MSPROAL_OK != result) {
        if(MSPROAL_FLASH_ERR != result) {
            if(MSPROAL_CMDNK_ERR != result) {
                return result;
            }

            return MSPROAL_PARAM_ERR;
        }

        /* Read Status Register1 */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_STATUS_REG1_ADRS,
                                        1,
                                        &stts1);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Uncorrectable error ? */
        if(stts1 & (MS_STTS1_UCEX | MS_STTS1_UCFG)) {
            uc_flag = MSPROAL_TRUE;
        }
    }

    /* Read ExtraData(OverwriteFlag, ManagementFlag and LogicalAddress) */
    result = msproal_msif_read_reg( msifhndl,
                                    MS_OVERWRITE_FLAG_ADRS,
                                    4,
                                    extradata);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MSPROAL_TRUE == uc_flag) {
        return MSPROAL_FLASH_READ_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_reset
*   DESCRIPTION : Reset Flash Memory Controller and internal register in Memory
*               Stick.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_reset(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_seq_v1_reset(MSIFHNDL *msifhndl)
{
    SINT result;

    /* SET_CMD[RESET] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_RESET,
                                    MS_TIMEOUT_MS_COMMAND);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        return result;
    }

    msproal_tpc_change_ifmode(msifhndl, MSPROAL_SERIAL_MODE);

    return msproal_msif_change_ifmode(msifhndl, MSPROAL_V1_4PARALLEL_MODE);
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_sleep
*   DESCRIPTION : Flash Memory Controller in Memory Stick is put to
*               sleep state.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_sleep(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_seq_v1_sleep(MSIFHNDL *msifhndl)
{
    SINT    result;

    /* SET_CMD[SLEEP] command transmission */
    result = msproal_msif_set_cmd(msifhndl, MS_CMD_SLEEP, MS_TIMEOUT_SLEEP);
    if(MSPROAL_CMDNK_ERR != result) {
        return result;
    }

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_stop
*   DESCRIPTION : Force to stop all action of Flash Memory.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_stop(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_seq_v1_stop(MSIFHNDL *msifhndl)
{
    SINT    result;

    /* SET_CMD[FLASH_STOP] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_FLASH_STOP,
                                    MS_TIMEOUT_FLASH_STOP);
    if(MSPROAL_CMDNK_ERR != result) {
        return result;
    }

    return MSPROAL_ACCESS_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_write_block
*   DESCRIPTION : Write PageData and ExtraData to the page within the range
*               of specified in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being written
*       spage       : Start page number being written
*       epage       : End page number being written
*       data        : Address to area where data for write is stored
*       extradata   : Address to area where extradata for write is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    SINT    result, page, loop_flag;
    UBYTE   extra[4], intreg;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Read Onyl Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* Setting Physical block number, Start page number, BLOCK access mode  */
    /* and ExtraData(OverwriteFlag, ManagementFlag and LogicalAddress)      */
    result = msproal_msif_set_para_extra(   msifhndl,
                                            pblk,
                                            spage,
                                            MS_CMDPARA_BLOCK,
                                            extradata,
                                            MSPROAL_WRITE);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* SET_CMD[BLOCK_WRITE] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_BLOCK_WRITE,
                                    MS_TIMEOUT_BLOCK_WRITE);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            return result;
        }

        return MSPROAL_PARAM_ERR;
    }

    page        = spage;
    loop_flag   = MSPROAL_TRUE;
    do {
        /* 1 PageData(= 512 bytes) is written */
        if(MSPROAL_OK != (result = msproal_tpc_write_page(msifhndl, data))) {
            return result;
        }

        /* offset 512 bytes */
        data += 512;

        /* Wait for execution result */
        /* CMD_BLOCK_WRITE */
        result = msproal_msif_get_int(  msifhndl,
                                        MS_TIMEOUT_BLOCK_WRITE,
                                        &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result){
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_PARAM_ERR;
            }

            /* BKST is set to 0 */
            extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        pblk,
                                                        0,
                                                        extra);
            if(MSPROAL_OK != result){
                return result;
            }

            /* Page Buffer Clear */
            result = msproal_seq_v1_clear_buffer(msifhndl);
            if(MSPROAL_OK != result) {
                return result;
            }

            return MSPROAL_FLASH_WRITE_ERR;
        }

        /* Last page was written ? */
        if(intreg & MS_INT_CED) {
            break;
        }

        /* End page number was written ? */
        if(page == epage) {
            /* SET_CMD[BLOCK_END] command transmission */
            result = msproal_msif_set_cmd(  msifhndl,
                                            MS_CMD_BLOCK_END,
                                            MS_TIMEOUT_BLOCK_WRITE);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            loop_flag = MSPROAL_FALSE;
        } else {
            page ++;
        }
    } while(loop_flag);

    return (page == epage) ? MSPROAL_OK : MSPROAL_PARAM_ERR;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_write_block
*   DESCRIPTION : Write PageData and ExtraData to the page within the range
*               of specified in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being written
*       spage       : Start page number being written
*       epage       : End page number being written
*       data        : Address to area where data for write is stored
*       extradata   : Address to area where extradata for write is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   ir0, ir2, ir3, ir4, ir5, ictrlreg, isysreg;
    UBYTE   *dst, *src;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Read Onyl Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_WRITE_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_write_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_WRITE_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_write_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    ir0         = (UWORD)(epage - spage + 1);
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), ir0);
    ir2         = (UWORD)pblk;
    msproal_user_write_mem16(ICON_GEN_REG2(msifhndl->BaseAdrs), ir2);
    ir3         = (UWORD)spage;
    msproal_user_write_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs), ir3);
    ir4         = MAKEWORD(extradata[0], extradata[1]);
    msproal_user_write_mem16(ICON_GEN_REG4(msifhndl->BaseAdrs), ir4);
    ir5         = MAKEWORD(extradata[2], extradata[3]);
    msproal_user_write_mem16(ICON_GEN_REG5(msifhndl->BaseAdrs), ir5);
    ictrlreg    = ICON_CTRL_GDIR_CPU_TO_MS
                | ICON_CTRL_GRPN_R0
                | ICON_CTRL_PDIR_CPU_TO_MS;
    msproal_user_write_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), ictrlreg);

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    isysreg |= ICON_SYS_DMASL_PB;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, ir0 << 9);
    result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                    (void *)src,
                                    (void *)dst,
                                    ir0 << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        msproal_user_end_dma();
        if(MSPROAL_MC_FLASH_WRITE_ERR != result) {
            return result;
        }

        return MSPROAL_FLASH_WRITE_ERR;
    }

    msproal_user_end_dma();

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)   */

#if         (4 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_write_block
*   DESCRIPTION : Write PageData and ExtraData to the page within the range
*               of specified in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being written
*       spage       : Start page number being written
*       epage       : End page number being written
*       data        : Address to area where data for write is stored
*       extradata   : Address to area where extradata for write is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   ir0, ir2, ir3, ir4, ir5, ictrlreg, isysreg;
    UBYTE   *dst, *src;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Read Onyl Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_WRITE_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_write_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_WRITE_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_write_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    ir0         = (UWORD)(epage - spage + 1);
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), ir0);
    ir2         = (UWORD)pblk;
    msproal_user_write_mem16(ICON_GEN_REG2(msifhndl->BaseAdrs), ir2);
    ir3         = (UWORD)spage;
    msproal_user_write_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs), ir3);
    ir4         = MAKEWORD(extradata[0], extradata[1]);
    msproal_user_write_mem16(ICON_GEN_REG4(msifhndl->BaseAdrs), ir4);
    ir5         = MAKEWORD(extradata[2], extradata[3]);
    msproal_user_write_mem16(ICON_GEN_REG5(msifhndl->BaseAdrs), ir5);
    ictrlreg    = ICON_CTRL_GDIR_CPU_TO_MS
                | ICON_CTRL_GRPN_R0
                | ICON_CTRL_PDIR_CPU_TO_MS;
    msproal_user_write_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), ictrlreg);

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_DMASL;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, ir0 << 9);
    result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                    (void *)src,
                                    (void *)dst,
                                    ir0 << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        msproal_user_end_dma();
        if(MSPROAL_MC_FLASH_WRITE_ERR != result) {
            return result;
        }

        return MSPROAL_FLASH_WRITE_ERR;
    }

    msproal_user_end_dma();

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (4 == MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_write_block
*   DESCRIPTION : Write PageData and ExtraData to the page within the range
*               of specified in the specified physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
*           SINT epage, UBYTE *data, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being written
*       spage       : Start page number being written
*       epage       : End page number being written
*       data        : Address to area where data for write is stored
*       extradata   : Address to area where extradata for write is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_seq_v1_write_block(MSIFHNDL *msifhndl, SINT pblk, SINT spage,
        SINT epage, UBYTE *data, UBYTE *extradata)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   ir0, ir2, ir3, ir4, ir5, ictrlreg;

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Read Onyl Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_V1_S_WRITE_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_s_write_block;
    } else {
        mcsize  = MSPROAL_MC_SEQ_V1_P_WRITE_BLOCK_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_v1_p_write_block;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    ir0         = (UWORD)(epage - spage + 1);
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), ir0);
    ir2         = (UWORD)pblk;
    msproal_user_write_mem16(ICON_GEN_REG2(msifhndl->BaseAdrs), ir2);
    ir3         = (UWORD)spage;
    msproal_user_write_mem16(ICON_GEN_REG3(msifhndl->BaseAdrs), ir3);
    ir4         = MAKEWORD(extradata[0], extradata[1]);
    msproal_user_write_mem16(ICON_GEN_REG4(msifhndl->BaseAdrs), ir4);
    ir5         = MAKEWORD(extradata[2], extradata[3]);
    msproal_user_write_mem16(ICON_GEN_REG5(msifhndl->BaseAdrs), ir5);
    ictrlreg    = ICON_CTRL_GDIR_CPU_TO_MS
                | ICON_CTRL_GRPN_R0
                | ICON_CTRL_PDIR_CPU_TO_MS;
    msproal_user_write_mem16(ICON_CTRL_REG(msifhndl->BaseAdrs), ictrlreg);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    result = msproal_icon_start_dma(msifhndl,
                                    (ULONG *)data,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        if(MSPROAL_MC_FLASH_WRITE_ERR != result) {
            return result;
        }

        return MSPROAL_FLASH_WRITE_ERR;
    }

    msproal_icon_end_dma(msifhndl);

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_seq_v1_write_extradata
*   DESCRIPTION : Write ExtraData to the specified page of the specified
*               physical block.
*------------------------------------------------------------------------------
*   SINT msproal_seq_v1_write_extradata(MSIFHNDL *msifhndl, SINT pblk,
*           SINT page, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_RO_ERR              : Read Only error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number being written
*       page        : Page number being written
*       extradata   : Address to area where extradata for write is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_seq_v1_write_extradata(MSIFHNDL *msifhndl, SINT pblk, SINT page,
        UBYTE *extradata)
{
    SINT    result;
    UBYTE   extra[4];

    /* Start page number is a negative value ? */
    if(0 > page) {
        return MSPROAL_PARAM_ERR;
    }

    /* Read Only Memory Stick ? */
    if(MSPROAL_READ_ONLY == msifhndl->Rw) {
        return MSPROAL_RO_ERR;
    }

    /* Setting Physical block number, Page number, EXTRA access mode        */
    /* and ExtraData(OverwriteFlag, ManagementFlag and LogicalAddress)      */
    result = msproal_msif_set_para_extra(   msifhndl,
                                            pblk,
                                            page,
                                            MS_CMDPARA_EXDATA,
                                            extradata,
                                            MSPROAL_WRITE);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* SET_CMD[BLOCK_WRITE] command transmission */
    result = msproal_msif_set_cmd(  msifhndl,
                                    MS_CMD_BLOCK_WRITE,
                                    MS_TIMEOUT_BLOCK_WRITE);
    if(MSPROAL_OK != result){
        if(MSPROAL_FLASH_ERR != result){
            if(MSPROAL_CMDNK_ERR != result) {
                return result;
            }

            return MSPROAL_PARAM_ERR;
        }

        /* The BKST is set to 0 */
        extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
        result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                    pblk,
                                                    0,
                                                    extra);
        if(MSPROAL_OK != result){
            return result;
        }

        return MSPROAL_FLASH_WRITE_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if  (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_tbl_check_useblock
*   DESCRIPTION : It judges whether the physical block number is the Disabled
*               block.
*------------------------------------------------------------------------------
*   SINT msproal_tbl_check_useblock(MSIFHNDL *msifhndl, SINT pblk)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_ERR                 : Abnormal
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical Block number
******************************************************************************/
SINT msproal_tbl_check_useblock(MSIFHNDL *msifhndl, SINT pblk)
{
    SINT    cnt, disblk, disblk_num;
    UBYTE   *disblk_tbl;

    disblk_num  = MAKELONG( MAKEWORD(   msifhndl->BootData[0x176],
                                        msifhndl->BootData[0x177]),
                            MAKEWORD(   msifhndl->BootData[0x174],
                                        msifhndl->BootData[0x175]));
    disblk_tbl  = &msifhndl->BootData[512];
    for(cnt = 0; cnt < disblk_num; cnt += 2) {
        disblk = MAKEWORD(disblk_tbl[cnt], disblk_tbl[cnt + 1]);
        if(disblk == pblk) {
            return MSPROAL_ERR;
        }
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */

#if  (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_tbl_get_freeblock
*   DESCRIPTION : Get alternative (available) block from segment number and
*                 physical block number.
*------------------------------------------------------------------------------
*   SINT msproal_tbl_get_freeblock(MSIFHNDL *msifhndl, SINT *pblk, SINT seg)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_WRITE_ERR           : Write error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       pblk        : Physical block number
*       seg         : Segment number
******************************************************************************/
SINT msproal_tbl_get_freeblock(MSIFHNDL *msifhndl, SINT *pblk, SINT seg)
{
    SINT    eseg, fn;
    UWORD   *ftbl;
    UBYTE   minfn;

    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;

    if((UINT)seg > (UINT)eseg) {
        return MSPROAL_PARAM_ERR;
    }

    if(seg == eseg) {
        minfn = 1;
    } else {
        minfn = 0;
    }

    fn      = msifhndl->FreeBlkNum[seg];
    ftbl    = &msifhndl->FreeBlkTbl[seg << 4];
    if(minfn >= fn) {
        msifhndl->Rw = MSPROAL_READ_ONLY;
        return MSPROAL_WRITE_ERR;
    }

    *pblk = ftbl[0];
    fn --;

    /* Update the value on alternative block information */
    msproal_user_memcpy((UBYTE *)ftbl, (UBYTE *)(ftbl + 1), fn << 1);
    ftbl[fn]                    = 0xFFFF;
    /* Update the value on table of the number of alternative blocks */
    msifhndl->FreeBlkNum[seg]   = fn;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */

#if  (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_tbl_init_tbl
*   DESCRIPTION : Initialize LoghPhyTable, FreeBlkTbl and FreeBlkNum of
*               specified segment
*------------------------------------------------------------------------------
*   SINT msproal_tbl_init_tbl(MSIFHNDL *msifhndl, SINT seg)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number.
******************************************************************************/
SINT msproal_tbl_init_tbl(MSIFHNDL *msifhndl, SINT seg)
{
    SINT    sseg, eseg, size;
    UWORD   sladrs;

    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;
    /* Check the range of segment number */
    if((UINT)seg > (UINT)eseg) {
        return MSPROAL_PARAM_ERR;
    }

    /* Initialize logical/physical translation table of specified segment */
    if(0 == seg){
        sladrs  = 0;
        size    = MS_SEG0_LADRS_NUM;
    } else {
        sladrs  = MS_SEG0_LADRS_NUM + MS_SEGN_LADRS_NUM * (seg - 1);
        size    = MS_SEGN_LADRS_NUM;
    }
    msproal_user_memset((UBYTE *)&msifhndl->LogiPhyTbl[sladrs],
                        0xff,
                        (size * sizeof(UWORD)));

    /* Initialize free block table of specified segment */
    sseg = 16 * seg;
    msproal_user_memset((UBYTE *)&msifhndl->FreeBlkTbl[sseg],
                        0xff,
                        (16 * sizeof(UWORD)));

    /* Initialize free block number of specified segment */
    msifhndl->FreeBlkNum[seg] = 0;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */

#if  (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_tbl_log_to_phy
*   DESCRIPTION : Return physical block number from logical address.
*------------------------------------------------------------------------------
*   SINT msproal_tbl_log_to_phy(MSIFHNDL *msifhndl, SINT seg, UWORD ladrs,
*           SINT *pblk)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       ladrs       : Logical address
*       pblk        : Physical block number
******************************************************************************/
SINT msproal_tbl_log_to_phy(MSIFHNDL *msifhndl, SINT seg, UWORD ladrs,
        SINT *pblk)
{
    SINT    eseg, ladrs_size;
    UWORD   sladrs;

    /* Check the range of segment number */
    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;
    if((UINT)seg > (UINT)eseg){
        return MSPROAL_PARAM_ERR;
    }

    /* Logical address is within the limits of  */
    /* logical address of the segment ?         */
    if(0 == seg) {
        sladrs      = 0;
        ladrs_size  = MS_SEG0_LADRS_NUM - 1;
    } else {
        sladrs      = MS_SEG0_LADRS_NUM + (MS_SEGN_LADRS_NUM * (seg - 1));
        ladrs_size  = MS_SEGN_LADRS_NUM - 1;
    }
    if((UWORD)ladrs_size < (UWORD)(ladrs - sladrs)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Return physical block number from logical address */
    *pblk = msifhndl->LogiPhyTbl[ladrs];

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */

#if  (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_tbl_update_lptbl
*   DESCRIPTION : Register physical block number into logical/physical
*               translation table
*------------------------------------------------------------------------------
*   SINT msproal_tbl_update_lptbl(MSIFHNDL *msifhndl, SINT seg, UWORD ladrs,
*           SINT pblk);
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       ladrs       : Logical Address
*       pblk        : Physical block number
******************************************************************************/
SINT msproal_tbl_update_lptbl(MSIFHNDL *msifhndl, SINT seg, UWORD ladrs,
        SINT pblk)
{
    SINT    spblk, eseg, ladrs_size;
    UWORD   sladrs;

    /* Check the range of segment number */
    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;
    if((UINT)seg > (UINT)eseg){
        return MSPROAL_PARAM_ERR;
    }

    /* Logical address is within the limits of  */
    /* logical address of the segment ?         */
    if(0 == seg) {
        sladrs      = 0;
        ladrs_size  = MS_SEG0_LADRS_NUM - 1;
    } else {
        sladrs      = MS_SEG0_LADRS_NUM + MS_SEGN_LADRS_NUM * (seg - 1);
        ladrs_size  = MS_SEGN_LADRS_NUM - 1;
    }
    if((UWORD)ladrs_size < (UWORD)(ladrs - sladrs)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Physical block number is within the limits of    */
    /* physical block number of the segment ?           */
    spblk = seg * MS_BLOCKS_IN_SEG;
    if((UINT)(MS_BLOCKS_IN_SEG - 1) < (UINT)(pblk - spblk)) {
        return MSPROAL_PARAM_ERR;
    }

    /* Register pblk to logical/physical translation table */
    msifhndl->LogiPhyTbl[ladrs] = pblk;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */

#if  (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_tbl_update_freeblock
*   DESCRIPTION : Update alternative block information and the number of
*               alternative blocks
*------------------------------------------------------------------------------
*   SINT msproal_tbl_update_freeblock(MSIFHNDL *msifhndl, SINT seg,
            UWORD *ftbl, SINT fn);
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       seg         : Segment number
*       ftbl        : Arrangement where alternative block numbers are stored
*       fn          : The number of alternative blocks to be added
******************************************************************************/
SINT msproal_tbl_update_freeblock(MSIFHNDL *msifhndl, SINT seg, UWORD *ftbl,
        SINT fn)
{
    SINT    eseg;
    SINT    fblk_num;
    UWORD   *fblk_tbl;
    UBYTE   minfn;

    /* Check the range of segment number */
    eseg    = (MAKEWORD(msifhndl->BootData[0x1A4],
                        msifhndl->BootData[0x1A5]) >> 9)
            - 1;
    if((UINT)seg > (UINT)eseg) {
        return MSPROAL_PARAM_ERR;
    }

    fblk_num = msifhndl->FreeBlkNum[seg];
    /* fn is beyond the number which can be added to alternative block  */
    /* information table ?                                              */
    if(fn > (16 - fblk_num)) {
        return MSPROAL_PARAM_ERR;
    }

    if(eseg == seg) {
        minfn = 1;
    } else {
        minfn = 0;
    }
    if((fblk_num + fn) <= minfn) {
        msifhndl->Rw = MSPROAL_READ_ONLY;
        return MSPROAL_OK;
    }

    /* Register alternative blocks to alternative block information table */
    fblk_tbl = &msifhndl->FreeBlkTbl[seg * 16 + fblk_num];
    msproal_user_memcpy((UBYTE *)fblk_tbl, (UBYTE *)ftbl, fn * sizeof(UWORD));

    /* Update the number of alternative blocks */
    msifhndl->FreeBlkNum[seg] = fblk_num + fn;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)  */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_copy_block
*   DESCRIPTION : Copy data of the specified page in the specified physical
*               block number (rpblk) to the specified physical block
*               number(wpblk).
*------------------------------------------------------------------------------
*   SINT msproal_trans_copy_block(MSIFHNDL *msifhndl, SINT rpblk, SINT wpblk,
*           SINT spage, SINT epage, UBYTE *extradata)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read Protected error
*       MSPROAL_EXTRACT_ERR         : Media extract
*       MSPROAL_FLASH_READ_ERR      : FlashReadError
*       MSPROAL_FLASH_WRITE_ERR     : FlashWriteError
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       rpblk       : Physical block number of the copy source
*       wpblk       : Physical block number of the copy destination
*       spage       : Start page number to copy
*       epage       : End page number to copy
*       extradata   : Address to area where ExtraData written in copy
*                   destination is stored
*                       [supplement] data alignment of extradata
*                       Offset0 : OverwriteFlag
*                       Offset1 : ManagementFlag
*                       Offset2 : LogicalAddress
*                       Offset3 :   ditto
******************************************************************************/
SINT msproal_trans_copy_block(MSIFHNDL *msifhndl, SINT rpblk, SINT wpblk,
        SINT spage, SINT epage, UBYTE *extradata)
{
    SINT    result, result_br, page, ps, uc_flg;
    UBYTE   rextra[4], wextra[4], *data, stts1, ovflg, manaflg;

    /* Physical block number of the copy source and Physical block number   */
    /* of the copy destination are the same ?                               */
    if(rpblk == wpblk) {
        return MSPROAL_OK;
    }

    /* Start page number is a negative value ? */
    if(0 > spage) {
        return MSPROAL_PARAM_ERR;
    }

    /* Start page number is larger than End page number ? */
    if(spage > epage) {
        return MSPROAL_PARAM_ERR;
    }

    wextra[0] = 0xF8;
    wextra[1] = 0xFF;
    wextra[2] = extradata[2];
    wextra[3] = extradata[3];

    data    = &msifhndl->DataBuf[512];
    uc_flg  = MSPROAL_FALSE;
    page    = spage;
    do {
        ps = 3;

        /* Setting Physical block number of the copy source, Page number    */
        /* and SINGLE access mode                                           */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                rpblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                0,
                                                MSPROAL_READ);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_READ] command transmission */
        result_br = msproal_msif_set_cmd(   msifhndl,
                                            MS_CMD_BLOCK_READ,
                                            MS_TIMEOUT_BLOCK_READ);
		
        /* Read ExtraData Register */
        result = msproal_msif_read_reg( msifhndl,
                                        MS_OVERWRITE_FLAG_ADRS,
                                        4,
                                        rextra);
        if(MSPROAL_OK != result) {
            return result;
        }

        if(MSPROAL_OK != result_br) {
            if(MSPROAL_FLASH_ERR != result_br) {
                if(MSPROAL_CMDNK_ERR != result_br) {
                    return result_br;
                }

                return MSPROAL_PARAM_ERR;
            }

            /* Read Status Register1 */
            result = msproal_msif_read_reg( msifhndl,
                                            MS_STATUS_REG1_ADRS,
                                            1,
                                            &stts1);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Uncorrectable error ? */
            if(stts1 & (MS_STTS1_UCDT | MS_STTS1_UCEX | MS_STTS1_UCFG)) {
                ovflg = rextra[MSPROAL_OVFLG_ADRS];
                /* PGST is 3? */
                if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                    rextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
                    result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                rpblk,
                                                                page,
                                                                rextra);
                                                                                  
                    if(MSPROAL_OK != result) {
                        return result;
                    }
                }

                ps      = 0;
                uc_flg  = MSPROAL_TRUE;
            }

            /* 1 PageData(= 512 bytes) is read */
            result = msproal_tpc_read_page(msifhndl, data);
            if(MSPROAL_OK != result) {
                return result;
            }
			
            /* 1 PageData(= 512 bytes) is written */
            result = msproal_tpc_write_page(msifhndl, data);
            if(MSPROAL_OK != result) {
                return result;
            }
        }

        ovflg = rextra[MSPROAL_OVFLG_ADRS];
        /* PGST is not 3? */
        if(!(ovflg & MS_OVFLG_PGST0) || !(ovflg & MS_OVFLG_PGST1)) {
            if(ovflg & (UBYTE)(MS_OVFLG_PGST0 | MS_OVFLG_PGST1)) {
                uc_flg  = MSPROAL_TRUE;
            }

            ps = 0;
        }

        if(ps) {
            wextra[MSPROAL_OVFLG_ADRS] |= (MS_OVFLG_PGST0 | MS_OVFLG_PGST1);
        } else {
            wextra[MSPROAL_OVFLG_ADRS] &=
                (UBYTE)(~(MS_OVFLG_PGST0 | MS_OVFLG_PGST1));
        }

        manaflg = rextra[MSPROAL_MANAFLG_ADRS];
        if(!(manaflg & MS_MANAFLG_SCMS0) || !(manaflg & MS_MANAFLG_SCMS1)) {
            /* Page Buffer Clear */
            result = msproal_seq_v1_clear_buffer(msifhndl);
            if(MSPROAL_OK != result) {
                return result;
            }

            return MSPROAL_READ_PROTECTED_ERR;
        }

        /* Setting Physical block number of the copy destination,       */
        /* Page number, SINGLE access mode and ExtraData(OverwriteFlag, */
        /* ManagementFlag and LogicalAddress)                           */
        result = msproal_msif_set_para_extra(   msifhndl,
                                                wpblk,
                                                page,
                                                MS_CMDPARA_SINGLE,
                                                wextra,
                                                MSPROAL_WRITE);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* SET_CMD[BLOCK_WRITE] command transmission */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS_CMD_BLOCK_WRITE,
                                        MS_TIMEOUT_BLOCK_WRITE);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                /* Page Buffer Clear */
                result = msproal_seq_v1_clear_buffer(msifhndl);
                if(MSPROAL_OK != result) {
                    return result;
                }

                return MSPROAL_PARAM_ERR;
            }

            wextra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_BKST));
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        wpblk,
                                                        0,
                                                        wextra);
            if(MSPROAL_OK != result) {
                return result;
            }

            return MSPROAL_FLASH_WRITE_ERR;
        }
    } while(page ++ < epage);

    if(MSPROAL_TRUE == uc_flg) {
        return MSPROAL_FLASH_READ_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (5 != MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_format
*   DESCRIPTION : Format function for Memory Stick V1.X.
*------------------------------------------------------------------------------
*   SINT msproal_trans_format(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Mode(MSPROAL_QUICK_FORMAT/MSPROAL_FULL_FORMAT)
******************************************************************************/
SINT msproal_trans_format(MSIFHNDL *msifhndl, SINT mode)
{
    ULONG               hid_sct, sct_num;
    ULONG               all_data, proc_data;
    SINT                result, cnt, sct, ms_no;
    SINT                pblk, seg, eseg, fn;
    UWORD               ftbl[16];
    UWORD               spf, fats, ladrs, eladrs, blk_num, blk_size, year;
    SBYTE               *fstype;
    UBYTE               *data, *prog, extradata[4], ovflg;
    const static UBYTE  def_spf[6] = {0x02, 0x03, 0x03, 0x06, 0x0C, 0x1F};
    const static UBYTE  hps[6] = {0x02, 0x02, 0x04, 0x04, 0x08, 0x10};
    const static UBYTE  mbr_part1[6][16] = {
        /* 4MB */
        {   0x80, 0x01, 0x0C, 0x00, 0x01, 0x01, 0x10, 0xF5,
            0x1B, 0x00, 0x00, 0x00, 0xA5, 0x1E, 0x00, 0x00},
        /* 8MB */
        {   0x80, 0x01, 0x0A, 0x00, 0x01, 0x01, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0xA7, 0x3D, 0x00, 0x00},
        /* 16MB */
        {   0x80, 0x01, 0x0A, 0x00, 0x01, 0x03, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0x67, 0x7B, 0x00, 0x00},
        /* 32MB */
        {   0x80, 0x01, 0x04, 0x00, 0x01, 0x03, 0xD0, 0xDD,
            0x13, 0x00, 0x00, 0x00, 0x6D, 0xF7, 0x00, 0x00},
        /* 64MB */
        {   0x80, 0x02, 0x08, 0x00, 0x01, 0x07, 0xD0, 0xDD,
            0x27, 0x00, 0x00, 0x00, 0xD9, 0xEE, 0x01, 0x00},
        /* 128MB */
        {   0x80, 0x02, 0x02, 0x00, 0x06, 0x0F, 0xD0, 0xDD,
            0x21, 0x00, 0x00, 0x00, 0xDF, 0xDD, 0x03, 0x00}
    };

    blk_num     = MAKEWORD( msifhndl->BootData[0x1A4],
                            msifhndl->BootData[0x1A5]);
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3]);
    eseg        = blk_num >> 9;
    all_data    = 1;
    proc_data   = 0;
    prog        = msifhndl->WorkArea;
    prog[0]     = HIBYTE(HIWORD(all_data));
    prog[1]     = LOBYTE(HIWORD(all_data));
    prog[2]     = HIBYTE(LOWORD(all_data));
    prog[3]     = LOBYTE(LOWORD(all_data));
    prog[4]     = HIBYTE(HIWORD(proc_data));
    prog[5]     = LOBYTE(HIWORD(proc_data));
    prog[6]     = HIBYTE(LOWORD(proc_data));
    prog[7]     = LOBYTE(LOWORD(proc_data));

    /* Full? */
    if(MSPROAL_FULL_FORMAT == mode) {
        all_data    += eseg;
        prog[3]     = LOBYTE(LOWORD(all_data));

        if(-1 == (pblk = msifhndl->BkBootBlk)) {
            pblk = msifhndl->BootBlk;
        }
        pblk ++;
        ladrs                           = 0;
        eladrs                          = MS_SEG0_LADRS_NUM - 1;
        extradata[MSPROAL_OVFLG_ADRS]   = 0xF8;
        extradata[MSPROAL_MANAFLG_ADRS] = 0xFF;
        for(seg = 0; eseg > seg; seg ++) {
            fn = 0;
            msproal_user_memset((UBYTE *)ftbl, 0xff, sizeof(ftbl));
            /* Initialize tables */
            result = msproal_tbl_init_tbl(msifhndl, seg);
            if(MSPROAL_OK != result) {
                return result;
            }

            do {
                /* Disabled block ? */
                result = msproal_tbl_check_useblock(msifhndl, pblk);
                if(MSPROAL_OK != result) {
                    continue;
                }

                /* Read extradata of page 0 in block pblk */
                result = msproal_seq_v1_read_extradata( msifhndl,
                                                        pblk,
                                                        0,
                                                        extradata);

                ovflg = extradata[MSPROAL_OVFLG_ADRS];

                if(MSPROAL_OK != result) {
                    if((MSPROAL_FLASH_READ_ERR != result)) {
                        return result;
                    }

                    /* BKST is 1? */
                    if(ovflg & MS_OVFLG_BKST) {
                        /* BKST is set to 0 */
                        extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
                        result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                    pblk,
                                                                    0,
                                                                    extradata);
                        if(MSPROAL_OK != result) {
                            return result;
                        }
                    }

                    continue;
                }

                /* BKST is 0? */
                if(!(ovflg & MS_OVFLG_BKST)) {
                    continue;
                }

                /* PGST is 1 ? */
                if(!(ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                    /* BKST of pblk is set to 0 */
                    extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
                    result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                pblk,
                                                                0,
                                                                extradata);
                    if(MSPROAL_OK != result) {
                        return result;
                    }

                    continue;
                }

                /* Erase block pblk */
                result = msproal_seq_v1_erase_block(msifhndl, pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_FLASH_ERASE_ERR != result) {
                        return result;
                    }

                    continue;
                }

                /* Logical address is beyond the defined segment number */
                if(eladrs < ladrs) {
                    ftbl[fn ++] = pblk;
                    continue;
                }

                extradata[MSPROAL_LADRS1_ADRS] = HIBYTE(ladrs);
                extradata[MSPROAL_LADRS0_ADRS] = LOBYTE(ladrs);
                result = msproal_seq_v1_write_extradata(msifhndl,
                                                        pblk,
                                                        0,
                                                        extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_FLASH_WRITE_ERR != result) {
                        return result;
                    }

                    continue;
                }

                /* Update logical/physical translation table */
                result = msproal_tbl_update_lptbl(  msifhndl,
                                                    seg,
                                                    ladrs,
                                                    pblk);
                if(MSPROAL_OK != result) {
                    return result;
                }

                ladrs ++;
            } while((pblk += 1) & 0x1FF);

            /* Update alternative block information and the number of   */
            /* alternative blocks                                       */
            result = msproal_tbl_update_freeblock(  msifhndl,
                                                    seg,
                                                    ftbl,
                                                    fn);
            if(MSPROAL_OK != result) {
                return result;
            }

            if(MSPROAL_READ_ONLY == msifhndl->Rw) {
                return MSPROAL_WRITE_ERR;
            }

            eladrs      += MS_SEGN_LADRS_NUM;
            proc_data ++;
            prog[7]     = LOBYTE(LOWORD(proc_data));
        }
    }

    /*---------     resetting MBR           -----------------------------*/
    /* ms_no =  0   4MB     */
    /*          1   8MB     */
    /*          2   16MB    */
    /*          3   32MB    */
    /*          4   64MB    */
    /*          5   128MB   */
    ms_no = 0;
    while(eseg >>= 1) {
        ms_no ++;
    }
    if(0x10 == blk_size) {
        ms_no ++;
    }

    data        = &msifhndl->WorkArea[512];
    msproal_user_memset(data, 0x00, 512);
    /* Set partition 1 */
    for(cnt = 0; cnt < 16; cnt ++) {
        data[0x1BE + cnt] = mbr_part1[ms_no][cnt];
    }
    /* Signature Wrod */
    data[0x1FE] = 0x55;
    data[0x1FF] = 0xAA;

    result = msproal_trans_write_lba(msifhndl, 0, 1, data);
    if(MSPROAL_OK != result) {
        return result;
    }

    /*---------     resetting PBR           -----------------------------*/
    msproal_user_memset(data, 0x00, 512);
    /* Jump code = 0xE90000 */
    data[0x000] = 0xE9;
    /* OEM name and version = "        " */
    msproal_user_memset(&data[0x003], ' ', 8);
    /* Number of bytes per sector = 512(0x0200) */
    data[0x00C] = 0x02;
    /* Number of sectors in a cluster */
    data[0x00D] = LOBYTE(blk_size << 1);
    /* Number of reserved sectors = 0x01 */
    data[0x00E] = 0x01;
    /* Number of FATs = 0x02 */
    data[0x010] = 0x02;
    /* Number of root directory entries = 0x0200 */
    data[0x012] = 0x02;
    /* Total sectors */
    /* Is the total sectors less than 65536 ? */
    if(0x00 == mbr_part1[ms_no][14]) {
        data[0x013] = mbr_part1[ms_no][12];
        data[0x014] = mbr_part1[ms_no][13];
    } else {
        data[0x020] = mbr_part1[ms_no][12];
        data[0x021] = mbr_part1[ms_no][13];
        data[0x022] = mbr_part1[ms_no][14];
        data[0x023] = mbr_part1[ms_no][15];
    }
    /* Media ID = 0xF8 */
    data[0x015] = 0xF8;
    /* Number of sectors in a FAT */
    data[0x016] = def_spf[ms_no];
    /* Number of sectors on a head(track) = 0x0010 */
    data[0x018] = 0x10;
    /* Number of heads */
    data[0x01A] = hps[ms_no];
    /* Number of hidden sectors */
    data[0x01C] = mbr_part1[ms_no][8];
    data[0x01D] = mbr_part1[ms_no][9];
    data[0x01E] = mbr_part1[ms_no][10];
    data[0x01F] = mbr_part1[ms_no][11];
    /* Extension boot signature = 0x29 */
    data[0x026] = 0x29;
    /* File system type */
    if(4096 < blk_num) {
        fstype = "FAT16   ";
    } else {
        fstype = "FAT12   ";
    }
    msproal_user_memcpy(&data[0x036], (UBYTE *)fstype, 8);
    /* Signature word */
    data[0x1FE] = 0x55;
    data[0x1FF] = 0xAA;

    hid_sct = MAKELONG( MAKEWORD(   mbr_part1[ms_no][9],
                                    mbr_part1[ms_no][8]),
                        MAKEWORD(   mbr_part1[ms_no][11],
                                    mbr_part1[ms_no][10]));
    result = msproal_trans_write_lba(msifhndl, hid_sct, 1, data);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl->HidSct    = hid_sct;
    spf                 = MAKEWORD(data[0x17], data[0x16]);
    fats                = data[0x10];

    /*---------     resetting FAT           ---------------------------------*/
    msproal_user_memset(data, 0x00, 512);
    data[0x0] = 0xF8;
    data[0x1] = 0xFF;
    data[0x2] = 0xFF;
    /* FAT16 ? */
    if(4096 < blk_num) {
        data[3] = 0xFF;
    }
    /* Start LBA sector of first block */
    /* Hidden sector + PBR */
    sct_num = hid_sct + 1;
    for(cnt = 0; cnt < fats; cnt ++) {
        result = msproal_trans_write_lba(   msifhndl,
                                            sct_num + spf * cnt,
                                            1,
                                            data);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    data[0x0] = 0x00;
    data[0x1] = 0x00;
    data[0x2] = 0x00;
    data[0x3] = 0x00;
    /* Start LBA sector of second block */
    for(cnt = 0; cnt < fats; cnt ++) {
        /* Hidden sector + Sector number of FAT * FAT number + PBR */
        sct_num = hid_sct + spf * cnt + 1;
        for(sct = 1; sct < spf; sct ++) {
            result = msproal_trans_write_lba(   msifhndl,
                                                sct_num + sct,
                                                1,
                                                data);
            if(MSPROAL_OK != result) {
                return result;
            }
        }
    }

    /*-----     resetting Root Directory Entry  -----------------------------*/
    /* Set special file(MEMSTICK.IND) */
    /* File name */
    data[0x0]   = 'M';
    data[0x1]   = 'E';
    data[0x2]   = 'M';
    data[0x3]   = 'S';
    data[0x4]   = 'T';
    data[0x5]   = 'I';
    data[0x6]   = 'C';
    data[0x7]   = 'K';
    /* Extension */
    data[0x8]   = 'I';
    data[0x9]   = 'N';
    data[0xA]   = 'D';
    /* Attribute */
    data[0xB]   = 0x03;
    /* Last edit time */
    data[0x16]  = LOBYTE(((msifhndl->BootData[0x1B3] >> 1) & 0x1F)
                | ((msifhndl->BootData[0x1B2] << 5) & 0xE0));
    data[0x17]  = LOBYTE(((msifhndl->BootData[0x1B2] >> 3) & 0x07)
                | ((msifhndl->BootData[0x1B1] << 3) & 0xF8));
    /* Last edit date */
    year        = MAKEWORD( msifhndl->BootData[0x1AD],
                            msifhndl->BootData[0x1AE]);
    data[0x18]  = LOBYTE((msifhndl->BootData[0x1B0] & 0x1F)
                | ((msifhndl->BootData[0x1AF] << 5) & 0xE0));
    data[0x19]  = LOBYTE(((msifhndl->BootData[0x1AF] >> 3) & 0x01)
                | (((year - 1980) << 1) & 0xFE));
    result = msproal_trans_write_lba(   msifhndl,
                                        hid_sct + spf * fats + 1,
                                        1,
                                        data);
    if(MSPROAL_OK != result) {
        return MSPROAL_OK;
    }

    msproal_user_memset(data, 0x00, 512);
    /* Hidden sector + Sector number of FAT * FAT number +  */
    sct_num = hid_sct + spf * fats + 1;
    for(cnt = 1; cnt < 32; cnt ++) {
        result = msproal_trans_write_lba(   msifhndl,
                                            sct_num + cnt,
                                            1,
                                            data);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    proc_data ++;
    prog[7]     = LOBYTE(LOWORD(proc_data));

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_format
*   DESCRIPTION : Format function for Memory Stick V1.X.
*------------------------------------------------------------------------------
*   SINT msproal_trans_format(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Mode(MSPROAL_QUICK_FORMAT/MSPROAL_FULL_FORMAT)
******************************************************************************/
SINT msproal_trans_format(MSIFHNDL *msifhndl, SINT mode)
{
    ULONG               hid_sct, sct_num;
    ULONG               all_data, proc_data;
    ULONG               pbuf_lltbl[3];
    SINT                result, cnt, sct, ms_no;
    SINT                pblk, seg, eseg, fn;
    UWORD               ftbl[16];
    UWORD               spf, fats, ladrs, eladrs, blk_num, blk_size, year;
    SBYTE               *fstype;
    UBYTE               *data, *prog, extradata[4], ovflg;
    const static UBYTE  def_spf[6] = {0x02, 0x03, 0x03, 0x06, 0x0C, 0x1F};
    const static UBYTE  hps[6] = {0x02, 0x02, 0x04, 0x04, 0x08, 0x10};
    const static UBYTE  mbr_part1[6][16] = {
        /* 4MB */
        {   0x80, 0x01, 0x0C, 0x00, 0x01, 0x01, 0x10, 0xF5,
            0x1B, 0x00, 0x00, 0x00, 0xA5, 0x1E, 0x00, 0x00},
        /* 8MB */
        {   0x80, 0x01, 0x0A, 0x00, 0x01, 0x01, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0xA7, 0x3D, 0x00, 0x00},
        /* 16MB */
        {   0x80, 0x01, 0x0A, 0x00, 0x01, 0x03, 0x50, 0xED,
            0x19, 0x00, 0x00, 0x00, 0x67, 0x7B, 0x00, 0x00},
        /* 32MB */
        {   0x80, 0x01, 0x04, 0x00, 0x01, 0x03, 0xD0, 0xDD,
            0x13, 0x00, 0x00, 0x00, 0x6D, 0xF7, 0x00, 0x00},
        /* 64MB */
        {   0x80, 0x02, 0x08, 0x00, 0x01, 0x07, 0xD0, 0xDD,
            0x27, 0x00, 0x00, 0x00, 0xD9, 0xEE, 0x01, 0x00},
        /* 128MB */
        {   0x80, 0x02, 0x02, 0x00, 0x06, 0x0F, 0xD0, 0xDD,
            0x21, 0x00, 0x00, 0x00, 0xDF, 0xDD, 0x03, 0x00}
    };

    blk_num     = MAKEWORD( msifhndl->BootData[0x1A4],
                            msifhndl->BootData[0x1A5]);
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3]);
    eseg        = blk_num >> 9;
    all_data    = 1;
    proc_data   = 0;
    prog        = msifhndl->WorkArea;
    prog[0]     = HIBYTE(HIWORD(all_data));
    prog[1]     = LOBYTE(HIWORD(all_data));
    prog[2]     = HIBYTE(LOWORD(all_data));
    prog[3]     = LOBYTE(LOWORD(all_data));
    prog[4]     = HIBYTE(HIWORD(proc_data));
    prog[5]     = LOBYTE(HIWORD(proc_data));
    prog[6]     = HIBYTE(LOWORD(proc_data));
    prog[7]     = LOBYTE(LOWORD(proc_data));

    /* Full? */
    if(MSPROAL_FULL_FORMAT == mode) {
        all_data    += eseg;
        prog[3]     = LOBYTE(LOWORD(all_data));

        if(-1 == (pblk = msifhndl->BkBootBlk)) {
            pblk = msifhndl->BootBlk;
        }
        pblk ++;
        ladrs                           = 0;
        eladrs                          = MS_SEG0_LADRS_NUM - 1;
        extradata[MSPROAL_OVFLG_ADRS]   = 0xF8;
        extradata[MSPROAL_MANAFLG_ADRS] = 0xFF;
        for(seg = 0; eseg > seg; seg ++) {
            fn = 0;
            msproal_user_memset((UBYTE *)ftbl, 0xff, sizeof(ftbl));
            /* Initialize tables */
            result = msproal_tbl_init_tbl(msifhndl, seg);
            if(MSPROAL_OK != result) {
                return result;
            }

            do {
                /* Disabled block ? */
                result = msproal_tbl_check_useblock(msifhndl, pblk);
                if(MSPROAL_OK != result) {
                    continue;
                }

                /* Read extradata of page 0 in block pblk */
                result = msproal_seq_v1_read_extradata( msifhndl,
                                                        pblk,
                                                        0,
                                                        extradata);

                ovflg = extradata[MSPROAL_OVFLG_ADRS];

                if(MSPROAL_OK != result) {
                    if((MSPROAL_FLASH_READ_ERR != result)) {
                        return result;
                    }

                    /* BKST is 1? */
                    if(ovflg & MS_OVFLG_BKST) {
                        /* BKST is set to 0 */
                        extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
                        result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                    pblk,
                                                                    0,
                                                                    extradata);
                        if(MSPROAL_OK != result) {
                            return result;
                        }
                    }

                    continue;
                }

                /* BKST is 0? */
                if(!(ovflg & MS_OVFLG_BKST)) {
                    continue;
                }

                /* PGST is 1 ? */
                if(!(ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
                    /* BKST of pblk is set to 0 */
                    extradata[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
                    result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                                pblk,
                                                                0,
                                                                extradata);
                    if(MSPROAL_OK != result) {
                        return result;
                    }

                    continue;
                }

                /* Erase block pblk */
                result = msproal_seq_v1_erase_block(msifhndl, pblk);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_FLASH_ERASE_ERR != result) {
                        return result;
                    }

                    continue;
                }

                /* Logical address is beyond the defined segment number */
                if(eladrs < ladrs) {
                    ftbl[fn ++] = pblk;
                    continue;
                }

                extradata[MSPROAL_LADRS1_ADRS] = HIBYTE(ladrs);
                extradata[MSPROAL_LADRS0_ADRS] = LOBYTE(ladrs);
                result = msproal_seq_v1_write_extradata(msifhndl,
                                                        pblk,
                                                        0,
                                                        extradata);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_FLASH_WRITE_ERR != result) {
                        return result;
                    }

                    continue;
                }

                /* Update logical/physical translation table */
                result = msproal_tbl_update_lptbl(  msifhndl,
                                                    seg,
                                                    ladrs,
                                                    pblk);
                if(MSPROAL_OK != result) {
                    return result;
                }

                ladrs ++;
            } while((pblk += 1) & 0x1FF);

            /* Update alternative block information and the number of   */
            /* alternative blocks                                       */
            result = msproal_tbl_update_freeblock(  msifhndl,
                                                    seg,
                                                    ftbl,
                                                    fn);
            if(MSPROAL_OK != result) {
                return result;
            }

            if(MSPROAL_READ_ONLY == msifhndl->Rw) {
                return MSPROAL_WRITE_ERR;
            }

            eladrs      += MS_SEGN_LADRS_NUM;
            proc_data ++;
            prog[7]     = LOBYTE(LOWORD(proc_data));
        }
    }

    /*---------     resetting MBR           -----------------------------*/
    /* ms_no =  0   4MB     */
    /*          1   8MB     */
    /*          2   16MB    */
    /*          3   32MB    */
    /*          4   64MB    */
    /*          5   128MB   */
    ms_no = 0;
    while(eseg >>= 1) {
        ms_no ++;
    }
    if(0x10 == blk_size) {
        ms_no ++;
    }

    data        = &msifhndl->WorkArea[512];
    msproal_user_memset(data, 0x00, 512);
    /* Set partition 1 */
    for(cnt = 0; cnt < 16; cnt ++) {
        data[0x1BE + cnt] = mbr_part1[ms_no][cnt];
    }
    /* Signature Wrod */
    data[0x1FE] = 0x55;
    data[0x1FF] = 0xAA;

#if         (1 == MSPROAL_SUPPORT_VMEM)
    msproal_user_virt_to_bus((void *)data, pbuf_lltbl);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
    pbuf_lltbl[0]   = (ULONG)data;
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
    pbuf_lltbl[1]   = 0;
    pbuf_lltbl[2]   = (ICON_DMA_CNF_DMAEN
                    | ICON_DMA_CNF_BSZ_64
                    | (512 >> 2));
    msproal_user_flush_cache((void *)data, 512);
    result = msproal_trans_write_lba(msifhndl, 0, 1, (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        return result;
    }

    /*---------     resetting PBR           -----------------------------*/
    msproal_user_memset(data, 0x00, 512);
    /* Jump code = 0xE90000 */
    data[0x000] = 0xE9;
    /* OEM name and version = "        " */
    msproal_user_memset(&data[0x003],' ',8);
    /* Number of bytes per sector = 512(0x0200) */
    data[0x00C] = 0x02;
    /* Number of sectors in a cluster */
    data[0x00D] = LOBYTE(blk_size << 1);
    /* Number of reserved sectors = 0x01 */
    data[0x00E] = 0x01;
    /* Number of FATs = 0x02 */
    data[0x010] = 0x02;
    /* Number of root directory entries = 0x0200 */
    data[0x012] = 0x02;
    /* Total sectors */
    /* Is the total sectors less than 65536 ? */
    if(0x00 == mbr_part1[ms_no][14]) {
        data[0x013] = mbr_part1[ms_no][12];
        data[0x014] = mbr_part1[ms_no][13];
    } else {
        data[0x020] = mbr_part1[ms_no][12];
        data[0x021] = mbr_part1[ms_no][13];
        data[0x022] = mbr_part1[ms_no][14];
        data[0x023] = mbr_part1[ms_no][15];
    }
    /* Media ID = 0xF8 */
    data[0x015] = 0xF8;
    /* Number of sectors in a FAT */
    data[0x016] = def_spf[ms_no];
    /* Number of sectors on a head(track) = 0x0010 */
    data[0x018] = 0x10;
    /* Number of heads */
    data[0x01A] = hps[ms_no];
    /* Number of hidden sectors */
    data[0x01C] = mbr_part1[ms_no][8];
    data[0x01D] = mbr_part1[ms_no][9];
    data[0x01E] = mbr_part1[ms_no][10];
    data[0x01F] = mbr_part1[ms_no][11];
    /* Extension boot signature = 0x29 */
    data[0x026] = 0x29;
    /* File system type */
    if(4096 < blk_num) {
        fstype = "FAT16   ";
    } else {
        fstype = "FAT12   ";
    }
    msproal_user_memcpy(&data[0x036], (UBYTE *)fstype, 8);
    /* Signature word */
    data[0x1FE] = 0x55;
    data[0x1FF] = 0xAA;

    hid_sct = MAKELONG( MAKEWORD(   mbr_part1[ms_no][9],
                                    mbr_part1[ms_no][8]),
                        MAKEWORD(   mbr_part1[ms_no][11],
                                    mbr_part1[ms_no][10]));
    msproal_user_flush_cache((void *)data, 512);
    result = msproal_trans_write_lba(   msifhndl,
                                        hid_sct,
                                        1,
                                        (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl->HidSct    = hid_sct;
    spf                 = MAKEWORD(data[0x17], data[0x16]);
    fats                = data[0x10];

    /*---------     resetting FAT           ---------------------------------*/
    msproal_user_memset(data, 0x00, 512);
    data[0x0] = 0xF8;
    data[0x1] = 0xFF;
    data[0x2] = 0xFF;
    /* FAT16 ? */
    if(4096 < blk_num) {
        data[3] = 0xFF;
    }
    msproal_user_flush_cache((void *)data, 512);
    /* Start LBA sector of first block */
    /* Hidden sector + PBR */
    sct_num = hid_sct + 1;
    for(cnt = 0; cnt < fats; cnt ++) {
        result = msproal_trans_write_lba(   msifhndl,
                                            sct_num + spf * cnt,
                                            1,
                                            (UBYTE *)pbuf_lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    data[0x0] = 0x00;
    data[0x1] = 0x00;
    data[0x2] = 0x00;
    data[0x3] = 0x00;
    msproal_user_flush_cache((void *)data, 512);
    /* Start LBA sector of second block */
    for(cnt = 0; cnt < fats; cnt ++) {
        /* Hidden sector + Sector number of FAT * FAT number + PBR */
        sct_num = hid_sct + spf * cnt + 1;
        for(sct = 1; sct < spf; sct ++) {
            result = msproal_trans_write_lba(   msifhndl,
                                                sct_num + sct,
                                                1,
                                                (UBYTE *)pbuf_lltbl);
            if(MSPROAL_OK != result) {
                return result;
            }
        }
    }

    /*-----     resetting Root Directory Entry  -----------------------------*/
    /* Set special file(MEMSTICK.IND) */
    /* File name */
    data[0x0]   = 'M';
    data[0x1]   = 'E';
    data[0x2]   = 'M';
    data[0x3]   = 'S';
    data[0x4]   = 'T';
    data[0x5]   = 'I';
    data[0x6]   = 'C';
    data[0x7]   = 'K';
    /* Extension */
    data[0x8]   = 'I';
    data[0x9]   = 'N';
    data[0xA]   = 'D';
    /* Attribute */
    data[0xB]   = 0x03;
    /* Last edit time */
    data[0x16]  = LOBYTE(((msifhndl->BootData[0x1B3] >> 1) & 0x1F)
                | ((msifhndl->BootData[0x1B2] << 5) & 0xE0));
    data[0x17]  = LOBYTE(((msifhndl->BootData[0x1B2] >> 3) & 0x07)
                | ((msifhndl->BootData[0x1B1] << 3) & 0xF8));
    /* Last edit date */
    year        = MAKEWORD( msifhndl->BootData[0x1AD],
                            msifhndl->BootData[0x1AE]);
    data[0x18]  = LOBYTE((msifhndl->BootData[0x1B0] & 0x1F)
                | ((msifhndl->BootData[0x1AF] << 5) & 0xE0));
    data[0x19]  = LOBYTE(((msifhndl->BootData[0x1AF] >> 3) & 0x01)
                | (((year - 1980) << 1) & 0xFE));
    msproal_user_flush_cache((void *)data, 512);
    result = msproal_trans_write_lba(   msifhndl,
                                        hid_sct + spf * fats + 1,
                                        1,
                                        (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        return MSPROAL_OK;
    }

    msproal_user_memset(data, 0x00, 512);
    msproal_user_flush_cache((void *)data, 512);
    /* Hidden sector + Sector number of FAT * FAT number +  */
    sct_num = hid_sct + spf * fats + 1;
    for(cnt = 1; cnt < 32; cnt ++) {
        result = msproal_trans_write_lba(   msifhndl,
                                            sct_num + cnt,
                                            1,
                                            (UBYTE *)pbuf_lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    proc_data ++;
    prog[7]     = LOBYTE(LOWORD(proc_data));

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_read_lba
*   DESCRIPTION : Read data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_trans_read_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of sectors for reading
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_trans_read_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result;
    SINT    cnt, seg, spage, epage, last_page, pblk, blk_size;
    UWORD   ladrs, last_ladrs;
    UBYTE   *extradata, *extra, page, manaflg, ovflg;

    /* Calculate logical address and start page number in a block */
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3])
                << 1;
    ladrs       = (UWORD)(lba / blk_size);
    spage       = lba % blk_size;
    last_page   = blk_size - 1;
    /* Calculate segment number */
    cnt = ladrs - MS_SEG0_LADRS_NUM - 1;
    seg = 0;
    while(0 < cnt) {
        cnt -= MS_SEGN_LADRS_NUM;
        seg ++;
    }
    /* Calculate maximum logical addres in the segment */
    last_ladrs  = MS_SEG0_LADRS_NUM + seg * MS_SEGN_LADRS_NUM - 1;
    extradata   = &msifhndl->DataBuf[512];

    while(0 < size) {
        if(blk_size < (spage + size)) {
            epage = last_page;
        } else {
            epage = spage + size - 1;
        }

        /* Logical address beyond the range of the segment ? */
        if(last_ladrs < ladrs) {
            seg ++;
            last_ladrs += MS_SEGN_LADRS_NUM;
        }

        /* Retrieve physical block for logical address using    */
        /* logical/physical translation table                   */
        result  = msproal_tbl_log_to_phy(msifhndl, seg, ladrs, &pblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Physical block not existing */
        if(MSPROAL_BLOCK_NOT_EXIST == pblk) {
            return MSPROAL_FORMAT_ERR;
        }

        result = msproal_seq_v1_read_block( msifhndl,
                                            pblk,
                                            spage,
                                            epage,
                                            data,
                                            extradata);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_READ_ERR != result) {
                return result;
            }

            return MSPROAL_READ_ERR;
        }

        extra = extradata;
        for(page = 0; epage >= spage; spage ++, page ++) {
            manaflg = extra[MSPROAL_MANAFLG_ADRS];
            ovflg   = extra[MSPROAL_OVFLG_ADRS];

            /* PGST of blk is 0 or 1 or 2 ? */
            if(!(ovflg & MS_OVFLG_PGST0) || !(ovflg & MS_OVFLG_PGST1)) {
                return MSPROAL_READ_ERR;
            }

            /* SCMS0 or SCMS1 of blk is 0 ? */
            if(!(manaflg & MS_MANAFLG_SCMS0)
            || !(manaflg & MS_MANAFLG_SCMS1)) {
                msproal_user_memset(data, 0x00, 512);
                return MSPROAL_READ_PROTECTED_ERR;
            }

            extra   += 4;
            data    += 512;
        }

        size    -= page;
        spage   = 0;
        ladrs ++;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_read_lba
*   DESCRIPTION : Read data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_trans_read_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of sectors for reading
*       data        : Address to area where read data is stored
******************************************************************************/
SINT msproal_trans_read_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   *lltbl, *cp_lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    result;
    SINT    cnt, seg, spage, epage, last_page, pblk, blk_size;
    SINT    trans_size, trans_cnt;
    UWORD   ladrs, last_ladrs;
    UBYTE   *extradata, *extra, page, manaflg, ovflg;

    /* Calculate logical address and start page number in a block */
    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3])
                << 1;
    ladrs       = (UWORD)(lba / blk_size);
    spage       = lba % blk_size;
    last_page   = blk_size - 1;
    /* Calculate segment number */
    cnt = ladrs - MS_SEG0_LADRS_NUM - 1;
    seg = 0;
    while(0 < cnt) {
        cnt -= MS_SEGN_LADRS_NUM;
        seg ++;
    }
    /* Calculate maximum logical addres in the segment */
    last_ladrs  = MS_SEG0_LADRS_NUM + seg * MS_SEGN_LADRS_NUM - 1;
    extradata   = &msifhndl->DataBuf[512];
    cp_lltbl    = (ULONG *)data;
#if         (1 == MSPROAL_SUPPORT_VMEM)
    msproal_user_bus_to_virt(cp_lltbl[0], (void **)&data);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
    data        = (UBYTE *)cp_lltbl[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
    idadrsreg   = 0;
    idcnfreg    = 0;

    while(0 < size) {
        if(blk_size < (spage + size)) {
            epage = last_page;
        } else {
            epage = spage + size - 1;
        }

        /* Logical address beyond the range of the segment ? */
        if(last_ladrs < ladrs) {
            seg ++;
            last_ladrs += MS_SEGN_LADRS_NUM;
        }

        /* Retrieve physical block for logical address using    */
        /* logical/physical translation table                   */
        result  = msproal_tbl_log_to_phy(msifhndl, seg, ladrs, &pblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Physical block not existing */
        if(MSPROAL_BLOCK_NOT_EXIST == pblk) {
            return MSPROAL_FORMAT_ERR;
        }

        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        trans_size  = (epage - spage + 1) * 512;

        if(0 != idadrsreg) {
            lltbl[0]        = idadrsreg;
            trans_cnt       = (idcnfreg & ICON_DMA_CNF_TRCNT_MASK) * 4;
            if(0 == trans_cnt) {
                trans_cnt   = MSPROAL_MAX_TRANS_NUM;
            }
            if(trans_size > trans_cnt) {
#if         (1 == MSPROAL_SUPPORT_VMEM)
                msproal_user_virt_to_bus((void *)&lltbl[3], &lltbl[1]);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[1]    = (ULONG)&lltbl[3];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[2]    = (idcnfreg
                            | (ICON_DMA_CNF_LLTEN | ICON_DMA_CNF_DMAEN));
                trans_size  -= trans_cnt;
                idadrsreg   = 0;
                idcnfreg    = 0;
            } else if(trans_size == trans_cnt) {
                lltbl[1]    = 0;
                lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                            | ICON_DMA_CNF_DMAEN);
                idadrsreg   = 0;
                idcnfreg    = 0;
                trans_size  -= trans_cnt;
            } else {
                lltbl[1]    = 0;
                lltbl[2]    = ((idcnfreg & ICON_DMA_CNF_BSZ_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | (trans_size >> 2));
                idadrsreg   += trans_size;
                idcnfreg    = ((idcnfreg & ~ICON_DMA_CNF_TRCNT_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | ((trans_cnt - trans_size) >> 2));
                trans_size  = 0;
            }

            lltbl += 3;
        }

        while(0 < trans_size) {
            lltbl[0]        = cp_lltbl[0];
            trans_cnt       = (cp_lltbl[2] & ICON_DMA_CNF_TRCNT_MASK) * 4;
            if(0 == trans_cnt) {
                trans_cnt   = MSPROAL_MAX_TRANS_NUM;
            }
            if(trans_size > trans_cnt) {
#if         (1 == MSPROAL_SUPPORT_VMEM)
                msproal_user_virt_to_bus((void *)&lltbl[3], &lltbl[1]);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[1]    = (ULONG)&lltbl[3];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[2]    = (cp_lltbl[2]
                            | (ICON_DMA_CNF_LLTEN | ICON_DMA_CNF_DMAEN));
                trans_size  -= trans_cnt;
            } else if(trans_size == trans_cnt) {
                lltbl[1]    = 0;
                lltbl[2]    = ((cp_lltbl[2] & ~ICON_DMA_CNF_LLTEN)
                            | ICON_DMA_CNF_DMAEN);
                trans_size  -= trans_cnt;
            } else {
                lltbl[1]    = 0;
                lltbl[2]    = ((cp_lltbl[2] & ICON_DMA_CNF_BSZ_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | (trans_size >> 2));
                idadrsreg   = cp_lltbl[0] + trans_size;
                idcnfreg    = ((cp_lltbl[2] & ~ICON_DMA_CNF_TRCNT_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | ((trans_cnt - trans_size) >> 2));
                trans_size  = 0;
            }

            lltbl       += 3;
            cp_lltbl    += 3;
        }

        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
        msproal_user_invalidate_cache((void *)extradata, 512);
        result = msproal_seq_v1_read_block( msifhndl,
                                            pblk,
                                            spage,
                                            epage,
                                            (UBYTE *)lltbl,
                                            extradata);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_READ_ERR != result) {
                return result;
            }

            return MSPROAL_READ_ERR;
        }

        extra = extradata;
        for(page = 0; epage >= spage; spage ++, page ++) {
            manaflg = extra[MSPROAL_MANAFLG_ADRS];
            ovflg   = extra[MSPROAL_OVFLG_ADRS];

            /* PGST of blk is 0 or 1 or 2 ? */
            if(!(ovflg & MS_OVFLG_PGST0) || !(ovflg & MS_OVFLG_PGST1)) {
                return MSPROAL_READ_ERR;
            }

            /* SCMS0 or SCMS1 of blk is 0 ? */
            if(!(manaflg & MS_MANAFLG_SCMS0)
            || !(manaflg & MS_MANAFLG_SCMS1)) {
                msproal_user_memset(data, 0x00, 512);
                return MSPROAL_READ_PROTECTED_ERR;
            }

            extra   += 4;
            data    += 512;
        }

        size    -= page;
        spage   = 0;
        ladrs ++;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_update_block
*   DESCRIPTION : Update sector data.
*------------------------------------------------------------------------------
*   SINT msproal_trans_update_block(MSIFHNDL *msifhndl, UWORD ladrs,
*           SINT spage, SINT epage, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       ladrs       : Logical address
*       spage       : Start page number being updated
*       epage       : End page number being updated
*       data        : Area used to store the data to be written to DataArea
******************************************************************************/
SINT msproal_trans_update_block(MSIFHNDL *msifhndl, UWORD ladrs, SINT spage,
        SINT epage, UBYTE *data)
{
    SINT    result;
    SINT    cnt, seg, rpblk, wpblk, end_page;
    SINT    PSErrFlg;
    SINT    fn;
    UWORD   pblk;
    UBYTE   extra[4], ovflg;

    PSErrFlg    = MSPROAL_FALSE;

    cnt = ladrs - 493;
    seg = 0;
    while(0 < cnt) {
        cnt -= 496;
        seg ++;
    }

    /* Retrieve physical block for logical address using logical/physical   */
    /* translation table                                                    */
    result = msproal_tbl_log_to_phy(msifhndl, seg, ladrs, &rpblk);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Read extradata of page 0 in block rpblk */
    result = msproal_seq_v1_read_extradata( msifhndl,
                                            rpblk,
                                            0,
                                            extra);

    ovflg = extra[MSPROAL_OVFLG_ADRS];

    if(MSPROAL_OK != result) {
        if((MSPROAL_FLASH_READ_ERR != result)) {
            return result;
        }

        /* PGST is 3? */
        if((ovflg & MS_OVFLG_PGST0) && (ovflg & MS_OVFLG_PGST1)) {
            /* Set page status to 1 */
            extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_PGST0);
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        rpblk,
                                                        0,
                                                        extra);
            if(MSPROAL_OK != result) {
               return result;
            }
        }
        PSErrFlg = MSPROAL_TRUE;
    }

    /* UDST is 1? */
    if(ovflg & MS_OVFLG_UDST) {
        /* Set update status to 0 */
        extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~(MS_OVFLG_UDST));
        result = msproal_seq_v1_overwrite_extradata(msifhndl, rpblk, 0, extra);
        if(MSPROAL_OK != result) {
            return result;
        }
    }


    end_page    = (MAKEWORD(msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3]) << 1)
                - 1;

    extra[MSPROAL_MANAFLG_ADRS] = 0xFF;
    extra[MSPROAL_LADRS1_ADRS]  = HIBYTE(ladrs);
    extra[MSPROAL_LADRS0_ADRS]  = LOBYTE(ladrs);
    while(MSPROAL_OK == (result = msproal_tbl_get_freeblock(msifhndl,
                                                            &wpblk,
                                                            seg))) {
        extra[MSPROAL_OVFLG_ADRS]   = 0xF8;

        if(spage > 0) {
            /* Copy to before the page updated first from page 0 */
            result = msproal_trans_copy_block(  msifhndl,
                                                rpblk,
                                                wpblk,
                                                0,
                                                spage - 1,
                                                extra);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_READ_ERR != result) {
                    if(MSPROAL_FLASH_WRITE_ERR != result) {
                        return result;
                    }

                    continue;
                }

                PSErrFlg = MSPROAL_TRUE;
            }
        }

        /* Write to update data */
        result = msproal_seq_v1_write_block(msifhndl,
                                            wpblk,
                                            spage,
                                            epage,
                                            data,
                                            extra);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_WRITE_ERR != result) {
                return result;
            }

            continue;
        }

        if(end_page != epage) {
            /* Copy to last page from after the last page to update */
            result = msproal_trans_copy_block(  msifhndl,
                                                rpblk,
                                                wpblk,
                                                epage + 1,
                                                end_page,
                                                extra);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_READ_ERR != result) {
                    if(MSPROAL_FLASH_WRITE_ERR != result) {
                        return result;
                    }

                    continue;
                }

                PSErrFlg = MSPROAL_TRUE;
            }
        }

        /* Update logical/physical translation table */
        result = msproal_tbl_update_lptbl(  msifhndl,
                                            seg,
                                            ladrs,
                                            wpblk);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* BKST is 1? and PGST of all pages in read block are 0 or 3 ? */
        if(!PSErrFlg) {
            /* Erase the original physical block */
            result = msproal_seq_v1_erase_block(msifhndl, rpblk);
            if(MSPROAL_OK != result) {
                if(MSPROAL_FLASH_ERASE_ERR != result) {
                    return result;
                }

                fn = 0;
            } else {
                fn = 1;
            }
            /* Update alternative block information and the number of   */
            /* alternative blocks                                       */
            pblk = (UWORD)rpblk;
            result = msproal_tbl_update_freeblock(  msifhndl,
                                                    seg,
                                                    &pblk,
                                                    fn);
            if(MSPROAL_OK != result) {
                return result;
            }
        } else {
            extra[MSPROAL_OVFLG_ADRS] = (UBYTE)(~MS_OVFLG_BKST);
            result = msproal_seq_v1_overwrite_extradata(msifhndl,
                                                        rpblk,
                                                        0,
                                                        extra);
            if(MSPROAL_OK != result) {
                return result;
            }
        }

        return MSPROAL_OK;
    }

    return result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#if         (5 != MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_write_lba
*   DESCRIPTION : Write data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_trans_write_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of sectors for writing
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_trans_write_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result;
    SINT    spage, epage, blk_size, last_page;
    UWORD   ladrs;

    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3])
                << 1;
    ladrs       = (UWORD)(lba / blk_size);
    spage       = lba % blk_size;
    last_page   = blk_size - 1;
    while(0 < size) {
        if(blk_size < (spage + size)) {
            epage = last_page;
        } else {
            epage = spage + size - 1;
        }

        result = msproal_trans_update_block(msifhndl,
                                            ladrs,
                                            spage,
                                            epage,
                                            data);
        if(MSPROAL_OK != result) {
            return result;
        }

        data    += (epage - spage + 1) << 9;
        size    -= (epage - spage + 1);
        ladrs ++;
        spage   = 0;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_V1)
/******************************************************************************
*   FUNCTION    : msproal_trans_write_lba
*   DESCRIPTION : Write data of specified sector size from specified logical
*               sector number.
*------------------------------------------------------------------------------
*   SINT msproal_trans_write_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_PROTECTED_ERR  : Read protected error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of sectors for writing
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_trans_write_lba(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   *lltbl, *cp_lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    result;
    SINT    trans_size, trans_cnt;
    SINT    spage, epage, blk_size, last_page;
    UWORD   ladrs;

    blk_size    = MAKEWORD( msifhndl->BootData[0x1A2],
                            msifhndl->BootData[0x1A3])
                << 1;
    ladrs       = (UWORD)(lba / blk_size);
    spage       = lba % blk_size;
    last_page   = blk_size - 1;
    cp_lltbl    = (ULONG *)data;
#if         (1 == MSPROAL_SUPPORT_VMEM)
    msproal_user_bus_to_virt(cp_lltbl[0], (void **)&data);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
    data        = (UBYTE *)cp_lltbl[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
    idadrsreg   = 0;
    idcnfreg    = 0;

    while(0 < size) {
        if(blk_size < (spage + size)) {
            epage = last_page;
        } else {
            epage = spage + size - 1;
        }

        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        trans_size  = (epage - spage + 1) * 512;

        if(0 != idadrsreg) {
            lltbl[0]        = idadrsreg;
            trans_cnt       = (idcnfreg & ICON_DMA_CNF_TRCNT_MASK) * 4;
            if(0 == trans_cnt) {
                trans_cnt   = MSPROAL_MAX_TRANS_NUM;
            }
            if(trans_size > trans_cnt) {
#if         (1 == MSPROAL_SUPPORT_VMEM)
                msproal_user_virt_to_bus((void *)&lltbl[3], &lltbl[1]);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[1]    = (ULONG)&lltbl[3];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[2]    = (idcnfreg
                            | (ICON_DMA_CNF_LLTEN | ICON_DMA_CNF_DMAEN));
                trans_size  -= trans_cnt;
                idadrsreg   = 0;
                idcnfreg    = 0;
            } else if(trans_size == trans_cnt) {
                lltbl[1]    = 0;
                lltbl[2]    = ((idcnfreg & ~ICON_DMA_CNF_LLTEN)
                            | ICON_DMA_CNF_DMAEN);
                idadrsreg   = 0;
                idcnfreg    = 0;
                trans_size  -= trans_cnt;
            } else {
                lltbl[1]    = 0;
                lltbl[2]    = ((idcnfreg & ICON_DMA_CNF_BSZ_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | (trans_size >> 2));
                idadrsreg   += trans_size;
                idcnfreg    = ((idcnfreg & ~ICON_DMA_CNF_TRCNT_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | ((trans_cnt - trans_size) >> 2));
                trans_size  = 0;
            }

            lltbl += 3;
        }

        while(0 < trans_size) {
            lltbl[0]        = cp_lltbl[0];
            trans_cnt       = (cp_lltbl[2] & ICON_DMA_CNF_TRCNT_MASK) * 4;
            if(0 == trans_cnt) {
                trans_cnt   = MSPROAL_MAX_TRANS_NUM;
            }
            if(trans_size > trans_cnt) {
#if         (1 == MSPROAL_SUPPORT_VMEM)
                msproal_user_virt_to_bus((void *)&lltbl[3], &lltbl[1]);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[1]    = (ULONG)&lltbl[3];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
                lltbl[2]    = (cp_lltbl[2]
                            | (ICON_DMA_CNF_LLTEN | ICON_DMA_CNF_DMAEN));
                trans_size  -= trans_cnt;
            } else if(trans_size == trans_cnt) {
                lltbl[1]    = 0;
                lltbl[2]    = ((cp_lltbl[2] & ~ICON_DMA_CNF_LLTEN)
                            | ICON_DMA_CNF_DMAEN);
                trans_size  -= trans_cnt;
            } else {
                lltbl[1]    = 0;
                lltbl[2]    = ((cp_lltbl[2] & ICON_DMA_CNF_BSZ_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | (trans_size >> 2));
                idadrsreg   = cp_lltbl[0] + trans_size;
                idcnfreg    = ((cp_lltbl[2] & ~ICON_DMA_CNF_TRCNT_MASK)
                            | ICON_DMA_CNF_DMAEN
                            | ((trans_cnt - trans_size) >> 2));
                trans_size  = 0;
            }

            lltbl       += 3;
            cp_lltbl    += 3;
        }

        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
        result = msproal_trans_update_block(msifhndl,
                                            ladrs,
                                            spage,
                                            epage,
                                            (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }

        data    += (epage - spage + 1) << 9;
        size    -= (epage - spage + 1);
        ladrs ++;
        spage   = 0;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */
