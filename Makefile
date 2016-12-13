CFLAGS = -std=c99 -Wall -g

LINK_LIBS = -ledit -lm

all: parsing

parsing:parsing.c mpc.c mpc.h build-dir
	@cc $(CFLAGS) parsing.c mpc.c $(LINK_LIBS) -o build/parsing

build-dir:
	@mkdir -p build

clean:
	@rm -rf build

.PHONY: all clean parsing
