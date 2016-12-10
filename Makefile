CC = cc

CFLAGS = -std=c99 -Wall

all: hello

hello: hello.c
	@cc $(CLFAGS) hello.c -o hello

clean:
	@rm -rf hello

phony: all clean hello
