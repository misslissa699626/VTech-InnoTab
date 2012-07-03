
#ifdef	_WIN32
//#include "stdafx.h"
#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>


#include "../include/nand_cfg.h"
#include "../include/fifo.h"
#else
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "../hal/hal_base.h"
#include "fifo.h"
#endif//#ifdef	_WIN32


#define FIFO_INCREASE_INDEX(size, x) ((++x >= size)? (x=0) : (x))

void FIFO_Get_Info(FIFO_Manage_t *p_fifo)
{
	printk("\n");
	printk("**********************FIFO INFO*************************\n");
	printk("p_fifo:0x%x\n", (unsigned int)p_fifo);
	printk("p_fifo->size:%d\n", p_fifo->size);	
	printk("p_fifo->p_index:%d\n", p_fifo->p_index);	
	printk("p_fifo->c_index:%d\n", p_fifo->c_index);	
	printk("p_fifo->reserved:%d\n", p_fifo->reserved);	
	printk("********************************************************\n");
	
}

unsigned int FIFO_new(FIFO_Manage_t *p_fifo)
{
	
	printk("FIFO_new malloc(%d)\n", p_fifo->size * sizeof(unsigned short));
	p_fifo->plist = (unsigned short*)memAlloc(p_fifo->size * sizeof(unsigned short));	
	if(p_fifo->plist)
	{
		return SUCCESS;
	}

	return FAILURE;
}

void FIFO_destroy(FIFO_Manage_t *p_fifo)
{
	if(p_fifo)
	{
		if(p_fifo->plist)
		{
			memFree(p_fifo->plist);
			p_fifo->plist = NULL;
		}
	}
}

//unsigned int FIFO_Init(unsigned short size)
unsigned int FIFO_Init(FIFO_Manage_t *p_fifo, unsigned int size)
{
	unsigned int state;
	p_fifo->c_index=0;
	p_fifo->p_index=0;
	p_fifo->reserved = 1;
	p_fifo->Recovering = MAX_FIFO_RESERVATIONS;//No reciverig
	p_fifo->size = size + p_fifo->reserved;

	state = FIFO_new(p_fifo);
	if(state==FAILURE)
	{
		return FAILURE;
	}
	
	return SUCCESS;		
}
void FIFO_free(FIFO_Manage_t *p_fifo)
{
	FIFO_destroy(p_fifo);
}

// empty ==>return SUCCESS
unsigned int FIFO_empty(FIFO_Manage_t *p_fifo)
{
	if(p_fifo->c_index == p_fifo->p_index)
	{
		return SUCCESS;
	}
	
	return FAILURE;
}
// full ==>return SUCCESS
unsigned int FIFO_full(FIFO_Manage_t *p_fifo)
{
	unsigned short tmp;
	tmp = p_fifo->p_index;

	FIFO_INCREASE_INDEX(p_fifo->size, tmp);

	if(tmp == p_fifo->c_index)
	{
		return SUCCESS;
	}
	
	return FAILURE;
}


unsigned int FIFO_Recovery(FIFO_Manage_t *p_fifo)
{
	unsigned short c_index =p_fifo->c_index;

	if(p_fifo->p_index >= p_fifo->c_index)
	{
		if(p_fifo->p_index >= p_fifo->c_index+p_fifo->Recovering)
		{
			return FAILURE;
		}
		else
		{
			return SUCCESS;	
		}
	}
	else//g_fifo.p_index < g_fifo.c_index 
	{
		c_index += p_fifo->Recovering;	
		if(c_index < p_fifo->size)
		{
			return FAILURE;
		}
		else //	c_index > g_fifo.size
		{
			c_index -= p_fifo->size;
			if(p_fifo->p_index >= c_index)
			{
				return FAILURE;
			}
			else
			{
				return SUCCESS;					
			}	
				
		}
	}

	return SUCCESS;	
}

///////////////////////////////////////////////////////////////////
unsigned int FIFO_pop(FIFO_Manage_t *p_fifo, unsigned short *val)
{
	*val = NO_BLOCK;
	
	if(FIFO_empty(p_fifo)==SUCCESS)
	{
		printk("ERROR : FIFO is empty!\n");
		FIFO_Get_Info(p_fifo);
		return FAILURE;
	}

	*val = p_fifo->plist[p_fifo->c_index];

	FIFO_INCREASE_INDEX(p_fifo->size, p_fifo->c_index);

	if(FIFO_Recovery(p_fifo)==SUCCESS)
	{		
		return RECOVER;
	}
	
	return SUCCESS;

}

unsigned int FIFO_push(FIFO_Manage_t *p_fifo, unsigned short val)
{
	if(FIFO_full(p_fifo)==SUCCESS)
	{
		printk("ERROR : FIFO is full!\n");
		FIFO_Get_Info(p_fifo);
		return FAILURE;
	}

	p_fifo->plist[p_fifo->p_index] = val;

	FIFO_INCREASE_INDEX(p_fifo->size, p_fifo->p_index);

	return SUCCESS;
}

