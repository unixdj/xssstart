PREFIX	?= /usr/local
BINDIR	?= $(PREFIX)/bin
MANDIR	?= $(PREFIX)/share/man

CFLAGS	+= -Wall -Wextra -Werror

LDLIBS	= -lX11 -lXss
PROG	= xssstart
OBJS	= $(PROG).o

all: $(PROG)

install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1
	install $(PROG) $(DESTDIR)$(BINDIR)
	install -m644 $(PROG).1 $(DESTDIR)$(MANDIR)/man1

clean:
	-rm -f $(PROG) $(OBJS) core $(PROG).core
