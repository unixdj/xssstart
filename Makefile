PREFIX	?= /usr/local
BINDIR	?= $(PREFIX)/bin
MANDIR	?= $(PREFIX)/share/man

CFLAGS	+= -Wall -Wextra -Werror
LDLIBS	= -lX11 -lXss

PROG	= xssstart xsssuspend
OBJS	= $(PROG:=.o)
MANS	= $(PROG:=.1)
INTRO	= intro.0

ROFF	?= groff -Tps -mps -mdoc
NROFF	?= nroff -mdoc

all: $(PROG)

.PHONY: all install bininstall maninstall clean

install: bininstall maninstall

bininstall: all
	install -d $(DESTDIR)$(BINDIR)
	install $(PROG) $(DESTDIR)$(BINDIR)

maninstall:
	install -d $(DESTDIR)$(MANDIR)/man1
	install -m644 $(MANS) $(DESTDIR)$(MANDIR)/man1

.SUFFIXES: .1 .ps

.1.ps:
	$(ROFF) $< >$@

README: $(INTRO) $(MANS)
	( for i in $(INTRO) $(MANS) ; do \
		$(NROFF) $$i ; \
	done ) | colcrt - >$@

clean:
	-rm -f $(PROG) $(OBJS) core $(PROG:=.core)
