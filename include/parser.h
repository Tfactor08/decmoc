#ifndef PARSER_H
#define PARSER_H

#define MAX_PARAMS (1 << 4)

typedef struct ParserTree ParserTree;

// NOTE: Interface to the inner Params struct.
typedef struct {
    size_t count;
    char param_list[MAX_PARAMS];
} ParserParams;

ParserTree*
parser_parse(const char *src);

void
parser_print(const ParserTree *tree);

float
parser_eval(const ParserTree *tree, float x);

void
parser_free(const ParserTree *tree);

ParserParams
parser_get_params(const ParserTree *tree);

void
parser_set_param(ParserTree *tree, char param, float value);

#endif // PARSER_H
