SHELL = /bin/bash
CFLAGS = -O2 \
         -g \
         -Iwireless_tools \
         -Iliblcd \
         $(DEBUGFLAGS)
TECHNO = $(shell uname -m)

LDFLAGS = -ldl -lm -lpthread -lcurl

LIBSOURCES=$(shell echo *.c)

OBJECTSDIR=$(TECHNO).objects

OBJECTS=$(addprefix $(OBJECTSDIR)/, $(LIBSOURCES:.c=.o))

$(OBJECTSDIR)/%.o: %.c
	@$(CC) -c $(CFLAGS) $(LIBSINCLUDES) -MM -MT $(TECHNO).objects/$*.o $*.c > .deps/$*.dep
	$(CC) -c $(CFLAGS) $(LIBSINCLUDES) $*.c -o $(TECHNO).objects/$*.o

all: depsdir $(OBJECTSDIR) librewifi2

$(OBJECTSDIR):
	@mkdir -p $(OBJECTSDIR)

librewifi2: $(OBJECTS)
	cc $(OBJECTS) wireless_tools/libiw.a liblcd/liblcd.a $(LDFLAGS) -o librewifi2

clean:
	rm -f $(OBJECTSDIR)/*.o *.o *.a *.tar .deps/*.dep

depsdir:
	@mkdir -p .deps

install: librewifi2
	sudo service librewifi2 stop ; make ; sleep 5 ; cp librewifi2 /data/librewifi2/bin ; sudo service librewifi2 start

-include .deps/*.dep
