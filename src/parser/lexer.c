#include "../includes/gb/gb.h"
#include "../includes/lexer.h"

Lexer make_lexer(gbFileContents fc) {
	Lexer lex = {0};
	lex.start = cast(char *)fc.data;
	lex.curr = lex.start;
	lex.end = lex.start + fc.size;

	lex.line_start = lex.start;
	return lex;
}

gb_inline b32 increment_line(Lexer *lex) {
	if (lex->curr[0] == '\n') {
		lex->line++;
		lex->line_start = ++lex->curr;
		return true;
	}
	return false;
}

b32 skip_space(Lexer *lex) {
	while (1) {
		if (gb_char_is_space(lex->curr[0]) && lex->curr[0] != '\n') {
			lex->curr++;
		}
		else if (lex->curr[0] == '\n') {
			increment_line(lex);
		}
		else if (lex->curr[0] == '\0' ||
			lex->curr >= lex->end) {
			return false;
		} else {
			return true;
		}
	}
}

void trim_space_from_end(Token *tok) {
	char *end = tok->text + tok->len-1;

	while (gb_char_is_space(*end)) {
		end--;
	}
	tok->len = end+1 - tok->text;
}

gb_inline b32 token_eq(Token tok, char *txt) {
	return gb_strncmp(tok.text, txt, tok.len) == 0;
}

Token get_token(Lexer *lex) {
	Token tok = {TOKEN_ERROR};
	char c;

	if (!skip_space(lex)) {
		tok.type = TOKEN_EOF;
		return tok;
	}

	tok.text = lex->curr;
	tok.len = 1;
	tok.line = lex->line;
	tok.col = tok.text - lex->line_start + 1;
	c = *lex->curr++;

	switch (c) {
		case '\0':
			tok.type = TOKEN_EOF;
			lex->curr--;
			break;

	    case '#': tok.type = TOKEN_TAG;			break;
	    case '@': tok.type = TOKEN_AT;			break;
	    case '~': tok.type = TOKEN_TILDE;		break;
	    case ';': tok.type = TOKEN_SEMI_COLON;	break;
	    case '(': tok.type = TOKEN_LPAREN;		break;
	    case ')': tok.type = TOKEN_RPAREN;		break;
	    case '[': tok.type = TOKEN_LBRACKET;	break;
	    case ']': tok.type = TOKEN_RBRACKET;	break;
	    case '{': tok.type = TOKEN_LBRACE;		break;
	    case '}': tok.type = TOKEN_RBRACE;		break;
	    case '$': tok.type = TOKEN_DOLLAR;		break;
	    case ',': tok.type = TOKEN_COMMA;		break;
	    case '.': tok.type = TOKEN_DOT;			break;

	    case '=':
	    	if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_EQEQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	}
	    	else {
	    		tok.type = TOKEN_EQ;
	    	}

	    	break;

	    case ':':
	    	if (lex->curr[0] == ':') {
	    		tok.type = TOKEN_COLON_COLON;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_COLON_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	}
	    	else {
	    		tok.type = TOKEN_COLON;
	    	}

	    	break;

	    case '+': {
	    	if (lex->curr[0] == '+') {
	    		tok.type = TOKEN_PLUS_PLUS;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_PLUS_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_PLUS;
	    	}

	    } break;

		case '-': {
			if (lex->curr[0] == '-') {
	    		tok.type = TOKEN_MINUS_MINUS;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_MINUS_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_MINUS;
	    	}
		} break;

	    case '*': {
	    	if (lex->curr[0] == '*') {
	    		tok.type = TOKEN_STAR_STAR;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_STAR_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_STAR;
	    	}
	    } break;

	    case '>': {
	    	if (lex->curr[0] == '>') {
	    		tok.type = TOKEN_GTGT;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_GT_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_GT;
	    	}
	    } break;

	    case '<': {
	    	if (lex->curr[0] == '<') {
	    		tok.type = TOKEN_LTLT;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_LT_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_LT;
	    	}
	    } break;

	    case '^': {
	    	if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_HAT_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_HAT;
	    	}
	    } break;

	    case '&': {
	    	if (lex->curr[0] == '&') {
	    		tok.type = TOKEN_AMP_AMP;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_AMP_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_AMP;
	    	}
	    } break;

	   	case '|': {
	   		if (lex->curr[0] == '|') {
	   			tok.type = TOKEN_PIPE_PIPE;
	   			tok.len - 2;
	   			lex->curr += 2;
	   		} else if (lex->curr[0] == '=') {
	   			tok.type = TOKEN_PIPE_EQ;
	   			tok.len = 2;
	   			lex->curr += 2;
	   		} else {
	   			tok.type = TOKEN_PIPE;
	   		}
	   	} break;

	    case '%': {
	    	if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_PERCENT_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_PERCENT;
	    	}
	    } break;

	    case '!': {
	    	if (lex->curr[0] == '=') {
	    		tok.type = TOKEN_BANG_EQ;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else {
	    		tok.type = TOKEN_BANG;
	    	}
	    } break;
	    
	    case '?': {
	    	if (lex->curr[0] == '?' && lex->curr[1] != '=') {
	    		tok.type = TOKEN_QUESTION_QUESTION;
	    		tok.len = 2;
	    		lex->curr += 2;
	    	} else if (lex->curr[0] == '?' && lex->curr[1] == '=') {
	    		tok.type = TOKEN_QUESTION_QUESTION_EQ;
	    		tok.len = 3;
	    		lex->curr += 3;
	    	} else {
	    		tok.type = TOKEN_QUESTION;
	    	}

	    } break;

	    case '"': {
	    	tok.type = TOKEN_STRING;
	    	tok.text = lex->curr;

	    	while (lex->curr[0] && lex->curr[0] != '"') {
	    		if (lex->curr[0] == '\\' && lex->curr[1]) {
	    			lex->curr++;
	    		}
	    		lex->curr++;
	    	}

	    	tok.len = lex->curr - tok.text;
	    	if (lex->curr[0] == '"') {
	    		lex->curr++;
	    	}
	    } break;

	    case '/': {
	    	if (lex->curr[0] == '/') {
	    		lex->curr++;
	    		tok.type = TOKEN_COMMENT;
	    		skip_space(lex);

	    		tok.text = lex->curr;
	    		while (lex->curr[0] &&
	    			lex->curr[0] != '\n'
	    			&& (lex->curr < lex->end)) {
	    			lex->curr++;
	    		}

	    		tok.len = lex->curr - tok.text;
	    		trim_space_from_end(&tok);
	    	} else if (lex->curr[0] == '*') {
	    		lex->curr++;
	    		skip_space(lex);
	    		tok.type = TOKEN_COMMENT;

	    		isize nesting = 1;

	    		while (lex->curr[0] && nesting > 0 && lex->curr < lex->end) {
	    			if (lex->curr[0] == '/' && lex->curr[1] == '*') {
	    				nesting++;
	    			}

	    			else if (lex->curr[0] == '*' && lex->curr[1] == '/') {
	    				nesting--;
	    			}

	    			if (!increment_line(lex)) {
	    				lex->curr++;
	    			}
	    		}
	    		tok.len = lex->curr-1 - tok.text;
	    		trim_space_from_end(&tok);
	    	} else {
	    		tok.type = TOKEN_SLASH;
	    	}

	    	if (lex->curr[-1] == '*' && lex->curr[0] == '/') {
	    		lex->curr += 2;
	    	}
	    } break;

	    default: {
	    	if (gb_char_is_alpha(c) || c == '_') {
	    		tok.type = TOKEN_IDENTIFIER;

	    		while (gb_char_is_alphanumeric(lex->curr[0]) || lex->curr[0] == '_') {
	    			lex->curr++;
	    		}

	    		tok.len = lex->curr - tok.text;

	    		if (gb_strncmp("if", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_IF;
	    		} else if (gb_strncmp("else if", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_ELIF;
	    		} else if (gb_strncmp("else", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_ELSE;
	    		} else if (gb_strncmp("while", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_WHILE;
	    		} else if (gb_strncmp("for", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_FOR;
	    		} else if (gb_strncmp("unless", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_UNLESS;
	    		} else if (gb_strncmp("break", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_BREAK;
	    		} else if (gb_strncmp("continue", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_CONTINUE;
	    		} else if (gb_strncmp("fallthrough", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_FALLTHROUGH;
	    		} else if (gb_strncmp("struct", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_STRUCT;
	    		} else if (gb_strncmp("enum", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_ENUM;
	    		} else if (gb_strncmp("as", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_AS;
	    		} else if (gb_strncmp("mut", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_MUT;
	    		} else if (gb_strncmp("module", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_MODULE;
	    		} else if (gb_strncmp("use", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_USE;
	    		} else if (gb_strncmp("func", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_FUNC;
	    		} else if (gb_strncmp("return", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_RETURN;
	    		} else if (gb_strncmp("impl", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_IMPL;
	    		} else if (gb_strncmp("trait", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_TRAIT;
	    		} else if (gb_strncmp("and", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_AND;
	    		} else if (gb_strncmp("or", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_OR;
	    		} else if (gb_strncmp("true", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_TRUE;
	    		} else if (gb_strncmp("false", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_FALSE;
	    		} else if (gb_strncmp("nil", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_NIL;
	    		} else if (gb_strncmp("println", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_PRINTLN;
	    		} else if (gb_strncmp("switch", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_SWITCH;
	    		} else if (gb_strncmp("case", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_CASE;
	    		} else if (gb_strncmp("default", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_DEFAULT;
	    		} else if (gb_strncmp("defer", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_DEFER;
	    		} else if (gb_strncmp("union", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_UNION;
	    		} else if (gb_strncmp("const", tok.text, tok.len) == 0) {
	    			tok.type = TOKEN_CONST;
	    		}
	    	} else {
	    		if (gb_char_is_digit(c) || (c == '.' && gb_char_is_digit(lex->curr[0]))) {
	    			b32 hex = false;
	    			b32 binary = false;
	    			b32 is_float = false;
	    			b32 octal = false;

	    			tok.type = TOKEN_INTEGER;

	    			while (gb_char_is_digit(lex->curr[0])) {
	    				lex->curr++;
	    			}

	    			switch (lex->curr[0]) {
	    				case 'x':
	    					hex = true;
	    					lex->curr++;
	    					break;

	    				case 'b':
	    					binary = true;
	    					lex->curr++;
	    					break;

	    				case 'o':
	    					octal = true;
	    					lex->curr++;
	    					break;

	    				case '.':
	    					is_float = true;
	    					lex->curr++;
	    					break;
	    			}

	    			while (gb_char_is_digit(lex->curr[0])) {
	    				lex->curr++;
	    			}

	    			tok.len = lex->curr - tok.text;

	    			if (is_float) {
	    				tok.type = TOKEN_FLOAT;
	    			}
	    		}
	    	}
	    } break;
	}

	return tok;
}