CC=
MKDIR=
RM=
PCFLAGS=
PLFLAGS=
LIBNAME=
OUTDIR=bin
LFLAGS=-shared -g -ggdb
CFLAGS=--std=c99 -g -ggdb -O2 -Wall
RMEXTRA=
EXAMPLEBIN=

ifeq ($(OS),Windows_NT)
	CC+=mingw32-gcc
	MKDIR+=mkdir -p
	RM+=rm
	PLFLAGS+=-Wl,--out-implib,bin/libLoveToken.a
	LIBNAME+=$(OUTDIR)/LoveToken.dll
	RMEXTRA+=bin/libLoveToken.a
else
	ifeq ($(shell uname -s), Linux)
		CC+=gcc
		MKDIR+=mkdir -p
		RM+=rm
		PCFLAGS+=-fPIC
		LIBNAME+=$(OUTDIR)/LoveToken.so
	endif
endif

all:
	$(MKDIR) $(OUTDIR)
	$(CC) $(CFLAGS) $(PCFLAGS) -c -o $(OUTDIR)/lt.o src/lt.c
	$(CC) $(LFLAGS) -o $(LIBNAME) $(OUTDIR)/lt.o $(PLFLAGS) -liconv

clean:
	$(RM) $(LIBNAME) bin/lt.o $(RMEXTRA)

example:
	$(CC) $(CFLAGS) -Isrc -Lbin -o bin/example examples/main.c -lLoveToken
