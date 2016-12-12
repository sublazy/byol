#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
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

static bool
is_paren_node(mpc_ast_t *tree)
{
        if ((strcmp(tree->contents, "(") == 0) ||
            (strcmp(tree->contents, ")") == 0)) {
                return true;
        } else {
                return false;
        }
}


static unsigned int
numof_leaves (mpc_ast_t *tree)
{
        unsigned int total = 0;

        if (tree->children_num == 0) {
                total = 1;
        } else {
                for (unsigned int i = 0; i < tree->children_num; i++) {
                        total += numof_leaves(tree->children[i]);
                }
        }

        return total;
}

static unsigned int
numof_branches (mpc_ast_t *tree)
{
        unsigned int total = 0;

        if (tree->children_num == 0) {
                total = 0;
        } else {
                total = 1;
                for (unsigned int i = 0; i < tree->children_num; i++) {
                        total += numof_branches(tree->children[i]);
                }
        }

        return total;
}

static unsigned int
numof_children (mpc_ast_t *tree)
{
        return tree->children_num;
}

static unsigned int
max_numof_children (mpc_ast_t *tree)
{
        unsigned int max_n = 0;

        if (tree->children_num == 0) {
                max_n = 0;
        } else {
                max_n = tree->children_num;

                for (unsigned int i = 0; i < tree->children_num; i++) {
                        unsigned int new_n =
                                        max_numof_children(tree->children[i]);
                        if (new_n > max_n) {
                                max_n = new_n;
                        }
                }
        }

        return max_n;
}

static mpc_ast_t*
get_widest_branch(mpc_ast_t *tree)
{
        mpc_ast_t *widest_branch = NULL;
        unsigned int max_children = tree->children_num;

        if (tree->children_num == 0) {
                widest_branch = tree;
        } else {
                for (unsigned int i = 0; i < tree->children_num; i++) {
                        unsigned int n_children =
                                            numof_children(tree->children[i]);

                        if (n_children > max_children) {
                                max_children = n_children;
                                widest_branch = tree->children[i];
                        }
                }
        }

        return widest_branch;
}

static long
min(long x, long y)
{
        if (x < y)
                return x;
        else
                return y;
}

static long
max(long x, long y)
{
        if (x > y)
                return x;
        else
                return y;
}

static long
eval_op(long x, char *op, long y)
{
        if (strcmp(op, "+") == 0) return x + y;
        if (strcmp(op, "-") == 0) return x - y;
        if (strcmp(op, "*") == 0) return x * y;
        if (strcmp(op, "/") == 0) return x / y;
        if (strcmp(op, "%") == 0) return x % y;
        if (strcmp(op, "^") == 0) return pow(x, y);
        if (strcmp(op, "min") == 0) return min(x, y);
        if (strcmp(op, "max") == 0) return max(x, y);

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

        // When '-' receives only one argument.
        if ((strcmp(operator, "-") == 0) &&
            (!is_expression_node(tree->children[i + 1]))) {
                    return -1 * atoi(node->contents);
        }

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
                  "operator :   '+' | '-' | '*' | '/' | '%' | '^' |"
                                " \"min\" | \"max\" ;"
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
                        mpc_ast_print(ast);

                        long expr_result = eval(ast);
                        printf(" %li\n", expr_result);

                        printf("Number of leaves: %i\n", numof_leaves(ast));
                        printf("Number of branches: %i\n", numof_branches(ast));
                        printf("Number of children in the widest branch: %i\n",
                                                      max_numof_children(ast));

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
