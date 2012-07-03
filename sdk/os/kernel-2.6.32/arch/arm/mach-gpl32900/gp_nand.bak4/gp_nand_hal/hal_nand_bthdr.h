#ifndef _DRV_L1_BTHDR_H_
#define _DRV_L1_BTHDR_H_
#include "hal_nand.h"
#include "hal_nand_bthdr_ext.h"

extern SINT32 nf_hdr_get_from_bt_area(UINT8* header);
extern UINT16 iotrap_info_get(UINT8 bit_offset, BIT_NUMS_MASK bit_nums);
extern SINT32 nf_hdr_to_info(UINT8* header, HEADER_NF_INFO *nv_info);
extern SINT16 check_key_status_get(UINT8 *bt_hdr_buf);
extern SINT16 hdr_check_key_count(UINT8 * bt_hdr_buf);
extern UINT16 iotrap_info_get(UINT8 bit_offset, BIT_NUMS_MASK bit_nums);
extern UINT8 hdr_sw_fuse_get(SW_FUSE_ENUM bit_index);
#endif //_DRV_L1_BTHDR_H_




