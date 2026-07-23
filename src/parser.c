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
#define ARRAY_LEN(arr) (sizeof((arr)) / sizeof(*(arr)))

#define MALLOC_CHECK(ptr)                                               \
    do {                                                                \
        if (!ptr) {                                                     \
            fprintf(stderr, "ERROR (parser): malloc failed at %s:%d\n", \
                    __FILE__, __LINE__);                                \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)

typedef struct {
    NODETREE_HEAD;
    NodeTree *left;
    NodeTree *right;
} NodeBinary;

typedef struct {
    NODETREE_HEAD;
    FUNC func;
    NodeTree *arg;
} NodeFunc;

typedef struct {
    NODETREE_HEAD;
    NodeTree *arg;
} NodeNegate;

typedef struct {
    NODETREE_HEAD;
    char var;
} NodeVar;

typedef struct {
    NODETREE_HEAD;
    float value;
} NodeNumber;

static const float const_to_float[] = {
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

#define MAKE_NODE_BINARY_EVAL_FUNC(name, operator)                                   \
    static float node_##name##_eval(void *self, float x, Params *params)             \
    {                                                                                \
        NodeBinary *node = self;                                                     \
        NodeTree *l = node->left;                                                    \
        NodeTree *r = node->right;                                                   \
        return l->vtable->eval(l, x, params) operator r->vtable->eval(r, x, params); \
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
static float node_div_eval(void *self, float x, Params *params)
{
    NodeBinary *node = self;
    NodeTree *l = node->left;
    NodeTree *r = node->right;
    if (x == 0) x = ZERO; // avoid 0/0 indetermination
    return l->vtable->eval(l, x, params) / r->vtable->eval(r, x, params);
}

// 'node_pow_eval' is implemented separately from the common NODE_BINARY_EVAL
// since it requires a specific behaviour
static float node_pow_eval(void *self, float x, Params *params)
{
    NodeBinary *node = self;
    NodeTree *l = node->left;
    NodeTree *r = node->right;
    return powf(l->vtable->eval(l, x, params), r->vtable->eval(r, x, params));
}

static float node_func_eval(void *self, float x, Params *params)
{
    NodeFunc *node = self;
    NodeTree *arg = node->arg;
    FUNC func = node->func;
    if (func == SIN)
        return sin(arg->vtable->eval(arg, x, params));
    else if (func == COS)
        return cos(arg->vtable->eval(arg, x, params));
    else if (func == EXP)
        return exp(arg->vtable->eval(arg, x, params));
    else
        assert(0 && "Unhandled function");
}

static float node_negate_eval(void *self, float x, Params *params)
{
    NodeNegate *node = self;
    NodeTree *arg = node->arg;
    return -(arg->vtable->eval(arg, x, params));
}

static float node_number_eval(void *self, float x, Params *params)
{
    NodeNumber *node = self;
    return node->value;
}

static float node_var_eval(void *self, float x, Params *params)
{
    NodeVar *node = self;
    char var = node->var;
    if (var != 'x')
        // TODO: is this cast safe?
        return params->param_to_value[(int) var];
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
    // The condition is used to silence the "-Wformat-truncation" warning.
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

static NodeTree *term(Lexer *, Params *);

// E -> T {+|- T}
static NodeTree *expression(Lexer *l, Params *params)
{
    NodeTree *a = term(l, params);
    if (!a) return NULL;
    while (true) {
        TOKEN_KIND tk_kind = lexer_current(l).kind;
        if (tk_kind == TK_PLUS) {
            lexer_next(l);
            NodeTree *b = term(l, params);
            if (!b) return NULL;
            a = (NodeTree *) node_add_make(a, b);
        } else if (tk_kind == TK_MINUS) {
            lexer_next(l);
            NodeTree *b = term(l, params);
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

static NodeTree *primary(Lexer *, Params *);
static NodeTree *factor(Lexer *, Params *);

// T -> P{*|/ P} | PP{P}
static NodeTree *term(Lexer *l, Params *params)
{
    NodeTree *a = primary(l, params);
    if (!a) return NULL;
    Token curr_tk = lexer_current(l);
    if (is_factor(curr_tk)) {
        do {
            NodeTree *b = primary(l, params);
            if (!b) return NULL;
            a = (NodeTree *) node_mul_make(a, b);
        } while (is_factor(lexer_current(l)));
        return a;
    } else {
        while (true) {
            curr_tk = lexer_current(l);
            if (curr_tk.kind == TK_MUL) {
                lexer_next(l);
                NodeTree *b = primary(l, params);
                if (!b) return NULL;
                a = (NodeTree *) node_mul_make(a, b);
            } else if (curr_tk.kind == TK_DIV) {
                lexer_next(l);
                NodeTree *b = primary(l, params);
                if (!b) return NULL;
                a = (NodeTree *) node_div_make(a, b);
            } else {
                return a;
            }
        }
    }
}

static NodeTree *factor(Lexer *, Params *);

// P -> F {^ F}
static NodeTree *primary(Lexer *l, Params *params)
{
    NodeTree *a = factor(l, params);
    if (!a) return NULL;
    while (true) {
        TOKEN_KIND tk_kind = lexer_current(l).kind;
        if (tk_kind == TK_POW) {
            lexer_next(l);
            NodeTree *b = factor(l, params);
            if (!b) return NULL;
            a = (NodeTree *) node_pow_make(a, b);
        } else {
            return a;
        }
    }
}

// F -> Id | Number | (E) | -F | Func(E)
static NodeTree *factor(Lexer *l, Params *params)
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
        char var = token_var_get(&curr_tk);
        if (var != 'x')
            params->params[params->count++] = var;
        return (NodeTree *) node_var_make(var);
    } else if (curr_tk.kind == TK_MINUS) {
        lexer_next(l);
        NodeTree *f = factor(l, params);
        if (!f) return NULL;
        return (NodeTree *) node_negate_make(f);
    } else if (curr_tk.kind == TK_OPENP) {
        lexer_next(l);
        NodeTree *e = expression(l, params);
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
        NodeTree *e = expression(l, params);
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

NodeTree *tree_parse(const char *src, Params *params)
{
    Lexer lexer = lexer_create(src);
    NodeTree *result = expression(&lexer, params);
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

float tree_eval(NodeTree *tree, float x, Params *params)
{
    return tree->vtable->eval(tree, x, params);
}

void tree_free(NodeTree *tree)
{
    tree->vtable->free(tree);
}

// NOTE: 'feat/parameters' branch:
//       at the moment, parser treats all variables in an input expression the same way --
//       as an independent variable 'x'. Although, we do not have multivariable functions support yet,
//       it would be rather simple to add parameters support -- variables whose names are different
//       from 'x'. Unlike 'x', they will require a fixed value, which user may tweak they want to
//       see how the funciton graph changes.
// TODO: looks like we need a convenient array wrapper for working with parameters (doubt that now).

#ifdef PARSER_MAIN
int main(void)
{
    char *expr = NULL;
    size_t len = 0;
    ssize_t nread = 0;
    while (true) {
        printf("f(x): ");
        if ((nread = getline(&expr, &len, stdin)) == -1)
            break;
        expr[nread - 1] = '\0';

        Params params = {0};

        NodeTree *result = tree_parse(expr, &params);
        if (!result) continue;

        for (size_t i = 0; i < params.count; i++) {
            char param = params.params[i];
            float param_value = 0;
            printf("%c: ", param);
            scanf("%f", &param_value);
            // TODO: is this cast safe?
            params.param_to_value[(int) param] = param_value;
        }

        tree_print(result);
        printf("%.2f\n", tree_eval(result, 1, &params));

        tree_free(result);
    }

    return 0;
}
#endif
