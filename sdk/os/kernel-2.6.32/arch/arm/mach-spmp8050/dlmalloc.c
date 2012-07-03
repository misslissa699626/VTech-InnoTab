//==========================================================================
//
// Author(s):    Doug Lea (dl at g.oswego.edu), jlarmour
// Version:      2.8.4
// Contributors:
// Date:         2009-05-27
// Purpose:      Doug Lea's malloc implementation
// Description:
// Usage:
//
//
//
//==========================================================================
#define IN_LINUX_KERNEL
#ifdef IN_LINUX_KERNEL
#include <mach/kernel.h>
#include <mach/dlmalloc.h>
#include <mach/gp_chunkmem.h>

#ifndef diag_printf
#define diag_printf printk
#endif
#define time(t) jiffies
#else
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "dlmalloc.h"

#ifndef diag_printf
#define diag_printf printf
#endif
#define PAGE_SIZE 4096U
#endif
#ifndef SUCCESS
#define SUCCESS   0
#endif
/* The maximum possible UINT32 value has all bits set */
#define MAX_SIZE_T           (~(UINT32)0)
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT     ((UINT32)PAGE_SIZE)
#endif  /* MALLOC_ALIGNMENT */
#ifndef ABORT
#define ABORT  abort()
#endif  /* ABORT */

/* ------------------- UINT32 and alignment properties -------------------- */

/* The byte and bit size of a UINT32 */
#define SIZE_T_SIZE         (sizeof(UINT32))
#define SIZE_T_BITSIZE      (sizeof(UINT32) << 3)

/* Some constants coerced to UINT32 */
/* Annoying but necessary to avoid errors on some platforms */
#define SIZE_T_ZERO         ((UINT32)0)
#define SIZE_T_ONE          ((UINT32)1)
#define SIZE_T_TWO          ((UINT32)2)
#define SIZE_T_FOUR         ((UINT32)4)
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define FOUR_SIZE_T_SIZES   (SIZE_T_SIZE<<2)
#define SIX_SIZE_T_SIZES    (FOUR_SIZE_T_SIZES+TWO_SIZE_T_SIZES)
#define HALF_MAX_SIZE_T     (MAX_SIZE_T / 2U)

/* The bit mask value corresponding to MALLOC_ALIGNMENT */
#define CHUNK_ALIGN_MASK    (MALLOC_ALIGNMENT - SIZE_T_ONE)

/* True if address a has acceptable alignment */
#define is_aligned(A)       (((UINT32)((A)) & (CHUNK_ALIGN_MASK)) == 0)

/* the number of bytes to offset an address to align it */
#define align_offset(A)\
	((((UINT32)(A) & CHUNK_ALIGN_MASK) == 0)? 0 :\
	((MALLOC_ALIGNMENT - ((UINT32)(A) & CHUNK_ALIGN_MASK)) & CHUNK_ALIGN_MASK))

/* -------------------------- MMAP preliminaries ------------------------- */

/* MORECORE and MMAP must return MFAIL on failure */
#define MFAIL                ((void*)(MAX_SIZE_T))
#define CMFAIL               ((char*)(MFAIL)) /* defined for convenience */


typedef struct malloc_chunk_s
{
	UINT32                 prev_foot;  /* Size of previous chunk (if free).  */
	UINT32                 head;       /* Size and inuse bits. */
	struct malloc_chunk_s* fd;         /* double links -- used only if free. */
	struct malloc_chunk_s* bk;
}malloc_chunk_t;

typedef struct malloc_chunk_s  mchunk;
typedef struct malloc_chunk_s* mchunkptr;
typedef struct malloc_chunk_s* sbinptr;  /* The type of bins of chunks */
typedef UINT32 bindex_t;         /* Described below */
typedef UINT32 binmap_t;         /* Described below */
typedef UINT32 flag_t;           /* The type of various bit flag sets */

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MCHUNK_SIZE         (sizeof(mchunk))
#define CHUNK_OVERHEAD      (sizeof(mchunk))//(SIZE_T_SIZE)

/* The smallest size we can malloc is an aligned minimal chunk */
#define MIN_CHUNK_SIZE\
	((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* conversion from malloc headers to user pointers, and back */
#define chunk2mem(p)        ((void*)((char*)(p)       + FOUR_SIZE_T_SIZES))//TWO_SIZE_T_SIZES))
#define mem2chunk(mem)      ((mchunkptr)((char*)(mem) - FOUR_SIZE_T_SIZES))//TWO_SIZE_T_SIZES))
/* chunk associated with aligned address A */
#define align_as_chunk(A)   (mchunkptr)((A) + align_offset(chunk2mem(A)))

/* Bounds on request (not chunk) sizes. */
#define MAX_REQUEST         ((-MIN_CHUNK_SIZE) << 2)
#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - SIZE_T_ONE)

/* pad request bytes into a usable size */
#define PAD_REQUEST(req) \
	(((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* pad request, checking for minimum (but not maximum) */
#define REQUEST2SIZE(req) \
	(((req) < MIN_REQUEST)? MIN_CHUNK_SIZE : PAD_REQUEST(req))


/* ------------------ Operations on head and foot fields ----------------- */

#define PINUSE_BIT          (SIZE_T_ONE)
#define CINUSE_BIT          (SIZE_T_TWO)
#define FLAG4_BIT           (SIZE_T_FOUR)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)
#define FLAG_BITS           (PINUSE_BIT|CINUSE_BIT|FLAG4_BIT)

/* Head value for fenceposts */
#define FENCEPOST_HEAD      (INUSE_BITS|SIZE_T_SIZE)

/* extraction of fields from head words */
#define cinuse(p)           ((p)->head & CINUSE_BIT)
#define pinuse(p)           ((p)->head & PINUSE_BIT)
#define is_inuse(p)         (((p)->head & INUSE_BITS) != PINUSE_BIT)
#define chunksize(p)        ((p)->head & ~(FLAG_BITS))
#define clear_pinuse(p)     ((p)->head &= ~PINUSE_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define chunk_plus_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))
#define chunk_minus_offset(p, s) ((mchunkptr)(((char*)(p)) - (s)))

/* Ptr to next or previous physical malloc_chunk_s. */
#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->head & ~FLAG_BITS)))
#define prev_chunk(p) ((mchunkptr)( ((char*)(p)) - ((p)->prev_foot) ))

/* extract next chunk's pinuse bit */
#define next_pinuse(p)  ((next_chunk(p)->head) & PINUSE_BIT)

/* Get/set size at footer */
#define get_foot(p, s)  (((mchunkptr)((char*)(p) + (s)))->prev_foot)
#define set_foot(p, s)  (((mchunkptr)((char*)(p) + (s)))->prev_foot = (s))

/* Set size, pinuse bit, and foot */
#define set_size_and_pinuse_of_free_chunk(p, s)\
	((p)->head = (s|PINUSE_BIT), set_foot(p, s))

/* Set size, pinuse bit, foot, and clear next pinuse */
#define set_free_with_pinuse(p, s, n)\
	(clear_pinuse(n), set_size_and_pinuse_of_free_chunk(p, s))

/* Get the internal overhead associated with chunk p */
#define overhead_for(p) (CHUNK_OVERHEAD)


typedef struct malloc_tree_chunk_s
{
	/* The first four fields must be compatible with malloc_chunk_s */
	UINT32                      prev_foot;
	UINT32                      head;
	struct malloc_tree_chunk_s* fd;
	struct malloc_tree_chunk_s* bk;

	struct malloc_tree_chunk_s* child[2];
	struct malloc_tree_chunk_s* parent;
	bindex_t                    index;
}malloc_tree_chunk_t;

typedef struct malloc_tree_chunk_s  tchunk;
typedef struct malloc_tree_chunk_s* tchunkptr;
typedef struct malloc_tree_chunk_s* tbinptr; /* The type of bins of trees */

/* A little helper macro for trees */
#define leftmost_child(t) ((t)->child[0] != 0? (t)->child[0] : (t)->child[1])

/* ----------------------------- Segments -------------------------------- */


typedef struct malloc_segment_s
{
	char*        base;             /* base address */
	UINT32       size;             /* allocated size */
	struct malloc_segment_s* next;   /* ptr to next segment */
	flag_t       sflags;           /* mmap and extern flag */
}malloc_segment_t;

typedef struct malloc_segment_s  msegment;
typedef struct malloc_segment_s* msegmentptr;

/* Bin types, widths and sizes */
#define NSMALLBINS        (32U)
#define NTREEBINS         (32U)
#define SMALLBIN_SHIFT    (3U)
#define TREEBIN_SHIFT     (8U)
#define MIN_LARGE_SIZE    (SIZE_T_ONE << TREEBIN_SHIFT)
#define MAX_SMALL_SIZE    (MIN_LARGE_SIZE - SIZE_T_ONE)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

typedef struct malloc_state_s
{
	binmap_t   smallmap;
	binmap_t   treemap;
	UINT32     dvsize;
	UINT32     topsize;
	char*      least_addr;
	mchunkptr  dv;
	mchunkptr  top;
	UINT32     magic;
	mchunkptr  smallbins[(NSMALLBINS+1)*2];
	tbinptr    treebins[NTREEBINS];
	UINT32     footprint;
	msegment   seg;
	void*      extp;      /* Unused but available for extensions */
	UINT32     exts;
	UINT32     page_size;
	UINT32     granularity;
}malloc_state_t;

typedef malloc_state_t*    ptrMstate;

/* The global malloc_state used for all non-"mspace" calls */
static  malloc_state_t      *pgdl_GM;
#define is_initialized(M)  ((M)->top != 0)

/* -------------------------- system alloc setup ------------------------- */

/* Operations on mflags */

/* page-align a size */
#define page_align(S)\
	(((S) + (pgdl_GM->page_size - SIZE_T_ONE)) & ~(pgdl_GM->page_size - SIZE_T_ONE))

/* granularity-align a size */
#define granularity_align(S)\
	(((S) + (pgdl_GM->granularity - SIZE_T_ONE))\
	& ~(pgdl_GM->granularity - SIZE_T_ONE))

#define is_page_aligned(S)\
	(((UINT32)(S) & (pgdl_GM->page_size - SIZE_T_ONE)) == 0)
#define is_granularity_aligned(S)\
	(((UINT32)(S) & (pgdl_GM->granularity - SIZE_T_ONE)) == 0)

/*  True if segment S holds address A */
#define segment_holds(S, A)\
	((char*)(A) >= S->base && (char*)(A) < S->base + S->size)

/* Return segment holding given address */
static msegmentptr
segment_holding(
	ptrMstate m,
	char* addr
)
{
	msegmentptr sp = &m->seg;
	for (;;){
		if (addr >= sp->base && addr < sp->base + sp->size){
			return sp;
		}
		if ((sp = sp->next) == 0){
			return 0;
		}
	}
}

#if 0
#define TOP_FOOT_SIZE\
	(align_offset(chunk2mem(0))+PAD_REQUEST(sizeof(struct malloc_segment_s))+MIN_CHUNK_SIZE)
#else
#define TOP_FOOT_SIZE align_offset(chunk2mem(0))
#endif


static UINT32   check_any_chunk(ptrMstate m, mchunkptr p);
static UINT32   check_top_chunk(ptrMstate m, mchunkptr p);
static UINT32   check_inuse_chunk(ptrMstate m, mchunkptr p);
static UINT32   check_free_chunk(ptrMstate m, mchunkptr p);
static UINT32   check_malloced_chunk(ptrMstate m, void* mem, UINT32 s, UINT32 id);
static UINT32   check_tree(ptrMstate m, tchunkptr t);
static UINT32   check_treebin(ptrMstate m, bindex_t i);
static UINT32   check_smallbin(ptrMstate m, bindex_t i);
static UINT32   check_malloc_state(ptrMstate m);
static SINT32   bin_find(ptrMstate m, mchunkptr x);
static UINT32   traverse_and_check(ptrMstate m);

/* ---------------------------- Indexing Bins ---------------------------- */

#define is_small(s)         (((s) >> SMALLBIN_SHIFT) < NSMALLBINS)
#define small_index(s)      ((s)  >> SMALLBIN_SHIFT)
#define small_index2size(i) ((i)  << SMALLBIN_SHIFT)
#define MIN_SMALL_INDEX     (small_index(MIN_CHUNK_SIZE))

/* addressing by index. See above about smallbin repositioning */
#define smallbin_at(M, i)   ((sbinptr)((char*)&((M)->smallbins[(i)<<1])))
#define treebin_at(M,i)     (&((M)->treebins[i]))

/* assign tree index for size S to variable I. Use x86 asm if possible  */
#define compute_tree_index(S, I)\
{\
	UINT32 X = S >> TREEBIN_SHIFT;\
	if (X == 0)\
	I = 0;\
  else if (X > 0xFFFF)\
  I = NTREEBINS-1;\
  else {\
  UINT32 Y = (UINT32)X;\
  UINT32 N = ((Y - 0x100) >> 16) & 8;\
  UINT32 K = (((Y <<= N) - 0x1000) >> 16) & 4;\
  N += K;\
  N += K = (((Y <<= K) - 0x4000) >> 16) & 2;\
  K = 14 - N + ((Y <<= K) >> 15);\
  I = (K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1));\
}\
}

/* Bit representing maximum resolved size in a treebin at i */
#define bit_for_tree_index(i) \
	(i == NTREEBINS-1)? (SIZE_T_BITSIZE-1) : (((i) >> 1) + TREEBIN_SHIFT - 2)

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
#define leftshift_for_tree_index(i) \
	((i == NTREEBINS-1)? 0 : \
	((SIZE_T_BITSIZE-SIZE_T_ONE) - (((i) >> 1) + TREEBIN_SHIFT - 2)))

/* The size of the smallest chunk held in bin with index i */
#define minsize_for_tree_index(i) \
	((SIZE_T_ONE << (((i) >> 1) + TREEBIN_SHIFT)) |  \
	(((UINT32)((i) & SIZE_T_ONE)) << (((i) >> 1) + TREEBIN_SHIFT - 1)))

/* ------------------------ Operations on bin maps ----------------------- */

/* bit corresponding to given index */
#define idx2bit(i)              ((binmap_t)(1) << (i))

/* Mark/Clear bits with given index */
#define mark_smallmap(M,i)      ((M)->smallmap |=  idx2bit(i))
#define clear_smallmap(M,i)     ((M)->smallmap &= ~idx2bit(i))
#define smallmap_is_marked(M,i) ((M)->smallmap &   idx2bit(i))

#define mark_treemap(M,i)       ((M)->treemap  |=  idx2bit(i))
#define clear_treemap(M,i)      ((M)->treemap  &= ~idx2bit(i))
#define treemap_is_marked(M,i)  ((M)->treemap  &   idx2bit(i))

/* isolate the least set bit of a bitmap */
#define least_bit(x)         ((x) & -(x))

/* mask with all bits to left of least bit of x on */
#define left_bits(x)         ((x<<1) | -(x<<1))

/* mask with all bits to left of or equal to least bit of x on */
#define same_or_left_bits(x) ((x) | -(x))

/* index corresponding to given bit.*/
#define compute_bit2idx(X, I)\
{\
	UINT32 Y = X - 1;\
	UINT32 K = Y >> (16-4) & 16;\
	UINT32 N = K;        Y >>= K;\
	N += K = Y >> (8-3) &  8;  Y >>= K;\
	N += K = Y >> (4-2) &  4;  Y >>= K;\
	N += K = Y >> (2-1) &  2;  Y >>= K;\
	N += K = Y >> (1-0) &  1;  Y >>= K;\
	I = (bindex_t)(N + Y);\
}

/* Check if address a is at least as high as any from MORECORE or MMAP */
#define ok_address(M, a) ((char*)(a) >= (M)->least_addr) && \
                         ((char*)(a) < ((M)->least_addr + (M)->footprint))
/* Check if address of next chunk n is higher than base chunk p */
#define ok_next(p, n)    ((char*)(p) < (char*)(n))
/* Check if p has inuse status */
#define ok_inuse(p)     is_inuse(p)
/* Check if p has its pinuse bit on */
#define ok_pinuse(p)     pinuse(p)
/* Check if (alleged) ptrMstate m has expected magic field */
#define ok_magic(M)      (1)

/* In gcc, use __builtin_expect to minimize impact of checks */
#if defined(__GNUC__) && __GNUC__ >= 3
#define RTCHECK(e)  __builtin_expect(e, 1)
#else
#define RTCHECK(e)  (e)
#endif

/* Macros for setting head/foot of non-mmapped chunks */

/* Set cinuse bit and pinuse bit of next chunk */
#define set_inuse(M,p,s)\
	((p)->head = (((p)->head & PINUSE_BIT)|s|CINUSE_BIT),\
	((mchunkptr)(((char*)(p)) + (s)))->head |= PINUSE_BIT)

/* Set cinuse and pinuse of this chunk and pinuse of next chunk */
#define set_inuse_and_pinuse(M,p,s)\
	((p)->head = (s|PINUSE_BIT|CINUSE_BIT),\
	((mchunkptr)(((char*)(p)) + (s)))->head |= PINUSE_BIT)

/* Set size, cinuse and pinuse bit of this chunk */
#define set_size_and_pinuse_of_inuse_chunk(M, p, s)\
	((p)->head = (s|PINUSE_BIT|CINUSE_BIT))

/* ---------------------------- setting mparams -------------------------- */

/* Check properties of any chunk, whether free, inuse, mmapped etc  */
static UINT32
check_any_chunk(
	ptrMstate m,
	mchunkptr p
)
{
	if(!((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD))){
		return 1;
	}
	else if(!(ok_address(m, p))){
		return 1;
	}

	return SUCCESS;
}

/* Check properties of top chunk */
static UINT32
check_top_chunk(
	ptrMstate m,
	mchunkptr p
)
{
	msegmentptr sp = segment_holding(m, (char*)p);
	UINT32  sz = p->head & ~INUSE_BITS; /* third-lowest bit can be set! */
	if (!(sp != 0)){
		return 1;
	}
	else if (!((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD))){
		return 1;
	}
	else if (!(ok_address(m, p))){
		return 1;
	}
	else if (sz != m->topsize){
		return 1;
	}
	else if (sz == 0){
		return 1;
	}
	else if (sz != (((sp->base + sp->size) - (char*)p) - TOP_FOOT_SIZE)){
		return 1;
	}
	else if (!pinuse(p)){
		return 1;
	}
	else if (pinuse(chunk_plus_offset(p, sz))){
		return 1;
	}

	return SUCCESS;
}

/* Check properties of inuse chunks */
static UINT32
check_inuse_chunk(
	ptrMstate m,
	mchunkptr p
)
{
	UINT32 uErr;

	uErr = check_any_chunk(m, p);
	if(uErr != SUCCESS){
		return uErr;
	}
	else if (!(is_inuse(p))){
		return 1;
	}
	else if (!(next_pinuse(p))){
		return 1;
	}
	else if (!(pinuse(p) || next_chunk(prev_chunk(p)) == p)){
		/* If not pinuse and not mmapped, previous chunk has OK offset */
		return 1;
	}

	return SUCCESS;
}

/* Check properties of free chunks */
static UINT32
check_free_chunk(
	ptrMstate m,
	mchunkptr p
)
{
	UINT32 uErr, sz = chunksize(p);
	mchunkptr next = chunk_plus_offset(p, sz);
	uErr = check_any_chunk(m, p);
	if(uErr != SUCCESS){
		return uErr;
	}
	else if (is_inuse(p)){
		return 1;
	}
	else if(next_pinuse(p)){
		return 1;
	}
	if (p != m->dv && p != m->top){
		if (sz >= MIN_CHUNK_SIZE){
			if ((sz & CHUNK_ALIGN_MASK) != 0){
				return 1;
			}
			else if(!is_aligned(chunk2mem(p))){
				return 1;
			}
			else if(next->prev_foot != sz){
				return 1;
			}
			else if(!pinuse(p)){
				return 1;
			}
			else if(!(next == m->top || is_inuse(next))){
				return 1;
			}
			else if(p->fd->bk != p){
				return 1;
			}
			else if(p->bk->fd != p){
				return 1;
			}
		}
		else if(sz != SIZE_T_SIZE){
			/* markers are always of size SIZE_T_SIZE */
			return 1;
		}
	}

	return SUCCESS;
}

/* Check properties of malloced chunks at the point they are malloced */
static UINT32
check_malloced_chunk(
	ptrMstate m,
	void* mem,
	UINT32 s,
	UINT32 id
)
{
	UINT32 uErr;

	if (mem == 0){
		return 1;
	}
	else{
		mchunkptr p = mem2chunk(mem);
		UINT32 sz = p->head & ~INUSE_BITS;
		p->fd = (malloc_chunk_t*)id;
		p->bk = (malloc_chunk_t*)1; /* init, reference_count = 1 */
		uErr = check_inuse_chunk(m, p);
		if(uErr != SUCCESS){
			return uErr;
		}
		if((sz & CHUNK_ALIGN_MASK) != 0){
			return 1;
		}
		else if(sz < MIN_CHUNK_SIZE){
			return 1;
		}
		else if(sz < s){
			return 1;
		}
		else if(sz > (s + MIN_CHUNK_SIZE)){
			/* unless mmapped, size is less than MIN_CHUNK_SIZE more than request */
			return 1;
		}
	}

	return SUCCESS;
}

/* Check a tree and its subtrees.  */
static UINT32
check_tree(
	ptrMstate m,
	tchunkptr t
)
{
	tchunkptr head = 0;
	tchunkptr u = t;
	bindex_t tindex = t->index;
	UINT32 tsize = chunksize(t);
	bindex_t idx;
	UINT32 uErr;

	compute_tree_index(tsize, idx);
	if (tindex != idx){
		//diag_printf("compute_tree_index fail: tindex=%08X idx=%08X\n", (UINT32)tindex, (UINT32)idx);
		return 1;
	}
	else if(tsize < MIN_LARGE_SIZE){
		//diag_printf("tsize < MIN_LARGE_SIZE: tsize=%08X MIN_LARGE_SIZE=%08X\n", tsize, MIN_LARGE_SIZE);
		return 1;
	}
	else if(tsize < minsize_for_tree_index(idx)){
		//diag_printf("tsize < minsize_for_tree_index: tsize=%08X minsize_for_tree_index=%08X\n", tsize, minsize_for_tree_index(idx));
		return 1;
	}
	else if(!((idx == NTREEBINS-1) || (tsize < minsize_for_tree_index((idx+1))))){
		//diag_printf("idx & tsize fail!\n");
		return 1;
	}

	do{
		/* traverse through chain of same-sized nodes */
		uErr = check_any_chunk(m, ((mchunkptr)u));
		if(uErr != SUCCESS){
			//diag_printf("check_any_chunk fail! %08X\n", gp_chunk_pa(u));
			return uErr;
		}
		else if (u->index != tindex){
			//diag_printf("u->index != tindex: u->index=%08X tindex=%08X\n", u->index, tindex);
			return 1;
		}
		else if (chunksize(u) != tsize){
			//diag_printf("chunksize(u) != tsize: chunksize(u)=%08X tsize=%08X\n", chunksize(u), tsize);
			return 1;
		}
		else if (is_inuse(u)){
			//diag_printf("is_inuse(u) fail\n");
			return 1;
		}
		else if (next_pinuse(u)){
			//diag_printf("next_pinuse(u) fail\n");
			return 1;
		}
		else if (u->fd->bk != u){
			//diag_printf("u->fd->bk != u\n");
			return 1;
		}
		else if (u->bk->fd != u){
			//diag_printf("u->bk->fd != u\n");
			return 1;
		}
		if (u->parent == 0){
			if (u->child[0] != 0){
				//diag_printf("u->child[0] != 0\n");
				return 1;
			}
			else if (u->child[1] != 0){
				//diag_printf("u->child[1] != 0\n");
				return 1;
			}
		}
		else{
			if(head != 0){ /* only one node on chain has parent */
				//diag_printf("head != 0\n");
				return 1;
			}
			head = u;
			if (u->parent == u){
				//diag_printf("u->parent == u\n");
				return 1;
			}
			else if (!(u->parent->child[0] == u ||
				u->parent->child[1] == u ||
				*((tbinptr*)(u->parent)) == u)){
					//diag_printf("u->parent fail!!!\n");
					return 1;
			}
			if (u->child[0] != 0){
				if (u->child[0]->parent != u){
					//diag_printf("u->child[0] fail!!!\n");
					return 1;
				}
				else if (u->child[0] == u){
					//diag_printf("u->child[0] == u\n");
					return 1;
				}
				uErr = check_tree(m, u->child[0]);
				if (uErr != SUCCESS){
					//diag_printf("check_tree u->child[0] fail!\n");
					return uErr;
				}
			}
			if (u->child[1] != 0){
				if (u->child[1]->parent != u){
					//diag_printf("u->child[1]->parent != u\n");
					return 1;
				}
				else if (u->child[1] == u){
					//diag_printf("u->child[1] == u\n");
					return 1;
				}
				uErr = check_tree(m, u->child[1]);
				if (uErr != SUCCESS){
					//diag_printf("check_tree u->child[1] fail!\n");
					return uErr;
				}
			}
			if (u->child[0] != 0 && u->child[1] != 0){
				if (chunksize(u->child[0]) >= chunksize(u->child[1])){
					//diag_printf("chunksize(u->child[0]) >= chunksize(u->child[1])\n");
					return 1;
				}
			}
		}
		u = u->fd;
	} while (u != t);
	if(head == 0){
		//diag_printf("head == 0!!!\n");
		return 1;
	}

	return SUCCESS;
}

/*  Check all the chunks in a treebin.  */
static UINT32
check_treebin(
	ptrMstate m,
	bindex_t i
)
{
	tbinptr* tb = treebin_at(m, i);
	tchunkptr t = *tb;
	SINT32 uErr, empty = (m->treemap & (1U << i)) == 0;

	//diag_printf("[check_treebin] %d --> tb=%p t=%p empty=%d\n", i, tb, t, empty);
	if (t == 0){
		if(!empty){
			//diag_printf("check empty fail!\n");
			return 1;
		}
	}
	if (!empty){
		uErr = check_tree(m, t);
		if (uErr != SUCCESS){
			//diag_printf("check_tree %08X fail!\n", gp_chunk_pa(t));
			return uErr;
		}
	}

	return SUCCESS;
}

/*  Check all the chunks in a smallbin.  */
static UINT32
check_smallbin(
	ptrMstate m,
	bindex_t i
)
{
	sbinptr b = smallbin_at(m, i);
	mchunkptr p = b->bk;
	UINT32 uErr, empty = (m->smallmap & (1U << i)) == 0;
	if (p == b){
		if(!empty){
			return 1;
		}
	}
	if (!empty){
		for (; p != b; p = p->bk){
			UINT32 size = chunksize(p);
			mchunkptr q;
			/* each chunk claims to be free */
			uErr = check_free_chunk(m, p);
			if(uErr != SUCCESS){
				return uErr;
			}
			/* chunk belongs in bin */
			else if(small_index(size) != i){
				return 1;
			}
			else if(!(p->bk == b || chunksize(p->bk) == chunksize(p))){
				return 1;
			}
			/* chunk is followed by an inuse chunk */
			q = next_chunk(p);
			if (q->head != FENCEPOST_HEAD){
				uErr = check_inuse_chunk(m, q);
				if(uErr != SUCCESS){
					return uErr;
				}
			}
		}
	}

	return SUCCESS;
}

/* Find x in a bin. Used in other check functions. */
static SINT32
bin_find(
	ptrMstate m,
	mchunkptr x
)
{
	UINT32 size = chunksize(x);
	if (is_small(size)){
		bindex_t sidx = small_index(size);
		sbinptr b = smallbin_at(m, sidx);
		if (smallmap_is_marked(m, sidx)){
			mchunkptr p = b;
			do{
				if (p == x){
					return 1;
				}
			} while ((p = p->fd) != b);
		}
	}
	else{
		bindex_t tidx;
		compute_tree_index(size, tidx);
		if (treemap_is_marked(m, tidx)){
			tchunkptr t = *treebin_at(m, tidx);
			UINT32 sizebits = size << leftshift_for_tree_index(tidx);
			while (t != 0 && chunksize(t) != size){
				t = t->child[(sizebits >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1];
				sizebits <<= 1;
			}
			if (t != 0){
				tchunkptr u = t;
				do{
					if (u == (tchunkptr)x){
						return 1;
					}
				} while ((u = u->fd) != t);
			}
		}
	}
	return 0;
}

/* Traverse each chunk and check it; return total */
static UINT32
traverse_and_check(
	ptrMstate m
)
{
	UINT32 uErr, sum = 0;
	if (is_initialized(m)){
		msegmentptr s = &m->seg;
		sum += m->topsize + TOP_FOOT_SIZE;
		while (s != 0){
			mchunkptr q = align_as_chunk(s->base);
			mchunkptr lastq = 0;
			if(!pinuse(q)){
				return 1;
			}
			while (segment_holds(s, q) &&
				q != m->top && q->head != FENCEPOST_HEAD){
					sum += chunksize(q);
					if (is_inuse(q)){
						if(bin_find(m, q)){
							return 1;
						}
						uErr = check_inuse_chunk(m, q);
						if(uErr != SUCCESS)	{
							return uErr;
						}
					}
					else{
						if(!(q == m->dv || bin_find(m, q))){
							return 1;
						}
						else if(!(lastq == 0 || is_inuse(lastq))){ /* Not 2 consecutive free */
							return 1;
						}
						uErr = check_free_chunk(m, q);
						if(uErr != SUCCESS){
							return uErr;
						}
					}
					lastq = q;
					q = next_chunk(q);
			}
			s = s->next;
		}
	}

	return sum;
}

/* Check all properties of malloc_state. */
static UINT32
check_malloc_state(
	ptrMstate m
)
{
	bindex_t i;
	UINT32 total;
	UINT32 uErr;

	uErr = 1;
	/* check bins */
	for (i = 0; i < NSMALLBINS; ++i){
		uErr = check_smallbin(m, i);
		if(uErr != SUCCESS){
			//diag_printf("check_smallbin fail!\n");
			return uErr;
		}
	}
	for (i = 0; i < NTREEBINS; ++i){
		uErr = check_treebin(m, i);
		if(uErr != SUCCESS){
			//diag_printf("check_treebin fail!\n");
			return uErr;
		}
	}

	if (m->dvsize != 0){
		/* check dv chunk */
		uErr = check_any_chunk(m, m->dv);
		if(uErr != SUCCESS){
			//diag_printf("check_any_chunk fail!\n");
			return uErr;
		}
		if(m->dvsize != chunksize(m->dv)){
			//diag_printf("m->dvsize != chunksize(m->dv)!\n");
			return 1;
		}
		else if(m->dvsize < MIN_CHUNK_SIZE){
			//diag_printf("m->dvsize < MIN_CHUNK_SIZE!\n");
			return 1;
		}
		else if(bin_find(m, m->dv) > 0){
			//diag_printf("bin_find(m, m->dv) > 0!\n");
			return 1;
		}
	}

	if (m->top != 0){
		/* check top chunk */
		uErr = check_top_chunk(m, m->top);
		if(uErr != SUCCESS){
			//diag_printf("check_top_chunk fail!\n");
			return uErr;
		}
		if(m->topsize == 0){
			//diag_printf("m->topsize == 0!\n");
			return 1;
		}
		else if(bin_find(m, m->top) > 0){
			//diag_printf("bin_find(m, m->top) > 0!\n");
			return 1;
		}
	}

	total = traverse_and_check(m);
	if(total > m->footprint){
		//diag_printf("traverse_and_check fail!\n");
		return 1;
	}

	return SUCCESS;
}

/* ----------------------------- statistics ------------------------------ */

#if 0
#define PRINT_CHUNK \
	diag_printf("[chunkmem] id:%-10d addr:%08X  size:%08X\n", \
		is_inuse(q)?(SINT32)q->fd:0,        /* id   */        \
		(UINT32)gp_chunk_pa(chunk2mem(q)),  /* addr */        \
		chunksize(q))                       /* size */
#else
static void
print_chunk(
	mchunkptr q
)
{
	if (is_inuse(q)) {
		int pid = (int)q->fd;
		diag_printf("[chunkmem] addr:%08X  size:%08X  id:%d %s\n",
			gp_chunk_pa(chunk2mem(q)), chunksize(q), pid, find_task_by_vpid(pid)->comm);
	}
	else {
		diag_printf("[chunkmem] addr:%08X  size:%08X  ---\n",
			gp_chunk_pa(chunk2mem(q)), chunksize(q));
	}
}

#define PRINT_CHUNK if (info == NULL || info->total_bytes == 0xDEADC0DE) print_chunk(q)
#endif

static void
internal_malloc_stats(
	ptrMstate m,
	mem_info_t *info
)
{
	/*UINT32 maxfp = 0;*/
	UINT32 fp = 0;
	UINT32 used = 0;
	UINT32 max_Csize = 0;
	UINT32 Csize = 0;
	UINT32 id = 0;

	if (info != NULL) {
		id = info->id;
	}

	check_malloc_state(m);
	if (is_initialized(m)){
		msegmentptr s = &m->seg;
		fp = m->footprint;
        //diag_printf("[internal_malloc_stats] %08X %08X %08X\n", fp, m->topsize, TOP_FOOT_SIZE);
		used = fp - m->topsize;
        if (id != 0) {
			used -= TOP_FOOT_SIZE + chunksize(mem2chunk(m));
		}

		while (s != 0){
			mchunkptr q = align_as_chunk(s->base);
			while (segment_holds(s, q) &&
				   q != m->top && q->head != FENCEPOST_HEAD){
					if (is_inuse(q)){
						if (Csize > max_Csize){
							max_Csize = Csize;
						}
						Csize = 0;
						if ((id != 0) && (id != (UINT32)q->fd)) {
							used -= chunksize(q);
						}
						else {
							PRINT_CHUNK;
						}
					}
					else{
						used -= chunksize(q);
						Csize += chunksize(q);
						if (id == 0) {
							PRINT_CHUNK;
						}
					}
					q = next_chunk(q);
			}
			s = s->next;
			if (!is_inuse(q)){
				Csize += chunksize(q);
				if (id == 0) {
					PRINT_CHUNK;
				}
			}
			else if ((id == 0) || (id == (UINT32)q->fd)) {
				PRINT_CHUNK;
			}
			if (Csize > max_Csize){
				max_Csize = Csize;
			}
		}
	}

	if (info != NULL) {
		info->total_bytes = fp;
		info->used_bytes = used;
		info->max_free_block_bytes = max_Csize;
	}
	else {
		diag_printf("system bytes = %08X (%u)\n", fp, fp);
		diag_printf("in use bytes = %08X (%u)\n", used, used);
		diag_printf("max chunk    = %08X (%u)\n\n", max_Csize, max_Csize);
	}
}

int gp_chunk_suspend(save_data_proc save_proc)
{
	int ret = check_malloc_state(pgdl_GM);

	//diag_printf("check_malloc_state(pgdl_GM) = %d\n", ret);
	//diag_printf("is_initialized(pgdl_GM) = %08X\n", is_initialized(pgdl_GM));

	if ((save_proc != NULL) &&
		(ret == SUCCESS) &&
		is_initialized(pgdl_GM)) {
		msegmentptr s = &pgdl_GM->seg;
		void *addr = NULL;

		save_proc(__pa(&pgdl_GM), sizeof(pgdl_GM));
		save_proc(gp_chunk_pa(pgdl_GM->least_addr + pgdl_GM->footprint - TOP_FOOT_SIZE + 4), 4);

		while (s != 0){
			mchunkptr q = align_as_chunk(s->base);
			while (segment_holds(s, q) &&
				   (q != pgdl_GM->top) &&
				   (q->head != FENCEPOST_HEAD)){
				if (is_inuse(q) || chunksize(q) == pgdl_GM->page_size) {
					if (addr == NULL) {
						addr = chunk2mem(q) - pgdl_GM->page_size;
					}
				}
				else {
					save_proc(gp_chunk_pa(addr), chunk2mem(q) - addr + 0x10);
					addr = NULL;
				}
				q = next_chunk(q);
			}
			save_proc(gp_chunk_pa(addr), chunk2mem(q) - addr + 0x10);
			addr = NULL;
			s = s->next;
		}
	}

	return ret;
}
EXPORT_SYMBOL(gp_chunk_suspend);

/* ----------------------- Operations on smallbins ----------------------- */

/* Link a free chunk into a smallbin  */
static UINT32 insert_small_chunk(
	ptrMstate ptrM,
	mchunkptr pChunk,
	UINT32    isSize
)
{
	bindex_t I  = small_index(isSize);
	mchunkptr B = smallbin_at(ptrM, I);
	mchunkptr F = B;

	if(isSize < MIN_CHUNK_SIZE){
		return 1;
	}
	if (!smallmap_is_marked(ptrM, I)){
		mark_smallmap(ptrM, I);
	}
	else if (RTCHECK(ok_address(ptrM, B->fd))){
		F = B->fd;
	}
	else {
	return 1;
	}
	B->fd = pChunk;
	F->bk = pChunk;
	pChunk->fd = F;
	pChunk->bk = B;

	return 0;
}
/* Unlink a chunk from a smallbin  */
static UINT32 unlink_small_chunk(
	ptrMstate ptrM,
	mchunkptr pChunk,
	UINT32    isSize
)
{
	mchunkptr F = pChunk->fd;
	mchunkptr B = pChunk->bk;
	bindex_t I  = small_index(isSize);

	if((pChunk == B) || (pChunk == F) || (chunksize(pChunk) != small_index2size(I))){
		return 1;
	}

	if (F == B){
		clear_smallmap(ptrM, I);
	}
	else if (RTCHECK(((F == smallbin_at(ptrM,I)) || (ok_address(ptrM, F))) &&
					 ((B == smallbin_at(ptrM,I)) || (ok_address(ptrM, B))))) {
		F->bk = B;
		B->fd = F;
	}
	else {
		return 1;
	}

	return 0;
}

/* Unlink the first chunk from a smallbin */
static UINT32 unlink_first_small_chunk(
									ptrMstate M,
									mchunkptr B,
									mchunkptr P,
									bindex_t  I
									)
{
	mchunkptr F = P->fd;

	if((P == B) || (P == F) || (chunksize(P) != small_index2size(I))){
		return 1;
	}

	if (B == F){
		clear_smallmap(M, I);
	}
	else if (RTCHECK(ok_address(M, F))) {
		B->fd = F;
		F->bk = B;
	}
	else {
		return 1;
	}

	return 0;
}


/* Replace dv node, binning the old one */
/* Used only when dvsize known to be small */
static UINT32 replace_dv(
	ptrMstate M,
	mchunkptr P,
	UINT32    S
)
{
	UINT32 uErr, DVS = M->dvsize;

	if (DVS != 0) {
		mchunkptr DV = M->dv;
		if(is_small(DVS)){
			uErr = insert_small_chunk(M, DV, DVS);
			if(uErr != SUCCESS){
				return uErr;
			}
		}
		else{
			return 1;
		}
	}
	M->dvsize = S;
	M->dv = P;

	return 0;
}
/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
static UINT32 insert_large_chunk(
	ptrMstate M,
	tchunkptr X,
	UINT32    S
)
{
	tbinptr* H;
	bindex_t I;
	compute_tree_index(S, I);
	H = treebin_at(M, I);
	X->index = I;
	X->child[0] = X->child[1] = 0;
	if (!treemap_is_marked(M, I)) {
		mark_treemap(M, I);
		*H = X;
		X->parent = (tchunkptr)H;
		X->fd = X->bk = X;
	}
	else {
		tchunkptr T = *H;
		UINT32 K = S << leftshift_for_tree_index(I);
		for (;;) {
			if (chunksize(T) != S) {
				tchunkptr* C = &(T->child[(K >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1]);
				K <<= 1;
				if (*C != 0){
					T = *C;
				}
				else if (RTCHECK(ok_address(M, C))) {
					*C = X;
					X->parent = T;
					X->fd = X->bk = X;
					break;
				}
				else {
					return 1;
				}
			}
			else {
				tchunkptr F = T->fd;
				if (RTCHECK(ok_address(M, T) && ok_address(M, F))) {
					T->fd = F->bk = X;
					X->fd = F;
					X->bk = T;
					X->parent = 0;
					break;
				}
				else {
					return 1;
				}
			}
		}
	}

	return 0;
}

static UINT32 unlink_large_chunk(
	ptrMstate M,
	tchunkptr X
)
{
	tchunkptr XP = X->parent;
	tchunkptr R;
	if (X->bk != X){
		tchunkptr F = X->fd;
		R = X->bk;
		if (RTCHECK(ok_address(M, F))){
			F->bk = R;
			R->fd = F;
		}
		else{
			return 1;
		}
	}
	else{
		tchunkptr* RP;
		if (((R = *(RP = &(X->child[1]))) != 0) ||
			((R = *(RP = &(X->child[0]))) != 0)){
			tchunkptr* CP;
			while ((*(CP = &(R->child[1])) != 0) ||
				(*(CP = &(R->child[0])) != 0)){
				R = *(RP = CP);
			}
			if (RTCHECK(ok_address(M, RP))){
				*RP = 0;
			}
			else {
				return 1;
			}
		}
	}
	if (XP != 0){
		tbinptr* H = treebin_at(M, X->index);
		if (X == *H){
			if ((*H = R) == 0){
				clear_treemap(M, X->index);
			}
		}
		else if (RTCHECK(ok_address(M, XP))){
			if (XP->child[0] == X){
				XP->child[0] = R;
			}
			else{
			XP->child[1] = R;
			}
		}
		else{
			return 1;
		}
		if (R != 0){
			if (RTCHECK(ok_address(M, R))){
				tchunkptr C0, C1;
				R->parent = XP;
				if ((C0 = X->child[0]) != 0){
					if (RTCHECK(ok_address(M, C0))){
						R->child[0] = C0;
						C0->parent = R;
					}
					else{
						return 1;
					}
				}
				if ((C1 = X->child[1]) != 0){
					if (RTCHECK(ok_address(M, C1))){
						R->child[1] = C1;
						C1->parent = R;
					}
					else{
						return 1;
					}
				}
			}
			else
			{
				return 1;
			}
		}
	}
	return 0;
}
/* Relays to large vs small bin operations */
static UINT32 insert_chunk(
	  ptrMstate M,
	  mchunkptr P,
	  UINT32    S
)
{
	UINT32 uErr;

	if (is_small(S)){
		uErr = insert_small_chunk(M, P, S);
		if(uErr != SUCCESS){
			return uErr;
		}
	}
	else{
		tchunkptr TP = (tchunkptr)(P);
		uErr = insert_large_chunk(M, TP, S);
		if(uErr != SUCCESS){
			return uErr;
		}
	}

	return 0;
}

static UINT32 unlink_chunk(
	ptrMstate M,
	mchunkptr P,
	UINT32    S
)
{
	UINT32 uErr;

	if (is_small(S)){
		uErr = unlink_small_chunk(M, P, S);
		if(uErr != SUCCESS){
			return uErr;
		}
	}
	else{
		tchunkptr TP = (tchunkptr)(P);
		uErr = unlink_large_chunk(M, TP);
		if(uErr != SUCCESS){
			return uErr;
		}
	}
	return 0;
}

/* Initialize top chunk and its size */
static void
init_top(
	ptrMstate m,
	mchunkptr p,
	UINT32 psize
)
{
	/* Ensure alignment */
	UINT32 offset = align_offset(chunk2mem(p));
	p = (mchunkptr)((char*)p + offset);
	psize -= offset;

	m->top = p;
	m->topsize = psize;
	p->head = psize | PINUSE_BIT;
	/* set size of fake trailing chunk holding overhead space only once */
	chunk_plus_offset(p, psize)->head = TOP_FOOT_SIZE;
}

/* Initialize bins for a new ptrMstate that is otherwise zeroed out */
static void
init_bins(
	ptrMstate m
)
{
	/* Establish circular links for smallbins */
	bindex_t i;
	for (i = 0; i < NSMALLBINS; ++i){
		sbinptr bin = smallbin_at(m,i);
		bin->fd = bin->bk = bin;
	}
}

static SINT32
init_user_mstate(
	char* tbase,
	UINT32 tsize,
	UINT32 page_size
)
{
	UINT32 msize, uErr;
	UINT32 magic;
	mchunkptr mn;
	mchunkptr msp;

	msize = PAD_REQUEST(sizeof(malloc_state_t)) - sizeof(malloc_chunk_t);
	//next memory malloc must page_start
	msp = (mchunkptr)tbase;//align_as_chunk(tbase);
	pgdl_GM = (ptrMstate)(chunk2mem(msp));
	memset(pgdl_GM, 0, sizeof(malloc_state_t));
	msp->head = (msize|INUSE_BITS);
	pgdl_GM->seg.base = pgdl_GM->least_addr = tbase;
	pgdl_GM->seg.size = pgdl_GM->footprint = tsize;
	pgdl_GM->extp = 0;
	pgdl_GM->exts = 0;
	pgdl_GM->granularity = page_size;
	pgdl_GM->page_size = page_size;

	if ((sizeof(UINT32) != sizeof(char*))                         ||
		(MAX_SIZE_T < MIN_CHUNK_SIZE)                             ||
		(sizeof(SINT32) < 4)                                      ||
		(MALLOC_ALIGNMENT < (UINT32)8U)                           ||
		((MALLOC_ALIGNMENT & (MALLOC_ALIGNMENT-SIZE_T_ONE)) != 0) ||
		((MCHUNK_SIZE      & (MCHUNK_SIZE-SIZE_T_ONE))      != 0) ||
		((page_size        & (page_size-SIZE_T_ONE))        != 0)){
		return 1;
	}
	magic = (UINT32)(time(0) ^ (UINT32)0x55555555U);
	magic |= (UINT32)8U;    /* ensure nonzero */
	magic &= ~(UINT32)7U;   /* improve chances of fault for bad values */
	pgdl_GM->magic = magic;

	init_bins(pgdl_GM);
	mn = next_chunk(mem2chunk(pgdl_GM));
	init_top(pgdl_GM, mn, (UINT32)((tbase + tsize) - (char*)mn) - TOP_FOOT_SIZE);
	uErr = check_top_chunk(pgdl_GM, pgdl_GM->top);
	if(uErr != SUCCESS){
		return uErr;
	}
#if 0
	diag_printf("tbase   = %p\n", tbase);
	diag_printf("msp     = %p\n", msp);
	diag_printf("pgdl_GM = %p\n", pgdl_GM);
	diag_printf("mn      = %p\n", mn);
	diag_printf("top     = %p\n", pgdl_GM->top);
#endif
	return SUCCESS;
}

/* ---------------------------- malloc support --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
static void*
tmalloc_large(
	ptrMstate m,
	UINT32 nb
)
{
	tchunkptr v = 0;
	UINT32 uErr, rsize = -nb; /* Unsigned negation */
	tchunkptr t;
	bindex_t idx;
	compute_tree_index(nb, idx);
	if ((t = *treebin_at(m, idx)) != 0){
		/* Traverse tree for this bin looking for node with size == nb */
		UINT32 sizebits = nb << leftshift_for_tree_index(idx);
		tchunkptr rst = 0;  /* The deepest untaken right subtree */
		for (;;){
			tchunkptr rt;
			UINT32 trem = chunksize(t) - nb;
			if (trem < rsize){
				v = t;
				if ((rsize = trem) == 0){
					break;
				}
			}
			rt = t->child[1];
			t = t->child[(sizebits >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1];
			if (rt != 0 && rt != t){
				rst = rt;
			}
			if (t == 0){
				t = rst; /* set t to least subtree holding sizes > nb */
				break;
			}
			sizebits <<= 1;
		}
	}
	if (t == 0 && v == 0){
		/* set t to root of next non-empty treebin */
		binmap_t leftbits = left_bits(idx2bit(idx)) & m->treemap;
		if (leftbits != 0){
			bindex_t i;
			binmap_t leastbit = least_bit(leftbits);
			compute_bit2idx(leastbit, i);
			t = *treebin_at(m, i);
		}
	}

	while (t != 0){
		/* find smallest of tree or subtree */
		UINT32 trem = chunksize(t) - nb;
		if (trem < rsize){
			rsize = trem;
			v = t;
		}
		t = leftmost_child(t);
	}

	/*  If dv is a better fit, return 0 so malloc will use it */
	if (v != 0 && rsize < (UINT32)(m->dvsize - nb)){
		if (RTCHECK(ok_address(m, v))){
			/* split */
			mchunkptr r = chunk_plus_offset(v, nb);
			if(chunksize(v) != rsize + nb){
				return NULL;
			}
			if (RTCHECK(ok_next(v, r))){
				uErr = unlink_large_chunk(m, v);
				if(uErr != SUCCESS){
					return NULL;
				}
				if (rsize < MIN_CHUNK_SIZE){
					set_inuse_and_pinuse(m, v, (rsize + nb));
				}
				else{
					set_size_and_pinuse_of_inuse_chunk(m, v, nb);
					set_size_and_pinuse_of_free_chunk(r, rsize);
					uErr = insert_chunk(m, r, rsize);
					if(uErr != SUCCESS){
						return NULL;
					}
				}
				return chunk2mem(v);
			}
		}
	}

	return NULL;
}

/* allocate a small request from the best fitting chunk in a treebin */
static void*
tmalloc_small(
	ptrMstate m,
	UINT32 nb
)
{
	tchunkptr t, v;
	UINT32 uErr, rsize;
	bindex_t i;
	binmap_t leastbit = least_bit(m->treemap);
	compute_bit2idx(leastbit, i);
	v = t = *treebin_at(m, i);
	rsize = chunksize(t) - nb;

	while ((t = leftmost_child(t)) != 0){
		UINT32 trem = chunksize(t) - nb;
		if (trem < rsize){
			rsize = trem;
			v = t;
		}
	}

	if (RTCHECK(ok_address(m, v))){
		mchunkptr r = chunk_plus_offset(v, nb);
		if(chunksize(v) != rsize + nb){
			return NULL;
		}
		if (RTCHECK(ok_next(v, r))){
			uErr = unlink_large_chunk(m, v);
			if(uErr != SUCCESS){
				return NULL;
			}
			if (rsize < MIN_CHUNK_SIZE){
				set_inuse_and_pinuse(m, v, (rsize + nb));
			}
			else{
				set_size_and_pinuse_of_inuse_chunk(m, v, nb);
				set_size_and_pinuse_of_free_chunk(r, rsize);
				uErr = replace_dv(m, r, rsize);
				if(uErr != SUCCESS){
					return NULL;
				}
			}
			return chunk2mem(v);
		}
	}

	return NULL;
}

/* --------------------------- realloc support --------------------------- */

static void*
internal_realloc(
	ptrMstate m,
	void* oldmem,
	UINT32 bytes,
	UINT32 id
)
{
	mchunkptr oldp = mem2chunk(oldmem);
	UINT32 uErr, oldsize = chunksize(oldp);
	mchunkptr next = chunk_plus_offset(oldp, oldsize);
	mchunkptr newp = 0;
	void* extra = 0;

	if (bytes >= MAX_REQUEST){
		return 0;
	}

	/* Try to either shrink or extend into top. Else malloc-copy-free */
	if (RTCHECK(ok_address(m, oldp) && ok_inuse(oldp) &&
		ok_next(oldp, next) && ok_pinuse(next))){
			UINT32 nb = REQUEST2SIZE(bytes);
			if (oldsize >= nb){
				/* already big enough */
				UINT32 rsize = oldsize - nb;
				newp = oldp;
				if (rsize >= MIN_CHUNK_SIZE){
					mchunkptr remainder = chunk_plus_offset(newp, nb);
					set_inuse(m, newp, nb);
					set_inuse_and_pinuse(m, remainder, rsize);
					extra = chunk2mem(remainder);
				}
			}
			else if (next == m->top && oldsize + m->topsize > nb){
				/* Expand into top */
				UINT32 newsize = oldsize + m->topsize;
				UINT32 newtopsize = newsize - nb;
				mchunkptr newtop = chunk_plus_offset(oldp, nb);
				set_inuse(m, oldp, nb);
				newtop->head = newtopsize |PINUSE_BIT;
				m->top = newtop;
				m->topsize = newtopsize;
				newp = oldp;
			}
	}
	else{
		return 0;
	}

	if (newp != 0){
		uErr = check_inuse_chunk(m, newp); /* Check requires lock */
		if(uErr != SUCCESS){
			return NULL;
		}
	}

	if (newp != 0){
		dlFree(extra);
		return chunk2mem(newp);
	}
	else{
		void* newmem = dlMalloc(id, bytes);
		if (newmem != 0){
			UINT32 oc = oldsize - overhead_for(oldp);
			memcpy(newmem, oldmem, (oc < bytes)? oc : bytes);
			dlFree(oldmem);
		}
		return newmem;
	}

	return NULL;
}

SINT32
dlMallocInit(
	void* tbase,
	UINT32 tsize,
	UINT32 page_size
)
{
	return init_user_mstate((char *)tbase, tsize, page_size);
}

void*
dlMalloc(
	UINT32 id,
	UINT32 bytes
)
{
	void* mem;
	UINT32 nb, uErr;

	mem = NULL;
	if ((bytes & 0x80000000) != 0){
		return mem;
	}
	//for Linux page control
	bytes = (bytes + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK;
	if (bytes >= MAX_REQUEST){
		nb = MAX_SIZE_T; /* Too big to allocate. Force failure (in sys alloc) */
	}
	//else if (bytes <= MAX_SMALL_REQUEST){
	else if (is_small(PAD_REQUEST(bytes))){
		bindex_t idx;
		binmap_t smallbits;
		nb = (bytes < MIN_REQUEST)? MIN_CHUNK_SIZE : PAD_REQUEST(bytes);
		idx = small_index(nb);
		smallbits = pgdl_GM->smallmap >> idx;

		if ((smallbits & 0x3U) != 0){
			/* Remainderless fit to a smallbin. */
			mchunkptr b, p;
			idx += ~smallbits & 1;       /* Uses next bin if idx empty */
			b = smallbin_at(pgdl_GM, idx);
			p = b->fd;
			if(chunksize(p) != small_index2size(idx)){
				return mem;
			}
			if(unlink_first_small_chunk(pgdl_GM, b, p, idx)){
				return mem;
			}
			set_inuse_and_pinuse(pgdl_GM, p, small_index2size(idx));
			mem = chunk2mem(p);
			check_malloced_chunk(pgdl_GM, mem, nb, id);
			return mem;
		}
		else if (nb > pgdl_GM->dvsize){
			if (smallbits != 0){
				/* Use chunk in next nonempty smallbin */
				mchunkptr b, p, r;
				UINT32 rsize;
				bindex_t i;
				binmap_t leftbits = (smallbits << idx) & left_bits(idx2bit(idx));
				binmap_t leastbit = least_bit(leftbits);
				compute_bit2idx(leastbit, i);
				b = smallbin_at(pgdl_GM, i);
				p = b->fd;
				if(chunksize(p) != small_index2size(i)){
					return mem;
				}
				if(unlink_first_small_chunk(pgdl_GM, b, p, i)){
					return mem;
				}
				rsize = small_index2size(i) - nb;
				/* Fit here cannot be remainderless if 4byte sizes */
				if (SIZE_T_SIZE != 4 && rsize < MIN_CHUNK_SIZE){
					set_inuse_and_pinuse(pgdl_GM, p, small_index2size(i));
				}
				else{
					set_size_and_pinuse_of_inuse_chunk(pgdl_GM, p, nb);
					r = chunk_plus_offset(p, nb);
					set_size_and_pinuse_of_free_chunk(r, rsize);
					if(replace_dv(pgdl_GM, r, rsize)){
						return mem;
					}
				}
				mem = chunk2mem(p);
				check_malloced_chunk(pgdl_GM, mem, nb, id);
				return mem;
			}
			else if (pgdl_GM->treemap != 0 && (mem = tmalloc_small(pgdl_GM, nb)) != 0){
				check_malloced_chunk(pgdl_GM, mem, nb, id);
				return mem;
			}
		}
	}
	else{
		nb = PAD_REQUEST(bytes);
		if (pgdl_GM->treemap != 0 && (mem = tmalloc_large(pgdl_GM, nb)) != 0){
			check_malloced_chunk(pgdl_GM, mem, nb, id);
			return mem;
		}
	}

	if (nb <= pgdl_GM->dvsize){
		UINT32 rsize = pgdl_GM->dvsize - nb;
		mchunkptr p = pgdl_GM->dv;
		if (rsize >= MIN_CHUNK_SIZE){
			/* split dv */
			mchunkptr r = pgdl_GM->dv = chunk_plus_offset(p, nb);
			pgdl_GM->dvsize = rsize;
			set_size_and_pinuse_of_free_chunk(r, rsize);
			set_size_and_pinuse_of_inuse_chunk(pgdl_GM, p, nb);
		}
		else{
			/* exhaust dv */
			UINT32 dvs = pgdl_GM->dvsize;
			pgdl_GM->dvsize = 0;
			pgdl_GM->dv = 0;
			set_inuse_and_pinuse(pgdl_GM, p, dvs);
		}
		mem = chunk2mem(p);
		check_malloced_chunk(pgdl_GM, mem, nb, id);
		return mem;
	}
	else if (nb < pgdl_GM->topsize){
		/* Split top */
		UINT32 rsize = pgdl_GM->topsize -= nb;
		mchunkptr p = pgdl_GM->top;
		mchunkptr r = pgdl_GM->top = chunk_plus_offset(p, nb);
		r->head = rsize | PINUSE_BIT;
		set_size_and_pinuse_of_inuse_chunk(pgdl_GM, p, nb);
		mem = chunk2mem(p);
		uErr = check_top_chunk(pgdl_GM, pgdl_GM->top);
		if (uErr != SUCCESS){
			return NULL;
		}
		check_malloced_chunk(pgdl_GM, mem, nb, id);
		return mem;
	}
	return mem;
}

#define MAX_SL_CHUNK_SIZE   (0x00500000)
#define MAX_SL_BUFFER_SIZE  (0x001FE000)
#define MAX_SL_BUF_NUM      (32)

void*
dlMallocEx(
	UINT32 id,
	UINT32 bytes
)
{
	void *pMem;

	pMem = NULL;
	if (bytes == 0) {
		return pMem;
	}

	pMem = dlMalloc(id, bytes);

	if ((ok_address(pgdl_GM, pMem)) && (bytes >= MAX_SL_BUFFER_SIZE)) {
		mchunkptr pChunk     = mem2chunk(pMem);
		mchunkptr pNextChunk = next_chunk(pChunk);
		UINT32    ChunkSize  = chunksize(pChunk);
		UINT32    NextChunkSize = chunksize(pNextChunk) & (~CHUNK_ALIGN_MASK);

		if ((ChunkSize + NextChunkSize) >= MAX_SL_CHUNK_SIZE) {
			if (ok_pinuse(pNextChunk)) {
				void * pTMem;
#if 0
				dlFree(pMem);
				pMem = NULL;
				pTMem = dlMalloc(id, NextChunkSize - MALLOC_ALIGNMENT);
				if (ok_address(pgdl_GM, pTMem)) {
					pMem = dlMalloc(id, bytes);
					if (pMem == NULL) {
						dlMalloc_Status(NULL);
					}
					dlFree(pTMem);
				}
#else
				UINT32 i;
				UINT32 MemAddr[MAX_SL_BUF_NUM];

				dlFree(pMem);
				memset(MemAddr, 0, sizeof(MemAddr));
				i = 0;
				do {
					pTMem = dlMalloc(id, NextChunkSize - MALLOC_ALIGNMENT);
					MemAddr[i++] = (UINT32)pTMem;
					if (i == MAX_SL_BUF_NUM) {
						break;
					}
				} while (pTMem != pMem);
				pMem = NULL;
				if (ok_address(pgdl_GM, pTMem)) {
					pMem = dlMalloc(id, bytes);
				}
				for (i = 0; i < MAX_SL_BUF_NUM; i++) {
					if (MemAddr[i] == 0) {
						break;
					} else {
						dlFree((void *)MemAddr[i]);
					}
				}
#endif
			}
		}
	}

	return pMem;
}

SINT32
dlFree(
	void* addr
)
{
	UINT32 uErr;

	if (ok_address(pgdl_GM, addr)){
		UINT32 refCount;
		mchunkptr pChunk = mem2chunk(addr);
		uErr = check_inuse_chunk(pgdl_GM, pChunk);
		if(uErr != SUCCESS){
			return uErr;
		}

		/* reference_count-- */
		refCount = (UINT32)pChunk->bk - 1;
		pChunk->bk = (malloc_chunk_t *)refCount;
		if (refCount != 0) {
			return SUCCESS; /* still shared, not do real free */
		}

		if (ok_inuse(pChunk)){
			UINT32 psize = chunksize(pChunk);
			mchunkptr next = chunk_plus_offset(pChunk, psize);
			if (!pinuse(pChunk)){
				UINT32 prevsize = pChunk->prev_foot;
				mchunkptr prev = chunk_minus_offset(pChunk, prevsize);
				psize += prevsize;
				pChunk = prev;
				if (RTCHECK(ok_address(pgdl_GM, prev))){
					/* consolidate backward */
					if (pChunk != pgdl_GM->dv){
						uErr = unlink_chunk(pgdl_GM, pChunk, prevsize);
						if(uErr != SUCCESS){
							return uErr;
						}
					}
					else if ((next->head & INUSE_BITS) == INUSE_BITS){
						pgdl_GM->dvsize = psize;
						set_free_with_pinuse(pChunk, psize, next);
						return 0;
					}
				}
				else{
					return 1;
				}
			}

			if (RTCHECK(ok_next(pChunk, next) && ok_pinuse(next))){
				if (!cinuse(next)){
					/* consolidate forward */
					if (next == pgdl_GM->top){
						UINT32 tsize = pgdl_GM->topsize += psize;
						pgdl_GM->top = pChunk;
						pChunk->head = tsize | PINUSE_BIT;
						if (pChunk == pgdl_GM->dv){
							pgdl_GM->dv = 0;
							pgdl_GM->dvsize = 0;
						}
						return 0;
					}
					else if (next == pgdl_GM->dv){
						UINT32 dsize = pgdl_GM->dvsize += psize;
						pgdl_GM->dv = pChunk;
						set_size_and_pinuse_of_free_chunk(pChunk, dsize);
						return 0;
					}
					else{
						UINT32 nsize = chunksize(next);
						psize += nsize;
						uErr = unlink_chunk(pgdl_GM, next, nsize);
						if(uErr != SUCCESS){
							return uErr;
						}
						set_size_and_pinuse_of_free_chunk(pChunk, psize);
						if (pChunk == pgdl_GM->dv){
							pgdl_GM->dvsize = psize;
							return 0;
						}
					}
				}
				else{
					set_free_with_pinuse(pChunk, psize, next);
				}

				if (is_small(psize)){
					uErr = insert_small_chunk(pgdl_GM, pChunk, psize);
					if(uErr != SUCCESS){
						return uErr;
					}
					uErr = check_free_chunk(pgdl_GM, pChunk);
					if(uErr != SUCCESS){
						return uErr;
					}
				}
				else{
					tchunkptr tp = (tchunkptr)pChunk;
					uErr = insert_large_chunk(pgdl_GM, tp, psize);
					if(uErr != SUCCESS){
						return uErr;
					}
					uErr = check_free_chunk(pgdl_GM, pChunk);
					if(uErr != SUCCESS){
						return uErr;
					}
				}
				return 0;
			}
		}
	}

	return 1;
}

static SINT32
internal_free_all(
	ptrMstate m,
	UINT32 OpenID
)
{
	SINT32 iFreeNum;
	UINT32 uErr;

	iFreeNum = -1;
	uErr = check_malloc_state(m);
	if(uErr != SUCCESS){
		return iFreeNum;
	}
	if ((is_initialized(m)) && (OpenID > 0)){
		void *mem;
		mchunkptr pChunk;
		msegmentptr s = &m->seg;
		iFreeNum = 0;
		while (s != 0){
			pChunk = align_as_chunk(s->base);
			while (segment_holds(s, pChunk) &&
				pChunk != m->top && pChunk->head != FENCEPOST_HEAD){
				if (is_inuse(pChunk) && (pChunk->fd == (malloc_chunk_t *)OpenID)){
					mem = chunk2mem(pChunk);
					if(dlFree(mem)){
						diag_printf("free error, address:0x%08X ID:%d\n", (UINT32)mem, OpenID);
					}
					else{
						iFreeNum++;
					}
				}
				pChunk = next_chunk(pChunk);
			}
			s = s->next;
		}
	}
	return iFreeNum;
}

SINT32
dlFreeAll(
	UINT32 id
)
{
	return internal_free_all(pgdl_GM, id);
}

void*
dlCalloc(
	UINT32 id,
	UINT32 n_elements,
	UINT32 elem_size
)
{
	void* mem;
	UINT32 req = 0;
	if (n_elements != 0){
		req = n_elements * elem_size;
		if (((n_elements | elem_size) & ~(UINT32)0xffff) &&
			(req / n_elements != elem_size)){
			req = MAX_SIZE_T; /* force downstream failure on overflow */
		}
	}
	mem = dlMalloc(id, req);

	return mem;
}

void*
dlRealloc(
	UINT32 id,
	void* oldmem,
	UINT32 bytes
)
{
	if (oldmem == 0){
		return dlMalloc(id, bytes);
	}
	else{
		return internal_realloc(pgdl_GM, oldmem, bytes, id);
	}
}

#if 0
UINT32 dlMalloc_ID_Get(void* addr)
{
	if (ok_address(pgdl_GM, addr)){
		mchunkptr pChunk = mem2chunk(addr);
		UINT32 uErr, ID;
		uErr = check_inuse_chunk(pgdl_GM, pChunk);
		if(uErr == SUCCESS){
			ID = (UINT32)pChunk->fd;
			return ID;
		}
	}
	return 0;
}

UINT32 dlMalloc_Counter_Get(void* addr)
{
	if (ok_address(pgdl_GM, addr)){
		mchunkptr pChunk = mem2chunk(addr);
		UINT32 uErr, MCounter;
		uErr = check_inuse_chunk(pgdl_GM, pChunk);
		if(uErr == SUCCESS){
			MCounter = (UINT32)pChunk->bk;
			return MCounter;
		}
	}
	return 0;
}
#endif

UINT32 dlShare(void* addr)
{
	if (ok_address(pgdl_GM, addr)){
		mchunkptr pChunk = mem2chunk(addr);
		UINT32 uErr;
		uErr = check_inuse_chunk(pgdl_GM, pChunk);
		if(uErr == SUCCESS){
			UINT32 refCount;

			/* reference_count++ */
			refCount = (UINT32)pChunk->bk + 1;
			pChunk->bk = (malloc_chunk_t*)refCount;

			return refCount;
		}
	}
	return 0;
}

void
dlMalloc_Status(
	mem_info_t *info
)
{
	internal_malloc_stats(pgdl_GM, info);
}

UINT32
dlMalloc_Usable_Size(
	void* mem
)
{
	if (mem != 0){
		mchunkptr pChunk = mem2chunk(mem);
		if (is_inuse(pChunk)){
			return chunksize(pChunk) - overhead_for(pChunk);
		}
	}

	return 0;
}

