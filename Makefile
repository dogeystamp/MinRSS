SRC = minrss.c util.c net.c xml.c
OBJ =  $(SRC:.c=.o)
CC = cc
INCS =
LIBS = -lcurl -I/usr/include/libxml2 -lxml2 -lm
WARN = -Wall -Wpedantic -Wextra
CFLAGS = $(LIBS) $(INCS) $(WARN)

PREFIX = /usr/local

all: config.h minrss

debug: CFLAGS += -g3
debug: config.h minrss 

config.h:
	cp config.def.h config.h

.c.o:
	$(CC) $(CFLAGS) -c $<

minrss: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -f minrss $(OBJ)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f minrss $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/minrss

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/minrss
