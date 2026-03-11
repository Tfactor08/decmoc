#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#define NODETREE_HEAD                   \
    float (*eval)(void *self, float x); \
    char  *(*print)(void *self);        \

typedef struct {
    NODETREE_HEAD
} NodeTree;

NodeTree *parse(const char *src);
char *print(NodeTree *tree);
float eval(NodeTree *tree, float x);
void destroy();

#endif
