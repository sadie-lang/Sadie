#ifndef sadie_lexer_h
#define sadie_lexer_h

#include "gb/gb.h"

#define TOKEN_LIT(tok) tok.len, tok.text

typedef enum TokenType {
	// single char tokens
    TOKEN_PLUS, TOKEN_MINUS, // +, -
    TOKEN_SLASH, TOKEN_STAR, // /, *
    TOKEN_GT, TOKEN_LT, // >, <
    TOKEN_EQ, TOKEN_COLON, // =, :
    TOKEN_HAT, TOKEN_AMP, // ^, &
    TOKEN_PERCENT, TOKEN_BANG, // %, !
    TOKEN_TAG, TOKEN_AT, // #, @
    TOKEN_TILDE, TOKEN_SEMI_COLON, // ~, ;
    TOKEN_LPAREN, TOKEN_RPAREN, // (, )
    TOKEN_LBRACKET, TOKEN_RBRACKET, // [, ]
    TOKEN_LBRACE, TOKEN_RBRACE, // {, }
    TOKEN_PIPE, TOKEN_QUESTION, // |, ?
    TOKEN_DOLLAR, TOKEN_COMMA, // $, ,
    TOKEN_DOT, TOKEN_COMMENT, // ., //

    // double char tokens
    TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS, // ++, --
    TOKEN_STAR_STAR, // **

    TOKEN_GTGT, TOKEN_LTLT, // >>, <<

    TOKEN_EQEQ, TOKEN_PLUS_EQ, // ==, +=
    TOKEN_MINUS_EQ, TOKEN_SLASH_EQ, // -=, /=
    TOKEN_STAR_EQ, TOKEN_PERCENT_EQ, // *=, %=
    TOKEN_HAT_EQ, TOKEN_GT_EQ, // ^=, >=
    TOKEN_LT_EQ, TOKEN_QUESTION_QUESTION, // <=, ??
    
    TOKEN_BANG_EQ, TOKEN_COLON_EQ, // !=, :=
    TOKEN_COLON_COLON, TOKEN_AMP_AMP, // ::, &&
    TOKEN_PIPE_PIPE, TOKEN_ARROW, // ||, ->
    TOKEN_AMP_EQ, TOKEN_PIPE_EQ, // &=, |=
    

    // Triple char tokens
    TOKEN_QUESTION_QUESTION_EQ, // ??=

    // Keywords
    TOKEN_IF, TOKEN_ELIF, // if, else if
    TOKEN_ELSE, TOKEN_WHILE, // else, while
    TOKEN_FOR, TOKEN_UNLESS, // for, unless
    TOKEN_BREAK, TOKEN_CONTINUE, // break, continue
    TOKEN_FALLTHROUGH, // fallthrough

    TOKEN_STRUCT, TOKEN_ENUM, // struct, enum
    TOKEN_AS, TOKEN_MUT, // as, mut
    TOKEN_MODULE, TOKEN_USE, // module, use
    TOKEN_FUNC, TOKEN_RETURN, // func, return
    TOKEN_IMPL, TOKEN_TRAIT, // impl, trait
    TOKEN_AND, TOKEN_OR, // and, or
    TOKEN_TRUE, TOKEN_FALSE, // true, false
    TOKEN_NIL, TOKEN_PRINTLN, // nil, println
    TOKEN_SWITCH, TOKEN_CASE, // switch, case
    TOKEN_DEFAULT, TOKEN_DEFER, // default, defer
    TOKEN_UNION, TOKEN_CONST, // union, const

    // Other
    TOKEN_IDENTIFIER, TOKEN_STRING,
    TOKEN_INTEGER, TOKEN_FLOAT,

    TOKEN_UNKNOWN,
    TOKEN_ERROR,

    TOKEN_COUNT, TOKEN_EOF,
} TokenType;

typedef struct Token {
	TokenType type;
	char *text;
	isize len;
	isize line, col;
} Token;

typedef struct Lexer {
	char *start, *curr, *end;
	char *line_start;
	isize line;
} Lexer;

gb_global const char *stringTok[TOKEN_COUNT] = {
	"TOKEN_PLUS",
	"TOKEN_MINUS",
    "TOKEN_SLASH",
    "TOKEN_STAR",
    "TOKEN_GT",
    "TOKEN_LT",
    "TOKEN_EQ",
    "TOKEN_COLON",
    "TOKEN_HAT",
    "TOKEN_AMP",
    "TOKEN_PERCENT",
    "TOKEN_BANG",
    "TOKEN_TAG",
    "TOKEN_AT",
    "TOKEN_TILDE",
    "TOKEN_SEMI_COLON",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
    "TOKEN_LBRACKET",
    "TOKEN_RBRACKET",
    "TOKEN_LBRACE",
    "TOKEN_RBRACE",
    "TOKEN_PIPE",
    "TOKEN_QUESTION",
    "TOKEN_DOLLAR",
    "TOKEN_COMMA",
    "TOKEN_DOT",
    "TOKEN_COMMENT",
    "TOKEN_PLUS_PLUS",
    "TOKEN_MINUS_MINUS",
    "TOKEN_STAR_STAR",
    "TOKEN_GTGT",
    "TOKEN_LTLT",
    "TOKEN_EQEQ",
    "TOKEN_PLUS_EQ",
    "TOKEN_MINUS_EQ",
    "TOKEN_SLASH_EQ", 
    "TOKEN_STAR_EQ",
    "TOKEN_PERCENT_EQ", 
    "TOKEN_HAT_EQ",
    "TOKEN_GT_EQ", 
    "TOKEN_LT_EQ",
    "TOKEN_QUESTION_QUESTION",
    "TOKEN_BANG_EQ",
    "TOKEN_COLON_EQ",
    "TOKEN_COLON_COLON",
    "TOKEN_AMP_AMP",
    "TOKEN_PIPE_PIPE",
    "TOKEN_ARROW",
    "TOKEN_AMP_EQ",
    "TOKEN_PIPE_EQ",
    "TOKEN_QUESTION_QUESTION_EQ",
    "TOKEN_IF",
    "TOKEN_ELIF", 
    "TOKEN_ELSE",
    "TOKEN_WHILE",
    "TOKEN_FOR",
    "TOKEN_UNLESS",
    "TOKEN_BREAK",
    "TOKEN_CONTINUE",
    "TOKEN_FALLTHROUGH",
    "TOKEN_STRUCT",
    "TOKEN_ENUM",
    "TOKEN_AS",
    "TOKEN_MUT",
    "TOKEN_MODULE",
    "TOKEN_USE",
    "TOKEN_FUNC",
    "TOKEN_RETURN",
    "TOKEN_IMPL",
    "TOKEN_TRAIT",
    "TOKEN_AND",
    "TOKEN_OR",
    "TOKEN_TRUE",
    "TOKEN_FALSE",
    "TOKEN_NIL",
    "TOKEN_PRINTLN",
    "TOKEN_SWITCH",
    "TOKEN_CASE",
    "TOKEN_DEFAULT",
    "TOKEN_DEFER",
    "TOKEN_UNION",
    "TOKEN_CONST",
    "TOKEN_IDENTIFIER",
    "TOKEN_STRING",
    "TOKEN_INTEGER",
    "TOKEN_FLOAT",
    "TOKEN_UNKNOWN",
    "TOKEN_ERROR",
};

Lexer make_lexer(gbFileContents fc);
Token get_token(Lexer *lex);

gb_inline b32 token_eq(Token tok, char *txt);

#endif // sadie_lexer_h