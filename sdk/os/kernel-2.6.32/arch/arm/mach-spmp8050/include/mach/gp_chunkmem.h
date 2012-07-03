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
 * @file    gp_chunkmem.h
 * @brief   Declaration of GP Chunk Memory Driver data structure &
 *  		interface.
 * @author  qinjian
 * @since   2010-09-01
 * @date    2010-09-01
 */
#ifndef _GP_CHUNKMEM_H_
#define _GP_CHUNKMEM_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Ioctl for device node definition */
#define CHUNK_MEM_IOCTL_MAGIC	'C'
#define CHUNK_MEM_ALLOC			_IOWR(CHUNK_MEM_IOCTL_MAGIC, 1, chunk_block_t)
#define CHUNK_MEM_SHARE         _IOWR(CHUNK_MEM_IOCTL_MAGIC, 2, chunk_block_t)
#define CHUNK_MEM_FREE			_IOW(CHUNK_MEM_IOCTL_MAGIC,  3, chunk_block_t)
#define CHUNK_MEM_INFO          _IOWR(CHUNK_MEM_IOCTL_MAGIC, 4, chunk_info_t)
#define CHUNK_MEM_VA2PA         _IOWR(CHUNK_MEM_IOCTL_MAGIC, 5, chunk_block_t)
#define CHUNK_MEM_MMAP          _IOWR(CHUNK_MEM_IOCTL_MAGIC, 6, chunk_block_t)
#define CHUNK_MEM_MUNMAP        _IOW(CHUNK_MEM_IOCTL_MAGIC,  7, chunk_block_t)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct chunk_block_s {
	unsigned int phy_addr; /*!< @brief start address of memory block, physical_addr */
	void *addr;            /*!< @brief start address of memory block, user_addr */
	unsigned int size;     /*!< @brief size of memory block */
} chunk_block_t;

typedef struct chunk_info_s {
	unsigned int pid;                   /*!< @breif [in] pid for query process used bytes, 0 for all, -1 for current proc */
	unsigned int total_bytes;           /*!< @brief [out] total size of chunkmem in bytes */
	unsigned int used_bytes;            /*!< @brief [out] used chunkmem size in bytes */
	unsigned int max_free_block_bytes;  /*!< @brief [out] max free block size in bytes */
} chunk_info_t;

/* callback function for gp_chunk_suspend save data */
typedef void (*save_data_proc)(unsigned long addr, unsigned long size);

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

void *gp_chunk_malloc(unsigned int id, unsigned int size);
void gp_chunk_free(void *addr);
void gp_chunk_free_all(unsigned int id);
void *gp_chunk_va(unsigned int pa);
unsigned int gp_chunk_pa(void *ka);
unsigned int gp_user_va_to_pa(void *va);
void gp_sync_cache(void);
int gp_chunk_suspend(save_data_proc save_proc);

#endif /* _GP_CHUNKMEM_H_ */
