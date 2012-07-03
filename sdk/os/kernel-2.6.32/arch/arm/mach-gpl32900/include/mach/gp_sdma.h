#ifndef _GP_SDMA_H_
#define _GP_SDMA_H_

#define SDMA_IOCTL_ID           's'
#define SDMA_IOCTL_TRIGGER     _IOR(SDMA_IOCTL_ID, 0, gp_Sdma_t)

/*sdma status*/
#define	ERROR_DSTBUS 1
#define	END_HALF_BLK 2			  
#define	END_DST_BLOCK 3			
#define	END_DST_FRAME 4			  
#define	END_DST_PACKET 5			  
#define	END_DST_INX_MODE 6		 
#define	DMA_FINISH 7				
#define	ERROR_SRCBUS 8		 
#define END_SRC_BLOCK 9		  
#define	END_SRC_FRAME 10           
#define END_SRC_PACKET 11         
#define	END_SRC_INX_MODE 12		  

/*!<error code difine*/
#define ERROR_ALLOCCHAN_TIMEOUT (1)
#define ERROR_TRANSFER_TIMEOUT (2)


typedef struct gpSdma_Chan_s {
	UINT8* srcAdress;
	UINT8* dstAdress;
	SINT32 size;
} gp_Sdma_t;

typedef struct gpSdma_irqInfo_s {
	SINT32 bitMask;
	SINT8* info;
	SINT32 meaning;
} gp_irqInfo_t;

SINT32 gp_sdma_memcpy_kernel(gp_Sdma_t* sdma );
#endif
