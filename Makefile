PREFIX = ~/.local
VERSION = 0.3

PKG_CONFIG = pkg-config

# Comment out if JSON output support isn't needed
JSONLIBS = `$(PKG_CONFIG) --libs json-c`
JSONINCS = `$(PKG_CONFIG) --cflags json-c`
JSONFLAG = -DJSON

CURL_CONFIG = curl-config
SRC = minrss.c util.c net.c handlers.c
OBJ =  $(SRC:.c=.o)
INCS = `$(PKG_CONFIG) --cflags libxml-2.0` `$(CURL_CONFIG) --cflags` $(JSONINC)
LIBS = `$(PKG_CONFIG) --libs libxml-2.0` `$(CURL_CONFIG) --libs` $(JSONLIBS)
WARN = -Wall -Wpedantic -Wextra
CFLAGS = $(INCS) $(LIBS) $(WARN) -DVERSION=\"$(VERSION)\" $(JSONFLAG)

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
