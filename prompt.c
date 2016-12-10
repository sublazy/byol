#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

#define INPUT_BUF_SIZE 2048
static char buffer[INPUT_BUF_SIZE];

char *readline (char *prompt)
{
        fputs(prompt, stdout);
        fgets(buffer, INPUT_BUF_SIZE, stdin);

        char *usr_input_s = malloc(strlen(buffer) + 1);
        strcpy(usr_input_s, buffer);
        usr_input_s[strlen(cpy - 1)] = '\0';

        return usr_input_s;
}

// stub for WIN32
void add_history(char* unused) {}

#else // _WIN32
#include <editline/readline.h>
#endif // _WIN32

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
