PREFIX = /tmp/test
NAME = libmqtt_plugin
LD = ld
CC = cc
PKG_CONFIG = pkg-config
INSTALL = install
CFLAGS = -g -O2 -Wall -Wextra
LDFLAGS =
LIBS =-lpaho-mqtt3c
VLC_PLUGIN_CFLAGS := $(shell $(PKG_CONFIG) --cflags vlc-plugin)
VLC_PLUGIN_LIBS := $(shell $(PKG_CONFIG) --libs vlc-plugin)

libdir = $(PREFIX)/lib
plugindir = $(libdir)/vlc/plugins

override CC += -std=gnu99
override CPPFLAGS += -DPIC -I. -Isrc
override CFLAGS += -fPIC
override LDFLAGS += -Wl,-no-undefined,-z,defs

override CPPFLAGS += -DMODULE_STRING=\"foo\"
override CFLAGS += $(VLC_PLUGIN_CFLAGS)
override LIBS += $(VLC_PLUGIN_LIBS)

TARGETS = $(NAME).so

all: $(NAME).so

install: all
	mkdir -p -- $(DESTDIR)$(plugindir)/misc
	$(INSTALL) --mode 0755 $(NAME).so $(DESTDIR)$(plugindir)/misc

install-strip:
	$(MAKE) install INSTALL="$(INSTALL) -s"

uninstall:
	rm -f $(plugindir)/misc/$(NAME).so

clean:
	rm -f -- $(NAME).so src/*.o

mostlyclean: clean

SOURCES = mqtt.c

#$(SOURCES:%.c=src/%.o): %: src/foo.h

$(NAME).so: $(SOURCES:%.c=%.o)
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)

.PHONY: all install install-strip uninstall clean mostlyclean