/*
 * Gpfb!! parameter define  Rev. 3.1.5
 *
 */

#ifndef _LINUX_GPFB_PARAM_H
#define _LINUX_GPFB_PARAM_H

#include <linux/gpfb.h>
#include <asm/gpfb.h>

#define GPFB_PARAM_VER_MAJOR    0x0002
#define GPFB_PARAM_VER_MINOR    0x0001

#ifdef CONFIG_MIPS
#define GPFB_HEADER(x)          ((x) * 0x08)
#else
#define GPFB_HEADER(x)          ((x) * 0x04)
#endif

#define GPFB_HEADER_ID          GPFB_HEADER(0)
#define GPFB_HEADER_COPY_SIZE   GPFB_HEADER(1)
#define GPFB_HEADER_DRV_SIZE    GPFB_HEADER(2)
#define GPFB_HEADER_SNAPSHOT    GPFB_HEADER(4)
#define GPFB_HEADER_HIBERNATE   GPFB_HEADER(5)
#define GPFB_HEADER_PREPROCESS  GPFB_HEADER(6)

#define GPFB_ID_DRIVER          0x44483357      /* W3HD */

#define GPFB_PART_SHIFT         0
#define GPFB_LUN_SHIFT          8
#define GPFB_DEV_SHIFT          16

#define GPFB_PART_MASK          (0xff << GPFB_PART_SHIFT)
#define GPFB_LUN_MASK           (0xff << GPFB_LUN_SHIFT)
#define GPFB_DEV_MASK           (0xff << GPFB_DEV_SHIFT)

#define GPFB_DEV_NOR            (0x01 << GPFB_DEV_SHIFT)
#define GPFB_DEV_NAND           (0x02 << GPFB_DEV_SHIFT)
#define GPFB_DEV_ATA            (0x03 << GPFB_DEV_SHIFT)
#define GPFB_DEV_SD             (0x04 << GPFB_DEV_SHIFT)
#define GPFB_DEV_MEM            (0x05 << GPFB_DEV_SHIFT)
#define GPFB_DEV_EXT            (0x7f << GPFB_DEV_SHIFT)

#ifdef GPFB_HIBDRV_FLOATING
#define GPFB_HIBDRV_VIRT        gpfb_hibdrv_buf
#endif


#ifndef __ASSEMBLY__

enum gpfb_progress {
    GPFB_PROGRESS_INIT,
    GPFB_PROGRESS_SYNC,
    GPFB_PROGRESS_FREEZE,
    GPFB_PROGRESS_SHRINK,
    GPFB_PROGRESS_SUSPEND,
    GPFB_PROGRESS_SAVE,
    GPFB_PROGRESS_SAVEEND,
    GPFB_PROGRESS_RESUME,
    GPFB_PROGRESS_THAW,
    GPFB_PROGRESS_EXIT,
    GPFB_PROGRESS_CANCEL,
};

struct gpfb_savetbl {
    unsigned long start;
    unsigned long end;
};

struct gpfb {
    unsigned short ver_major;
    unsigned short ver_minor;
    int mode;
    int compress;
    int oneshot;
    int halt;
    unsigned long bootflag_dev;
    unsigned long bootflag_area;
    unsigned long bootflag_size;
    unsigned long snapshot_dev;
    unsigned long snapshot_area;
    unsigned long snapshot_size;
    unsigned long v2p_offset;
    unsigned long lowmem_size;
    int page_shift;
    int zonetbl_num;
    int exttbl_num;
    int dramtbl_num;
    int preload_exttbl;
    struct gpfb_savetbl *zonetbl;
    struct gpfb_savetbl *exttbl;
    struct gpfb_savetbl *dramtbl;
    unsigned long maxarea;
    unsigned long maxsize;
    unsigned long lowmem_maxarea;
    unsigned long lowmem_maxsize;
    int console;
    int bps;
    int silent;
    unsigned long private;
    unsigned long snapshot_ver;
    int reserve[6];
    int stat;
    int retry;
};

struct gpfb_savearea {
    unsigned long bootflag_dev;
    unsigned long bootflag_area;
    unsigned long bootflag_size;
    unsigned long snapshot_dev;
    unsigned long snapshot_area;
    unsigned long snapshot_size;
};

struct gpfb_ops {
    int (*drv_load)(void *buf, size_t size);
    int (*drv_init)(void);
    int (*device_suspend_early)(void);
    int (*device_suspend)(void);
    int (*pre_snapshot)(void);
    int (*snapshot)(void);
    void (*post_snapshot)(void);
    void (*device_resume)(void);
    void (*device_resume_late)(void);
    void (*drv_uninit)(void);
    void (*putc)(char c);
    void (*progress)(int val);
};

int swsusp_page_is_saveable(struct zone *zone, unsigned long pfn);

int hibdrv_snapshot(void);
int gpfb_register_machine(struct gpfb_ops *ops);
int gpfb_unregister_machine(struct gpfb_ops *ops);

#ifdef CONFIG_MTD
int gpfb_mtd_load(int mtdno, void *buf, size_t size);
int gpfb_mtd_load_nm(const char *mtdname, void *buf, size_t size);
#endif

#ifdef GPFB_HIBDRV_DEV_LOAD
int gpfb_dev_load(const char *dev, void *buf, size_t size);
#endif

extern struct gpfb gpfb_param;

extern unsigned char *gpfb_hibdrv_buf;

#define GPFB_ID         (*(unsigned long *)(GPFB_HIBDRV_VIRT + GPFB_HEADER_ID))
#define GPFB_DRV_COPY_SIZE \
    (*(unsigned long *)(GPFB_HIBDRV_VIRT + GPFB_HEADER_COPY_SIZE))
#define GPFB_DRV_SNAPSHOT(x) \
    ((int (*)(void *))(GPFB_HIBDRV_VIRT + GPFB_HEADER_SNAPSHOT))(x)

#endif  /* __ASSEMBLY__ */

#endif  /* _LINUX_GPFB_PARAM_H */
