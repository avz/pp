CC?=cc
LD=$(CC)

PROJECT=pp

OBJS=main.o progress.o
VPATH=src

CFLAGS?=-O2

build: $(PROJECT)

$(PROJECT): $(OBJS)
	$(LD) -lc $(LDFLAGS) $(OBJS) -o "$(PROJECT)"

.c.o:
	$(CC) -c -Wall -D_FILE_OFFSET_BITS=64 $(CFLAGS) src/$*.c

clean:
	rm -f *.o "$(PROJECT)"

install: build
	install "$(PROJECT)" "$(PREFIX)/bin"

test: build
	sh tests/all.sh
