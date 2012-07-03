# Used to build Makefile.in

EXTRA_DIST = list.h
INCLUDES = -I$(top_builddir)/include
LDADDS = ../lib/libid3.la

noinst_PROGRAMS = id3test
id3test_SOURCES = test.c
id3test_LDADD = $(LDADDS)

record:
	./id3test > output.res

test: id3test
	./id3test > output.cmp
	diff output.res output.cmp

valgrind:
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes id3test
