#include <stdio.h>

#define BUF_SIZE 2048

static char input[BUF_SIZE];

int main(int argc, char** argv) {

        puts("Lispy version 0.0.1");
        puts("Press Ctrl+C to exit");

        while (1) {
                fputs("\nlispy> ", stdout);
                fgets(input, BUF_SIZE, stdin);
                printf("You entered: %s", input);
        }

        return 0;
}
