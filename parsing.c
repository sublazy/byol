#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <editline/readline.h>
#include "mpc.h"

/* Typedefs
 * -------------------------------------------------------------------------- */
typedef struct {
        int type;
        union {
                long inum;
                double fnum;
                int err;
        } value;
} lval_t;

// Possible lval_t types.
enum { LVAL_INUM, LVAL_FNUM, LVAL_ERR };

// Possible lval_t errors.
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Private routines
 * -------------------------------------------------------------------------- */
static lval_t
lval_inum(long x)
{
        lval_t v;
        v.type = LVAL_INUM;
        v.value.inum = x;
        return v;
}

static lval_t
lval_fnum(double x)
{
        lval_t v;
        v.type = LVAL_FNUM;
        v.value.fnum = x;
        return v;
}

static lval_t
lval_err(int x)
{
        lval_t v;
        v.type = LVAL_ERR;
        v.value.err = x;
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
        case LVAL_INUM:
                printf("%li", v.value.inum);
                break;
        case LVAL_FNUM:
                printf("%lf", v.value.fnum);
                break;
        case LVAL_ERR:
                lval_print_err(v.value.err);
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
eval_op_int(lval_t x, char *op, lval_t y)
{
        if (strcmp(op, "+") == 0) return lval_inum(x.value.inum + y.value.inum);
        if (strcmp(op, "-") == 0) return lval_inum(x.value.inum - y.value.inum);
        if (strcmp(op, "*") == 0) return lval_inum(x.value.inum * y.value.inum);
        if (strcmp(op, "/") == 0) {
                lval_t result =
                        y.value.inum == 0 ?
                        lval_err(LERR_DIV_ZERO)
                        : lval_inum(x.value.inum / y.value.inum);
                return result;
        }
        if (strcmp(op, "%") == 0) {
                lval_t result =
                        y.value.inum == 0 ?
                        lval_err(LERR_DIV_ZERO)
                        : lval_inum(x.value.inum % y.value.inum);
                return result;
        }

        return lval_err(LERR_BAD_OP);
}

static lval_t
eval_op_float(lval_t x, char *op, lval_t y)
{
        if (x.type == LVAL_INUM) x = lval_fnum((double)x.value.inum);
        if (y.type == LVAL_INUM) y = lval_fnum((double)y.value.inum);

        if (strcmp(op, "+") == 0) return lval_fnum(x.value.fnum + y.value.fnum);
        if (strcmp(op, "-") == 0) return lval_fnum(x.value.fnum - y.value.fnum);
        if (strcmp(op, "*") == 0) return lval_fnum(x.value.fnum * y.value.fnum);
        if (strcmp(op, "%") == 0) return lval_err(LERR_BAD_OP);
        if (strcmp(op, "/") == 0) {
                lval_t result =
                        y.value.fnum == 0 ?
                        lval_err(LERR_DIV_ZERO)
                        : lval_fnum(x.value.fnum / y.value.fnum);
                return result;
        }

        return lval_err(LERR_BAD_OP);
}

static lval_t
eval_op(lval_t x, char *op, lval_t y)
{
        if (x.type == LVAL_ERR) return x;
        if (y.type == LVAL_ERR) return y;

        if (x.type == LVAL_FNUM || y.type == LVAL_FNUM)
                return eval_op_float(x, op, y);

        if (x.type == LVAL_INUM && y.type == LVAL_INUM)
                return eval_op_int(x, op, y);

        return lval_err(LERR_BAD_OP);
}

static lval_t
eval (mpc_ast_t *tree)
{
        if (is_number_leaf(tree)) {
                errno = 0;
                lval_t result;

                if (strchr(tree->contents, '.')) {
                        double x = strtod(tree->contents, NULL);
                        result = lval_fnum(x);
                } else {
                        long x = strtol(tree->contents, NULL, 10);
                        result = lval_inum(x);
                }

                if (errno != 0)
                        result = lval_err(LERR_BAD_NUM);

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
                  "number   :   /-?[0-9]+(\\.[0-9]+)?/ ;"
                  "operator :   '+' | '-' | '*' | '/' | '%' ;"
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
