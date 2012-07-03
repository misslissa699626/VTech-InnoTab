#include <linux/delay.h>

#include "hal_base.h"
#include "bch.h"

int bchProcess(unsigned long* a_PyldBuffer,unsigned long* a_ReduntBuffer,int a_BufLen,int a_OP)
{
	int i=0,nValue=0,retBlkNo=0;
	
	rBCH_DATA_START_ADDR = (unsigned long)a_PyldBuffer;
	rBCH_PARITY_START_ADDR = (unsigned long)a_ReduntBuffer;
			
	switch(a_OP)
	{
		case BCH_ENCODE:
				rBCH_INT_MASK = BCH_FINISH_MASK(1)|BCH_DECODE_FAIL_MASK(1);
				nValue =BCH_SET_BLOCK_NUM(BCH_DIV512(a_BufLen))|BCH_8BIT_REDO_MODE(1)|BCH_MODE(0)|BCH_START;
				rBCH_CFG = nValue;
		break;
		
		case BCH_DECODE:
				rBCH_INT_MASK = BCH_FINISH_MASK(1)|BCH_DECODE_FAIL_MASK(1);
				nValue =BCH_SET_BLOCK_NUM(BCH_DIV512(a_BufLen))|BCH_8BIT_REDO_MODE(1)|BCH_MODE(1)|BCH_START;
				rBCH_CFG = nValue;
		break;
		
		default:
		break;
	}
	
	for(i=1;i<200;i++)
	{
		rBCH_STATUS =nValue;
		if((nValue&0x03) != 0)
		{
			break;
		}
		else
		{
			mdelay(10);
			continue;
		}
	}
		if((nValue&BCH_DECODE_FAIL) != BCH_DECODE_FAIL)
		{
			rBCH_INT = BCH_START;
			rBCH_SOFT_RESET = BCH_START;
			return OK_DEV_BCH;
		}
		else
		{
			retBlkNo = BCH_ERROR_BLK_NO(nValue);
			rBCH_INT = BCH_START;
			rBCH_SOFT_RESET = BCH_START;
			return ERR_BCH_ERROR ;
		}

	rBCH_INT = BCH_START;
	rBCH_SOFT_RESET = BCH_START;
	return ERR_DEV_BCH_DECODE;
}
//----------------------------------------------------------------------------

