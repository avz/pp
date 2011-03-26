CC?=cc
LD=$(CC)

PROJECT=pp

OBJS=main.o progress.o
VPATH=src

all: $(PROJECT)

$(PROJECT): $(OBJS)
	$(LD) -lc $(LDFLAGS) $(OBJS) -o "$(PROJECT)"

.c.o:
	$(CC) -c -Wall -D_FILE_OFFSET_BITS=64 $(CFLAGS) src/$*.c

clean:
	rm -f *.o "$(PROJECT)"

install:
	install "$(PROJECT)" "$(PREFIX)/bin/$(PROJECT)"
