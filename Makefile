CC=gcc
CFLAGS=-Wall -Werror -ggdb
Z80EXT_LOC=/home/ceade/build/z80ex

INCLUDE=-I. -I$(Z80EXT_LOC)/include

all: z8t

z8t: z8t.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $< $(Z80EXT_LOC)/z80ex.o

clean:
	rm -f z8t
