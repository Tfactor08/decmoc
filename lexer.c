/*
Id:      {char}+
Integer: {digit}+(.{digit}+)? | .{digit}+
Func:    sin | cos
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define MAX_STRING_LEN 256

#define SIN_STR "sin"
#define COS_STR "cos"

typedef enum {
    TK_ID = 1,
    TK_INT,
    TK_DEC,
    TK_OPENP,
    TK_CLOSEP,
    TK_PLUS,
    TK_MINUS,
    TK_MULT,
    TK_DIV,
    TK_POWER,
    TK_FUNC,
    TK_EOF,
    TK_ERROR
} TokenKind;

typedef struct Token {
    TokenKind kind;
    union {
        int   i;
        float f;
        char  c;
        char  s[MAX_STRING_LEN+1];
    } value;
    void (*print)(struct Token *self);
} Token;

typedef struct {
    const char *content;
    size_t count;
    size_t pos;
    Token current;
    Token next;
} Lexer;

static void token_print_int(Token *tk)
{
    printf("Int: %d\n", tk->value.i);
}

static void token_print_dec(Token *tk)
{
    printf("Dec: %f\n", tk->value.f);
}

static void token_print_literal(Token *tk)
{
    printf("Literal: %c\n", tk->value.c);
}

static void token_print_id(Token *tk)
{
    printf("Id: %s\n", tk->value.s);
}

static void token_print_func(Token *tk)
{
    printf("Func: %s\n", tk->value.s);
}

static void token_print_error(Token *tk)
{
    printf("Error: %s\n", tk->value.s);
}

static Token token_create_int(int val)
{
    Token tk = { .kind = TK_INT, .value.i = val, .print = &token_print_int };
    return tk;
}

static Token token_create_dec(float val)
{
    Token tk = { .kind = TK_DEC, .value.f = val, .print = &token_print_dec };
    return tk;
}

static Token token_create_literal(TokenKind kind, char val)
{
    assert(kind == TK_OPENP || kind == TK_CLOSEP || kind == TK_PLUS || kind == TK_MINUS || kind == TK_MULT || kind == TK_DIV || kind == TK_POWER);
    Token tk = { .kind = kind, .value.c = val, .print = &token_print_literal };
    return tk;
}

static Token token_create_string(TokenKind kind, const char *val)
{
    assert(kind == TK_ID || kind == TK_FUNC || kind == TK_ERROR);
    assert(strlen(val) <= MAX_STRING_LEN);
    Token tk = { .kind = kind };
    strcpy(tk.value.s, val);
    if (kind == TK_ID) tk.print = &token_print_id;
    else if (kind == TK_FUNC) tk.print = &token_print_func;
    else if (kind == TK_ERROR) tk.print = &token_print_error;
    return tk;
}

static Token token_create_eof()
{
    Token tk = { .kind = TK_EOF };
    return tk;
}

static Token token_create_error(const char *msg)
{
    assert(strlen(msg) <= MAX_STRING_LEN);
    Token tk = { .kind = TK_ERROR };
    strcpy(tk.value.s, msg);
    return tk;
}

int token_get_int(Token *tk)
{
    assert(tk->kind == TK_INT);
    return tk->value.i;
}

float token_get_dec(Token *tk)
{
    assert(tk->kind == TK_DEC);
    return tk->value.f;
}

char *token_get_string(Token *tk)
{
    assert(tk->kind == TK_ID || tk->kind == TK_FUNC || tk->kind == TK_ERROR);
    return tk->value.s;
}

char token_get_literal(Token *tk)
{
    assert(tk->kind == TK_OPENP || tk->kind == TK_CLOSEP || tk->kind == TK_PLUS || tk->kind == TK_MINUS || tk->kind == TK_MULT || tk->kind == TK_DIV || tk->kind == TK_POWER);
    return tk->value.c;
}

static Token token_next(Lexer *l)
{
    while (l->pos < l->count && isspace(l->content[l->pos]))
        l->pos += 1;
    if (l->pos >= l->count)
        return token_create_eof();
    char c = l->content[l->pos];
    char next = '\0';
    if (l->pos+1 < l->count)
        next = l->content[l->pos + 1];
    if (isdigit(c) || (c == '-' && (isdigit(next) || next == '.')) || (c == '.' && isdigit(next))) {
        size_t start = l->pos;
        int is_float = 0;
        if (c == '-')
            l->pos += 1;
        while (l->pos < l->count && isdigit(l->content[l->pos]))
            l->pos += 1;
        if (l->pos < l->count && l->content[l->pos] == '.') {
            l->pos += 1;
            is_float = 1;
            while (l->pos < l->count && isdigit(l->content[l->pos]))
                l->pos += 1;
        }
        size_t num_len = l->pos - start;
        if (num_len > MAX_STRING_LEN) {
            char error_msg[MAX_STRING_LEN+1];
            snprintf("max number length is %d\n", MAX_STRING_LEN+1, error_msg, MAX_STRING_LEN);
            return token_create_error(error_msg);
        }
        const char *num_start = l->content + start;
        char num_str[MAX_STRING_LEN+1];
        memcpy(num_str, num_start, num_len);
        num_str[num_len] = '\0';
        if (!is_float) {
            int num = atoi(num_str);
            return token_create_int(num);
        } else {
            float num = atof(num_str);
            return token_create_dec(num);
        }
    } else if (isalpha(l->content[l->pos])) {
        size_t start = l->pos;
        while (l->pos < l->count && isalpha(l->content[l->pos]))
            l->pos += 1;
        size_t string_len = l->pos - start;
        if (string_len > MAX_STRING_LEN) {
            char error_msg[MAX_STRING_LEN+1];
            snprintf(error_msg, MAX_STRING_LEN+1, "max id length is %d\n", MAX_STRING_LEN);
            return token_create_error(error_msg);
        }
        const char *string_start = l->content + start;
        char string[MAX_STRING_LEN];
        memcpy(string, string_start, string_len);
        string[string_len] = '\0';
        if (strcmp(string, SIN_STR) == 0 || strcmp(string, COS_STR) == 0)
            return token_create_string(TK_FUNC, string);
        else
            return token_create_string(TK_ID, string);
    } else if (c == '+') {
        l->pos += 1;
        return token_create_literal(TK_PLUS, c);
    } else if (c == '-') {
        l->pos += 1;
        return token_create_literal(TK_MINUS, c);
    } else if (c == '*') {
        l->pos += 1;
        return token_create_literal(TK_MULT, c);
    } else if (c == '/') {
        l->pos += 1;
        return token_create_literal(TK_DIV, c);
    } else if (c == '^') {
        l->pos += 1;
        return token_create_literal(TK_POWER, c);
    } else if (c == '(') {
        l->pos += 1;
        return token_create_literal(TK_OPENP, c);
    } else if (c == ')') {
        l->pos += 1;
        return token_create_literal(TK_CLOSEP, c);
    } else {
        char error_msg[MAX_STRING_LEN+1];
        snprintf(error_msg, MAX_STRING_LEN+1, "unexpected symbol %c at %zu\n", c, l->pos);
        return token_create_error(error_msg);
    }
}

Token lexer_next(Lexer *l)
{
    l->current = l->next;
    l->next = token_next(l);
    return l->current;
}

Token lexer_current(Lexer *l)
{
    return l->current;
}

Token lexer_peek(Lexer *l)
{
    return l->next;
}

Lexer lexer_create(const char *content)
{
    Lexer l = {
        .content = content,
        .count = strlen(content),
    };
    l.next = token_next(&l);
    return l;
}

#if 0
int main(void)
{
    char *source = "sin * cos x - -.69 + duck / 2^duck?";
    Lexer lexer = lexer_create(source);
    Token tk = {0};

    printf("%s\n", source);
    while ((tk = lexer_next(&lexer)).kind != TK_EOF) {
        if (tk.kind == TK_ERROR) {
            fprintf(stderr, "ERROR (LEXER): %s\n", token_get_string(&tk));
            return 1;
        }
        tk.print(&tk);
    }
}
#endif
