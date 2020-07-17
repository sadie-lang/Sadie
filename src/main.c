#define GB_IMPLEMENTATION
#include "includes/gb/gb.h"
#include "includes/lexer.h"

int main(int argc, char **argv) {
	gbAllocator a = gb_heap_allocator();

	if (argc < 2) {
		gb_printf_err("Please specify a .sadie file!\n");
		return 1;
	}

	char *filename = argv[1];

	gb_printf("Running %s\n", filename);
	gbFileContents fc = {0};
	char const *ext = NULL;

	ext = gb_path_extension(filename);

	if (gb_strcmp(ext, "sadie") != 0) {
		gb_printf_err("The specified file is not a .sadie file!\n");

		return 1;
	}

	fc = gb_file_read_contents(a, true, filename);

	if (fc.data) {
		isize idx;
		Lexer lexer = make_lexer(fc);
		gbArray(Token) toks;
		gb_array_init(toks, a);
		for (;;) {
			Token tok = get_token(&lexer);
			gb_array_append(toks, tok);
			if (tok.type == TOKEN_EOF) {
				break;
			}
		}

		for (idx = 0; idx < gb_array_count(toks); idx++) {
			Token tok = toks[idx];
			gb_printf("%s ", stringTok[tok.type]);
			gb_printf("(%td:%td) \t%.*s\n", tok.line, tok.col, TOKEN_LIT(tok));
		}
	}

	return 0;
}