/*
E -> T {+|- T}
T -> P {*|/ P}
P -> F {^ F}
F -> Id | Integer | (E) | -F | Func(E)
*/

#include "lexer.c"
#include "arena.c"
#include "utils.c"

#include "parser.h"

#define INITIAL_MEM (1 << 10)

typedef struct {
    NODETREE_HEAD
    NodeTree *left;
    NodeTree *right;
} NodeBinary;

typedef enum {
    SIN,
    COS
} FUNC; 

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
    char *string;
} NodeId;

typedef struct {
    NODETREE_HEAD
    float val;
} NodeNumber;

static Arena arena;

#define NODE_BINARY_CREATE_FUNC(name)                                 \
    NodeBinary *node_##name##_create(NodeTree *left, NodeTree *right) \
    {                                                                 \
        float node_##name##_eval(void *, float);                      \
        char *node_##name##_print(void *);                            \
        NodeBinary *node = PUSH_STRUCT(&arena, NodeBinary);           \
        node->eval = &node_##name##_eval;                             \
        node->print = &node_##name##_print;                           \
        node->left = left;                                            \
        node->right = right;                                          \
        return node;                                                  \
    }                                                                 \

#define NODE_BINARY_EVAL_FUNC(name, operator)        \
    float node_##name##_eval(void *self, float x)    \
    {                                                \
        NodeBinary *node = (NodeBinary *)self;       \
        NodeTree *l = node->left;                    \
        NodeTree *r = node->right;                   \
        return l->eval(l, x) operator r->eval(r, x); \
    }                                                \

#define NODE_BINARY_PRINT_FUNC(name, operator)                               \
    char *node_##name##_print(void *self)                                    \
    {                                                                        \
        NodeBinary *node = (NodeBinary *)self;                               \
        char *right_string = node->right->print(node->right);                \
        char *left_string = node->left->print(node->left);                   \
        size_t result_len = strlen(left_string) + strlen(right_string) + 6;  \
        char *result = PUSH_STRING(&arena, result_len);                      \
        sprintf(result, "(%s " #operator " %s)", left_string, right_string); \
        return result;                                                       \
    }                                                                        \

NODE_BINARY_CREATE_FUNC(add);
NODE_BINARY_CREATE_FUNC(sub);
NODE_BINARY_CREATE_FUNC(mult);
NODE_BINARY_CREATE_FUNC(div);

NodeFunc *node_func_create(NodeTree *arg, FUNC func)
{
    float node_func_eval(void *, float);
    char *node_func_print(void *);

    // TODO: use arena
    NodeFunc *node = malloc(sizeof(NodeFunc));
    node->eval = &node_func_eval;
    node->print = &node_func_print;
    node->func = func;
    node->arg = arg;
    return node;
}

NodeNegate *node_negate_create(NodeTree *arg)
{
    float node_negate_eval(void *, float);
    char *node_negate_print(void *);

    // TODO: use arena
    NodeNegate *node = malloc(sizeof(NodeNegate));
    node->eval = &node_negate_eval;
    node->print = &node_negate_print;
    node->arg = arg;
    return node;
}

NodeNumber *node_number_create(float val)
{
    float node_number_eval(void *, float);
    char *node_number_print(void *);

    NodeNumber *node = PUSH_STRUCT(&arena, NodeNumber);
    node->eval = &node_number_eval;
    node->print = &node_number_print;
    node->val = val;
    return node;
}

NodeId *node_id_create(char *string)
{
    float node_id_eval(void *, float);
    char *node_id_print(void *);

    // TODO: use arena
    NodeId *node = malloc(sizeof(NodeId));
    node->eval = &node_id_eval;
    node->print = &node_id_print;
    // TODO: use arena
    node->string = malloc(sizeof(char) * (strlen(string) + 1));
    strcpy(node->string, string);
    return node;
}

NODE_BINARY_EVAL_FUNC(add, +);
NODE_BINARY_EVAL_FUNC(sub, -);
NODE_BINARY_EVAL_FUNC(mult, *);
NODE_BINARY_EVAL_FUNC(div, /);

float node_func_eval(void *self, float x)
{
    NodeFunc *node = (NodeFunc *)self;
    NodeTree *arg = node->arg;
    FUNC func = node->func;
    if (func == SIN)
        return sin(arg->eval(arg, x));
    else if (func == COS)
        return cos(arg->eval(arg, x));
    else
        assert(0 && "Unhandled function");
}

float node_negate_eval(void *self, float x)
{
    NodeNegate *node = (NodeNegate *)self;
    NodeTree *arg = node->arg;
    return -(arg->eval(arg, x));
}

float node_number_eval(void *self, float x)
{
    NodeNumber *node = (NodeNumber *)self;
    return node->val;
}

float node_id_eval(void *self, float x)
{
    return x;
}

NODE_BINARY_PRINT_FUNC(add, +);
NODE_BINARY_PRINT_FUNC(sub, -);
NODE_BINARY_PRINT_FUNC(mult, *);
NODE_BINARY_PRINT_FUNC(div, /);

char *node_func_print(void *self)
{
    NodeFunc *node = (NodeFunc *)self;
    char *arg_string = node->arg->print(node->arg);
    char *func_str;
    switch (node->func) {
        case SIN:
            func_str = "sin";
            break;
        case COS:
            func_str = "cos";
            break;
        default:
            assert(0 && "Unhandled function");
    }
    size_t result_len = strlen(arg_string) + strlen(func_str) + 5;
    // TODO: use arena
    char *result = malloc(sizeof(char) * result_len);
    sprintf(result, "(%s(%s))", func_str, arg_string);
    return result;
}

char *node_negate_print(void *self)
{
    NodeNegate *node = (NodeNegate *)self;
    char *arg_string = node->arg->print(node->arg);
    size_t result_len = strlen(arg_string) + 4;
    // TODO: use arena
    char *result = malloc(sizeof(char) * result_len);
    sprintf(result, "-(%s)", arg_string);
    return result;
}

char *node_number_print(void *self)
{
    NodeNumber *node = (NodeNumber *)self;
    size_t result_len = int_len((int)node->val) + 4;
    char *result = PUSH_STRING(&arena, result_len);
    sprintf(result, "%.2f", node->val);
    return result;
}

char *node_id_print(void *self)
{
    NodeId *node = (NodeId *)self;
    return node->string;
}

NodeTree *expression(Lexer *l)
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
    NodeTree *factor(Lexer *);

    NodeTree *a = factor(l);
    if (!a) return NULL;
    while (true) {
        if (lexer_current(l).kind == TK_MULT) {
            lexer_next(l);
            NodeTree *b = factor(l);
            if (!b) return NULL;
            a = (NodeTree *)node_mult_create(a, b);
        } else if (lexer_current(l).kind == TK_DIV) {
            lexer_next(l);
            NodeTree *b = factor(l);
            if (!b) return NULL;
            a = (NodeTree *)node_div_create(a, b);
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
        char *func_str = token_get_string(&func_tk);
        // TODO: inconsistency: in the lexer function token is defined by the corresponding string but in the parser
        // by the FUNC enum => we have to compare strings thus creating an overhead
        if (strcmp(func_str, SIN_STR) == 0)
            func = node_func_create(e, SIN);
        else if (strcmp(func_str, COS_STR) == 0)
            func = node_func_create(e, COS);
        else
            assert(0 && "Unhandled function");
        if (lexer_current(l).kind != TK_CLOSEP) {
            fprintf(stderr, "Unmatching ) for function");
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
}

NodeTree *parse(const char *src)
{
    if (!arena.current)
        arena_create(&arena, INITIAL_MEM);
    Lexer lexer = lexer_create(src);
    NodeTree *result = expression(&lexer);
    if (!result) {
        return NULL;
    }
    if (lexer_current(&lexer).kind != TK_EOF) {
        fprintf(stderr, "ERROR (parser): invalid expression\n");
        return NULL;
    }
    return result;
}

char *print(NodeTree *tree)
{
    return tree->print(tree);
}

float eval(NodeTree *tree, float x)
{
    return tree->eval(tree, x);
}

void destroy()
{
    arena_destroy(&arena);
}

#ifdef PARSER_MAIN
int main(void)
{
    char *src = "42 + 69";

    NodeTree *result = parse(src);
    if (!result) return 1;

    printf("%s\n", print(result));
    printf("%.2f\n", eval(result, 0));

    //NodeTree *result2 = parse("cos(0)");
    //if (!result2) return 1;

    //printf("%s\n", print(result2));
    //printf("%.2f\n", eval(result2, 0));

    destroy();

    return 0;
}
#endif
