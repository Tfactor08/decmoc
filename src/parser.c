/*
    E -> T {+|- T}
    T -> P {*|/ P} | PP{P}
    P -> F {^ F}
    F -> ID | NUMBER | (E) | -F | FUNC(E)
    FUNC: sin | cos | exp
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "lexer.c"
#include "utils/basic_utils.c"

#include "parser.h"

#define ZERO (1e-8)
#define FLOAT_PRECISION "2"

#define PRINT_BUFFER_CAP (1 << 8)

#define NODE_HEAD \
    VTable *vtable

// TODO: arrange struct definitions properly

typedef struct {
    char str[PRINT_BUFFER_CAP];
} PrintBuffer;

typedef struct {
    size_t count;
    char param_list[MAX_PARAMS];
    float param_to_value[1 << 6]; // Overall we need 58 array places (2*26 + 6 extra characters that lay in between)
} Params;

typedef struct {
    PrintBuffer (*print)(void *self);
    float (*eval)(void *self, float x, Params *params);
    void (*free)(void *self);
} VTable;

typedef struct {
    NODE_HEAD;
} Node;

typedef struct {
    NODE_HEAD;
    Node *left;
    Node *right;
} NodeBinary;

typedef struct {
    NODE_HEAD;
    FUNC func;
    Node *arg;
} NodeFunc;

typedef struct {
    NODE_HEAD;
    Node *arg;
} NodeNegate;

typedef struct {
    NODE_HEAD;
    char var;
} NodeVar;

typedef struct {
    NODE_HEAD;
    float value;
} NodeNumber;

// This is what tree_eval should return.
// This struct points to the root of the actual tree and contains necessary data
// about the whole tree (parameters, print buffer, etc).
struct ParserTree {
    Node *root;
    Params *params;
    // TODO: think about the PrintBuffer
};

static const float const_to_value[] = {
    [PI]  = 3.14f,
    [E]   = 2.71f,
    [PHI] = 1.62f
};

#define MAKE_NODE_BINARY_MAKE_FUNC(name)                           \
    static NodeBinary *node_##name##_make(Node *left, Node *right) \
    {                                                              \
        NodeBinary *node = malloc(sizeof(NodeBinary));             \
        MALLOC_CHECK(node);                                        \
        node->vtable = &node_##name##_vtable;                      \
        node->left = left;                                         \
        node->right = right;                                       \
        return node;                                               \
    }

#define MAKE_NODE_BINARY_EVAL_FUNC(name, operator)                                   \
    static float node_##name##_eval(void *self, float x, Params *params)             \
    {                                                                                \
        NodeBinary *node = self;                                                     \
        Node *l = node->left;                                                        \
        Node *r = node->right;                                                       \
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
    Node *l = node->left;
    Node *r = node->right;
    if (x == 0) x = ZERO; // avoid 0/0 indetermination
    return l->vtable->eval(l, x, params) / r->vtable->eval(r, x, params);
}

// 'node_pow_eval' is implemented separately from the common NODE_BINARY_EVAL
// since it requires a specific behaviour
static float node_pow_eval(void *self, float x, Params *params)
{
    NodeBinary *node = self;
    Node *l = node->left;
    Node *r = node->right;
    return powf(l->vtable->eval(l, x, params), r->vtable->eval(r, x, params));
}

static float node_func_eval(void *self, float x, Params *params)
{
    NodeFunc *node = self;
    Node *arg = node->arg;
    FUNC func = node->func;
    if (func == SIN)
        return sinf(arg->vtable->eval(arg, x, params));
    else if (func == COS)
        return cosf(arg->vtable->eval(arg, x, params));
    else if (func == EXP)
        return expf(arg->vtable->eval(arg, x, params));
    else if (func == LOG)
        return logf(arg->vtable->eval(arg, x, params));
    else
        assert(0 && "Unhandled function");
}

static float node_negate_eval(void *self, float x, Params *params)
{
    NodeNegate *node = self;
    Node *arg = node->arg;
    return -(arg->vtable->eval(arg, x, params));
}

static float node_number_eval(void *self, float x, Params *params)
{
    NodeNumber *node = self;
    return node->value;
}

// Used by 'node_var_eval'.
static float tree_get_param(Params *params, char param)
{
    assert(isalpha(param) && "Parameter must be a letter.");
    unsigned char param_index = param - 'A'; // this value guaranteed to be >= 0.
    return params->param_to_value[param_index];
}

static float node_var_eval(void *self, float x, Params *params)
{
    NodeVar *node = self;
    char var = node->var;
    if (var != 'x')
        return tree_get_param(params, var);
        //return params->param_to_value[(int) var];
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

static NodeFunc *node_func_make(Node *arg, FUNC func)
{
    NodeFunc *node = malloc(sizeof(NodeFunc));
    MALLOC_CHECK(node);
    node->vtable = &node_func_vtable;
    node->func = func;
    node->arg = arg;
    return node;
}

static NodeNegate *node_negate_make(Node *arg)
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

static Node *term(Lexer *, Params *);

// E -> T {+|- T}
static Node *expression(Lexer *l, Params *params)
{
    Node *a = term(l, params);
    if (!a) return NULL;
    while (true) {
        TOKEN_KIND tk_kind = lexer_current(l).kind;
        if (tk_kind == TK_PLUS) {
            lexer_next(l);
            Node *b = term(l, params);
            if (!b) return NULL;
            a = (Node *) node_add_make(a, b);
        } else if (tk_kind == TK_MINUS) {
            lexer_next(l);
            Node *b = term(l, params);
            if (!b) return NULL;
            a = (Node *) node_sub_make(a, b);
        } else {
            return a;
        }
    }
}

static bool is_factor(Token token)
{
    static const TOKEN_KIND factor_tks[] = {
        TK_VAR, TK_INT, TK_DEC, TK_OPENP, TK_FUNC, TK_CONST
    };
    for (size_t i = 0; i < ARRAY_LEN(factor_tks); i++)
        if (token.kind == factor_tks[i])
            return true;
    return false;
}

static Node *primary(Lexer *, Params *);
static Node *factor(Lexer *, Params *);

// T -> P {*|/ P} | PP{P}
static Node *term(Lexer *l, Params *params)
{
    Node *a = primary(l, params);
    if (!a) return NULL;
    Token curr_tk = lexer_current(l);
    if (is_factor(curr_tk)) {
        do {
            Node *b = primary(l, params);
            if (!b) return NULL;
            a = (Node *) node_mul_make(a, b);
        } while (is_factor(lexer_current(l)));
        return a;
    } else {
        while (true) {
            curr_tk = lexer_current(l);
            if (curr_tk.kind == TK_MUL) {
                lexer_next(l);
                Node *b = primary(l, params);
                if (!b) return NULL;
                a = (Node *) node_mul_make(a, b);
            } else if (curr_tk.kind == TK_DIV) {
                lexer_next(l);
                Node *b = primary(l, params);
                if (!b) return NULL;
                a = (Node *) node_div_make(a, b);
            } else {
                return a;
            }
        }
    }
}

static Node *factor(Lexer *, Params *);

// P -> F {^ F}
static Node *primary(Lexer *l, Params *params)
{
    Node *a = factor(l, params);
    if (!a) return NULL;
    while (true) {
        TOKEN_KIND tk_kind = lexer_current(l).kind;
        if (tk_kind == TK_POW) {
            lexer_next(l);
            Node *b = factor(l, params);
            if (!b) return NULL;
            a = (Node *) node_pow_make(a, b);
        } else {
            return a;
        }
    }
}

// F -> ID | NUMBER | (E) | -F | FUNC(E)
static Node *factor(Lexer *l, Params *params)
{
    Token curr_tk = lexer_current(l);
    if (curr_tk.kind == TK_INT) {
        lexer_next(l);
        return (Node *) node_number_make(token_int_get(&curr_tk));
    } else if (curr_tk.kind == TK_DEC) {
        lexer_next(l);
        return (Node *) node_number_make(token_dec_get(&curr_tk));
    } else if (curr_tk.kind == TK_CONST) {
        lexer_next(l);
        CONST constt = token_const_get(&curr_tk);
        return (Node *) node_number_make(const_to_value[constt]);
    } else if (curr_tk.kind == TK_VAR) {
        lexer_next(l);
        char var = token_var_get(&curr_tk);
        if (var != 'x')
            params->param_list[params->count++] = var;
        return (Node *) node_var_make(var);
    } else if (curr_tk.kind == TK_MINUS) {
        lexer_next(l);
        Node *f = factor(l, params);
        if (!f) return NULL;
        return (Node *) node_negate_make(f);
    } else if (curr_tk.kind == TK_OPENP) {
        lexer_next(l);
        Node *e = expression(l, params);
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
            fprintf(stderr, "ERROR (parser): ( expected after function name\n");
            return NULL;
        }
        lexer_next(l);
        Node *e = expression(l, params);
        if (!e) return NULL;
        FUNC func_kind = token_func_get(&func_tk);
        func = node_func_make(e, func_kind);
        if (lexer_current(l).kind != TK_CLOSEP) {
            fprintf(stderr, "ERROR (parser): unmatching ( for function\n");
            return NULL;
        }
        lexer_next(l);
        return (Node *) func;
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

ParserTree *parser_parse(const char *src)
{
    ParserTree *tree = malloc(sizeof(ParserTree));
    tree->params = calloc(1, sizeof(Params));

    Lexer lexer = lexer_create(src);

    Node *tree_root = expression(&lexer, tree->params);
    if (!tree_root)
        return NULL;
    if (lexer_current(&lexer).kind != TK_EOF) {
        fprintf(stderr, "ERROR (parser): invalid expression\n");
        tree_root->vtable->free(tree_root);
        return NULL;
    }

    tree->root = tree_root;
    return tree;
}

void parser_print(const ParserTree *tree)
{
    PrintBuffer result = tree->root->vtable->print(tree->root);
    printf("%s\n", result.str);
}

ParserParams parser_get_params(const ParserTree *tree)
{
    ParserParams params = {0};
    size_t param_count = tree->params->count; 
    params.count = param_count;
    memcpy(params.param_list, tree->params->param_list, param_count); 
    return params;
}

void parser_set_param(ParserTree *tree, char param, float value)
{
    assert(isalpha(param) && "Parameter must be a letter.");
    unsigned char param_index = param - 'A'; // This value guaranteed to be >= 0
    tree->params->param_to_value[param_index] = value;
}

float parser_eval(const ParserTree *tree, float x)
{
    return tree->root->vtable->eval(tree->root, x, tree->params);
}

void parser_free(const ParserTree *tree)
{
    tree->root->vtable->free(tree->root);
    free(tree->params);
}

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

        ParserTree *result = parser_parse(expr);
        if (!result) continue;

        parser_print(result);
        printf("%.2f\n", parser_eval(result, 1));

        parser_free(result);
    }

    return 0;
}
#endif
