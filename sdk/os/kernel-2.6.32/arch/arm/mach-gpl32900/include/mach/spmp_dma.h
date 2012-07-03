#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

enum dma_system_type {
	APBDMAA_SYSTEM,
	APBDMAC_SYSTEM,		
};

enum dma_buffer {
	DMA_BUFFERA,
	DMA_BUFFERB,		
};

/*
 * DMA registration
 */

//int __init spmp_init_dma(void);

int spmp_request_dma (enum dma_system_type systype,
	         char *name,
			 void (*irq_handler)(int, void *),
			 void *data);

int spmp_request_disable(enum dma_system_type systype,
	           int dma_ch);

int spmp_request_enable(enum dma_system_type systype,
	           int dma_ch,
	           void *apbaddr,
	           unsigned int flag);


int spmp_request_address(enum dma_system_type systype, 
               	int dma_ch,
                enum dma_buffer bufidx,
                void *miuaddr,
                unsigned int len);
void spmp_free_dma (enum dma_system_type systype,int dma_ch);

#endif /* _ASM_ARCH_DMA_H */
