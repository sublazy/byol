CC = cc

CFLAGS = -std=c99 -Wall

all: hello prompt

prompt: prompt.c build-dir
	@cc $(CLFAGS) -ledit prompt.c -o build/prompt

hello: hello.c build-dir
	@cc $(CLFAGS) hello.c -o build/hello

build-dir:
	@mkdir -p build

clean:
	@rm -rf build

phony: all clean hello prompt
