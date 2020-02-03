CC=gcc
CFLAGS=-Wall -Werror -ggdb

INCLUDE=-I. -Iz80ex/include

all: z8t

z8t: z8t.c z80ex/z80ex.o
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $< z80ex/z80ex.o

z80ex/z80ex.o:
	cd z80ex/ && mkdir -p lib && make

clean:
	rm -f z8t
	cd z80ex/ && make clean
