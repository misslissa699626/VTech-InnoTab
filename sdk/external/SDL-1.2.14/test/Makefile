# Makefile to build the SDL tests

srcdir  = .
TOPDIR := ../../../../
include $(TOPDIR)sdk/build/core/config.mak

CC      = arm-none-linux-gnueabi-gcc
EXE	= 
LIBS	=  -L$(OUT_SDK_DIR)/lib -lSDL -lpthread -lts
CFLAGS  += -I$(OUT_SDK_DIR)/include/SDL
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -I$(OUT_SDK_DIR)/include/libts
CFLAGS  += -g -O2 
CFLAGS  += -D_GNU_SOURCE=1 -fvisibility=hidden -D_REENTRANT -DHAVE_LINUX_VERSION_H


TARGETS = checkkeys$(EXE) graywin$(EXE) loopwave$(EXE) testalpha$(EXE) testbitmap$(EXE) testblitspeed$(EXE) testcursor$(EXE) testdyngl$(EXE) testerror$(EXE) testfile$(EXE) testgamma$(EXE) testhread$(EXE) testiconv$(EXE) testkeys$(EXE) testlock$(EXE) testoverlay2$(EXE) testoverlay$(EXE) testpalette$(EXE) testplatform$(EXE) testsem$(EXE) testsprite$(EXE) testtimer$(EXE) testver$(EXE) testvidinfo$(EXE) testwin$(EXE) testwm$(EXE) threadwin$(EXE) torturethread$(EXE) testloadso$(EXE)

all: $(TARGETS)
checkkeys$(EXE): $(srcdir)/checkkeys.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

graywin$(EXE): $(srcdir)/graywin.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

loopwave$(EXE): $(srcdir)/loopwave.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testalpha$(EXE): $(srcdir)/testalpha.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS) -lm

testbitmap$(EXE): $(srcdir)/testbitmap.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testblitspeed$(EXE): $(srcdir)/testblitspeed.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testcursor$(EXE): $(srcdir)/testcursor.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testdyngl$(EXE): $(srcdir)/testdyngl.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testerror$(EXE): $(srcdir)/testerror.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testfile$(EXE): $(srcdir)/testfile.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testgamma$(EXE): $(srcdir)/testgamma.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS) -lm

testhread$(EXE): $(srcdir)/testhread.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testiconv$(EXE): $(srcdir)/testiconv.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testkeys$(EXE): $(srcdir)/testkeys.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testlock$(EXE): $(srcdir)/testlock.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testoverlay2$(EXE): $(srcdir)/testoverlay2.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testoverlay$(EXE): $(srcdir)/testoverlay.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testpalette$(EXE): $(srcdir)/testpalette.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS) -lm

testplatform$(EXE): $(srcdir)/testplatform.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testsem$(EXE): $(srcdir)/testsem.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testsprite$(EXE): $(srcdir)/testsprite.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS) -lm

testtimer$(EXE): $(srcdir)/testtimer.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testver$(EXE): $(srcdir)/testver.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testvidinfo$(EXE): $(srcdir)/testvidinfo.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testwin$(EXE): $(srcdir)/testwin.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testwm$(EXE): $(srcdir)/testwm.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

threadwin$(EXE): $(srcdir)/threadwin.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

torturethread$(EXE): $(srcdir)/torturethread.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

testloadso$(EXE): $(srcdir)/testloadso.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)


clean:
	rm -f $(TARGETS)

distclean: clean
	rm -f Makefile
	rm -f config.status config.cache config.log
	rm -rf $(srcdir)/autom4te*
