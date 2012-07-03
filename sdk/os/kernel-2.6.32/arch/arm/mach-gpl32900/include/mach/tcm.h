//#define iTCMSIZE		0x05   //define TCM size = 16kb
//#define dTCMSIZE		0x05   //define TCM size = 16kb

#define 	TCM_ALIGN64K_MASK		0xFFFF0000		//	__align(64*1024)

void ITCM_Enable(unsigned long addr);
void ITCM_Disable(void);
void DTCM_Enable(unsigned long addr);
void DTCM_Disable(void);
