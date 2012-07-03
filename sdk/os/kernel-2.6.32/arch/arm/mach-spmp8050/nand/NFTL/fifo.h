#ifndef _FIFO_H_
#define _FIFO_H_

#ifdef	_WIN32
#include "nand_cfg.h"
#else
#include "nand_cfg.h"
#endif //#ifdef	_WIN32

//#define MAX_FIFO_RESERVATIONS 3

typedef struct FIFO_Manage_Data
{
	unsigned short p_index;
	unsigned short c_index;
	unsigned int size;
	unsigned short blk_count;
	unsigned char Recovering;
	unsigned char reserved;
}FIFO_Manage_Data_t;

typedef struct FIFO_Manage
{
	unsigned short p_index;
	unsigned short c_index;
	unsigned int size;
	unsigned short blk_count;
	unsigned char Recovering;
	unsigned char reserved;
	unsigned short *plist;
}FIFO_Manage_t;

unsigned int FIFO_Init(FIFO_Manage_t *p_fifo, unsigned int size);
void FIFO_free(FIFO_Manage_t *p_fifo);
unsigned int FIFO_pop(FIFO_Manage_t *p_fifo, unsigned short *val);
unsigned int FIFO_push(FIFO_Manage_t *p_fifo, unsigned short val);
void FIFO_Get_Info(FIFO_Manage_t *p_fifo);

#endif
