# Used to build Makefile.in

INCLUDES = -I$(top_builddir)/include -I$(srcdir)/../include
LDADDS = ../lib/libid3.la

bin_PROGRAMS = tagpuller
tagpuller_SOURCES = tagpuller.c
tagpuller_LDADD = $(LDADDS)
