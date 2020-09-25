LIBCOAP?=libcoap-2-gnutls

#definiera pkgconfig funktion (från shell-kommandot pkg-config)
pkgconfig=$(shell pkg-config $(1) $(2))

#använd output från pkg-config som C-kompilerings flagga, d.v.s. låt pkg-config säga vilken flagga som ska med
CPPFLAGS=-Wall -Wextra $(call pkgconfig,--cflags,$(LIBCOAP))

#samma som ovan fasst för länkaren
LDLIBS=$(call pkgconfig,--libs,$(LIBCOAP))
#LINK.o=$(LINK.cc)

#CXXFLAGS=-std=c++14

$(CC) = gcc

%.o:  %.c
	$(CC) -c -o $@ $<

all: client 
#-\nserver

client: CoAPclient.o resolveAdd.o
	$(CC) CoAPclient.o resolveAdd.o -o $@

#server:: server.o common.o
clean:
	rm -f *.o client server