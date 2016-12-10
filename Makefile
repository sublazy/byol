CC = cc

CFLAGS = -std=c99 -Wall

all: hello prompt

prompt: prompt.c
	@cc $(CLFAGS) prompt.c -o prompt

hello: hello.c
	@cc $(CLFAGS) hello.c -o hello

clean:
	@rm -rf hello

phony: all clean hello prompt
