/*
 * include/asm/gpfb.h
 *
 */

#ifndef _ASM_GPFB_H
#define _ASM_GPFB_H

#include <mach/hardware.h>

#ifdef CONFIG_ARCH_GPL32900

#if 1	/* T-DEBUG: for debug */
#define GPFB_HIBDRV_FLOATING
#define GPFB_HIBDRV_DEV_LOAD
#define GPFB_HIBDRV_OFFSET		0x00000000
#else
#define GPFB_HIBDRV_VIRT		0xfca00000
#define GPFB_HIBDRV_PHYS		0xb2000000
#define GPFB_HIBDRV_SIZE		0x00008000
#endif

#define GPFB_PFN_IS_NOSAVE

#define GPFB_BOOTFLAG_SIZE     0x00080000	/* block size */
#define GPFB_SNAPSHOT_NUM      2
/* dummy info */
#define GPFB_BOOTFLAG_AREA0		0x00000000	/* snapshot number */
#define GPFB_BOOTFLAG_SIZE0		0x00000000
#define GPFB_BOOTFLAG_AREA1		0x00000001	/* snapshot number */
#define GPFB_BOOTFLAG_SIZE1		0x00000000
#define GPFB_SNAPSHOT_AREA0		0x00000000	/* snapshot number */
#define GPFB_SNAPSHOT_SIZE0		0x00000000
#define GPFB_SNAPSHOT_AREA1		0x00000001	/* snapshot number */
#define GPFB_SNAPSHOT_SIZE1		0x00000000

#define GPFB_SAVEAREA                                      \
   /* Save area 0 */                                       \
{                                                          \
   GPFB_DEV_EXT,              /* Bootflag dev     */      \
   GPFB_BOOTFLAG_AREA0,        /* Bootflag area    */      \
   GPFB_BOOTFLAG_SIZE0,        /* Bootflag size    */      \
   GPFB_DEV_EXT,              /* Snapshot dev     */      \
   GPFB_SNAPSHOT_AREA0,        /* Snapshot area    */      \
   GPFB_SNAPSHOT_SIZE0,        /* Snapshot size    */      \
},                                                         \
   /* Save area 1 */                                       \
{                                                          \
   GPFB_DEV_EXT,              /* Bootflag dev     */      \
   GPFB_BOOTFLAG_AREA1,        /* Bootflag area    */      \
   GPFB_BOOTFLAG_SIZE1,        /* Bootflag size    */      \
   GPFB_DEV_EXT,              /* Snapshot dev     */      \
   GPFB_SNAPSHOT_AREA1,        /* Snapshot area    */      \
   GPFB_SNAPSHOT_SIZE1,        /* Snapshot size    */      \
},                                                         \

#define GPFB_CONSOLE   2
#define GPFB_BPS           115200

#define GPFB_MEMADDR(x)            __pa(x)

#endif /* CONFIG_ARCH_GPL32900 */

#endif

