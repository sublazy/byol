#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

int main(int argc, char** argv) {

        puts("Lispy version 0.1.0");
        puts("Press Ctrl+C to exit\n");

        while (1) {
                char *usr_input = readline("> ");
                add_history(usr_input);
                printf("You entered: %s\n", usr_input);
                free(usr_input);
        }

        return 0;
}
