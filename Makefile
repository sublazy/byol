CFLAGS = -std=c99 -Wall

all: parsing

parsing: parsing.c build-dir
	@cc $(CLFAGS) -ledit parsing.c -o build/parsing

build-dir:
	@mkdir -p build

clean:
	@rm -rf build

.PHONY: all clean parsing
