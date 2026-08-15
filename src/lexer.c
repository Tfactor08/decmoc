/*
Var:      char
Number:   {digit}+(.{digit}+)? | .{digit}+
Function: sin | cos | exp
Constant: PI/pi | E/e | PHI/phi
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#define MAX_STRING_LEN 256
#define PRINT_BUFFER_CAP (1 << 8)

#define SIN_STR "sin"
#define COS_STR "cos"
#define EXP_STR "exp"

#define PI_STR  "pi"
#define E_STR   "e"
#define PHI_STR "phi"

typedef enum {
    TK_VAR = 1,
    TK_INT,
    TK_DEC,
    TK_OPENP,
    TK_CLOSEP,
    TK_PLUS,
    TK_MINUS,
    TK_MUL,
    TK_DIV,
    TK_POW,
    TK_FUNC,
    TK_CONST,
    TK_EOF,
    TK_ERROR
} TOKEN_KIND;

typedef enum {
    SIN,
    COS,
    EXP
} FUNC; 

typedef enum {
    PI,
    E,
    PHI
} CONST; 

typedef struct {
    char str[PRINT_BUFFER_CAP];
} LexPrintBuffer;

typedef struct Token Token;

struct Token {
    TOKEN_KIND kind;
    union {
        int   i;
        float f;
        char  c;
        char  s[MAX_STRING_LEN+1];
    } value;
    LexPrintBuffer (*print)(Token *self);
};

typedef struct {
    const char *content;
    size_t count;
    size_t pos;
    Token current;
    Token next;
} Lexer;

static char char_to_token[] = {
    ['+'] = TK_PLUS,
    ['-'] = TK_MINUS,
    ['*'] = TK_MUL,
    ['/'] = TK_DIV,
    ['^'] = TK_POW,
    ['('] = TK_OPENP,
    [')'] = TK_CLOSEP
};

static bool is_literal(TOKEN_KIND kind)
{
    static const TOKEN_KIND literal_tks[] = {
        TK_OPENP, TK_CLOSEP, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_POW
    };
    static const size_t literal_tks_count = sizeof(literal_tks) / sizeof(literal_tks[0]);
    for (size_t i = 0; i < literal_tks_count; i++) 
        if (kind == literal_tks[i])
            return true;
    return false;
}

/* --- PRINT FUNCTIONS --- */

#define MAKE_PRINT_FUNC(name, desc, format, field)            \
    static LexPrintBuffer token_##name##_print(Token *tk)     \
    {                                                         \
        LexPrintBuffer buf = {0};                             \
        snprintf(buf.str, PRINT_BUFFER_CAP, desc ": " format, \
                 tk->value.field) < 0 ?                       \
                 exit(EXIT_FAILURE) : (void) 0;               \
        return buf;                                           \
    }

MAKE_PRINT_FUNC(int,     "Int",     "%d",   i);
MAKE_PRINT_FUNC(dec,     "Dec",     "%.2f", f);
MAKE_PRINT_FUNC(literal, "Literal", "%c",   c);
MAKE_PRINT_FUNC(var,     "Var",     "%c",   c);
MAKE_PRINT_FUNC(error,   "Error",   "%s",   s);

static LexPrintBuffer token_func_print(Token *tk)
{
    LexPrintBuffer buf = {0};
    switch (tk->value.i) {
        case SIN:
            snprintf(buf.str, PRINT_BUFFER_CAP, "Func: %s", SIN_STR);
            break;
        case COS:
            snprintf(buf.str, PRINT_BUFFER_CAP, "Func: %s", COS_STR);
            break;
        case EXP:
            snprintf(buf.str, PRINT_BUFFER_CAP, "Func: %s", EXP_STR);
            break;
        default:
            assert(0 && "Unhandled function\n");
    }
    return buf;
}

static LexPrintBuffer token_const_print(Token *tk)
{
    LexPrintBuffer buf = {0};
    switch (tk->value.i) {
        case PI:
            snprintf(buf.str, PRINT_BUFFER_CAP, "Const: %s", PI_STR);
            break;
        case E:
            snprintf(buf.str, PRINT_BUFFER_CAP, "Const: %s", E_STR);
            break;
        case PHI:
            snprintf(buf.str, PRINT_BUFFER_CAP, "Const: %s", PHI_STR);
            break;
    }
    return buf;
}

static LexPrintBuffer token_eof_print(Token *tk)
{
    LexPrintBuffer buf = {0};
    snprintf(buf.str, PRINT_BUFFER_CAP, "EOF");
    return buf;
}

/* --- MAKE FUNCTIONS --- */

static Token token_var_make(char val)
{
    Token tk = { .kind = TK_VAR, .value.c = val, .print = &token_var_print };
    return tk;
}

static Token token_int_make(int val)
{
    Token tk = { .kind = TK_INT, .value.i = val, .print = &token_int_print };
    return tk;
}

static Token token_dec_make(float val)
{
    Token tk = { .kind = TK_DEC, .value.f = val, .print = &token_dec_print };
    return tk;
}

static Token token_func_make(FUNC func)
{
    Token tk = { .kind = TK_FUNC, .value.i = func, .print = &token_func_print };
    return tk;
}

static Token token_literal_make(TOKEN_KIND kind, char val)
{
    assert(is_literal(kind));
    Token tk = { .kind = kind, .value.c = val, .print = &token_literal_print };
    return tk;
}

static Token token_const_make(CONST _const)
{
    Token tk = { .kind = TK_CONST, .value.i = _const, .print = &token_const_print };
    return tk;
}

static Token token_eof_make()
{
    Token tk = { .kind = TK_EOF };
    tk.print = &token_eof_print;
    return tk;
}

static Token token_error_make(const char *msg)
{
    assert(strlen(msg) <= MAX_STRING_LEN);
    Token tk = { .kind = TK_ERROR };
    tk.print = &token_error_print;
    strcpy(tk.value.s, msg);
    return tk;
}

/* --- GET FUNCTIONS --- */

int token_int_get(Token *tk)
{
    assert(tk->kind == TK_INT);
    return tk->value.i;
}

float token_dec_get(Token *tk)
{
    assert(tk->kind == TK_DEC);
    return tk->value.f;
}

FUNC token_func_get(Token *tk)
{
    assert(tk->kind == TK_FUNC);
    return tk->value.i;
}

CONST token_const_get(Token *tk)
{
    assert(tk->kind == TK_CONST);
    return tk->value.i;
}

char *token_error_get(Token *tk)
{
    assert(tk->kind == TK_ERROR);
    return tk->value.s;
}

char token_literal_get(Token *tk)
{
    assert(is_literal(tk->kind));
    return tk->value.c;
}

char token_var_get(Token *tk)
{
    assert(tk->kind == TK_VAR);
    return tk->value.c;
}

#define RETURN_TOKEN(make, ...)  \
    prev_tk = make(__VA_ARGS__); \
    return prev_tk;

static Token token_next(Lexer *l)
{
    while (l->pos < l->count && isspace(l->content[l->pos]))
        l->pos += 1;
    if (l->pos >= l->count)
        return token_eof_make();

    static Token prev_tk;
    char c = l->content[l->pos];
    char next_char = '\0';

    if (l->pos+1 < l->count)
        next_char = l->content[l->pos + 1];
    if (isdigit(c) || (c == '-' && (prev_tk.kind != TK_INT && prev_tk.kind != TK_DEC) && (isdigit(next_char) || next_char == '.')) || (c == '.' && isdigit(next_char))) {
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
            RETURN_TOKEN(token_error_make, error_msg);
        }
        const char *num_start = l->content + start;
        char num_str[MAX_STRING_LEN+1];
        memcpy(num_str, num_start, num_len);
        num_str[num_len] = '\0';
        if (!is_float) {
            int num = atoi(num_str);
            RETURN_TOKEN(token_int_make, num);
        } else {
            float num = atof(num_str);
            RETURN_TOKEN(token_dec_make, num);
        }
    } else if (isalpha(l->content[l->pos])) {
        const char *str_ptr = l->content + l->pos;
        // TODO: this condition chain can be shrinked
        if (strncasecmp(str_ptr, SIN_STR, strlen(SIN_STR)) == 0) {
            l->pos += strlen(SIN_STR);
            RETURN_TOKEN(token_func_make, SIN);
        } else if (strncasecmp(str_ptr, COS_STR, strlen(COS_STR)) == 0) {
            l->pos += strlen(COS_STR);
            RETURN_TOKEN(token_func_make, COS);
        } else if (strncasecmp(str_ptr, EXP_STR, strlen(EXP_STR)) == 0) {
            l->pos += strlen(EXP_STR);
            RETURN_TOKEN(token_func_make, EXP);
        } else if (strncasecmp(str_ptr, PI_STR, strlen(PI_STR)) == 0) {
            l->pos += strlen(PI_STR);
            RETURN_TOKEN(token_const_make, PI);
        } else if (strncasecmp(str_ptr, E_STR, strlen(E_STR)) == 0) {
            l->pos += strlen(E_STR);
            RETURN_TOKEN(token_const_make, E);
        } else if (strncasecmp(str_ptr, PHI_STR, strlen(PHI_STR)) == 0) {
            l->pos += strlen(PHI_STR);
            RETURN_TOKEN(token_const_make, PHI);
        } else {
            l->pos += 1;
            RETURN_TOKEN(token_var_make, *str_ptr);
        }
    } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '(' || c == ')') {
        l->pos += 1;
        RETURN_TOKEN(token_literal_make, char_to_token[(int) c], c);
    } else {
        char error_msg[MAX_STRING_LEN+1];
        snprintf(error_msg, MAX_STRING_LEN+1, "unexpected symbol %c at %zu", c, l->pos);
        RETURN_TOKEN(token_error_make, error_msg);
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
    l.current = token_next(&l);
    l.next = token_next(&l);
    return l;
}

#ifdef MAIN
int main(void)
{
    char *expr = "1-1 + xsincos * cosEXP - -.69 + PI / 2^d?";
    Lexer lexer = lexer_create(expr);
    Token tk = {0};

    printf("%s\n", expr);
    do {
        tk = lexer_current(&lexer);
        if (tk.kind == TK_ERROR) {
            fprintf(stderr, "ERROR (LEXER): %s\n", token_error_get(&tk));
            return 1;
        }
        LexPrintBuffer res = tk.print(&tk);
        printf("%s\n", res.str);
    } while ((tk = lexer_next(&lexer)).kind != TK_EOF);
}
#endif
