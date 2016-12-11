CC = cc

CFLAGS = -std=c99 -Wall

all: hello parsing

parsing: parsing.c build-dir
	@cc $(CLFAGS) -ledit parsing.c -o build/parsing

hello: hello.c build-dir
	@cc $(CLFAGS) hello.c -o build/hello

build-dir:
	@mkdir -p build

clean:
	@rm -rf build

phony: all clean hello parsing
