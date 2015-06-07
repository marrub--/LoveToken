CC=
MKDIR=
PCFLAGS=
PLFLAGS=
LIBNAME=
OUTDIR=bin
LFLAGS=-shared -g -ggdb
CFLAGS=--std=c99 -g -ggdb -O2 -Wall

ifeq ($(OS),Windows_NT)
	CC+=mingw32-gcc
	MKDIR+=mkdir -p
	PLFLAGS+=-Wl,--out-implib,bin/libLoveToken.a
	LIBNAME+=$(OUTDIR)/LoveToken.dll
else
	ifeq ($(shell uname -s), Linux)
		CC+=gcc
		MKDIR+=mkdir -p
		PCFLAGS+=-fPIC
		LIBNAME+=$(OUTDIR)/LoveToken.so
	endif
endif

all:
	$(MKDIR) $(OUTDIR)
	$(CC) $(CFLAGS) $(PCFLAGS) -c -o $(OUTDIR)/lt.o src/lt.c
	$(CC) $(LFLAGS) -o $(LIBNAME) $(OUTDIR)/lt.o $(PLFLAGS) -liconv
