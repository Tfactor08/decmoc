#ifndef PARSER_H
#define PARSER_H

#define NODETREE_HEAD \
    VTable *vtable;   \

typedef struct {
    void (*print)(void *self, char *buffer);
    float (*eval)(void *self, float x);
    void (*free)(void *self);
} VTable;

typedef struct {
    NODETREE_HEAD
} NodeTree;

NodeTree *parse(const char *src);
char *print(NodeTree *tree);
float eval(NodeTree *tree, float x);
void destroy();

#endif
