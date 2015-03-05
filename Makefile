IDIR =.
ifeq ($(CC),)
CC=gcc
endif
CFLAGS=-g -I$(IDIR)

ODIR=.
LDIR =.

LIBS=

_DEPS = ring-buffer.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = read-mote.o ring-buffer.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

read-mote: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
	
