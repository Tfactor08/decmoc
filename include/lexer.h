#ifndef LEXER_H
#define LEXER_H

#define MAX_STRING_LEN 256
#define PRINT_BUFFER_CAP (1 << 8)

#define SIN_STR "sin"
#define COS_STR "cos"
#define EXP_STR "exp"
#define LOG_STR "ln"

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
    EXP,
    LOG
} FUNC; 

typedef enum {
    PI,
    E,
    PHI
} CONST; 

typedef struct Lexer Lexer;
typedef struct Token Token;

typedef struct {
    char str[PRINT_BUFFER_CAP];
} LexerPrintBuffer;

struct Token {
    TOKEN_KIND kind;
    union {
        int   i;
        float f;
        char  c;
        char  s[MAX_STRING_LEN+1];
    } value;
    LexerPrintBuffer (*print)(Token *self);
};

LexerPrintBuffer lexer_token_print(Token *tk);

int lexer_token_int_get(Token *tk);
float lexer_token_dec_get(Token *tk);
FUNC lexer_token_func_get(Token *tk);
CONST lexer_token_const_get(Token *tk);
char *lexer_token_error_get(Token *tk);
char lexer_token_literal_get(Token *tk);
char lexer_token_var_get(Token *tk);

Lexer *lexer_create(const char *content);
Token lexer_next(Lexer *l);
Token lexer_current(Lexer *l);
Token lexer_peek(Lexer *l);

#endif // LEXER_H
