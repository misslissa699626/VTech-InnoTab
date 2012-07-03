#ifndef _GP_HAL_BCH_H_
#define _GP_HAL_BCH_H_
#include "hal_nand.h"


extern void bch_mode_set(BCH_MODE_ENUM bch_set);
extern void bch_boot_area_init(void);
extern void bch_boot_area_uninit(void);
extern UINT16 nand_bch_err_bits_get(void);
extern UINT16 bch_get_max_bit_err_cnt(void);

#endif

