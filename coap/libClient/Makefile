LIBCOAP?=libcoap-2-gnutls

#definiera pkgconfig funktion (från shell-kommandot pkg-config)
pkgconfig=$(shell pkg-config $(1) $(2))

#använd output från pkg-config som C-kompilerings flagga, d.v.s. låt pkg-config säga vilken flagga som ska med
cflags=-Wall -Wextra $(call pkgconfig,--cflags,$(LIBCOAP))

#samma som ovan fast för länkaren
biblio=$(call pkgconfig,--libs,$(LIBCOAP))
#LDFLAGS=$(call pkgconfig,--libs,$(LIBCOAP))
LDFLAGS = $(filter -L%, $(biblio))
LDLIBS = $(filter -l%, $(biblio))
#LINK.o=$(LINK.cc)

#CXXFLAGS=-std=c++14

$(CC) = gcc

%.o:  %.c
	$(CC) -c -o $@ $< $(cflags)

all: client 
#-\nserver

client: CoAPclient.o resolveAdd.o
	$(info  biblio = $(biblio), flags = $(LDFLAGS), libs = $(LDLIBS))
	$(CC) CoAPclient.o resolveAdd.o -o $@ $(LDFLAGS) $(LDLIBS)

#server:: server.o common.o
clean:
	rm -f *.o client server