PREFIX	?= /usr/local
BINDIR	?= $(PREFIX)/bin
MANDIR	?= $(PREFIX)/share/man

CFLAGS	+= -Wall -Wextra -Werror

LDLIBS	= -lX11 -lXss
PROG	= xssstart xsssuspend
OBJS	= $(PROG:=.o)
MANS	= $(PROG:=.1)

all: $(PROG)

install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
	install $(PROG) $(DESTDIR)$(BINDIR)
	install -m644 $(MANS) $(DESTDIR)$(MANDIR)/man1

clean:
	-rm -f $(PROG) $(OBJS) core $(PROG:=.core)
