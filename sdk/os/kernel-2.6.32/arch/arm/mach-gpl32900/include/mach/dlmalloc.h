#ifndef _DL_MALLOC_H_
#define _DL_MALLOC_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef struct mem_info_s {
		unsigned int id;
		unsigned int total_bytes;
		unsigned int used_bytes;
		unsigned int max_free_block_bytes;
	} mem_info_t;

	/* ------------------- Declarations of public routines ------------------- */
	SINT32 dlMallocInit(void* tbase, UINT32 tsize, UINT32 page_size);
	/*
	malloc(UINT32 n)
	Returns a pointer to a newly allocated chunk of at least n bytes, or
	null if no space is available, in which case errno is set to ENOMEM
	on ANSI C systems.

	If n is zero, malloc returns a minimum-sized chunk. (The minimum
	size is 16 bytes on most 32bit systems, and 32 bytes on 64bit
	systems.)  Note that UINT32 is an unsigned type, so calls with
	arguments that would be negative if signed are interpreted as
	requests for huge amounts of space, which will often fail. The
	maximum supported value of n differs across systems, but is in all
	cases less than the maximum representable value of a UINT32.
	*/
	void* dlMalloc(UINT32 id, UINT32 bytes);

	/*
	free(void* p)
	Releases the chunk of memory pointed to by p, that had been previously
	allocated using malloc or a related routine such as realloc.
	It has no effect if p is null. If p was not malloced or already
	freed, free(p) will by default cause the current program to abort.
	*/
	SINT32 dlFree(void* mem);

	/*

	*/
	SINT32 dlFreeAll(UINT32 id);
	/*
	calloc(UINT32 n_elements, UINT32 element_size);
	Returns a pointer to n_elements * element_size bytes, with all locations
	set to zero.
	*/
	void* dlCalloc(UINT32 id, UINT32 n_elements, UINT32 elem_size);

	/*
	realloc(void* p, UINT32 n)
	Returns a pointer to a chunk of size n that contains the same data
	as does chunk p up to the minimum of (n, p's size) bytes, or null
	if no space is available.

	The returned pointer may or may not be the same as p. The algorithm
	prefers extending p in most cases when possible, otherwise it
	employs the equivalent of a malloc-copy-free sequence.

	If p is null, realloc is equivalent to malloc.

	If space is not available, realloc returns null, errno is set (if on
	ANSI) and p is NOT freed.

	if n is for fewer bytes than already held by p, the newly unused
	space is lopped off and freed if possible.  realloc with a size
	argument of zero (re)allocates a minimum-sized chunk.

	The old unix realloc convention of allowing the last-free'd chunk
	to be used as an argument to realloc is not supported.
	*/

	void* dlRealloc(UINT32 id, void* oldmem, UINT32 bytes);

	/*
	malloc_stats();
	Prints on stderr the amount of space obtained from the system (both
	via sbrk and mmap), the maximum amount (which may be more than
	current if malloc_trim and/or munmap got called), and the current
	number of bytes allocated via malloc (or realloc, etc) but not yet
	freed. Note that this is the number of bytes allocated, not the
	number requested. It will be larger than the number requested
	because of alignment and bookkeeping overhead. Because it includes
	alignment wastage as being in use, this figure may be greater than
	zero even when no user-level chunks are allocated.

	The reported current and maximum system memory can be inaccurate if
	a program makes other calls to system memory allocation functions
	(normally sbrk) outside of malloc.

	malloc_stats prints only the most commonly interesting statistics.
	More information can be obtained by calling mallinfo.
	*/
	void  dlMalloc_Status(mem_info_t *info);

	/*
	malloc_usable_size(void* p);

	Returns the number of bytes you can actually use in
	an allocated chunk, which may be more than you requested (although
	often not) due to alignment and minimum size constraints.
	You can use this many bytes without worrying about
	overwriting other allocated objects. This is not a particularly great
	programming practice. malloc_usable_size can be more useful in
	debugging and assertions, for example:

	p = malloc(n);
	*/
	UINT32 dlMalloc_Usable_Size(void* mem);

	/* increase mem chunk's reference count */
	UINT32 dlShare(void* addr);

	void* dlMallocEx(UINT32 id, UINT32 bytes);

#ifdef __cplusplus
};  /* end of extern "C" */
#endif /* __cplusplus */

#endif
