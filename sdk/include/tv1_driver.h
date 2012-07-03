#include <mach/gp_tv.h>

extern void tv1_init(void);
extern void tv1_start(signed int nTvStd, signed int nResolution, signed int nNonInterlace);
extern void tv1_buffer_set(BUFFER_COLOR_FORMAT buffer_color_mode);
extern void tv1_color_set(unsigned int buffer_ptr);
extern void tv1_reverse_set(signed int mode);
extern signed int tv1_buffer_ck(void);
extern void tv1_ypbpr_set(unsigned int mode,unsigned enable);
extern signed int tv1_irq_wait_poll(unsigned int Timeout_sec,unsigned int Timeout_usec);
