#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <editline/readline.h>
#include "mpc.h"

static bool
is_number_leaf(mpc_ast_t *tree)
{
        if (strstr(tree->tag, "number")) {
                return true;
        } else {
                return false;
        }
}

static bool
is_expression_node(mpc_ast_t *tree)
{
        if (strstr(tree->tag, "expr")) {
                return true;
        } else {
                return false;
        }
}

static long
eval_op(long x, char *op, long y)
{
        if (strcmp(op, "+") == 0) return x + y;
        if (strcmp(op, "-") == 0) return x - y;
        if (strcmp(op, "*") == 0) return x * y;
        if (strcmp(op, "/") == 0) return x / y;

        return 0;
}


static long
eval (mpc_ast_t *tree)
{
        if (is_number_leaf(tree))
                return atoi(tree->contents);

        // Non-number nodes.
        unsigned int i = 1;
        mpc_ast_t *node = tree->children[i];
        char *operator = node->contents;

        node = tree->children[++i];
        long x = eval(node);

        node = tree->children[++i];
        while (is_expression_node(node)) {
                long y = eval(node);
                x = eval_op(x, operator, y);
                node = tree->children[++i];
        }

        return x;
}

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

        puts("Lispy version 0.3.0");
        puts("Press Ctrl+C to exit\n");

        while (1) {

                char *usr_input = readline("> ");
                add_history(usr_input);

                mpc_result_t r;
                // Attempt to parse the user input.
                if (mpc_parse("<stdin>", usr_input, Lispy, &r)) {
                        // Parsing successful.
                        mpc_ast_t *ast = r.output;
                        long expr_result = eval(ast);
                        printf(" %li\n", expr_result);
                        mpc_ast_delete(ast);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(usr_input);
        }

        mpc_cleanup(4, Number, Operator, Expr, Lispy);

        return 0;
}
