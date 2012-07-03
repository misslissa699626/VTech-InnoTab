/*=============================================================================
* Copyright 2003-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_pro.c
*
* DESCRIPTION   : Memory Stick Pro API
*
* FUNCTION LIST
*                   msproal_drv_pro_attribute_confirmation
*                   msproal_drv_pro_check_mbsr
*                   msproal_drv_pro_check_mpbr
*                   msproal_drv_pro_get_model_name
*                   msproal_drv_pro_mount
*                   msproal_drv_pro_read_data
*                   msproal_drv_pro_recovery
*                   msproal_drv_pro_stop
*                   msproal_drv_pro_wakeup
*                   msproal_drv_pro_write_data
*                   msproal_seq_pro_change_power
*                   msproal_seq_pro_erase
*                   msproal_seq_pro_format
*                   msproal_seq_pro_read_atrb
*                   msproal_seq_pro_read_data
*                   msproal_seq_pro_read_2k_data
*                   msproal_seq_pro_sleep
*                   msproal_seq_pro_startup
*                   msproal_seq_pro_write_data
*                   msproal_seq_pro_write_2k_data
=============================================================================*/
#include <mach/ms/msproal_pro.h>
#include <mach/ms/msproal_msif.h>
#include <mach/ms/msproal_tpc.h>
#include <mach/ms/msproal_icon.h>
#include <mach/ms/msproal_config.h>
#include <mach/ms/msproal_mc_pro.h>


#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_attribute_confirmation
*   DESCRIPTION : The contents of the attribute information on Memory Stick PRO
*               are checked.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_attribute_confirmation(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_LOCKED_ERR          : Read/Write protect error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_attribute_confirmation(MSIFHNDL *msifhndl)
{
    ULONG   ent_size, ent_adrs, sct, stick;
    SINT    result;
    SINT    ent_num, ent;
    UBYTE   *btdt, ent_cnt, devid, devtype;

    /* Read attribute information */
    btdt = msifhndl->WorkArea;
    result = msproal_seq_pro_read_atrb(msifhndl, 0, 1, btdt);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_MEDIA_ERR;
    }

    /* Signature Code != 0xA5C3 */
    if((0xA5 != btdt[0x00]) || (0xC3 != btdt[0x01])) {
        return MSPROAL_MEDIA_ERR;
    }

    /* Is entry count less than 1, or larger than 12? */
    ent_cnt = btdt[0x04];
    if(11 < (UINT)(ent_cnt - 1)) {
        return MSPROAL_MEDIA_ERR;
    }

    msproal_user_memcpy(&msifhndl->BootData[0], &btdt[0], 0x1A0);

    ent = 0x10;
    for(ent_num = 0; ent_num < ent_cnt; ent_num ++) {
        /* System Information? */
        devid = btdt[ent + 8];
#if         (1 == MSPROAL_SUPPORT_XC)
        if((0x10 == devid) || (0x13 == devid)) {
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
        if(0x10 == devid) {
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
            /* Entry size is not 0x60 */
            ent_size    = MAKELONG( MAKEWORD(btdt[ent + 6], btdt[ent + 7]),
                                    MAKEWORD(btdt[ent + 4], btdt[ent + 5]));
            if(0x60 != ent_size) {
                return MSPROAL_MEDIA_ERR;
            }

            /* Is entry address less than 0x1A0? */
            /* Is (entry address + entry size) larger than 0x8000? */
            ent_adrs    = MAKELONG( MAKEWORD(btdt[ent + 2], btdt[ent + 3]),
                                    MAKEWORD(btdt[ent + 0], btdt[ent + 1]));

            if(0x7E00 < (ULONG)(ent_adrs - 0x1A0)) {
                return MSPROAL_MEDIA_ERR;
            }

            /* Is there system information in the 512 bytes read? */
            if(0x1A0 != ent_adrs) {
                sct         = ent_adrs >> 9;
                ent_adrs    &= 0x000001FF;

                /* Read 2 pages from sct */
                result = msproal_seq_pro_read_atrb(msifhndl, sct, 2, btdt);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_READ_ERR != result) {
                        return result;
                    }

                    return MSPROAL_MEDIA_ERR;
                }
            }

#if         (1 == MSPROAL_SUPPORT_XC)
            if(0x10 == devid) {
                /* Class of Memory Stick is not 2 */
                if(0x02 != btdt[ent_adrs]) {
                    return MSPROAL_MEDIA_ERR;
                }

                stick = msifhndl->Stick;
            } else {
                /* Class of Memory Stick is not 3 */
                if(0x03 != btdt[ent_adrs]) {
                    return MSPROAL_MEDIA_ERR;
                }

                stick = msifhndl->Stick;
                stick &= ~MSPROAL_STICK_PRO;
                stick |= MSPROAL_STICK_XC;
            }
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
            /* Class of Memory Stick is not 2 */
            if(0x02 != btdt[ent_adrs]) {
                return MSPROAL_MEDIA_ERR;
            }

            stick = msifhndl->Stick;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
			
			msifhndl->BlockSize = MAKEWORD(btdt[ent_adrs + 2], btdt[ent_adrs + 3]);
            msifhndl->UserAreaBlocks = MAKEWORD(btdt[ent_adrs + 6], btdt[ent_adrs + 7]);
            msifhndl->PageSize = MAKEWORD(btdt[ent_adrs + 8], btdt[ent_adrs + 9]);
            msifhndl->UnitSize = MAKEWORD(btdt[ent_adrs + 44], btdt[ent_adrs + 45]);
            
            /* devtype is not 0, 1, 2, or 3 */
            devtype = btdt[ent_adrs + 56];
            if(0x03 < devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }

            /* Media type determination processing is performed */
            /* Class Register is 0x00 */
            if(MSPROAL_STICK_RW & stick) {
                ;
            /* Class Register is 0x01 */
            } else if(MSPROAL_STICK_ROM & stick) {
                if((0 == devtype) || (2 == devtype)) {
                    msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                    return MSPROAL_UNSUPPORT_ERR;
                }
            /* Class Register is 0x02 */
            } else if(MSPROAL_STICK_R & stick) {
                if(0 == devtype) {
                    msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                    return MSPROAL_UNSUPPORT_ERR;
                }
            /* Class Register is 0x03 */
            } else {
                if(0 == devtype) {
                    msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                    return MSPROAL_UNSUPPORT_ERR;
                }

                devtype = 3;
            }
            /* Media type is determined */
            stick &= ~MSPROAL_STICK_DEVICE_MASK;
            stick |= (1L << devtype);

            msifhndl->Stick = stick;

            /* In case of Read Only */
            if(!(MSPROAL_STICK_RW & stick)) {
                msifhndl->Rw = MSPROAL_READ_ONLY;
            }

#if         (1 == MSPROAL_SUPPORT_XC)
            /* Power type of Sub class is stored */
            msifhndl->Stick |=
            (btdt[ent_adrs + 46] & MS2_SUBCLS_PW_CLS_MASK) << 6;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */

            /* System Information is stored in the same position as */
            /* Boot&Attribute Information registered into bootblock */
            /* of Memory Stick                                      */
            msproal_user_memcpy(&msifhndl->BootData[MSPROAL_BOOT_ATTR_ADRS],
                                &btdt[ent_adrs],
                                (SINT)ent_size);

            if(btdt[ent_adrs + 46] &
                    (MS2_SUBCLS_EXP_LOCKED | MS2_SUBCLS_STD_LOCKED)) {
                return MSPROAL_LOCKED_ERR;
            }

            return MSPROAL_OK;
        }

        ent += 12;
    }

    return MSPROAL_MEDIA_ERR;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_attribute_confirmation
*   DESCRIPTION : The contents of the attribute information on Memory Stick PRO
*               are checked.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_attribute_confirmation(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_LOCKED_ERR          : Read/Write protect error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_attribute_confirmation(MSIFHNDL *msifhndl)
{
    ULONG   pbuf_lltbl[3];
    ULONG   ent_size, ent_adrs, sct, stick;
    SINT    result;
    SINT    ent_num, ent;
    UBYTE   *btdt, ent_cnt, devid, devtype;

    /* Read attribute information */
    btdt = msifhndl->WorkArea;
#if         (1 == MSPROAL_SUPPORT_VMEM)
    msproal_user_virt_to_bus((void *)btdt, pbuf_lltbl);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
    pbuf_lltbl[0] = (ULONG)btdt;
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
    pbuf_lltbl[1] = 0;
    pbuf_lltbl[2]   = (ICON_DMA_CNF_DMAEN
                    | ICON_DMA_CNF_BSZ_64
                    | (512 >> 2));
    msproal_user_invalidate_cache((void *)btdt, 512);
    result = msproal_seq_pro_read_atrb(msifhndl, 0, 1, (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_MEDIA_ERR;
    }

    /* Signature Code != 0xA5C3 */
    if((0xA5 != btdt[0x00]) || (0xC3 != btdt[0x01])) {
        return MSPROAL_MEDIA_ERR;
    }

    /* Is entry count less than 1, or larger than 12? */
    ent_cnt = btdt[0x04];
    if(11 < (UINT)(ent_cnt - 1)) {
        return MSPROAL_MEDIA_ERR;
    }

    msproal_user_memcpy(&msifhndl->BootData[0], &btdt[0], 0x1A0);

    ent = 0x10;
    for(ent_num = 0; ent_num < ent_cnt; ent_num ++) {
        /* System Information? */
        devid = btdt[ent + 8];
#if         (1 == MSPROAL_SUPPORT_XC)
        if((0x10 == devid) || (0x13 == devid)) {
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
        if(0x10 == devid) {
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
            /* Entry size is not 0x60 */
            ent_size    = MAKELONG( MAKEWORD(btdt[ent + 6], btdt[ent + 7]),
                                    MAKEWORD(btdt[ent + 4], btdt[ent + 5]));
            if(0x60 != ent_size) {
                return MSPROAL_MEDIA_ERR;
            }

            /* Is entry address less than 0x1A0? */
            /* Is (entry address + entry size) larger than 0x8000? */
            ent_adrs    = MAKELONG( MAKEWORD(btdt[ent + 2], btdt[ent + 3]),
                                    MAKEWORD(btdt[ent + 0], btdt[ent + 1]));

            if(0x7E00 < (ULONG)(ent_adrs - 0x1A0)) {
                return MSPROAL_MEDIA_ERR;
            }

            /* Is there system information in the 512 bytes read? */
            if(0x1A0 != ent_adrs) {
                sct         = ent_adrs >> 9;
                ent_adrs    &= 0x000001FF;

                /* Read 2 pages from sct */
                pbuf_lltbl[2]   = (ICON_DMA_CNF_DMAEN
                                | ICON_DMA_CNF_BSZ_64
                                | (1024 >> 2));
                msproal_user_invalidate_cache((void *)btdt, 512 * 2);
                result = msproal_seq_pro_read_atrb( msifhndl,
                                                    sct,
                                                    2,
                                                    (UBYTE *)pbuf_lltbl);
                if(MSPROAL_OK != result) {
                    if(MSPROAL_READ_ERR != result) {
                        return result;
                    }

                    return MSPROAL_MEDIA_ERR;
                }
            }

#if         (1 == MSPROAL_SUPPORT_XC)
            if(0x10 == devid) {
                /* Class of Memory Stick is not 2 */
                if(0x02 != btdt[ent_adrs]) {
                    return MSPROAL_MEDIA_ERR;
                }

                stick = msifhndl->Stick;
            } else {
                /* Class of Memory Stick is not 3 */
                if(0x03 != btdt[ent_adrs]) {
                    return MSPROAL_MEDIA_ERR;
                }

                stick = msifhndl->Stick;
                stick &= ~MSPROAL_STICK_PRO;
                stick |= MSPROAL_STICK_XC;
            }
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
            /* Class of Memory Stick is not 2 */
            if(0x02 != btdt[ent_adrs]) {
                return MSPROAL_MEDIA_ERR;
            }

            stick = msifhndl->Stick;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */

            /* devtype is not 0, 1, 2, or 3 */
            devtype = btdt[ent_adrs + 56];
            if(0x03 < devtype) {
                msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                return MSPROAL_UNSUPPORT_ERR;
            }

            /* Media type determination processing is performed */
            /* Class Register is 0x00 */
            if(MSPROAL_STICK_RW & stick) {
                ;
            /* Class Register is 0x01 */
            } else if(MSPROAL_STICK_ROM & stick) {
                if((0 == devtype) || (2 == devtype)) {
                    msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                    return MSPROAL_UNSUPPORT_ERR;
                }
            /* Class Register is 0x02 */
            } else if(MSPROAL_STICK_R & stick) {
                if(0 == devtype) {
                    msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                    return MSPROAL_UNSUPPORT_ERR;
                }
            /* Class Register is 0x03 */
            } else {
                if(0 == devtype) {
                    msifhndl->Stick = MSPROAL_STICK_UNKNOWN;
                    return MSPROAL_UNSUPPORT_ERR;
                }

                devtype = 3;
            }
            /* Media type is determined */
            stick &= ~MSPROAL_STICK_DEVICE_MASK;
            stick |= (1L << devtype);

            msifhndl->Stick = stick;

            /* In case of Read Only */
            if(!(MSPROAL_STICK_RW & stick)) {
                msifhndl->Rw = MSPROAL_READ_ONLY;
            }

#if         (1 == MSPROAL_SUPPORT_XC)
            /* Power type of Sub class is stored */
            msifhndl->Stick |=
            (btdt[ent_adrs + 46] & MS2_SUBCLS_PW_CLS_MASK) << 6;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */

            /* System Information is stored in the same position as */
            /* Boot&Attribute Information registered into bootblock */
            /* of Memory Stick                                      */
            msproal_user_memcpy(&msifhndl->BootData[MSPROAL_BOOT_ATTR_ADRS],
                                &btdt[ent_adrs],
                                (SINT)ent_size);

            if(btdt[ent_adrs + 46] &
                    (MS2_SUBCLS_EXP_LOCKED | MS2_SUBCLS_STD_LOCKED)) {
                return MSPROAL_LOCKED_ERR;
            }

            return MSPROAL_OK;
        }

        ent += 12;
    }

    return MSPROAL_MEDIA_ERR;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_XC)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_check_mbsr
*   DESCRIPTION : Checking the contents of MBR and BSR of Memory Stick XC.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_check_mbsr(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_check_mbsr(MSIFHNDL *msifhndl)
{
    ULONG   blk_size, effect_blk_num;
    ULONG   total_sct, sct_num;
    SINT    result;
    SINT    bsr;
    SINT    cnt;
    UWORD   unit_size;
    UBYTE   *data;

    data = msifhndl->WorkArea;
    result = msproal_seq_pro_read_data(msifhndl, 0, 1, data);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    blk_size        = MAKELONG( MAKEWORD(   msifhndl->BootData[0x1C2],
                                            msifhndl->BootData[0x1C3]),
                                MAKEWORD(   msifhndl->BootData[0x1C0],
                                            msifhndl->BootData[0x1C1]));
    effect_blk_num  = MAKELONG( MAKEWORD(   msifhndl->BootData[0x1A8],
                                            msifhndl->BootData[0x1A9]),
                                MAKEWORD(   msifhndl->BootData[0x1A6],
                                            msifhndl->BootData[0x1A7]));
    unit_size       = MAKEWORD( msifhndl->BootData[0x1CC],
                                msifhndl->BootData[0x1CD]);
    total_sct       = blk_size * effect_blk_num * (unit_size / 512);

    bsr             = MAKELONG( MAKEWORD(data[0x1C7], data[0x1C6]),
                                MAKEWORD(data[0x1C9], data[0x1C8]));
    sct_num         = MAKELONG( MAKEWORD(data[0x1CB], data[0x1CA]),
                                MAKEWORD(data[0x1CD], data[0x1CC]));

    /* MBR? */
    if(((0x80 != data[0x1BE]) && (0x00 != data[0x1BE]))
    || (0x55 != data[0x1FE])
    || (0xAA != data[0x1FF])
    || (0x07 != data[0x1C2])
    || (total_sct < (bsr + sct_num))) {
        return MSPROAL_FORMAT_ERR;
    }

    result = msproal_seq_pro_read_data(msifhndl, bsr, 1, data);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    /* BSR? */
    if(((0x00 == data[0x000])
        && (0x00 == data[0x001])
        && (0x00 == data[0x002]))
    || (0x55 != data[0x1FE])
    || (0xAA != data[0x1FF])) {
        return MSPROAL_FORMAT_ERR;
    }

    /* exFAT? */
    if((0x45 != data[0x003])
    || (0x58 != data[0x004])
    || (0x46 != data[0x005])
    || (0x41 != data[0x006])
    || (0x54 != data[0x007])
    || (0x20 != data[0x008])
    || (0x20 != data[0x009])
    || (0x20 != data[0x00A])) {
        return MSPROAL_FORMAT_ERR;
    }
    for(cnt = 0x0B; 0x40 > cnt; cnt ++) {
        if(0x00 != data[cnt]) {
            return MSPROAL_FORMAT_ERR;
        }
    }

    msifhndl->HidSct = bsr;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_XC)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_check_mbsr
*   DESCRIPTION : Checking the contents of MBR and BSR of Memory Stick XC.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_check_mbsr(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_check_mbsr(MSIFHNDL *msifhndl)
{
    ULONG   pbuf_lltbl[3];
    ULONG   blk_size, effect_blk_num;
    ULONG   total_sct, sct_num;
    SINT    result;
    SINT    bsr;
    SINT    cnt;
    UWORD   unit_size;
    UBYTE   *data;

    data = msifhndl->WorkArea;
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
    result = msproal_seq_pro_read_data(msifhndl, 0, 1, (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    blk_size        = MAKELONG( MAKEWORD(   msifhndl->BootData[0x1C2],
                                            msifhndl->BootData[0x1C3]),
                                MAKEWORD(   msifhndl->BootData[0x1C0],
                                            msifhndl->BootData[0x1C1]));
    effect_blk_num  = MAKELONG( MAKEWORD(   msifhndl->BootData[0x1A8],
                                            msifhndl->BootData[0x1A9]),
                                MAKEWORD(   msifhndl->BootData[0x1A6],
                                            msifhndl->BootData[0x1A7]));
    unit_size       = MAKEWORD( msifhndl->BootData[0x1CC],
                                msifhndl->BootData[0x1CD]);
    total_sct       = blk_size * effect_blk_num * (unit_size / 512);

    bsr             = MAKELONG( MAKEWORD(data[0x1C7], data[0x1C6]),
                                MAKEWORD(data[0x1C9], data[0x1C8]));
    sct_num         = MAKELONG( MAKEWORD(data[0x1CB], data[0x1CA]),
                                MAKEWORD(data[0x1CD], data[0x1CC]));

    /* MBR? */
    if(((0x80 != data[0x1BE]) && (0x00 != data[0x1BE]))
    || (0x55 != data[0x1FE])
    || (0xAA != data[0x1FF])
    || (0x07 != data[0x1C2])
    || (total_sct < (bsr + sct_num))) {
        return MSPROAL_FORMAT_ERR;
    }

    msproal_user_invalidate_cache((void *)data, 512);
    result = msproal_seq_pro_read_data( msifhndl,
                                        bsr,
                                        1,
                                        (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    /* BSR? */
    if(((0x00 == data[0x000])
        && (0x00 == data[0x001])
        && (0x00 == data[0x002]))
    || (0x55 != data[0x1FE])
    || (0xAA != data[0x1FF])) {
        return MSPROAL_FORMAT_ERR;
    }

    /* exFAT? */
    if((0x45 != data[0x003])
    || (0x58 != data[0x004])
    || (0x46 != data[0x005])
    || (0x41 != data[0x006])
    || (0x54 != data[0x007])
    || (0x20 != data[0x008])
    || (0x20 != data[0x009])
    || (0x20 != data[0x00A])) {
        return MSPROAL_FORMAT_ERR;
    }
    for(cnt = 0x0B; 0x40 > cnt; cnt ++) {
        if(0x00 != data[cnt]) {
            return MSPROAL_FORMAT_ERR;
        }
    }

    msifhndl->HidSct = bsr;

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_check_mpbr
*   DESCRIPTION : Checking the contents of MBR and PBR of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_check_mpbr(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_check_mpbr(MSIFHNDL *msifhndl)
{
    SINT    result;
    SINT    pbr;
    UBYTE   *data;

    data = msifhndl->WorkArea;
    result = msproal_seq_pro_read_data(msifhndl, 0, 1, data);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    /* MBR? */
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
        pbr = (data[0x1C6]
            | (data[0x1C7] << 8)
            | (data[0x1C8] << 16)
            | (data[0x1C9] << 24));
        result = msproal_seq_pro_read_data(msifhndl, pbr, 1, data);
        if(MSPROAL_OK != result) {
            if(MSPROAL_READ_ERR != result) {
                return result;
            }

            return MSPROAL_FORMAT_ERR;
        }
    } else {
        pbr = 0;
    }

    /* PBR? */
    if(((0xE9 == data[0x00]) || (0xEB == data[0x00]))
    && (0x55 == data[0x1FE])
    && (0xAA == data[0x1FF])) {
        msifhndl->HidSct = pbr;

        return MSPROAL_OK;
    }

    return MSPROAL_FORMAT_ERR;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_check_mpbr
*   DESCRIPTION : Checking the contents of MBR and PBR of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_check_mpbr(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_check_mpbr(MSIFHNDL *msifhndl)
{
    ULONG   pbuf_lltbl[3];
    SINT    result;
    SINT    pbr;
    UBYTE   *data;

    data = msifhndl->WorkArea;
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
    result = msproal_seq_pro_read_data(msifhndl, 0, 1, (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    /* MBR? */
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
        pbr = (data[0x1C6]
            | (data[0x1C7] << 8)
            | (data[0x1C8] << 16)
            | (data[0x1C9] << 24));
        msproal_user_invalidate_cache((void *)data, 512);
        result = msproal_seq_pro_read_data( msifhndl,
                                            pbr,
                                            1,
                                            (UBYTE *)pbuf_lltbl);
        if(MSPROAL_OK != result) {
            if(MSPROAL_READ_ERR != result) {
                return result;
            }

            return MSPROAL_FORMAT_ERR;
        }
    } else {
        pbr = 0;
    }

    /* PBR? */
    if(((0xE9 == data[0x00]) || (0xEB == data[0x00]))
    && (0x55 == data[0x1FE])
    && (0xAA == data[0x1FF])) {
        msifhndl->HidSct = pbr;

        return MSPROAL_OK;
    }

    return MSPROAL_FORMAT_ERR;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_get_model_name
*   DESCRIPTION : Get the Model name.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_get_model_name(MSIFHNDL *msifhndl, SBYTE *modelname)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       modelname   : Address to area where the information is stored
******************************************************************************/
SINT msproal_drv_pro_get_model_name(MSIFHNDL *msifhndl, SBYTE *modelname)
{
    ULONG   ent_adrs, sct;
    ULONG   blk_size, effect_blk_num, capa;
    SINT    result;
    SINT    ent_num, ent;
    UWORD   unit_size;
    UBYTE   *btdt, ent_cnt;

    btdt    = msifhndl->BootData;
    ent_cnt = btdt[0x04];
    ent     = 0x10;
    for(ent_num = 0; ent_num < ent_cnt; ent_num ++) {
        /* Model name? */
        if(0x15 == btdt[ent + 8]) {
            /* Entry address */
            ent_adrs    = MAKELONG( MAKEWORD(btdt[ent + 2], btdt[ent + 3]),
                                    MAKEWORD(btdt[ent + 0], btdt[ent + 1]));

            /* Read Attribute Information */
            sct         = ent_adrs >> 9;
            ent_adrs    &= 0x000001FF;

            /* Read 2 pages from sct */
            btdt = msifhndl->WorkArea;
            result = msproal_seq_pro_read_atrb(msifhndl, sct, 2, btdt);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Model name is stored in modelname */
            msproal_user_memcpy((UBYTE *)modelname, &btdt[ent_adrs], 0x30);

            return MSPROAL_OK;
        }

        ent += 12;
    }

    /* If there is not the Model name in the device information entry,  */
    /* create it according to Memory Stick Pro Format                   */
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
#if         (1 == MSPROAL_SUPPORT_XC)
    if(msifhndl->Stick & MSPROAL_STICK_XC) {    /* Memory Stick XC */
        modelname[12]   = ' ';
        modelname[13]   = 'X';
        modelname[14]   = 'C';

        if(msifhndl->Stick & MSPROAL_STICK_R) {
            modelname[15]   = '-';
            modelname[16]   = 'R';
            modelname[17]   = ' ';
            modelname[18]   = 'D';
            modelname[19]   = 'u';
            modelname[20]   = 'o';
            modelname[21]   = ' ';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
            modelname[15]   = '-';
            modelname[16]   = 'R';
            modelname[17]   = 'O';
            modelname[18]   = 'M';
            modelname[19]   = ' ';
            modelname[20]   = 'D';
            modelname[21]   = 'u';
            modelname[22]   = 'o';
            modelname[23]   = ' ';
        } else {
            modelname[15]   = ' ';
            modelname[16]   = 'D';
            modelname[17]   = 'u';
            modelname[18]   = 'o';
            modelname[19]   = ' ';
            modelname[20]   = ' ';
            modelname[21]   = ' ';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        }

        blk_size        = MAKELONG( MAKEWORD( msifhndl->BootData[0x1C2],
                                              msifhndl->BootData[0x1C3]),
                                    MAKEWORD( msifhndl->BootData[0x1C0],
                                              msifhndl->BootData[0x1C1]));
        effect_blk_num  = MAKELONG( MAKEWORD( msifhndl->BootData[0x1A8],
                                              msifhndl->BootData[0x1A9]),
                                    MAKEWORD( msifhndl->BootData[0x1A6],
                                              msifhndl->BootData[0x1A7]));
    } else {                                    /* Memory Stick PRO */
        modelname[12]   = ' ';
        modelname[13]   = 'P';
        modelname[14]   = 'R';
        modelname[15]   = 'O';

        if(msifhndl->Stick & (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK)) {
            /* Memory Stick PRO-HG Duo */
            modelname[16]   = '-';
            modelname[17]   = 'H';
            modelname[18]   = 'G';
            modelname[19]   = ' ';
            modelname[20]   = 'D';
            modelname[21]   = 'u';
            modelname[22]   = 'o';
            modelname[23]   = ' ';
        } else {
            if(msifhndl->Stick & MSPROAL_STICK_R) {
                modelname[16]   = '-';
                modelname[17]   = 'R';
                modelname[18]   = ' ';
                modelname[19]   = 'D';
                modelname[20]   = 'u';
                modelname[21]   = 'o';
                modelname[22]   = ' ';
                modelname[23]   = ' ';
            } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
                modelname[16]   = '-';
                modelname[17]   = 'R';
                modelname[18]   = 'O';
                modelname[19]   = 'M';
                modelname[20]   = ' ';
                modelname[21]   = 'D';
                modelname[22]   = 'u';
                modelname[23]   = 'o';
            } else {
                modelname[16]   = ' ';
                modelname[17]   = 'D';
                modelname[18]   = 'u';
                modelname[19]   = 'o';
                modelname[20]   = ' ';
                modelname[21]   = ' ';
                modelname[22]   = ' ';
                modelname[23]   = ' ';
            }
        }

        blk_size        = MAKEWORD( msifhndl->BootData[0x1A2],
                                    msifhndl->BootData[0x1A3]);
        effect_blk_num  = MAKEWORD( msifhndl->BootData[0x1A6],
                                    msifhndl->BootData[0x1A7]);
    }
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
    modelname[12]   = ' ';
    modelname[13]   = 'P';
    modelname[14]   = 'R';
    modelname[15]   = 'O';

    if(msifhndl->Stick & (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK)) {
        /* Memory Stick PRO-HG Duo */
        modelname[16]   = '-';
        modelname[17]   = 'H';
        modelname[18]   = 'G';
        modelname[19]   = ' ';
        modelname[20]   = 'D';
        modelname[21]   = 'u';
        modelname[22]   = 'o';
        modelname[23]   = ' ';
    } else {
        if(msifhndl->Stick & MSPROAL_STICK_R) {
            modelname[16]   = '-';
            modelname[17]   = 'R';
            modelname[18]   = ' ';
            modelname[19]   = 'D';
            modelname[20]   = 'u';
            modelname[21]   = 'o';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
            modelname[16]   = '-';
            modelname[17]   = 'R';
            modelname[18]   = 'O';
            modelname[19]   = 'M';
            modelname[20]   = ' ';
            modelname[21]   = 'D';
            modelname[22]   = 'u';
            modelname[23]   = 'o';
        } else {
            modelname[16]   = ' ';
            modelname[17]   = 'D';
            modelname[18]   = 'u';
            modelname[19]   = 'o';
            modelname[20]   = ' ';
            modelname[21]   = ' ';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        }
    }

    blk_size        = MAKEWORD( msifhndl->BootData[0x1A2],
                                msifhndl->BootData[0x1A3]);
    effect_blk_num  = MAKEWORD( msifhndl->BootData[0x1A6],
                                msifhndl->BootData[0x1A7]);
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
    modelname[24]   = ' ';

    unit_size       = MAKEWORD( msifhndl->BootData[0x1CC],
                                msifhndl->BootData[0x1CD]);

    /* Capacity */
    capa = blk_size * effect_blk_num * (unit_size / 512);

    modelname[25]   = '(';
    modelname[26]   = ' ';
    if(0x00010000 > capa) {         /*   32MB */
        modelname[27]   = ' ';
        modelname[28]   = '3';
        modelname[29]   = '2';
        modelname[30]   = 'M';
    } else if(0x00020000 > capa) {  /*   64MB */
        modelname[27]   = ' ';
        modelname[28]   = '6';
        modelname[29]   = '4';
        modelname[30]   = 'M';
    } else if(0x00040000 > capa) {  /*  128MB */
        modelname[27]   = '1';
        modelname[28]   = '2';
        modelname[29]   = '8';
        modelname[30]   = 'M';
    } else if(0x00080000 > capa) {  /*  256MB */
        modelname[27]   = '2';
        modelname[28]   = '5';
        modelname[29]   = '6';
        modelname[30]   = 'M';
    } else if(0x00100000 > capa) {  /*  512MB */
        modelname[27]   = '5';
        modelname[28]   = '1';
        modelname[29]   = '2';
        modelname[30]   = 'M';
    } else if(0x00200000 > capa) {  /*    1GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '1';
        modelname[30]   = 'G';
    } else if(0x00400000 > capa) {  /*    2GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '2';
        modelname[30]   = 'G';
    } else if(0x00800000 > capa) {  /*    4GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '4';
        modelname[30]   = 'G';
    } else if(0x01000000 > capa) {  /*    8GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '8';
        modelname[30]   = 'G';
    } else if(0x02000000 > capa) {  /*   16GB */
        modelname[27]   = ' ';
        modelname[28]   = '1';
        modelname[29]   = '6';
        modelname[30]   = 'G';
    } else if(0x04000000 > capa) {  /*   32GB */
        modelname[27]   = ' ';
        modelname[28]   = '3';
        modelname[29]   = '2';
        modelname[30]   = 'G';
    } else if(0x08000000 > capa) {  /*   64GB */
        modelname[27]   = ' ';
        modelname[28]   = '6';
        modelname[29]   = '4';
        modelname[30]   = 'G';
    } else if(0x10000000 > capa) {  /*  128GB */
        modelname[27]   = '1';
        modelname[28]   = '2';
        modelname[29]   = '8';
        modelname[30]   = 'G';
    } else if(0x20000000 > capa) {  /*  256GB */
        modelname[27]   = '2';
        modelname[28]   = '5';
        modelname[29]   = '6';
        modelname[30]   = 'G';
    } else if(0x40000000 > capa) {  /*  512GB */
        modelname[27]   = '5';
        modelname[28]   = '1';
        modelname[29]   = '2';
        modelname[30]   = 'G';
    } else if(0x80000000 > capa) {  /*    1TB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '1';
        modelname[30]   = 'T';
    } else {                        /*    2TB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '2';
        modelname[30]   = 'T';
    }
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
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_get_model_name
*   DESCRIPTION : Get the Model name.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_get_model_name(MSIFHNDL *msifhndl, SBYTE *modelname)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       modelname   : Address to area where the information is stored
******************************************************************************/
SINT msproal_drv_pro_get_model_name(MSIFHNDL *msifhndl, SBYTE *modelname)
{
    ULONG   pbuf_lltbl[3];
    ULONG   ent_adrs, sct;
    ULONG   blk_size, effect_blk_num, capa;
    SINT    result;
    SINT    ent_num, ent;
    UWORD   unit_size;
    UBYTE   *btdt, ent_cnt;

    btdt    = msifhndl->BootData;
    ent_cnt = btdt[0x04];
    ent     = 0x10;
    for(ent_num = 0; ent_num < ent_cnt; ent_num ++) {
        /* Model name? */
        if(0x15 == btdt[ent + 8]) {
            /* Entry address */
            ent_adrs    = MAKELONG( MAKEWORD(btdt[ent + 2], btdt[ent + 3]),
                                    MAKEWORD(btdt[ent + 0], btdt[ent + 1]));

            /* Read Attribute Information */
            sct         = ent_adrs >> 9;
            ent_adrs    &= 0x000001FF;

            /* Read 2 pages from sct */
            btdt = msifhndl->WorkArea;
#if         (1 == MSPROAL_SUPPORT_VMEM)
            msproal_user_virt_to_bus((void *)btdt, pbuf_lltbl);
#else   /*  (0 == MSPROAL_SUPPORT_VMEM) */
            pbuf_lltbl[0]   = (ULONG)btdt;
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
            pbuf_lltbl[1]   = 0;
            pbuf_lltbl[2]   = (ICON_DMA_CNF_DMAEN
                            | ICON_DMA_CNF_BSZ_64
                            | (1024 >> 2));
            msproal_user_invalidate_cache((void *)btdt, 512 * 2);
            result = msproal_seq_pro_read_atrb( msifhndl,
                                                sct,
                                                2,
                                                (UBYTE *)pbuf_lltbl);
            if(MSPROAL_OK != result) {
                return result;
            }

            /* Model name is stored in modelname */
            msproal_user_memcpy((UBYTE *)modelname, &btdt[ent_adrs], 0x30);

            return MSPROAL_OK;
        }

        ent += 12;
    }

    /* If there is not the Model name in the device information entry,  */
    /* create it according to Memory Stick Pro Format                   */
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
#if         (1 == MSPROAL_SUPPORT_XC)
    if(msifhndl->Stick & MSPROAL_STICK_XC) {    /* Memory Stick XC */
        modelname[12]   = ' ';
        modelname[13]   = 'X';
        modelname[14]   = 'C';

        if(msifhndl->Stick & MSPROAL_STICK_R) {
            modelname[15]   = '-';
            modelname[16]   = 'R';
            modelname[17]   = ' ';
            modelname[18]   = 'D';
            modelname[19]   = 'u';
            modelname[20]   = 'o';
            modelname[21]   = ' ';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
            modelname[15]   = '-';
            modelname[16]   = 'R';
            modelname[17]   = 'O';
            modelname[18]   = 'M';
            modelname[19]   = ' ';
            modelname[20]   = 'D';
            modelname[21]   = 'u';
            modelname[22]   = 'o';
            modelname[23]   = ' ';
        } else {
            modelname[15]   = ' ';
            modelname[16]   = 'D';
            modelname[17]   = 'u';
            modelname[18]   = 'o';
            modelname[19]   = ' ';
            modelname[20]   = ' ';
            modelname[21]   = ' ';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        }

        blk_size        = MAKELONG( MAKEWORD( msifhndl->BootData[0x1C2],
                                              msifhndl->BootData[0x1C3]),
                                    MAKEWORD( msifhndl->BootData[0x1C0],
                                              msifhndl->BootData[0x1C1]));
        effect_blk_num  = MAKELONG( MAKEWORD( msifhndl->BootData[0x1A8],
                                              msifhndl->BootData[0x1A9]),
                                    MAKEWORD( msifhndl->BootData[0x1A6],
                                              msifhndl->BootData[0x1A7]));
    } else {                                    /* Memory Stick PRO */
        modelname[12]   = ' ';
        modelname[13]   = 'P';
        modelname[14]   = 'R';
        modelname[15]   = 'O';

        if(msifhndl->Stick & (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK)) {
            /* Memory Stick PRO-HG Duo */
            modelname[16]   = '-';
            modelname[17]   = 'H';
            modelname[18]   = 'G';
            modelname[19]   = ' ';
            modelname[20]   = 'D';
            modelname[21]   = 'u';
            modelname[22]   = 'o';
            modelname[23]   = ' ';
        } else {
            if(msifhndl->Stick & MSPROAL_STICK_R) {
                modelname[16]   = '-';
                modelname[17]   = 'R';
                modelname[18]   = ' ';
                modelname[19]   = 'D';
                modelname[20]   = 'u';
                modelname[21]   = 'o';
                modelname[22]   = ' ';
                modelname[23]   = ' ';
            } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
                modelname[16]   = '-';
                modelname[17]   = 'R';
                modelname[18]   = 'O';
                modelname[19]   = 'M';
                modelname[20]   = ' ';
                modelname[21]   = 'D';
                modelname[22]   = 'u';
                modelname[23]   = 'o';
            } else {
                modelname[16]   = ' ';
                modelname[17]   = 'D';
                modelname[18]   = 'u';
                modelname[19]   = 'o';
                modelname[20]   = ' ';
                modelname[21]   = ' ';
                modelname[22]   = ' ';
                modelname[23]   = ' ';
            }
        }

        blk_size        = MAKEWORD( msifhndl->BootData[0x1A2],
                                    msifhndl->BootData[0x1A3]);
        effect_blk_num  = MAKEWORD( msifhndl->BootData[0x1A6],
                                    msifhndl->BootData[0x1A7]);
    }
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
    modelname[12]   = ' ';
    modelname[13]   = 'P';
    modelname[14]   = 'R';
    modelname[15]   = 'O';

    if(msifhndl->Stick & (MSPROAL_STICK_2K | MSPROAL_STICK_S8P_STICK)) {
        /* Memory Stick PRO-HG Duo */
        modelname[16]   = '-';
        modelname[17]   = 'H';
        modelname[18]   = 'G';
        modelname[19]   = ' ';
        modelname[20]   = 'D';
        modelname[21]   = 'u';
        modelname[22]   = 'o';
        modelname[23]   = ' ';
    } else {
        if(msifhndl->Stick & MSPROAL_STICK_R) {
            modelname[16]   = '-';
            modelname[17]   = 'R';
            modelname[18]   = ' ';
            modelname[19]   = 'D';
            modelname[20]   = 'u';
            modelname[21]   = 'o';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        } else if(msifhndl->Stick & MSPROAL_STICK_ROM) {
            modelname[16]   = '-';
            modelname[17]   = 'R';
            modelname[18]   = 'O';
            modelname[19]   = 'M';
            modelname[20]   = ' ';
            modelname[21]   = 'D';
            modelname[22]   = 'u';
            modelname[23]   = 'o';
        } else {
            modelname[16]   = ' ';
            modelname[17]   = 'D';
            modelname[18]   = 'u';
            modelname[19]   = 'o';
            modelname[20]   = ' ';
            modelname[21]   = ' ';
            modelname[22]   = ' ';
            modelname[23]   = ' ';
        }
    }

    blk_size        = MAKEWORD( msifhndl->BootData[0x1A2],
                                msifhndl->BootData[0x1A3]);
    effect_blk_num  = MAKEWORD( msifhndl->BootData[0x1A6],
                                msifhndl->BootData[0x1A7]);
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
    modelname[24]   = ' ';

    unit_size       = MAKEWORD( msifhndl->BootData[0x1CC],
                                msifhndl->BootData[0x1CD]);

    /* Capacity */
    capa = blk_size * effect_blk_num * (unit_size / 512);

    modelname[25]   = '(';
    modelname[26]   = ' ';
    if(0x00010000 > capa) {         /*   32MB */
        modelname[27]   = ' ';
        modelname[28]   = '3';
        modelname[29]   = '2';
        modelname[30]   = 'M';
    } else if(0x00020000 > capa) {  /*   64MB */
        modelname[27]   = ' ';
        modelname[28]   = '6';
        modelname[29]   = '4';
        modelname[30]   = 'M';
    } else if(0x00040000 > capa) {  /*  128MB */
        modelname[27]   = '1';
        modelname[28]   = '2';
        modelname[29]   = '8';
        modelname[30]   = 'M';
    } else if(0x00080000 > capa) {  /*  256MB */
        modelname[27]   = '2';
        modelname[28]   = '5';
        modelname[29]   = '6';
        modelname[30]   = 'M';
    } else if(0x00100000 > capa) {  /*  512MB */
        modelname[27]   = '5';
        modelname[28]   = '1';
        modelname[29]   = '2';
        modelname[30]   = 'M';
    } else if(0x00200000 > capa) {  /*    1GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '1';
        modelname[30]   = 'G';
    } else if(0x00400000 > capa) {  /*    2GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '2';
        modelname[30]   = 'G';
    } else if(0x00800000 > capa) {  /*    4GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '4';
        modelname[30]   = 'G';
    } else if(0x01000000 > capa) {  /*    8GB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '8';
        modelname[30]   = 'G';
    } else if(0x02000000 > capa) {  /*   16GB */
        modelname[27]   = ' ';
        modelname[28]   = '1';
        modelname[29]   = '6';
        modelname[30]   = 'G';
    } else if(0x04000000 > capa) {  /*   32GB */
        modelname[27]   = ' ';
        modelname[28]   = '3';
        modelname[29]   = '2';
        modelname[30]   = 'G';
    } else if(0x08000000 > capa) {  /*   64GB */
        modelname[27]   = ' ';
        modelname[28]   = '6';
        modelname[29]   = '4';
        modelname[30]   = 'G';
    } else if(0x10000000 > capa) {  /*  128GB */
        modelname[27]   = '1';
        modelname[28]   = '2';
        modelname[29]   = '8';
        modelname[30]   = 'G';
    } else if(0x20000000 > capa) {  /*  256GB */
        modelname[27]   = '2';
        modelname[28]   = '5';
        modelname[29]   = '6';
        modelname[30]   = 'G';
    } else if(0x40000000 > capa) {  /*  512GB */
        modelname[27]   = '5';
        modelname[28]   = '1';
        modelname[29]   = '2';
        modelname[30]   = 'G';
    } else if(0x80000000 > capa) {  /*    1TB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '1';
        modelname[30]   = 'T';
    } else {                        /*    2TB */
        modelname[27]   = ' ';
        modelname[28]   = ' ';
        modelname[29]   = '2';
        modelname[30]   = 'T';
    }
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
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_mount
*   DESCRIPTION : Get the information of Memory Stick PRO, and enable access
*               from a filesystem.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_mount(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_LOCKED_ERR          : Read/Write protect error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE/MSPROAL_8PARALLEL_MODE)
******************************************************************************/
SINT msproal_drv_pro_mount(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result;

    /* Starting check */
    result = msproal_seq_pro_startup(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl->IfModeMax = mode;
#if         (1 != MSPROAL_SUPPORT_IFMODE)
    if(MSPROAL_SERIAL_MODE != mode) {
        /* Change to parallel mode */
        result = msproal_msif_change_ifmode(msifhndl,
                                            MSPROAL_PRO_4PARALLEL_MODE);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

#if         ((1 == MSPROAL_SUPPORT_PROHG) && (3 == MSPROAL_SUPPORT_IFMODE))
    if(MSPROAL_8PARALLEL_MODE == mode) {
        /* Supporting 8bit Parallel Mode? */
        if(MSPROAL_STICK_S8P_COM & msifhndl->Stick) {
            /* Change to 8bit parallel mode */
            result = msproal_msif_change_ifmode(msifhndl,
                                                MSPROAL_8PARALLEL_MODE);
            if(MSPROAL_OK != result) {
                return result;
            }
        }
    }
#endif  /*  ((1 == MSPROAL_SUPPORT_PROHG) && (3 == MSPROAL_SUPPORT_IFMODE)) */
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE)   */

    /* The contents confirmation of attribute information */
    result = msproal_drv_pro_attribute_confirmation(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

#if         (1 == MSPROAL_SUPPORT_XC)
    if(msifhndl->Stick & MSPROAL_STICK_XC) {    /* Memory Stick XC */
        return msproal_drv_pro_check_mbsr(msifhndl);
    } else {
        return msproal_drv_pro_check_mpbr(msifhndl);
    }
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
    return msproal_drv_pro_check_mpbr(msifhndl);
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
}
#endif  /*  !((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))   */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_mount
*   DESCRIPTION : Get the information of Memory Stick PRO, and enable access
*               from a filesystem.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_mount(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_MEDIA_ERR           : Media error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_UNSUPPORT_ERR       : Unsupport media error
*       MSPROAL_LOCKED_ERR          : Read/Write protect error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Interface Mode(MSPROAL_SERIAL_MODE/
*                   MSPROAL_4PARALLEL_MODE)
******************************************************************************/
SINT msproal_drv_pro_mount(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result;

    /* Starting check */
    result = msproal_seq_pro_startup(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    msifhndl->IfModeMax = mode;
#if         (1 != MSPROAL_SUPPORT_IFMODE)
    if(MSPROAL_SERIAL_MODE != mode) {
        /* Change to parallel mode */
        result = msproal_msif_change_ifmode(msifhndl,
                                            MSPROAL_PRO_4PARALLEL_MODE);
        if(MSPROAL_OK != result) {
        	
        	/*
        	========== TEST# : T8-003 ==========

			*** Check Item ***

			<<<2-8-3>>>
		  	If the interface mode cannot be switched even though the TPC has been retried, 
		  	the targeted product shall turn off the power, and then turn it back on in order
		  	 to reset the interface mode.  In this case, the targeted product shall access by the serial interface mode.

			<<<2-2-38>>>
		  	To restore the power to medium back on after having turned it off, the power supply voltage shall be applied 
		  	more than 10 [ms] or later after its power voltage drops to 2.0 [V] or lower.

        	*/
			result = msproal_user_control_power(MSPROAL_POWER_OFF);
			if(MSPROAL_OK != result) {
				return result;
			}

			/* Waiting for 10ms */
			result = msproal_tpc_wait_int(msifhndl, 20, MSPROAL_WTIME);
			if(MSPROAL_OK != result) {
				return result;
			}

			result = msproal_user_control_power(MSPROAL_POWER_ON);
			if(MSPROAL_OK != result) {
				return result;
			}

			/* Waiting for 10ms */
			result = msproal_tpc_wait_int(msifhndl, 10, MSPROAL_WTIME);
			if(MSPROAL_OK != result) {
				return result;
			}

        	/* Starting check */
        	if(MSPROAL_OK != (result = msproal_seq_pro_startup(msifhndl))) {
            	return result;
        	}
        }
    }
#endif  /*  (1 != MSPROAL_SUPPORT_IFMODE)   */

    /* The contents confirmation of attribute information */
    result = msproal_drv_pro_attribute_confirmation(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

#if         (1 == MSPROAL_SUPPORT_XC)
    if(msifhndl->Stick & MSPROAL_STICK_XC) {    /* Memory Stick XC */
        return msproal_drv_pro_check_mbsr(msifhndl);
    } else {
        return msproal_drv_pro_check_mpbr(msifhndl);
    }
#else   /*  (1 == MSPROAL_SUPPORT_XC)   */
    return msproal_drv_pro_check_mpbr(msifhndl);
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         ((1 == MSPROAL_SUPPORT_IP) || (2 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_drv_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
#if         (1 == MSPROAL_SUPPORT_PROHG)
    SINT    result, nsize;

    if(0 == size) {
        return MSPROAL_OK;
    }

    if(!(MSPROAL_STICK_2K & msifhndl->Stick)) {
        return msproal_seq_pro_read_data(msifhndl, lba, size, data);
    }

    /* LBA and size is a multiple of 4 ? */
    if(0 == (lba & 0x3)) {
        if(0 == (size & 0x3)) {
            return msproal_seq_pro_read_2k_data(msifhndl, lba, size, data);
        }
    } else {
        /* Size is less than the size from lba to the following address     */
        /* which is multiple of 4 ?                                         */
        if((SINT)(4 - (lba & 0x3)) > size) {
            nsize = size;
        } else {
            /* Set the size from lba to the following address which is  */
            /* multiple of 4                                            */
            nsize = 4 - (lba & 0x3);
        }

        result = msproal_seq_pro_read_data(msifhndl, lba, nsize, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
        data    += (nsize << 9);
    }

    /* Reading is possible by the unit of 2048 byte ? */
    if(3 < size) {
        nsize   = (size & ~0x3);
        result = msproal_seq_pro_read_2k_data(msifhndl, lba, nsize, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
        data    += (nsize << 9);
    }

    /* There is one or more sectors that have not been read yet ? */
    if(0 < size) {
        result = msproal_seq_pro_read_data(msifhndl, lba, size, data);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
#else   /*  (0 == MSPROAL_SUPPORT_PROHG)    */
    if(0 == size) {
        return MSPROAL_OK;
    }

    return msproal_seq_pro_read_data(msifhndl, lba, size, data);
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (2 == MSPROAL_SUPPORT_IP))    */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_drv_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    if(0 == size) {
        return MSPROAL_OK;
    }

    return msproal_seq_pro_read_data(msifhndl, lba, size, data);
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of read-out sectors
*       data        : Address to area where read Data is stored
******************************************************************************/
SINT msproal_drv_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
#if         (1 == MSPROAL_SUPPORT_PROHG)
    ULONG   *lltbl, *cp_lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    result, nsize;
    SINT    trans_size, trans_cnt;

    if(0 == size) {
        return MSPROAL_OK;
    }

    if(!(MSPROAL_STICK_2K & msifhndl->Stick)) {
        return msproal_seq_pro_read_data(msifhndl, lba, size, data);
    }

    cp_lltbl        = (ULONG *)data;
    idadrsreg       = 0;
    idcnfreg        = 0;

    /* LBA and size is a multiple of 4 ? */
    if(0 == (lba & 0x3)) {
        if(0 == (size & 0x3)) {
            return msproal_seq_pro_read_2k_data(msifhndl, lba, size, data);
        }
    } else {
        /* Size is less than the size from lba to the following address     */
        /* which is multiple of 4 ?                                         */
        if((SINT)(4 - (lba & 0x3)) > size) {
            nsize = size;
        } else {
            /* Set the size from lba to the following address which is  */
            /* multiple of 4                                            */
            nsize = 4 - (lba & 0x3);
        }

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        trans_size  = nsize * 512;

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

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
        msproal_user_flush_cache((void *)lltbl, 512 * 4);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        result = msproal_seq_pro_read_data( msifhndl,
                                            lba,
                                            nsize,
                                            (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
    }

    /* Reading is possible by the unit of 2048 byte ? */
    if(3 < size) {
        nsize       = (size & ~0x3);
#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        trans_size  = nsize * 512;

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

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
        msproal_user_flush_cache((void *)lltbl, 512 * 4);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        result = msproal_seq_pro_read_2k_data(  msifhndl,
                                                lba,
                                                nsize,
                                                (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
    }

    /* There is one or more sectors that have not been read yet ? */
    if(0 < size) {
#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        trans_size  = size * 512;

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
                trans_size  = 0;
            }

            lltbl       += 3;
            cp_lltbl    += 3;
        }

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
        msproal_user_flush_cache((void *)lltbl, 512 * 4);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        result  = msproal_seq_pro_read_data(msifhndl,
                                            lba,
                                            size,
                                            (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
#else   /*  (0 == MSPROAL_SUPPORT_PROHG)    */
    if(0 == size) {
        return MSPROAL_OK;
    }

    return msproal_seq_pro_read_data(msifhndl, lba, size, data);
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

/******************************************************************************
*   FUNCTION    : msproal_drv_pro_recovery
*   DESCRIPTION : Execute recovery procedure for Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_recovery(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_recovery(MSIFHNDL *msifhndl)
{
    SINT    result;
    SINT    ifmode;
#if         (1 == MSPROAL_SUPPORT_XC)
    SINT    pwrcls;
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */

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

    /* 20 MHz */
    msproal_user_change_clock(MSPROAL_SERIAL_MODE);

    /* Starting check */
    result = msproal_seq_pro_startup(msifhndl);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Change the interface mode into the preserved value */
    ifmode = msifhndl->IfMode;
    switch(ifmode) {
    case MSPROAL_SERIAL_MODE:
        break;
    case MSPROAL_4PARALLEL_MODE:
        result = msproal_msif_change_ifmode(msifhndl,
                                            MSPROAL_PRO_4PARALLEL_MODE);
        if(MSPROAL_OK != result) {
            return result;
        }
        break;
    case MSPROAL_8PARALLEL_MODE:
        result = msproal_msif_change_ifmode(msifhndl,
                                            MSPROAL_PRO_4PARALLEL_MODE);
        if(MSPROAL_OK != result) {
            return result;
        }

        result = msproal_msif_change_ifmode(msifhndl, MSPROAL_8PARALLEL_MODE);
        if(MSPROAL_OK != result) {
            return result;
        }
        break;
    default:
        return MSPROAL_SYSTEM_ERR;
    }

#if         (1 == MSPROAL_SUPPORT_XC)
    /* Change the power class into the preserved value */
    pwrcls = msifhndl->PowerClass;
    if(0 != pwrcls) {
        return msproal_seq_pro_change_power(msifhndl, pwrcls);
    }
#endif  /*  (1 == MSPROAL_SUPPORT_XC)   */

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_drv_pro_stop
*   DESCRIPTION : Stop the command under execution.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_stop(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_stop(MSIFHNDL *msifhndl)
{
    SINT    result;
    UBYTE   intreg;

    /* Issuing of GET_INT TPC */
    result = msproal_tpc_get_int(msifhndl, &intreg);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* BREQ=1? */
    if(intreg & MS_INT_BREQ) {
        /* Issue STOP command */
        result = msproal_msif_set_cmd(  msifhndl,
                                        MS2_CMD_STOP,
                                        MS2_TIMEOUT_STOP);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }
        }
    }

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_drv_pro_wakeup
*   DESCRIPTION : Memory Stick PRO wakes up from sleep status.
*               (Return to normal power consunmption status.)
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_wakeup(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_drv_pro_wakeup(MSIFHNDL *msifhndl)
{
    SINT    result;
    UBYTE   stts;

    result = msproal_msif_set_cmd(  msifhndl,
                                    MS2_CMD_STOP,
                                    MS2_TIMEOUT_STOP + MS2_TIMEOUT_WAKEUP);
    if(MSPROAL_CMDNK_ERR != result) {
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Read Status Register */
        result = msproal_msif_read_reg( msifhndl,
                                        MS2_STATUS_REG_ADRS,
                                        1,
                                        &stts);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Sleep flag ON? */
        if(stts & MS2_STTS_SL) {
            return MSPROAL_ACCESS_ERR;
        }
    }

    return MSPROAL_OK;
}

#if         ((1 == MSPROAL_SUPPORT_IP) || (2 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of write-in sectors
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_drv_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
#if         (1 == MSPROAL_SUPPORT_PROHG)
    SINT    result, nsize;

    if(0 == size) {
        return MSPROAL_OK;
    }

    if(!(MSPROAL_STICK_2K & msifhndl->Stick)) {
        return msproal_seq_pro_write_data(msifhndl, lba, size, data);
    }

    /* LBA and size is a multiple of 4 ? */
    if(0 == (lba & 0x3)) {
        if(0 == (size & 0x3)) {
            return msproal_seq_pro_write_2k_data(msifhndl, lba, size, data);
        }
    } else {
        /* Size is less than the size from lba to the following address */
        /* which is multiple of 4 ?                                     */
        if((SINT)(4 - (lba & 0x3)) > size) {
            nsize = size;
        } else {
            /* Set the size from lba to the following address which is  */
            /* multiple of 4                                            */
            nsize = 4 - (lba & 0x3);
        }

        result = msproal_seq_pro_write_data(msifhndl, lba, nsize, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
        data    += (nsize << 9);
    }

    /* Writing is possible by the unit of 2048 byte ? */
    if(3 < size) {
        nsize   = (size & ~3);
        result = msproal_seq_pro_write_2k_data(msifhndl, lba, nsize, data);
        if(MSPROAL_OK != result) {
            return result;
        }
        lba     += nsize;
        size    -= nsize;
        data    += (nsize << 9);
    }

    /* There is one or more sectors that have not been written yet ? */
    if(0 < size) {
        result = msproal_seq_pro_write_data(msifhndl, lba, size, data);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
#else   /*  (0 == MSPROAL_SUPPORT_PROHG)    */
    if(0 == size) {
        return MSPROAL_OK;
    }

    return msproal_seq_pro_write_data(msifhndl, lba, size, data);
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (2 == MSPROAL_SUPPORT_IP))    */

#if         ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of write-in sectors
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_drv_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    if(0 == size) {
        return MSPROAL_OK;
    }

    return msproal_seq_pro_write_data(msifhndl, lba, size, data);
}
#endif  /*  ((3 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_drv_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_drv_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number
*       size        : The number of write-in sectors
*       data        : Address to area where data for write is stored
******************************************************************************/
SINT msproal_drv_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
#if         (1 == MSPROAL_SUPPORT_PROHG)
    ULONG   *lltbl, *cp_lltbl;
    ULONG   idadrsreg, idcnfreg;
    SINT    result, nsize;
    SINT    trans_size, trans_cnt;

    if(0 == size) {
        return MSPROAL_OK;
    }

    if(!(MSPROAL_STICK_2K & msifhndl->Stick)) {
        return msproal_seq_pro_write_data(msifhndl, lba, size, data);
    }

    cp_lltbl        = (ULONG *)data;
    idadrsreg       = 0;
    idcnfreg        = 0;
    /* LBA and size is a multiple of 4 ? */
    if(0 == (lba & 0x3)) {
        if(0 == (size & 0x3)) {
            return msproal_seq_pro_write_2k_data(msifhndl, lba, size, data);
        }
    } else {
        /* Size is less than the size from lba to the following address */
        /* which is multiple of 4 ?                                     */
        if((SINT)(4 - (lba & 0x3)) > size) {
            nsize = size;
        } else {
            /* Set the size from lba to the following address which is  */
            /* multiple of 4                                            */
            nsize = 4 - (lba & 0x3);
        }

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        trans_size  = nsize * 512;

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

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
        msproal_user_flush_cache((void *)lltbl, 512 * 4);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        result = msproal_seq_pro_write_data(msifhndl,
                                            lba,
                                            nsize,
                                            (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
    }

    /* Writing is possible by the unit of 2048 byte ? */
    if(3 < size) {
        nsize = (size & ~3);
#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        trans_size  = nsize * 512;

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

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
        msproal_user_flush_cache((void *)lltbl, 512 * 4);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        result = msproal_seq_pro_write_2k_data( msifhndl,
                                                lba,
                                                nsize,
                                                (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }

        lba     += nsize;
        size    -= nsize;
    }

    /* There is one or more sectors that have not been written yet ? */
    if(0 < size) {
#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        trans_size  = size * 512;

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
                trans_size  = 0;
            }

            lltbl       += 3;
            cp_lltbl    += 3;
        }

#if         (1 == MSPROAL_SUPPORT_VMEM)
        lltbl       = (ULONG *)&msifhndl->WorkArea[0];
        msproal_user_flush_cache((void *)lltbl, 512 * 4);
#else   /*  (1 == MSPROAL_SUPPORT_VMEM) */
        lltbl       = (ULONG *)&msifhndl->DataBuf[0];
        msproal_user_flush_cache((void *)lltbl, 512);
#endif  /*  (1 == MSPROAL_SUPPORT_VMEM) */
        result = msproal_seq_pro_write_data(msifhndl,
                                            lba,
                                            size,
                                            (UBYTE *)lltbl);
        if(MSPROAL_OK != result) {
            return result;
        }
    }

    return MSPROAL_OK;
#else   /*  (0 == MSPROAL_SUPPORT_PROHG)    */
    if(0 == size) {
        return MSPROAL_OK;
    }

    return msproal_seq_pro_write_data(msifhndl, lba, size, data);
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

/******************************************************************************
*   FUNCTION    : msproal_seq_pro_change_power
*   DESCRIPTION : Change the power class of Memory Stick XC.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_change_power(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       mode        : Power class mode
******************************************************************************/
SINT msproal_seq_pro_change_power(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result;

    result = msproal_msif_ex_set_cmd(   msifhndl,
                                        MS2_CMD_CHG_POWER_CLS,
                                        0x00,
                                        (UWORD)mode,
                                        MS2_TIMEOUT_CHG_POWER_CLS);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            return result;
        }

        return MSPROAL_PARAM_ERR;
    }

    msifhndl->PowerClass = mode;

    return MSPROAL_OK;
}

/******************************************************************************
*   FUNCTION    : msproal_seq_pro_erase
*   DESCRIPTION : Erase specified sector size from specified sector
*               of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_erase(MSIFHNDL *msifhndl, ULONG lba, SINT size)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : Start sector number to erase
*       size        : The number of sectors to erase
******************************************************************************/
SINT msproal_seq_pro_erase(MSIFHNDL *msifhndl, ULONG lba, SINT size)
{
    SINT    result;
    UBYTE   *data;
    UBYTE   tpcparam, intreg;

    /* Is size less than 1? */
    if(1 > size) {
        return MSPROAL_PARAM_ERR;
    }

    /* The data size of READ_SHORT_DATA is specified as 32bytes */
    tpcparam = 0x00;
    result = msproal_msif_write_reg(msifhndl,
                                    MS2_TPC_PARAM_ADRS,
                                    1,
                                    &tpcparam);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_tpc_ex_set_cmd(msifhndl, MS2_CMD_ERASE, lba, (UWORD)size);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Wait for execution result */
    result = msproal_msif_get_int(msifhndl, MS2_TIMEOUT_ERASE, &intreg);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            return MSPROAL_ACCESS_ERR;
        }

        return MSPROAL_PARAM_ERR;
    }

    data = msifhndl->WorkArea;
    while(intreg & MS2_INT_BREQ) {
        /* Read Progress Status */
        result = msproal_tpc_read_short_data(msifhndl, 0x20, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Wait for execution result */
        result = msproal_msif_get_int(msifhndl, MS2_TIMEOUT_ERASE, &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            return MSPROAL_ACCESS_ERR;
        }
    }

    return MSPROAL_OK;
}

#if         (5 != MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_format
*   DESCRIPTION : Format Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_format(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Mode(MSPROAL_QUICK_FORMAT/MSPROAL_FULL_FORMAT)
******************************************************************************/
SINT msproal_seq_pro_format(MSIFHNDL *msifhndl, SINT mode)
{
    SINT    result;
    UBYTE   *data;
    UBYTE   tpcparam, intreg;

    /* The data size of READ_SHORT_DATA is specified as 32bytes */
    tpcparam = 0x00;
    result = msproal_msif_write_reg(msifhndl,
                                    MS2_TPC_PARAM_ADRS,
                                    1,
                                    &tpcparam);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MSPROAL_QUICK_FORMAT == mode) {
        mode = 0;
    } else {
        mode = 1;
    }
    result = msproal_tpc_ex_set_cmd(msifhndl,
                                    MS2_CMD_FORMAT,
                                    0x00,
                                    (UWORD)mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Wait for execution result */
    result = msproal_msif_get_int(msifhndl, MS2_TIMEOUT_FORMAT, &intreg);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }
        }

        return MSPROAL_ACCESS_ERR;
    }

    data = msifhndl->WorkArea;
    while(intreg & MS2_INT_BREQ) {
        /* Read Progress Status */
        result = msproal_tpc_read_short_data(msifhndl, 0x20, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Wait for execution result */
        result = msproal_msif_get_int(msifhndl, MS2_TIMEOUT_FORMAT, &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_ACCESS_ERR != result) {
                if(MSPROAL_FLASH_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            if(msifhndl->IntState & (MSIF_STTS_CRC | MSIF_STTS_TOE)) {
                return result;
            }

            if(*(ULONG *)&data[0] != *(ULONG *)&data[4]) {
                return MSPROAL_ACCESS_ERR;
            }

            result = msproal_drv_pro_recovery(msifhndl);
            if(MSPROAL_OK != result) {
                return result;
            }

            break;
        }
    }

    data = &msifhndl->WorkArea[512];
    result = msproal_seq_pro_read_data(msifhndl, 0, 1, data);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    msifhndl->HidSct = MAKELONG(MAKEWORD(data[0x1C7], data[0x1C6]),
                                MAKEWORD(data[0x1C9], data[0x1C8]));

    return MSPROAL_OK;
}
#endif  /*  (5 != MSPROAL_SUPPORT_IP)   */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_format
*   DESCRIPTION : Format Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_format(MSIFHNDL *msifhndl, SINT mode)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_FORMAT_ERR          : Data format error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       mode        : Mode(MSPROAL_QUICK_FORMAT/MSPROAL_FULL_FORMAT)
******************************************************************************/
SINT msproal_seq_pro_format(MSIFHNDL *msifhndl, SINT mode)
{
    ULONG   pbuf_lltbl[3];
    SINT    result;
    UBYTE   *data;
    UBYTE   tpcparam, intreg;

    /* The data size of READ_SHORT_DATA is specified as 32bytes */
    tpcparam = 0x00;
    result = msproal_msif_write_reg(msifhndl,
                                    MS2_TPC_PARAM_ADRS,
                                    1,
                                    &tpcparam);
    if(MSPROAL_OK != result) {
        return result;
    }

    if(MSPROAL_QUICK_FORMAT == mode) {
        mode = 0;
    } else {
        mode = 1;
    }
    result = msproal_tpc_ex_set_cmd(msifhndl,
                                    MS2_CMD_FORMAT,
                                    0x00,
                                    (UWORD)mode);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Wait for execution result */
    result = msproal_msif_get_int(msifhndl, MS2_TIMEOUT_FORMAT, &intreg);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }
        }

        return MSPROAL_ACCESS_ERR;
    }

    data = msifhndl->WorkArea;
    while(intreg & MS2_INT_BREQ) {
        /* Read Progress Status */
        result = msproal_tpc_read_short_data(msifhndl, 0x20, data);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Wait for execution result */
        result = msproal_msif_get_int(msifhndl, MS2_TIMEOUT_FORMAT, &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_ACCESS_ERR != result) {
                if(MSPROAL_FLASH_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            if(msifhndl->IntState & (MSIF_STTS_CRC | MSIF_STTS_TOE)) {
                return result;
            }

            if(*(ULONG *)&data[0] != *(ULONG *)&data[4]) {
                return MSPROAL_ACCESS_ERR;
            }

            result = msproal_drv_pro_recovery(msifhndl);
            if(MSPROAL_OK != result) {
                return result;
            }

            break;
        }
    }

    data = &msifhndl->WorkArea[512];
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
    result = msproal_seq_pro_read_data(msifhndl, 0, 1, (UBYTE *)pbuf_lltbl);
    if(MSPROAL_OK != result) {
        if(MSPROAL_READ_ERR != result) {
            return result;
        }

        return MSPROAL_FORMAT_ERR;
    }

    msifhndl->HidSct = MAKELONG(MAKEWORD(data[0x1C7], data[0x1C6]),
                                MAKEWORD(data[0x1C9], data[0x1C8]));

    return MSPROAL_OK;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)   */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_atrb
*   DESCRIPTION : Read the attribute information of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lsct        : Sector number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
        UBYTE *data)
{
    SINT    result;
    UBYTE   *pdata, *pend, intreg;

    /* Is size less than 1? */
    if(1 > size) {
        return MSPROAL_PARAM_ERR;
    }

    result = msproal_msif_ex_set_cmd(   msifhndl,
                                        MS2_CMD_READ_ATRB,
                                        lsct,
                                        (UWORD)size,
                                        MS2_TIMEOUT_READ_ATRB);
                                  
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            /* Issue STOP command */
            result = msproal_msif_ex_set_cmd(   msifhndl,
                                                MS2_CMD_STOP,
                                                0,
                                                0,
                                                MS2_TIMEOUT_STOP);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result){
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            return MSPROAL_READ_ERR;
        }

        return MSPROAL_PARAM_ERR;
    }

    pdata   = data;
    pend    = pdata + (size << 9);
    while(pdata < pend) {
        /* 1 PageData(= 512 bytes) is read */
        if(MSPROAL_OK != (result = msproal_tpc_read_page(msifhndl, pdata))) {
            return result;
        }

        /* Wait for execution result */
        result = msproal_msif_get_int(  msifhndl,
                                        MS2_TIMEOUT_READ_ATRB,
                                        &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            /* Issue STOP command */
            result = msproal_msif_ex_set_cmd(   msifhndl,
                                                MS2_CMD_STOP,
                                                0,
                                                0,
                                                MS2_TIMEOUT_STOP);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            return MSPROAL_READ_ERR;
        }

        pdata += 512;
    }

    return MSPROAL_OK;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (2 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_atrb
*   DESCRIPTION : Read the attribute information of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lsct        : Sector number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_ATRB_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_atrb;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_ATRB_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_atrb;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    isysreg |= ICON_SYS_DMASL_PB;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_ATRB);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lsct)));

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

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK == result) {
        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_DMA,
                                        MSPROAL_WDMA);
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (2 == MSPROAL_SUPPORT_IP)       */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_atrb
*   DESCRIPTION : Read the attribute information of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lsct        : Sector number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
        UBYTE *data)
{
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_ATRB_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_atrb;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_ATRB_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_atrb;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_DMASL;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_ATRB);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lsct)));

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

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK == result) {
        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_DMA,
                                        MSPROAL_WDMA);
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)       */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_atrb
*   DESCRIPTION : Read the attribute information of Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lsct        : Sector number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_atrb(MSIFHNDL *msifhndl, ULONG lsct, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_ATRB_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_atrb;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_ATRB_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_atrb;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_ATRB);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lsct)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lsct)));

    result = msproal_icon_start_dma(msifhndl,
                                    (ULONG *)data,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);

    msproal_icon_end_dma(msifhndl);

    return result;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result;
    UBYTE   *pdata, *pend, intreg;

    /* Is size less than 1? */
    if(1 > size) {
        return MSPROAL_PARAM_ERR;
    }

    result = msproal_msif_ex_set_cmd(   msifhndl,
                                        MS2_CMD_READ_DATA,
                                        lba,
                                        (UWORD)size,
                                        MS2_TIMEOUT_READ_DATA);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            /* Issue STOP command */
            result = msproal_msif_ex_set_cmd(   msifhndl,
                                                MS2_CMD_STOP,
                                                0,
                                                0,
                                                MS2_TIMEOUT_STOP);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            return MSPROAL_READ_ERR;
        }

        return MSPROAL_PARAM_ERR;
    }

    pdata   = data;
    pend    = pdata + (size << 9);
    while(pdata < pend) {
        /* 1 PageData(= 512 bytes) is read */
        if(MSPROAL_OK != (result = msproal_tpc_read_page(msifhndl, pdata))) {
            return result;
        }

        /* Wait for execution result */
        result = msproal_msif_get_int(  msifhndl,
                                        MS2_TIMEOUT_READ_DATA+70, //T4-001 2-4-1
                                        &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            /* Issue STOP command */
            result = msproal_msif_ex_set_cmd(   msifhndl,
                                                MS2_CMD_STOP,
                                                0,
                                                0,
                                                MS2_TIMEOUT_STOP);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            return MSPROAL_READ_ERR;
        }

        pdata += 512;
    }

    return MSPROAL_OK;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (2 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    isysreg |= ICON_SYS_DMASL_PB;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

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

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK == result) {
        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_DMA,
                                        MSPROAL_WDMA);
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (2 == MSPROAL_SUPPORT_IP)       */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_DMASL;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

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

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK == result) {
        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_DMA,
                                        MSPROAL_WDMA);
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)       */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    result = msproal_icon_start_dma(msifhndl,
                                    (ULONG *)data,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);

    msproal_icon_end_dma(msifhndl);

    return result;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

#if         (1 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_2k_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result;
    UBYTE   *pdata, *pend, intreg;

    /* Is size less than 1? */
    if(1 > size) {
        return MSPROAL_PARAM_ERR;
    }

    result = msproal_msif_ex_set_cmd(   msifhndl,
                                        MS2_CMD_READ_2K_DATA,
                                        lba,
                                        (UWORD)size,
                                        MS2_TIMEOUT_READ_2K_DATA);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            /* Issue STOP command */
            result = msproal_msif_ex_set_cmd(   msifhndl,
                                                MS2_CMD_STOP,
                                                0,
                                                0,
                                                MS2_TIMEOUT_STOP);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            return MSPROAL_READ_ERR;
        }

        return MSPROAL_PARAM_ERR;
    }

    pdata   = data;
    pend    = pdata + (size << 9);
    while(pdata < pend) {
        /* 4 PageData(= 2048 bytes) is read */
        result = msproal_tpc_read_quad_long_data( msifhndl, pdata);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Wait for execution result */
        result = msproal_msif_get_int(  msifhndl,
                                        MS2_TIMEOUT_READ_2K_DATA,
                                        &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            /* Issue STOP command */
            result = msproal_msif_ex_set_cmd(   msifhndl,
                                                MS2_CMD_STOP,
                                                0,
                                                0,
                                                MS2_TIMEOUT_STOP);
            if(MSPROAL_OK != result) {
                if(MSPROAL_CMDNK_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            return MSPROAL_READ_ERR;
        }

        pdata += 2048;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  (1 == MSPROAL_SUPPORT_IP)       */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_2k_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_2k_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_2k_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    isysreg |= ICON_SYS_DMASL_PB;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 2 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    imctlreg |= ICON_MEM_CTRL_PBBC_1K;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_2K_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

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

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK == result) {
        result = msproal_icon_wait_int( msifhndl,
                                        MSPROAL_TIMEOUT_DMA,
                                        MSPROAL_WDMA);
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)       */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_read_2k_data
*   DESCRIPTION : Read data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_read_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_READ_ERR            : Read error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where read starts
*       size        : The number of sectors to be read
*       data        : Address where read data is stored
******************************************************************************/
SINT msproal_seq_pro_read_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_READ_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_read_2k_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_READ_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_read_2k_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be read */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_NOWRITE));

    /* Set PBBC to 2 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg &= ~ICON_MEM_CTRL_PBBC_MASK;
    imctlreg |= ICON_MEM_CTRL_PBBC_1K;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_READ_2K_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    result = msproal_icon_start_dma(msifhndl,
                                    (ULONG *)data,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);

    msproal_icon_end_dma(msifhndl);

    return result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

/******************************************************************************
*   FUNCTION    : msproal_seq_pro_sleep
*   DESCRIPTION : Memory Stick PRO is put to sleep status.
*               (Shifts to low power consumption status.)
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_sleep(MSIFHNDL *msifhndl)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
******************************************************************************/
SINT msproal_seq_pro_sleep(MSIFHNDL *msifhndl)
{
    SINT    result;

    result = msproal_msif_set_cmd(  msifhndl,
                                    MS2_CMD_SLEEP,
                                    MS2_TIMEOUT_SLEEP);
    if(MSPROAL_CMDNK_ERR != result) {
        return result;
    }

    return MSPROAL_ACCESS_ERR;
}

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_startup
*   DESCRIPTION : Confirm the startup of Memory Stick
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
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
SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
{
    SINT    result;
    SINT    cnt;
    UBYTE   intreg;

    for(cnt = 0; cnt < 1000; cnt ++) {
        /* Waiting for 10ms */
        result = msproal_tpc_wait_int(msifhndl, 10, MSPROAL_WTIME);
        if(MSPROAL_OK != result) {
            return result;
        }

        /* Issuing of GET_INT TPC */
        if(MSPROAL_OK != (result = msproal_tpc_get_int1(msifhndl, &intreg))) {
            return result;
        }

        /* CED=0? */
        if(!(intreg & MS2_INT_CED)) {
            continue;
        }

        /* Issuing of GET_INT TPC */
        if(MSPROAL_OK != (result = msproal_tpc_get_int1(msifhndl, &intreg))) {
            return result;
        }

        /* ERR=1? */
        if(intreg & MS2_INT_ERR) {
            /* CMDNK=0? */
            if(!(intreg & MS2_INT_CMDNK)) {
                return MSPROAL_MEDIA_ERR;
            }

            /* Switching to write-protect mode */
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }

        return MSPROAL_OK;
    }

    return MSPROAL_MEDIA_ERR;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (2 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_startup
*   DESCRIPTION : Confirm the startup of Memory Stick
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
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
SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
{
    ULONG   pgbf;
    SINT    result;
    UWORD   sysreg, isysreg;

    /* Set HOST MSIEN to 0 */
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &sysreg);
    sysreg  &= ~MSIF_SYS_MSIEN;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), sysreg);

    /* Set PDIR to 0, GRPN to 3 */
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                ICON_CTRL_GRPN_NOWRITE);

    /* set DMAE1 to 0 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMAE1;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* load microcode */
    result  = msproal_icon_load_mc( msifhndl,
                                    0,
                                    MSPROAL_MC_SEQ_PRO_STARTUP_LEN,
                                    (UWORD *)msproal_mc_seq_pro_startup);
    if(MSPROAL_OK != result) {
        return result;
    }

    result  = msproal_icon_exec_mc(msifhndl, 0);

    /* Set HOST MSIEN to 1 */
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &sysreg);
    sysreg  |= MSIF_SYS_MSIEN;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), sysreg);

    if(MSPROAL_OK == result) {
        msproal_user_read_mem32(ICON_PBUFF(msifhndl->BaseAdrs), &pgbf);
        if(MSPROAL_READ_WRITE != (((UBYTE *)&pgbf)[0])) {
            msifhndl->Rw    = MSPROAL_READ_ONLY;
        }

        /* set DMAE1 to 1 */
        msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
        isysreg |= ICON_SYS_DMAE1;
        msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);
    }

    return result;
}
#endif  /*  (2 == MSPROAL_SUPPORT_IP)       */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_startup
*   DESCRIPTION : Confirm the startup of Memory Stick
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
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
SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
{
    ULONG   pgbf;
    SINT    result;
    UWORD   sysreg;

    /* Set HOST MSIEN to 0 */
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &sysreg);
    sysreg  &= ~MSIF_SYS_MSIEN;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), sysreg);

    /* Set PDIR to 0, GRPN to 3 */
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                ICON_CTRL_GRPN_NOWRITE);

    /* load microcode */
    result  = msproal_icon_load_mc( msifhndl,
                                    0,
                                    MSPROAL_MC_SEQ_PRO_STARTUP_LEN,
                                    (UWORD *)msproal_mc_seq_pro_startup);
    if(MSPROAL_OK != result) {
        return result;
    }

    result  = msproal_icon_exec_mc(msifhndl, 0);

    /* Set HOST MSIEN to 1 */
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &sysreg);
    sysreg  |= MSIF_SYS_MSIEN;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), sysreg);

    if(MSPROAL_OK == result) {
        msproal_user_read_mem32(ICON_PBUFF(msifhndl->BaseAdrs), &pgbf);
        if(MSPROAL_READ_WRITE != (((UBYTE *)&pgbf)[0])) {
            msifhndl->Rw    = MSPROAL_READ_ONLY;
        }
    }

    return result;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)       */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_startup
*   DESCRIPTION : Confirm the startup of Memory Stick
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
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
SINT msproal_seq_pro_startup(MSIFHNDL *msifhndl)
{
    ULONG   pgbf;
    SINT    result;
    UWORD   sysreg;

    /* Set HOST MSIEN to 0 */
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &sysreg);
    sysreg  &= ~MSIF_SYS_MSIEN;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), sysreg);

    /* Set PDIR to 0, GRPN to 3 */
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                ICON_CTRL_GRPN_NOWRITE);

    /* load microcode */
    result  = msproal_icon_load_mc( msifhndl,
                                    0,
                                    MSPROAL_MC_SEQ_PRO_STARTUP_LEN,
                                    (UWORD *)msproal_mc_seq_pro_startup);
    if(MSPROAL_OK != result) {
        return result;
    }

    result  = msproal_icon_exec_mc(msifhndl, 0);

    /* Set HOST MSIEN to 1 */
    msproal_user_read_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), &sysreg);
    sysreg  |= MSIF_SYS_MSIEN;
    msproal_user_write_mem16(MSIF_SYS_ADRS(msifhndl->BaseAdrs), sysreg);

    if(MSPROAL_OK == result) {
        msproal_user_read_mem32(ICON_PBUFF(msifhndl->BaseAdrs), &pgbf);
        if(MSPROAL_READ_WRITE != (((UBYTE *)&pgbf)[0])) {
            msifhndl->Rw    = MSPROAL_READ_ONLY;
        }
    }

    return result;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result, time;
    UBYTE   *pdata, *pend;
    UBYTE   intreg;

    if(1 > size) {
        return MSPROAL_PARAM_ERR;
    }

    result = msproal_tpc_ex_set_cmd(msifhndl,
                                    MS2_CMD_WRITE_DATA,
                                    lba,
                                    (UWORD)size);
    if(MSPROAL_OK != result) {
        return result;
    }

    time    = MS2_TIMEOUT_WRITE_DATA;
    pdata   = data;
    pend    = pdata + (size << 9);
    while(pdata < pend) {
        /* Wait for execution result */
        result = msproal_msif_get_int(  msifhndl,
                                        time,
                                        &intreg);
        if(MSPROAL_OK != result) {
        	
        	if (MSPROAL_CMDNK_ERR == result) {//write disable ========== TEST# : T9-002 ==========
    			return result;
    		}
            if(MSPROAL_CMDNK_ERR != result) {
                if(MSPROAL_FLASH_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            if(intreg & MS2_INT_ERR) {
                msifhndl->Rw = MSPROAL_READ_ONLY;
                return MSPROAL_WRITE_ERR;
            }

            return MSPROAL_PARAM_ERR;
        }

        /* 1 PageData(= 512 bytes) is written */
        if(MSPROAL_OK != (result = msproal_tpc_write_page(msifhndl, pdata))) {
            return result;
        }

        pdata += 512;
    }

    /* Wait for execution result */
    result = msproal_msif_get_int(  msifhndl,
                                    time,
                                    &intreg);
    if(MSPROAL_OK != result) {
    	
    	if (MSPROAL_CMDNK_ERR == result) {//write disable ========== TEST# : T9-002 ==========
    		return result;
    	}
    	
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            return MSPROAL_ACCESS_ERR;
        }

        if(intreg & MS2_INT_ERR) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
            return MSPROAL_WRITE_ERR;
        }

        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */

#if         (2 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_WRITE_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_write_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_WRITE_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_write_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be written */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_R0
                                    | ICON_CTRL_PDIR_CPU_TO_MS));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    isysreg |= ICON_SYS_DMASL_PB;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg    &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_WRITE_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        if(MSPROAL_WRITE_ERR == result) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (2 == MSPROAL_SUPPORT_IP)       */

#if         (4 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_WRITE_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_write_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_WRITE_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_write_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be written */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_R0
                                    | ICON_CTRL_PDIR_CPU_TO_MS));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg |= ICON_SYS_DMASL;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_WRITE_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        if(MSPROAL_WRITE_ERR == result) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (4 == MSPROAL_SUPPORT_IP)       */

#if         (5 == MSPROAL_SUPPORT_IP)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
*           UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_WRITE_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_write_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_WRITE_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_write_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be written */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_R0
                                    | ICON_CTRL_PDIR_CPU_TO_MS));

    /* Set PBBC to 0 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg    &= ~ICON_MEM_CTRL_PBBC_MASK;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_WRITE_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    result = msproal_icon_start_dma(msifhndl,
                                    (ULONG *)data,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        if(MSPROAL_WRITE_ERR == result) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }
    }
    msproal_icon_end_dma(msifhndl);

    return result;
}
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */

#if         (1 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_2k_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_2k_data(MSIFHNDL *msifhndl, ULONG lba,
*           SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    SINT    result, time;
    UBYTE   *pdata, *pend;
    UBYTE   intreg;

    if(1 > size) {
        return MSPROAL_PARAM_ERR;
    }

    result = msproal_tpc_ex_set_cmd(msifhndl,
                                    MS2_CMD_WRITE_2K_DATA,
                                    lba,
                                    (UWORD)size);
    if(MSPROAL_OK != result) {
        return result;
    }

    time    = MS2_TIMEOUT_WRITE_2K_DATA;
    pdata   = data;
    pend    = pdata + (size << 9);
    while(pdata < pend) {
        /* Wait for execution result */
        result = msproal_msif_get_int(  msifhndl,
                                        time,
                                        &intreg);
        if(MSPROAL_OK != result) {
            if(MSPROAL_CMDNK_ERR != result) {
                if(MSPROAL_FLASH_ERR != result) {
                    return result;
                }

                return MSPROAL_ACCESS_ERR;
            }

            if(intreg & MS2_INT_ERR) {
                msifhndl->Rw = MSPROAL_READ_ONLY;
                return MSPROAL_WRITE_ERR;
            }

            return MSPROAL_PARAM_ERR;
        }

        /* 4 PageData(= 2048 bytes) is written */
        result = msproal_tpc_write_quad_long_data(msifhndl, pdata);
        if(MSPROAL_OK != result) {
            return result;
        }

        pdata += 2048;
    }

    /* Wait for execution result */
    result = msproal_msif_get_int(  msifhndl,
                                    time,
                                    &intreg);
    if(MSPROAL_OK != result) {
        if(MSPROAL_CMDNK_ERR != result) {
            if(MSPROAL_FLASH_ERR != result) {
                return result;
            }

            return MSPROAL_ACCESS_ERR;
        }

        if(intreg & MS2_INT_ERR) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
            return MSPROAL_WRITE_ERR;
        }

        return MSPROAL_PARAM_ERR;
    }

    return MSPROAL_OK;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  (1 == MSPROAL_SUPPORT_IP)       */

#if         (2 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_2k_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_2k_data(MSIFHNDL *msifhndl, ULONG lba,
*           SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;
    UWORD   isysreg;
    UBYTE   *dst, *src;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_WRITE_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_write_2k_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_WRITE_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_write_2k_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be written */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_R0
                                    | ICON_CTRL_PDIR_CPU_TO_MS));

    /* Set DMASL to 1 */
    msproal_user_read_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), &isysreg);
    isysreg &= ~ICON_SYS_DMASL_MASK;
    isysreg |= ICON_SYS_DMASL_PB;
    msproal_user_write_mem16(ICON_SYS_REG(msifhndl->BaseAdrs), isysreg);

    /* Set PBBC to 2 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg    &= ~ICON_MEM_CTRL_PBBC_MASK;
    imctlreg    |= ICON_MEM_CTRL_PBBC_1K;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_WRITE_2K_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    msproal_user_virt_to_bus((void *)data, (ULONG *)&src);
    msproal_user_virt_to_bus(   (void *)ICON_PBUFF(msifhndl->BaseAdrs),
                                (ULONG *)&dst);
    msproal_user_flush_cache((void *)data, size << 9);
    result = msproal_user_start_dma(MSPROAL_INC_SADR,
                                    (void *)src,
                                    (void *)dst,
                                    size << 9,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        if(MSPROAL_WRITE_ERR == result) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }
    }
    msproal_user_end_dma();

    return result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  (2 == MSPROAL_SUPPORT_IP)       */

#if         (5 == MSPROAL_SUPPORT_IP)
#if         (1 == MSPROAL_SUPPORT_PROHG)
/******************************************************************************
*   FUNCTION    : msproal_seq_pro_write_2k_data
*   DESCRIPTION : Write data of specified sector size from specified sector of
*               Memory Stick PRO.
*------------------------------------------------------------------------------
*   SINT msproal_seq_pro_write_2k_data(MSIFHNDL *msifhndl, ULONG lba,
*           SINT size, UBYTE *data)
*   RETURN
*       MSPROAL_OK                  : Normal
*       MSPROAL_PARAM_ERR           : Parameter error
*       MSPROAL_ACCESS_ERR          : Access error
*       MSPROAL_SYSTEM_ERR          : System error
*       MSPROAL_WRITE_ERR           : Write error
*       MSPROAL_EXTRACT_ERR         : Media extract
*   ARGUMENT
*       msifhndl    : Address to initialized MSIFHNDL type
*       lba         : LBA number where write starts
*       size        : The number of sectors to be written
*       data        : Address where data for write is stored
******************************************************************************/
SINT msproal_seq_pro_write_2k_data(MSIFHNDL *msifhndl, ULONG lba, SINT size,
        UBYTE *data)
{
    ULONG   imctlreg;
    SINT    result;
    SINT    mcsize;
    UWORD   *mcdata;

    if(size < 1) {
        return MSPROAL_PARAM_ERR;
    }

    /* load microcode */
    if(MSPROAL_SERIAL_MODE == msifhndl->IfMode) {
        mcsize  = MSPROAL_MC_SEQ_PRO_S_WRITE_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_s_write_2k_data;
    } else {
        mcsize  = MSPROAL_MC_SEQ_PRO_P_WRITE_2K_DATA_LEN;
        mcdata  = (UWORD *)msproal_mc_seq_pro_p_write_2k_data;
    }
    result = msproal_icon_load_mc(msifhndl, 0, mcsize, mcdata);
    if(MSPROAL_OK != result) {
        return result;
    }

    /* Set the number of sectors to be written */
    msproal_user_write_mem16(ICON_GEN_REG0(msifhndl->BaseAdrs), (UWORD)size);
    msproal_user_write_mem16(   ICON_CTRL_REG(msifhndl->BaseAdrs),
                                (   ICON_CTRL_GDIR_CPU_TO_MS
                                    | ICON_CTRL_GRPN_R0
                                    | ICON_CTRL_PDIR_CPU_TO_MS));

    /* Set PBBC to 2 */
    msproal_user_read_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), &imctlreg);
    imctlreg    &= ~ICON_MEM_CTRL_PBBC_MASK;
    imctlreg    |= ICON_MEM_CTRL_PBBC_1K;
    msproal_user_write_mem32(ICON_MEM_CTRL_REG(msifhndl->BaseAdrs), imctlreg);

    /* Store command and parameters which EX_SET_CMD uses */
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            MS2_CMD_WRITE_2K_DATA);
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(size));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(HIWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            HIBYTE(LOWORD(lba)));
    msproal_user_write_mem8(ICON_GDFIFO(msifhndl->BaseAdrs),
                            LOBYTE(LOWORD(lba)));

    result = msproal_icon_start_dma(msifhndl,
                                    (ULONG *)data,
                                    MSPROAL_SELECT_PBUFF);
    if(MSPROAL_OK != result) {
        return result;
    }

    result = msproal_icon_exec_mc(msifhndl, 0);
    if(MSPROAL_OK != result) {
        if(MSPROAL_WRITE_ERR == result) {
            msifhndl->Rw = MSPROAL_READ_ONLY;
        }
    }
    msproal_icon_end_dma(msifhndl);

    return result;
}
#endif  /*  (1 == MSPROAL_SUPPORT_PROHG)    */
#endif  /*  (5 == MSPROAL_SUPPORT_IP)       */
