# make PREFIX=/some/where if you've got custom HarfBuzz installed somewhere

PREFIX?=/usr
export PKG_CONFIG_PATH:=$(PREFIX)/lib/pkgconfig
CC=g++
CXXFLAGS=-W -Wall -Wextra -pedantic -std=c++11 -g\
    `pkg-config sdl --cflags` -DHAVE_SDL\
    `pkg-config freetype2 --cflags` \
    `pkg-config harfbuzz --cflags` \
    `pkg-config glib-2.0 --cflags` \
    `pkg-config fribidi --cflags` \
    -fprofile-arcs -ftest-coverage

LIBS=\
    `pkg-config sdl --libs` \
    `pkg-config freetype2 --libs` \
    `pkg-config harfbuzz --libs` \
    `pkg-config glib-2.0 --libs` \
    `pkg-config fribidi --libs` \
    -llinebreak -lpugixml \
    -fprofile-arcs -ftest-coverage

#    hb/harfbuzz-0.9.28/src/.libs/libharfbuzz.a \

# so as not to have to use LD_LIBRARY_PATH when prefix is custom
LDFLAGS=-Wl,-rpath -Wl,$(PREFIX)/lib

all: test main

main: main.o layouter.o layouterSDL.o layouterFont.o utf-8.o layouterCSS.o layouterXHTML.o
	$(CC) -o main main.o layouter.o layouterSDL.o layouterFont.o utf-8.o layouterCSS.o layouterXHTML.o $(LIBS)

test: test.o layouter.o layouterSDL.o layouterFont.o utf-8.o layouterCSS.o layouterXHTML.o
	$(CC) -o test test.o layouter.o layouterSDL.o layouterFont.o utf-8.o layouterCSS.o layouterXHTML.o $(LIBS) -lboost_unit_test_framework
	rm -f *.gcov
	rm -f *.gcda
	./test
	~/Programme/lcov/bin/lcov -q --capture --no-external --directory . --output-file coverage.info
	~/Programme/lcov/bin/genhtml coverage.info --output-directory coverage -q
	rm -f *.gcov
	rm -f *.gcda
	rm -f coverage.info

clean:
	rm -f ex-sdl-freetype-harfbuzz
	rm -f *.o
	rm -f test main
	rm -f *.gcov
	rm -f *.gcda
	rm -f *.gcno
	rm -f coverage.info
