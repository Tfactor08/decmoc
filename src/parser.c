/*
E -> T {+|- T}
T -> P {*|/ P}
P -> F {^ F}
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
#include "utils.c"

#include "parser.h"

#define ZERO (1e-8)
#define FLOAT_PRECISION "2"
#define PRINT_BUFFER_CAP (1 << 8)

#define MALLOC_CHECK(ptr)                                                                    \
    do {                                                                                     \
        if (!ptr) {                                                                          \
            fprintf(stderr, "ERROR (parser): malloc failed at %s:%d\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                              \
        }                                                                                    \
    } while (0)

typedef struct {
    NODETREE_HEAD
    NodeTree *left;
    NodeTree *right;
} NodeBinary;

typedef struct {
    NODETREE_HEAD
    FUNC func;
    NodeTree *argument;
} NodeFunc;

typedef struct {
    NODETREE_HEAD
    NodeTree *argument;
} NodeNegate;

typedef struct {
    NODETREE_HEAD
    char *string;
} NodeId;

typedef struct {
    NODETREE_HEAD
    float value;
} NodeNumber;

#define NODE_BINARY_CREATE_FUNC(name)                                        \
    static NodeBinary *node_##name##_create(NodeTree *left, NodeTree *right) \
    {                                                                        \
        NodeBinary *node = malloc(sizeof(NodeBinary));                       \
        MALLOC_CHECK(node);                                                  \
        node->vtable = &node_##name##_vtable;                                \
        node->left = left;                                                   \
        node->right = right;                                                 \
        return node;                                                         \
    }

#define NODE_BINARY_EVAL_FUNC(name, operator)                        \
    static float node_##name##_eval(void *self, float x)             \
    {                                                                \
        NodeBinary *node = self;                                     \
        NodeTree *l = node->left;                                    \
        NodeTree *r = node->right;                                   \
        return l->vtable->eval(l, x) operator r->vtable->eval(r, x); \
    }

#define NODE_BINARY_PRINT_FUNC(name, operator)                            \
    static void node_##name##_print(void *self, char *buffer)             \
    {                                                                     \
        NodeBinary *node = self;                                          \
        char left_buf[PRINT_BUFFER_CAP] = {0};                            \
        node->left->vtable->print(node->left, left_buf);                  \
        char right_buf[PRINT_BUFFER_CAP] = {0};                           \
        node->right->vtable->print(node->right, right_buf);               \
        size_t buf_left_cap = PRINT_BUFFER_CAP - (strlen(buffer) + 1);    \
        assert(buf_left_cap >= strlen(left_buf) + strlen(right_buf) + 5); \
        sprintf(buffer, "(%s " #operator " %s)", left_buf, right_buf);    \
    }

/* --- EVAL FUNCTIONS --- */

NODE_BINARY_EVAL_FUNC(add, +);
NODE_BINARY_EVAL_FUNC(sub, -);
NODE_BINARY_EVAL_FUNC(mul, *);

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
    NodeTree *arg = node->argument;
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
    NodeTree *arg = node->argument;
    return -(arg->vtable->eval(arg, x));
}

static float node_number_eval(void *self, float x)
{
    NodeNumber *node = self;
    return node->value;
}

static float node_id_eval(void *self, float x)
{
    return x;
}

/* --- PRINT FUNCTIONS --- */

NODE_BINARY_PRINT_FUNC(add, +);
NODE_BINARY_PRINT_FUNC(sub, -);
NODE_BINARY_PRINT_FUNC(mul, *);
NODE_BINARY_PRINT_FUNC(div, /);
NODE_BINARY_PRINT_FUNC(pow, ^);

static void node_func_print(void *self, char *buffer)
{
    NodeFunc *node = self;
    char arg_buf[PRINT_BUFFER_CAP];
    node->argument->vtable->print(node->argument, arg_buf);
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
    size_t buf_left_cap = PRINT_BUFFER_CAP - (strlen(buffer) + 1);
    assert(buf_left_cap >= strlen(func_str) + strlen(arg_buf) + 5);
    sprintf(buffer, "(%s(%s))", func_str, arg_buf);
}

static void node_negate_print(void *self, char *buffer)
{
    NodeNegate *node = self;
    char arg_buf[PRINT_BUFFER_CAP];
    node->argument->vtable->print(node->argument, arg_buf);
    size_t buf_left_cap = PRINT_BUFFER_CAP - (strlen(buffer) + 1);
    assert(buf_left_cap >= strlen(arg_buf) + 4);
    sprintf(buffer, "-(%s)", arg_buf);
}

static void node_number_print(void *self, char *buffer)
{
    NodeNumber *node = self;
    size_t buf_left_cap = PRINT_BUFFER_CAP - (strlen(buffer) + 1);
    assert(buf_left_cap >= int_len((int) node->value) + atoi(FLOAT_PRECISION) + 1);
    sprintf(buffer, "%." FLOAT_PRECISION "f", node->value);
}

static void node_id_print(void *self, char *buffer)
{
    NodeId *node = self;
    size_t buf_left_cap = PRINT_BUFFER_CAP - (strlen(buffer) + 1);
    assert(buf_left_cap >= strlen(node->string) + 1);
    sprintf(buffer, "%s", node->string);
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
    node->argument->vtable->free(node->argument);
    free(node);
}

static void node_negate_free(void *self)
{
    NodeNegate *node = self;
    node->argument->vtable->free(node->argument);
    free(node);
}

static void node_id_free(void *self)
{
    NodeId *node = self;
    free(node->string);
    free(node);
}

static void node_number_free(void *self)
{
    NodeNumber *node = self;
    free(node);
}

#define NODE_VTABLE_STRUCT(node)           \
    static VTable node_##node##_vtable = { \
        .print = node_##node##_print,      \
        .eval = node_##node##_eval,        \
        .free = node_##node##_free         \
    };

// Specific macro for the Binary node is needed since the free pointer differs
#define NODE_BINARY_VTABLE_STRUCT(node)    \
    static VTable node_##node##_vtable = { \
        .print = node_##node##_print,      \
        .eval = node_##node##_eval,        \
        .free = node_binary_free           \
    };

NODE_BINARY_VTABLE_STRUCT(add);
NODE_BINARY_VTABLE_STRUCT(sub);
NODE_BINARY_VTABLE_STRUCT(mul);
NODE_BINARY_VTABLE_STRUCT(div);
NODE_BINARY_VTABLE_STRUCT(pow);

NODE_VTABLE_STRUCT(func);
NODE_VTABLE_STRUCT(negate);
NODE_VTABLE_STRUCT(id);
NODE_VTABLE_STRUCT(number);

NODE_BINARY_CREATE_FUNC(add);
NODE_BINARY_CREATE_FUNC(sub);
NODE_BINARY_CREATE_FUNC(mul);
NODE_BINARY_CREATE_FUNC(div);
NODE_BINARY_CREATE_FUNC(pow);

/* --- CREATE FUNCTIONS --- */

static NodeFunc *node_func_create(NodeTree *arg, FUNC func)
{
    NodeFunc *node = malloc(sizeof(NodeFunc));
    MALLOC_CHECK(node);
    node->vtable = &node_func_vtable;
    node->func = func;
    node->argument = arg;
    return node;
}

static NodeNegate *node_negate_create(NodeTree *arg)
{
    NodeNegate *node = malloc(sizeof(NodeNegate));
    MALLOC_CHECK(node);
    node->vtable = &node_negate_vtable;
    node->argument = arg;
    return node;
}

static NodeNumber *node_number_create(float val)
{
    NodeNumber *node = malloc(sizeof(NodeNumber));
    MALLOC_CHECK(node);
    node->vtable = &node_number_vtable;
    node->value = val;
    return node;
}

static NodeId *node_id_create(char *string)
{
    NodeId *node = malloc(sizeof(NodeId));
    MALLOC_CHECK(node);
    node->vtable = &node_id_vtable;
    node->string = strdup(string);
    return node;
}

static NodeTree *expression(Lexer *l)
{
    NodeTree *term(Lexer *);

    NodeTree *a = term(l);
    if (!a) return NULL;
    while (true) {
        if (lexer_current(l).kind == TK_PLUS) {
            lexer_next(l);
            NodeTree *b = term(l);
            if (!b) return NULL;
            a = (NodeTree *)node_add_create(a, b);
        } else if (lexer_current(l).kind == TK_MINUS) {
            lexer_next(l);
            NodeTree *b = term(l);
            if (!b) return NULL;
            a = (NodeTree *)node_sub_create(a, b);
        } else {
            return a;
        }
    }
}

NodeTree *term(Lexer *l)
{
    NodeTree *primary(Lexer *);

    NodeTree *a = primary(l);
    if (!a) return NULL;
    while (true) {
        if (lexer_current(l).kind == TK_MUL) {
            lexer_next(l);
            NodeTree *b = primary(l);
            if (!b) return NULL;
            a = (NodeTree *)node_mul_create(a, b);
        } else if (lexer_current(l).kind == TK_DIV) {
            lexer_next(l);
            NodeTree *b = primary(l);
            if (!b) return NULL;
            a = (NodeTree *)node_div_create(a, b);
        } else {
            return a;
        }
    }
}

NodeTree *primary(Lexer *l)
{
    NodeTree *factor(Lexer *);

    NodeTree *a = factor(l);
    if (!a) return NULL;
    while (true) {
        if (lexer_current(l).kind == TK_POW) {
            lexer_next(l);
            NodeTree *b = factor(l);
            if (!b) return NULL;
            a = (NodeTree *)node_pow_create(a, b);
        } else {
            return a;
        }
    }
}

NodeTree *factor(Lexer *l)
{
    Token curr_tk = lexer_current(l);
    if (curr_tk.kind == TK_INT || curr_tk.kind == TK_DEC) {
        lexer_next(l);
        if (curr_tk.kind == TK_INT)
            return (NodeTree *)node_number_create(token_get_int(&curr_tk));
        else if (curr_tk.kind == TK_DEC)
            return (NodeTree *)node_number_create(token_get_dec(&curr_tk));
    } else if (curr_tk.kind == TK_ID) {
        lexer_next(l);
        return (NodeTree *)node_id_create(token_get_string(&curr_tk));
    } else if (curr_tk.kind == TK_MINUS) {
        lexer_next(l);
        NodeTree *f = factor(l);
        if (!f) return NULL;
        return (NodeTree *)node_negate_create(f);
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
        FUNC func_kind = token_get_func(&func_tk);
        func = node_func_create(e, func_kind);
        if (lexer_current(l).kind != TK_CLOSEP) {
            printf("%d\n", lexer_current(l).kind);
            fprintf(stderr, "ERROR (parser): unmatching ) for function\n");
            return NULL;
        }
        lexer_next(l);
        return (NodeTree *)func;
    } else if (curr_tk.kind == TK_ERROR) {
        fprintf(stderr, "ERROR (lexer): %s\n", token_get_string(&curr_tk));
        return NULL;
    } else {
        fprintf(stderr, "ERROR (parser): unknown token\n");
        return NULL;
    }
    return NULL; // Unrechable but silences the warning
}

NodeTree *tree_parse(const char *src)
{
    Lexer lexer = lexer_create(src);
    NodeTree *result = expression(&lexer);
    if (!result)
        return NULL;
    if (lexer_current(&lexer).kind != TK_EOF) {
        fprintf(stderr, "ERROR (parser): invalid expression\n");
        return NULL;
    }
    return result;
}

void tree_print(NodeTree *tree)
{
    char buffer[PRINT_BUFFER_CAP] = {0};
    tree->vtable->print(tree, buffer);
    printf("%s\n", buffer);
}

float tree_eval(NodeTree *tree, float x)
{
    return tree->vtable->eval(tree, x);
}

void tree_free(NodeTree *tree)
{
    tree->vtable->free(tree);
}

#ifdef PARSER_MAIN
int main(void)
{
    char *src = "1 + 2";

    NodeTree *result = tree_parse(src);
    if (!result) return 1;

    tree_print(result);
    printf("%.2f\n", tree_eval(result, 0));

    tree_free(result);

    return 0;
}
#endif
