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
 
#include <linux/io.h>
#include <linux/module.h> 
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/hdreg.h> 		/* HDIO_GETGEO */
#include <linux/blkdev.h>
#include <mach/gp_ppu.h>
#include <mach/gp_gpio.h>
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

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void gp_ppu_text_number_array_update_flag_clear(void);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static unsigned char text_charnum_update_flag[4];
 /**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
 * @brief 		PPU text register flag set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @return 	SUCCESS/ERROR_ID.
*/
static void 
gp_ppu_text_set_update_reg_flag(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index
)
{
	// Notify PPU driver to update text registers
	if (text_index == C_PPU_TEXT1) {
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT1;
	} else if (text_index == C_PPU_TEXT2) {
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT2;
	} else if (text_index == C_PPU_TEXT3) {
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT3;
	} else if (text_index == C_PPU_TEXT4) {
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT4;
	}
}

/**
 * @brief 		PPU text initial function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_init(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	text_charnum_update_flag[text_index] = 1;

	//gp_memset((void *) &p_register_set->text[text_index], 0x0, sizeof(struct ppu_text_register_sets));

	if (text_index == C_PPU_TEXT3) {
		p_register_set->text3_25d_y_compress = 0x10;
	}
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_init);

/**
 * @brief 		PPU text enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_enable_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->text[text_index].control |= TXN_ENABLE;
	} else {
		p_register_set->text[text_index].control &= ~TXN_ENABLE;
	}

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_enable_set);

/**
 * @brief 		PPU text compress disable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=allow TEXT1 and TEXT2 to use horizontal/vertical compress 1=only allow 2D and rotate mode for TEXT1 and TEXT2.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_compress_disable_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= HVCMP_DISABLE;
	} else {
		p_register_set->ppu_enable &= ~HVCMP_DISABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_compress_disable_set);

/**
 * @brief 		PPU text mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0(2D) 1(HCMP/Rotate) 2(VCMP/2.5D) 3(HCMP+VCMP).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_mode_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>3) {
		return -ENOIOCTLCMD;
	}

	switch (text_index) {
	case C_PPU_TEXT2:
	case C_PPU_TEXT3:
		if (value > 2) {
			return -1;
		}
		break;
	case C_PPU_TEXT4:
		if (value > 1) {
			return -1;
		}
	}
	p_register_set->text[text_index].control &= ~MASK_TXN_MODE;
	p_register_set->text[text_index].control |= (value<<B_TXN_MODE) & MASK_TXN_MODE;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_mode_set);

/**
 * @brief 		PPU text direct mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=relative mode 1=direct address mode.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_direct_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= TX_DIRECT_ADDRESS;
	} else {
		p_register_set->ppu_enable &= ~TX_DIRECT_ADDRESS;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_direct_set_mode);

/**
 * @brief 		PPU text wallpaper mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_wallpaper_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->text[text_index].control |= TXN_WALL_ENABLE;
	} else {
		p_register_set->text[text_index].control &= ~TXN_WALL_ENABLE;
	}

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_wallpaper_set_mode);

/**
 * @brief 		PPU text attribute select mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=get attribut from TxN_A_PTR 1=get attribut from register.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_attribute_source_select(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	if (value) {		// Get TEXT attributes from register
		p_register_set->text[text_index].control |= TXN_REGMODE;
	} else {			// Get TEXT attributes from TXN_A_PTR
		p_register_set->text[text_index].control &= ~TXN_REGMODE;
	}

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_attribute_source_select);

/**
 * @brief 		PPU text horizontal move enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_horizontal_move_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->text[text_index].control |= TXN_MVE_ENABLE;
	} else {
		p_register_set->text[text_index].control &= ~TXN_MVE_ENABLE;
	}

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_horizontal_move_set_enable);

/**
 * @brief 		PPU text size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0(512x256) 1(512x512) ... 7(4096x4096).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_size_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>7) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].attribute &= ~MASK_TXN_SIZE;
	p_register_set->text[text_index].attribute |= (value<<B_TXN_SIZE) & MASK_TXN_SIZE;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_size_set);

/**
 * @brief 		PPU text character size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	txn_hs [in]:large_text_en=0: txn_hs:0:8,1:16,2:32,3:64, 1:0:32,1:64,2:128,3:256.
* @param 	txn_vs [in]:large_text_en=0: txn_vs:0:8,1:16,2:32,3:64, 1:0:32,1:64,2:128,3:256.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_character_size_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int txn_hs, 
unsigned int txn_vs
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || txn_hs>3 || txn_vs>3) {
		return -ENOIOCTLCMD;
	}

	// Character width
	p_register_set->text[text_index].attribute &= ~MASK_TXN_HS;
	p_register_set->text[text_index].attribute |= (txn_hs<<B_TXN_HS) & MASK_TXN_HS;
	// Character height
	p_register_set->text[text_index].attribute &= ~MASK_TXN_VS;
	p_register_set->text[text_index].attribute |= (txn_vs<<B_TXN_VS) & MASK_TXN_VS;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_character_size_set);

/**
 * @brief 		PPU text bitmap mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=character mode 1=bitmap mode.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_bitmap_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->text[text_index].control |= TXN_BMP;
	} else {
		p_register_set->text[text_index].control &= ~TXN_BMP;
	}

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_bitmap_set_mode);

/**
 * @brief 		PPU text color mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	rgb_mode:[in]: rgb_mode:0:palette mode,1:high color mode.
* @param 	color:[in]: color=0:1555,1:565,2:RGBG,3:YUYV.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_color_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int rgb_mode, 
unsigned int color
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || rgb_mode>1 || color>3) {
		return -ENOIOCTLCMD;
	}

	if (rgb_mode) {
		p_register_set->text[text_index].control |= TXN_RGB15P;
	} else {
		p_register_set->text[text_index].control &= ~TXN_RGB15P;
	}
	p_register_set->text[text_index].attribute &= ~MASK_TXN_COLOR;
	p_register_set->text[text_index].attribute |= (color<<B_TXN_COLOR) & MASK_TXN_COLOR;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_color_set);

/**
 * @brief 		PPU text palette mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	bank:[in]: bank:0~3.
* @param 	palette_idx:[in]: palette_idx:0~15.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_palette_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int bank, 
unsigned int palette_idx
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || bank>3 || palette_idx>15) {
		return -ENOIOCTLCMD;
	}

	// Set TEXT palette bank
	p_register_set->text[text_index].attribute &= ~MASK_TXN_PALETTE_BANK;
	p_register_set->text[text_index].attribute |= (bank<<B_TXN_PB) & MASK_TXN_PALETTE_BANK;
	// Set TEXT palette index
	p_register_set->text[text_index].attribute &= ~MASK_TXN_PALETTE;
	p_register_set->text[text_index].attribute |= (palette_idx<<B_TXN_PALETTE) & MASK_TXN_PALETTE;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_palette_set);

/**
 * @brief 		PPU text data segment set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_segment_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	unsigned int temp;
	
	if (!p_register_set || text_index>C_PPU_TEXT4 || (value&0x1)) {
		return -ENOIOCTLCMD;
	}
  
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
	p_register_set->text[text_index].segment = temp;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_segment_set);

/**
 * @brief 		PPU text attribute ram ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_attribute_array_set_ptr(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	unsigned int temp,temp1;
	
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
  temp1 = (unsigned int)gp_chunk_va((unsigned int)temp);
	p_register_set->text[text_index].a_ptr = temp1;
  p_register_set->text[text_index].a_ptr_pa = temp;
  
	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_attribute_array_set_ptr);

/**
 * @brief 		PPU text number ram ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_number_array_set_ptr(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	unsigned int temp,temp1;
	
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}
  
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
  temp1 = (unsigned int)gp_chunk_va((unsigned int)temp);
	p_register_set->text[text_index].n_ptr = temp1;
  p_register_set->text[text_index].n_ptr_pa = temp;
  
	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_number_array_set_ptr);

/**
 * @brief 		PPU text number ram ptr update flag clear set function.
* @return		None.
*/
void 
gp_ppu_text_number_array_update_flag_clear(
void
)
{
	text_charnum_update_flag[C_PPU_TEXT1] = 0;
	text_charnum_update_flag[C_PPU_TEXT2] = 0;
	text_charnum_update_flag[C_PPU_TEXT3] = 0;
	text_charnum_update_flag[C_PPU_TEXT4] = 0;
}
//EXPORT_SYMBOL(gp_ppu_text_number_array_update_flag_clear);

/**
 * @brief 		PPU text calculate number ram set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	photo_width:[in]: photo width.
* @param 	photo_height:[in]: photo height.
* @param 	data_ptr:[in]: data_ptr: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_calculate_number_array(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int photo_width, 
unsigned int photo_height, 
unsigned int data_ptr
)
{
	unsigned char text_size_idx, char_hs, char_vs;
	unsigned short text_width, text_height;
	unsigned char char_width, char_height;
	unsigned short text_char_count_x, text_char_count_y;
	unsigned short photo_char_count_x, photo_char_count_y;
	unsigned short *p16;
	unsigned int  *p32,temp;
	unsigned int  number_value;
	unsigned int  char_number_offset, bitmap_offset;
	unsigned short i, j;

	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}
  
  temp= (unsigned int)gp_user_va_to_pa((unsigned short *)data_ptr);
	text_size_idx = (p_register_set->text[text_index].attribute & MASK_TXN_SIZE) >> B_TXN_SIZE;		// TX_SIZE(0~7)
	char_hs = (p_register_set->text[text_index].attribute & MASK_TXN_HS) >> B_TXN_HS;		// TX_HS(0~3)
	char_vs = (p_register_set->text[text_index].attribute & MASK_TXN_VS) >> B_TXN_VS;		// TX_VS(0~3)
	if (text_size_idx < 0x4) {			// 512x256 ~ 1024x1024
		text_width = 1 << (9+(text_size_idx>>1));					// 512*(1<<(text_size_idx>>1))
		text_char_count_x = 1 << (6+(text_size_idx>>1)-char_hs);	// 512*(1<<(text_size_idx>>1))/(8<<TX_HS)
		if (text_size_idx & 0x1) {		// Text height is equal to text width
			text_height = text_width;
			text_char_count_y = 1 << (6+(text_size_idx>>1)-char_vs);		// 512*(1<<(text_size_idx>>1)) / (8<<TX_VS)
		} else {						// Text height is half of text width
			text_height = text_width >> 1;
			text_char_count_y = 1 << (5+(text_size_idx>>1)-char_vs);		// 256*(1<<(text_size_idx>>1)) / (8<<TX_VS)
		}

		char_width = 1 << (3+char_hs);
		char_height = 1 << (3+char_vs);

		photo_char_count_x = (photo_width+char_width-1) >> (3+char_hs);		// count = photo width / char width
		photo_char_count_y = (photo_height+char_height-1) >> (3+char_vs);	// count = photo height / char height

		char_number_offset = 1 << (3+3+char_hs+char_vs+1);		// Assume character size is char_width*char_height*2

	} else {		// 2048x1024 ~ 4096x4096
		text_width = 1 << (11+(text_size_idx>>1));					// 2048*(1<<(text_size_idx>>1))
		text_char_count_x = 1 << (6+(text_size_idx>>1)-char_hs);	// 2048*(1<<(text_size_idx>>1))/(32<<TX_HS)
		if (text_size_idx & 0x1) {		// Text height is equal to text width
			text_height = text_width;
			text_char_count_y = 1 << (6+(text_size_idx>>1)-char_vs);		// 2048*(1<<(text_size_idx>>1)) / (32<<TX_VS)
		} else {						// Text height is half of text width
			text_height = text_width >> 1;
			text_char_count_y = 1 << (5+(text_size_idx>>1)-char_vs);		// 1024*(1<<(text_size_idx>>1)) / (32<<TX_VS)
		}

		char_width = 1 << (5+char_hs);
		char_height = 1 << (5+char_vs);

		photo_char_count_x = (photo_width+char_width-1) >> (5+char_hs);		// count = photo width / char width
		photo_char_count_y = (photo_height+char_height-1) >> (5+char_vs);	// count = photo height / char height

		char_number_offset = 1 << (5+5+char_hs+char_vs+1);		// Assume character size is char_width*char_height*2
	}

	// First, check whether special bitmap mode is used
	if ((p_register_set->text[text_index].control&0x3)==0x3 && (p_register_set->text[text_index].attribute&MASK_TXN_HS)==MASK_TXN_HS) {
		// Special bitmap uses segment address as start address of pixel data. Character number array is not used.
		p_register_set->text[text_index].segment = temp;//data_ptr;

		return 0;
	} else if (p_register_set->text[text_index].control & TXN_BMP) {	// Second, handle bitmap mode
		p32 = (unsigned int *) p_register_set->text[text_index].n_ptr;
		if (!p32) {
			return -ENOIOCTLCMD;
		}

		if (p_register_set->text[text_index].control & TXN_WALL_ENABLE) {		// Wallpaper mode
			*p32 = temp;//data_ptr;

			return 0;
		} else {		// Normal bitmap mode
			bitmap_offset = photo_width;
			if (p_register_set->text[text_index].control & TXN_RGB15P) {		// YUYV or RGBG
				switch ((p_register_set->text[text_index].attribute & MASK_TXN_COLOR) >> B_TXN_COLOR) {
				case 0:	
					bitmap_offset <<= 1;		// Each pixel contains 2-byte data 
					break;
				case 1:		
					bitmap_offset <<= 1;		// Each pixel contains 2-byte data 
					break;
				case 2:		
				  if(p_register_set->ppu_misc & TEXT_RGBA_ENABLE)
				    bitmap_offset <<= 2;		// RGBA8888 color mode
				  else
				    bitmap_offset <<= 1;		// Each pixel contains 2-byte data  
					break;
				case 3:			// 8-bit color mode
				  bitmap_offset <<= 1;		// Each pixel contains 2-byte data  
					break;
				}			  			  
			} else {
				switch ((p_register_set->text[text_index].attribute & MASK_TXN_COLOR) >> B_TXN_COLOR) {
				case 0:			// 2-bit color mode
					bitmap_offset >>= 2;
					break;
				case 1:			// 4-bit color mode
					bitmap_offset >>= 1;
					break;
				case 2:			// 6-bit color mode
					bitmap_offset += (bitmap_offset>>1);	// +0.5
					bitmap_offset >>= 1;					// divided by 2
					break;
				case 3:			// 8-bit color mode
					break;
				}
			}

			if (((p_register_set->text[text_index].attribute & MASK_TXN_FLIP)>>B_TXN_FLIP)== 0x2) {		// Vertical flip
				number_value = temp + (photo_height-1)*bitmap_offset;//data_ptr + (photo_height-1)*bitmap_offset;
				for (i=0; i<text_height; i++) {
					*p32 = number_value;
					p32++;
					number_value -= bitmap_offset;
				}
			} else {
				number_value = temp;//data_ptr;
				for (i=0; i<text_height; i++) {
					*p32 = number_value;
					p32++;
					number_value += bitmap_offset;
				}
			}
			text_charnum_update_flag[text_index] = 1;
		}

		return 0;
	} else {		// Third, handle character mode
		// Direct address mode or RGB mode is enabled, character number is a 32-bit array
		if ((p_register_set->ppu_enable & TX_DIRECT_ADDRESS) || (p_register_set->text[text_index].control & TXN_RGB15P)) {
			unsigned int  *p32_x_start, *p32_y_start;
			unsigned char flip;

			p32 = (unsigned int  *) p_register_set->text[text_index].n_ptr;
			if (!p32) {
				return -ENOIOCTLCMD;
			}

			if (p_register_set->text[text_index].control & TXN_WALL_ENABLE) {		// Wallpaper mode
				//if (data_ptr >= p_register_set->text[text_index].segment) {
				if (temp >= p_register_set->text[text_index].segment) {
					*p32 = temp - p_register_set->text[text_index].segment;//data_ptr - p_register_set->text[text_index].segment;

					return 0;
				}

				return STATUS_FAIL;			// Segment value is incorrect
			} else {		// Normal character mode
				//if (data_ptr < p_register_set->text[text_index].segment) {
				if (temp < p_register_set->text[text_index].segment) {
					return -ENOIOCTLCMD;
				}

				if (!(p_register_set->text[text_index].control & TXN_RGB15P)) {
					// In the begining of this function, we assume color is 2-byte. Now we have to shrink char_number_offset here.
					switch ((p_register_set->text[text_index].attribute & MASK_TXN_COLOR) >> B_TXN_COLOR) {
					case 0:			// 2-bit color mode
						char_number_offset >>= 3;
						break;
					case 1:			// 4-bit color mode
						char_number_offset >>= 2;
						break;
					case 2:			// 6-bit color mode
						char_number_offset *= 3;
						char_number_offset >>= 3;
						break;
					case 3:			// 8-bit color mode
						char_number_offset >>= 1;
						break;
					}
				}

				number_value = temp - p_register_set->text[text_index].segment;//data_ptr - p_register_set->text[text_index].segment;
				p32_y_start = p32;
				flip = (p_register_set->text[text_index].attribute & MASK_TXN_FLIP) >> B_TXN_FLIP;
				for (j=0; j<text_char_count_y; j++) {
					if (flip==0x2 && j<photo_char_count_y) {
						p32_x_start = p32_y_start + (photo_char_count_y-1-j)*text_char_count_x;
					} else {
						p32_x_start = p32;
					}
					for (i=0; i<text_char_count_x; i++) {
						if (i<photo_char_count_x && j<photo_char_count_y) {
							if (flip == 0x1) {		// Horizontal flip
								*(p32_x_start+(photo_char_count_x-1-i)) = number_value;
							} else if (flip == 0x2) {		// Vertical flip
								*(p32_x_start + i) = number_value;
							} else {
								*p32 = number_value;
							}

							number_value += char_number_offset;
						} else {
							if (p_register_set->ppu_enable & CHAR0_TRANSPARENT_ENABLE) {	// Character 0 transparent mode
								*p32 = 0x0;
							} else {	// Use next character
								*p32 = number_value;
							}
						}
						p32++;
					}

					// If photo width is larger than TEXT width, we have to skip some photo character
					while (i < photo_char_count_x) {
						number_value += char_number_offset;
						i++;
					}
				}
				text_charnum_update_flag[text_index] = 1;
			}
			return 0;
		} else {	// Releative address mode and color is 2-bit or 4-bit or 6-bit, character number is a 16-bit array
			unsigned short *p16_x_start, *p16_y_start;
			unsigned char flip;

			//if (data_ptr < p_register_set->text[text_index].segment) {
			if (temp < p_register_set->text[text_index].segment) {
				return -ENOIOCTLCMD;
			}

			number_value = 	temp - p_register_set->text[text_index].segment;//data_ptr - p_register_set->text[text_index].segment;
			p16 = (unsigned short *) p_register_set->text[text_index].n_ptr;
			if (!p16) {
				return -ENOIOCTLCMD;
			}

			if (text_size_idx < 0x4) {
				i = 3 + 3 + char_hs + char_vs + 1;	// assume each pixel contains 2-bytes
			} else {
				i = 5 + 5 + char_hs + char_vs + 1;	// assume each pixel contains 2-bytes
			}
			// In the begining of this function, we assume color is 2-byte. Now we have to shrink char_number_offset here.
			switch ((p_register_set->text[text_index].attribute & MASK_TXN_COLOR) >> B_TXN_COLOR) {
			case 0:			// 2-bit color mode
				i -= 3;
				number_value >>= i;
				break;
			case 1:			// 4-bit color mode
				i -= 2;
				number_value >>= i;
				break;
			case 2:			// 6-bit color mode

				char_number_offset += (char_number_offset>>1);	// +0.5
				char_number_offset >>= 2;						// divided by 2 then divided by 2 again
				number_value /= char_number_offset;
				break;
			case 3:			// 8-bit color mode
				i -= 1;
				number_value >>= i;
				break;
			}

			p16_y_start = p16;
			flip = (p_register_set->text[text_index].attribute & MASK_TXN_FLIP) >> B_TXN_FLIP;
			for (j=0; j<text_char_count_y; j++) {
				if (flip==0x2 && j<photo_char_count_y) {
					p16_x_start = p16_y_start + (photo_char_count_y-1-j)*text_char_count_x;
				} else {
					p16_x_start = p16;
				}
				for (i=0; i<text_char_count_x; i++) {
					if (i<photo_char_count_x && j<photo_char_count_y) {
						if (flip == 0x1) {		// Horizontal flip
							*(p16_x_start+(photo_char_count_x-1-i)) = number_value;
						} else if (flip == 0x2) {		// Vertical flip
							*(p16_x_start + i) = number_value;
						} else {
							*p16 = number_value;
						}

						number_value++;
					} else {
						if (p_register_set->ppu_enable & CHAR0_TRANSPARENT_ENABLE) {	// Character 0 transparent mode
							*p16 = 0x0;
						} else {	// Use next character
							*p16 = number_value;
						}
					}
					p16++;
				}

				// If photo width is larger than TEXT width, we have to skip some photo character
				while (i < photo_char_count_x) {
					number_value++;
					i++;
				}
			}
			text_charnum_update_flag[text_index] = 1;
		}
	}

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_calculate_number_array);

/**
 * @brief 		PPU text position set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	pos_x:[in]: pos_x:0~0xFFF.
* @param 	pos_y:[in]: pos_y:0~0xFFF.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_position_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int pos_x, 
unsigned int pos_y
)
{

	//if (!p_register_set || text_index>C_PPU_TEXT4 || pos_x>0xFFF || pos_y>0xFFF) {
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].position_x = (unsigned short)(pos_x & 0xFFF);
	p_register_set->text[text_index].position_y = (unsigned short)(pos_y & 0xFFF);

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_position_set);

/**
 * @brief 		PPU text position offset set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	offset_x:[in]: pos_x:0~0xFFF.
* @param 	offset_y:[in]: pos_y:0~0xFFF.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_offset_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int offset_x, 
unsigned int offset_y
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].offset_x = (unsigned short) offset_x;
	p_register_set->text[text_index].offset_y = (unsigned short) offset_y;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_offset_set);

/**
 * @brief 		PPU text depth set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_depth_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index,
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>3) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].attribute &= ~MASK_TXN_DEPTH;
	p_register_set->text[text_index].attribute |= (value<<B_TXN_DEPTH) & MASK_TXN_DEPTH;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_depth_set);

/**
 * @brief 		PPU text blend set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	enable:[in]: enable:0~1.
* @param 	mode:[in]: mode:0(4 level) 1(64 level).
* @param 	value:[in]: value:0~3 or 0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_blend_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int enable, 
unsigned int mode, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || mode>1 || value>63) {
		return -ENOIOCTLCMD;
	}

	// Set blend enable bit
	if (enable) {		// enable blending function
		p_register_set->text[text_index].control |= TXN_BLD_ENABLE;
	} else {			// disable blending function
		p_register_set->text[text_index].control &= ~TXN_BLD_ENABLE;
	}
	// Set blend mode bit
	if (mode) {			// Use 64 level blending in TEXT control register
		p_register_set->text[text_index].control |= TXN_BLDMODE64_ENABLE;
	} else {			// Use a global 4 level blending value in P_Blending register
		p_register_set->text[text_index].control &= ~TXN_BLDMODE64_ENABLE;
	}
	// Set blend value bits
	p_register_set->text[text_index].control &= ~MASK_TXN_BLDLVL;
	p_register_set->text[text_index].control |= (value<<B_TXN_BLDLVL) & MASK_TXN_BLDLVL;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_blend_set);

/**
 * @brief 		PPU text flip set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0(no flip) 1(horizontal flip) 2(vertical flip) 3(h+v flip).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_flip_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>3) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].attribute &= ~MASK_TXN_FLIP;
	p_register_set->text[text_index].attribute |= (value<<B_TXN_FLIP) & MASK_TXN_FLIP;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_flip_set);

/**
 * @brief 		PPU text sin cosine set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	r_sine:[in]: r_sine:0~0x1FFF.
* @param 	r_cosine:[in]: r_cosine:0~0x1FFF.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_sine_cosine_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned short r_sine, 
unsigned short r_cosine
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].sine = (unsigned short) (r_sine & 0x1FFF);
	p_register_set->text[text_index].cosine = (unsigned short) (r_cosine & 0x1FFF);

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_sine_cosine_set);

/**
 * @brief 		PPU text window set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0(window1) 1(window2) 2(window3) 3(window4).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_window_select(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>3) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].attribute &= ~MASK_TXN_WINDOW;
	p_register_set->text[text_index].attribute |= (value<<B_TXN_WINDOW) & MASK_TXN_WINDOW;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_window_select);

/**
 * @brief 		PPU text special effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0(no effect) 1(negative color) 2(grayscale) 3(mono color).
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_special_effect_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>3) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].attribute &= ~MASK_TXN_SPECIAL_EFFECT;
	p_register_set->text[text_index].attribute |= (value<<B_TXN_SPECIAL_EFFECT) & MASK_TXN_SPECIAL_EFFECT;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_special_effect_set);

/**
 * @brief 		PPU text vertical compress set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0~0x3FF.
* @param 	offset:[in]: offset:0~0x3FF.
* @param 	step:[in]: step:0~0x3FF.
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_ppu_text_vertical_compress_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value, 
unsigned int offset, 
unsigned int step
)
{
	if (!p_register_set || value>0x1FF || offset>0x1FF || step>0x1FF) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_vcompress_value = (unsigned short) value;
	p_register_set->ppu_vcompress_offset = (unsigned short) offset;
	p_register_set->ppu_vcompress_step = (unsigned short) step;

	// Notify PPU driver to update text registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT1;

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_vertical_compress_set);

/**
 * @brief 		PPU text horizontal move ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to horizontal move control ram.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_horizontal_move_ptr_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_horizontal_move_ptr = value;

	// Notify PPU driver to update text registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_HORIZONTAL_MOVE;

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_horizontal_move_ptr_set);

/**
 * @brief 		PPU text1 horizontal compress ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to horizontal compress control ram.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text1_horizontal_compress_ptr_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_text1_hcompress_ptr = value;

	// Notify PPU driver to update text registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT1_HCOMPRESS;

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text1_horizontal_compress_ptr_set);

#if 0
static const signed short C_SIN_TABLE[] = {		//sine data table (0, 1,..., 359 degreed)
     0,     18,     36,     54,     71,     89,    107,    125,    143,    160,    178,    195,    213,    230,    248,
   265,    282,    299,    316,    333,    350,    367,    384,    400,    416,    433,    449,    465,    481,    496,
   512,    527,    543,    558,    573,    587,    602,    616,    630,    644,    658,    672,    685,    698,    711,
   724,    737,    749,    761,    773,    784,    796,    807,    818,    828,    839,    849,    859,    868,    878,
   887,    896,    904,    912,    920,    928,    935,    943,    949,    956,    962,    968,    974,    979,    984,
   989,    994,    998,   1002,   1005,   1008,   1011,   1014,   1016,   1018,   1020,   1022,   1023,   1023,   1024,
  1024,   1024,   1023,   1023,   1022,   1020,   1018,   1016,   1014,   1011,   1008,   1005,   1002,    998,    994,
   989,    984,    979,    974,    968,    962,    956,    949,    943,    935,    928,    920,    912,    904,    896,
   887,    878,    868,    859,    849,    839,    828,    818,    807,    796,    784,    773,    761,    749,    737,
   724,    711,    698,    685,    672,    658,    644,    630,    616,    602,    587,    573,    558,    543,    527,
   512,    496,    481,    465,    449,    433,    416,    400,    384,    367,    350,    333,    316,    299,    282,
   265,    248,    230,    213,    195,    178,    160,    143,    125,    107,     89,     71,     54,     36,     18,
     0,    -18,    -36,    -54,    -71,    -89,   -107,   -125,   -143,   -160,   -178,   -195,   -213,   -230,   -248,
  -265,   -282,   -299,   -316,   -333,   -350,   -367,   -384,   -400,   -416,   -433,   -449,   -465,   -481,   -496,
  -512,   -527,   -543,   -558,   -573,   -587,   -602,   -616,   -630,   -644,   -658,   -672,   -685,   -698,   -711,
  -724,   -737,   -749,   -761,   -773,   -784,   -796,   -807,   -818,   -828,   -839,   -849,   -859,   -868,   -878,
  -887,   -896,   -904,   -912,   -920,   -928,   -935,   -943,   -949,   -956,   -962,   -968,   -974,   -979,   -984,
  -989,   -994,   -998,  -1002,  -1005,  -1008,  -1011,  -1014,  -1016,  -1018,  -1020,  -1022,  -1023,  -1023,  -1024,
 -1024,  -1024,  -1023,  -1023,  -1022,  -1020,  -1018,  -1016,  -1014,  -1011,  -1008,  -1005,  -1002,   -998,   -994,
  -989,   -984,   -979,   -974,   -968,   -962,   -956,   -949,   -943,   -935,   -928,   -920,   -912,   -904,   -896,
  -887,   -878,   -868,   -859,   -849,   -839,   -828,   -818,   -807,   -796,   -784,   -773,   -761,   -749,   -737,
  -724,   -711,   -698,   -685,   -672,   -658,   -644,   -630,   -616,   -602,   -587,   -573,   -558,   -543,   -527,
  -512,   -496,   -481,   -465,   -449,   -433,   -416,   -400,   -384,   -367,   -350,   -333,   -316,   -299,   -282,
  -265,   -248,   -230,   -213,   -195,   -178,   -160,   -143,   -125,   -107,    -89,    -71,    -54,    -36,    -18
};

static const signed short C_COS_TABLE[] = {		//cosine data table (0, 1,..., 359 degreed)
	  1024,   1024,   1023,   1023,   1022,   1020,   1018,   1016,   1014,   1011,   1008,   1005,   1002,    998,    994,
	   989,    984,    979,    974,    968,    962,    956,    949,    943,    935,    928,    920,    912,    904,    896,
	   887,    878,    868,    859,    849,    839,    828,    818,    807,    796,    784,    773,    761,    749,    737,
	   724,    711,    698,    685,    672,    658,    644,    630,    616,    602,    587,    573,    558,    543,    527,
	   512,    496,    481,    465,    449,    433,    416,    400,    384,    367,    350,    333,    316,    299,    282,
	   265,    248,    230,    213,    195,    178,    160,    143,    125,    107,     89,     71,     54,     36,     18,
	     0,    -18,    -36,    -54,    -71,    -89,   -107,   -125,   -143,   -160,   -178,   -195,   -213,   -230,   -248,
	  -265,   -282,   -299,   -316,   -333,   -350,   -367,   -384,   -400,   -416,   -433,   -449,   -465,   -481,   -496,
	  -512,   -527,   -543,   -558,   -573,   -587,   -602,   -616,   -630,   -644,   -658,   -672,   -685,   -698,   -711,
	  -724,   -737,   -749,   -761,   -773,   -784,   -796,   -807,   -818,   -828,   -839,   -849,   -859,   -868,   -878,
	  -887,   -896,   -904,   -912,   -920,   -928,   -935,   -943,   -949,   -956,   -962,   -968,   -974,   -979,   -984,
	  -989,   -994,   -998,  -1002,  -1005,  -1008,  -1011,  -1014,  -1016,  -1018,  -1020,  -1022,  -1023,  -1023,  -1024,
	 -1024,  -1024,  -1023,  -1023,  -1022,  -1020,  -1018,  -1016,  -1014,  -1011,  -1008,  -1005,  -1002,   -998,   -994,
	  -989,   -984,   -979,   -974,   -968,   -962,   -956,   -949,   -943,   -935,   -928,   -920,   -912,   -904,   -896,
	  -887,   -878,   -868,   -859,   -849,   -839,   -828,   -818,   -807,   -796,   -784,   -773,   -761,   -749,   -737,
	  -724,   -711,   -698,   -685,   -672,   -658,   -644,   -630,   -616,   -602,   -587,   -573,   -558,   -543,   -527,
	  -512,   -496,   -481,   -465,   -449,   -433,   -416,   -400,   -384,   -367,   -350,   -333,   -316,   -299,   -282,
	  -265,   -248,   -230,   -213,   -195,   -178,   -160,   -143,   -125,   -107,    -89,    -71,    -54,    -36,    -18,
	     0,     18,     36,     54,     71,     89,    107,    125,    143,    160,    178,    195,    213,    230,    248,
	   265,    282,    299,    316,    333,    350,    367,    384,    400,    416,    433,    449,    465,    481,    496,
	   512,    527,    543,    558,    573,    587,    602,    616,    630,    644,    658,    672,    685,    698,    711,
	   724,    737,    749,    761,    773,    784,    796,    807,    818,    828,    839,    849,    859,    868,    878,
	   887,    896,    904,    912,    920,    928,    935,    943,    949,    956,    962,    968,    974,    979,    984,
	   989,    994,    998,   1002,   1005,   1008,   1011,   1014,   1016,   1018,   1020,   1022,   1023,   1023,   1024
};
#endif

/**
 * @brief 		PPU text rotate zoom set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	angle [in]: angle:0~359.
* @param 	factor_k [in]: factor_k: zoom factor.

* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_rotate_zoom_set(
PPU_REGISTER_SETS *p_register_set,
unsigned int text_index, 
signed short angle, 
signed short factor_k
)
{
	#if 0
	unsigned short r_sine, r_cosine;
	signed int value;
  #endif
  
	#if 0
	if (!p_register_set || text_index>C_PPU_TEXT4 || factor_k<1)
	#else
	if (!p_register_set || text_index>C_PPU_TEXT4)
	#endif
  {
		return -ENOIOCTLCMD;
	}

#if 1
  gp_ppu_text_sine_cosine_set(p_register_set, text_index, (unsigned short)angle, (unsigned short)factor_k);
#else
	while (angle >= 3600) {
		angle -= 3600;
	}
	while (angle >= 360) {
		angle -= 360;
	}
	while (angle <= -3600) {
		angle += 3600;
	}
	while (angle < 0) {
		angle += 360;
	}
	if (factor_k == 10) {
		value = (signed int) C_SIN_TABLE[angle];
	} else {
		value = (signed int) (C_SIN_TABLE[angle] / factor_k);
	}

	if (value > 0x7FF*64) {					// Out of maximum positive value
		r_sine = 0x17FF;
	} else if (value > 0x7FF) {				// 64 times boost must be used for positive value
		r_sine = (unsigned short) (value >> 6);
		r_sine |= 0x1000;					// enable 64 times burst
	} else if (value >= 0) {				// 1.999 >= r*cosine >= 0
		r_sine = (unsigned short) value;
	} else if (value >= -(0x7FF)) {			// 0 > r*cosine >= -1.999
		r_sine = (unsigned short) value;
		r_sine &= 0xFFF;
	} else if (value >= -(0x7FF*64)) {		// 64 times boost must be used for negative value
		r_sine = (unsigned short) (value >> 6);
		r_sine &= 0xFFF;
		r_sine |= 0x1000;
	} else {								// Out of maximum negative value
		r_sine = 0x1800;
	}

	if (factor_k == 10) {
		value = (signed int) C_COS_TABLE[angle];
	} else {
		value = (signed int) ( C_COS_TABLE[angle] / factor_k);
	}
	if (value > 0x7FF*64) {					// Out of maximum positive value
		r_cosine = 0x17FF;
	} else if (value > 0x7FF) {				// 64 times boost must be used for positive value
		r_cosine = (unsigned short) (value >> 6);
		r_cosine |= 0x1000;					// enable 64 times burst
	} else if (value >= 0) {				// 1.999 >= r*cosine >= 0
		r_cosine = (unsigned short) value;
	} else if (value >= -(0x7FF)) {			// 0 > r*cosine >= -1.999
		r_cosine = (unsigned short) value;
		r_cosine &= 0xFFF;
	} else if (value >= -(0x7FF*64)) {		// 64 times boost must be used for negative value
		r_cosine = (unsigned short) (value >> 6);
		r_cosine &= 0xFFF;
		r_cosine |= 0x1000;
	} else {								// Out of maximum negative value
		r_cosine = 0x1800;
	}
	
  #if 0
	gp_ppu_text_sine_cosine_set(p_register_set, text_index, (unsigned short) r_sine, (unsigned short) r_cosine);
  #else
	p_register_set->text[text_index].sine = (unsigned short) (r_sine & 0x1FFF);
	p_register_set->text[text_index].cosine = (unsigned short) (r_cosine & 0x1FFF);
	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);  
  #endif
#endif
  
	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_rotate_zoom_set);

/**
 * @brief 		PPU text3 25d set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	angle [in]: angle:0~359.
* @param 	factor_k [in]: factor_k: zoom factor of 240 line.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text3_25d_set(
PPU_REGISTER_SETS *p_register_set, 
signed short angle, 
signed short *factor_ptr
)
{
	#if 0
  unsigned short r_sine, r_cosine;
	signed int value;
	unsigned short *ptr;
	unsigned short i;
  #endif
 
	if (!p_register_set || !factor_ptr) {
		return -ENOIOCTLCMD;
	}
#if 1
	// Notify PPU driver to update text registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT3_25D;
#else
	while (angle >= 3600) {
		angle -= 3600;
	}
	while (angle >= 360) {
		angle -= 360;
	}
	while (angle <= -3600) {
		angle += 3600;
	}
	while (angle < 0) {
		angle += 360;
	}

	ptr = (unsigned short *) p_register_set->ppu_text3_25d_ptr;
	if (!ptr) {
		return -1;
	}
	for (i=0; i<240; i++) {
		value = (signed int) ( C_SIN_TABLE[angle] / *(factor_ptr+i));
		if (value > 0x7FF*64) {					// Out of maximum positive value
			r_sine = 0x17FF;
		} else if (value > 0x7FF) {				// 64 times boost must be used for positive value
			r_sine = (unsigned short) (value >> 6);
			r_sine |= 0x1000;					// enable 64 times burst
		} else if (value >= 0) {				// 1.999 >= r*cosine >= 0
			r_sine = (unsigned short) value;
		} else if (value >= -(0x7FF)) {			// 0 > r*cosine >= -1.999
			r_sine = (unsigned short) value;
			r_sine &= 0xFFF;
		} else if (value >= -(0x7FF*64)) {		// 64 times boost must be used for negative value
			r_sine = (unsigned short) (value >> 6);
			r_sine &= 0xFFF;
			r_sine |= 0x1000;
		} else {								// Out of maximum negative value
			r_sine = 0x1800;
		}

		value = (signed int) (C_COS_TABLE[angle] / *(factor_ptr+i));
		if (value > 0x7FF*64) {					// Out of maximum positive value
			r_cosine = 0x17FF;
		} else if (value > 0x7FF) {				// 64 times boost must be used for positive value
			r_cosine = (unsigned short) (value >> 6);
			r_cosine |= 0x1000;					// enable 64 times burst
		} else if (value >= 0) {				// 1.999 >= r*cosine >= 0
			r_cosine = (unsigned short) value;
		} else if (value >= -(0x7FF)) {			// 0 > r*cosine >= -1.999
			r_cosine = (unsigned short) value;
			r_cosine &= 0xFFF;
		} else if (value >= -(0x7FF*64)) {		// 64 times boost must be used for negative value
			r_cosine = (unsigned short) (value >> 6);
			r_cosine &= 0xFFF;
			r_cosine |= 0x1000;
		} else {								// Out of maximum negative value
			r_cosine = 0x1800;
		}

		*ptr++ = r_cosine;
		*ptr++ = r_sine;
	}

	// Notify PPU driver to update text registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT3_25D;
#endif

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text3_25d_set);

/**
 * @brief 		PPU text3 25d y compress set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 0~0x3F.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text3_25d_y_compress_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>0x3F) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text3_25d_y_compress = (unsigned short) value;

	// Notify PPU driver to update text registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_TEXT3;

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text3_25d_y_compress_set);

/**
 * @brief 		PPU text3 25d cossinebuf set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	CosSineBuf [in]: CosSineBuf: 32-bit buffer address.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text25D_cossinebuf_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int CosSineBuf
)
{
    #if 1
    unsigned int temp;
    #else
    unsigned int temp,temp1;
    #endif
    
    if (!p_register_set || !CosSineBuf)
		return -ENOIOCTLCMD;
	  
	  #if 1
    temp = (unsigned int)gp_user_va_to_pa((unsigned short *)CosSineBuf);    
    p_register_set->ppu_text3_25d_ptr=temp;
    p_register_set->ppu_text3_25d_ptr_va=CosSineBuf;
    #elif 0
    temp = (unsigned int)gp_user_va_to_pa((unsigned short *)CosSineBuf);
    temp1 = (unsigned int)gp_chunk_va((unsigned int)temp);
    p_register_set->ppu_text3_25d_ptr=temp1;
    #else
    p_register_set->ppu_text3_25d_ptr=CosSineBuf;    
    #endif
    
    return 0;
}
//EXPORT_SYMBOL(gp_ppu_text25D_cossinebuf_set);

/**
 * @brief 		PPU text special effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_interpolation_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->text[text_index].control |= TXN_INTP_ENABLE;
	} else {
		p_register_set->text[text_index].control &= ~TXN_INTP_ENABLE;
	}

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_interpolation_set_mode);

/**
 * @brief 		PPU text color mask set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_color_mask_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int text_index, 
unsigned int value
)
{
	if (!p_register_set || text_index>C_PPU_TEXT4 || value>3) {
		return -ENOIOCTLCMD;
	}

	p_register_set->text[text_index].attribute &= ~MASK_TXN_COLOR_MASK;
	p_register_set->text[text_index].attribute |= (value<<B_TXN_COLOR_MASK) & MASK_TXN_COLOR_MASK;

	// Notify PPU driver to update text registers
	gp_ppu_text_set_update_reg_flag(p_register_set, text_index);

	return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_color_mask_set_mode);

/**
 * @brief 		PPU text number array update function.
* @param 	text_info [in]: text information struct value set.
* @return 	NONE.
*/
signed int 
gp_ppu_text_number_array_update(
PnTX_Num_Arr text_info
)
{
     unsigned short TXaddr_mode;
     unsigned int NumX,NumY,xsize,ysize,IdxTab,ptIdx,i,temp;
     
     if(!text_info || !text_info->index_ptr || !text_info->Num_arr_ptr 
     || !text_info->nChar_width || !text_info->nChar_height || !text_info->nImage_width 
     || !text_info->nImage_height)
     {
         return -ENOIOCTLCMD;
     }
     #if 1
     temp=(unsigned int)gp_user_va_to_pa((unsigned short *)text_info->index_ptr);
     IdxTab=temp;
     temp=(unsigned int)gp_user_va_to_pa((unsigned short *)text_info->Num_arr_ptr);
     ptIdx=temp;     
     #else
     IdxTab=text_info->index_ptr;
     ptIdx=text_info->Num_arr_ptr;
     #endif
     temp = text_info->nImage_width % text_info->nChar_width;
     if(temp)
       NumX = (text_info->nImage_width / text_info->nChar_width)+1;	    //number of horizontal character in real text
     else
       NumX = text_info->nImage_width / text_info->nChar_width;	    //number of horizontal character in real text
     temp = text_info->nImage_height % text_info->nChar_height;
     if(temp)
       NumY = (text_info->nImage_height / text_info->nChar_height)+1;	    //number of vertical character in real text
     else  
	     NumY = text_info->nImage_height / text_info->nChar_height;	    //number of vertical character in real text
	   xsize = text_info->ntext_width / text_info->nChar_width;	    //number of horizontal character in number array					 
	   ysize = text_info->ntext_height / text_info->nChar_height;	    //number of vertical character in number array
	   TXaddr_mode=text_info->nTxAddr_mode;
     if(TXaddr_mode==0){
        for(i=0; i<ysize; i++) 	//Update number array
            gp_cpu_cpy_mem((unsigned int *)(ptIdx+(xsize*2)*i),(unsigned int  *)(IdxTab+(NumX*2)*i),(xsize*2),16);
     }else{
        for(i=0; i<ysize; i++) 	//Update number array
            gp_cpu_cpy_mem((unsigned int *)(ptIdx+(xsize*4)*i),(unsigned int  *)(IdxTab+(NumX*4)*i),(xsize*4),32);
     }             
     return 0;
}
//EXPORT_SYMBOL(gp_ppu_text_number_array_update);
