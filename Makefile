CC := clang
CFLAGS := -std=c11 -Wall -Wextra -pedantic-errors -D _XOPEN_SOURCE=700

all: sntp

sntp:
	${CC} ${CFLAGS} -o sntp src/*.c

run: sntp
	./sntp

clean:
	rm -f src/*.o
	rm -f sntp
