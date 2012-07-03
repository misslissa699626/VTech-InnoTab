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
 * @file    hal_tv.c
 * @brief   Implement of TV HAL API.
 * @author  Cater Chen
 * @since   2010-11-18
 * @date    2010-11-18
 */
 
/*******************************************************************************
*                         H E A D E R   F I L E S
*******************************************************************************/
//Include files
#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/io.h>
#include <mach/hal/regmap/reg_tv.h>
#include <mach/hal/hal_tv.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
*                          D A T A    T Y P E S
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
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
const UINT16 Tbl_TV_RegSet[][9] = {
      {0x815E,0x5A00,0x8200,0x0000,0x6D4E,0x1F10,0x7C00,0xF003,0x2100},  //0: NTSC-M
      {0x815E,0x5A00,0x7600,0x0000,0x6D4E,0xB800,0x1E00,0xF003,0x2100},  //1: NTSC-J
      {0x8161,0x5A00,0x8200,0x0000,0x6D4E,0xCB10,0x8A00,0x0903,0x2A00},  //2: NTSC-N
      {0x815E,0x5A00,0x8200,0x0000,0x6D4E,0x27F0,0xA500,0xF007,0x2100},  //3: PAL-M
      {0x8161,0x5A00,0x7600,0x0000,0x6D4E,0xCBE0,0x8A00,0x0905,0x2A00},  //4: PAL-B
      {0x8161,0x5A00,0x8200,0x0000,0x6D4E,0xCBF0,0x8A00,0x0905,0x2A00},  //5: PAL-N
      {0x8161,0x5A00,0x7600,0x0000,0x6D4E,0xCBE0,0x8A00,0x0905,0x2A00},  //6: PAL-Nc
      {0x815E,0x5A00,0x7600,0x0000,0x6D4E,0xB800,0x1E00,0xF003,0x2100},  //7: NTSC-J (Non-Interlaced)  
      {0x8161,0x5A00,0x7600,0x0000,0x6D5F,0xCBE0,0x8A00,0x0905,0x2A00},  //8: PAL-B (Non-Interlaced)  
};

static gpTvReg_t *pTVreg = (gpTvReg_t *)(TV_BASE_REG);
static gpTvSysReg_t *pTVsysreg = (gpTvSysReg_t *)(SYSTEM_BASE_REG);
static UINT32 tv_temp_buffer,tv_temp_buffer_flag,temp_reg;
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
* @brief	       gp tv1 initial
* @param 	none
* @return 	none
*/
void gpHalTVinit (void)
{   
    pTVreg->gpTvCotrl0 = 0x0;          //  Disable TV at first
    
    #if 1
	    pTVreg->gpTVIrqen &= ~TV1_IRQ_EN;
	    if(pTVreg->gpTVCotrl2 & 0x80)
	       pTVreg->gpTVIrqsts = TV1_IRQ_EN;   
	    if(!(pTVreg->gpTVCotrl2 & 0x1))
	       pTVreg->gpTVCotrl2 &=~0x80; 
    #endif
    
    tv_temp_buffer_flag = 0;
              
    pTVsysreg->gpTvSysCotrl0 |= 0x00380020;
    pTVsysreg->gpTvSysCotrl1 |= 0x32000000;
    
    // tv clock set
    #if 1
      pTVsysreg->gpTvEnable0 |=0x21; 
      pTVsysreg->gpTvEnable1 &= ~0x7; 
      pTVsysreg->gpTvEnable1 |= 0x18;
    #endif
}

/**
* @brief	  ppu display resolution set
* @param 	  DISPLAY_MODE[in]:display resolution.
* @param 	  INTL_MODE[in]:interlace mode.
* @return 	success=0,fail=-1.
*/

static void gpHalPicDisplayresolution(UINT32 DISPLAY_MODE, UINT32 INTL_MODE)
{
	   switch(DISPLAY_MODE)
	   {
	   	  case TV_QVGA:
	           pTVreg->gpTVCotrl2 &= ~MASK_TV1_D1_SET;
	           pTVreg->gpTVCotrl2 &= ~TV1_VGA;         	  
	   	       break;
	   	       
	   	  case TV_HVGA:
	           pTVreg->gpTVCotrl2 |= TV1_VGA;	        	  
	   	       break; 
	   	       
	   	  case TV_D1:
	           pTVreg->gpTVCotrl2 &= ~MASK_TV1_D1_SET;
	           pTVreg->gpTVCotrl2 &= ~TV1_QVGA;
	           pTVreg->gpTVCotrl2 |= TV1_D1;         	  
	   	       break;
	   	       
	   	  case TV_HVGA_576:
	           pTVreg->gpTVCotrl2 &= ~MASK_TV1_D1_SET;
	           pTVreg->gpTVCotrl2 &= ~B_TV1_VGA_EN;
	           pTVreg->gpTVCotrl2 |= TV1_576;
	           pTVreg->gpTVCotrl3 = ((INTL_MODE<<B_INTEL_TYPE)|(TV_640)|(TV_576));		        	  
	   	       break;        	       
	   	       
	   	  case TV_D1_576:
	           pTVreg->gpTVCotrl2 &= ~MASK_TV1_D1_SET;
	           pTVreg->gpTVCotrl2 &= ~B_TV1_VGA_EN;
	           pTVreg->gpTVCotrl2 |= TV1_576;
	           pTVreg->gpTVCotrl3 = ((INTL_MODE<<B_INTEL_TYPE)|(TV_720)|(TV_576));	        	  
	   	       break;       	              	             	                 
	   }      	
}
/**
* @brief	gp tv1 start
* @param 	nTvStd[in]:tv type ,ntsc/pal.
* @param 	nResolution[in]:tv resolution ,qvga/vga/d1.
* @param 	nNonInterlace[in]:tv noninterlace ,0=disable,1=enable.
* @return 	none
*/
void gpHalTVstart(SINT32 nTvStd, SINT32 nResolution, SINT32 nNonInterlace)
{
    if(pTVreg->gpTvCotrl0)
       pTVreg->gpTvCotrl0 = 0x0;          //  Disable TV at first   
    
    if(!(pTVreg->gpTVCotrl2 & 0x1))
    {
       gpHalPicDisplayresolution(nResolution,nNonInterlace);     
       pTVreg->gpTVCotrl2 |= TV1_DISPLAY_EN;
    }
    
    if(nResolution == TV_HVGA_576)
    {
       nResolution = TV_HVGA;
       pTVreg->gpTvCotrl1 |= TV1_PAL576_ENABLE;
    }
    
    else if(nResolution == TV_D1_576)
    {
       nResolution = TV_D1;
       pTVreg->gpTvCotrl1 |= TV1_PAL576_ENABLE;   	    	
    }       
      
    if (nNonInterlace) {
        if (nTvStd < TVSTD_PAL_M) {     //  NTSC
            nTvStd  = TVSTD_NTSC_J_NONINTL;    
        } else {
            if(nTvStd==TVSTD_NTSC_J_NONINTL)
              nTvStd  = TVSTD_NTSC_J_NONINTL;
            else
              nTvStd  = TVSTD_PAL_B_NONINTL;
        }
    }

    pTVreg->gpTVSaturation = Tbl_TV_RegSet[nTvStd][0];
    pTVreg->gpTVHue        = Tbl_TV_RegSet[nTvStd][1];
    pTVreg->gpTVBrightness = Tbl_TV_RegSet[nTvStd][2];
    pTVreg->gpTVSharpness  = Tbl_TV_RegSet[nTvStd][3];
    pTVreg->gpTVYGain     = Tbl_TV_RegSet[nTvStd][4];
    pTVreg->gpTVYDelay    = Tbl_TV_RegSet[nTvStd][5];
    pTVreg->gpTVVPosition = Tbl_TV_RegSet[nTvStd][6];
    pTVreg->gpTVHPosition = Tbl_TV_RegSet[nTvStd][7];
    pTVreg->gpTVVideodac   = Tbl_TV_RegSet[nTvStd][8];
    pTVreg->gpTvCotrl0    =   (   TV_BIT_VALUE_SET (B_TVSTD, 7) |                \
                            TV_BIT_VALUE_SET (B_NONINTL, nNonInterlace) |  \
                            TV_BIT_VALUE_SET (B_RESOLUTION, nResolution)|  \
                            TV_BIT_VALUE_SET (B_TVEN, 1)
                        );
                     
    if(nResolution==TV_D1)
    {
       if((nTvStd < TVSTD_PAL_M)||(nTvStd==TVSTD_NTSC_J_NONINTL)){
         pTVreg->gpTVHPosition=0xF0F6; 
       } 
       pTVreg->gpTvCotrl1|=0x1;
    }       
    #if 0  
     pTVreg->gpTVIrqen |= TV1_IRQ_EN;
    #endif        
}

/**
* @brief	 none ppu display Y/Pb/Pr set
* @param 	 mode[in]:0:480i,1:720p.
* @param 	 enable[in]:0:disable, 1:enable.
* @return  none.
*/
void gpHalYbpbcrenable(UINT32 mode,UINT32 enable)
{
    UINT32 temp;
    
    if(mode)
    {     
    	 //for Y/Pb/Pr output 720p
    	 if(enable)
    	 {
    	    pTVreg->gpTvCotrl1|=0x25;
          pTVsysreg->gpTvEnable0 &=~0x20; 
          pTVsysreg->gpTvEnable0 |=0x15; 
          pTVsysreg->gpTvEnable1 &=~0x10;
          pTVsysreg->gpTvEnable1 |= 0x28;
          pTVsysreg->gpTvEnable2 |= 0xC0000;
          temp_reg = pTVreg->gpTVCotrl3;
          temp = pTVreg->gpTVCotrl3;
          temp &=~(0x1 << B_INTEL_TYPE); 
          pTVreg->gpTVCotrl3 = ((temp)|(TV_720P_H)|(TV_720P_V));
          pTVreg->gpTVCotrl2 |= 0x80;
    	 }
    	 else
    	 {
    	    pTVreg->gpTvCotrl1&=~0x25; 
          pTVsysreg->gpTvEnable0 &=~0x14; 
          pTVsysreg->gpTvEnable0 |=0x21; 
          pTVsysreg->gpTvEnable1 &=~0x20;
          pTVsysreg->gpTvEnable1 |=~0x18;
          pTVsysreg->gpTvEnable2 &=~0xC0000;          
          pTVreg->gpTVCotrl3 = temp_reg;           	    
    	 }     	    
    }
    else
    {  	    
       //for Y/Pb/Pr output 480i
       if(enable)
       {
          pTVreg->gpTvCotrl1|=0x4;
          pTVsysreg->gpTvEnable1 &=~0x10; 
          pTVsysreg->gpTvEnable1 |= 0x8; 
          pTVsysreg->gpTvEnable2 |= 0xC0000;             
          pTVsysreg->gpTvEnable0 &=~0x10;       
          pTVsysreg->gpTvEnable0 |=0x25;
          pTVreg->gpTVCotrl2 |= 0x80;                      
       }
       else
       {
          pTVreg->gpTvCotrl1&=~0x4;
          pTVsysreg->gpTvEnable1 |=0x18; 
          pTVsysreg->gpTvEnable2 &=~0xC0000;              
          pTVsysreg->gpTvEnable0 &=~0x25;
          pTVsysreg->gpTvEnable0 |=0x21;         
       }                         	        
    }	
}

/**
* @brief	 none ppu display color set
* @param 	 SHOW_TYPE[in]:display color type.
* @return  none.
*/
void gpHalPicDisplaycolor(BUFFER_COLOR_FORMAT COLOR_MODE)
{ 
	   switch(COLOR_MODE)
	   {
			   case BUFFER_COLOR_FORMAT_RGB565:
			           	pTVreg->gpTVCotrl2 &= ~TV1_RGBG_YUYV;			           	
			           	pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;    
			   		  break;
			    
			   case BUFFER_COLOR_FORMAT_BGRG:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_RGBG_SET);			        	  		        	  
			   		  break;
			   
			   case BUFFER_COLOR_FORMAT_GBGR:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_RGBG_SET | TV1_GBGR_YVYU_TYPE);		        	  
			        break;
			        
			   case BUFFER_COLOR_FORMAT_RGBG:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_RGBG_SET | TV1_RGBG_UYVY_TYPE);
			        break;
			        
			   case BUFFER_COLOR_FORMAT_GRGB:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_RGBG_SET | TV1_GRGB_YUYV_TYPE);
			   			break;
			  
			   case BUFFER_COLOR_FORMAT_VYUY:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_YUYV_SET);    
			   		  break;
			    
			   case BUFFER_COLOR_FORMAT_YVYU:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_YUYV_SET | TV1_GRGB_YUYV_TYPE);
			   		  break;
			   
			   case BUFFER_COLOR_FORMAT_UYVY:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_YUYV_SET | TV1_RGBG_UYVY_TYPE);	
			        break;
			        
			   case BUFFER_COLOR_FORMAT_YUYV:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_YUYV_SET | TV1_GRGB_YUYV_TYPE);
			        break;
			        
			   case BUFFER_COLOR_FORMAT_RGBA:
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_SET;
			        	  pTVreg->gpTVCotrl2 &= ~MASK_RGBG_YUYV_TYPE;			        	  
			        	  pTVreg->gpTVCotrl2 |= (TV1_RGBG_YUYV | TV1_RGBA_SET);
			   			break;
		     
		     default:
		              if(!(pTVreg->gpTVCotrl2 & 0x1))
			           	   pTVreg->gpTVCotrl2 = 0x0;			           	
			        break;			   						   						   				
	   }			   			
}

/**
* @brief	  tv1 display buffer set
* @param 	  display_buffer[in]:display buffer.
* @return 	none.
*/
void gpHalTvFramebufferset(UINT32 display_buffer)
{
     #if 1
     pTVreg->gpTVFbiaddr=(UINT32)display_buffer;
     #else
     tv_temp_buffer = (UINT32)display_buffer;
     tv_temp_buffer_flag = 1;
     #endif
}

/**
* @brief	  tv1 display buffer state
* @return 	display buffer state, success=0,fail=-1.
*/
signed int gpHalTvFramebufferstate(void)
{ 
    if(!tv_temp_buffer_flag)
       return 0;
    else
       return -1;   
}

/**
* @brief	  tv1 display reverse set
* @param 	  enable[in]:0:disable, 1:enable.
* @return 	none.
*/
void gpHalTvFramebufferReverse(UINT32 enable)
{
     if(enable)
       pTVreg->gpTVCotrl21 |=TV1_REVERSE_ENABLE;
     else
       pTVreg->gpTVCotrl21 &=~TV1_REVERSE_ENABLE; 
}

/**
* @brief	  tv1 display isr
* @return 	isr_number.
*/
SINT32 gpHalTvIsr(void)
{
	   SINT32 tv1isr_state = pTVreg->gpTVIrqsts;
	   SINT32 temp = 0;
	   
	   if((pTVreg->gpTVIrqen & TV1_IRQ_EN) && (tv1isr_state & TV1_IRQ_EN)) 
	   {
				  pTVreg->gpTVIrqsts = TV1_IRQ_EN;
				  if(tv_temp_buffer_flag)
				  {
				  	 pTVreg->gpTVFbiaddr=(UINT32)tv_temp_buffer;
				  	 tv_temp_buffer_flag=0;
				  }
				  temp = TV1_IRQ_START;  
		 }
		 else
		   temp = -1;
		   	 
	   return temp;
}