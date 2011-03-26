CC=cc
LD=$(CC)
CFLAGS:= -c -Wall -D_FILE_OFFSET_BITS=64
LIBS=

PROJECT=pp

OBJS=main.o progress.o
VPATH=src

all: $(PROJECT)

$(PROJECT): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o "$(PROJECT)"

.c.o:
	$(CC) $(CFLAGS) src/$*.c

clean:
	rm -f *.o "$(PROJECT)"

install:
	install "$(PROJECT)" "$(PREFIX)/bin/$(PROJECT)"
