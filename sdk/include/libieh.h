#ifndef _LIB_IEH_H_
#define _LIB_IEH_H_

#include "typedef.h"

UINT32 Hue_Correct_YUV( UINT8 *source, UINT8 *target, UINT32 height, UINT32 width, UINT32 bpl );
/*
UINT32 Hue_Correct_RGB( UINT8 *source, UINT8 *target, UINT32 height, UINT32 width, UINT32 bpl, UINT8 mode);
*/
UINT32 img_EDGE( UINT8 *source, UINT8 *target, UINT32 height, UINT32 width, UINT32 bpl );
void rgb2hsv( UINT8 *in_rgb, UINT32 *out_hsv  );
void yuv2rgb( UINT8 *in_yuv, UINT8 *out_rgb  );
void rgb2yuv( UINT8 *in_rgb, UINT8 *out_yuv  );

#endif /* _LIB_IEH_H_ */
