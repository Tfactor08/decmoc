/*
E -> T{+|- T}
T -> P{*|/ P} | PP{P}
P -> F{^ F}
F -> Id | Number | (E) | -F | Func(E)
Func: sin | cos | exp
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "lexer.c"
#include "basic_utils.c"

#include "parser.h"

#define ZERO (1e-8)
#define FLOAT_PRECISION "2"
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define MALLOC_CHECK(ptr)                                               \
    do {                                                                \
        if (!ptr) {                                                     \
            fprintf(stderr, "ERROR (parser): malloc failed at %s:%d\n", \
                    __FILE__, __LINE__);                                \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)

typedef struct {
    NODETREE_HEAD
    NodeTree *left;
    NodeTree *right;
} NodeBinary;

typedef struct {
    NODETREE_HEAD
    FUNC func;
    NodeTree *arg;
} NodeFunc;

typedef struct {
    NODETREE_HEAD
    NodeTree *arg;
} NodeNegate;

typedef struct {
    NODETREE_HEAD
    char var;
} NodeVar;

typedef struct {
    NODETREE_HEAD
    float value;
} NodeNumber;

static float const_to_float[] = {
    [PI]  = 3.14f,
    [E]   = 2.71f,
    [PHI] = 1.62f
};

#define MAKE_NODE_BINARY_MAKE_FUNC(name)                                     \
    static NodeBinary *node_##name##_make(NodeTree *left, NodeTree *right)   \
    {                                                                        \
        NodeBinary *node = malloc(sizeof(NodeBinary));                       \
        MALLOC_CHECK(node);                                                  \
        node->vtable = &node_##name##_vtable;                                \
        node->left = left;                                                   \
        node->right = right;                                                 \
        return node;                                                         \
    }

#define MAKE_NODE_BINARY_EVAL_FUNC(name, operator)                   \
    static float node_##name##_eval(void *self, float x)             \
    {                                                                \
        NodeBinary *node = self;                                     \
        NodeTree *l = node->left;                                    \
        NodeTree *r = node->right;                                   \
        return l->vtable->eval(l, x) operator r->vtable->eval(r, x); \
    }

#define MAKE_NODE_BINARY_PRINT_FUNC(name, operator)                      \
    static PrintBuffer node_##name##_print(void *self)                   \
    {                                                                    \
        NodeBinary *node = self;                                         \
        PrintBuffer buf = {0};                                           \
        PrintBuffer left_buf = node->left->vtable->print(node->left);    \
        PrintBuffer right_buf = node->right->vtable->print(node->right); \
        int str_len = strlen(left_buf.str) + strlen(right_buf.str) + 6;  \
        assert(PRINT_BUFFER_CAP >= str_len);                             \
        snprintf(buf.str, PRINT_BUFFER_CAP, "(%s " #operator " %s)",     \
                 left_buf.str, right_buf.str) < 1 ?                      \
                 exit(EXIT_FAILURE) : (void) 1;                          \
        return buf;                                                      \
    }

/* --- EVAL FUNCTIONS --- */

MAKE_NODE_BINARY_EVAL_FUNC(add, +);
MAKE_NODE_BINARY_EVAL_FUNC(sub, -);
MAKE_NODE_BINARY_EVAL_FUNC(mul, *);

// node_div_eval is implemented separately from the common NODE_BINARY_EVAL
// since it must be realized in a special way 
static float node_div_eval(void *self, float x)
{
    NodeBinary *node = self;
    NodeTree *l = node->left;
    NodeTree *r = node->right;
    if (x == 0) x = ZERO; // avoid 0/0 indetermination
    return l->vtable->eval(l, x) / r->vtable->eval(r, x);
}

// node_pow_eval is implemented separately from the common NODE_BINARY_EVAL
// since it must be realized in a special way 
static float node_pow_eval(void *self, float x)
{
    NodeBinary *node = self;
    NodeTree *l = node->left;
    NodeTree *r = node->right;
    return powf(l->vtable->eval(l, x), r->vtable->eval(r, x));
}

static float node_func_eval(void *self, float x)
{
    NodeFunc *node = self;
    NodeTree *arg = node->arg;
    FUNC func = node->func;
    if (func == SIN)
        return sin(arg->vtable->eval(arg, x));
    else if (func == COS)
        return cos(arg->vtable->eval(arg, x));
    else if (func == EXP)
        return exp(arg->vtable->eval(arg, x));
    else
        assert(0 && "Unhandled function");
}

static float node_negate_eval(void *self, float x)
{
    NodeNegate *node = self;
    NodeTree *arg = node->arg;
    return -(arg->vtable->eval(arg, x));
}

static float node_number_eval(void *self, float x)
{
    NodeNumber *node = self;
    return node->value;
}

static float node_var_eval(void *self, float x)
{
    return x;
}

/* --- PRINT FUNCTIONS --- */

MAKE_NODE_BINARY_PRINT_FUNC(add, +);
MAKE_NODE_BINARY_PRINT_FUNC(sub, -);
MAKE_NODE_BINARY_PRINT_FUNC(mul, *);
MAKE_NODE_BINARY_PRINT_FUNC(div, /);
MAKE_NODE_BINARY_PRINT_FUNC(pow, ^);

static PrintBuffer node_func_print(void *self)
{
    NodeFunc *node = self;
    PrintBuffer buf = {0};
    PrintBuffer arg_buf = node->arg->vtable->print(node->arg);
    char *func_str;
    switch (node->func) {
        case SIN:
            func_str = SIN_STR;
            break;
        case COS:
            func_str = COS_STR;
            break;
        case EXP:
            func_str = EXP_STR;
            break;
        default:
            assert(0 && "Unhandled function");
    }
    int str_len = strlen(func_str) + strlen(arg_buf.str) + 5;
    assert(PRINT_BUFFER_CAP >= str_len);
    // The condition is used to silence the "-Wformat-truncation" warning
    snprintf(buf.str, PRINT_BUFFER_CAP, "(%s(%s))",
             func_str, arg_buf.str) < 0 ?
             exit(EXIT_FAILURE) : (void) 0;
    return buf;
}

static PrintBuffer node_negate_print(void *self)
{
    NodeNegate *node = self;
    PrintBuffer buf = {0};
    PrintBuffer arg_buf = node->arg->vtable->print(node->arg);
    int str_len = strlen(arg_buf.str) + 4;
    assert(PRINT_BUFFER_CAP >= str_len);
    // The condition is used to silence the "-Wformat-truncation" warning
    snprintf(buf.str, PRINT_BUFFER_CAP, "-(%s)", arg_buf.str) < 0 ?
             exit(EXIT_FAILURE) : (void) 0;
    return buf;
}

static PrintBuffer node_number_print(void *self)
{
    NodeNumber *node = self;
    PrintBuffer buf = {0};
    int str_len = int_len((int) node->value) + atoi(FLOAT_PRECISION) + 1;
    assert(PRINT_BUFFER_CAP >= str_len);
    snprintf(buf.str, PRINT_BUFFER_CAP, "%." FLOAT_PRECISION "f", node->value);
    return buf;
}

static PrintBuffer node_var_print(void *self)
{
    NodeVar *node = self;
    PrintBuffer buf = {0};
    int str_len = 1;
    assert(PRINT_BUFFER_CAP >= str_len);
    snprintf(buf.str, PRINT_BUFFER_CAP, "%c", node->var);
    return buf;
}

/* --- FREE FUNCTIONS --- */

static void node_binary_free(void *self)
{
    NodeBinary *node = self;
    node->left->vtable->free(node->left);
    node->right->vtable->free(node->right);
    free(node);
}

static void node_func_free(void *self)
{
    NodeFunc *node = self;
    node->arg->vtable->free(node->arg);
    free(node);
}

static void node_negate_free(void *self)
{
    NodeNegate *node = self;
    node->arg->vtable->free(node->arg);
    free(node);
}

static void node_var_free(void *self)
{
    NodeVar *node = self;
    free(node);
}

static void node_number_free(void *self)
{
    NodeNumber *node = self;
    free(node);
}

/* --- VTABLES --- */

#define MAKE_NODE_VTABLE_STRUCT(node)      \
    static VTable node_##node##_vtable = { \
        .print = node_##node##_print,      \
        .eval = node_##node##_eval,        \
        .free = node_##node##_free         \
    };

// Specific macro for the Binary node is needed since the free pointer differs
#define MAKE_NODE_BINARY_VTABLE_STRUCT(node) \
    static VTable node_##node##_vtable = {   \
        .print = node_##node##_print,        \
        .eval = node_##node##_eval,          \
        .free = node_binary_free             \
    };

MAKE_NODE_BINARY_VTABLE_STRUCT(add);
MAKE_NODE_BINARY_VTABLE_STRUCT(sub);
MAKE_NODE_BINARY_VTABLE_STRUCT(mul);
MAKE_NODE_BINARY_VTABLE_STRUCT(div);
MAKE_NODE_BINARY_VTABLE_STRUCT(pow);

MAKE_NODE_VTABLE_STRUCT(func);
MAKE_NODE_VTABLE_STRUCT(negate);
MAKE_NODE_VTABLE_STRUCT(var);
MAKE_NODE_VTABLE_STRUCT(number);

/* --- MAKE FUNCTIONS --- */

MAKE_NODE_BINARY_MAKE_FUNC(add);
MAKE_NODE_BINARY_MAKE_FUNC(sub);
MAKE_NODE_BINARY_MAKE_FUNC(mul);
MAKE_NODE_BINARY_MAKE_FUNC(div);
MAKE_NODE_BINARY_MAKE_FUNC(pow);

static NodeFunc *node_func_make(NodeTree *arg, FUNC func)
{
    NodeFunc *node = malloc(sizeof(NodeFunc));
    MALLOC_CHECK(node);
    node->vtable = &node_func_vtable;
    node->func = func;
    node->arg = arg;
    return node;
}

static NodeNegate *node_negate_make(NodeTree *arg)
{
    NodeNegate *node = malloc(sizeof(NodeNegate));
    MALLOC_CHECK(node);
    node->vtable = &node_negate_vtable;
    node->arg = arg;
    return node;
}

static NodeNumber *node_number_make(float val)
{
    NodeNumber *node = malloc(sizeof(NodeNumber));
    MALLOC_CHECK(node);
    node->vtable = &node_number_vtable;
    node->value = val;
    return node;
}

static NodeVar *node_var_make(char var)
{
    NodeVar *node = malloc(sizeof(NodeVar));
    MALLOC_CHECK(node);
    node->vtable = &node_var_vtable;
    node->var = var;
    return node;
}

static NodeTree *term(Lexer *);

// E -> T {+|- T}
static NodeTree *expression(Lexer *l)
{
    NodeTree *a = term(l);
    if (!a) return NULL;
    while (true) {
        TOKEN_KIND tk_kind = lexer_current(l).kind;
        if (tk_kind == TK_PLUS) {
            lexer_next(l);
            NodeTree *b = term(l);
            if (!b) return NULL;
            a = (NodeTree *) node_add_make(a, b);
        } else if (tk_kind == TK_MINUS) {
            lexer_next(l);
            NodeTree *b = term(l);
            if (!b) return NULL;
            a = (NodeTree *) node_sub_make(a, b);
        } else {
            return a;
        }
    }
}

static bool is_factor(Token token)
{
    static const TOKEN_KIND factor_tks[] = {
        TK_VAR, TK_INT, TK_DEC, TK_OPENP, TK_FUNC
    };
    for (size_t i = 0; i < ARRAY_LEN(factor_tks); i++)
        if (token.kind == factor_tks[i])
            return true;
    return false;
}

static NodeTree *primary(Lexer *);
static NodeTree *factor(Lexer *);

// T -> P{*|/ P} | PP{P}
static NodeTree *term(Lexer *l)
{
    NodeTree *a = primary(l);
    if (!a) return NULL;
    Token curr_tk = lexer_current(l);
    if (is_factor(curr_tk)) {
        do {
            NodeTree *b = primary(l);
            if (!b) return NULL;
            a = (NodeTree *) node_mul_make(a, b);
        } while (is_factor(lexer_current(l)));
        return a;
    } else {
        while (true) {
            curr_tk = lexer_current(l);
            if (curr_tk.kind == TK_MUL) {
                lexer_next(l);
                NodeTree *b = primary(l);
                if (!b) return NULL;
                a = (NodeTree *) node_mul_make(a, b);
            } else if (curr_tk.kind == TK_DIV) {
                lexer_next(l);
                NodeTree *b = primary(l);
                if (!b) return NULL;
                a = (NodeTree *) node_div_make(a, b);
            } else {
                return a;
            }
        }
    }
}

static NodeTree *factor(Lexer *);

// P -> F {^ F}
static NodeTree *primary(Lexer *l)
{
    NodeTree *a = factor(l);
    if (!a) return NULL;
    while (true) {
        TOKEN_KIND tk_kind = lexer_current(l).kind;
        if (tk_kind == TK_POW) {
            lexer_next(l);
            NodeTree *b = factor(l);
            if (!b) return NULL;
            a = (NodeTree *) node_pow_make(a, b);
        } else {
            return a;
        }
    }
}

// F -> Id | Number | (E) | -F | Func(E)
static NodeTree *factor(Lexer *l)
{
    Token curr_tk = lexer_current(l);
    if (curr_tk.kind == TK_INT) {
        lexer_next(l);
        return (NodeTree *) node_number_make(token_int_get(&curr_tk));
    } else if (curr_tk.kind == TK_DEC) {
        lexer_next(l);
        return (NodeTree *) node_number_make(token_dec_get(&curr_tk));
    } else if (curr_tk.kind == TK_CONST) {
        lexer_next(l);
        CONST constt = token_const_get(&curr_tk);
        return (NodeTree *) node_number_make(const_to_float[constt]);
    } else if (curr_tk.kind == TK_VAR) {
        lexer_next(l);
        return (NodeTree *) node_var_make(token_var_get(&curr_tk));
    } else if (curr_tk.kind == TK_MINUS) {
        lexer_next(l);
        NodeTree *f = factor(l);
        if (!f) return NULL;
        return (NodeTree *) node_negate_make(f);
    } else if (curr_tk.kind == TK_OPENP) {
        lexer_next(l);
        NodeTree *e = expression(l);
        if (!e) return NULL;
        if (lexer_current(l).kind == TK_CLOSEP) {
            lexer_next(l);
            return e;
        } else {
            fprintf(stderr, "ERROR (parser): unmatching (\n");
            return NULL;
        }
    } else if (curr_tk.kind == TK_FUNC) {
        NodeFunc *func;
        Token func_tk = curr_tk;
        lexer_next(l);
        if (lexer_current(l).kind != TK_OPENP) {
            fprintf(stderr, "ERROR (parser): ( expected after function\n");
            return NULL;
        }
        lexer_next(l);
        NodeTree *e = expression(l);
        if (!e) return NULL;
        FUNC func_kind = token_func_get(&func_tk);
        func = node_func_make(e, func_kind);
        if (lexer_current(l).kind != TK_CLOSEP) {
            fprintf(stderr, "ERROR (parser): unmatching ) for function\n");
            return NULL;
        }
        lexer_next(l);
        return (NodeTree *) func;
    } else if (curr_tk.kind == TK_ERROR) {
        fprintf(stderr, "ERROR (lexer): %s\n", token_error_get(&curr_tk));
        return NULL;
    } else {
        LexPrintBuffer tk_buf = curr_tk.print(&curr_tk);
        fprintf(stderr, "ERROR (parser): unexpected token: %s\n", tk_buf.str);
        return NULL;
    }
    return NULL; // Unreachable but silences the warning
}

NodeTree *tree_parse(const char *src)
{
    Lexer lexer = lexer_create(src);
    NodeTree *result = expression(&lexer);
    if (!result)
        return NULL;
    if (lexer_current(&lexer).kind != TK_EOF) {
        fprintf(stderr, "ERROR (parser): invalid expression\n");
        result->vtable->free(result);
        return NULL;
    }
    return result;
}

void tree_print(NodeTree *tree)
{
    PrintBuffer res = tree->vtable->print(tree);
    printf("%s\n", res.str);
}

float tree_eval(NodeTree *tree, float x)
{
    return tree->vtable->eval(tree, x);
}

void tree_free(NodeTree *tree)
{
    tree->vtable->free(tree);
}

// NOTE: 'feat/parameters' branch:
//       at the moment, parser treats all variables in an input expression the same way --
//       as an independent variable 'x'. Although, we do not have multivariable functions support yet,
//       it would be rather simple to add parameters support -- variables whose names are different
//       from 'x'. Unlike 'x', they will require a fixed value, which user may tweak as he wants to
//       see how the funciton graph changes.

#ifdef PARSER_MAIN
int main(void)
{
    char *expr = NULL;
    size_t len = 0;
    ssize_t nread = 0;
    while (true) {
        printf("Expr: ");
        if ((nread = getline(&expr, &len, stdin)) == -1)
            break;
        expr[nread - 1] = '\0';

        NodeTree *result = tree_parse(expr);
        if (!result) continue;

        tree_print(result);
        printf("%.2f\n", tree_eval(result, 1));

        tree_free(result);
    }

    return 0;
}
#endif
