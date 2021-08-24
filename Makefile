PREFIX = ~/.local
VERSION = 0.1

# CC = cc
SRC = minrss.c util.c net.c xml.c
OBJ =  $(SRC:.c=.o)
PKG_CONFIG = pkg-config
CURL_CONFIG = curl-config
INCS = `$(PKG_CONFIG) --cflags libxml-2.0` `$(CURL_CONFIG) --cflags`
LIBS = `$(PKG_CONFIG) --libs libxml-2.0` `$(CURL_CONFIG) --libs`
WARN = -Wall -Wpedantic -Wextra
CFLAGS = $(INCS) $(LIBS) $(WARN) -DVERSION=\"$(VERSION)\"

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

install: CFLAGS += -O3
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f minrss $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/minrss

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/minrss
