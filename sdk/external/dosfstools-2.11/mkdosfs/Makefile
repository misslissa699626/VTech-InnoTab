CC = arm-none-linux-gnueabi-gcc 
#CC = /mips/3.3el/bin/mipsel-linux-gcc
OBJECTS = mkdosfs.o

all: mkdosfs

mkdosfs: $(OBJECTS)
	$(CC) -O2 -Wl,-elf2flt="-z -s 8192" $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) -O2 $(CFLAGS) -c $< -o $@

install: mkdosfs
	mkdir -p $(SBINDIR) $(MANDIR)
	install -m 755 mkdosfs $(SBINDIR)
	install -m 644 mkdosfs.8 $(MANDIR)
	rm -f $(SBINDIR)/mkfs.msdos
	rm -f $(SBINDIR)/mkfs.vfat
	ln -s mkdosfs $(SBINDIR)/mkfs.msdos
	ln -s mkdosfs $(SBINDIR)/mkfs.vfat
	rm -f $(MANDIR)/mkfs.msdos.8
	ln -s mkdosfs.8 $(MANDIR)/mkfs.msdos.8
	ln -s mkdosfs.8 $(MANDIR)/mkfs.vfat.8

clean :
	echo *.o *.i *.s *~ \#*# core .#* .new*
	rm -f *.o *.i *.s *~ \#*# core .#* .new*

distclean: clean
	rm -f mkdosfs *.a *# *.orig *.rej TAGS

dep:
