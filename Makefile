CC := clang
CFLAGS := -std=c11 -Wall -Weverything -pedantic-errors -Wno-missing-prototypes -Wno-cast-align \
			-D _XOPEN_SOURCE=700

all: sntp

sntp:
	${CC} ${CFLAGS} -o sntp src/*.c

run: sntp
	./sntp

clean:
	rm -f src/*.o
	rm -f sntp
