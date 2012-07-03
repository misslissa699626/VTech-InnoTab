#/**************************************************************************
# *                                                                        *
# *         Copyright (c) 2002 by Sunplus mMedia Technology Co., Ltd.      *
# *                                                                        *
# *  This software is copyrighted by and is the property of Sunplus        *
# *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
# *  Co., Ltd. This software may only be used in accordance with the       *
# *  corresponding license agreement. Any unauthorized use, duplication,   *
# *  distribution, or disclosure of this software is expressly forbidden.  *
# *                                                                        *
# *  This Copyright notice MUST not be removed or modified without prior   *
# *  written consent of Sunplus Technology Co., Ltd.                       *
# *                                                                        *
# *  Sunplus mMedia Technology Co., Ltd. reserves the right to modify this *
# *  software without notice.                                              *
# *                                                                        *
# *  Sunplus mMedia Technology Co., Ltd.                                   *
# *  19, Innovation First Road, Science-Based Industrial Park,             *
# *  Hsin-Chu, Taiwan, R.O.C.                                              *
# *                                                                        *
# *  Author: Roger Hsu                                                     *
# *                                                                        *
# **************************************************************************/
TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

GOAL     = libSDL

CFLAGS  += -I./include
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -I$(OUT_SDK_DIR)/include/libts
CFLAGS  += -g -O2 
CFLAGS  += -D_GNU_SOURCE=1 -fvisibility=hidden -D_REENTRANT -DHAVE_LINUX_VERSION_H

LDFLAGS += -lm -ldl -lpthread


ifeq ($(SYSCONFIG_SIMULATOR), y)
CFLAGS  += -DXTHREADS
endif

HDRS = SDL_config.h SDL.h SDL_active.h SDL_audio.h SDL_byteorder.h SDL_cdrom.h SDL_cpuinfo.h SDL_endian.h SDL_error.h SDL_events.h SDL_getenv.h SDL_joystick.h SDL_keyboard.h SDL_keysym.h SDL_loadso.h SDL_main.h SDL_mouse.h SDL_mutex.h SDL_name.h SDL_opengl.h SDL_platform.h SDL_quit.h SDL_rwops.h SDL_stdinc.h SDL_syswm.h SDL_thread.h SDL_timer.h SDL_types.h SDL_version.h SDL_video.h begin_code.h close_code.h


#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += \
	./src/SDL.o \
	./src/SDL_error.o \
	./src/SDL_fatal.o \
	\
	./src/stdlib/SDL_stdlib.o \
	./src/stdlib/SDL_string.o \
	./src/stdlib/SDL_malloc.o \
	./src/stdlib/SDL_qsort.o \
	./src/stdlib/SDL_getenv.o \
	./src/stdlib/SDL_iconv.o \
	\
	./src/thread/SDL_thread.o \
	./src/thread/pthread/SDL_systhread.o \
	./src/thread/pthread/SDL_sysmutex.o \
	./src/thread/pthread/SDL_syssem.o \
	./src/thread/pthread/SDL_syscond.o \
	\
	./src/timer/SDL_timer.o \
	./src/timer/unix/SDL_systimer.o \
	\
	./src/loadso/dlopen/SDL_sysloadso.o \
	./src/file/SDL_rwops.o \
	./src/cpuinfo/SDL_cpuinfo.o \
	\
	./src/video/SDL_RLEaccel.o \
	./src/video/SDL_cursor.o \
	./src/video/SDL_yuv_mmx.o \
	./src/video/SDL_pixels.o \
	./src/video/SDL_blit_A.o \
	./src/video/SDL_video.o \
	./src/video/SDL_stretch.o \
	./src/video/SDL_gamma.o \
	./src/video/SDL_yuv_sw.o \
	./src/video/SDL_surface.o \
	./src/video/SDL_blit.o \
	./src/video/SDL_blit_1.o \
	./src/video/SDL_blit_N.o \
	./src/video/SDL_blit_0.o \
	./src/video/SDL_yuv.o \
	./src/video/SDL_bmp.o \
	./src/video/dummy/SDL_nullvideo.o \
	./src/video/dummy/SDL_nullevents.o \
	./src/video/dummy/SDL_nullmouse.o \
	\
	./src/audio/SDL_audio.o \
	./src/audio/SDL_audiocvt.o \
	./src/audio/SDL_audiodev.o \
	./src/audio/SDL_mixer.o \
	./src/audio/SDL_wave.o \
	./src/audio/dummy/SDL_dummyaudio.o \
	./src/audio/dsp/SDL_dspaudio.o \
	./src/audio/dma/SDL_dmaaudio.o \
	\
	./src/events/SDL_active.o \
	./src/events/SDL_events.o \
	./src/events/SDL_quit.o \
	./src/events/SDL_mouse.o \
	./src/events/SDL_resize.o \
	./src/events/SDL_expose.o \
	./src/events/SDL_keyboard.o \
	\
	./src/joystick/dummy/SDL_sysjoystick.o \
	./src/joystick/linux/SDL_sysjoystick.o \
	./src/joystick/SDL_joystick.o \
	\
	./src/cdrom/SDL_cdrom.o \
	./src/cdrom/dummy/SDL_syscdrom.o

ifeq ($(SYSCONFIG_SIMULATOR), y)
OBJS += \
	./src/video/x11/SDL_x11video.o \
	./src/video/x11/SDL_x11events.o \
	./src/video/x11/SDL_x11mouse.o \
	./src/video/x11/SDL_x11image.o \
	./src/video/x11/SDL_x11gamma.o \
	./src/video/x11/SDL_x11modes.o \
	./src/video/x11/SDL_x11yuv.o \
	./src/video/x11/SDL_x11wm.o \
	./src/video/x11/SDL_x11gl.o \
	./src/video/x11/SDL_x11dyn.o \
	./src/video/x11/SDL_x11dga.o
else
OBJS += \
	./src/video/fbcon/SDL_fbvideo.o \
	./src/video/fbcon/SDL_fbevents.o \
	./src/video/fbcon/SDL_fbmouse.o \
	./src/video/fbcon/SDL_fb3dfx.o \
	./src/video/fbcon/SDL_fbmatrox.o \
	./src/video/fbcon/SDL_fbriva.o \
	./src/video/fbcon/SDL_fbelo.o
endif

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/SDL
	@for file in $(HDRS); do \
		cp -u ./include/$$file $(OUT_SDK_DIR)/include/SDL/$$file; \
	done
	
$(GOAL).a: $(OBJS)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL).a $(OBJS)
	$(MSG_GOAL_OK)

$(GOAL).so: $(OBJS)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL).so $(OBJS)
	$(MSG_GOAL_OK)

.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL).so
	$(RM) -f $(OBJS:.o=.d) $(OBJS) $(GOAL).a $(GOAL).so

.PHONY: release
release:

-include $(OBJS:.o=.d)
