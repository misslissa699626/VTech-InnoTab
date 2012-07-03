#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H


typedef struct _dma_desc {
	volatile u32 ahb_a_sadr;	
	volatile u32 ahb_a_eadr;		
	volatile u32 ahb_b_sadr;	
	volatile u32 ahb_b_eadr;	
	volatile u32 apb_sadr;				
} spmp_dma_desc;


// APB DMA
#define DMA_M2P			0x00000000   //MIU to APB
#define DMA_P2M			0x00000001   //APB to MIU

#define DMA_AUTO		0x00000000   //AUTO ENABLE
#define DMA_REQ			0x00000002   //REQ ENABLE 

#define DMA_CON			0x00000000   //Address mode = CONTINUE
#define DMA_FIX			0x00000004   //Address mode = FIX

#define DMA_SINGLE_BUF	0x00000000   //Single Buffer
#define DMA_DOUBLE_BUF	0x00000008   //Nulti Buffer

#define DMA_8BIT		0x00000000   //8-bits single transfer
#define DMA_16BIT		0x00000010   //16-bits single transfer
#define DMA_32BIT		0x00000020   //32-bits single transfer
#define DMA_32BIT_BURST	0x00000030   //32-bits burst transfer

#define DMA_IRQOFF		0x00000000
#define DMA_IRQON		0x00000040

#define DMA_OFF			0x00000000
#define DMA_ON			0x00000080

/*
 * DMA registration
 */

int __init spmp_init_dma(int num_ch);

int spmp_request_dma (char *name,
			 pxa_dma_prio prio,
			 void (*irq_handler)(int, void *),
			 void *data);

void spmp_free_dma (int dma_ch);

#endif /* _ASM_ARCH_DMA_H */
