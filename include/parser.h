#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#define NODETREE_HEAD                   \
    void (*free)(void *self);           \
    float (*eval)(void *self, float x); \
    char  *(*print)(void *self);        \

#define ZERO (1e-8)

//typedef enum {
//    NODE_BINARY = 1,
//    NODE_FUNC,
//    NODE_NEGATE,
//    NODE_ID,
//    NODE_NUMBER
//} NodeKind; 

typedef struct {
    NODETREE_HEAD
} NodeTree;

NodeTree *parse(const char *src);
char *print(NodeTree *tree);
float eval(NodeTree *tree, float x);
void destroy();

#endif
