# sorry this makefile doesn't include anything cross-platform
# was hurriedly made to test it

all:
	mkdir -p bin
	mingw32-gcc --std=c99 -g -ggdb -c -o bin/lt.o lt.c
	mingw32-gcc -shared -g -ggdb -o bin/LoveToken.dll bin/lt.o -Wl,--out-implib,bin/libLoveToken.a
	# cp bin/LoveToken.dll test/LoveToken.dll
