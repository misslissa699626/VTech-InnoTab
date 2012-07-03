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
 * @file    hal_ppu.c
 * @brief   Implement of PPU HAL API.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */
 
/*******************************************************************************
*                         H E A D E R   F I L E S
*******************************************************************************/
#include <linux/io.h>
#include <linux/module.h>
#include <mach/hal/regmap/reg_ppu.h>
#include <mach/hal/hal_ppu.h>
//#include <mach/gp_ppu_module_irq.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define DISPLAY_DEBUG                   1 
#define C_PPU_DRV_MAX_FRAME_NUM			    3
#define C_PPU_ENABLE					          (1 << 0)
#define C_PPU_VGA_ENABLE				        (1 << 4)
#define C_PPU_NON_INTERLACE				      (1 << 5)
#define C_PPU_DISPLAY_FRAME_MODE		    (1 << 7)
#define C_PPU_SP_COLOR_DITHER_ENABLE	  (1 << 17)
#define C_PPU_FRAME_FIFO_ENABLE	        (1 << 16)
#define C_PPU_FREESIZE_INTERLACE_ENABLE	(1 << 31)
#define PPU_TV_BUFFER_WAIT              0x8000      
#define PPU_TFT_BUFFER_WAIT             0x2000 

#if DISPLAY_DEBUG == 1
#define R_TFT_FBI_ADDR       			 (*((volatile UINT32 *) (PPU_BASE_REG+0x033C)))
#define R_TV_FBI_ADDR       			 (*((volatile UINT32 *) (PPU_BASE_REG+0x01E0)))
#define R_PPU_ENABLE        			 (*((volatile UINT32 *) (PPU_BASE_REG+0x01FC)))
#define R_SPRITE_RAM_ADDR       	 (PPU_BASE_REG+0x2000)
#define R_SPRITE_EXRAM_ADDR        (PPU_BASE_REG+0x6000)
#endif

#define PPU_SYSTEM_0               (*((volatile UINT32 *) 0xFC807004))
#define PPU_SYSTEM_1               (*((volatile UINT32 *) 0xFC807018))

#define	PPU_YUYV_TYPE3					    (3<<20)
#define	PPU_YUYV_TYPE2					    (2<<20)
#define	PPU_YUYV_TYPE1					    (1<<20)
#define	PPU_YUYV_TYPE0					    (0<<20)

#define	PPU_RGBG_TYPE3					    (3<<20)
#define	PPU_RGBG_TYPE2					    (2<<20)
#define	PPU_RGBG_TYPE1					    (1<<20)
#define	PPU_RGBG_TYPE0					    (0<<20)

#define PPU_LB							        (1<<19)
#define	PPU_YUYV_MODE					      (1<<10)
#define	PPU_RGBG_MODE			          (0<<10)

#define TFT_SIZE_800X480            (5<<16)
#define TFT_SIZE_720x480				    (4<<16)
#define TFT_SIZE_640X480            (1<<16)
#define TFT_SIZE_320X240            (0<<16)

#define	PPU_YUYV_RGBG_FORMAT_MODE		(1<<8)
#define	PPU_RGB565_MODE			        (0<<8)

#define	PPU_FRAME_BASE_MODE			    (1<<7)
#define	PPU_VGA_NONINTL_MODE			  (0<<5)

#define	PPU_VGA_MODE					      (1<<4)
#define	PPU_QVGA_MODE					      (0<<4) 

/**************************************************************************
*                          D A T A    T Y P E S
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#if 1
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

#define DERROR printk 
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static volatile UINT32 free_frame_buffer_array[C_PPU_DRV_MAX_FRAME_NUM];
static volatile UINT32 free_frame_buffer_array_va[C_PPU_DRV_MAX_FRAME_NUM];
static UINT8 free_fb_get_idx = 0;
static UINT8 free_fb_put_idx = 0;
static UINT8 display_fb_get_idx = 0;
static UINT8 display_fb_put_idx = 0;

static UINT8 line_mode_update_register_request = 0;
static volatile UINT8 update_register_done = 0;
static volatile UINT8 wait_ppu_frame_output_done = 0;
static UINT32 dma_current_register_set = 0;
static UINT32 ppu_current_frame = 0;
static UINT32 tft_tv_current_frame = 0;
static UINT8 ppu_frame_mode_busy = 0;
static UINT32 wait_done_flag = 0;
static UINT32 Display_temp_frame = 0;
static UINT32 ppu_irq_temp_flag = 0;
static UINT32 ppu_set_buffer_temp_flag = 0;
static volatile UINT8 ppu_frame_fifo_end = 0;
static volatile UINT8 fifo_go_ck = 0;
static PPU_REGISTER_SETS *new_register_sets_ptr;

static ppuFunIrq_t *pPPUIrqreg = (ppuFunIrq_t *)(PPU_BASE_REG);
static ppuSpriteReg_t *pPPUSpritereg = (ppuSpriteReg_t *)(PPU_BASE_REG);
static ppuText1Reg_t *pPPUText1reg = (ppuText1Reg_t *)(PPU_BASE_REG);
static ppuText2Reg_t *pPPUText2reg = (ppuText2Reg_t *)(PPU_BASE_REG);
static ppuText3Reg_t *pPPUText3reg = (ppuText3Reg_t *)(PPU_BASE_REG);
static ppuText4Reg_t *pPPUText4reg = (ppuText4Reg_t *)(PPU_BASE_REG);
static ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);
static ppustnFun_t *pPPUSTNFunreg = (ppustnFun_t *)(PPU_BASE_REG);
 /**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/


/**
* @brief	       ppu initial
* @param 	none
* @return 	none
*/
void gpHalPPUEn( int en )
{
	UINT32 i;

	if( en ) {
		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		for(i=0; i<C_PPU_DRV_MAX_FRAME_NUM; i++)
		{
			 free_frame_buffer_array[i] = 0x0;
			 free_frame_buffer_array_va[i] = 0x0;
		}
	
		free_fb_get_idx = 0;
		free_fb_put_idx = 0;
		display_fb_get_idx = 0;
		display_fb_put_idx = 0;
	
		line_mode_update_register_request = 0;
		update_register_done = 0;
	
		dma_current_register_set = 0;
		ppu_current_frame = 0;
		tft_tv_current_frame = 0;
		wait_ppu_frame_output_done = 0;
		ppu_frame_mode_busy = 0;
		Display_temp_frame = 0;  
		new_register_sets_ptr = NULL;	
		ppu_irq_temp_flag = 0;
		ppu_set_buffer_temp_flag = 0;  

		/*Enable PPU module*/
		pPPUFunreg->ppuEnable |= 0x1;

		pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_PPU_MASK;
		pPPUIrqreg->ppuIrqEn &= ~C_PPU_INT_EN_PPU_VBLANK;

		pPPUSTNFunreg->ppuStnEn = 0x1;                   //Frame mode irq occur in the ppu module.when TV or TTFT module not enable. 

		//PPU_SYSTEM_0 |=0x04380020;
		//PPU_SYSTEM_1 |=0x23000001;
	}
	else{
		pPPUIrqreg->ppuIrqState = 0;
		pPPUIrqreg->ppuIrqEn = 0;
		pPPUSTNFunreg->ppuStnEn = 0;    
		pPPUFunreg->ppuEnable = 0;
	}
}
EXPORT_SYMBOL(gpHalPPUEn);

/**
* @brief	       ppu irq mode
* @param 	enable[in]:0=disable,1=enable.
* @return 	none
*/
void gpHalPPUIrqCtrl(UINT32 enable)
{
     if(enable)
        pPPUIrqreg->ppuIrqEn |=C_PPU_INT_EN_PPU_VBLANK;
     else
        pPPUIrqreg->ppuIrqEn &=~C_PPU_INT_EN_PPU_VBLANK;   
}
EXPORT_SYMBOL(gpHalPPUIrqCtrl);

/**
* @brief	       added ppu frame buffer
* @param 	frame_buf[in]:frame buffer
* @return 	success=0,fail=-1.
*/
SINT32 gpHalPPUFramebufferAdd(UINT32 *frame_buf,UINT32 *frame_buf_va)
{
		UINT32 i,temp=0;
		
		if (!frame_buf || !frame_buf_va) {
			return -1;
		}
		
		#if 1
			for(i=0;i<C_PPU_DRV_MAX_FRAME_NUM;i++)
			{
				  temp = free_frame_buffer_array[i];
				  if(!temp)
				  {
				     free_fb_put_idx = i;
				     break;
				  }   
			}
			if(temp)
			  return -1;
			else
			{	 
			  free_frame_buffer_array[free_fb_put_idx] = (volatile UINT32) frame_buf;
			  free_frame_buffer_array_va[free_fb_put_idx] = (volatile UINT32) frame_buf_va;
			}
			//printk("ppu_set_addr = 0x%x\n",(signed int)frame_buf);
			#if 0
			if(free_fb_put_idx >= C_PPU_DRV_MAX_FRAME_NUM)
			{
				 free_fb_put_idx = 0;
			}
				
		  #endif
    #else 
	    free_frame_buffer_array[free_fb_put_idx] = (volatile UINT32) frame_buf;
			if(++free_fb_put_idx >= C_PPU_DRV_MAX_FRAME_NUM)
			{
				 free_fb_put_idx = 0;
			}
		#endif	
  	
  	#if 0
  	if(!tft_tv_current_frame) 
  	{
		   pPPUFunreg->ppuTvFbiAddr = (UINT32) frame_buf;
		   pPPUFunreg->ppuTftFbiAddr = (UINT32) frame_buf;
		   tft_tv_current_frame=1;	
  	}
  	#endif
  		
	  return 0;
}
EXPORT_SYMBOL(gpHalPPUFramebufferAdd);

/**
* @brief	  ppu frame buffer release
* @param 	  frame_buf[in]:frame buffer for release
* @return 	success=0,fail=-1.
*/
SINT32 gpHalPPUframebufferRelease(UINT32 frame_buf)
{

	 UINT32 i,temp;
		
	 if(!frame_buf){
			return -1;
	 }
			
	 for(i=0;i<C_PPU_DRV_MAX_FRAME_NUM;i++)
	 {
		  temp = free_frame_buffer_array[i];
		  if(temp == frame_buf)
		  {
		     free_frame_buffer_array[i]=0;
		     free_frame_buffer_array_va[i]=0;
		     return 0;
		  }
	 }	

	 return -1;
}
EXPORT_SYMBOL(gpHalPPUframebufferRelease);

/**
* @brief	  get ppu frame buffer
* @return 	buffer of ppu frame end.
*/
SINT32 gpHalPPUframebufferGet(void)
{
	UINT32 i,buffer_temp;

#if 0	
	if(!wait_ppu_frame_output_done)
	{ 
			if(Display_temp_frame)
			{
			   buffer_temp = Display_temp_frame;
			   Display_temp_frame = 0;
			   return buffer_temp;
			}
			else
			{
				 return -1;		 
			}
  }
		 
  else
  		return -2;
#else
			if(Display_temp_frame)
			{
				 for(i=0;i<C_PPU_DRV_MAX_FRAME_NUM;i++)
				 {
					  buffer_temp = free_frame_buffer_array[i];
					  if(buffer_temp == Display_temp_frame)
					  {
					     buffer_temp=free_frame_buffer_array_va[i];
			         Display_temp_frame = 0;
			   
			         return buffer_temp;
					  }
				 }
					  
	       return -1;			  
			}
			else
			{
				 return -2;		 
			}
#endif  		
}
EXPORT_SYMBOL(gpHalPPUframebufferGet);

/**
* @brief	       sprite ram move
* @param 	none
* @return 	none
*/

void gpHalSpriteramDmamove(void)
{
	UINT32 flag;
	#if CPU_MOVE_MODE == 1
	UINT32 *pPtr32Dest, *pPtr32Src;
	#endif

	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_SPRITE_ATTRIBUTE)
	{		
		dma_current_register_set = C_UPDATE_REG_SET_SPRITE_ATTRIBUTE;
		new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
	  #if CPU_MOVE_MODE == 1
	     pPtr32Dest = (unsigned int *)R_SPRITE_RAM_ADDR;
       pPtr32Src = (unsigned int *)new_register_sets_ptr->ppu_sprite_attribute_ptr;
       #if 1
       	for (i=0;i<(0x7FF/sizeof (unsigned int));i++)
       #else
       	for (i=0;i<8;i++)
       #endif	
       	{
           *(pPtr32Dest++) = *(pPtr32Src++);
       	}
	  #else
	    #if 0
	    	pPPUSpritereg->ppuSpriteDmaTarg=(UINT32) (pPPUSpritereg->ppuSpriteramAttribute & 0x7FFF);
	    #else
	    	pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x2000;
	    #endif
	    pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_sprite_attribute_ptr;
			if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
			{
				if ((new_register_sets_ptr->sprite_control & 0xFF00) == 0)  {
					  pPPUSpritereg->ppuSpriteDmaNum = 0xFFF;            // 1024 sprites are used
				} else if (new_register_sets_ptr->sprite_control & C_PPU_SP_COLOR_DITHER_ENABLE) {
					// If color dithering mode is used, only 512 sprites are supported and sprite size is doubled
					pPPUSpritereg->ppuSpriteDmaNum = ((new_register_sets_ptr->sprite_control & 0x7F00)>>3) - 1;		// dma number = (sprite number * 4) * 32 / 4 - 1
				} else {
					pPPUSpritereg->ppuSpriteDmaNum = ((new_register_sets_ptr->sprite_control & 0xFF00)>>4) - 1;		// dma number = (sprite number * 4) * 16 / 4 - 1
				}
				while(pPPUSpritereg->ppuSpriteDmaNum!=0);
			}
		#endif
	}

	if (flag & C_UPDATE_REG_SET_SPRITE_EX_ATTRIBUTE)
	{
		dma_current_register_set = C_UPDATE_REG_SET_SPRITE_EX_ATTRIBUTE;
	  new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
	  #if CPU_MOVE_MODE == 1	
	     pPtr32Dest = (unsigned int *)R_SPRITE_EXRAM_ADDR;
       pPtr32Src = (unsigned int *)new_register_sets_ptr->ppu_sprite_ex_attribute_ptr;
       #if 1
       	for (i=0;i<(0x1FF/sizeof (unsigned int));i++)
       #else
       	for (i=0;i<8;i++)
       #endif
       	{	
           *(pPtr32Dest++) = *(pPtr32Src++);
       	}	    
	  #else
	    #if 0
	    	pPPUSpritereg->ppuSpriteDmaTarg=(UINT32) (pPPUSpritereg->ppuSpriteramExattribute & 0x7FFF);
	    #else
	    	pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x6000;
	    #endif
	    pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_sprite_ex_attribute_ptr;
			if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
			{
				if ((new_register_sets_ptr->sprite_control & 0xFF00) == 0)			
				        pPPUSpritereg->ppuSpriteDmaNum = 0x400-1;
				else
				        pPPUSpritereg->ppuSpriteDmaNum = ((new_register_sets_ptr->sprite_control & 0xFF00)>>6) - 1;		// dma number = (sprite number * 4) * 16 / 4 - 1
				while(pPPUSpritereg->ppuSpriteDmaNum!=0);
			}
		#endif
	}
	
}

/**
* @brief	       text1 horizontal compress ram move
* @param 	none
* @return 	none
*/
void gpHalText1horizontalcompressDmamove(void)
{
	UINT32 flag;
       //ppuText1Reg_t *pPPUText1reg = (ppuText1Reg_t *)(PPU_BASE_REG);
	//ppuSpriteReg_t *pPPUSpritereg = (ppuSpriteReg_t *)(PPU_BASE_REG);   

	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_HORIZONTAL_MOVE)
	{
		dma_current_register_set = C_UPDATE_REG_SET_HORIZONTAL_MOVE;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUText1reg->ppuText1HOffset & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x400;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_horizontal_move_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0xF0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}
	
	if (flag & C_UPDATE_REG_SET_TEXT1_HCOMPRESS)
	{
		dma_current_register_set = C_UPDATE_REG_SET_TEXT1_HCOMPRESS;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUText1reg->ppuText1HCompValue & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x800;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_text1_hcompress_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0xF0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}
}



/**
* @brief	       text3 25d ram move
* @param 	none
* @return 	none
*/
void gpHalText325dDmamove(void)
{
	UINT32 flag;
       //ppuText3Reg_t *pPPUText3reg = (ppuText3Reg_t *)(PPU_BASE_REG);
	//ppuSpriteReg_t *pPPUSpritereg = (ppuSpriteReg_t *)(PPU_BASE_REG);   

	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_TEXT3_25D)
	{
		//printk("ppu demo start!\n");
		dma_current_register_set = C_UPDATE_REG_SET_TEXT3_25D;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUText3reg->ppuText3Cosine & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x800;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_text3_25d_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0x1E0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}
}

/**
* @brief	       text3 palette ram move
* @param 	none
* @return 	none
*/
void gpHalPaletteramDmamove(void)
{
	UINT32 flag;
       //ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);
	//ppuSpriteReg_t *pPPUSpritereg = (ppuSpriteReg_t *)(PPU_BASE_REG);   

	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_PALETTE0)
	{
		dma_current_register_set = C_UPDATE_REG_SET_PALETTE0;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUFunreg->ppuPaletteRam0 & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x1000;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_palette0_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0x1E0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}

	if (flag & C_UPDATE_REG_SET_PALETTE1)
	{
		dma_current_register_set = C_UPDATE_REG_SET_PALETTE1;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUFunreg->ppuPaletteRam1 & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x1400;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_palette1_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0x1E0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}

	if (flag & C_UPDATE_REG_SET_PALETTE2)
	{
		dma_current_register_set = C_UPDATE_REG_SET_PALETTE2;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUFunreg->ppuPaletteRam2 & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x1800;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_palette2_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0x1E0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}

	if (flag & C_UPDATE_REG_SET_PALETTE3)
	{
		dma_current_register_set = C_UPDATE_REG_SET_PALETTE3;
	       new_register_sets_ptr->update_register_flag &= ~dma_current_register_set;	// Clear DMA flag that is done
              #if 0
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)(pPPUFunreg->ppuPaletteRam3 & 0x7FFF);
              #else
              pPPUSpritereg->ppuSpriteDmaTarg=(UINT32)0x1C00;
              #endif
              pPPUSpritereg->ppuSpriteDmaSrc= new_register_sets_ptr->ppu_palette3_ptr;
		if (pPPUSpritereg->ppuSpriteDmaTarg != pPPUSpritereg->ppuSpriteDmaSrc) 
		{		
			pPPUSpritereg->ppuSpriteDmaNum = 0x1E0 - 1;
			while(pPPUSpritereg->ppuSpriteDmaNum!=0);
		}
	}	
	
}


/**
* @brief	       ppu register set
* @param 	none
* @return 	none
*/
void gpHalPPURegCpumove(void)
{
	UINT32 flag;
       //ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);
	//ppuSpriteReg_t *pPPUSpritereg = (ppuSpriteReg_t *)(PPU_BASE_REG);      

	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_PPU) {
		pPPUFunreg->ppuBlending = (UINT32)new_register_sets_ptr->ppu_blending_level;
		pPPUFunreg->ppuFadeCtrl = (UINT32)new_register_sets_ptr->ppu_fade_control;
		pPPUFunreg->ppuPaletteCtrl = (UINT32)new_register_sets_ptr->ppu_palette_control;
		pPPUFunreg->ppuBldColor = (UINT32)new_register_sets_ptr->ppu_rgb565_transparent_color;
		pPPUFunreg->ppuEnable = (UINT32)new_register_sets_ptr->ppu_enable;
		pPPUFunreg->ppuMisc = (UINT32)new_register_sets_ptr->ppu_misc;                                  
		pPPUFunreg->ppuFreesize = (UINT32)new_register_sets_ptr->ppu_free_mode;                           
		pPPUFunreg->ppuFbfifoSetup = (UINT32)new_register_sets_ptr->ppu_frame_buffer_fifo;                	
		pPPUFunreg->ppuWindow0X = (UINT32)new_register_sets_ptr->ppu_window1_x;
		pPPUFunreg->ppuWindow0Y = (UINT32)new_register_sets_ptr->ppu_window1_y;
		pPPUFunreg->ppuWindow1X = (UINT32)new_register_sets_ptr->ppu_window2_x;
		pPPUFunreg->ppuWindow1Y = (UINT32)new_register_sets_ptr->ppu_window2_y;
		pPPUFunreg->ppuWindow2X = (UINT32)new_register_sets_ptr->ppu_window3_x;
		pPPUFunreg->ppuWindow2Y = (UINT32)new_register_sets_ptr->ppu_window3_y;
		pPPUFunreg->ppuWindow3X = (UINT32)new_register_sets_ptr->ppu_window4_x;
		pPPUFunreg->ppuWindow3Y = (UINT32)new_register_sets_ptr->ppu_window4_y;
		pPPUFunreg->ppuVcompValue = (UINT32)new_register_sets_ptr->ppu_vcompress_value;
		pPPUFunreg->ppuVcompOffset = (UINT32)new_register_sets_ptr->ppu_vcompress_offset;
		pPPUFunreg->ppuVcompStep = (UINT32)new_register_sets_ptr->ppu_vcompress_step;		
		pPPUSpritereg->ppuSpriteRgbOffset = (UINT32)new_register_sets_ptr->ppu_special_effect_rgb;
		
	}
	
}

/**
* @brief	       text1 register set
* @param 	none
* @return 	none
*/
void gpHalText1RegCpumove(void)
{
	UINT32 flag;
       //ppuText1Reg_t *pPPUText1reg = (ppuText1Reg_t *)(PPU_BASE_REG);
    
	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_TEXT1) {
		new_register_sets_ptr->update_register_flag &=~C_UPDATE_REG_SET_TEXT1;
		pPPUText1reg->ppuText1XPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].position_x;
		pPPUText1reg->ppuText1YPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].position_y;
		pPPUText1reg->ppuText1XOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].offset_x;
		pPPUText1reg->ppuText1YOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].offset_y;
		pPPUText1reg->ppuText1Attr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].attribute;
		pPPUText1reg->ppuText1Ctrl = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].control;
		pPPUText1reg->ppuText1NumPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].n_ptr_pa;
		pPPUText1reg->ppuText1AttrPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].a_ptr_pa;
		pPPUText1reg->ppuText1Sine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].sine;
		pPPUText1reg->ppuText1Cosine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].cosine;
		pPPUText1reg->ppuText1Segment = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT1].segment;
	}
	
}


/**
* @brief	       text2 register set
* @param 	none
* @return 	none
*/
void gpHalText2RegCpumove(void)
{
	UINT32 flag;
       //ppuText2Reg_t *pPPUText2reg = (ppuText2Reg_t *)(PPU_BASE_REG);
    
	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_TEXT2) {
		new_register_sets_ptr->update_register_flag &=~C_UPDATE_REG_SET_TEXT2;
		pPPUText2reg->ppuText2XPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].position_x;
		pPPUText2reg->ppuText2YPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].position_y;
		pPPUText2reg->ppuText2XOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].offset_x;
		pPPUText2reg->ppuText2YOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].offset_y;
		pPPUText2reg->ppuText2Attr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].attribute;
		pPPUText2reg->ppuText2Ctrl = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].control;
		pPPUText2reg->ppuText2NumPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].n_ptr_pa;
		pPPUText2reg->ppuText2AttrPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].a_ptr_pa;
		pPPUText2reg->ppuText2Sine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].sine;
		pPPUText2reg->ppuText2Cosine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].cosine;
		pPPUText2reg->ppuText2Segment = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT2].segment;
	}
	
}

/**
* @brief	       text3 register set
* @param 	none
* @return 	none
*/
void gpHalText3RegCpumove(void)
{
	UINT32 flag;
       //ppuText3Reg_t *pPPUText3reg = (ppuText3Reg_t *)(PPU_BASE_REG);
    
	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_TEXT3) {
		new_register_sets_ptr->update_register_flag &=~C_UPDATE_REG_SET_TEXT3;
		pPPUText3reg->ppuText3XPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].position_x;
		pPPUText3reg->ppuText3YPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].position_y;
		pPPUText3reg->ppuText3XOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].offset_x;
		pPPUText3reg->ppuText3YOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].offset_y;
		pPPUText3reg->ppuText3Attr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].attribute;
		pPPUText3reg->ppuText3Ctrl = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].control;
		pPPUText3reg->ppuText3NumPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].n_ptr_pa;
		pPPUText3reg->ppuText3AttrPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].a_ptr_pa;
		if(!(pPPUText3reg->ppuText3Ctrl & C_PPU_TEXT3_V25D_EN))
		{
		      pPPUText3reg->ppuText3Sine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].sine;
		      pPPUText3reg->ppuText3Cosine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].cosine;
		}
		pPPUText3reg->ppuText3Segment = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT3].segment;
		pPPUText3reg->ppuText3YComp = (UINT32)new_register_sets_ptr->text3_25d_y_compress;
	}
	
}

/**
* @brief	       text4 register set
* @param 	none
* @return 	none
*/
void gpHalText4RegCpumove(void)
{
	UINT32 flag;
       //ppuText4Reg_t *pPPUText4reg = (ppuText4Reg_t *)(PPU_BASE_REG);
    
	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_TEXT4) {
		new_register_sets_ptr->update_register_flag &=~C_UPDATE_REG_SET_TEXT4;
		pPPUText4reg->ppuText4XPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].position_x;
		pPPUText4reg->ppuText4YPos = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].position_y;
		pPPUText4reg->ppuText4XOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].offset_x;
		pPPUText4reg->ppuText4YOffset = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].offset_y;
		pPPUText4reg->ppuText4Attr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].attribute;
		pPPUText4reg->ppuText4Ctrl = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].control;
		pPPUText4reg->ppuText4NumPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].n_ptr_pa;
		pPPUText4reg->ppuText4AttrPtr = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].a_ptr_pa;
		pPPUText4reg->ppuText4Sine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].sine;
	       pPPUText4reg->ppuText4Cosine = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].cosine;
		pPPUText4reg->ppuText4Segment = (UINT32)new_register_sets_ptr->text[C_PPU_TEXT4].segment;
	}
	
}


/**
* @brief	       sprite register set
* @param 	none
* @return 	none
*/
void gpHalSpriteRegCpumove(void)
{
	UINT32 flag;
       //ppuSpriteReg_t *pPPUSpritereg = (ppuSpriteReg_t *)(PPU_BASE_REG);
    
	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_SPRITE) {
		new_register_sets_ptr->update_register_flag &=~C_UPDATE_REG_SET_SPRITE;
		pPPUSpritereg->ppuSpriteCtrl = new_register_sets_ptr->sprite_control;
		pPPUSpritereg->ppuSpriteSegment = new_register_sets_ptr->sprite_segment;
		pPPUSpritereg->ppuExspriteCtrl = new_register_sets_ptr->extend_sprite_control;
		pPPUSpritereg->ppuExspriteAddr = new_register_sets_ptr->extend_sprite_addr;
	}
	
}

/**
* @brief	       sprite register set
* @param 	none
* @return 	none
*/
void gpHalColormappingramCpumove(void)
{
	UINT32 flag;
	UINT32 *src_ptr;
	volatile UINT32 *dest_ptr;
	UINT32 i;	
       //ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);
    
	if (!new_register_sets_ptr) {
		return;
	}
	flag = new_register_sets_ptr->update_register_flag;

	if (flag & C_UPDATE_REG_SET_COLOR) {
		new_register_sets_ptr->update_register_flag &=~C_UPDATE_REG_SET_COLOR;
		if (new_register_sets_ptr->color_mapping_ptr && new_register_sets_ptr->color_mapping_ptr!=(UINT32) pPPUFunreg->ppuColormappingRam) {
			src_ptr = (UINT32 *) (new_register_sets_ptr->color_mapping_ptr & ~(0x3));
			dest_ptr = (volatile UINT32 *) (pPPUFunreg->ppuColormappingRam);
			for (i=0; i<768; i++) {
				*dest_ptr++ = *src_ptr++;
			}
		}
	}
	
}

/**
* @brief	       ppu module register set
* @param 	none
* @return 	success=0,fail=-1.
*/
SINT32 gpHalUpdatePPURegisters(void)
{
	if (!new_register_sets_ptr) {
		return STATUS_FAIL;
	}
  gpHalPPURegCpumove();
	gpHalSpriteRegCpumove(); 
	gpHalText1RegCpumove();
	gpHalText2RegCpumove();
	gpHalText3RegCpumove();
	gpHalText4RegCpumove();	 
	gpHalSpriteramDmamove();
	gpHalText1horizontalcompressDmamove();
	gpHalText325dDmamove();
	gpHalPaletteramDmamove();
	gpHalColormappingramCpumove();

	return STATUS_OK;
	
}


/**
* @brief	       ppu module register set
* @param 	wait_available[in]:0:without check ppu done. 1:check ppu done
* @param 	wait_done[in]:0:without wait ppu frame end.1:wait ppu frame end.
* @return 	success=0,fail=-1.
*/
SINT32 gpHalPPUGo(PPU_REGISTER_SETS *p_register_set, UINT32 wait_available, UINT32 wait_done)
{
  UINT32 buffer_ck_counter=0;
  
	if (!p_register_set) {
		return STATUS_FAIL;
	}

#if 1
	if(++free_fb_get_idx >= C_PPU_DRV_MAX_FRAME_NUM)
	{
		 free_fb_get_idx = 0;
	}
	if (p_register_set->ppu_enable & C_PPU_DISPLAY_FRAME_MODE) {		// Frame base mode
		// Obtain a frame from free_frame_buffer_array
		if(++free_fb_get_idx >= C_PPU_DRV_MAX_FRAME_NUM)
		{
			 free_fb_get_idx = 0;
		}		
		while(!(ppu_current_frame = free_frame_buffer_array[free_fb_get_idx])) 
		{
				if(++free_fb_get_idx >= C_PPU_DRV_MAX_FRAME_NUM)
				{
					 free_fb_get_idx = 0;
				}
				if(++buffer_ck_counter > C_PPU_DRV_MAX_FRAME_NUM*2)
				   return STATUS_FAIL;			
		}	
		//printk("ppu_set_addr = 0x%x\n",(signed int)ppu_current_frame);	
		// Wait until previous PPU operation is done
    if(wait_available)
    {
		   #if 0
		   if(wait_done_flag)
		   {
		       wait_done_flag=0;
		       while (wait_ppu_frame_output_done);
			     new_register_sets_ptr = NULL;  
			 }
			 #endif
  	}
		new_register_sets_ptr = p_register_set;

		pPPUFunreg->ppuFboAddr = ppu_current_frame;
    #if 1
    if(!ppu_set_buffer_temp_flag)
    {
       Display_temp_frame = ppu_current_frame;
       ppu_set_buffer_temp_flag=1;
    }
    #endif
    
		// Set registers
		if (gpHalUpdatePPURegisters())
		{
				new_register_sets_ptr = NULL;
				return STATUS_FAIL;
		}

		// Start PPU engine
		wait_ppu_frame_output_done = 1;
		ppu_frame_mode_busy = 1;
		#if PPU_FRAME_REGISTER_WAIT == 1
		  if(pPPUIrqreg->ppuIrqEn & PPU_TV_BUFFER_WAIT)
		  {
		    while((pPPUFunreg->ppuFbGo & PPU_TV_BUFFER_WAIT)==0);
		       pPPUFunreg->ppuFbGo = 0x1;
		  }
		  else if(pPPUIrqreg->ppuIrqEn & PPU_FRAME_REGISTER_WAIT)
		  {
		    while((pPPUFunreg->ppuFbGo & PPU_TFT_BUFFER_WAIT)==0);
		       pPPUFunreg->ppuFbGo = 0x1;
		  }
		  else
		  	pPPUFunreg->ppuFbGo = 0x1;
		#else
        pPPUFunreg->ppuFbGo = 0x1;
		#endif
    wait_done_flag=1;
		// Wait until PPU operation is done
		if (wait_done) {
			#if 0
			while (wait_ppu_frame_output_done);			  
			wait_done_flag=0;
			new_register_sets_ptr = NULL;
			#elif 0
	      while((pPPUIrqreg->ppuIrqState & 0x1) == 0);
		          pPPUIrqreg->ppuIrqState = 0x1;
			  if(!(p_register_set->ppu_enable & 0x40)) 
			  {	    // VGA Interlace mode
						if(pPPUFunreg->ppuFbGo & (1<<14)) 
						{		// If FBO_F bit is 1, it means only even line is done.
							 pPPUFunreg->ppuFbGo = 0x1;			// PPU will start calculating odd line after PPU_GO bit is set.
	             while((pPPUIrqreg->ppuIrqState & 0x1) == 0);
		                pPPUIrqreg->ppuIrqState = 0x1;
						}
			  }
			  while((pPPUIrqreg->ppuIrqState & C_PPU_INT_PEND_TV_VBLANK) == 0);
			       pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_VBLANK;		
	      //while((pPPUFunreg->ppuFbGo & 0x8000) == 0);        			
			  //pPPUFunreg->ppuTvFbiAddr = (UINT32) ppu_current_frame;
			  ppu_current_frame=0;
			  wait_done_flag=0;
			  new_register_sets_ptr = NULL;			  			
			#endif
		}
	} 
	else 
	{			
		printk("gplib_ppu_go_line_mode !\n");
		// Line mode
		new_register_sets_ptr = p_register_set;

		// Set register update flag
		line_mode_update_register_request = 1;

		// Wait until register update is done
		while (!update_register_done){
			;
		}
		update_register_done = 0;
		new_register_sets_ptr = NULL;
		wait_ppu_frame_output_done = 0;
		ppu_frame_mode_busy = 0;	
	}
#else	
	if (p_register_set->ppu_enable & C_PPU_DISPLAY_FRAME_MODE) {		// Frame base mode
		// Obtain a frame from free_frame_buffer_array
		#if 1
		while(!(ppu_current_frame = free_frame_buffer_array[free_fb_get_idx])) 
		{
				if(++free_fb_get_idx >= C_PPU_DRV_MAX_FRAME_NUM)
				{
					 free_fb_get_idx = 0;
				}
				if(++buffer_ck_counter > C_PPU_DRV_MAX_FRAME_NUM*2)
				   return STATUS_FAIL;			
		}		
		#else
		while (!(ppu_current_frame = free_frame_buffer_array[free_fb_get_idx])) {
			;
		}
		#endif
		if (++free_fb_get_idx >= C_PPU_DRV_MAX_FRAME_NUM) {
			free_fb_get_idx = 0;
		}

		// Wait until previous PPU operation is done
    if(wait_available)
    {
		   if(wait_done_flag)
		   {
		       wait_done_flag=0;
		       while (wait_ppu_frame_output_done);
			     new_register_sets_ptr = NULL;  
			 }
  	}
		new_register_sets_ptr = p_register_set;

		pPPUFunreg->ppuFboAddr = ppu_current_frame;

		// Set registers
		if (gpHalUpdatePPURegisters())
		{
				#if 0
				free_frame_buffer_array[free_fb_put_idx] = ppu_current_frame;
				if (++free_fb_put_idx >= C_PPU_DRV_MAX_FRAME_NUM) {
					free_fb_put_idx = 0;
				}
				#endif
				new_register_sets_ptr = NULL;
				return STATUS_FAIL;
		}

		// Start PPU engine
		wait_ppu_frame_output_done = 1;
		ppu_frame_mode_busy = 1;
		#if PPU_FRAME_REGISTER_WAIT == 1
		  if(pPPUIrqreg->ppuIrqEn & PPU_TV_BUFFER_WAIT)
		  {
		    while((pPPUFunreg->ppuFbGo & PPU_TV_BUFFER_WAIT)==0);
		       pPPUFunreg->ppuFbGo = 0x1;
		  }
		  else if(pPPUIrqreg->ppuIrqEn & PPU_TFT_BUFFER_WAIT)
		  {
		    while((pPPUFunreg->ppuFbGo & PPU_TFT_BUFFER_WAIT)==0);
		       pPPUFunreg->ppuFbGo = 0x1;
		  }
		  else
		  	pPPUFunreg->ppuFbGo = 0x1;
		#else
        pPPUFunreg->ppuFbGo = 0x1;
		#endif
    wait_done_flag=1;
		// Wait until PPU operation is done
		if (wait_done) {
      #if 0
			  while (wait_ppu_frame_output_done);			  
      #else
	      while((pPPUIrqreg->ppuIrqState & 0x1) == 0);
		          pPPUIrqreg->ppuIrqState = 0x1;
			  if(!(p_register_set->ppu_enable & 0x40)) 
			  {	    // VGA Interlace mode
						if(pPPUFunreg->ppuFbGo & (1<<14)) 
						{		// If FBO_F bit is 1, it means only even line is done.
							 pPPUFunreg->ppuFbGo = 0x1;			// PPU will start calculating odd line after PPU_GO bit is set.
	             while((pPPUIrqreg->ppuIrqState & 0x1) == 0);
		                pPPUIrqreg->ppuIrqState = 0x1;
						}
			  }
			  while((pPPUIrqreg->ppuIrqState & C_PPU_INT_PEND_TV_VBLANK) == 0);
			          pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_VBLANK;		
	      while((pPPUFunreg->ppuFbGo & 0x8000) == 0);        			
			       //pPPUFunreg->ppuTvFbiAddr = (UINT32) ppu_current_frame;
			          ppu_current_frame=0;
			#endif
			wait_done_flag=0;
			new_register_sets_ptr = NULL;
		}
	} 
	else 
	{			// Line mode
		new_register_sets_ptr = p_register_set;

		// Set register update flag
		line_mode_update_register_request = 1;

		// Wait until register update is done
		while (!update_register_done){
			;
		}
		update_register_done = 0;
		new_register_sets_ptr = NULL;
		wait_ppu_frame_output_done = 0;
		ppu_frame_mode_busy = 0;	
	}
#endif
	
	return 0;
	
}
EXPORT_SYMBOL(gpHalPPUGo);

/**
* @brief	       ppu frame end hander
* @param 	none
* @return 	none
*/
void gpHalPPUVblankHander(void)
{

	if (pPPUFunreg->ppuEnable & C_PPU_DISPLAY_FRAME_MODE) {		// Frame base mode
		if ((pPPUFunreg->ppuEnable & C_PPU_VGA_ENABLE) && !(pPPUFunreg->ppuEnable & C_PPU_NON_INTERLACE)) {	// VGA Interlace mode
			#if 0
			if (pPPUFunreg->ppuFbGo & (1<<14)) {		// If FBO_F bit is 1, it means only even line is done.
				pPPUFunreg->ppuFbGo |= 0x1;			// PPU will start calculating odd line after PPU_GO bit is set.
				return;
			}				
			#else
		  	pPPUFunreg->ppuFbGo = 0x1;			// PPU will start calculating odd line after PPU_GO bit is set.
				return;			
			#endif
    
		}
		
		if(pPPUFunreg->ppuFreesize & C_PPU_FREESIZE_INTERLACE_ENABLE)
		{
			#if 0
			if (pPPUFunreg->ppuFbGo & (1<<14)) {		// If FBO_F bit is 1, it means only even line is done.
				pPPUFunreg->ppuFbGo |= 0x1;			// PPU will start calculating odd line after PPU_GO bit is set.
				return;
			}
				
		  #else
		  	pPPUFunreg->ppuFbGo = 0x1;			// PPU will start calculating odd line after PPU_GO bit is set.
				return;
		  #endif
		}
		
		#if 1
		Display_temp_frame = ppu_current_frame;	
		#endif
		ppu_current_frame = 0;
		ppu_frame_mode_busy = 0;
		wait_ppu_frame_output_done = 0;				// Clear this flag to notify program that ppu output is done
	} else {			// Line base mode
		if (line_mode_update_register_request) {
			if ((pPPUFunreg->ppuEnable & C_PPU_VGA_ENABLE) && !(pPPUFunreg->ppuEnable & C_PPU_NON_INTERLACE)) {	// VGA Interlace mode
				if (pPPUFunreg->ppuFbGo & (1<<14)) {		// If FBO_F bit is 1, it means only even line is done.
					// We must wait until odd line is also done before updating registers.
					return;
				}
			}
			line_mode_update_register_request = 0;
			// Now update register sets
			if (gpHalUpdatePPURegisters()) {
				// Do nothing if something wrong
				return;
			}
			//new_register_sets_ptr->update_register_flag &= C_UPDATE_REG_SET_DMA_MASK;	// Clear all flags except DMA
			if (!(new_register_sets_ptr->update_register_flag)) {
				// If sprite DMA copy is done or not needed, notify task that we are done here.
				update_register_done = 1;
			}
			// else, leave the notification job to ppu_dma_handler
		}
	}
}

#if DISPLAY_DEBUG == 1
void ppu_tv_vblank_handler(void)
{	
  	UINT32 frame;
	//ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);
	
	frame = Display_temp_frame;
	if (frame) {		// Check whether new frame buffer is available for display
		// Display new frame buffer			
		//pPPUFunreg->ppuTvFbiAddr = (UINT32) frame;
	}		
}

/**
* @brief	       none ppu module register set
* @param 	TV_TFT[in]:display device.
* @param 	display_buffer[in]:display buffer.
* @param 	DISPLAY_MODE[in]:display resolution.
* @param 	SHOW_TYPE[in]:0:display color type.
* @return 	success=0,fail=-1.
*/
void ppu_reg_set(UINT8 TV_TFT,UINT32 display_buffer,UINT32 DISPLAY_MODE ,UINT32 SHOW_TYPE)
{
  switch(SHOW_TYPE)
	{      
	   case IMAGE_OUTPUT_FORMAT_RGB565:
	        if(DISPLAY_MODE==QVGA_MODE)
	           	R_PPU_ENABLE=(PPU_QVGA_MODE|PPU_FRAME_BASE_MODE|PPU_RGB565_MODE);
	        else if(DISPLAY_MODE == VGA_MODE)
	           	R_PPU_ENABLE=(PPU_VGA_MODE|PPU_FRAME_BASE_MODE|PPU_RGB565_MODE);
	        else if(DISPLAY_MODE == D1_MODE)
	           	R_PPU_ENABLE=(PPU_QVGA_MODE|TFT_SIZE_720x480|PPU_FRAME_BASE_MODE|PPU_RGB565_MODE);    
	   		  break;
	    
	   case IMAGE_OUTPUT_FORMAT_RGBG:
	        if(DISPLAY_MODE==QVGA_MODE)
	           	R_PPU_ENABLE=(PPU_QVGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_RGBG_MODE|PPU_RGBG_TYPE2);
	        else if(DISPLAY_MODE == VGA_MODE)
	           	R_PPU_ENABLE=(PPU_VGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_RGBG_MODE|PPU_RGBG_TYPE2);
	        else if(DISPLAY_MODE == D1_MODE)
	        	R_PPU_ENABLE=(PPU_QVGA_MODE|TFT_SIZE_720x480|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_RGBG_MODE|PPU_RGBG_TYPE2);
	   		  break;
	   
	   case IMAGE_OUTPUT_FORMAT_GRGB:
	   		  if(DISPLAY_MODE==QVGA_MODE)
	           	R_PPU_ENABLE=(PPU_QVGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_RGBG_MODE|PPU_RGBG_TYPE3);
	        else if(DISPLAY_MODE == VGA_MODE)
	           	R_PPU_ENABLE=(PPU_VGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_RGBG_MODE|PPU_RGBG_TYPE3);
	        else if(DISPLAY_MODE == D1_MODE)
	        	R_PPU_ENABLE=(PPU_QVGA_MODE|TFT_SIZE_720x480|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_RGBG_MODE|PPU_RGBG_TYPE3);	
	        break;
	        
	   case IMAGE_OUTPUT_FORMAT_UYVY:
	        if(DISPLAY_MODE == QVGA_MODE)
	           	R_PPU_ENABLE=(PPU_QVGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE2);
	        else if(DISPLAY_MODE == VGA_MODE)
	           	R_PPU_ENABLE=(PPU_VGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE2);    
	   		  else if(DISPLAY_MODE == D1_MODE)
	   			    R_PPU_ENABLE=(PPU_QVGA_MODE|TFT_SIZE_720x480|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE2);
	        break;
	        
	   case IMAGE_OUTPUT_FORMAT_YUYV:
	        if(DISPLAY_MODE == QVGA_MODE)
	           	R_PPU_ENABLE=(PPU_QVGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE3);
	        else if(DISPLAY_MODE == VGA_MODE)
	           	R_PPU_ENABLE=(PPU_VGA_MODE|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE3);    
	   		  else if(DISPLAY_MODE == D1_MODE)
	   			    R_PPU_ENABLE=(PPU_QVGA_MODE|TFT_SIZE_720x480|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE3);
	   			break;
	} 
	if(TV_TFT)
	   R_TV_FBI_ADDR=(UINT32)display_buffer;
	else
	   R_TFT_FBI_ADDR=(UINT32)display_buffer;
}
EXPORT_SYMBOL(ppu_reg_set);
#endif

/**
* @brief	ppu irq isr
* @param 	none
* @return 	none
*/
SINT32 gpHalPPUIsr(void)
{
		UINT32 ppuisr_state=pPPUIrqreg->ppuIrqState;
		UINT32 temp;
		
		temp=0;
		#if 0     
			// TV Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TV_VBLANK) && (pPPUIrqreg->ppuIrqState & C_PPU_INT_PEND_TV_VBLANK)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_VBLANK;	
				  temp = 0;	
			    #if DISPLAY_DEBUG == 1
				  ppu_tv_vblank_handler();
				  #endif	  
			}
			
			// PPU Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_PPU_VBLANK) && (pPPUIrqreg->ppuIrqState & C_PPU_INT_PEND_PPU_VBLANK)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_PPU_VBLANK;
			    temp = 1;	
			  	gpHalPPUVblankHander();
			    ppu_frame_fifo_end = 1;
			}
		#elif 1
			#if 0
				// TV Vertical-Blanking interrupt
				if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TV_VBLANK) && (ppuisr_state & C_PPU_INT_PEND_TV_VBLANK)) 
				{
					  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_VBLANK; 
					  if(!ppu_irq_temp_flag)
	          {
	        	   temp = PPU_MODULE_PPU_VBLANK;
	        	   ppu_irq_temp_flag=1;  
	          }
				}
	
				if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TFT_VBLANK) && (ppuisr_state & C_PPU_INT_PEND_TFT_VBLANK)) 
				{
					  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TFT_VBLANK;	
					  temp |= PPU_MODULE_TFT_VBLANK;	    
				}			    
			#endif
			
			// TV Vertical-Blanking interrupt
			if(!ppu_irq_temp_flag)
			{
					#if 1
						if(ppuisr_state & C_PPU_INT_PEND_TV_VBLANK) 
						{
							  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_VBLANK; 
							  if(!ppu_irq_temp_flag)
			          {
			        	   temp = PPU_MODULE_PPU_VBLANK;
			        	   ppu_irq_temp_flag=1;  
			          }
						}
						
						if(ppuisr_state & C_PPU_INT_PEND_TFT_VBLANK) 
						{
							  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TFT_VBLANK; 
							  if(!ppu_irq_temp_flag)
			          {
			        	   temp = PPU_MODULE_PPU_VBLANK;
			        	   ppu_irq_temp_flag=1;  
			          }
						}		        
		        ppu_irq_temp_flag=1;   
				  #else
					  temp = PPU_MODULE_PPU_VBLANK;
			      ppu_irq_temp_flag=1;  
				  #endif
		  }	
		  
			// PPU Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_PPU_VBLANK) && (ppuisr_state & C_PPU_INT_PEND_PPU_VBLANK)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_PPU_VBLANK;
			    temp = PPU_MODULE_PPU_VBLANK;	
			  	#if 1
			  	gpHalPPUVblankHander();
			  	#endif	
			    ppu_frame_fifo_end = 1;
			}					
		#else   
			// TV Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TV_VBLANK) && (ppuisr_state & C_PPU_INT_PEND_TV_VBLANK)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_VBLANK;	
				  /*
				  #if 1
				  if(pPPUFunreg->ppuEnable & C_PPU_DISPLAY_FRAME_MODE)
				    temp |= PPU_MODULE_TV_VBLANK;
				  else
				    temp |= PPU_MODULE_PPU_VBLANK;	
			    #else
			    temp |= PPU_MODULE_TV_VBLANK;
			    #endif

			    #if DISPLAY_DEBUG == 1
				  ppu_tv_vblank_handler();
				  #endif
				  */	  
			}
			// TFT Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TFT_VBLANK) && (ppuisr_state & C_PPU_INT_PEND_TFT_VBLANK)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TFT_VBLANK;	
				  if(pPPUFunreg->ppuEnable & C_PPU_DISPLAY_FRAME_MODE)
				    temp |= PPU_MODULE_PPU_VBLANK;
				  else
				    temp |= PPU_MODULE_PPU_VBLANK;	    
			}
			
			// Sensor Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_SENSOR_FRAME_END) && (ppuisr_state & C_PPU_INT_PEND_SENSOR_FRAME_END)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_SENSOR_FRAME_END;	
				  temp |= PPU_MODULE_SENSOR_FRAME_END;	  
			}			
				  
		
			// PPU Vertical-Blanking interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_PPU_VBLANK) && (ppuisr_state & C_PPU_INT_PEND_PPU_VBLANK)) 
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_PPU_VBLANK;
			    temp |= PPU_MODULE_PPU_VBLANK;	
			  	#if 1
			  	gpHalPPUVblankHander();
			  	#endif	
			    ppu_frame_fifo_end = 1;
			}
			
			// DMA End interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_DMA_COMPLETE) && (ppuisr_state & C_PPU_INT_PEND_DMA_COMPLETE))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_DMA_COMPLETE;
				  temp |= PPU_MODULE_DMA_COMPLETE;
			}			
			
			// VIDEO POSITION interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_VIDEO_POSITION) && (ppuisr_state & C_PPU_INT_PEND_VIDEO_POSITION))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_VIDEO_POSITION;
				  temp |= PPU_MODULE_VIDEO_POSITION;
			}
			
			// PALETTE ERROR interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_PALETTE_ERROR) && (ppuisr_state & C_PPU_INT_PEND_PALETTE_ERROR))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_PALETTE_ERROR;
				  temp |= PPU_MODULE_PALETTE_ERROR;
			}
				  
			
			// TEXT UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TEXT_UNDERRUN) && (ppuisr_state & C_PPU_INT_PEND_TEXT_UNDERRUN))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TEXT_UNDERRUN;
				  temp |= PPU_MODULE_TEXT_UNDERRUN;
			}

			// SPRITE UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_SPRITE_UNDERRUN) && (ppuisr_state & C_PPU_INT_PEND_SPRITE_UNDERRUN))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_SPRITE_UNDERRUN;
				  temp |= PPU_MODULE_SPRITE_UNDERRUN;
			}
				  

			// MOTION DETECT interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_MOTION_DETECT) && (ppuisr_state & C_PPU_INT_PEND_MOTION_DETECT))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_MOTION_DETECT;
				  temp |= PPU_MODULE_MOTION_DETECT;
			}
				  

			// SENSOR POSITION HIT interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_SENSOR_POSITION_HIT) && (ppuisr_state & C_PPU_INT_PEND_SENSOR_POSITION_HIT))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_SENSOR_POSITION_HIT;
				  temp |= PPU_MODULE_MOTION_DETECT;
			}
				  

			// MOTION UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_MOTION_UNDERRUN) && (ppuisr_state & C_PPU_INT_PEND_MOTION_UNDERRUN))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_MOTION_UNDERRUN;
				  temp |= PPU_MODULE_MOTION_UNDERRUN;
			}

			// TV UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TV_UNDERRUN) && (ppuisr_state & C_PPU_INT_PEND_TV_UNDERRUN))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TV_UNDERRUN;
				  temp |= PPU_MODULE_TV_UNDERRUN;
			}
			
			// TFT UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_TFT_UNDERRUN) && (ppuisr_state & C_PPU_INT_PEND_TFT_UNDERRUN))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_TFT_UNDERRUN;
				  temp |= PPU_MODULE_TFT_UNDERRUN;
			}			

			// TFT UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_PPU_HBLANK) && (ppuisr_state & C_PPU_INT_PEND_PPU_HBLANK))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_PPU_HBLANK;
				  temp |= PPU_MODULE_PPU_HBLANK;
			}
				  

			// SENSOR UNDERRUN interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_SENSOR_UNDERRUN) && (ppuisr_state & C_PPU_INT_PEND_SENSOR_UNDERRUN))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_SENSOR_UNDERRUN;
				  temp |= PPU_MODULE_SENSOR_UNDERRUN;
			}
				  

			// IIIEGAL WRITE interrupt
			if((pPPUIrqreg->ppuIrqEn & C_PPU_INT_EN_IIIEGAL_WRITE) && (ppuisr_state & C_PPU_INT_PEND_IIIEGAL_WRITE))
			{
				  pPPUIrqreg->ppuIrqState = C_PPU_INT_PEND_IIIEGAL_WRITE;
				  temp |= PPU_MODULE_IIIEGAL_WRITE;
			}
				  			
		#endif
		
		return temp;
}
EXPORT_SYMBOL(gpHalPPUIsr);

/**
* @brief	       ppu fb mode
* @param 	enable[in]:0=disable,1=enable.
* @return 	none
*/
void gpHalPPUFbCtrl(UINT32 enable)
{
     //ppuFunReg_t *pPPUFunreg = (ppuFunReg_t *)(PPU_BASE_REG);

     if(enable)
        pPPUFunreg->ppuMisc |=FB_LOCK_ENABLE;
     else
        pPPUFunreg->ppuMisc &=~FB_LOCK_ENABLE;   
}

/**
* @brief	       ppu fifo go mode
* @param 	none
* @return 	success=0,fail=-1.
*/
SINT32 gpHalPPUFifogowithdone(void)
{
	   
	// Set registers
	if (gpHalUpdatePPURegisters()) {
		free_frame_buffer_array[free_fb_put_idx] = ppu_current_frame;
		if (++free_fb_put_idx >= C_PPU_DRV_MAX_FRAME_NUM) {
			free_fb_put_idx = 0;
		}
		new_register_sets_ptr = NULL;
		return STATUS_FAIL;
	}
	
	if(fifo_go_ck==0)
	{
	          pPPUFunreg->ppuFbGo=0x1;
		   while(1)
		   {
			      if((pPPUIrqreg->ppuIrqState & C_PPU_FRAME_FIFO_ENABLE) == C_PPU_FRAME_FIFO_ENABLE)
			      {
				    pPPUIrqreg->ppuIrqState=C_PPU_FRAME_FIFO_ENABLE;
		                  fifo_go_ck=1;
		                  break;	   
			      }
		   }    
       }
       else
       {
		   ppu_frame_fifo_end=0;
		   pPPUFunreg->ppuFbfifoGo=0x1;
		   while(1)
		   {
			      if(ppu_frame_fifo_end==1)
			      {
				    fifo_go_ck=0;
				    break;
			      }   
			      if((pPPUIrqreg->ppuIrqState & C_PPU_FRAME_FIFO_ENABLE) == C_PPU_FRAME_FIFO_ENABLE) 
			      {
			           pPPUIrqreg->ppuIrqState=C_PPU_FRAME_FIFO_ENABLE;
			           break;	   
			      } 	
		   }
	}   
	
    return ppu_frame_fifo_end;    
}
EXPORT_SYMBOL(gpHalPPUFifogowithdone);

/**
* @brief	sprite 25d position convert
* @param 	pos_in[in]:sprite position.
* @param 	sp_out[in]:sprite address of sprite ram.
* @return 	none
*/
void gpHalPPUsprite25dPosconvert(SpN_RAM *sp_out, POS_STRUCT_PTR pos_in)
{
       // pos in
       pPPUSpritereg->ppuSpriteX0 = pos_in->x0;
       pPPUSpritereg->ppuSpriteY0 = pos_in->y0;
       pPPUSpritereg->ppuSpriteX1 = pos_in->x1;
       pPPUSpritereg->ppuSpriteY1 = pos_in->y1;
       pPPUSpritereg->ppuSpriteX2 = pos_in->x2;
       pPPUSpritereg->ppuSpriteY2 = pos_in->y2;
       pPPUSpritereg->ppuSpriteX3 = pos_in->x3;
       pPPUSpritereg->ppuSpriteY3 = pos_in->y3;
       //pos out
       sp_out->uPosX_16 = pPPUSpritereg->ppuSpriteW0;
       sp_out->uPosY_16 = pPPUSpritereg->ppuSpriteW1;
       sp_out->uX1_16 = pPPUSpritereg->ppuSpriteW2;
       sp_out->uX2_16 = pPPUSpritereg->ppuSpriteW3;
       sp_out->uX3_16 = pPPUSpritereg->ppuSpriteW4;		
}
EXPORT_SYMBOL(gpHalPPUsprite25dPosconvert);



void
gpHalPPUSetTftBurst(
	UINT32 mode
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x1 << 24);
	regVal |= (mode << 24);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetTftBurst);

void
gpHalPPUSetYuvType(
	UINT32 type
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x7 << 20);
	regVal |= (type << 20);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetYuvType);

void
gpHalPPUSetRes(
	UINT32 mode
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x7 << 16);
	regVal |= (mode << 16);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetRes);

void
gpHalPPUSetFbMono(
	UINT32 mode
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x3 << 10);
	regVal |= (mode << 10);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetFbMono);

void
gpHalPPUSetFbFormat(
	UINT32 mode
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x1 << 8);
	regVal |= (mode << 8);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetFbFormat);

void
gpHalPPUSetFbEnable(
	UINT32 enable
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x1 << 7);
	regVal |= (enable << 7);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetFbEnable);

void
gpHalPPUSetVgaEnable(
	UINT32 enable
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuEnable;
	regVal &= ~(0x1 << 4);
	regVal |= (enable << 4);
	pPPUFunreg->ppuEnable = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetVgaEnable);

void
gpHalPPUSetIrqEnable(
	UINT32 field
)
{
	pPPUIrqreg->ppuIrqEn |= field;
}
EXPORT_SYMBOL(gpHalPPUSetIrqEnable);

void
gpHalPPUSetIrqDisable(
	UINT32 field
)
{
	pPPUIrqreg->ppuIrqEn &= ~field;
}
EXPORT_SYMBOL(gpHalPPUSetIrqDisable);

void
gpHalPPUClearIrqFlag(
	UINT32 field
)
{
	pPPUIrqreg->ppuIrqState = field;
}
EXPORT_SYMBOL(gpHalPPUClearIrqFlag);

UINT32
gpHalPPUGetIrqStatus(
	void
)
{
	return pPPUIrqreg->ppuIrqState;
}
EXPORT_SYMBOL(gpHalPPUGetIrqStatus);

void
gpHalPPUSetTftBufferAddr(
	UINT32 addr
)
{
	pPPUFunreg->ppuTftFbiAddr = addr;
}
EXPORT_SYMBOL(gpHalPPUSetTftBufferAddr);

void
gpHalPPUSetTvBufferAddr(
	UINT32 addr
)
{
	pPPUFunreg->ppuTvFbiAddr = addr;
}
EXPORT_SYMBOL(gpHalPPUSetTvBufferAddr);

void
gpHalPPUSetFlip(
	UINT32 enable
)
{
	UINT32 regVal;

	regVal = pPPUFunreg->ppuMisc;
	regVal &= ~(0x1 << 17);
	regVal |= (enable << 17);
	pPPUFunreg->ppuMisc = regVal;
}
EXPORT_SYMBOL(gpHalPPUSetFlip);
