#ifndef _MACH_VPPU_H_
#define _MACH_VPPU_H_

typedef struct ppu_register_region_s {
	unsigned int start;
	unsigned int end;
} ppu_register_region_t; 

/* ioctl command */
#define VPPUIO_TYPE 'D'
#define VPPU_IOC_DRAWFB	                          		_IOW(VPPUIO_TYPE, 0x01, void*)	
// #define VPPU_IOC_WAIT_DRAWING                      		_IOW(VPPUIO_TYPE, 0x02, unsigned int)	
// #define VPPU_IOC_IS_DRAWING                        		_IOW(VPPUIO_TYPE, 0x03, unsigned int)	

/* irq status flag */
#define VPPU_IRQSTATUS_VB				(1 << 0)

/* register offsets */
#define VPPU_TEXT3_X_POSITION			0x0000
#define VPPU_TEXT3_Y_POSITION			0x0004
#define VPPU_TEXT3_X_OFFSET				0x0008
#define VPPU_TEXT3_Y_OFFSET				0x000c
#define VPPU_TEXT3_ATTRIBUTE			0x0010
#define VPPU_TEXT3_CTRL					0x0014
#define VPPU_TEXT3_N_PTR				0x0018
#define VPPU_TEXT3_A_PTR				0x001c

#define VPPU_TEXT4_X_POSITION			0x0020
#define VPPU_TEXT4_Y_POSITION			0x0024
#define VPPU_TEXT4_X_OFFSET				0x0028
#define VPPU_TEXT4_Y_OFFSET				0x002c
#define VPPU_TEXT4_ATTRIBUTE			0x0030
#define VPPU_TEXT4_CTRL					0x0034
#define VPPU_TEXT4_N_PTR				0x0038
#define VPPU_TEXT4_A_PTR				0x003c

#define VPPU_TEXT1_X_POSITION			0x0040
#define VPPU_TEXT1_Y_POSITION			0x0044
#define VPPU_TEXT1_X_OFFSET				0x0340
#define VPPU_TEXT1_Y_OFFSET				0x0344
#define VPPU_TEXT1_ATTRIBUTE			0x0048
#define VPPU_TEXT1_CTRL					0x004c
#define VPPU_TEXT1_N_PTR				0x0050
#define VPPU_TEXT1_A_PTR				0x0054

#define VPPU_TEXT2_X_POSITION			0x0058
#define VPPU_TEXT2_Y_POSITION			0x005c
#define VPPU_TEXT2_X_OFFSET				0x0350
#define VPPU_TEXT2_Y_OFFSET				0x0354
#define VPPU_TEXT2_ATTRIBUTE			0x0060
#define VPPU_TEXT2_CTRL					0x0064
#define VPPU_TEXT2_N_PTR				0x0068
#define VPPU_TEXT2_A_PTR				0x006c

#define VPPU_VCOMP_VALUE				0x0070
#define VPPU_VCOMP_OFFSET				0x0074
#define VPPU_VCOMP_STEP					0x0078

#define VPPU_TEXT1_SEGMENT				0x0080
#define VPPU_TEXT2_SEGMENT				0x0084
#define VPPU_SPRITE_SEGMENT				0x0088
#define VPPU_TEXT3_SEGMENT				0x008c
#define VPPU_TEXT4_SEGMENT				0x0090

#define VPPU_TEXT1_COSINE				0x0348
#define VPPU_TEXT1_SINE					0x034c
#define VPPU_TEXT2_COSINE				0x0358
#define VPPU_TEXT2_SINE					0x035c
#define VPPU_TEXT4_COSINE				0x00a0
#define VPPU_TEXT4_SINE					0x00a4

#define VPPU_BLENDING					0x00a8
#define VPPU_FADE_CTRL					0x00c0
#define VPPU_IRQTMV						0x00d8
#define VPPU_IRQTMH						0x00dc
#define VPPU_LINE_COUNTER				0x00e0
#define VPPU_LIGHTPEN_CTRL				0x00e4
#define VPPU_PALETTE_CTRL				0x00e8
#define VPPU_LPHPOSITION				0x00f8
#define VPPU_LPVPOSITION				0x00fc
#define VPPU_Y25D_COMPRESS				0x0104

#define VPPU_SPRITE_CTRL				0x0108

#define VPPU_WINDOW0_X					0x0120
#define VPPU_WINDOW0_Y					0x0124
#define VPPU_WINDOW1_X					0x0128
#define VPPU_WINDOW1_Y					0x012c
#define VPPU_WINDOW2_X					0x0130
#define VPPU_WINDOW2_Y					0x0134
#define VPPU_WINDOW3_X					0x0138
#define VPPU_WINDOW3_Y					0x013c

#define VPPU_IRQ_EN						0x0188
#define VPPU_IRQ_STATUS					0x018c

#define VPPU_SPRITE_DMA_SOURCE			0x01c0
#define VPPU_SPRITE_DMA_TARGET			0x01c4
#define VPPU_SPRITE_DMA_NUMBER			0x01c8

#define VPPU_HB_CTRL					0x01cc
#define VPPU_HB_GO						0x01d0

#define P_TV_FBI_ADDR					0x01e0
#define P_TFT_FBI_ADDR					0x033c
#define VPPU_FBO_ADDR					0x01e8
#define VPPU_FB_GO						0x01f0

#define VPPU_BLD_COLOR					0x01f4

#define VPPU_MISC						0x01f8
#define VPPU_ENABLE						0x01fc

#define VPPU_SPRITE_X0					0x0300
#define VPPU_SPRITE_Y0					0x0304
#define VPPU_SPRITE_X1					0x0308
#define VPPU_SPRITE_Y1					0x030c
#define VPPU_SPRITE_X2					0x0310
#define VPPU_SPRITE_Y2					0x0314
#define VPPU_SPRITE_X3					0x0318
#define VPPU_SPRITE_Y3					0x031c
#define VPPU_SPRITE_W0					0x0320
#define VPPU_SPRITE_W1					0x0324
#define VPPU_SPRITE_W2					0x0328
#define VPPU_SPRITE_W3					0x032c
#define VPPU_SPRITE_W4					0x0330

#define VPPU_EXTENDSPRITE_CONTROL		0x0334
#define VPPU_EXTENDSPRITE_ADDR			0x0338

#define VPPU_RGB_OFFSET					0x037c

#define VPPU_RANDOM0					0x0380
#define VPPU_RANDOM1					0x0384

#define P_FREE_SIZE						0x036c

#define VPPU_DEF_PARA					0x0370
#define P_TFT_3D_OFFSET					0x03a8
#define P_TFTFBI_ADDR_R					0x03ac

#define VPPU_TEXT_H_OFFSET0				0x0400
#define VPPU_TEXT_H_OFFSET239			0x07bc

#define VPPU_TEXT_HCMP_VALUE0			0x0800
#define VPPU_TEXT_HCMP_VALUE239			0x0bbc

#define VPPU_TEXT3_COS0					0x0800
#define VPPU_TEXT3_SIN0					0x0804
#define VPPU_TEXT3_COS239				0x0f78
#define VPPU_TEXT3_SIN239				0x0f7c

#define VPPU_PALETTE_RAM0_0				0x1000
#define VPPU_PALETTE_RAM0_255			0x13fc
#define VPPU_PALETTE_RAM1_0				0x1400
#define VPPU_PALETTE_RAM1_255			0x17fc
#define VPPU_PALETTE_RAM2_0				0x1800
#define VPPU_PALETTE_RAM2_255			0x1bfc
#define VPPU_PALETTE_RAM3_0				0x1c00
#define VPPU_PALETTE_RAM3_255			0x1ffc

#define VPPU_SPRITE_REC0				0x2000
#define VPPU_SPRITE_REC1023				0x5ff0

#define P_SPRITE0_EXT					0x6000
#define P_SPRITE1023_EXT				0x6ffc

#define VPPU_TFT_COLOR_MAP_B_0			0x7000
#define VPPU_TFT_COLOR_MAP_B_255		0x73fc
#define VPPU_TFT_COLOR_MAP_G_0			0x7400
#define VPPU_TFT_COLOR_MAP_G_255		0x77fc
#define VPPU_TFT_COLOR_MAP_R_0			0x7800
#define VPPU_TFT_COLOR_MAP_R_255		0x7bfc


#endif /* _MACH_VPPU_H_ */
