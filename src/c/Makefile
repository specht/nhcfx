nhcfx: nhcfx.o /raw/cache/tinyexpr.o
	gcc -o nhcfx nhcfx.o /raw/cache/tinyexpr.o -lm

nhcfx.o: nhcfx.c
	gcc -c -O3 nhcfx.c

/raw/cache/tinyexpr.o: /src/3rd_party/tinyexpr/tinyexpr.c
	gcc -c -O3 -o /raw/cache/tinyexpr.o /src/3rd_party/tinyexpr/tinyexpr.c
