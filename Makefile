CC := clang
CFLAGS := -Wall -Weverything -pedantic-errors -Wno-missing-prototypes -Wno-cast-align

all: sntp

sntp:
	${CC} ${CFLAGS} -o sntp src/*.c

run: sntp
	./sntp

clean:
	rm -f src/*.o
	rm -f sntp
