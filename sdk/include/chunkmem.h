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
 * @file    chunkmem.H
 * @brief   Declare 2d api wrap for device driver.
 * @author  clhuang
 * @since   2010-12-03
 * @date    2010-12-03
 */
#ifndef _CHUNK_MEM_H_
#define _CHUNK_MEM_H_

#include <stdint.h>
#include "typedef.h"

/**
 * @brief chunkmem alloc.
 * @param UINT32 size.
 * @retval the mem addr or NULL.
 */
void* gpChunkMemAlloc(UINT32 size);

/**
 * @brief chunkmem free.
 * @param void *pMem.
 */
void gpChunkMemFree(void *pMem);

#endif /* _CHUNK_MEM_H_ */

