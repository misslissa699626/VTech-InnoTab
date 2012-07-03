/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2007 by Sunplus mMedia Inc.                      *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  mMedia Inc. All rights are reserved by Sunplus mMedia Inc.            *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus mMedia Inc. reserves the right to modify this software        *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus mMedia Inc.                                                   *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *                                                                        *
 **************************************************************************/

/**
 * @file mem.h
 * @brief The memory routine header.
 * @author kt.huang@sunplusmm.com
 */
#ifndef _MEM_H_
#define _MEM_H_

#if 1

#include <string.h>
#include <stdlib.h>

#define osMemAlloc(x)    malloc( x ) /*!< Macro for doing dynamic memory manageemnt. */
#define osMemFree(x)     free( ( void * )( x ) ) /*!< Macro for freeing memory. */

/* use system allocator */
#define DO_MALLOC(x)     malloc( x ) /*!< Macro for doing dynamic memory manageemnt. */
#define DO_FREE(x)       free( ( void * )( x ) ) /*!< Macro for freeing memory. */
#define DO_REALLOC(x, s) realloc( (x), (s) ) /*!< Macro for realloc memory. */
#define DO_CALLOC(x, s)  calloc( (x), (s) ) /*!< Macro for doing dynamic memory manageemnt. */
///#define DO_MALIGN(align, bytes)   __malign( (align), (bytes) ) /*!< Macro for allocating a block of memory of size bytes aligned to align (in bytes). */
#define DO_MALIGN(align, bytes)   malloc( (bytes) ) /*!< Macro for allocating a block of memory of size bytes aligned to align (in bytes). */
#define DO_MSIZE(x)			msize( ( void * )( x ) ) /*!< Macro for query memory size. */
#define DO_STRDUP(x)		strdup(x) /*!< Macro for realloc memory. */

#if 0
#ifdef ARM
extern void *chunkMemAlloc(UINT32 size);
extern void chunkMemFree(void *pMem);

#define CM_MALLOC(x)	chunkMemAlloc(x)	/*!< Macro for doing dynamic chunk memory manageemnt. */
#define CM_FREE(x)		chunkMemFree((void *)(x))	/*!< Macro for freeing chunk memory. */
#else
#define CM_MALLOC(x)	malloc(x)	/*!< Macro for doing dynamic chunk memory manageemnt. */
#define CM_FREE(x)		free((void *)(x))	/*!< Macro for freeing chunk memory. */
#endif
#endif
#define CM_MALLOC(x) gpChunkMemAlloc(x)
#define CM_FREE(x) gpChunkMemFree((void *)(x))
#else

#include "typedef.h"


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * @brief Allocate memory block
 * @param pid ID of the process allocating the memory block.
 * @param size Size of the memory block, in bytes.
 * @param file Filename of source code
 * @param line Line number of source code
 * @return On success, a pointer to the memory block allocated by the function.\n
 * The type of this pointer is always \c void*, which can be cast to the desired type of data pointer in order to be dereferenceable.\n
 * If the function failed to allocate the requested block of memory, a null pointer is returned.
 *
 * Allocates a block of \a size bytes of memory, returning a pointer to the beginning of the block.
 * The content of the newly allocated block of memory is not initialized, remaining with indeterminate values.
 */
void *
__malloc(
	UINT32 size
);


void *
__malign(
	UINT32 align,
	UINT32 size
);

/**
 * @brief Deallocate space in memory
 * @param ptr Pointer to a memory block previously allocated with __malloc() or __realloc() to be deallocated.\n
 * @param file Filename of source code
 * @param line Line number of source code
 * If a null pointer is passed as argument, no action occurs.
 *
 * A block of memory previously allocated using a call to __malloc() or __realloc() is deallocated, making it availbale again for further allocations.
 */
UINT32
__free(
	void *ptr
);



UINT32
__freeDbg( void *ptr, char *fileName, int line );

/**
 * @brief Reallocate memory block
 * @param pid ID of the process allocating the memory block.
 * @param ptr Pointer to a memory block previously allocated with malloc, calloc or realloc to be reallocated.\n
 * If this is NULL, a new block is allocated and a pointer to it is returned by the function.
 * @param size Size of the memory block, in bytes.
 * @param file Filename of source code
 * @param line Line number of source code
 *
 * The size of the memory block pointed to by the ptr parameter is changed to the size bytes, expanding or reducing the amount of memory available in the block.
 *
 * The function may move the memory block to a new location, in which case the new location is returned. The content of the memory block is preserved up to the lesser of the new and old sizes, even if the block is moved. If the new size is larger, the value of the newly allocated portion is indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is deallocated as if a call to free was made, and a NULL pointer is returned.
 */
void *
__realloc(
	void *ptr,
	UINT32 size
);

/**
 * @brief		Query the size of an allocated memory
 * @param ptr	The memory pointer
 * @param file Filename of source code
 * @param line Line number of source code
 */
UINT32
__msize(
	void *ptr
);

/**
 * @brief Duplicate string
 * @param str null-terminated string to duplicate.
 * @return On success, this returns a pointer to a newly allocated copy of the string \a str.\n
 * @param file Filename of source code
 * @param line Line number of source code
 * On failure, this returns a null pointer.
 *
 * Allocates memory and copies into it the string addressed by \a str, including the terminating null character.\n
 * It is the user's responsibility to free the allocated storage by calling __free().
 */
void *
__strdup(
	const UINT8 *str
);


/**
 * @breif calloc.
 */
void *
__calloc(
	UINT32 num,
	UINT32 size
);


/**
 * @breif memcpy.
 */
void *
__memcpy(
	void *dst0,
	void *src0,
	UINT32 len0
);


/**
 * @brief Initialize buddy memory system
 *
 * Must invoke this first once before invoking any of the buddy memory functions.
 */
void
__memInit(
	void
);

/**
 * @brief Register the garbage collection method
 * @param gc The garbage collection method to register.
 *
 * The registered method will be invoked when garbage collection is needed.
 */
void
registerGarbageCollector(
	void (*gc)(void)
);

/**
 * @brief Show the buddy memory usage
 *
 * The usage will be printed to the standard output.
 */
void
showProcess(
	void
);


/**
 * @brief Dump the buddy memory
 */
void
buddyMemDump(
	void
);

/**
 * @brief Dump the buddy memory
 */
void
buddyMemDumpTree(
	void
);


#if DO_MEMLOG
#define DO_MALLOC(x)		__mallocModule( __FILE__, __LINE__, x )
#define DO_FREE(x)			__freeModule( __FILE__, __LINE__, ( void * )( x ) ) /*!< Macro for freeing memory. */
#define DO_REALLOC(x, s)	__reallocModule( __FILE__, __LINE__, ( void * )( x ), s )
#define DO_CALLOC(x, s)		__callocModule( __FILE__, __LINE__, x, s )

/* API for memory debug */
extern void *__mallocModule( UINT8 *pModuleName, UINT32 line, UINT32 size );
extern void __freeModule( UINT8 *pModuleName, UINT32 line, void *p );
extern void *__reallocModule( UINT8 *pModuleName, UINT32 line, void *p, UINT32 size );
extern void *__callocModule( UINT8 *pModuleName, UINT32 line, UINT32 num, UINT32 size );

#else

#define DO_MALLOC(x)    __malloc( x ) /*!< Macro for doing dynamic memory manageemnt. */
#define DO_FREE(x)      __free( ( void * )( x ) ) /*!< Macro for freeing memory. */
#define DO_REALLOC(x, s)	__realloc( (x), (s) ) /*!< Macro for realloc memory. */
#define DO_CALLOC(x, s)		__calloc( (x), (s) ) /*!< Macro for doing dynamic memory manageemnt. */
#endif

#define DO_MALIGN(align, bytes)		__malign( align, bytes ) /*!< Macro for allocating a block of memory of size bytes aligned to align (in bytes). */
#define DO_MSIZE(x)			__msize( ( void * )( x ) ) /*!< Macro for query memory size. */
#define DO_STRDUP(x)		__strdup(x) /*!< Macro for realloc memory. */
#define DO_MEMCPY	__memcpy /*!< Macro for memory copy. */


/**
 * @brief Bitstream buffer if fixed memory area.
 * @author kt.huang@sunplusmm.com
 */
#define JPEG_DEC_BS		0	/*!< JPEG decode bitstream */

/**
 * @brief Get pointer of fixed memory.
 * @param id The memory id.
 * @param size The memory size.
 * @retval 0 if ths ID is not supported.
 */
UINT32 *fixMemGet( UINT32 id, UINT32 *size );

/* Import old osMem API here */
void*  osMemAlloc(UINT32 size);
void*  osMemAllocCache(UINT32 size);
UINT32 osMemFree(void *ptr);
void*  osMemRealloc(void *ptr, UINT32 size);
#endif


#endif

