SHELL = /bin/bash
CFLAGS = -O2 \
         -g \
         $(DEBUGFLAGS)
TECHNO = $(shell uname -m)
LDFLAGS = 

LIBSOURCES=$(shell echo *.c)

OBJECTSDIR=$(TECHNO).objects

OBJECTS=$(addprefix $(OBJECTSDIR)/, $(LIBSOURCES:.c=.o))

$(OBJECTSDIR)/%.o: %.c
	@$(CC) -c $(CFLAGS) $(LIBSINCLUDES) -MM -MT $(TECHNO).objects/$*.o $*.c > .deps/$*.dep
	$(CC) -c $(CFLAGS) $(LIBSINCLUDES) $*.c -o $(TECHNO).objects/$*.o

all: depsdir $(OBJECTSDIR) liblcd.a includelcd.tar

$(OBJECTSDIR):
	@mkdir -p $(OBJECTSDIR)

liblcd.a: $(OBJECTS)
	ar -rcs liblcd.a $(OBJECTS)
	ranlib liblcd.a

includelcd.tar: *.h
	tar cf includelcd.tar *.h

clean:
	rm -f $(OBJECTSDIR)/*.o *.o *.a *.tar .deps/*.dep

depsdir:
	@mkdir -p .deps

-include .deps/*.dep
