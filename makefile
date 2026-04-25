.POSIX: # POSIX Makefile, use make,gmake,pdpmake,bmake
.SUFFIXES:
.PHONY: all clean install check

PROJECT    =scal
VERSION    =1.0.0
CC         =$(TARGET_PREFIX)cc
CFLAGS     = -Wall -g3 -std=c99
PREFIX     =/usr/local
TOOLCHAINS =aarch64-linux-musl x86_64-linux-musl x86_64-w64-mingw32
EXE        =$(HOMEDRIVE:C:=.exe)
DESTDIR    =$(HOMEDRIVE)
PROGS      =scal$(EXE)

all: $(PROGS)
clean:
	rm -f $(PROGS)
install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(PROGS) $(DESTDIR)$(PREFIX)/bin
	rm -f $(DESTDIR)$(PREFIX)/bin/cal; ln -s scal $(DESTDIR)$(PREFIX)/bin/cal
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/cal.1; ln -s scal.1 $(DESTDIR)$(PREFIX)/share/man/man1/cal.1
check:
scal$(EXE): scal.c
	$(CC) -o $@ scal.c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBS)
## -- BLOCK:man --
install: install-man
install-man:
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp ./scal.1 $(DESTDIR)$(PREFIX)/share/man/man1
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man5
	cp ./calendar.scal.5 $(DESTDIR)$(PREFIX)/share/man/man5
## -- BLOCK:man --
