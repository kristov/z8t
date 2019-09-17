CC=gcc
CFLAGS=-Wall -Werror -ggdb
CCK_LOC=/home/ceade/src/personal/github/cck
Z80EXT_LOC=/home/ceade/build/z80ex

INCLUDE=-I. -I$(CCK_LOC) -I$(Z80EXT_LOC)/include

all: z8t.a

z8t.a: z8t.o $(CCK_LOC)/cck.o $(Z80EXT_LOC)/z80ex.o
	ar rcs $@ $^

z8t.o: z8t.c z8t.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f z8t.o z8t.a
