CC := gcc
PKG_CONFIG := pkg-config

CFLAGS = -Wall -g `$(PKG_CONFIG) --cflags glib-2.0 gtk+-2.0`
LIBS += `$(PKG_CONFIG) --libs glib-2.0 gtk+-2.0`

APPNAME := rinks
PREFIX := /usr

rinks_SRC := $(wildcard *.c)
rinks_OBJ := $(rinks_SRC:.c=.o)
rinks_HEADERS := $(wildcard *.h)

all: $(APPNAME)

$(APPNAME): $(rinks_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

%.o: %.c $(rinks_HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(APPNAME)
	install $(APPNAME) $(PREFIX)/bin

clean:
	rm -f $(APPNAME) $(rinks_OBJ)

.PHONY: all clean install
