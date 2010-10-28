# vim:ts=8

CC	= cc
CFLAGS	= -O2 -Wall -Wunused -Wmissing-prototypes -Wstrict-prototypes
INCLUDES= -I/usr/X11R6/include

LIBS	= -lX11 -lXmu
LDPATH	= -L/usr/X11R6/lib

PREFIX	= /usr/local
BINDIR	= $(DESTDIR)$(PREFIX)/bin

INSTALL_PROGRAM = install -s

PROG	= vmwh
OBJS	= vmware.o vmwh.o x11.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): *.o: *.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: all
	$(INSTALL_PROGRAM) $(PROG) $(BINDIR)

clean:
	rm -f $(PROG) $(OBJS) 

.PHONY: all install clean
