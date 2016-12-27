#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <editline/readline.h>
#include "mpc.h"

/* Typedefs
 * -------------------------------------------------------------------------- */
typedef struct lval {
        int type;
        long num;
        char *err;
        char *sym;
        int cnt; // Number of elements in cell list;
        struct lval **cell;
} lval_t;

// Possible lval_t types.
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// Possible lval_t errors.
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Private routine prototypes
 * -------------------------------------------------------------------------- */
static lval_t* lval_new_num(long x);
static lval_t* lval_new_err(char *msg);
static lval_t* lval_new_sym(char *sym);
static lval_t* lval_new_sexpr(void);
static void sexpr_del(lval_t *sexpr);
static void lval_del(lval_t *v);
static void lval_print_expr(lval_t *v, char open, char close);
static void lval_print(lval_t *v);
static void lval_println(lval_t *v);
static bool is_number_leaf(mpc_ast_t *tree);
static bool is_expression_node(mpc_ast_t *tree);
static lval_t* eval_op(lval_t *x, char *op, lval_t *y);
static lval_t* lval_read_num(mpc_ast_t *tree);
static lval_t* lval_add_to_sexpr(lval_t *dst_node, lval_t *src_node);
static lval_t* eval (mpc_ast_t *tree);

/* Private routines
 * -------------------------------------------------------------------------- */
static lval_t*
lval_new_num(long x)
{
        lval_t *v = malloc(sizeof(lval_t));
        v->type = LVAL_NUM;
        v->num = x;
        return v;
}

static lval_t*
lval_new_err(char *msg)
{
        lval_t *v = malloc(sizeof(lval_t));
        v->type = LVAL_ERR;
        v->err = malloc(strlen(msg) + 1);
        strcpy(v->err, msg);
        return v;
}

static lval_t*
lval_new_sym(char *sym)
{
        lval_t *v = malloc(sizeof(lval_t));
        v->type = LVAL_SYM;
        v->sym = malloc(strlen(sym) + 1);
        strcpy(v->sym, sym);
        return v;
}

static lval_t*
lval_new_sexpr(void)
{
        lval_t *v = malloc(sizeof(lval_t));
        v->type = LVAL_SEXPR;
        v->cnt = 0;
        v->cell = NULL;
        return v;
}

static void
sexpr_del(lval_t *sexpr)
{
        for (int i = 0; i < sexpr->cnt; i++) {
                lval_del(sexpr->cell[i]);
        }
        free(sexpr->cell);
}

static void
lval_del(lval_t *v)
{
        switch (v->type) {
        case LVAL_NUM:
                break;
        case LVAL_ERR:
                free(v->err);
                break;
        case LVAL_SYM:
                free(v->sym);
                break;
        case LVAL_SEXPR:
                sexpr_del(v);
                break;
        }

        free(v);
}

static void
lval_print_expr(lval_t *v, char open, char close)
{
        putchar(open);

        for (int i = 0; i < v->cnt; i++) {
                lval_print(v->cell[i]);

                if (i != (v->cnt - 1)) {
                        putchar(' ');
                }
        }
        putchar(close);
}

static void
lval_print(lval_t *v)
{
        switch (v->type) {
        case LVAL_NUM:
                printf("%li", v->num);
                break;
        case LVAL_ERR:
                printf("Error: %s", v->err);
                break;
        case LVAL_SYM:
                printf("%s", v->sym);
                break;
        case LVAL_SEXPR:
                lval_print_expr(v, '(', ')');
                break;
        default:
                break;
        }
}

static void
lval_println(lval_t *v)
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
is_node_of_type(mpc_ast_t *node, char *type)
{
        return strstr(node->tag, type) != 0 ? true : false;
}

static bool
is_node_root(mpc_ast_t *node)
{
        return strcmp(node->tag, ">") == 0 ? true : false;
}

static bool
is_expression_node(mpc_ast_t *node)
{
        if (strstr(node->tag, "expr")) {
                return true;
        } else {
                return false;
        }
}

static lval_t*
lval_read_num(mpc_ast_t *tree)
{
        errno = 0;
        long x = strtol(tree->contents, NULL, 10);
        lval_t *result =
                errno != ERANGE ?
                lval_new_num(x) : lval_new_err("invalid number");
        return result;
}

static lval_t*
eval_op(lval_t *x, char *op, lval_t *y)
{
        if (x->type == LVAL_ERR) return x;
        if (y->type == LVAL_ERR) return y;

        if (strcmp(op, "+") == 0) return lval_new_num(x->num + y->num);
        if (strcmp(op, "-") == 0) return lval_new_num(x->num - y->num);
        if (strcmp(op, "*") == 0) return lval_new_num(x->num * y->num);
        if (strcmp(op, "/") == 0) {
                lval_t *result =
                        y->num == 0 ?
                        lval_new_err("division by zero") :
                        lval_new_num(x->num / y->num);
                return result;
        }

        return lval_new_err("unknown operator");
}

static lval_t*
lval_read(mpc_ast_t *tree)
{
        if (is_node_of_type(tree, "number")) {
                return lval_read_num(tree);
        }

        if (is_node_of_type(tree, "symbol")) {
                return lval_new_sym(tree->contents);
        }

        lval_t *sexpr = NULL;

        if ((is_node_of_type(tree, "sexpr")) || (is_node_root(tree))) {
                sexpr = lval_new_sexpr();
        }

        for (int i = 0; i < tree->children_num; i++) {
                mpc_ast_t *node = tree->children[i];

                if ((is_node_of_type(node, "number")) ||
                    (is_node_of_type(node, "symbol")) ||
                    (is_node_of_type(node, "sexpr"))) {

                        sexpr = lval_add_to_sexpr(sexpr, lval_read(node));
                }
        }

        return sexpr;
}

static lval_t*
lval_add_to_sexpr(lval_t *dst_node, lval_t *src_node)
{
        dst_node->cnt++;
        dst_node->cell =
                realloc(dst_node->cell, sizeof(lval_t*) * dst_node->cnt);

        dst_node->cell[dst_node->cnt - 1] = src_node;
        return dst_node;
}

static lval_t*
eval (mpc_ast_t *tree)
{
        if (is_number_leaf(tree)) {
                return lval_read_num(tree);
        }

        // Non-number nodes.
        unsigned int i = 1;
        mpc_ast_t *node = tree->children[i];
        char *operator = node->contents;

        node = tree->children[++i];
        lval_t *x = eval(node);

        node = tree->children[++i];
        while (is_expression_node(node)) {
                lval_t *y = eval(node);
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
        mpc_parser_t* Symbol   = mpc_new("symbol");
        mpc_parser_t* Sexpr    = mpc_new("sexpr");
        mpc_parser_t* Expr     = mpc_new("expr");
        mpc_parser_t* Lispy    = mpc_new("lispy");

        // Define parsers with the following language.
        mpca_lang(MPCA_LANG_DEFAULT,
                  "number   :   /-?[0-9]+/ ;"
                  "symbol   :   '+' | '-' | '*' | '/' ;"
                  "sexpr    :   '(' <expr>* ')' ;"
                  "expr     :   <number> | <symbol> | <sexpr> ;"
                  "lispy    :   /^/ <expr>* /$/ ;"
                  ,
                  Number, Symbol, Sexpr, Expr, Lispy);

        puts("Lispy version 0.5.0");
        puts("Press Ctrl+C to exit\n");

        while (1) {

                char *usr_input = readline("> ");
                add_history(usr_input);

                mpc_result_t r;
                // Attempt to parse the user input.
                if (mpc_parse("<stdin>", usr_input, Lispy, &r)) {
                        // Parsing successful.
                        mpc_ast_t *ast = r.output;
                        lval_t *x = lval_read(ast);
                        lval_println(x);
                        lval_del(x);
                        //lval_t *expr_result = eval(ast);
                        //lval_println(expr_result);
                        mpc_ast_delete(ast);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(usr_input);
        }

        mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

        return 0;
}
