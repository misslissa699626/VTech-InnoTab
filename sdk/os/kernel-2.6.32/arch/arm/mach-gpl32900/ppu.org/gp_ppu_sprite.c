/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file    gp_ppu.c
 * @brief   Implement of PPU module driver.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */
 
//#include <linux/io.h>
#include <linux/module.h> 
#include <mach/hardware.h>
//#include <linux/sched.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
//#include <linux/hdreg.h> 		/* HDIO_GETGEO */
#include <linux/blkdev.h>
#include <mach/gp_ppu.h>
//#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>
#if PPU_HARDWARE_MODULE == MODULE_ENABLE
#include <mach/hal/hal_ppu.h>
#endif

#if 1
	#define DEBUG	printk
#else
	#define DEBUG(...)
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/
// Define Max sprite ram size and buffer size 
#define SPRITERAM_TYPE_SET           1
#define Spritebuf_number             1024//128
#define Sprite_ptr_number            1024//128

// Define Max V3D buffer size
#define SP25D_MAX_CHAR_V             8
#define SP25D_MAX_CHAR_H             8

// Define Sprite Coordinate Fraction Mode Enable
#define SP_FRACTION_ENABLE           0

// Define sprite constant
#define spirte_pos_max_bit           0x3FF
#define max_sp_combination_number    160
#define data_end_word                0xFFFF
#define hflip_bit                    0x0004
#define vflip_bit                    0x0008

// Define sprite constant
#define spirte_pos_max_bit           0x3FF
#define max_sp_combination_number    160
#define data_end_word                0xFFFF
#define hflip_bit                    0x0004
#define vflip_bit                    0x0008
#define pos_x                        0
#define sp_init_en                   0x8
#define p1024_enable                 0x10
#define spv3d_enable                 0x200
#define pal_control_0                7
#define pal_control_1                15
#define cdm_enable                   0x20000
#define excdm_enable                 0x2
#define sp_frac_x1                   4
#define sp_frac_x2                   8
#define sp_frac_x3                   12
#define exsp_frac_x0                 8
#define exsp_frac_x1                 12
#define sp_frac_y0                   2
#define sp_frac_y1                   6
#define sp_frac_y2                   10
#define sp_frac_y3                   14
#define exsp_frac_y0                 10
#define exsp_frac_y1                 14
#define b_spn_group                  28
#define mask_group_id                0x3
#define sp_cdm_init_en               (0x1<<11)
#define sp_no_cdm_en                 (0x1<<15)
#define b_cdm_exspn_group            12

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
//#define	R_PPU_SPRITE_CTRL					(*((volatile unsigned int *) (IO3_BASE + 0x20108)))//0x93020108
#define cdm_enable                          0x20000
//#define GPSP_CMD_COMPARE()                  (R_PPU_SPRITE_CTRL & cdm_enable)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
// Gobel value
static unsigned int nV_Size,nH_Size,h_size_tmp,v_size_tmp,h_pos_tmp,v_pos_tmp;
static unsigned int SpriteRAM_number;

// Spritebuf and SpriteRAM Memory management
#if 0
FP32 HGridBuff[SP25D_MAX_CHAR_V+1][SP25D_MAX_CHAR_H+1];             //V3D Horizontal grid buffer
FP32 VGridBuff[SP25D_MAX_CHAR_V+1][SP25D_MAX_CHAR_H+1];             //V3D Vertical grid buffer
#else
//static unsigned int HGridBuff_va[SP25D_MAX_CHAR_V+1][SP25D_MAX_CHAR_H+1];             //V3D Horizontal grid buffer
//static unsigned int VGridBuff_va[SP25D_MAX_CHAR_V+1][SP25D_MAX_CHAR_H+1];             //V3D Vertical grid buffer
static unsigned int HGridBuff[SP25D_MAX_CHAR_V+1][SP25D_MAX_CHAR_H+1];             //V3D Horizontal grid buffer
static unsigned int VGridBuff[SP25D_MAX_CHAR_V+1][SP25D_MAX_CHAR_H+1];             //V3D Vertical grid buffer
#endif
//static unsigned int *HGridBuff,*VGridBuff;
//static SPRITE	Spritebuf_va[Spritebuf_number*sizeof(SPRITE)];
//static SpN_ptr Sprite_ptr_va[Sprite_ptr_number*sizeof(SpN_ptr)];
static PPU_REGISTER_SETS *ppu_module_register_set;
static SPRITE *Spritebuf;
static SpN_ptr *Sprite_ptr;
static SpN_RAM *SpriteRAM;
static SpN_EX_RAM *SpriteExRAM;

 /**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

/**
 * @brief 		PPU sprite initial set function.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_init(
PPU_REGISTER_SETS *p_register_set
)
{
	
	if (!p_register_set) {
		return STATUS_FAIL;
	}
	p_register_set->sprite_control = 0;
	p_register_set->sprite_segment = 0;
	p_register_set->extend_sprite_control = 0;
	p_register_set->extend_sprite_addr = 0;

	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

  ppu_module_register_set = p_register_set;
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_init);

static unsigned int GPSP_CMD_COMPARE(void)
{
	if(ppu_module_register_set->sprite_control & cdm_enable)
	  return 1;
	else
	  return 0;
}

/**
 * @brief 		PPU sprite ram initial set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	sprute_number [in]: display sprite number.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_spriteram_init(
PPU_REGISTER_SETS *p_register_set,
unsigned int sprute_number
)
{
#if 0	
	unsigned int temp,temp1;
	
	SpriteRAM_number = sprute_number;
	temp = (unsigned int)gp_chunk_malloc(current->pid,(sprute_number*sizeof(SpN_RAM)));
	SpriteRAM = (SpN_RAM *)temp;
	temp1 = (unsigned int)gp_chunk_pa((unsigned int *)temp);
	#if 1
	  #if CPU_MOVE_MODE == 1
	    gp_ppu_sprite_attribute_ram_set_ptr(p_register_set,temp);
	  #else 
	    gp_ppu_sprite_attribute_ram_set_ptr(p_register_set,temp1);
	  #endif
	#else	
	  p_register_set->ppu_sprite_attribute_ptr = temp1;
  #endif
	
	temp = (unsigned int)gp_chunk_malloc(current->pid,(sprute_number*sizeof(SpN_EX_RAM)));
	//temp1 = (unsigned int)gp_user_va_to_pa((unsigned short *)temp);
	SpriteExRAM =(SpN_EX_RAM *)temp;
	temp1 = (unsigned int)gp_chunk_pa((unsigned int *)temp);
	#if 1
	  #if CPU_MOVE_MODE == 1
	     gp_ppu_sprite_extend_attribute_ram_set_ptr(p_register_set,temp);
	  #else
	     gp_ppu_sprite_extend_attribute_ram_set_ptr(p_register_set,temp1);
	  #endif
	#else
	  p_register_set->ppu_sprite_ex_attribute_ptr = temp;
	#endif
	
	#if 0
	  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)&Spritebuf_va[0]);
	  Spritebuf = (SPRITE *)temp;
	#else
	  Spritebuf = (SPRITE *)&Spritebuf_va[0];
	#endif
	
	#if 0
	  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)&Sprite_ptr_va[0]);
	  Sprite_ptr = (SpN_ptr *)temp;	
	#else
	  Sprite_ptr = (SpN_ptr *)&Sprite_ptr_va[0];	
	#endif
#endif	
/*	
	#if 0
	  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)&HGridBuff_va[0]);
	  HGridBuff = (unsigned int *)temp;	
	#else
	  HGridBuff = (unsigned int *)&HGridBuff_va[0];	
	#endif
	
	#if 0
	  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)&VGridBuff_va[0]);
	  VGridBuff = (unsigned int *)temp;	
	#else
	  VGridBuff = (unsigned int *)&VGridBuff_va[0];	
	#endif		
*/	
	return STATUS_OK;		
}

/**
 * @brief 		PPU sprite function enable set.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->sprite_control |= SP_ENABLE;
	} else {
		p_register_set->sprite_control &= ~SP_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_set_enable);

/**
 * @brief 		PPU sprite coordinate set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=center coordinate 1=top-left coordinate.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_set_coordinate(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Top-Left coordinate
		p_register_set->sprite_control |= SP_COORD1;
	} else {			// Center coordinate
		p_register_set->sprite_control &= ~SP_COORD1;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_set_coordinate);

/**
 * @brief 		PPU sprite blend set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=4 level blending mode 1=16 or 64 level blending mode.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_blend_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Use 64 level blending in sprite attribute ram
		p_register_set->sprite_control |= SP_BLDMODE64_ENABLE;
	} else {			// Use a global 4 level blending value in P_Blending
		p_register_set->sprite_control &= ~SP_BLDMODE64_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_blend_set_mode);

/**
 * @brief 		PPU sprite direct set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=relative address mode 1=direct address mode.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_direct_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Direct address mode
		p_register_set->sprite_control |= SP_DIRECT_ADDRESS_MODE;
	} else {			// Relative address mode
		p_register_set->sprite_control &= ~SP_DIRECT_ADDRESS_MODE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_direct_set_mode);

/**
 * @brief 		PPU sprite zoom set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_zoom_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable zoom function
		p_register_set->sprite_control |= SP_ZOOM_ENABLE;
	} else {			// Disable zoom function
		p_register_set->sprite_control &= ~SP_ZOOM_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_zoom_set_enable);

/**
 * @brief 		PPU sprite rotate set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_rotate_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable rotate function
		p_register_set->sprite_control |= SP_ROTATE_ENABLE;
	} else {			// Disable rotate function
		p_register_set->sprite_control &= ~SP_ROTATE_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_rotate_set_enable);

/**
 * @brief 		PPU sprite masic set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_mosaic_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable mosaic function
		p_register_set->sprite_control |= SP_MOSAIC_ENABLE;
	} else {			// Disable mosaic function
		p_register_set->sprite_control &= ~SP_MOSAIC_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_mosaic_set_enable);

/**
 * @brief 		PPU sprite number set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0(1024 sprites) 1(4 sprites) 2(8 sprites) ... 255(1020 sprites).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_number_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>0xFF) {
		return STATUS_FAIL;
	}

	p_register_set->sprite_control &= ~MASK_SP_NUMBER;
	p_register_set->sprite_control |= (value << B_SP_NUMBER) & MASK_SP_NUMBER;

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_number_set);

/**
 * @brief 		PPU sprite secial effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_special_effect_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable special effect function
		p_register_set->sprite_control |= SP_SPECIAL_EFFECT_ENABLE;
	} else {			// Disable special effect function
		p_register_set->sprite_control &= ~SP_SPECIAL_EFFECT_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_special_effect_set_enable);

/**
 * @brief 		PPU sprite color dither set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_color_dither_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable color dither function
		p_register_set->sprite_control |= SP_COLOR_DITHER_ENABLE;
	} else {			// Disable color dither function
		p_register_set->sprite_control &= ~SP_COLOR_DITHER_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_color_dither_set_mode);

/**
 * @brief 		PPU sprite 25d mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=sprite 2D mode 1= sprite 2.5D mode.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_25d_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_SPR25D_ENABLE;
	} else {
		p_register_set->ppu_enable &= ~PPU_SPR25D_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_25d_set_mode);

/**
 * @brief 		PPU sprite window mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable sprite window function 1=enable sprite window function.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_window_enable_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->ppu_enable |= SPR_WIN_ENABLE;
	} else {
		p_register_set->ppu_enable &= ~SPR_WIN_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_window_enable_set);

/**
 * @brief 		PPU sprite date segment set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
	
signed int 
gp_ppu_sprite_segment_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
  unsigned int temp;
	
	if (!p_register_set || (value&0x1)) {
		return STATUS_FAIL;
	}
  
	temp = (unsigned int)gp_user_va_to_pa((unsigned int *)value);
	p_register_set->sprite_segment = temp;

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_segment_set);

/**
 * @brief 		PPU sprite attribute ram set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to sprite attribute ram.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_ram_set_ptr(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
  unsigned int temp;
  
	if (!p_register_set) {
		return STATUS_FAIL;
	}
  
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
	p_register_set->ppu_sprite_attribute_ptr = temp;

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE_ATTRIBUTE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_ram_set_ptr);

/**
 * @brief 		PPU sprite sfr mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_sfr_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Sfr mode enable
		p_register_set->sprite_control |= SP_FAR_ADDRESS_ENABLE;
	} else {			// Sfr mode disable
		p_register_set->sprite_control &= ~SP_FAR_ADDRESS_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_sfr_set);

/**
 * @brief 		PPU large sprite mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_large_sprite_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Large Size mode enable
		p_register_set->sprite_control |= SP_LARGE_SIZE_ENABLE;
	} else {			// Large Size mode disable
		p_register_set->sprite_control &= ~SP_LARGE_SIZE_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_large_sprite_set);

/**
 * @brief 		PPU sprite interpolation mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_interpolation_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable zoom function
		p_register_set->sprite_control |= SP_INTERPOLATION_ENABLE;
	} else {			// Disable zoom function
		p_register_set->sprite_control &= ~SP_INTERPOLATION_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_interpolation_set);

/**
 * @brief 		PPU sprite group mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_group_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable zoom function
		p_register_set->sprite_control |= SP_GROUP_ENABLE;
	} else {			// Disable zoom function
		p_register_set->sprite_control &= ~SP_GROUP_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_group_set);

/**
 * @brief 		PPU sprite cdm attribute mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_cdm_attribute_set_enable(
SpN_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>1) {
		return STATUS_FAIL;
	}
    sprite_attr++;
	sprite_attr->uPosX_16 = (value << (B_SPN_CDM-1));	
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_cdm_attribute_set_enable);

/**
 * @brief 		PPU sprite fraction mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_fraction_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {		// Enable zoom function
		p_register_set->sprite_control |= SP_FRACTION_COORDINATE_ENABLE;
	} else {			// Disable zoom function
		p_register_set->sprite_control &= ~SP_FRACTION_COORDINATE_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_fraction_set);

/**
 * @brief 		PPU sprite extend attribute mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to sprite exterd attribute ram.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_extend_attribute_ram_set_ptr(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	unsigned int temp;
	
	if (!p_register_set) {
		return STATUS_FAIL;
	}
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
	p_register_set->ppu_sprite_ex_attribute_ptr = temp;

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE_EX_ATTRIBUTE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_extend_attribute_ram_set_ptr);

/**
 * @brief 		PPU exsprite enable set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->extend_sprite_control |= SP_ENABLE;
	} else {
		p_register_set->extend_sprite_control &= ~SP_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_set_enable);

/**
 * @brief 		PPU exsprite cdm set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_cdm_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->extend_sprite_control |= EXSP_CDM_ENABLE;
	} else {
		p_register_set->extend_sprite_control &= ~EXSP_CDM_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_cdm_set_enable);

/**
 * @brief 		PPU exsprite interpolation set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_interpolation_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->extend_sprite_control |= EXSP_INTERPOLATION_ENABLE;
	} else {
		p_register_set->extend_sprite_control &= ~EXSP_INTERPOLATION_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_interpolation_set);

/**
 * @brief 		PPU large exsprite mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_large_size_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->extend_sprite_control |= EXSP_LARGE_SIZE_ENABLE;
	} else {
		p_register_set->extend_sprite_control &= ~EXSP_LARGE_SIZE_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_large_size_set);

/**
 * @brief 		PPU large exsprite group set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_group_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->extend_sprite_control |= EXSP_GROUP_ENABLE;
	} else {
		p_register_set->extend_sprite_control &= ~EXSP_GROUP_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_group_set);

/**
 * @brief 		PPU large exsprite fraction set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_fraction_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return STATUS_FAIL;
	}

	if (value) {
		p_register_set->extend_sprite_control |= EXSP_FRACTION_COORDINATE_ENABLE;
	} else {
		p_register_set->extend_sprite_control &= ~EXSP_FRACTION_COORDINATE_ENABLE;
	}

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_fraction_set);

/**
 * @brief 		PPU large exsprite number set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_number_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>0xFF) {
		return STATUS_FAIL;
	}

	p_register_set->extend_sprite_control &= ~MASK_SP_NUMBER;
	p_register_set->extend_sprite_control |= (value << B_SP_NUMBER) & MASK_SP_NUMBER;

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_number_set);

/**
 * @brief 		PPU large exsprite ram ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to sprite exterd attribute ram.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_start_address_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	unsigned int temp;
	
	if (!p_register_set || (value&0x1)) {
		return STATUS_FAIL;
	}
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
	p_register_set->extend_sprite_addr = temp;

	// Notify PPU driver to update sprite registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_start_address_set);

/**
 * @brief 		PPU 2d sprite position set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	x0 [in]: x0: x0 represent sprite top/left or center position. Only 10-bits are valid.
* @param 	y0 [in]: y0: y0 represent sprite top/left or center position. Only 10-bits are valid.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_2d_position_set(
SpN_RAM *sprite_attr, 
signed short x0, 
signed short y0
)
{
	if (!sprite_attr) {
		return STATUS_FAIL;
	}

	sprite_attr->uPosX_16 &= ~MASK_SPN_POSX;
	sprite_attr->uPosX_16 |= (x0 & MASK_SPN_POSX);
	sprite_attr->uPosY_16 &= ~MASK_SPN_POSY;
	sprite_attr->uPosY_16 |= (y0 & MASK_SPN_POSY);

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_2d_position_set);

/**
 * @brief 		PPU 25d sprite position set function.
* @param 	out [in]: sprite ram struct value set.
* @param 	in [in]: position: osition defines four coordinates of the sprite.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_25d_position_set(
SpN_RAM *out, 
POS_STRUCT_PTR in
)
{
	if (!out || !in) {
		return STATUS_FAIL;
	}

	gpHalPPUsprite25dPosconvert((SpN_RAM *)out,(POS_STRUCT_PTR)in);

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_25d_position_set);

/**
 * @brief 		PPU sprite attribute rotate set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_rotate_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>63) {
		return STATUS_FAIL;
	}
	//printk(KERN_WARNING "value = %d\n", value);
	sprite_attr->uPosX_16 &= ~MASK_SPN_ROTATE;
	sprite_attr->uPosX_16 |= (value << B_SPN_ROTATE) & MASK_SPN_ROTATE;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_rotate_set);

/**
 * @brief 		PPU sprite attribute zoom set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_zoom_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>63) {
		return STATUS_FAIL;
	}

	sprite_attr->uPosY_16 &= ~MASK_SPN_ZOOM;
	sprite_attr->uPosY_16 |= (value << B_SPN_ZOOM) & MASK_SPN_ZOOM;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_zoom_set);

/**
 * @brief 		PPU sprite attribute color set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0(2-bit) 1(4-bit) 2(6-bit) 3(8-bit/5-bit/16-bit/RGBG/YUYV/8+6 blending/RGBA888/ARGB444).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_color_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}

	sprite_attr->attr0 &= ~MASK_SPN_COLOR;
	sprite_attr->attr0 |= (value << B_SPN_COLOR) & MASK_SPN_COLOR;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_color_set);

/**
 * @brief 		PPU sprite attribute flip set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0(No flip) 1(H-flip) 2(V-flip) 3(H+V-flip).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_flip_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}

	sprite_attr->attr0 &= ~MASK_SPN_FLIP;
	sprite_attr->attr0 |= (value << B_SPN_FLIP) & MASK_SPN_FLIP;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_flip_set);

/**
 * @brief 		PPU sprite attribute character size set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	hs [in]: hs :0(8) 1(16) 2(32) 3(64).
* @param 	vs [in]: vs:0(8) 1(16) 2(32) 3(64).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_character_size_set(
SpN_RAM *sprite_attr, 
unsigned int hs, 
unsigned int vs
)
{
	if (!sprite_attr || hs>3 || vs>3) {
		return STATUS_FAIL;
	}

	// Sprite character width
	sprite_attr->attr0 &= ~MASK_SPN_HS;
	sprite_attr->attr0 |= (hs << B_SPN_HS) & MASK_SPN_HS;
	// Sprite character height
	sprite_attr->attr0 &= ~MASK_SPN_VS;
	sprite_attr->attr0 |= (vs << B_SPN_VS) & MASK_SPN_VS;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_character_size_set);

/**
 * @brief 		PPU sprite attribute palette set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	bank [in]: bank:0~3.
* @param 	palette_idx [in]: palette_idx:0~15.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_palette_set(
SpN_RAM *sprite_attr, 
unsigned int bank, 
unsigned int palette_idx
)
{
	if (!sprite_attr || bank>3 || palette_idx>0xF) {
		return STATUS_FAIL;
	}

	// Palette index
	sprite_attr->attr0 &= ~MASK_SPN_PALETTE;
	sprite_attr->attr0 |= (palette_idx << B_SPN_PALETTE) & MASK_SPN_PALETTE;
	// Palette bank high bit
	sprite_attr->attr0 &= ~MASK_SPN_PB_HIGH;
	sprite_attr->attr0 |= ((bank>>1) << B_SPN_PB_HIGH) & MASK_SPN_PB_HIGH;
	// Palette bank low bit
	sprite_attr->attr1 &= ~MASK_SPN_PB_LOW;
	sprite_attr->attr1 |= (bank << B_SPN_PB_LOW) & MASK_SPN_PB_LOW;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_palette_set);

/**
 * @brief 		PPU sprite attribute depth set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	bank [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_depth_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}

	sprite_attr->attr0 &= ~MASK_SPN_DEPTH;
	sprite_attr->attr0 |= (value << B_SPN_DEPTH) & MASK_SPN_DEPTH;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_depth_set);

/**
 * @brief 		PPU sprite attribute blend64 set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	enable [in]: enable:0=disable 1=enable.
* @param 	value [in]: value:0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_blend64_set(
SpN_RAM *sprite_attr, 
unsigned int enable, 
unsigned int value
)
{
	if (!sprite_attr || value>0x3F) {
		return STATUS_FAIL;
	}

	// Blending enable
	if (enable) {
		sprite_attr->attr0 |= SPN_BLD_ENABLE;
	} else {
		sprite_attr->attr0 &= ~SPN_BLD_ENABLE;
	}
	// Blending level
	sprite_attr->attr1 &= ~MASK_SPN_BLD_64_LVL;
	sprite_attr->attr1 |= (value << B_SPN_BLD_64_LVL) & MASK_SPN_BLD_64_LVL;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_blend64_set);

/**
 * @brief 		PPU sprite attribute blend16 set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	enable [in]: enable:0=disable 1=enable.
* @param 	value [in]: value:0~15 level.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_blend16_set(
SpN_RAM *sprite_attr, 
unsigned int enable, 
unsigned int value
)
{
	if (!sprite_attr || value>0xF) {
		return STATUS_FAIL;
	}

	// Blending enable
	if (enable) {
		sprite_attr->attr0 |= SPN_BLD_ENABLE;
	} else {
		sprite_attr->attr0 &= ~SPN_BLD_ENABLE;
	}
	// Blending level
	sprite_attr->attr1 &= ~MASK_SPN_BLD_16_LVL;
	sprite_attr->attr1 |= (value << B_SPN_BLD_16_LVL) & MASK_SPN_BLD_16_LVL;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_blend16_set);

/**
 * @brief 		PPU sprite attribute window mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_window_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}

	sprite_attr->attr1 &= ~MASK_SPN_WIN;
	sprite_attr->attr1 |= (value << B_SPN_WIN) & MASK_SPN_WIN;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_window_set);

/**
 * @brief 		PPU sprite attribute mosaic mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: large_sprite_en=0:value:0(no effect) 1(2x2 pixels) 2(4x4 pixels) 3(8x8 pixels), 1:value:0(no effect) 1(8x8 pixels) 2(16x16 pixels) 3(32x32 pixels).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_mosaic_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}

	sprite_attr->attr1 &= ~MASK_SPN_MOSAIC;
	sprite_attr->attr1 |= (value << B_SPN_MOSAIC) & MASK_SPN_MOSAIC;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_mosaic_set);

/**
 * @brief 		PPU sprite attribute charnum mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: charnum value.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_attribute_charnum_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{	
	
	if (!sprite_attr ) {
		return STATUS_FAIL;
	}

	// Sprite character width
	sprite_attr->nCharNumLo_16 &= (unsigned short)~MASK_SPN_CHARNUM_LO;
	sprite_attr->nCharNumLo_16 = (value & MASK_SPN_CHARNUM_LO);
	// Sprite character height
	sprite_attr->attr1 &= (unsigned char)~MASK_SPN_CHARNUM_HI;
	sprite_attr->attr1 |= (value >> B_SPN_CHARNUM_HI);

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_attribute_charnum_set);

/**
 * @brief 		PPU exsprite attribute group mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_group_attribute_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}
    sprite_attr++;
	if(GPSP_CMD_COMPARE())
	{
	   sprite_attr->uPosX_16 &=~ MASK_ESPN_GROUP_ID;
	   sprite_attr->uPosX_16 |=(value << B_ESPN_GROUP_ID) & MASK_ESPN_GROUP_ID;
	}
	else
	{
	   sprite_attr->nCharNumLo_16 &= ~MASK_SPN_GROUP_ID;
	   sprite_attr->nCharNumLo_16|=(value << B_SPN_GROUP_ID) & MASK_SPN_GROUP_ID;
	}   
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_group_attribute_set);

/**
 * @brief 		PPU exsprite attribute group mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_group_attribute_set(
SpN_EX_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}

	sprite_attr->ex_attr0 &= ~MASK_SPN_GROUP_ID;
	sprite_attr->ex_attr0 |= (value << B_SPN_GROUP_ID) & MASK_SPN_GROUP_ID;

	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_group_attribute_set);

/**
 * @brief 		PPU sprite attribute large size mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_large_size_attribute_set(
SpN_EX_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>1) {
		return STATUS_FAIL;
	}
	
	sprite_attr->ex_attr0 &= ~MASK_SPN_LG_SIZE;
	sprite_attr->ex_attr0 |= (value << B_SPN_LG_SIZE) & MASK_SPN_LG_SIZE;
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_large_size_attribute_set);

/**
 * @brief 		PPU exsprite attribute large size mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_large_size_attribute_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>1) {
		return STATUS_FAIL;
	}
    sprite_attr++;
	if(GPSP_CMD_COMPARE())
	{
	   sprite_attr->uPosX_16 &= ~MASK_ESPN_LG_SIZE;
	   sprite_attr->uPosX_16 |= (value << B_ESPN_LG_SIZE) & MASK_ESPN_LG_SIZE;
	}
	else
	{
	   sprite_attr->nCharNumLo_16 &= ~MASK_SPN_LG_SIZE;
	   sprite_attr->nCharNumLo_16 |= (value << B_SPN_LG_SIZE) & MASK_SPN_LG_SIZE;
	}   
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_large_size_attribute_set);

/**
 * @brief 		PPU sprite attribute interpolation mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_interpolation_attribute_set(
SpN_EX_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>1) {
		return STATUS_FAIL;
	}
	
	sprite_attr->ex_attr0 &= ~MASK_SPN_INTERPOLATION;
	sprite_attr->ex_attr0 |= (value << B_SPN_INTERPOLATION) & MASK_SPN_INTERPOLATION;
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_interpolation_attribute_set);

/**
 * @brief 		PPU exsprite attribute interpolation mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_exsprite_interpolation_attribute_set(
SpN_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>1) {
		return STATUS_FAIL;
	}
    sprite_attr++;
	if(GPSP_CMD_COMPARE())
	{
	   sprite_attr->uPosX_16 &= ~MASK_ESPN_INTERPOLATION;
	   sprite_attr->uPosX_16 |= (value << B_ESPN_INTERPOLATION) & MASK_ESPN_INTERPOLATION;
	}
	else
	{
	   sprite_attr->nCharNumLo_16 &= ~MASK_SPN_INTERPOLATION;
	   sprite_attr->nCharNumLo_16 |= (value << B_SPN_INTERPOLATION) & MASK_SPN_INTERPOLATION;
	}
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_exsprite_interpolation_attribute_set);

/**
 * @brief 		PPU sprite attribute cdm mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	in [in]: in: Color RGB value defines four point of the CDM sprite.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_cdm_attribute_set(
SpN_RAM *sprite_attr, 
unsigned int value,
CDM_STRUCT_PTR in
)
{	
	if (!sprite_attr || value>1 || !in) {
		return STATUS_FAIL;
	}
       sprite_attr++;
	sprite_attr->nCharNumLo_16 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->nCharNumLo_16 = (in->cdm0 & MASK_CDM_MODE);
	sprite_attr->uPosX_16 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->uPosX_16 = (value << (B_SPN_CDM-1)|in->cdm0 >> B_SPN_CDM);	
	sprite_attr->uPosY_16 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->uPosY_16 = (in->cdm1 & MASK_CDM_MODE);
	sprite_attr->attr0 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->attr0 = (in->cdm1 >> B_SPN_CDM);	
	sprite_attr->attr1 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->attr1 = (in->cdm2 & MASK_CDM_MODE);
	sprite_attr->uX1_16 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->uX1_16 = (in->cdm2 >> B_SPN_CDM);		
	sprite_attr->uX2_16 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->uX2_16 = (in->cdm3 & MASK_CDM_MODE);
	sprite_attr->uX3_16 &= (unsigned short)~MASK_CDM_MODE;
	sprite_attr->uX3_16 = (in->cdm3 >> B_SPN_CDM);	
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_cdm_attribute_set);

/**
 * @brief 		PPU sprite color mask mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_color_mask_attribute_set(
SpN_EX_RAM *sprite_attr, 
unsigned int value
)
{	
	if (!sprite_attr || value>3) {
		return STATUS_FAIL;
	}
	
	if(value==0)
	{
	   sprite_attr->ex_attr1 &= ~MASK_COLOR_MASK;	
	}
	
	else if(value==1)
	{
	   sprite_attr->ex_attr1 &= ~MASK_COLOR_MASK;
	   sprite_attr->ex_attr1 |= MASK_COLOR_MASK_BLUE;		
	}
	else if(value==2)
	{
	   sprite_attr->ex_attr1 &= ~MASK_COLOR_MASK;
	   sprite_attr->ex_attr1 |= MASK_COLOR_MASK_GREEN;		
	}
	else if(value==3)
	{
	   sprite_attr->ex_attr1 &= ~MASK_COLOR_MASK;
	   sprite_attr->ex_attr1 |= MASK_COLOR_MASK_RED;		
	}
	
	return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_sprite_color_mask_attribute_set);

// Sprite API
/**
 * @brief 		PPU sprite G+ director data initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_ptr:[in]: 32 bit address.
* @return 	none.
*/
void 
gp_ppu_sprite_image_data_init
(unsigned int sprite_number,
unsigned int sprite_ptr
)
{
  SPRITE *sprite_pointer,*Spritebuf_temp=Spritebuf;
  
  sprite_pointer=(SPRITE *)sprite_ptr;
  #if SPRITERAM_TYPE_SET == 1
     Spritebuf_temp+=(sprite_number*sizeof(SPRITE));
     Spritebuf_temp->nSizeX=(signed short)sprite_pointer->nSizeX;
     Spritebuf_temp->nSizeY=(signed short)sprite_pointer->nSizeY;   
  #else
     Spritebuf[sprite_number].nSizeX=(signed short)sprite_pointer->nSizeX;
     Spritebuf[sprite_number].nSizeY=(signed short)sprite_pointer->nSizeY;
  #endif
}
EXPORT_SYMBOL(gp_ppu_sprite_image_data_init);

/**
 * @brief 		PPU sprite G+ director data display initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_pos_x [in]: sprite x position on the screen.
* @param 	sprite_pos_y [in]: sprite y position on the screen.
* @param 	sprite_ptr:[in]: 32 bit address.
* @return 	none.
*/
void 
gp_ppu_sprite_display_set_init(
unsigned int sprite_number,
signed short sprite_pos_x,
signed short sprite_pos_y,
unsigned int sprite_ptr
)
{
  SPRITE *sprite_pointer,*Spritebuf_temp=Spritebuf;
  unsigned int temp;
  
  sprite_pointer=(SPRITE *)sprite_ptr;
  #if SPRITERAM_TYPE_SET == 1
     Spritebuf_temp+=(sprite_number*sizeof(SPRITE));
		 temp = (unsigned int)gp_user_va_to_pa((unsigned short *)sprite_pointer);
     Spritebuf_temp->SpCell=(unsigned int *)temp; 
     Spritebuf_temp->nPosX=sprite_pos_x;
     Spritebuf_temp->nPosY=sprite_pos_y;      
  #else
     Spritebuf[sprite_number].SpCell=(unsigned int *)sprite_pointer;
     Spritebuf[sprite_number].nPosX=sprite_pos_x;
     Spritebuf[sprite_number].nPosY=sprite_pos_y;
  #endif
}
EXPORT_SYMBOL(gp_ppu_sprite_display_set_init);

/**
 * @brief 		PPU sprite G+ director data display number of image initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_ptr:[in]: 32 bit address.
* @return 	none.
*/
void 
gp_ppu_sprite_image_number_set(
unsigned int sprite_number,
unsigned int sprite_ptr
)
{
  SPRITE *sprite_pointer,*Spritebuf_temp=Spritebuf;
  unsigned int temp;
  
  sprite_pointer=(SPRITE *)sprite_ptr;
  #if SPRITERAM_TYPE_SET == 1
     Spritebuf_temp+=(sprite_number*sizeof(SPRITE));
		 temp = (unsigned int)gp_user_va_to_pa((unsigned short *)sprite_pointer);
     Spritebuf_temp->SpCell=(unsigned int *)temp;  
  #else 
     Spritebuf[sprite_number].SpCell=(unsigned int *)sprite_pointer;
  #endif
}
EXPORT_SYMBOL(gp_ppu_sprite_image_number_set);

/**
 * @brief 		PPU sprite disable number of image initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @return 	none.
*/
void 
gp_ppu_sprite_disable_set(
unsigned int sprite_number
)
{
  SPRITE *Spritebuf_temp=Spritebuf;
  
  #if SPRITERAM_TYPE_SET == 1
     Spritebuf_temp+=(sprite_number*sizeof(SPRITE));
     Spritebuf_temp->SpCell=0;
  #else
     Spritebuf[sprite_number].SpCell=0;
  #endif
}
EXPORT_SYMBOL(gp_ppu_sprite_disable_set);

/**
 * @brief 		PPU sprite information of sprite ram function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_ptr:[in]: 32 bit address for SpN_ptr stuct.
* @return 	none.
*/
static unsigned int temp,temp1;
void 
Get_ppu_sprite_image_info(
unsigned int sprite_number,
SpN_ptr *sprite_ptr
)
{
   SpN_ptr *sp_tmp,*spexram_tmp=Sprite_ptr;

   #if SPRITERAM_TYPE_SET == 1 
     #if 1
      temp = (unsigned int)gp_user_va_to_pa((unsigned short *)sprite_ptr);
      temp1 = (unsigned int)gp_chunk_va((unsigned int)temp);
      sp_tmp = (SpN_ptr *)temp1; 
     #else
      sp_tmp = sprite_ptr;
     #endif
     spexram_tmp+=(sprite_number*sizeof(SpN_ptr));
     sp_tmp->nSP_CharNum=spexram_tmp->nSP_CharNum;
     sp_tmp->nSP_Hsize=spexram_tmp->nSP_Hsize;
     sp_tmp->nSP_Vsize=spexram_tmp->nSP_Vsize;
     sp_tmp->nSPNum_ptr=spexram_tmp->nSPNum_ptr;  
     sp_tmp->nEXSPNum_ptr=spexram_tmp->nEXSPNum_ptr;            
   #else
     sprite_ptr->nSP_CharNum=Sprite_ptr[sprite_number].nSP_CharNum;
     sprite_ptr->nSP_Hsize=Sprite_ptr[sprite_number].nSP_Hsize;
     sprite_ptr->nSP_Vsize=Sprite_ptr[sprite_number].nSP_Vsize;
     sprite_ptr->nSPNum_ptr=Sprite_ptr[sprite_number].nSPNum_ptr;  
     sprite_ptr->nEXSPNum_ptr=Sprite_ptr[sprite_number].nEXSPNum_ptr; 
   #endif
}
EXPORT_SYMBOL(Get_ppu_sprite_image_info);

/**
 * @brief 		PPU memory set.
* @param 	pDest [in]: target address.
* @param 	uValue:[in]: memory set value.
* @param 	nBytes [in]: data length.
* @param 	mem_copy_8_16_32:[in]: move data length of once.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_set_mem(
unsigned int *pDest, 
unsigned int uValue, 
signed int nBytes,
unsigned short  mem_copy_8_16_32
)
{
    signed int  i;
    unsigned int  *pPtrDest;
    unsigned short  *pPtr16Dest;
    char    *pPtr8Dest;

    pPtrDest = pDest;
    if (!pPtrDest) {
        return STATUS_FAIL;
    }
    if(mem_copy_8_16_32==32){
      //  32-bit memory set
        pPtrDest = pDest;
        for (i=0;i<(nBytes/sizeof (unsigned int));i++) {
            *(pPtrDest++) = uValue;
        }
    }else if(mem_copy_8_16_32==16){
      //  16-bit memory set      
        pPtr16Dest = (unsigned short *)pDest;
        for (i=0;i<(nBytes/sizeof (unsigned short));i++) {
            *(pPtr16Dest++) = uValue;
        }
    }else{
        //  8-bit memory set
        pPtr8Dest = (char *)pPtrDest;
        for (i=0;i<nBytes;i++) {
            *(pPtr8Dest++) = ((uValue&(((unsigned int)0xF)<<(i<<3)))>>(i<<3));
        }
    }
    return STATUS_OK;
}
EXPORT_SYMBOL(gp_ppu_set_mem);

/**
 * @brief 		PPU sprite attribute coordination of sprite ram function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	display_mode:[in]: display resolution.
* @param 	coordinate_mode [in]: coordinate mode.
* @param 	sprite_number:[in]: sprite number 0~1024.
* @return 	none.
*/
   unsigned short temp_x,temp_y,cx,cy;
   unsigned short uAttribute0,uAttribute1;
   unsigned int i,j,sprite_large_size_enable;
   signed int nPosXN, nPosYN;
   signed int nScrWidth, nScrHeight;
static   unsigned int temp,temp1;
   PSPRITE nSprite; 
   PSpN_RAM sprite_attr;   
   PSpN_EX_RAM sprite_ex_attr;
   PSpN16_CellIdx pSpCell;
   PSpN_ptr  sp_ptr;
void 
gp_ppu_paint_spriteram(
PPU_REGISTER_SETS *p_register_set,
Sprite_Coordinate_Mode display_mode, 
Coordinate_Change_Mode coordinate_mode, 
unsigned short sprite_number
)
{


   switch(display_mode)
   {
       case Sprite_Coordinate_320X240:
            nScrWidth = 320;
		    nScrHeight = 240 + 14;
            break;
       case Sprite_Coordinate_640X480:
       		nScrWidth = 640;
		    nScrHeight = 480 + 14;
            break;         
       case Sprite_Coordinate_480X234:
       		nScrWidth = 480;
		    nScrHeight = 234;       
            break;
       case Sprite_Coordinate_480X272:
            nScrWidth = 480;
		    nScrHeight = 272;  
            break;
       case Sprite_Coordinate_720X480:
       		nScrWidth = 720;
		    nScrHeight = 480;         
            break;     
       case Sprite_Coordinate_800X480:
       		nScrWidth = 800;
		    nScrHeight = 480;         
            break;         
       case Sprite_Coordinate_800X600:
       		nScrWidth = 800;
		    nScrHeight = 600;         
            break;
       case Sprite_Coordinate_1024X768:
       		nScrWidth = 1024;
		    nScrHeight = 768;         
            break;
       default:
            nScrWidth = 640;
		    nScrHeight = 480 + 14;
            break;      
    }                      
    sprite_large_size_enable=0;
    sprite_attr=(PSpN_RAM)SpriteRAM;
    gp_ppu_set_mem((unsigned int *)sprite_attr,0,SpriteRAM_number*sizeof(SpN_RAM),32);
    nSprite=(PSPRITE)Spritebuf;
    sp_ptr=(PSpN_ptr)Sprite_ptr;
    gp_ppu_set_mem((unsigned int *)sp_ptr,0,Sprite_ptr_number*sizeof(PSpN_ptr),32);  
    sprite_ex_attr=(PSpN_EX_RAM)SpriteExRAM;
    gp_ppu_set_mem((unsigned int *)sprite_ex_attr,0,SpriteRAM_number*sizeof(SpN_EX_RAM),32);

    for(i=0;i<sprite_number;i++) {// Check sprite number?
	   if(nSprite->SpCell!= 0) { // Check sprite if exist?			  		
		  //sprite ram start address
		  sp_ptr->nSPNum_ptr=(unsigned int)sprite_attr;		  
		  //sprite extend ram start address
		  sp_ptr->nEXSPNum_ptr=(unsigned int)sprite_ex_attr;		  
		  // MAX Sprite Characters Combination Mode	
		  for(j=0; j<max_sp_combination_number; j++) 
		  {	            				     
			  
			#if 1
			  temp = (unsigned int)nSprite->SpCell+j;
			  temp1 = (unsigned int)gp_chunk_va((unsigned int)temp);
			  pSpCell = (PSpN16_CellIdx)temp1;
			#else
			  pSpCell = (PSpN16_CellIdx) nSprite->SpCell+j;
			#endif
			  if( pSpCell->nCharNumLo == data_end_word ) 
			  {
			        // Check Sprite Characters Combination Flag	
					sp_ptr->nSP_CharNum=j;
					sp_ptr->nSP_Hsize=nH_Size;
			        sp_ptr->nSP_Vsize=nV_Size; 
					break;
			  } 
			  uAttribute0 = 0;
			  uAttribute1 = 0;
			  h_size_tmp=pSpCell->nHS;
			  v_size_tmp=pSpCell->nVS;
			  h_pos_tmp=pSpCell->nPosX;
			  v_pos_tmp=pSpCell->nPosY;		  
			  // Sprite RAM - Character Number
			  sprite_attr->nCharNumLo_16 = pSpCell->nCharNumLo;
			  // SpN Rotate
			  sprite_attr->uPosX_16 = pSpCell->nRotate;
			  // SpN Zoom
			  sprite_attr->uPosY_16 = pSpCell->nZoom;
			  // Sprite RAM - Attribute 0
			  if((p_register_set->ppu_palette_control & p1024_enable) == p1024_enable)
			  {
			    if(pSpCell->nPaletteBank == SP_PBANK3){
			       uAttribute0 |= (1<<pal_control_1);
			       uAttribute1 |= (1<<pal_control_0);
			    }else if(pSpCell->nPaletteBank == SP_PBANK2){
			       uAttribute0 |= (1<<pal_control_1);
			       uAttribute1 |= (0<<pal_control_0);
			    }else if(pSpCell->nPaletteBank == SP_PBANK1){
		           uAttribute0 |= (0<<pal_control_1);
			       uAttribute1 |= (1<<pal_control_0);
		        }else {
		           uAttribute0 |= (0<<pal_control_1);
			       uAttribute1 |= (0<<pal_control_0);
		        }
		      }else{
		        uAttribute1 = pSpCell->nPaletteBank;
			    if(uAttribute1 == SP_PBANK3)
			         uAttribute0 |= (1<<pal_control_1);
		        else
		           uAttribute0 |= (0<<pal_control_1);
		      }
		      uAttribute1 = 0;
		      uAttribute0 |= pSpCell->nBlend;
		      uAttribute0 |= pSpCell->nDepth;
		      uAttribute0 |= pSpCell->nPalette;
		      uAttribute0 |= pSpCell->nVS; 
		      uAttribute0 |= pSpCell->nHS;
		      uAttribute0 |= pSpCell->nFlipV;
		      uAttribute0 |= pSpCell->nFlipH;
		      uAttribute0 |= pSpCell->nColor;  
			  sprite_attr->attr0 = uAttribute0;
		      // Sprite RAM - Attribute 1
		      uAttribute1 |= pSpCell->nCharNumHi;
		      uAttribute1 |= pSpCell->nMosaic;
		      uAttribute1 |= pSpCell->nBldLvl;
		      sprite_attr->attr1 = uAttribute1;		      
		      // Sprite Extend RAM - Attribute 1
		      uAttribute1 = 0;
		      uAttribute1 |= pSpCell->nSPGroup;
		      uAttribute1 |= pSpCell->nSPLargeSize;
		      uAttribute1 |= pSpCell->nSPInterpolation;
			  sprite_ex_attr->ex_attr1 = uAttribute1;
		      // Sprite size compare
		      if(pSpCell->nSPLargeSize==SPLarge_ENABLE)
		      {
		         sprite_large_size_enable=1;
		         //Get sprite H size
		         if(pSpCell->nHS==SP_HSIZE_L32)
		           nH_Size=32;
		         else if(pSpCell->nHS==SP_HSIZE_L64)
		           nH_Size=64;
		         else if(pSpCell->nHS==SP_HSIZE_L128)  
			       nH_Size=128;
		         else if(pSpCell->nHS==SP_HSIZE_L256)  
			       nH_Size=256;
			     //Get sprite V size  
			     if(pSpCell->nVS==SP_VSIZE_L32)
		           nV_Size=32;
		         else if(pSpCell->nVS==SP_VSIZE_L64)
		           nV_Size=64;
		         else if(pSpCell->nVS==SP_VSIZE_L128)  
			       nV_Size=128;
		         else if(pSpCell->nVS==SP_VSIZE_L256)  
			       nV_Size=256;
			  }    			  
              if(sprite_large_size_enable == 0)
              {	
              ///*
              #if 1
                switch(h_size_tmp)
                {
                	  case SP_HSIZE8:
                	       nH_Size=8;
                	       break;
                	  case SP_HSIZE16:
                	       nH_Size=16;
                	       break;                	
                	  case SP_HSIZE32:
                	       nH_Size=32;
                	       break;
                	  case SP_VSIZE64:
                	       nH_Size=64;
                	       break;                 	   	 			  
		            }
                switch(v_size_tmp)
                {
                	  case SP_VSIZE8:
                	       nV_Size=8;
                	       break;
                	  case SP_VSIZE16:
                	       nV_Size=16;
                	       break;                	
                	  case SP_VSIZE32:
                	       nV_Size=32;
                	       break;
                	  case SP_VSIZE64:
                	       nV_Size=64;
                	       break;                 	   	 			  
		            }                	       
		            uAttribute0 = 0;
		          #else
			          //Get sprite H size
			          if(pSpCell->nHS==SP_HSIZE8)
			            nH_Size=8;
			          else if(pSpCell->nHS==SP_HSIZE16)
			            nH_Size=16;
			          else if(pSpCell->nHS==SP_HSIZE32)  
				        nH_Size=32;
			          else if(pSpCell->nHS==SP_HSIZE64)  
				        nH_Size=64;
				        //Get sprite V size  
				        if(pSpCell->nVS==SP_VSIZE8)
			            nV_Size=8;
			          else if(pSpCell->nVS==SP_VSIZE16)
			            nV_Size=16;
			          else if(pSpCell->nVS==SP_VSIZE32)  
				        nV_Size=32;
			          else if(pSpCell->nVS==SP_VSIZE64)  
				        nV_Size=64; 
			        #endif
			        //*/
			        //nH_Size=64;
			        //nV_Size=64;
			  }  
			  #if 0
			  temp_x=pSpCell->nPosX;
			  temp_y=pSpCell->nPosY;
			  #else
			  temp_x=h_pos_tmp;
			  temp_y=v_pos_tmp;			  
			  #endif
			  if((p_register_set->ppu_enable & spv3d_enable) == spv3d_enable)
			  {
			     cx = (nSprite->nSizeX & spirte_pos_max_bit) - nH_Size;  // get maximum x position
                 cy = nV_Size - (nSprite->nSizeY & spirte_pos_max_bit);  // get minimum y position
                 if((nSprite->uAttr0 & hflip_bit) != 0)          // HFlip 						
                    temp_x = cx - temp_x;					
                 if((nSprite->uAttr0 & vflip_bit) != 0)          // VFlip				
                    temp_y = cy - temp_y;
              }    
			  // coordinates change  			     
			  if (coordinate_mode == Center2LeftTop_coordinate) 
			  {
				 // Center2LeftTop_coordinate
				 nPosXN = (nScrWidth>>1) + (nH_Size>>1) + 1 + nSprite->nPosX + temp_x;
				 //nPosXN = (nScrWidth>>1) + temp_x;
				 nPosYN = (nScrHeight>>1) - 1 - nSprite->nPosY + temp_y - (nV_Size>>1);	                               
                 //nPosYN = (nScrHeight>>1) - temp_y; 				            
			  } 
			  else if (coordinate_mode == LeftTop2Center_coordinate)
			  {
				 // LeftTop2Center_coordinate
				 nPosXN = nSprite->nPosX + temp_x - (nScrWidth>>1) + (nH_Size>>1) + 1;
				 nPosYN = (nScrHeight>>1) - 1 - nSprite->nPosY - temp_y - (nV_Size>>1);
			  } 
			  else
			  {
			  	 // PPU_hardware_coordinate
			  	 // fix x, y the origin of the coordinates by hardware
				 nPosXN = nSprite->nPosX + temp_x;
				 nPosYN = nSprite->nPosY + temp_y;
			  } 			    
			  // Sprite RAM - Rotate / Position X
			  sprite_attr->uPosX_16 |= (nPosXN & spirte_pos_max_bit);
			  // Sprite RAM - Zoom / Position Y
			  sprite_attr->uPosY_16 |= (nPosYN & spirte_pos_max_bit);
			  sprite_attr++;
			  if((p_register_set->sprite_control & cdm_enable) == cdm_enable)
			    sprite_attr++;			  
			  sprite_ex_attr++;			  		  
	      }                  
	   }
	 nSprite++;
	 sp_ptr++;	  
   }
   p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE_ATTRIBUTE;
   p_register_set->update_register_flag |= C_UPDATE_REG_SET_SPRITE_EX_ATTRIBUTE;  	        
}
EXPORT_SYMBOL(gp_ppu_paint_spriteram);

/**
 * @brief 		PPU sprite set position function.
* @param 	sp_num [in]: sprite number.
* @param 	display_mode:[in]: display resolution.
* @param 	x [in]: x position offset value of move for once.
* @param 	y:[in]: y position offset value of move for once.
* @param 	scroll:[in]: check of dispiay boundary.
* @return 	none.
*/
void SetSpritePosition(unsigned short sp_num,Sprite_Coordinate_Mode display_mode, signed short x, signed short y, unsigned short scroll)
{
	unsigned short screen_width, screen_height;
	signed short xSize, ySize,xmin, xmax, ymin, ymax;
	PSPRITE nSprite;
	
    nSprite=(PSPRITE)Spritebuf;
	if(nSprite->SpCell!= 0) // check sprite exist/non-exist
	{
			Spritebuf[sp_num].nPosX += x;
			Spritebuf[sp_num].nPosY += y;
			if (scroll)
			{
                switch(display_mode)
                {
                    case Sprite_Coordinate_320X240:
                         screen_width = 320;
		                 screen_height = 240 + 14;
                         break;
                    case Sprite_Coordinate_640X480:
       		             screen_width = 640;
		                 screen_height = 480 + 14;
                         break;         
                    case Sprite_Coordinate_480X234:
       		             screen_width = 480;
		                 screen_height = 234;       
                         break;
                    case Sprite_Coordinate_480X272:
                         screen_width = 480;
		                 screen_height = 272;  
                         break;
                    case Sprite_Coordinate_720X480:
       		             screen_width = 720;
		                 screen_height = 480;         
                         break;     
                    case Sprite_Coordinate_800X480:
       		             screen_width = 800;
		                 screen_height = 480;         
                         break;         
                    case Sprite_Coordinate_800X600:
       		             screen_width = 800;
		                 screen_height = 600;         
                         break;
                    case Sprite_Coordinate_1024X768:
       		             screen_width = 1024;
		                 screen_height = 768;         
                         break;
                    default:
                         screen_width = 640;
		                 screen_height = 480 + 14;
                         break;      
                }
				xSize = nH_Size;	// get x size
				ySize = nV_Size;	// get y size
				   xmin = 0 - xSize;
				   xmax = screen_width + xSize;
				   ymin = 0 - ySize;
				   ymax = screen_height + ySize;
				   if (Spritebuf[sp_num].nPosX > xmax)
					   Spritebuf[sp_num].nPosX = xmin;
				   if (Spritebuf[sp_num].nPosY > ymax)
					   Spritebuf[sp_num].nPosY = ymin;
				   if (Spritebuf[sp_num].nPosX < xmin)
					   Spritebuf[sp_num].nPosX = xmax;
				   if (Spritebuf[sp_num].nPosY < ymin)
					   Spritebuf[sp_num].nPosY = ymax;
			}
	}			
}



// Sprite V3D API
void spriteV3D_Pos_Coordinate_Change(Sprite_Coordinate_Mode display_mode,Coordinate_Change_Mode coordinate_mode,POS_STRUCT_PTR in)
{
    signed int nPosXN,nPosYN,temp_x,temp_y,nScrWidth,nScrHeight;
    POS_STRUCT_PTR Sprite_pos;
    
    if(coordinate_mode!=PPU_hardware_coordinate)
    {    
	    switch(display_mode)
	    {
	       case Sprite_Coordinate_320X240:
	            nScrWidth = 320;
			    nScrHeight = 240 + 14;
	            break;
	       case Sprite_Coordinate_640X480:
	       		nScrWidth = 640;
			    nScrHeight = 480 + 14;
	            break;         
	       case Sprite_Coordinate_480X234:
	       		nScrWidth = 480;
			    nScrHeight = 234;       
	            break;
	       case Sprite_Coordinate_480X272:
	            nScrWidth = 480;
			    nScrHeight = 272;  
	            break;
	       case Sprite_Coordinate_720X480:
	       		nScrWidth = 720;
			    nScrHeight = 480;         
	            break;     
	       case Sprite_Coordinate_800X480:
	       		nScrWidth = 800;
			    nScrHeight = 480;         
	            break;         
	       case Sprite_Coordinate_800X600:
	       		nScrWidth = 800;
			    nScrHeight = 600;         
	            break;
	       case Sprite_Coordinate_1024X768:
	       		nScrWidth = 1024;
			    nScrHeight = 768;         
	            break;
	       default:
	            nScrWidth = 640;
			    nScrHeight = 480 + 14;
	            break;      
	    }
	    Sprite_pos=in;

	    //x0,y0
		temp_x=Sprite_pos->x0;
	    temp_y=Sprite_pos->y0; 
	    if(coordinate_mode==1)
	    {
	      // Center2LeftTop_coordinate
	      nPosXN = (nScrWidth>>1) + temp_x;
	      nPosYN = (nScrHeight>>1) - temp_y;       
	    }
	    else if(coordinate_mode==2)
	    {
	      // LeftTop2Center_coordinate
	      nPosXN = temp_x  - (nScrWidth>>1);
	      nPosYN = (nScrHeight>>1) - temp_y;  
	    }
	    else
	    {
	      nPosXN = temp_x;
	      nPosYN = temp_y;	    	
	    	
	    }	    
	    Sprite_pos->x0=nPosXN;
	    Sprite_pos->y0=nPosYN;
	    
	    //x1,y1
		temp_x=Sprite_pos->x1;
	    temp_y=Sprite_pos->y1; 
	    if(coordinate_mode==1)
	    {
	      // Center2LeftTop_coordinate
	      nPosXN = (nScrWidth>>1) + temp_x;
	      nPosYN = (nScrHeight>>1) - temp_y;      
	    }
	    else if(coordinate_mode==2)
	    {
	      // LeftTop2Center_coordinate
	      nPosXN = temp_x  - (nScrWidth>>1);
	      nPosYN = (nScrHeight>>1) - temp_y;         
	    }
	    else
	    {
	      nPosXN = temp_x;
	      nPosYN = temp_y;	    	
	    	
	    }	    
	    Sprite_pos->x1=nPosXN;
	    Sprite_pos->y1=nPosYN; 
	    
	    //x2,y2
		temp_x=Sprite_pos->x2;
	    temp_y=Sprite_pos->y2; 
	    if(coordinate_mode==1)
	    {
	      // Center2LeftTop_coordinate
	      nPosXN = (nScrWidth>>1) + temp_x;
	      nPosYN = (nScrHeight>>1) - temp_y;       
	    }
	    else if(coordinate_mode==2)
	    {
	      // LeftTop2Center_coordinate
	      nPosXN = temp_x  - (nScrWidth>>1);
	      nPosYN = (nScrHeight>>1) - temp_y;       
	    }
	    else
	    {
	      nPosXN = temp_x;
	      nPosYN = temp_y;	    	
	    	
	    }	    
	    Sprite_pos->x2=nPosXN;
	    Sprite_pos->y2=nPosYN;	    
	    
	    //x3,y3
		temp_x=Sprite_pos->x3;
	    temp_y=Sprite_pos->y3; 
	    if(coordinate_mode==1)
	    {
	      // Center2LeftTop_coordinate
	      nPosXN = (nScrWidth>>1) + temp_x;
	      nPosYN = (nScrHeight>>1) - temp_y;      
	    }
	    else if(coordinate_mode==2)
	    {
	      // LeftTop2Center_coordinate
	      nPosXN = temp_x  - (nScrWidth>>1);
	      nPosYN = (nScrHeight>>1) - temp_y;
	    }
	    else
	    {
	      nPosXN = temp_x;
	      nPosYN = temp_y;	    	
	    	
	    }
	    Sprite_pos->x3=nPosXN;
	    Sprite_pos->y3=nPosYN;	    
    }      
}

static void SpCellXPos25d(unsigned int xnum, unsigned int ynum, unsigned int xpos0, unsigned int xpos1, unsigned int xpos2, unsigned int xpos3)
{
	unsigned int i, j;
	#if 0
	  FP32 GridPoint[SP25D_MAX_CHAR_V+1][2];
	#else
	  unsigned int GridPoint[SP25D_MAX_CHAR_V+1][2];
	#endif
		
	//find 2-point of each grid line
	for (i=0;i<=ynum;i++)	
	{
		GridPoint[i][0] = xpos0 + i*(xpos3-xpos0)/ynum; //start position of horizontal grid line
		GridPoint[i][1] = xpos1 + i*(xpos2-xpos1)/ynum; //end position of horizontal grid line  
	}	
	//get grid line division   
	for (i=0;i<=ynum;i++)
	{
		for (j=0;j<=xnum;j++)	
			#if 0
			  HGridBuff[i][j] = (FP32)(GridPoint[i][0] + j*(GridPoint[i][1]-GridPoint[i][0])/xnum);
      #else
        HGridBuff[i][j] = (unsigned int)(GridPoint[i][0] + j*(GridPoint[i][1]-GridPoint[i][0])/xnum);
      #endif
 	}					   		
}		

static void SpCellYPos25d(unsigned int xnum, unsigned int ynum, unsigned int ypos0, unsigned int ypos1, unsigned int ypos2, unsigned int ypos3)
{
	unsigned int i, j;
	#if 0
	  FP32 GridPoint[SP25D_MAX_CHAR_V+1][2];
	#else
	  unsigned int GridPoint[SP25D_MAX_CHAR_V+1][2];
	#endif	
	
	//find 2-point of each grid line
	for (i=0;i<=ynum;i++)
	{
		 GridPoint[i][0] = ypos0 + i*(ypos3-ypos0)/ynum; //start position of horizontal grid line   
		 GridPoint[i][1] = ypos1 + i*(ypos2-ypos1)/ynum; //end position of horizontal grid line
	}	
	//get grid line division   
	for (i=0;i<=ynum;i++)
	{
		for (j=0;j<=xnum;j++)	
			#if 0
			  VGridBuff[i][j] = (FP32)(GridPoint[i][0] + j*(GridPoint[i][1]-GridPoint[i][0])/xnum);
			#else
			  VGridBuff[i][j] = (unsigned int)(GridPoint[i][0] + j*(GridPoint[i][1]-GridPoint[i][0])/xnum);
			#endif
	}							   		
}

void sp_group_set(PSpN_EX_RAM ex_ram,unsigned char group_id)
{
     ex_ram->ex_attr1|=(group_id & mask_group_id);	
}

void sp_interpolation_enable(PSpN_EX_RAM ex_ram)
{
     ex_ram->ex_attr1|=sp_init_en;	
}

void sp_interpolation_disable(PSpN_EX_RAM ex_ram)
{
     ex_ram->ex_attr1 &= ~sp_init_en;	
}

void exsp_group_set(unsigned int ex_ram,unsigned char group_id)
{
     PSpN_RAM sprite_cdm_attr;
     
     if(GPSP_CMD_COMPARE())
     {
        sprite_cdm_attr=(PSpN_RAM)ex_ram;
        sprite_cdm_attr++;
        sprite_cdm_attr->uPosX_16 |=((group_id & mask_group_id)<<b_spn_group);	
     }
     else
     {    
        sprite_cdm_attr=(PSpN_RAM)ex_ram;
        sprite_cdm_attr++;
        sprite_cdm_attr->nCharNumLo_16|=(group_id & mask_group_id);
     } 	
}

void exsp_interpolation_enable(unsigned int ex_ram)
{
     PSpN_RAM sprite_cdm_attr;
     
     if(GPSP_CMD_COMPARE())
     {
        sprite_cdm_attr=(PSpN_RAM)ex_ram;
        sprite_cdm_attr++;
        sprite_cdm_attr->uPosX_16|=sp_cdm_init_en;
     }
     else
     {    
        sprite_cdm_attr=(PSpN_RAM)ex_ram;
        sprite_cdm_attr++;
        sprite_cdm_attr->nCharNumLo_16|=sp_init_en;
     }      	
}
void exsp_interpolation_disable(unsigned int ex_ram)
{
     PSpN_RAM sprite_cdm_attr;
     
     if(GPSP_CMD_COMPARE())
     {
        sprite_cdm_attr=(PSpN_RAM)ex_ram;
        sprite_cdm_attr++;
        sprite_cdm_attr->uPosX_16 &= ~sp_cdm_init_en;
     }
     else
     {    
        sprite_cdm_attr=(PSpN_RAM)ex_ram;
        sprite_cdm_attr++;
        sprite_cdm_attr->nCharNumLo_16 &= ~sp_init_en;
     }      	
}

/**
 * @brief 		PPU sprite set position function.
* @param 	sp_num [in]: sprite number.
* @param 	display_mode:[in]: display resolution.
* @param 	x [in]: x position offset value of move for once.
* @param 	y:[in]: y position offset value of move for once.
* @param 	scroll:[in]: check of dispiay boundary.
* @return 	none.
*/
void SpriteV3D_set(unsigned int sprite_number,Sprite_Coordinate_Mode display_mode,Coordinate_Change_Mode coordinate_mode,SpN_ptr *sprite_ptr,V3D_POS_STRUCT_PTR in)
{
    signed int nPosXN,nPosYN;
    unsigned int i,k,h,nSP_Hsize,nSP_Vsize,sp_num_addr,temp_x,temp_y,INT,temp_ck;
    PSPRITE nSprite;
    PSpN16_CellIdx pSpCell;    
#if ((SP_FRACTION_ENABLE == 1))     
    POS_STRUCT_GP32XXX Sprite_pos;
    #if 0
      FP32 REM;
    #else
      FP32 REM;
    #endif
#else 
    POS_STRUCT Sprite_pos;
#endif    
    PSpN_ptr sp_ptr;  
    PSpN_EX_RAM sprite_ex_attr;
    #if 0    
      FP32 SUM;
    #else
      unsigned int SUM;
    #endif

	nSP_Hsize=(Spritebuf[sprite_number].nSizeX);
	nSP_Vsize=(Spritebuf[sprite_number].nSizeY);
	nPosXN=Spritebuf[sprite_number].nPosX;
	nPosYN=Spritebuf[sprite_number].nPosY;
#if 0 	
	Sprite_pos.x0=in->V3D_POS1.x0;
	Sprite_pos.y0=in->V3D_POS1.y0;
	Sprite_pos.x1=in->V3D_POS1.x1;
	Sprite_pos.y1=in->V3D_POS1.y1;
	Sprite_pos.x2=in->V3D_POS1.x2;
	Sprite_pos.y2=in->V3D_POS1.y2;
	Sprite_pos.x3=in->V3D_POS1.x3;
	Sprite_pos.y3=in->V3D_POS1.y3;
#else
	Sprite_pos.x0=in->V3D_POS2.x0;
	Sprite_pos.y0=in->V3D_POS2.y0;
	Sprite_pos.x1=in->V3D_POS2.x1;
	Sprite_pos.y1=in->V3D_POS2.y1;
	Sprite_pos.x2=in->V3D_POS2.x2;
	Sprite_pos.y2=in->V3D_POS2.y2;
	Sprite_pos.x3=in->V3D_POS2.x3;
	Sprite_pos.y3=in->V3D_POS2.y3;	
#endif	
	sp_ptr=sprite_ptr; 	
	sprite_ex_attr=(PSpN_EX_RAM)sp_ptr->nEXSPNum_ptr;
	temp_ck=(unsigned int)&Spritebuf[sprite_number];
	nSprite=(PSPRITE)temp_ck;
	sp_num_addr=sp_ptr->nSPNum_ptr;	
	if(sp_ptr->nSP_CharNum > 1)
    {
		temp_ck=nSP_Hsize%sp_ptr->nSP_Hsize;
		temp_x=nSP_Hsize/sp_ptr->nSP_Hsize;
		if(temp_ck)
		   temp_x++;
		temp_ck=nSP_Vsize%sp_ptr->nSP_Vsize;   
		temp_y=nSP_Vsize/sp_ptr->nSP_Vsize;
		if(temp_ck)
		   temp_y++;
		if((Sprite_pos.x0 < 0)||(Sprite_pos.x1 < 0)||(Sprite_pos.x2 < 0)||(Sprite_pos.x3 < 0)||
		(Sprite_pos.y0 < 0)||(Sprite_pos.y1 < 0)||(Sprite_pos.y2 < 0)||(Sprite_pos.y3 < 0))              
		   spriteV3D_Pos_Coordinate_Change(display_mode,Center2LeftTop_coordinate,(POS_STRUCT_PTR)&Sprite_pos);       
		SpCellXPos25d(temp_x,temp_y,Sprite_pos.x0,Sprite_pos.x1,Sprite_pos.x2,Sprite_pos.x3);
		SpCellYPos25d(temp_x,temp_y,Sprite_pos.y0,Sprite_pos.y1,Sprite_pos.y2,Sprite_pos.y3);
		for(i=0;i<sp_ptr->nSP_CharNum;i++)
		{
			pSpCell = (PSpN16_CellIdx) nSprite->SpCell+i;
			temp_x=pSpCell->nPosX;
			temp_y=pSpCell->nPosY;
			h = temp_x/(sp_ptr->nSP_Hsize);
			k = temp_y/(sp_ptr->nSP_Vsize);		   	   
			#if 0
				//x0
				SUM = (FP32)HGridBuff[k][h];
				INT = (unsigned int)SUM;
				Sprite_pos.x0 = INT;
				//x1
				SUM = (FP32)HGridBuff[k][h+1];
				INT = (unsigned int)SUM-nPosXN;
				INT = (unsigned int)INT+nPosXN;
				Sprite_pos.x1 = INT;       			
				//x2
				SUM = (FP32)HGridBuff[k+1][h+1];
				INT = (unsigned int)SUM-nPosXN;
				INT = (unsigned int)INT+nPosXN;
				Sprite_pos.x2 = INT;			
				//x3
				SUM = (FP32)HGridBuff[k+1][h];
				INT = (unsigned int)SUM-nPosXN;
				INT = (unsigned int)INT+nPosXN;
				Sprite_pos.x3 = INT;       			
				//y0
				SUM = (FP32)VGridBuff[k][h];
				INT = (unsigned int)SUM;
				Sprite_pos.y0 = INT;			
				//y1
				SUM = (FP32)VGridBuff[k][h+1];
				INT = (unsigned int)SUM-nPosYN;
				INT = (unsigned int)INT+nPosYN;
				Sprite_pos.y1 = INT;       		
				//y2
				SUM = (FP32)VGridBuff[k+1][h+1];
				INT = (unsigned int)SUM-nPosYN;
				INT = (unsigned int)INT+nPosYN;
				Sprite_pos.y2 = INT;			
				//y3
				SUM = (FP32)VGridBuff[k+1][h];
				INT = (unsigned int)SUM-nPosYN;
				INT = (unsigned int)INT+nPosYN;
				Sprite_pos.y3 = INT; 
			#else
				SUM = (unsigned int)HGridBuff[k][h];
				INT = (unsigned int)SUM;
				Sprite_pos.x0 = INT;
				//x1
				SUM = (unsigned int)HGridBuff[k][h+1];
				INT = (unsigned int)SUM-nPosXN;
				INT = (unsigned int)INT+nPosXN;
				Sprite_pos.x1 = INT;       			
				//x2
				SUM = (unsigned int)HGridBuff[k+1][h+1];
				INT = (unsigned int)SUM-nPosXN;
				INT = (unsigned int)INT+nPosXN;
				Sprite_pos.x2 = INT;			
				//x3
				SUM = (unsigned int)HGridBuff[k+1][h];
				INT = (unsigned int)SUM-nPosXN;
				INT = (unsigned int)INT+nPosXN;
				Sprite_pos.x3 = INT;       			
				//y0
				SUM = (unsigned int)VGridBuff[k][h];
				INT = (unsigned int)SUM;
				Sprite_pos.y0 = INT;			
				//y1
				SUM = (unsigned int)VGridBuff[k][h+1];
				INT = (unsigned int)SUM-nPosYN;
				INT = (unsigned int)INT+nPosYN;
				Sprite_pos.y1 = INT;       		
				//y2
				SUM = (unsigned int)VGridBuff[k+1][h+1];
				INT = (unsigned int)SUM-nPosYN;
				INT = (unsigned int)INT+nPosYN;
				Sprite_pos.y2 = INT;			
				//y3
				SUM = (unsigned int)VGridBuff[k+1][h];
				INT = (unsigned int)SUM-nPosYN;
				INT = (unsigned int)INT+nPosYN;
				Sprite_pos.y3 = INT; 			
			#endif      
			sp_group_set(sprite_ex_attr,in->group_id);
			sp_interpolation_disable(sprite_ex_attr);		
			spriteV3D_Pos_Coordinate_Change(display_mode,coordinate_mode,(POS_STRUCT_PTR)&Sprite_pos);         
			gp_ppu_sprite_attribute_25d_position_set((SpN_RAM *)sp_num_addr,(POS_STRUCT_PTR)&Sprite_pos);
			sp_num_addr+=sizeof(SpN_RAM);
			if(GPSP_CMD_COMPARE())
			   sp_num_addr+=sizeof(SpN_RAM); 			
			sprite_ex_attr++;
			if(GPSP_CMD_COMPARE())
			   sprite_ex_attr++;	
		}
	}
	else
	{
	 #if ((SP_FRACTION_ENABLE == 1))  		
		//x0
		SUM = (FP32)Sprite_pos.x0;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.x0 = INT;	    
	    sp_frac_set(REM,sprite_ex_attr,0,0);		
		//x1
		SUM = (FP32)Sprite_pos.x1;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.x1 = INT;       	    
	    sp_frac_set(REM,sprite_ex_attr,0,1);	    
	    //x2
	    SUM = (FP32)Sprite_pos.x2;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.x2 = INT;	    
	    sp_frac_set(REM,sprite_ex_attr,0,2);		
		//x3
		SUM = (FP32)Sprite_pos.x3;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.x3 = INT;       	    
	    sp_frac_set(REM,sprite_ex_attr,0,3);	    
	    //y0
		SUM = (FP32)Sprite_pos.y0;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.y0 = INT;	    
	    sp_frac_set(REM,sprite_ex_attr,1,0);		
		//y1
		SUM = (FP32)Sprite_pos.y1;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.y1 = INT;       	    
	    sp_frac_set(REM,sprite_ex_attr,1,1);	    
	    //y2
	    SUM = (FP32)Sprite_pos.y2;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.y2 = INT;
	    sp_frac_set(REM,sprite_ex_attr,1,2);		
		//y3
		SUM = (FP32)Sprite_pos.y3;
		INT = (INT32U)SUM;
		REM = (FP32)SUM-INT;
	    Sprite_pos.y3 = INT;       	    
	    sp_frac_set(REM,sprite_ex_attr,1,3);
	    sp_interpolation_enable(sprite_ex_attr);	
	 #endif        
	    spriteV3D_Pos_Coordinate_Change(display_mode,coordinate_mode,(POS_STRUCT_PTR)&Sprite_pos);         
	    gp_ppu_sprite_attribute_25d_position_set((SpN_RAM *)sp_num_addr,(POS_STRUCT_PTR)&Sprite_pos);   	
	} 	 
       
}

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus PPU Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");