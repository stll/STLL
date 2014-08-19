# make PREFIX=/some/where if you've got custom HarfBuzz installed somewhere

PREFIX?=/usr
export PKG_CONFIG_PATH:=$(PREFIX)/lib/pkgconfig
CC=g++
CXXFLAGS=-W -Wall -Wextra -pedantic -std=c++11 -g -O3\
    `pkg-config sdl --cflags` -DHAVE_SDL\
    `pkg-config freetype2 --cflags` \
    `pkg-config harfbuzz --cflags` \
    `pkg-config glib-2.0 --cflags` \
    `pkg-config fribidi --cflags`

LIBS=\
    `pkg-config sdl --libs` \
    `pkg-config freetype2 --libs` \
    `pkg-config harfbuzz --libs` \
    `pkg-config glib-2.0 --libs` \
    `pkg-config fribidi --libs` \
    -llinebreak -lpugixml

#    hb/harfbuzz-0.9.28/src/.libs/libharfbuzz.a \

# so as not to have to use LD_LIBRARY_PATH when prefix is custom
LDFLAGS=-Wl,-rpath -Wl,$(PREFIX)/lib

all: test

test: main.o layouter.o layouterSDL.o layouterFont.o utf-8.o layouterCSS.o layouterXHTML.o
	$(CC) -o test main.o layouter.o layouterSDL.o layouterFont.o utf-8.o layouterCSS.o layouterXHTML.o $(LIBS)

clean:
	rm -f ex-sdl-freetype-harfbuzz
	rm *.o
	rm test
