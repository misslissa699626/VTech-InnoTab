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
 * @file    gp_chunkmem.c
 * @brief   Implement of GP chunk memory driver.
 * @author  qinjian
 * @since   2010-09-01
 * @date    2010-09-01
 */
/*#define DIAG_LEVEL DIAG_LVL_VERB*/
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/hardware.h>
#include <mach/dlmalloc.h>
#include <mach/gp_chunkmem.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define MAX_OPEN_PROCESS 50

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define USE_DLMALLOC_EX 1
#define CHUNK_SUSPEND_TEST  0

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct open_process_s {
	unsigned int pid;           /*!< @brief process id */
	unsigned int count;         /*!< @brief open count in this process */
} open_process_t;

typedef struct chunkmem_info_s {
	struct miscdevice dev;      /*!< @brief chunkmem device */

	unsigned int pbase;         /*!< @brief start address of the chunkmem region, phy_addr */
	unsigned int size;          /*!< @brief size of the chunkmem region */
	void __iomem *vbase;        /*!< @brief start address of the remaped chunkmem region, kernel_addr */
	struct resource *mem;       /*!< @brief memory resource to hold chunkmem region */
	unsigned int mmap_enable;   /*!< @brief mmap flag, used to disable calling mmap from user AP */
	struct semaphore sem;       /*!< @brief mutex semaphore for mem ops */

	open_process_t opens[MAX_OPEN_PROCESS]; /*!< @brief all process open the device */
	unsigned int opens_count;   /*!< @brief open process count */
	struct semaphore opens_sem; /*!< @brief mutex semaphore for opens */
} chunkmem_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static chunkmem_info_t *chunkmem = NULL;

static unsigned int membase = CHUNKMEM_BASE;
static unsigned int memsize = CHUNKMEM_SIZE;
module_param(membase, uint, S_IRUGO);
module_param(memsize, uint, S_IRUGO);

/**
 * @brief   Sync cache
 * @return  None
 * @see
 */
void gp_sync_cache(void)
{
	unsigned long oldIrq;

	local_irq_save(oldIrq);
	flush_cache_all();
	flush_tlb_all();
	local_irq_restore(oldIrq);
}
EXPORT_SYMBOL(gp_sync_cache);

/**
 * @brief   Allocate memory block from chunkmem.
 * @param   id [in] ownerID of the chunkmem block.
 * @param   size [in] size of the chunkmem block.
 * @return 	success: start address of allocated chunkmem block,
 *  		fail: NULL
 * @see 	dlMalloc
 */
void* gp_chunk_malloc(unsigned int id, unsigned int size)
{
	void *ret = NULL;

	if ((chunkmem != NULL) && (size != 0)) {
		if (down_interruptible(&chunkmem->sem) == 0) {
#if USE_DLMALLOC_EX
			ret = dlMallocEx(id, size);
#else
			ret = dlMalloc(id, size);
#endif
			up(&chunkmem->sem);
		}
	}

	return ret;
}
EXPORT_SYMBOL(gp_chunk_malloc);

/**
 * @brief 	Free chunkmem block.
 * @param 	addr [in] start address of chunkmem block to free,
 *  			kernel_addr.
 * @return  None
 * @see 	dlFree
 */
void gp_chunk_free(void *addr)
{
	if (chunkmem != NULL) {
		if (down_interruptible(&chunkmem->sem) == 0) {
#if (DIAG_LEVEL >= DIAG_LVL_VERB) && !defined(DIAG_VERB_OFF)
			int ret = dlFree(addr);
#else
			dlFree(addr);
#endif
			up(&chunkmem->sem);
			DIAG_VERB("dlFree: %d\n", ret);
		}
	}
}
EXPORT_SYMBOL(gp_chunk_free);

/**
 * @brief   Free chunkmem blocks by ownerID.
 * @param   id [in] ownerID of the chunkmem blocks to free.
 * @return  None
 * @see     dlFreeAll
 */
void gp_chunk_free_all(unsigned int id)
{
	if (chunkmem != NULL) {
		if (down_interruptible(&chunkmem->sem) == 0) {
#if (DIAG_LEVEL >= DIAG_LVL_VERB) && !defined(DIAG_VERB_OFF)
			int ret = dlFreeAll(id);
#else
			dlFreeAll(id);
#endif
			up(&chunkmem->sem);
			DIAG_VERB("dlFreeAll: %d\n", ret);
		}
	}
}
EXPORT_SYMBOL(gp_chunk_free_all);

/**
 * @brief   Translate chunkmem kernel virtual address to physical address.
 * @param   va [in] kernel virtual address.
 * @return  success: physical address, fail: 0
 * @see
 */
unsigned int gp_chunk_pa(void *ka)
{
	unsigned int i;

	i = ka - chunkmem->vbase;
	if (i >= chunkmem->size) {
		return 0;      /* ka not in chunkmem region */
	}

	return (chunkmem->pbase + i);
}
EXPORT_SYMBOL(gp_chunk_pa);

/**
 * @brief   Translate physical address to chunkmem kernel virtual address.
 * @param   pa [in] physical address.
 * @return  success: kernel virtual address, fail: NULL
 * @see     CHUNK_MEM_FREE
 */
void* gp_chunk_va(unsigned int pa)
{
	unsigned int i;

	i = pa - chunkmem->pbase;
	if (i >= chunkmem->size) {
		return NULL;   /* pa not in chunkmem region */
	}

	return (chunkmem->vbase + i);
}
EXPORT_SYMBOL(gp_chunk_va);

/**
 * @brief   Translate user virtual address to physical address.
 * @param   va [in] user virtual address.
 * @return 	success: physical address, fail: 0
 * @see     CHUNK_MEM_FREE
 */
unsigned int gp_user_va_to_pa(void *va)
{
	pgd_t *pgd = NULL;
	pud_t *pud = NULL;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;
	struct mm_struct *mm = current->mm;
	unsigned int addr = (unsigned int)va;
	unsigned int pa = 0;

	down_read(&mm->mmap_sem);

	/* query page tables */
	if (!find_vma(mm, addr)) {
		DIAG_VERB("virt_addr %08X not available.\n", addr);
		goto out;
	}
	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd)) {
		DIAG_VERB("Not mapped in pgd.\n");
		goto out;
	}
	pud = pud_offset(pgd, addr);
	if (pud_none(*pud)) {
		DIAG_VERB("Not mapped in pud.\n");
		goto out;
	}
	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd)) {
		DIAG_VERB("Not mapped in pmd.\n");
		goto out;
	}
	pte = pte_offset_kernel(pmd, addr);
	if (pte_none(*pte)) {
		DIAG_VERB("Not mapped in pte.\n");
		goto out;
	}
	if (!pte_present(*pte)) {
		DIAG_VERB("pte not in RAM.\n");
		goto out;
	}

	pa = (pte_val(*pte) & PAGE_MASK) | (addr & ~PAGE_MASK);

out:
	up_read(&mm->mmap_sem);
	return pa;
}
EXPORT_SYMBOL(gp_user_va_to_pa);


/**
 * @brief   Chunkmem device open function
 */
static int chunkmem_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	unsigned int i;
	unsigned int pid = current->tgid;
	open_process_t *opens = chunkmem->opens;

	if (down_interruptible(&chunkmem->opens_sem) != 0) {
		return -ERESTARTSYS;
	}

	i = chunkmem->opens_count;
	if (i == MAX_OPEN_PROCESS) {
		DIAG_ERROR("chunkmem can't opened by more than %d process!\n", MAX_OPEN_PROCESS);
		ret = -EBUSY;
		goto out;
	}

	while (i--) {
		if (opens[i].pid == pid) { /* alreay open */
			opens[i].count++;
			goto out;
		}
	}

	/* append current process to opens */
	i = chunkmem->opens_count;
	opens[i].pid = pid;
	opens[i].count = 1;
	chunkmem->opens_count++;

out:
	up(&chunkmem->opens_sem);
	DIAG_VERB("[chunkmem_open] opens_count=%d pid=%d count=%d\n", chunkmem->opens_count, pid, opens[i].count);

	return ret;
}

/**
 * @brief   Chunkmem device release function
 */
static int chunkmem_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	int i;
	unsigned int pid = current->tgid;
	open_process_t *opens = chunkmem->opens;

	DIAG_VERB("chunkmem_release: pid = %d\n", pid);
	if (down_interruptible(&chunkmem->opens_sem) != 0) {
		return -ERESTARTSYS;
	}

	i = chunkmem->opens_count;
	while (i--) {
		if (opens[i].pid == pid) { /* found */
			break;
		}
	}

	opens[i].count--;
	DIAG_VERB("[chunkmem_release] opens_count=%d pid=%d count=%d\n", chunkmem->opens_count, pid, opens[i].count);
	if (opens[i].count == 0) {
		/* remove current process from opens */
		chunkmem->opens_count--;
		memcpy(&opens[i],
			   &opens[i + 1],
			   (chunkmem->opens_count - i) * sizeof(open_process_t));

		/* free all chunkmem blocks belong to current process */
		gp_chunk_free_all(pid);
	}

	up(&chunkmem->opens_sem);
	return ret;
}

/**
 * @brief   Chunkmem device mmap function
 */
static int chunkmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;

	if (!chunkmem->mmap_enable) {
		ret = -EPERM; /* disable calling mmap from user AP */
		goto out;
	}

	vma->vm_pgoff += (chunkmem->pbase >> PAGE_SHIFT);
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;
	ret = io_remap_pfn_range(vma,
							 vma->vm_start,
							 vma->vm_pgoff,
							 vma->vm_end - vma->vm_start,
							 vma->vm_page_prot);
	if (ret != 0) {
		ret = -EAGAIN;
	}
out:
	return ret;
}

#if CHUNK_SUSPEND_TEST
typedef struct data_block_s {
	void *addr;
	unsigned long size;
	struct data_block_s *next;
	unsigned char data[0];
} data_block_t;

data_block_t *blocks = NULL;

void my_save_data(unsigned long addr, unsigned long size)
{
	void *va;
	data_block_t *block = kmalloc(sizeof(data_block_t) + size, GFP_KERNEL);

	if (block == NULL) {
		DIAG_ERROR("save data error: out of memory! %p %08X\n", addr, size);
		return;
	}

	va = gp_chunk_va(addr);
	if (va == NULL) {
		va = __va(addr);
	}
	memcpy(&block->data, va, size);
	block->addr = va;
	DIAG_DEBUG("save data: %08X(%p) %08X\n", addr, va, size);
	block->size = size;
	block->next = blocks;
	blocks = block;
}
#endif

/**
 * @brief   Chunkmem device ioctl function
 */
static long chunkmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	chunk_block_t block;
	void *ka;           /* kernel_addr */
	unsigned int va;    /* user_addr */
	unsigned int pa;    /* phy_addr*/
	long ret = 0;
	unsigned int offset = 0;

	switch (cmd) {
	case CHUNK_MEM_ALLOC:
	case CHUNK_MEM_SHARE:
	case CHUNK_MEM_MMAP:
		{
			if (copy_from_user(&block, (void __user*)arg, sizeof(block))) {
				ret = -EFAULT;
				break;
			}

			/* alloc|share|mmap memory */
			if (cmd == CHUNK_MEM_MMAP) {
				DIAG_VERB("CHUNK_MEM_MMAP:\n");
				ka = gp_chunk_va(block.phy_addr);
				if (ka == NULL) {
					DIAG_ERROR("CHUNK_MEM_MMAP: bad address! (%s:%08X)\n", current->comm, block.phy_addr);
					ret = -EFAULT; /* mmap fail */
					break;
				}
				/* page alignment */
				offset = block.phy_addr & ~PAGE_MASK;
				ka = (void *)((unsigned long)ka & PAGE_MASK);
				DIAG_VERB("CHUNK_MEM_MMAP: phy_addr                  = %08X\n", block.phy_addr);
				DIAG_VERB("CHUNK_MEM_MMAP: size                      = %08X\n", block.size);
				DIAG_VERB("CHUNK_MEM_MMAP: ka                        = %08X\n", (unsigned int)ka);
				DIAG_VERB("CHUNK_MEM_MMAP: offset                    = %08X\n", offset);
				DIAG_VERB("CHUNK_MEM_MMAP: PAGE_ALIGN(size + offset) = %08X\n", PAGE_ALIGN(block.size + offset));
			}
			else {
				if (cmd == CHUNK_MEM_ALLOC) {
					DIAG_VERB("CHUNK_MEM_ALLOC:\n");
					DIAG_VERB("size = %08X (%d)\n", block.size, block.size);
					ka = gp_chunk_malloc(current->tgid, block.size);
					DIAG_VERB("gp_chunk_malloc return ka=%08X\n", ka);
					if (ka == NULL) {
						DIAG_ERROR("CHUNK_MEM_ALLOC: out of memory! (%s:%08X)\n", current->comm, block.size);
						dlMalloc_Status(NULL);
						ret = -ENOMEM;
						break;
					}
					block.phy_addr = gp_chunk_pa(ka);
				}
				else { /* CHUNK_MEM_SHARE */
					DIAG_VERB("CHUNK_MEM_SHARE:\n");
					ka = gp_chunk_va(block.phy_addr);
					if ((ka == NULL) || (dlShare(ka) == 0)) {
						DIAG_ERROR("CHUNK_MEM_SHARE: bad address! (%s:%08X)\n", current->comm, block.phy_addr);
						ret = -EFAULT; /* share fail */
						break;
					}
				}
				block.size = dlMalloc_Usable_Size(ka) & PAGE_MASK; /* actual allocated size */
				DIAG_VERB("actual size = %08X (%d)\n", block.size, block.size);
				DIAG_VERB("ka = %08X\n", (unsigned int)ka);
			}

			/* mmap to userspace */
			down_write(&current->mm->mmap_sem);
			chunkmem->mmap_enable = 1; /* enable mmap in CHUNK_MEM_ALLOC */
			va = do_mmap_pgoff(
				file, 0, PAGE_ALIGN(block.size + offset),
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				(ka - chunkmem->vbase) >> PAGE_SHIFT);
			chunkmem->mmap_enable = 0; /* disable it */
			up_write(&current->mm->mmap_sem);
			if (IS_ERR_VALUE(va)) {
				DIAG_ERROR("chunkmem mmap fail! (%s)\n", current->comm);
				ret = va; /* errcode */
				break;
			}
			va += offset;
			block.addr = (void *)va;
			DIAG_VERB("va = %08X\n\n", va);

			if (copy_to_user((void __user*)arg, &block, sizeof(block))) {
				ret = -EFAULT;
				break;
			}
		}
		break;

	case CHUNK_MEM_FREE:
		{
			if (copy_from_user(&block, (void __user*)arg, sizeof(block))) {
				ret = -EFAULT;
				break;
			}

			/* translate user_va to ka */
			DIAG_VERB("CHUNK_MEM_FREE:\n");
			DIAG_VERB("va = %08X\n", (unsigned int)block.addr);
			pa = gp_user_va_to_pa(block.addr);    /* user_addr to phy_addr */
			if (pa == 0) {
				DIAG_ERROR("CHUNK_MEM_FREE: chunkmem user_va_to_pa fail! (%s:%08X)\n", current->comm, block.addr);
				ret = -EFAULT;
				break;
			}
			DIAG_VERB("pa = %08X\n", pa);
			ka = gp_chunk_va(pa);                  /* phy_addr to kernel_addr */
			if (ka == NULL) {
				DIAG_ERROR("CHUNK_MEM_FREE: not a chunkmem address! (%s:%08X)\n", current->comm, pa);
				ret = -EFAULT;
				break;
			}
			block.size = dlMalloc_Usable_Size(ka) & PAGE_MASK;
			DIAG_VERB("ka = %08X\n", (unsigned int)ka);
			DIAG_VERB("actual size = %08X (%d)\n\n", block.size, block.size);

			/* munmap memory */
			down_write(&current->mm->mmap_sem);
			do_munmap(current->mm, (unsigned int)block.addr, block.size);
			up_write(&current->mm->mmap_sem);

			/* free memory */
			gp_chunk_free(ka);
#if (DIAG_LEVEL >= DIAG_LVL_VERB) && !defined(DIAG_VERB_OFF)
			dlMalloc_Status(NULL);
#endif
		}
		break;

	case CHUNK_MEM_INFO:
		{
			chunk_info_t info;

            if (copy_from_user(&info, (void __user*)arg, sizeof(info))) {
                ret = -EFAULT;
                break;
            }

            if (info.pid == (unsigned int)(-1)) {
                info.pid = current->tgid;
            }

#if CHUNK_SUSPEND_TEST
			if (info.pid) {
				dlMalloc_Status(NULL);
			}
			else {
				gp_chunk_suspend(my_save_data);
				memset(chunkmem->vbase, 0, chunkmem->size);
				/* restore */
				while (blocks != NULL) {
					data_block_t *block = blocks;
					blocks = block->next;
					DIAG_DEBUG("restore data: %p %08X\n", block->addr, block->size);
					memcpy(block->addr, &block->data, block->size);
					kfree(block);
				}
			}
#else
			dlMalloc_Status((mem_info_t *)&info);
#endif
			if (copy_to_user((void __user*)arg, &info, sizeof(info))) {
				ret = -EFAULT;
				break;
			}
		}
		break;

	case CHUNK_MEM_VA2PA:
		{
			ret = -EFAULT;
			if (copy_from_user(&block, (void __user*)arg, sizeof(block))) {
				break;
			}

			pa = gp_user_va_to_pa(block.addr);    /* user_addr to phy_addr */
			if (pa != 0) {
				ka = gp_chunk_va(pa);             /* phy_addr to kernel_addr */
				if (ka != NULL) {
					block.phy_addr = pa;
					if (copy_to_user((void __user*)arg, &block, sizeof(block)) == 0) {
						ret = 0;
					}
				}
			}
		}
		break;

	case CHUNK_MEM_MUNMAP:
		{
			if (copy_from_user(&block, (void __user*)arg, sizeof(block))) {
				ret = -EFAULT;
				break;
			}

			va = (unsigned int)block.addr;
			/* page alignment */
			offset = va & ~PAGE_MASK;
			va &= PAGE_MASK;

			/* munmap memory */
			down_write(&current->mm->mmap_sem);
			do_munmap(current->mm, va, PAGE_ALIGN(block.size + offset));
			up_write(&current->mm->mmap_sem);
		}
		break;

	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	return ret;
}

static struct file_operations chunkmem_fops = {
	.owner          = THIS_MODULE,
	.open           = chunkmem_open,
	.release        = chunkmem_release,
	.mmap           = chunkmem_mmap,
	.unlocked_ioctl = chunkmem_ioctl,
};

/**
 * @brief   Chunkmem driver init function
 */
static int __init chunkmem_init(void)
{
	int ret = -ENXIO;

	chunkmem = (chunkmem_info_t *)kzalloc(sizeof(chunkmem_info_t), GFP_KERNEL);
	if (chunkmem == NULL) {
		DIAG_ERROR("chunkmem kmalloc fail\n");
		ret = -ENOMEM;
		goto fail_kmalloc;
	}

	/* request memory region */
	chunkmem->pbase = membase;
	chunkmem->size  = memsize;
	chunkmem->mem   = request_mem_region(chunkmem->pbase, chunkmem->size, "chunkmem");
	if (chunkmem->mem == NULL) {
		DIAG_ERROR("chunkmem request mem region fail\n");
		goto fail_request_mem;
	}

	/* remap memory region */
	chunkmem->vbase = ioremap(chunkmem->pbase, chunkmem->size);
	if (chunkmem->vbase == NULL) {
		DIAG_ERROR("chunkmem ioremap fail\n");
		goto fail_ioremap;
	}

	/* initialize */
	ret = dlMallocInit(chunkmem->vbase, chunkmem->size, PAGE_SIZE);
	if (ret != 0) {
		DIAG_ERROR("dlmalloc init fail\n");
		ret = -ENXIO;
		goto fail_dlmalloc_init;
	}

	init_MUTEX(&chunkmem->sem);
	init_MUTEX(&chunkmem->opens_sem);

	/* register device */
	chunkmem->dev.name  = "chunkmem";
	chunkmem->dev.minor = MISC_DYNAMIC_MINOR;
	chunkmem->dev.fops  = &chunkmem_fops;
	ret = misc_register(&chunkmem->dev);
	if (ret != 0) {
		DIAG_ERROR("chunkmem device register fail\n");
		goto fail_device_register;
	}
	DIAG_INFO("[%s] pbase=%08X size=%08X\n",
			  chunkmem->dev.name, chunkmem->pbase, chunkmem->size);
	dlMalloc_Status(NULL);

	return 0;

	/* error rollback */
fail_device_register:
fail_dlmalloc_init:
	iounmap(chunkmem->vbase);
fail_ioremap:
	release_resource(chunkmem->mem);
fail_request_mem:
	kfree(chunkmem);
	chunkmem = NULL;
fail_kmalloc:
	return ret;
}

/**
 * @brief   Chunkmem driver exit function
 */
static void __exit chunkmem_exit(void)
{
	misc_deregister(&chunkmem->dev);
	iounmap(chunkmem->vbase);
	release_resource(chunkmem->mem);
	kfree(chunkmem);
	chunkmem = NULL;
}

module_init(chunkmem_init);
module_exit(chunkmem_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus Chunk Memory Driver");
MODULE_LICENSE_GP;
