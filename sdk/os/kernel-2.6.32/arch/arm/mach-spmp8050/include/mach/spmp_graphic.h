
#ifndef _SPMP_GRAPHIC_H_
#define _SPMP_GRAPHIC_H_

#include <mach/graphic_utils/graphics_util.h>

#define GRAPHIC_IOCTL_MAGIC 'g'

#define GRAPHIC_BITBLT_HW	_IOW(GRAPHIC_IOCTL_MAGIC, 1, unsigned int)
#define GRAPHIC_SPRITE_HW	_IOW(GRAPHIC_IOCTL_MAGIC, 2, unsigned int)

#endif //_SPMP_GRAPHIC_H_