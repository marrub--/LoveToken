CC=
MKDIR=
PCFLAGS=
PLFLAGS=
LIBEXT=
LFLAGS=-shared -g -ggdb
CFLAGS=--std=c99 -g -ggdb -Wall

ifeq ($(OS),Windows_NT)
	CC+=mingw32-gcc
	MKDIR+=mkdir -p
	PLFLAGS+=-Wl,--out-implib,bin/libLoveToken.a
	LIBEXT+=dll
else
	ifeq ($(shell uname -s), Linux)
		CC+=gcc
		MKDIR+=mkdir -p
		PCFLAGS+=-fPIC
		LIBEXT+=so
	endif
endif

all:
	$(MKDIR) bin
	$(CC) $(CFLAGS) $(PCFLAGS) -c -o bin/lt.o src/lt.c
	$(CC) $(LFLAGS) -o bin/LoveToken.$(LIBEXT) bin/lt.o $(PLFLAGS) -liconv
