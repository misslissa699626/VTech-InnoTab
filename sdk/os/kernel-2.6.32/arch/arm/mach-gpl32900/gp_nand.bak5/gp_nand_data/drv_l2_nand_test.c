#include "drv_l2_data_hal.h"
//#include "../gp_nand_hal/hal_nand.h"
#include "drv_l2_nand_manage.h"

extern void Nand_Malloc_Buffer(void);
extern SINT32 nand_read_id_all(UINT16* MainID, UINT32* VendorID);
void read_write_code(void);

extern UINT16	DrvNand_flush_allblk(void);
extern UINT16 DrvNand_lowlevelformat(void);
extern UINT16 DrvNand_initial(void);

#define NF_TEST_DIAG(...)

void gp_nand_test(void)
{
//		UINT16 main_id;
//		UINT32 vendor_id;
//		UINT32 i,j,m;
		UINT32 ret;
		
		//DrvNand_lowlevelformat();
		
		ret = DrvNand_initial();	
		if(ret !=0)
		{
			//while(1)
			for(ret = 0;ret<50;ret++)
			{
				printk("==Nand initial Fail!!==\n");
			}			
#ifdef PART0_WRITE_MONITOR_DEBUG			
			printk("==Hangup to debug!!==\n");
			while(1);
#endif			
		} 
		else 
		{
			NF_TEST_DIAG("==Nand initial success!!==\n");	
		}
		
		//read_write_code();
}

void read_write_code(void)
{
#if 0
	UINT32 i,j;
	UINT32 ret;    
    UINT8  *buffer1;
	UINT8  *buffer2;
	unsigned long test_loop = 0;
	
	buffer1 = (UINT8*)kmalloc(512, GFP_DMA);	//parse header时，page size最大为1024
	buffer2 = (UINT8*)kmalloc(512, GFP_DMA);	//parse header时，page size最大为1024
	if((buffer1==0)||(buffer2==0))
	{
		printk("Can`t malloc test buffer \n");
		return;
	}

	
while(1)	
{
    for(j = 0;j < 10240;j++)
    {	
		memset(buffer1,j,512);
    	ret = DrvNand_write_sector(j, 1, (UINT32)buffer1,0);
    	if(ret != 0)
		{
    		NF_TEST_DIAG("DrvNand_write_sector Fail!! loop:%d \n",test_loop);
    		while(1);
    	}
		else
		{
			printk("write sector 0x%x ok loop:%d \n",j,test_loop);
		}
    }
  
    for(j = 0;j < 10240;j++)
    {
		memset(buffer1,j,512);
	    ret = DrvNand_read_sector(j, 1, (UINT32)buffer2,0);
	    if(ret != 0) 
		{
	    	NF_TEST_DIAG("DrvNand_read_sector Fail!!,loop:%d \n",test_loop);
	    	while(1);
	    } 
		
		if(!memcmp(buffer1,buffer2,512))
		{
			printk("CMP sector 0x%x ok loop:%d \n",j,test_loop);
		}
		else
		{
			printk("CMP sector 0x%x failed loop:%d------> \n",j,test_loop);
			for(i=0;i<512;i++)
			{
				printk(" 0x%x ",buffer1[i]);
			}
			
			for(i=0;i<512;i++)
			{
				printk(" 0x%x ",buffer2[i]);
			}
			while(1);
		}
    }
	
	test_loop++;
}
	DrvNand_flush_allblk();	
#endif	
}
