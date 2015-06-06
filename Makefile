CC=
MKDIR=
PCFLAGS=
PLFLAGS=
LIBNAME=
LFLAGS=-shared -g -ggdb
CFLAGS=--std=c99 -g -ggdb -Wall

ifeq ($(OS),Windows_NT)
	CC+=mingw32-gcc
	MKDIR+=mkdir -p
	PLFLAGS+=-Wl,--out-implib,bin/libLoveToken.a
	LIBNAME+=bin/LoveToken.dll
else
	ifeq ($(shell uname -s), Linux)
		CC+=gcc
		MKDIR+=mkdir -p
		PCFLAGS+=-fPIC
		LIBNAME+=bin/lovetoken.so
	endif
endif

all:
	$(MKDIR) bin
	$(CC) $(CFLAGS) $(PCFLAGS) -c -o bin/lt.o src/lt.c
	$(CC) $(LFLAGS) -o $(LIBNAME) bin/lt.o $(PLFLAGS) -liconv
