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
 * @file    gp_reg.h
 * @brief   Declaration of register dump/fill driver
 * @author  Daolong Li
 */
#ifndef _GP_REG_H_
#define _GP_REG_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Ioctl for device node definition */
#define REG_IOCTL_MAGIC	'R'
#define REG_DUMP		_IOWR(REG_IOCTL_MAGIC, 1, unsigned int)
#define REG_FILL		_IOW(REG_IOCTL_MAGIC,  2, unsigned int)

/* Dump / Fill format */
#define REG_FORMAT_BYTE		(sizeof(unsigned char))
#define REG_FORMAT_WORD		(sizeof(unsigned short))
#define REG_FORMAT_DWORD	(sizeof(unsigned int))

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
 /**
*@bridef register command struct
*/
typedef struct gp_reg_cmd_s{
	unsigned int addr;		/*!< @brief register address*/
	unsigned int len;		/*!< @brief length of address to dump/fill*/
	unsigned int format;	/*!< @brief data format, accept REG_FORMAT_BYTE, REG_FORMAT_WORD, REG_FORMAT_DWORD*/	
	unsigned int data;		/*!< @brief value for fill*/
} gp_reg_cmd_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
*@brief Dump register 
*@param addr Start address of register
*@param len Length of register to be dump
*@param format How many bytes to be dump, accept REG_FORMAT_BYTE, REG_FORMAT_WORD,  REG_FORMAT_DWORD
 *@return 0 Success; Other fail
*/
int gp_reg_dump(unsigned int addr, unsigned int len, unsigned int format);

/**
*@brief Fill register
*@param addr Start address of register
*@param len Length of register to be filled
*@param format How many bytes to be dump, accept REG_FORMAT_BYTE, REG_FORMAT_WORD,  REG_FORMAT_DWORD
*@param value data to be filled
 *@return 0 Success; Other fail
*/
int gp_reg_fill(unsigned int addr, unsigned int len, unsigned int format, unsigned int value);
 

#endif /* _GP_REG_H_ */
