/*
 * Gpfb!! common driver  Rev. 3.1.10
 *
 *  Copyright (C) 2008-2011   Solutions, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/buffer_head.h>
#include <linux/blkdev.h>
#include <linux/gpfb_param.h>
#include <asm/uaccess.h>

#ifdef CONFIG_MTD
#include <linux/mtd/mtd.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
#include <linux/freezer.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#include <asm/suspend.h>
#endif

#include "power.h"

#ifndef GPFB_WORK_SIZE
#define GPFB_WORK_SIZE          (128 * 1024)
#endif

#define ZONETBL_DEFAULT_NUM     8
#define EXTTBL_DEFAULT_NUM      8

#define SHRINK_BITE             (0xc0000000 >> PAGE_SHIFT)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,33)
#ifndef GPFB_SHRINK_REPEAT
#define GPFB_SHRINK_REPEAT      1
#endif
#else
#ifndef GPFB_SHRINK_REPEAT
#define GPFB_SHRINK_REPEAT      10
#endif
#ifndef GPFB_SHRINK_REPEAT2
#define GPFB_SHRINK_REPEAT2     1
#endif
#ifndef GPFB_SHRINK_REPEAT3
#define GPFB_SHRINK_REPEAT3     2
#endif
#endif

#ifndef GPFB_SHRINK_REPEAT_P1
#define GPFB_SHRINK_REPEAT_P1   100
#endif

#ifndef GPFB_SHRINK_THRESHOLD
#define GPFB_SHRINK_THRESHOLD   1
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
#define pm_device_suspend(x)    dpm_suspend_start(x)
#define pm_device_resume(x)     dpm_resume_end(x)
#define pm_device_power_down(x) dpm_suspend_noirq(x)
#define pm_device_power_up(x)   dpm_resume_noirq(x)
#else
#define pm_device_suspend(x)    device_suspend(x)
#define pm_device_power_down(x) device_power_down(x)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define pm_device_resume(x)     device_resume(x)
#define pm_device_power_up(x)   device_power_up(x)
#else
#define pm_device_resume(x)     device_resume()
#define pm_device_power_up(x)   device_power_up()
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
#define STATE_FREEZE    PMSG_FREEZE
#else
#define STATE_FREEZE    PM_SUSPEND_DISK
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define STATE_RESTORE   (!gpfb_stat ? (ret ? PMSG_RECOVER : PMSG_THAW) : \
                         PMSG_RESTORE)
#else
#define STATE_RESTORE
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
#define hibernate       pm_suspend_disk
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define current_uid()   (current->uid)
#endif

#ifdef CONFIG_SWAP
#define GPFB_SEPARATE_MAX       2
#else
#define GPFB_SEPARATE_MAX       0
#endif

#ifndef gpfb_pfn_valid
#define gpfb_pfn_valid(pfn)     pfn_valid(pfn)
#endif

#define GPFB_SAVEAREA_NUM       (sizeof(gpfb_savearea) / \
                                 sizeof(struct gpfb_savearea))

static const struct gpfb_savearea gpfb_savearea[] = {
    GPFB_SAVEAREA
};

int pm_device_down;
int gpfb_shrink;
int gpfb_swapout_disable;
int gpfb_separate_pass;
int gpfb_canceled;
struct gpfb gpfb_param;

EXPORT_SYMBOL(pm_device_down);
EXPORT_SYMBOL(gpfb_shrink);
EXPORT_SYMBOL(gpfb_swapout_disable);
EXPORT_SYMBOL(gpfb_separate_pass);
EXPORT_SYMBOL(gpfb_canceled);
EXPORT_SYMBOL(gpfb_param);

EXPORT_SYMBOL(hibdrv_snapshot);
EXPORT_SYMBOL(gpfb_set_savearea);
EXPORT_SYMBOL(gpfb_save_cancel);
EXPORT_SYMBOL(gpfb_register_machine);
EXPORT_SYMBOL(gpfb_unregister_machine);

static struct gpfb_ops *gpfb_ops;

static int gpfb_stat;
static int gpfb_error;
static int gpfb_retry;
static int gpfb_saveno;
static int gpfb_separate;

static int gpfb_save_pages;
static void *gpfb_work;
static unsigned long gpfb_work_start, gpfb_work_end;
static unsigned long gpfb_nosave_area, gpfb_nosave_size;
static unsigned long gpfb_lowmem_nosave_area, gpfb_lowmem_nosave_size;

static unsigned long zonetbl_max, exttbl_max, dramtbl_max;

#ifdef GPFB_HIBDRV_FLOATING
unsigned char *gpfb_hibdrv_buf;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22) && defined(GPFB_PFN_IS_NOSAVE)

extern const void __nosave_begin, __nosave_end;

int pfn_is_nosave(unsigned long pfn)
{
    unsigned long nosave_begin_pfn, nosave_end_pfn;

    nosave_begin_pfn = __pa(&__nosave_begin) >> PAGE_SHIFT;
    nosave_end_pfn = PAGE_ALIGN(__pa(&__nosave_end)) >> PAGE_SHIFT;
    return (pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}

#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)

bool system_entering_hibernation(void)
{
    return pm_device_down != GPFB_STATE_NORMAL;
}

EXPORT_SYMBOL(system_entering_hibernation);

#endif

#ifdef CONFIG_PM_GPFB_DEBUG

void gpfb_putc(char c)
{
    if (gpfb_ops->putc) {
        if (c == '\n')
            gpfb_ops->putc('\r');
        gpfb_ops->putc(c);
    }
}

int gpfb_printf(const char *fmt, ...)
{
    int i, len;
    va_list args;
    char buf[256];

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    for (i = 0; i < len; i++)
        gpfb_putc(buf[i]);
    return len;
}

EXPORT_SYMBOL(gpfb_putc);
EXPORT_SYMBOL(gpfb_printf);

#endif

#ifdef GPFB_HIBDRV_FLOATING

#ifdef CONFIG_MTD

static int gpfb_mtd(struct mtd_info *mtd, void *buf, size_t size)
{
    int ret;
    size_t read_size;
    loff_t offs = GPFB_HIBDRV_OFFSET;

    if (IS_ERR(mtd)) {
        printk("Can't find hibernation driver partition.\n");
        return PTR_ERR(mtd);
    }

#ifdef GPFB_HIBDRV_AREA_SIZE
    ret = -EIO;
    while (offs < GPFB_HIBDRV_OFFSET + GPFB_HIBDRV_AREA_SIZE) {
        if (!mtd->block_isbad || !mtd->block_isbad(mtd, offs)) {
            ret = mtd->read(mtd, offs, PAGE_SIZE, &read_size, buf);
            break;
        }
        offs += mtd->erasesize;
    }
#else
    ret = mtd->read(mtd, offs, PAGE_SIZE, &read_size, buf);
#endif
    if (ret >= 0) {
        if (GPFB_ID != GPFB_ID_DRIVER) {
            printk("Can't find hibernation driver.\n");
            return -EIO;
        }
        if (size <= GPFB_DRV_COPY_SIZE) {
            ret = -ENOMEM;
        } else {
            if (GPFB_DRV_COPY_SIZE <= PAGE_SIZE ||
                (ret = mtd->read(mtd, offs + PAGE_SIZE,
                                 GPFB_DRV_COPY_SIZE - PAGE_SIZE, &read_size,
                                 buf + PAGE_SIZE)) >= 0) {
                flush_cache_all();
                put_mtd_device(mtd);
                return 0;
            }
        }
    }

    printk("Can't load hibernation driver.\n");
    put_mtd_device(mtd);
    return ret;
}

int gpfb_mtd_load(int mtdno, void *buf, size_t size)
{
    return gpfb_mtd(get_mtd_device(NULL, mtdno), buf, size);
}

EXPORT_SYMBOL(gpfb_mtd_load);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)

int gpfb_mtd_load_nm(const char *mtdname, void *buf, size_t size)
{
    return gpfb_mtd(get_mtd_device_nm(mtdname), buf, size);
}

EXPORT_SYMBOL(gpfb_mtd_load_nm);

#endif

#endif

#ifdef GPFB_HIBDRV_DEV_LOAD

int gpfb_dev_load(const char *dev, void *buf, size_t size)
{
    int ret = 0;
    struct file *f;

    f = filp_open(dev, O_RDONLY, 0777);
    if (IS_ERR(f)) {
        printk("Can't open hibernation driver device.\n");
        return PTR_ERR(f);
    }

    if ((ret = kernel_read(f, GPFB_HIBDRV_OFFSET, buf, PAGE_SIZE)) >= 0) {
        if (GPFB_ID != GPFB_ID_DRIVER) {
            printk("Can't find hibernation driver.\n");
            return -EIO;
        }
        if (size <= GPFB_DRV_COPY_SIZE) {
            ret = -ENOMEM;
        } else {
            if (GPFB_DRV_COPY_SIZE <= PAGE_SIZE ||
                (ret = kernel_read(f, GPFB_HIBDRV_OFFSET + PAGE_SIZE,
                                   buf + PAGE_SIZE,
                                   GPFB_DRV_COPY_SIZE - PAGE_SIZE)) >= 0) {
                filp_close(f, NULL);
                flush_cache_all();
                return 0;
            }
        }
    }

    filp_close(f, NULL);

    printk("Can't load hibernation driver.\n");
    return ret;
}

EXPORT_SYMBOL(gpfb_dev_load);

#endif

#ifdef GPFB_HIBDRV_MEM_LOAD

int gpfb_mem_load(const void *hibdrv_addr, void *buf, size_t size)
{
    memcpy(buf, hibdrv_addr, PAGE_SIZE);
    if (GPFB_ID != GPFB_ID_DRIVER) {
        printk("Can't find hibernation driver.\n");
        return -EIO;
    }
    if (size > GPFB_DRV_COPY_SIZE) {
        printk("Can't load hibernation driver.\n");
        return -ENOMEM;
    }

    if (GPFB_DRV_COPY_SIZE > PAGE_SIZE)
        memcpy(buf + PAGE_SIZE, hibdrv_addr + PAGE_SIZE,
               GPFB_DRV_COPY_SIZE - PAGE_SIZE);
    flush_cache_all();
    return 0;
}

EXPORT_SYMBOL(gpfb_mem_load);

#endif

#endif  /* GPFB_HIBDRV_FLOATING */

static int gpfb_work_init(void)
{
#ifdef GPFB_HIBDRV_FLOATING
    int ret;
#endif
    int work_size, table_size;
    struct gpfb_savetbl *savetbl;

    for (work_size = GPFB_WORK_SIZE; ; work_size >>= 1) {
        if (work_size < PAGE_SIZE) {
            printk("gpfb: Can't alloc work memory.\n");
            return -ENOMEM;
        }
        if ((gpfb_work = kmalloc(work_size, GFP_KERNEL)) != NULL)
            break;
    }
    printk("Gpfb work area 0x%p-0x%p\n", gpfb_work, gpfb_work + work_size);

#ifdef GPFB_HIBDRV_FLOATING
    gpfb_hibdrv_buf = gpfb_work;
    BUG_ON(!gpfb_ops->drv_load);
    if ((ret = gpfb_ops->drv_load(gpfb_work, work_size)) < 0) {
        kfree(gpfb_work);
        return ret;
    }
    savetbl = gpfb_work + GPFB_DRV_COPY_SIZE;
    table_size = work_size - GPFB_DRV_COPY_SIZE;
#else
    savetbl = gpfb_work;
    table_size = work_size;
#endif

    zonetbl_max = ZONETBL_DEFAULT_NUM;
    exttbl_max = EXTTBL_DEFAULT_NUM;
    dramtbl_max = table_size / sizeof(struct gpfb_savetbl) -
        ZONETBL_DEFAULT_NUM - EXTTBL_DEFAULT_NUM;
    gpfb_param.zonetbl = savetbl + dramtbl_max + exttbl_max;
    gpfb_param.exttbl = savetbl + dramtbl_max;
    gpfb_param.dramtbl = savetbl;
    gpfb_param.zonetbl_num = 0;
    gpfb_param.exttbl_num = 0;
    gpfb_param.dramtbl_num = 0;
    gpfb_work_start = page_to_pfn(virt_to_page(gpfb_work));
    gpfb_work_end = gpfb_work_start + ((work_size - 1) >> PAGE_SHIFT) + 1;
    gpfb_nosave_area = (unsigned long)-1;
    gpfb_nosave_size = 0;
    gpfb_lowmem_nosave_area = (unsigned long)-1;
    gpfb_lowmem_nosave_size = 0;
    gpfb_param.maxarea = 0;
    gpfb_param.maxsize = 0;
    gpfb_param.lowmem_maxarea = 0;
    gpfb_param.lowmem_maxsize = 0;
    gpfb_save_pages = 0;
    return 0;
}

static void gpfb_work_free(void)
{
    kfree(gpfb_work);
}

static void gpfb_set_tbl(unsigned long start, unsigned long end,
                         struct gpfb_savetbl *tbl, int *num)
{
    if (*num > 0 && start == tbl[*num - 1].end) {
        tbl[*num - 1].end = end;
    } else if (start < end) {
        tbl[*num].start = start;
        tbl[*num].end = end;
        (*num)++;
    }
}

int gpfb_set_savearea(unsigned long start, unsigned long end)
{
    struct gpfb_savetbl *tbl;

    if (gpfb_param.exttbl_num >= exttbl_max) {
        if (gpfb_param.dramtbl_num + EXTTBL_DEFAULT_NUM > dramtbl_max) {
            printk("gpfb: save table overflow\n");
            return -ENOMEM;
        }
        tbl = gpfb_param.exttbl - EXTTBL_DEFAULT_NUM;
        memmove(tbl, gpfb_param.exttbl,
                exttbl_max * sizeof(struct gpfb_savetbl));
        gpfb_param.exttbl = tbl;
        exttbl_max += EXTTBL_DEFAULT_NUM;
    }
    gpfb_set_tbl(start, end, gpfb_param.exttbl, &gpfb_param.exttbl_num);
    return 0;
}

static int gpfb_set_save_zone(unsigned long pfn)
{
    struct gpfb_savetbl *tbl;

    if (gpfb_param.zonetbl_num >= zonetbl_max) {
        if (gpfb_param.dramtbl_num + EXTTBL_DEFAULT_NUM > dramtbl_max) {
            printk("gpfb: save table overflow\n");
            return -ENOMEM;
        }
        tbl = gpfb_param.exttbl - ZONETBL_DEFAULT_NUM;
        memmove(tbl, gpfb_param.exttbl,
                (exttbl_max + zonetbl_max) * sizeof(struct gpfb_savetbl));
        gpfb_param.exttbl = tbl;
        gpfb_param.zonetbl -= ZONETBL_DEFAULT_NUM;
        zonetbl_max += ZONETBL_DEFAULT_NUM;
    }
    gpfb_set_tbl(pfn, pfn + 1, gpfb_param.zonetbl, &gpfb_param.zonetbl_num);
    return 0;
}

static int gpfb_set_save_dram(struct zone *zone, unsigned long pfn)
{
    if (gpfb_param.dramtbl_num >= dramtbl_max) {
        printk("gpfb: save table overflow\n");
        return -ENOMEM;
    }
    gpfb_set_tbl(pfn, pfn + 1, gpfb_param.dramtbl, &gpfb_param.dramtbl_num);
    return 0;
}

static void gpfb_set_nosave_dram(struct zone *zone, unsigned long pfn)
{
    if (pfn == gpfb_nosave_area + gpfb_nosave_size) {
        gpfb_nosave_size++;
        if (gpfb_param.maxsize < gpfb_nosave_size) {
            gpfb_param.maxarea = gpfb_nosave_area;
            gpfb_param.maxsize = gpfb_nosave_size;
        }
    } else {
        gpfb_nosave_area = pfn;
        gpfb_nosave_size = 1;
    }
    if (!is_highmem(zone)) {
        if (pfn == gpfb_lowmem_nosave_area + gpfb_lowmem_nosave_size) {
            gpfb_lowmem_nosave_size++;
            if (gpfb_param.lowmem_maxsize < gpfb_lowmem_nosave_size) {
                gpfb_param.lowmem_maxarea = gpfb_lowmem_nosave_area;
                gpfb_param.lowmem_maxsize = gpfb_lowmem_nosave_size;
            }
        } else {
            gpfb_lowmem_nosave_area = pfn;
            gpfb_lowmem_nosave_size = 1;
        }
    }
}

static int gpfb_make_save_table(void)
{
    int ret;
    struct zone *zone;
    unsigned long pfn, end;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
    unsigned long pfn2;
#endif

    for_each_zone (zone) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
        mark_free_pages(zone);
#endif
        end = zone->zone_start_pfn + zone->spanned_pages;
        for (pfn = zone->zone_start_pfn; pfn < end; pfn++) {
            if (!gpfb_pfn_valid(pfn) ||
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
                swsusp_page_is_forbidden(pfn_to_page(pfn)) ||
#endif
                (gpfb_param.mode == 1 &&
                 pfn >= gpfb_work_start && pfn < gpfb_work_end))
                continue;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
            pfn2 = pfn;
            if (gpfb_param.mode != 1 || swsusp_page_is_saveable(zone, &pfn2)) {
                gpfb_save_pages++;
                if ((ret = gpfb_set_save_zone(pfn)) < 0)
                    return ret;
                if ((ret = gpfb_set_save_dram(zone, pfn)) < 0)
                    return ret;
            } else {
                pfn--;
                while (pfn < pfn2) {
                    if ((ret = gpfb_set_save_zone(++pfn)) < 0)
                        return ret;
                    gpfb_set_nosave_dram(zone, pfn);
                }
            }
#else
            if ((ret = gpfb_set_save_zone(pfn)) < 0)
                return ret;
            if (gpfb_param.mode != 1 || swsusp_page_is_saveable(zone, pfn)) {
                gpfb_save_pages++;
                if ((ret = gpfb_set_save_dram(zone, pfn)) < 0)
                    return ret;
            } else {
                gpfb_set_nosave_dram(zone, pfn);
            }
#endif
        }
    }
    return 0;
}

#ifdef GPFB_PRINT_MEMINFO

#define K(x) ((x) << (PAGE_SHIFT - 10))

static void gpfb_print_meminfo(void)
{
    struct sysinfo si;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
    struct page_state ps;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)
    unsigned long active, inactive, free;
#endif
    unsigned long buffers, cached, dirty, mapped;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)
    get_zone_counts(&active, &inactive, &free);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
    get_page_state(&ps);
    dirty = ps.nr_dirty;
    mapped = ps.nr_mapped;
    cached = get_page_cache_size();
#else
    dirty = global_page_state(NR_FILE_DIRTY);
    mapped = global_page_state(NR_FILE_MAPPED);
    cached = global_page_state(NR_FILE_PAGES);
#endif
    buffers = nr_blockdev_pages();
    cached -= total_swapcache_pages + buffers;
    si_swapinfo(&si);

    printk("Buffers        :%8lu KB\n"
           "Cached         :%8lu KB\n"
           "SwapCached     :%8lu KB\n"
           "SwapUsed       :%8lu KB\n",
           K(buffers),
           K(cached),
           K(total_swapcache_pages),
           K(si.totalswap - si.freeswap));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)
    printk("Active         :%8lu KB\n"
           "Inactive       :%8lu KB\n",
           K(active),
           K(inactive));
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
    printk("Active         :%8lu KB\n"
           "Inactive       :%8lu KB\n",
           K(global_page_state(NR_ACTIVE)),
           K(global_page_state(NR_INACTIVE)));
#else
    printk("Active(anon)   :%8lu KB\n"
           "Inactive(anon) :%8lu KB\n"
           "Active(file)   :%8lu KB\n"
           "Inactive(file) :%8lu KB\n",
           K(global_page_state(NR_ACTIVE_ANON)),
           K(global_page_state(NR_INACTIVE_ANON)),
           K(global_page_state(NR_ACTIVE_FILE)),
           K(global_page_state(NR_INACTIVE_FILE)));
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
    printk("AnonPages      :%8lu KB\n",
           K(global_page_state(NR_ANON_PAGES)));
#endif
    printk("Dirty          :%8lu KB\n"
           "Mapped         :%8lu KB\n",
           K(dirty),
           K(mapped));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
    printk("Slab           :%8lu kB\n"
           "PageTables     :%8lu kB\n",
           K(ps.nr_slab),
           K(ps.nr_page_table_pages));
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
    printk("Slab           :%8lu kB\n"
           "PageTables     :%8lu kB\n",
           K(global_page_state(NR_SLAB)),
           K(global_page_state(NR_PAGETABLE)));
#else
    printk("SReclaimable   :%8lu kB\n"
           "SUnreclaim     :%8lu kB\n"
           "PageTables     :%8lu kB\n",
           K(global_page_state(NR_SLAB_RECLAIMABLE)),
           K(global_page_state(NR_SLAB_UNRECLAIMABLE)),
           K(global_page_state(NR_PAGETABLE)));
#endif
}

#else

#define gpfb_print_meminfo()

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)

static void invalidate_filesystems(int invalidate)
{
    struct super_block *sb;
    struct block_device *bdev;

    spin_lock(&sb_lock);
restart:
    list_for_each_entry(sb, &super_blocks, s_list) {
        if (sb->s_bdev) {
            sb->s_count++;
            spin_unlock(&sb_lock);
            fsync_bdev(sb->s_bdev);
            if (invalidate)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
                __invalidate_device(sb->s_bdev);
#else
                __invalidate_device(sb->s_bdev, 0);
#endif
        } else {
            sb->s_count++;
            spin_unlock(&sb_lock);
            bdev = bdget(sb->s_dev);
            fsync_bdev(bdev);
            fsync_super(sb);
            if (invalidate) {
                shrink_dcache_sb(sb);
                invalidate_inodes(sb);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
                invalidate_bdev(bdev);
#else
                invalidate_bdev(bdev, 0);
#endif
            }
        }
        spin_lock(&sb_lock);
        if (__put_super_and_need_restart(sb))
            goto restart;
    }
    spin_unlock(&sb_lock);
}

#endif

static int gpfb_shrink_memory(void)
{
    int i, pages;
    int shrink_sav = gpfb_shrink;
    int repeat = GPFB_SHRINK_REPEAT;

#ifdef CONFIG_SWAP
    if (!gpfb_swapout_disable) {
        gpfb_shrink = GPFB_SHRINK_ALL;
        repeat = GPFB_SHRINK_REPEAT_P1;
    }
#endif

    if (gpfb_shrink != GPFB_SHRINK_NONE) {
        gpfb_print_meminfo();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
        invalidate_filesystems(1);
#endif
#if LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,18) || \
    LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
        if (gpfb_shrink == GPFB_SHRINK_LIMIT1)
            repeat = GPFB_SHRINK_REPEAT2;
        else if (gpfb_shrink == GPFB_SHRINK_LIMIT2)
            repeat = GPFB_SHRINK_REPEAT3;
#endif
        for (i = 0; i < repeat; i++) {
            printk("Shrinking memory...  ");
            pages = shrink_all_memory(SHRINK_BITE);
            printk("\bdone (%d pages freed)\n", pages);
            if (pages <= GPFB_SHRINK_THRESHOLD)
                break;
        }
    }
    gpfb_print_meminfo();

    gpfb_shrink = shrink_sav;
    return 0;
}

int hibdrv_snapshot(void)
{
    int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
    drain_local_pages(NULL);
#else
    drain_local_pages();
#endif
    if ((ret = gpfb_make_save_table()) < 0)
        return ret;

    printk("dram save %d pages\n", gpfb_save_pages);
    printk("maxarea 0x%08lx(0x%08lx)  lowmem_maxarea 0x%08lx(0x%08lx)\n",
           gpfb_param.maxarea << PAGE_SHIFT,
           gpfb_param.maxsize << PAGE_SHIFT,
           gpfb_param.lowmem_maxarea << PAGE_SHIFT,
           gpfb_param.lowmem_maxsize << PAGE_SHIFT);
    printk("zonetbl %d  exttbl %d  dramtbl %d\n", gpfb_param.zonetbl_num,
           gpfb_param.exttbl_num, gpfb_param.dramtbl_num);

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_SAVE);

    if (gpfb_canceled) {
        ret = -ECANCELED;
    } else {
        if ((ret = GPFB_DRV_SNAPSHOT(&gpfb_param)) == -ECANCELED)
            gpfb_canceled = 1;
    }

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_SAVEEND);

    return ret;
}

static int gpfb_save_snapshot(void)
{
    int ret = 0;

    gpfb_param.bootflag_dev  = gpfb_savearea[gpfb_saveno].bootflag_dev;
    gpfb_param.bootflag_area = gpfb_savearea[gpfb_saveno].bootflag_area;
    gpfb_param.bootflag_size = gpfb_savearea[gpfb_saveno].bootflag_size;
    gpfb_param.snapshot_dev  = gpfb_savearea[gpfb_saveno].snapshot_dev;
    gpfb_param.snapshot_area = gpfb_savearea[gpfb_saveno].snapshot_area;
    gpfb_param.snapshot_size = gpfb_savearea[gpfb_saveno].snapshot_size;

#ifdef CONFIG_SWAP
    if (!gpfb_param.oneshot && total_swap_pages > nr_swap_pages)
        gpfb_swapout_disable = 1;
    else
        gpfb_swapout_disable = 0;
#endif
    if (GPFB_ID != GPFB_ID_DRIVER) {
        printk("Can't find hibernation driver.\n");
        ret = -EIO;
    } else {
        if ((ret = gpfb_ops->snapshot()) == 0) {
            gpfb_stat = gpfb_param.stat;
            gpfb_retry = gpfb_param.retry;
        }
    }

    return ret;
}

void gpfb_save_cancel(void)
{
    if (pm_device_down == GPFB_STATE_SUSPEND || gpfb_separate_pass == 1) {
        gpfb_canceled = 1;
        if (gpfb_ops->progress)
            gpfb_ops->progress(GPFB_PROGRESS_CANCEL);
    }
}

int hibernate(void)
{
    int ret;

    if (!gpfb_ops) {
        printk("Snapshot driver not found.\n");
        return -EIO;
    }

    BUG_ON(!gpfb_ops->snapshot);

    gpfb_stat = 0;
    gpfb_retry = 0;
    pm_device_down = GPFB_STATE_SUSPEND;

    if (gpfb_separate == 0) {
        gpfb_separate_pass = 0;
        gpfb_swapout_disable = 1;
    } else if (gpfb_separate == 1) {
        gpfb_separate_pass = 0;
        gpfb_swapout_disable = 0;
    } else if (gpfb_separate == 2) {
        if (gpfb_separate_pass != 1) {
            gpfb_separate_pass = 1;
            gpfb_swapout_disable = 0;
        } else {
            gpfb_separate_pass = 2;
            gpfb_swapout_disable = 1;
        }
    }

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_INIT);

    if (gpfb_separate_pass == 2 && gpfb_canceled) {
        gpfb_separate_pass = 0;
        ret = -ECANCELED;
        goto gpfb_work_init_err;
    }

    if ((ret = gpfb_work_init()) < 0) {
        gpfb_separate_pass = 0;
        goto gpfb_work_init_err;
    }

    if (gpfb_ops->drv_init && (ret = gpfb_ops->drv_init()) < 0) {
        gpfb_separate_pass = 0;
        goto gpfb_drv_init_err;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    system_state = SYSTEM_SUSPEND_DISK;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    mutex_lock(&pm_mutex);
#endif

#ifdef GPFB_PREPARE_CONSOLE
    pm_prepare_console();
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
    if ((ret = pm_notifier_call_chain(PM_HIBERNATION_PREPARE))) {
        gpfb_separate_pass = 0;
        pm_device_down = GPFB_STATE_RESUME;
        goto pm_notifier_call_chain_err;
    }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    if ((ret = usermodehelper_disable())) {
        gpfb_separate_pass = 0;
        pm_device_down = GPFB_STATE_RESUME;
        goto usermodehelper_disable_err;
    }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    /* Allocate memory management structures */
    if ((ret = create_basic_memory_bitmaps())) {
        gpfb_separate_pass = 0;
        pm_device_down = GPFB_STATE_RESUME;
        goto create_basic_memory_bitmaps_err;
    }
#endif

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_SYNC);

    printk("Syncing filesystems ... ");
    sys_sync();
    printk("done.\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,21)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    if ((ret = disable_nonboot_cpus())) {
        gpfb_separate_pass = 0;
        pm_device_down = GPFB_STATE_RESUME;
        goto disable_nonboot_cpus_err;
    }
#else
    disable_nonboot_cpus();
#endif
#endif

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_FREEZE);

    if (freeze_processes()) {
        ret = -EBUSY;
        gpfb_separate_pass = 0;
        pm_device_down = GPFB_STATE_RESUME;
        goto freeze_processes_err;
    }

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_SHRINK);

    /* Free memory before shutting down devices. */
    if ((ret = gpfb_shrink_memory()) || gpfb_separate_pass == 1 ||
        gpfb_canceled) {
        pm_device_down = GPFB_STATE_RESUME;
        goto gpfb_shrink_memory_err;
    }

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_SUSPEND);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    suspend_console();
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    disable_nonboot_cpus();
#endif

    if (gpfb_ops->device_suspend_early &&
        (ret = gpfb_ops->device_suspend_early())) {
        pm_device_down = GPFB_STATE_RESUME;
        goto gpfb_device_suspend_early_err;
    }

    if ((ret = pm_device_suspend(STATE_FREEZE))) {
        pm_device_down = GPFB_STATE_RESUME;
        goto pm_device_suspend_err;
    }

    if (gpfb_ops->device_suspend && (ret = gpfb_ops->device_suspend())) {
        pm_device_down = GPFB_STATE_RESUME;
        goto gpfb_device_suspend_err;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,30)
    if ((ret = disable_nonboot_cpus())) {
        pm_device_down = GPFB_STATE_RESUME;
        goto disable_nonboot_cpus_err;
    }
#endif

    if ((ret = arch_prepare_suspend())) {
        pm_device_down = GPFB_STATE_RESUME;
        goto arch_prepare_suspend_err;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    if ((ret = pm_device_power_down(STATE_FREEZE))) {
        pm_device_down = GPFB_STATE_RESUME;
        goto pm_device_power_down_err;
    }
#endif

    if (gpfb_ops->pre_snapshot && (ret = gpfb_ops->pre_snapshot())) {
        pm_device_down = GPFB_STATE_RESUME;
        goto gpfb_pre_snapshot_err;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    if ((ret = disable_nonboot_cpus())) {
        pm_device_down = GPFB_STATE_RESUME;
        goto disable_nonboot_cpus_err;
    }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,30)
    device_pm_lock();
#endif

    local_irq_disable();

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    if ((ret = pm_device_power_down(STATE_FREEZE))) {
        pm_device_down = GPFB_STATE_RESUME;
        goto pm_device_power_down_err;
    }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    if ((ret = sysdev_suspend(STATE_FREEZE))) {
        pm_device_down = GPFB_STATE_RESUME;
        goto sysdev_suspend_err;
    }
#endif

    save_processor_state();

    /* Snapshot save */
    ret = gpfb_save_snapshot();

    pm_device_down = GPFB_STATE_RESUME;
    restore_processor_state();

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_RESUME);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    sysdev_resume();
sysdev_suspend_err:
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    pm_device_power_up(STATE_RESTORE);
pm_device_power_down_err:
#endif

    local_irq_enable();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,30)
    device_pm_unlock();
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
disable_nonboot_cpus_err:
    enable_nonboot_cpus();
#endif

    if (gpfb_ops->post_snapshot)
        gpfb_ops->post_snapshot();
gpfb_pre_snapshot_err:

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    pm_device_power_up(STATE_RESTORE);
pm_device_power_down_err:
#endif

arch_prepare_suspend_err:

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,30)
disable_nonboot_cpus_err:
    enable_nonboot_cpus();
#endif

    if (gpfb_ops->device_resume)
        gpfb_ops->device_resume();
gpfb_device_suspend_err:

    pm_device_resume(STATE_RESTORE);
pm_device_suspend_err:

    if (gpfb_ops->device_resume_late)
        gpfb_ops->device_resume_late();
gpfb_device_suspend_early_err:

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    enable_nonboot_cpus();
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    resume_console();
#endif

gpfb_shrink_memory_err:

freeze_processes_err:
    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_THAW);
    thaw_processes();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && \
    LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,21)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
disable_nonboot_cpus_err:
#endif
    enable_nonboot_cpus();
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    free_basic_memory_bitmaps();
create_basic_memory_bitmaps_err:
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    usermodehelper_enable();
usermodehelper_disable_err:
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
    pm_notifier_call_chain(PM_POST_HIBERNATION);
pm_notifier_call_chain_err:
#endif

#ifdef GPFB_PREPARE_CONSOLE
    pm_restore_console();
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    mutex_unlock(&pm_mutex);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    system_state = SYSTEM_RUNNING;
#endif

    if (gpfb_ops->drv_uninit)
        gpfb_ops->drv_uninit();
gpfb_drv_init_err:

    gpfb_work_free();
gpfb_work_init_err:

    if (gpfb_separate_pass == 2)
        gpfb_separate_pass = 0;

    pm_device_down = GPFB_STATE_NORMAL;

    if (gpfb_ops->progress)
        gpfb_ops->progress(GPFB_PROGRESS_EXIT);

    if (gpfb_canceled)
        ret = -ECANCELED;
    gpfb_error = ret;
    if (ret < 0)
        printk(KERN_ERR "Gpfb!! error %d\n", ret);
    return ret;
}

#ifdef CONFIG_PROC_FS

static struct proc_dir_entry *proc_gpfb;
static struct proc_dir_entry *proc_gpfb_stat;
static struct proc_dir_entry *proc_gpfb_error;
static struct proc_dir_entry *proc_gpfb_retry;
static struct proc_dir_entry *proc_gpfb_canceled;
static struct proc_dir_entry *proc_gpfb_saveno;
static struct proc_dir_entry *proc_gpfb_mode;
static struct proc_dir_entry *proc_gpfb_compress;
static struct proc_dir_entry *proc_gpfb_shrink;
static struct proc_dir_entry *proc_gpfb_separate;
static struct proc_dir_entry *proc_gpfb_oneshot;
static struct proc_dir_entry *proc_gpfb_halt;
static struct proc_dir_entry *proc_gpfb_silent;

static int write_proc_gpfb(const char *buffer, unsigned long count,
                           int max, const char *str)
{
    int val;
    char buf[16];

    if (current_uid() != 0)
        return -EACCES;
    if (count == 0 || count >= 16)
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;
    buf[count] = '\0';

    sscanf(buf, "%d", &val);

    if (val > max) {
        printk("gpfb: %s too large !!\n", str);
        return -EINVAL;
    }
    return val;
}

static int read_proc_gpfb_stat(char *page, char **start, off_t offset,
                               int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_stat);
    *eof = 1;
    return len;
}

static int read_proc_gpfb_error(char *page, char **start, off_t offset,
                                int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_error);
    *eof = 1;
    return len;
}

static int read_proc_gpfb_retry(char *page, char **start, off_t offset,
                                int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_retry);
    *eof = 1;
    return len;
}

static int read_proc_gpfb_canceled(char *page, char **start, off_t offset,
                                   int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_canceled);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_canceled(struct file *file, const char *buffer,
                                    unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 1, "canceled")) < 0)
        return val;
    gpfb_canceled = val;
    return count;
}

static int read_proc_gpfb_saveno(char *page, char **start, off_t offset,
                                 int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_saveno);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_saveno(struct file *file, const char *buffer,
                                  unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, GPFB_SAVEAREA_NUM - 1,
                               "saveno")) < 0)
        return val;
    gpfb_saveno = val;
    return count;
}

static int read_proc_gpfb_mode(char *page, char **start, off_t offset,
                               int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_param.mode);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_mode(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 2, "mode")) < 0)
        return val;
    gpfb_param.mode = val;
    return count;
}

static int read_proc_gpfb_compress(char *page, char **start, off_t offset,
                                   int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_param.compress);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_compress(struct file *file, const char *buffer,
                                    unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 2, "compress")) < 0)
        return val;
    gpfb_param.compress = val;
    return count;
}

static int read_proc_gpfb_shrink(char *page, char **start, off_t offset,
                                 int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_shrink);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_shrink(struct file *file, const char *buffer,
                                  unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 3, "shrink")) < 0)
        return val;
    gpfb_shrink = val;
    return count;
}

static int read_proc_gpfb_separate(char *page, char **start, off_t offset,
                               int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_separate);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_separate(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, GPFB_SEPARATE_MAX,
                               "separate")) < 0)
        return val;
    gpfb_separate = val;
    gpfb_separate_pass = 0;
    return count;
}

static int read_proc_gpfb_oneshot(char *page, char **start, off_t offset,
                                  int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_param.oneshot);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_oneshot(struct file *file, const char *buffer,
                                   unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 1, "oneshot")) < 0)
        return val;
    gpfb_param.oneshot = val;
    return count;
}


static int read_proc_gpfb_halt(char *page, char **start, off_t offset,
                               int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_param.halt);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_halt(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 1, "halt")) < 0)
        return val;
    gpfb_param.halt = val;
    return count;
}

static int read_proc_gpfb_silent(char *page, char **start, off_t offset,
                                 int count, int *eof, void *data)
{
    int len;

    len = sprintf(page, "%d\n", gpfb_param.silent);
    *eof = 1;
    return len;
}

static int write_proc_gpfb_silent(struct file *file, const char *buffer,
                                  unsigned long count, void *data)
{
    int val;

    if ((val = write_proc_gpfb(buffer, count, 3, "silent")) < 0)
        return val;
    gpfb_param.silent = val;
    return count;
}

#endif

int gpfb_register_machine(struct gpfb_ops *ops)
{
    gpfb_ops = ops;
    return 0;
}

int gpfb_unregister_machine(struct gpfb_ops *ops)
{
    gpfb_ops = NULL;
    return 0;
}

static int __init gpfb_init(void)
{
#ifdef CONFIG_PROC_FS
    if ((proc_gpfb = proc_mkdir("gpfb", NULL))) {
        proc_gpfb_stat =
            create_proc_read_entry("stat",
                                   S_IRUGO,
                                   proc_gpfb,
                                   read_proc_gpfb_stat,
                                   NULL);
        proc_gpfb_error =
            create_proc_read_entry("error",
                                   S_IRUGO,
                                   proc_gpfb,
                                   read_proc_gpfb_error,
                                   NULL);
        proc_gpfb_retry =
            create_proc_read_entry("retry",
                                   S_IRUGO,
                                   proc_gpfb,
                                   read_proc_gpfb_retry,
                                   NULL);
        if ((proc_gpfb_canceled =
             create_proc_entry("canceled",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_canceled->read_proc = read_proc_gpfb_canceled;
            proc_gpfb_canceled->write_proc = write_proc_gpfb_canceled;
        }
        if ((proc_gpfb_saveno =
             create_proc_entry("saveno",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_saveno->read_proc = read_proc_gpfb_saveno;
            proc_gpfb_saveno->write_proc = write_proc_gpfb_saveno;
        }
        if ((proc_gpfb_mode =
             create_proc_entry("mode",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_mode->read_proc = read_proc_gpfb_mode;
            proc_gpfb_mode->write_proc = write_proc_gpfb_mode;
        }
        if ((proc_gpfb_compress =
             create_proc_entry("compress",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_compress->read_proc = read_proc_gpfb_compress;
            proc_gpfb_compress->write_proc = write_proc_gpfb_compress;
        }
        if ((proc_gpfb_shrink =
             create_proc_entry("shrink",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_shrink->read_proc = read_proc_gpfb_shrink;
            proc_gpfb_shrink->write_proc = write_proc_gpfb_shrink;
        }
        if ((proc_gpfb_separate =
             create_proc_entry("separate",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_separate->read_proc = read_proc_gpfb_separate;
            proc_gpfb_separate->write_proc = write_proc_gpfb_separate;
        }
        if ((proc_gpfb_oneshot =
             create_proc_entry("oneshot",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_oneshot->read_proc = read_proc_gpfb_oneshot;
            proc_gpfb_oneshot->write_proc = write_proc_gpfb_oneshot;
        }
        if ((proc_gpfb_halt =
             create_proc_entry("halt",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_halt->read_proc = read_proc_gpfb_halt;
            proc_gpfb_halt->write_proc = write_proc_gpfb_halt;
        }
        if ((proc_gpfb_silent =
             create_proc_entry("silent",
                               S_IRUGO | S_IWUSR,
                               proc_gpfb))) {
            proc_gpfb_silent->read_proc = read_proc_gpfb_silent;
            proc_gpfb_silent->write_proc = write_proc_gpfb_silent;
        }
    }
#endif

    gpfb_saveno = CONFIG_PM_GPFB_SAVENO;
    gpfb_param.mode = CONFIG_PM_GPFB_MODE;
    gpfb_param.compress = CONFIG_PM_GPFB_COMPRESS;
    gpfb_param.silent = CONFIG_PM_GPFB_SILENT;
    gpfb_shrink = CONFIG_PM_GPFB_SHRINK;
#ifdef CONFIG_SWAP
    gpfb_separate = CONFIG_PM_GPFB_SEPARATE;
#else
    gpfb_separate = 0;
#endif
#ifdef CONFIG_PM_GPFB_ONESHOT
    gpfb_param.oneshot = 1;
#else
    gpfb_param.oneshot = 0;
#endif
#ifdef CONFIG_PM_GPFB_HALT
    gpfb_param.halt = 1;
#else
    gpfb_param.halt = 0;
#endif
    gpfb_param.ver_major = GPFB_PARAM_VER_MAJOR;
    gpfb_param.ver_minor = GPFB_PARAM_VER_MINOR;

#ifdef GPFB_PRELOAD_EXTTBL
    gpfb_param.preload_exttbl = 1;
#else
    gpfb_param.preload_exttbl = 0;
#endif

    gpfb_param.v2p_offset = PAGE_OFFSET - __pa(PAGE_OFFSET);
    gpfb_param.lowmem_size = (unsigned long)high_memory - PAGE_OFFSET;
    gpfb_param.page_shift = PAGE_SHIFT;

    gpfb_param.console = GPFB_CONSOLE;
    gpfb_param.bps = GPFB_BPS;

    gpfb_swapout_disable = 0;
    gpfb_separate_pass = 0;
    gpfb_canceled = 0;

    printk(KERN_INFO " Gpfb!! module loaded\n");

    return 0;
}

static void __exit gpfb_exit(void)
{
#ifdef CONFIG_PROC_FS
    if (proc_gpfb_stat) {
        remove_proc_entry("stat", proc_gpfb_stat);
        proc_gpfb_stat = NULL;
    }
    if (proc_gpfb_error) {
        remove_proc_entry("error", proc_gpfb_error);
        proc_gpfb_error = NULL;
    }
    if (proc_gpfb_retry) {
        remove_proc_entry("retry", proc_gpfb_retry);
        proc_gpfb_retry = NULL;
    }
    if (proc_gpfb_canceled) {
        remove_proc_entry("canceled", proc_gpfb_canceled);
        proc_gpfb_canceled = NULL;
    }
    if (proc_gpfb_saveno) {
        remove_proc_entry("saveno", proc_gpfb_saveno);
        proc_gpfb_saveno = NULL;
    }
    if (proc_gpfb_mode) {
        remove_proc_entry("mode", proc_gpfb_mode);
        proc_gpfb_mode = NULL;
    }
    if (proc_gpfb_compress) {
        remove_proc_entry("compress", proc_gpfb_compress);
        proc_gpfb_compress = NULL;
    }
    if (proc_gpfb_shrink) {
        remove_proc_entry("shrink", proc_gpfb_shrink);
        proc_gpfb_shrink = NULL;
    }
    if (proc_gpfb_separate) {
        remove_proc_entry("separate", proc_gpfb_separate);
        proc_gpfb_separate = NULL;
    }
    if (proc_gpfb_oneshot) {
        remove_proc_entry("oneshot", proc_gpfb_oneshot);
        proc_gpfb_oneshot = NULL;
    }
    if (proc_gpfb_halt) {
        remove_proc_entry("halt", proc_gpfb_halt);
        proc_gpfb_halt = NULL;
    }
    if (proc_gpfb_silent) {
        remove_proc_entry("silent", proc_gpfb_silent);
        proc_gpfb_silent = NULL;
    }
    if (proc_gpfb) {
        remove_proc_entry("gpfb", proc_gpfb);
        proc_gpfb = NULL;
    }
#endif
}

module_init(gpfb_init);
module_exit(gpfb_exit);

MODULE_LICENSE("GPL");
