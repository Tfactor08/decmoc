#ifndef PARSER_H
#define PARSER_H

#define PRINT_BUFFER_CAP (1 << 8)
#define MAX_PARAMS (1 << 4)

#define NODETREE_HEAD \
    VTable *vtable    \

typedef struct {
    char str[PRINT_BUFFER_CAP];
} PrintBuffer;

// TODO: maybe create "Params"-related functions like "set_param(...)" instead of
//       requiring user to do "params.param_to_value[(int) param] = value" manually?
typedef struct {
    size_t count;
    char params[MAX_PARAMS];
    float param_to_value[1 << 7];
} Params;

typedef struct {
    PrintBuffer (*print)(void *self);
    float (*eval)(void *self, float x, Params *params);
    void (*free)(void *self);
} VTable;

typedef struct {
    NODETREE_HEAD;
} NodeTree;

NodeTree *tree_parse(const char *src, Params *params);
void tree_print(NodeTree *tree);
float tree_eval(NodeTree *tree, float x, Params *params);
void tree_free(NodeTree *tree);

#endif
