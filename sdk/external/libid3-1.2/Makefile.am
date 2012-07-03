# Used to build Makefile.in

INCLUDES = -I$(top_builddir)/include
SUBDIRS = include lib src tests Docs

test: all
	cd tests; make test; cd ..

ltest: all 
	./tests/testapp output.txt

valgrind:
	cd tests; make valgrind; cd ..

