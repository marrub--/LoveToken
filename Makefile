CC=
LD=
MKDIR=mkdir -p
RM=rm
PCFLAGS=
PLFLAGS=
LIBNAME=
OUTDIR=bin
SRCDIR=src
LFLAGS=
CFLAGS=
RMEXTRA=
GDCCBUILD=OFF
EXAMPLEO=
EXAMPLEC=

ifeq ($(GDCCBUILD),ON)
	CC+=gdcc-cc
	LD+=gdcc-ld
	MKDIR+=mkdir
	LIBNAME+=$(OUTDIR)/LoveToken.bin
	PCFLAGS+=--bc-target=ZDoom -i$(SRCDIR)
	LFLAGS+=--bc-target=ZDoom
	EXAMPLEC+=examples/gdcc.c
	# These are completely arbitrary.
	EXAMPLEO+=$(OUTDIR)/libc.ir $(OUTDIR)/libGDCC.ir $(OUTDIR)/libGDCC-c.ir $(OUTDIR)/libGDCC-ZDACS-asm.ir
else
	EXAMPLEC+=examples/main.c
	PCFLAGS+=--std=c99 -g -ggdb -O2 -Wall -c -I$(SRCDIR)
	
	ifeq ($(OS),Windows_NT)
		CC+=mingw32-gcc
		LD+=mingw32-gcc
		PLFLAGS+=-shared -g -ggdb
		PLFLAGS2+=-Wl,--out-implib,bin/libLoveToken.a -liconv
		LIBNAME+=$(OUTDIR)/LoveToken.dll
		RMEXTRA+=bin/libLoveToken.a
	else
		ifeq ($(shell uname -s), Linux)
			CC+=gcc
			LD+=gcc
			PCFLAGS+=-fPIC
			PLFLAGS2+=-liconv
			LIBNAME+=$(OUTDIR)/LoveToken.so
		endif
	endif
endif

all:
	$(MKDIR) $(OUTDIR)
	$(CC) $(CFLAGS) $(PCFLAGS) -o $(OUTDIR)/lt.o $(SRCDIR)/lt.c
	$(LD) $(LFLAGS) $(PLFLAGS) -o $(LIBNAME) $(OUTDIR)/lt.o $(PLFLAGS2)

clean:
	$(RM) $(LIBNAME) $(OUTDIR)/lt.o $(RMEXTRA)

example: all
	$(CC) $(CFLAGS) $(PCFLAGS) -o $(OUTDIR)/example.o $(EXAMPLEC)
	$(LD) $(LFLAGS) -o $(OUTDIR)/example $(OUTDIR)/example.o $(EXAMPLEO) $(OUTDIR)/lt.o $(PLFLAGS2)
