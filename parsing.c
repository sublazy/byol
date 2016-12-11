#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "mpc.h"

int main(int argc, char** argv)
{
        // Create parsers.
        mpc_parser_t* Number   = mpc_new("number");
        mpc_parser_t* Operator = mpc_new("operator");
        mpc_parser_t* Expr     = mpc_new("expr");
        mpc_parser_t* Lispy    = mpc_new("lispy");

        // Define parsers with the following language.
        mpca_lang(MPCA_LANG_DEFAULT,
                  "number   :   /-?[0-9]+/ ;"
                  "operator :   '+' | '-' | '*' | '/' ;"
                  "expr     :   <number> | '(' <operator> <expr>+ ')' ;"
                  "lispy    :   /^/ <operator> <expr>+ /$/ ;"
                  ,
                  Number, Operator, Expr, Lispy);

        puts("Lispy version 0.2.0");
        puts("Press Ctrl+C to exit\n");

        while (1) {

                char *usr_input = readline("> ");
                add_history(usr_input);

                mpc_result_t r;
                // Attempt to parse the user input.
                if (mpc_parse("<stdin>", usr_input, Lispy, &r)) {
                        // Parsing successful.
                        mpc_ast_print(r.output);
                        mpc_ast_delete(r.output);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(usr_input);
        }

        mpc_cleanup(4, Number, Operator, Expr, Lispy);

        return 0;
}
