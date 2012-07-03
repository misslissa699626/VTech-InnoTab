# Source-level makefile for btplay
# Note that configuration is specified via scons config files,
# and this project requires scons 2.0.1 or later
#

BUILD=release

BTTARGET=arm-unknown-linux
GCC_CROSS_PREFIX=arm-none-linux-gnueabi-

ifeq ($(pc), 1)
BTTARGET=x86-unknown-linux
GCC_CROSS_PREFIX=
endif

BTBUILD=$(BUILD)
ifeq ($(BUILD),release)
  BTBUILD=Release
else
  BTBUILD=Debug
endif

FOR_GPROF=0
ifeq ($(FOR_GPROF),1)
  PROFOPTS= enable_profiling=True
else
  PROFOPTS=
endif

EXEDIR= ./output/$(BTTARGET)/$(BUILD)/bin
LIBDIR= ./output/$(BTTARGET)/$(BUILD)/lib
PLUGINDIR = ./output/$(BTTARGET)/$(BUILD)/plugin
INCDIR= ./output/include
OUTPUT_EXES=
OUTPUT_LIBS=


STRIP= $(GCC_CROSS_PREFIX)strip

all: $(OUTPUT_EXES) $(OUTPUT_LIBS)
	scons -C BlueTune target=$(BTTARGET) build_config=$(BTBUILD) $(PROFOPTS)
	-mkdir -p $(LIBDIR)
	cp -p BlueTune/Build/Targets/$(BTTARGET)/$(BTBUILD)/*.a $(LIBDIR)
	#cp -p BlueTune/ThirdParty/*/Targets/$(BTTARGET)/lib/*.a $(LIBDIR)
	-mkdir -p $(EXEDIR)
	cp -p BlueTune/Build/Targets/$(BTTARGET)/$(BTBUILD)/btplay $(EXEDIR)
	cp -p BlueTune/Build/Targets/$(BTTARGET)/$(BTBUILD)/btplaypipe $(EXEDIR)
	$(STRIP) $(EXEDIR)/btplay
	cp -p BlueTune/Build/Targets/$(BTTARGET)/$(BTBUILD)/btplay $(EXEDIR)/btplay.debug
	-mkdir -p $(PLUGINDIR)
	-cp -p BlueTune/Build/Targets/$(BTTARGET)/$(BTBUILD)/*Decoder.plugin $(PLUGINDIR)
	./bthcopy $(INCDIR) BlueTune BlueTune Core Decoder Player 
	./bthcopy $(INCDIR) Atomix Core
	./bthcopy $(INCDIR) Neptune Core
	@echo "Build btplay done!"

.PHONY: clean
clean:
	-rm -rf $(LIBDIR) $(EXEDIR) $(PLUGINDIR)
	scons -C BlueTune -c target=$(BTTARGET) build_config=$(BTBUILD)

