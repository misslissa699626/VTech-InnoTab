TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak


ifeq ($(SYSCONFIG_TARGET), linux-arm)
CFLAGS += -DFPM_ARM
else
CFLAGS = -DFPM_INTEL -g
#CFLAGS = -DFPM_FLOAT -g
endif

CFLAGS += -I$(SDK_DIR)/include
CFLAGS += -I$(PRODUCT_DIR)


OBJS = bit.o decoder.o fixed.o frame.o huffman.o layer12.o layer3.o stream.o synth.o timer.o version.o


mp3dec : $(OBJS) minimad.o
	$(TARGET_CC) $(OBJS) minimad.o -o mp3dec

all: mp3dec
	@echo OK !

.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -f $(OBJS:.o=.d) $(OBJS) minimad.o mp3dec
