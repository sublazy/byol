#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <editline/readline.h>
#include "mpc.h"

/* Typedefs
 * -------------------------------------------------------------------------- */
typedef struct {
        int type;
        long num;
        int err;
} lval_t;

// Possible lval_t types.
enum { LVAL_NUM, LVAL_ERR };

// Possible lval_t errors.
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Private routines
 * -------------------------------------------------------------------------- */
static lval_t
lval_num(long x)
{
        lval_t v;
        v.type = LVAL_NUM;
        v.num = x;
        return v;
}

static lval_t
lval_err(int x)
{
        lval_t v;
        v.type = LVAL_ERR;
        v.err = x;
        return v;
}

static void
lval_print_err(int e)
{
        switch (e) {
        case LERR_DIV_ZERO:
                printf("Error: Division by zero.");
                break;
        case LERR_BAD_OP:
                printf("Error: Invalid operator.");
                break;
        case LERR_BAD_NUM:
                printf("Error: Invalid number.");
                break;
        default:
                printf("Error: Unclassified error.");
        }
}

static void
lval_print(lval_t v)
{
        switch (v.type) {
        case LVAL_NUM:
                printf("%li", v.num);
                break;
        case LVAL_ERR:
                lval_print_err(v.err);
                break;
        default:
                break;
        }
}

static void
lval_println(lval_t v)
{
        printf(" ");
        lval_print(v);
        putchar('\n');
}

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

static lval_t
eval_op(lval_t x, char *op, lval_t y)
{
        if (x.type == LVAL_ERR) return x;
        if (y.type == LVAL_ERR) return y;


        if (strcmp(op, "+") == 0) return lval_num(x.num + y.num);
        if (strcmp(op, "-") == 0) return lval_num(x.num - y.num);
        if (strcmp(op, "*") == 0) return lval_num(x.num * y.num);
        if (strcmp(op, "/") == 0) {
                lval_t result =
                        y.num == 0 ?
                        lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
                return result;
        }

        return lval_err(LERR_BAD_OP);
}

static lval_t
eval (mpc_ast_t *tree)
{
        if (is_number_leaf(tree)) {
                errno = 0;
                long x = strtol(tree->contents, NULL, 10);
                lval_t result =
                        errno != ERANGE ?
                          lval_num(x) : lval_err(LERR_BAD_NUM);
                return result;
        }

        // Non-number nodes.
        unsigned int i = 1;
        mpc_ast_t *node = tree->children[i];
        char *operator = node->contents;

        node = tree->children[++i];
        lval_t x = eval(node);

        node = tree->children[++i];
        while (is_expression_node(node)) {
                lval_t y = eval(node);
                x = eval_op(x, operator, y);
                node = tree->children[++i];
        }

        return x;
}

/* main
 * -------------------------------------------------------------------------- */
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

        puts("Lispy version 0.4.0");
        puts("Press Ctrl+C to exit\n");

        while (1) {

                char *usr_input = readline("> ");
                add_history(usr_input);

                mpc_result_t r;
                // Attempt to parse the user input.
                if (mpc_parse("<stdin>", usr_input, Lispy, &r)) {
                        // Parsing successful.
                        mpc_ast_t *ast = r.output;
                        lval_t expr_result = eval(ast);
                        lval_println(expr_result);
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
