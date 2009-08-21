SOURCES = gtunet-gtk.c \
  gtunet.c\
	../des.c \
	../dot1x.c \
	../ethcard_bpf.c \
	../ethcard_eth.c \
	../ethcard_win.c \
	../logs.c \
	../md5.c \
	../mytunet.c \
	../mytunetsvc.c \
	../os.c \
	../os_posix.c \
	../os_win32.c \
	../setting.c \
	../tunet.c \
	../userconfig.c \
	../util.c

OBJS = $(subst .c,.o,$(SOURCES))

CC = gcc
CPP = g++

CFLAGS = -O2 -Wall -export-dynamic -D_POSIX -DPOSIX -D_LINUX
CFLAGS += `pkg-config --cflags gtk+-2.0 libglade-2.0 gmodule-export-2.0`
LDLIBS = `pkg-config --libs gtk+-2.0 libglade-2.0 gmodule-export-2.0`
LDFLAGS = -pthread -lm

all: gtunet

$(OBJS): common.h
gtunet: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f gtunet $(OBJS) *~ 
.PHONY: all clean
