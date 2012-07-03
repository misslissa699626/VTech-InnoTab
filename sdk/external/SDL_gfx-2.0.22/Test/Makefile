# Makefile to build the SDL tests

srcdir  = .
TOPDIR := ../../../../
include $(TOPDIR)sdk/build/core/config.mak

CC      = arm-none-linux-gnueabi-gcc
EXE	= 
LIBS	=  -L$(OUT_SDK_DIR)/lib -lSDL_gfx -lSDL -lpthread -lm -ldl -lts
CFLAGS  += -I$(OUT_SDK_DIR)/include/SDL
CFLAGS  += -I$(OUT_SDK_DIR)/include
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2 
CFLAGS  += -D_GNU_SOURCE=1 -D_REENTRANT

TARGETS = TestABGR$(EXE) TestFonts$(EXE) TestFramerate$(EXE) TestGfxBlit$(EXE) TestGfxPrimitives$(EXE) TestGfxTexture$(EXE) TestImageFilter$(EXE) TestRotozoom$(EXE) TestShrink$(EXE)

all: $(TARGETS)
TestABGR$(EXE): $(srcdir)/TestABGR.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestFonts$(EXE): $(srcdir)/TestFonts.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestFramerate$(EXE): $(srcdir)/TestFramerate.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestGfxBlit$(EXE): $(srcdir)/TestGfxBlit.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestGfxPrimitives$(EXE): $(srcdir)/TestGfxPrimitives.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestGfxTexture$(EXE): $(srcdir)/TestGfxTexture.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestImageFilter$(EXE): $(srcdir)/TestImageFilter.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestRotozoom$(EXE): $(srcdir)/TestRotozoom.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

TestShrink$(EXE): $(srcdir)/TestShrink.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

distclean: clean
	rm -f Makefile
	rm -f config.status config.cache config.log
	rm -rf $(srcdir)/autom4te*
