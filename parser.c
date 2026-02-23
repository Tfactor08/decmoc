/*
E -> T {+|- T}
T -> P {*|/ P}
P -> F {^ F}
F -> Id | Integer | (E) | -F | Func(E)
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.c"
#include "utils.c"

#define NODETREE_HEAD \
    float (*eval)(void *self); \
    char  *(*print)(void *self);

typedef struct {
    NODETREE_HEAD
} NodeTree;

typedef struct {
    NODETREE_HEAD
    NodeTree *left;
    NodeTree *right;
} NodeAdd;

typedef struct {
    NODETREE_HEAD
    NodeTree *left;
    NodeTree *right;
} NodeSubtract;

typedef struct {
    NODETREE_HEAD
    NodeTree *arg;
} NodeNegate;

typedef struct {
    NODETREE_HEAD
    int val;
} NodeInteger;

NodeAdd *node_add_create(NodeTree *left, NodeTree *right)
{
    float node_add_eval(void *);
    char *node_add_print(void *);

    NodeAdd *node = malloc(sizeof(NodeAdd));
    node->eval = &node_add_eval;
    node->print = &node_add_print;
    node->left = left;
    node->right = right;
    return node;
}

NodeAdd *node_subtract_create(NodeTree *left, NodeTree *right)
{
    float node_subtract_eval(void *);
    char *node_subtract_print(void *);

    NodeAdd *node = malloc(sizeof(NodeAdd));
    node->eval = &node_subtract_eval;
    node->print = &node_subtract_print;
    node->left = left;
    node->right = right;
    return node;
}

NodeNegate *node_negate_create(NodeTree *arg)
{
    float node_negate_eval(void *);
    char *node_negate_print(void *);

    NodeNegate *node = malloc(sizeof(NodeNegate));
    node->eval = &node_negate_eval;
    node->print = &node_negate_print;
    node->arg = arg;
    return node;
}

NodeInteger *node_integer_create(int val)
{
    float node_integer_eval(void *);
    char *node_integer_print(void *);

    NodeInteger *node = malloc(sizeof(NodeInteger));
    node->eval = &node_integer_eval;
    node->print = &node_integer_print;
    node->val = val;
    return node;
}

float node_add_eval(void *self)
{
    NodeAdd *node = (NodeAdd *)self;
    NodeTree *l = node->left;
    NodeTree *r = node->right;
    return l->eval(l) + r->eval(r);
}

float node_subtract_eval(void *self)
{
    NodeSubtract *node = (NodeSubtract *)self;
    NodeTree *l = node->left;
    NodeTree *r = node->right;
    return l->eval(l) - r->eval(r);
}

float node_negate_eval(void *self)
{
    NodeNegate *node = (NodeNegate *)self;
    NodeTree *arg = node->arg;
    return -(arg->eval(arg));
}

float node_integer_eval(void *self)
{
    NodeInteger *node = (NodeInteger *)self;
    return node->val;
}

char *node_add_print(void *self)
{
    NodeAdd *node = (NodeAdd *)self;
    char *right_string = node->right->print(node->right);
    char *left_string = node->left->print(node->left);
    size_t result_len = strlen(left_string) + strlen(right_string) + 6;
    char *result = malloc(sizeof(char) * result_len);
    sprintf(result, "(%s + %s)", left_string, right_string);
    return result;
}

char *node_subtract_print(void *self)
{
    NodeSubtract *node = (NodeSubtract *)self;
    char *right_string = node->right->print(node->right);
    char *left_string = node->left->print(node->left);
    size_t result_len = strlen(left_string) + strlen(right_string) + 6;
    char *result = malloc(sizeof(char) * result_len);
    sprintf(result, "(%s - %s)", left_string, right_string);
    return result;
}

char *node_negate_print(void *self)
{
    NodeNegate *node = (NodeNegate *)self;
    char *arg_string = node->arg->print(node->arg);
    size_t result_len = strlen(arg_string) + 4;
    char *result = malloc(sizeof(char) * result_len);
    sprintf(result, "-(%s)", arg_string);
    return result;
}

char *node_integer_print(void *self)
{
    NodeInteger *node = (NodeInteger *)self;
    char *result = malloc(sizeof(char) * int_len(node->val));
    itoa(node->val, result);
    return result;
}

NodeTree *expression(Lexer *l)
{
    NodeTree *factor(Lexer *);

    NodeTree *a = factor(l);
    while (true) {
        lexer_next(l);
        if (lexer_current(l).kind == TK_PLUS) {
            NodeTree *b = factor(l);
            a = (NodeTree *)node_add_create(a, b);
        } else if (lexer_current(l).kind == TK_MINUS) {
            NodeTree *b = factor(l);
            a = (NodeTree *)node_subtract_create(a, b);
        } else {
            return a;
        }
    }
}

NodeTree *factor(Lexer *l)
{
    Token tk = lexer_next(l);
    if (tk.kind == TK_INT) {
        return (NodeTree *)node_integer_create(token_get_int(&tk));
    } else if (tk.kind == TK_MINUS) {
        return (NodeTree *)node_negate_create(factor(l));
    } else if (tk.kind == TK_OPENP) {
        NodeTree *a = expression(l);
        if (lexer_current(l).kind == TK_CLOSEP) {
            return a;
        } else {
            fprintf(stderr, "ERROR: unmatching )\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "ERROR: unknown token\n");
        exit(1);
    }
}

int main(void)
{
    char *source = "2 + 3";
    Lexer lexer = lexer_create(source);

    NodeTree *result = expression(&lexer);
    if (lexer_next(&lexer).kind != TK_EOF) {
        fprintf(stderr, "ERROR: smth went wrong\n");
        exit(1);
    }
    printf("%f\n", result->eval(result));

    return 0;
}
