#include "koml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static unsigned long long int koml_internal_hash(char * start, unsigned long long int length) {
	unsigned long long int hash = 5381;

	for (unsigned long long int i = 0; i < length; ++i) {
		hash = (hash << 5) + hash + start[i];
	}

	return hash;
}

static int koml_table_alloc_new(koml_table_t * table) {
	++table->length;
	if (table->hashes == NULL) {
		table->hashes = malloc(table->length * sizeof(unsigned long long int));
	} else {
		table->hashes = realloc(table->hashes, table->length * sizeof(unsigned long long int));
	}

	if (table->hashes == NULL) {
		return 1;
	}

	if (table->symbols == NULL) {
		table->symbols = malloc(table->length * sizeof(koml_symbol_t));
	} else {
		table->symbols = realloc(table->symbols, table->length * sizeof(koml_symbol_t));
	}

	if (table->symbols == NULL) {
		return 2;
	}

	memset(&table->symbols[table->length - 1].data, 0, sizeof(table->symbols[table->length - 1].data));

	return 0;
}

static int koml_array_alloc_new(koml_array_t * array) {
	++array->length;
	if (array->strides == NULL) {
		array->strides = malloc(array->length * sizeof(unsigned long long int));
	} else {
		array->strides = realloc(array->strides, array->length * sizeof(unsigned long long int));
	}

	if (array->strides == NULL) {
		return 1;
	}

	unsigned long long int stride = 0;
	switch (array->type) {
		case KOML_TYPE_INT:
		case KOML_TYPE_FLOAT:
			stride = 4;
			break;
		case KOML_TYPE_STRING:
			stride = sizeof(char *);
			break;
		case KOML_TYPE_BOOLEAN:
			stride = 1;
			break;
		default:
			return 3;
	}

	if (array->elements.voidptr == NULL) {
		array->elements.voidptr = malloc(array->length * stride);
	} else {
		array->elements.voidptr = realloc(array->elements.voidptr, array->length * stride);
	}

	if (array->elements.voidptr == NULL) {
		return 2;
	}

	return 0;
}

static int koml_array_alloc_new_amount(koml_array_t * array, unsigned long long int amount) {
	array->length = amount;
	if (array->strides == NULL) {
		array->strides = malloc(array->length * sizeof(unsigned long long int));
	} else {
		array->strides = realloc(array->strides, array->length * sizeof(unsigned long long int));
	}

	if (array->strides == NULL) {
		return 1;
	}

	unsigned long long int stride = 0;
	switch (array->type) {
		case KOML_TYPE_INT:
		case KOML_TYPE_FLOAT:
			stride = 4;
			break;
		case KOML_TYPE_STRING:
			stride = sizeof(char *);
			break;
		case KOML_TYPE_BOOLEAN:
			stride = 1;
			break;
		default:
			return 3;
	}

	if (array->elements.voidptr == NULL) {
		array->elements.voidptr = malloc(array->length * stride);
	} else {
		array->elements.voidptr = realloc(array->elements.voidptr, array->length * stride);
	}

	if (array->elements.voidptr == NULL) {
		return 2;
	}

	return 0;
}

static char is_whitespace(char c) {
	if (c == '\t' || c == '\r' || c == '\n' || c == ' ') {
		return 1;
	}

	return 0;
}

static int wtoi(char * start, unsigned long long int length, unsigned int radix) {
	int ret = 0;

	for (unsigned long long int i = 0; i < length; ++i) {
		ret = ret * radix + (start[i] - '0');
	}

	return ret;
}

static float wtof(char * start, unsigned long long int length) {
	(void) length;
	return (float) strtod(start, NULL);
}

static unsigned char is_boolean(char * start, unsigned long long int length) {
	if (length == 1) {
		if (*start == '1' || *start == '0' || *start == 't' || *start == 'f') {
			return 1;
		}
	}

	if (length == 4) {
		if (strncmp("true", start, length) == 0) {
			return 1;
		}
	}

	if (length == 5) {
		if (strncmp("false", start, length) == 0) {
			return 1;
		}
	}

	return 0;
}

static unsigned char is_num(char c) {
	return ((isalnum(c) && !isalpha(c)) || c == '-');
}

static unsigned char wtotf(char * start, unsigned long long int length) {
	if (length == 1) {
		if (*start == '1' || *start == 't') {
			return 1;
		}
		if (*start == '0' || *start == 'f') {
			return 0;
		}
	}

	if (length == 4) {
		if (strncmp("true", start, length) == 0) {
			return 1;
		}
	}

	if (length == 5) {
		if (strncmp("false", start, length) == 0) {
			return 0;
		}
	}

	return 0;
}

static void koml_printline(char * buffer, unsigned long long int line, unsigned long long int column) {
	while (*buffer != '\0') {
		if (*buffer == '\n') {
			--line;
		}
		++buffer;
		if (line == 0) {
			break;
		}
	}
	while (*buffer != '\n' && *buffer != '\0') {
		putc(*(buffer++), stdout);
	}
}

static void koml_printcursor(unsigned long long int column) {
	while (--column) {
		putc(' ', stdout);
	}
	putc('^', stdout);
}

static char * koml_type_strings[] = {
	"unknown",
	"int",
	"float",
	"string",
	"boolean",
	"array",
};

void koml_array_print(koml_array_t * array) {
	printf("(%s) [ ", koml_type_strings[array->type]);
	for (unsigned long long int i = 0; i < array->length; ++i) {
		switch (array->type) {
			case KOML_TYPE_INT:
				printf("%i", array->elements.i32[i]);
				break;
			case KOML_TYPE_FLOAT:
				printf("%f", array->elements.f32[i]);
				break;
			case KOML_TYPE_STRING:
				printf("\"%s\"", array->elements.string[i]);
				break;
			case KOML_TYPE_BOOLEAN:
				printf("%s", (array->elements.boolean[i]) ? "true" : "false");
				break;
			case KOML_TYPE_UNKNOWN:
			default:
				printf("???");
				break;
		}

		if (i < array->length - 1) {
			printf(", ");
		}
	}
	printf(" ]");
}

void koml_symbol_print(koml_symbol_t * symbol) {
	printf("%s (%s): ", symbol->name, koml_type_strings[symbol->type]);
	switch (symbol->type) {
		case KOML_TYPE_INT:
			printf("%i", symbol->data.i32);
			break;
		case KOML_TYPE_FLOAT:
			printf("%f", symbol->data.f32);
			break;
		case KOML_TYPE_STRING:
			printf("\"%s\"", symbol->data.string);
			break;
		case KOML_TYPE_BOOLEAN:
			printf("%s", (symbol->data.boolean) ? "true" : "false");
			break;
		case KOML_TYPE_ARRAY:
			koml_array_print(&symbol->data.array);
			break;
		case KOML_TYPE_UNKNOWN:
		default:
			printf("???");
			break;
	}
}

void koml_table_print(koml_table_t * table) {
	puts("{");
	for (unsigned long long int i = 0; i < table->length; ++i) {
		putc(' ', stdout);
		putc(' ', stdout);
		koml_symbol_print(&table->symbols[i]);
		putc('\n', stdout);
	}
	puts("}");
}

typedef enum KOMLParserStateEnum {
	KOML_PARSER_STATE_NONE = 0,
	KOML_PARSER_STATE_SECTION_WAIT,
	KOML_PARSER_STATE_SECTION_NAME,
	KOML_PARSER_STATE_INTEGER_WAIT,
	KOML_PARSER_STATE_INTEGER_NAME,
	KOML_PARSER_STATE_INTEGER_VALUE,
	KOML_PARSER_STATE_FLOAT_WAIT,
	KOML_PARSER_STATE_FLOAT_NAME,
	KOML_PARSER_STATE_FLOAT_VALUE,
	KOML_PARSER_STATE_STRING_WAIT,
	KOML_PARSER_STATE_STRING_NAME,
	KOML_PARSER_STATE_STRING_VALUE,
	KOML_PARSER_STATE_BOOLEAN_WAIT,
	KOML_PARSER_STATE_BOOLEAN_NAME,
	KOML_PARSER_STATE_BOOLEAN_VALUE,
	KOML_PARSER_STATE_ARRAY_TYPE_WAIT,
	KOML_PARSER_STATE_ARRAY_WAIT,
	KOML_PARSER_STATE_ARRAY_NAME,
	KOML_PARSER_STATE_ARRAY_VALUE,
} koml_parser_state_enum;

int koml_table_load(koml_table_t * out_table, char * buffer, unsigned long long int buffer_length) {
	if (buffer == NULL || buffer_length == 0) {
		return 1;
	}

	out_table->length = 0;
	out_table->hashes = NULL;
	out_table->symbols = NULL;

	struct {
		char * start;
		unsigned long long int length;
		unsigned long long int hash;
	} word = {
		.start = NULL,
		.length = 0,
		.hash = 0,
	};

	struct {
		char * start;
		unsigned long long int length;
	} section = {
		.start = NULL,
		.length = 0,
	};

	char c = 127;
	char prevc = 127;

	koml_parser_state_enum state = KOML_PARSER_STATE_NONE;
	unsigned long long int line = 0;
	unsigned long long int column = 0;

	for (unsigned long long int i = 0; i < buffer_length; ++i) {
		prevc = c;
		c = buffer[i];
		++column;

		if (c == '|') {
			++i;
			c = buffer[i];
			while (c != '|') {
				++i;
				if (i >= buffer_length) {
					printf("Comment never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column);
					printf("\n");
					return 6;
				}
				c = buffer[i];
			}

			++i;
			c = buffer[i];
		}

		if (c == '\n') {
			++line;
			column = 0;
		}

		switch (state) {
			case KOML_PARSER_STATE_SECTION_WAIT:
				word.start = &buffer[i];
				word.length = 0;
				state = KOML_PARSER_STATE_SECTION_NAME;
				continue;
			case KOML_PARSER_STATE_SECTION_NAME:
				++word.length;
				if (c == ']') {
					state = KOML_PARSER_STATE_NONE;
					section.start = word.start;
					section.length = word.length;
				} else if ((isalnum(c) || ispunct(c)) && word.start != NULL && is_whitespace(prevc)) {
					printf("Invalid section name (line %llu: column %llu)\n  | ", line + 1, column);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column - 1);
					printf("\n");
					return 14;
				}
				continue;
			case KOML_PARSER_STATE_INTEGER_WAIT:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				state = KOML_PARSER_STATE_INTEGER_NAME;
				continue;
			case KOML_PARSER_STATE_INTEGER_NAME:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					out_table->symbols[out_table->length - 1].name = NULL;
					unsigned long long int tmp_length = 0;
					if (section.start != NULL) {
						tmp_length = word.length + section.length + 1;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL || section.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}

						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';

						memcpy(out_table->symbols[out_table->length - 1].name, section.start, section.length);
						out_table->symbols[out_table->length - 1].name[section.length] = ':';
						memcpy(&out_table->symbols[out_table->length - 1].name[section.length + 1], word.start, word.length);
					} else {
						tmp_length = word.length;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}
						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';
						memcpy(out_table->symbols[out_table->length - 1].name, word.start, tmp_length);
					}
					if (out_table->symbols[out_table->length - 1].name == NULL) {
						printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 1;
					}

					out_table->hashes[out_table->length - 1] = koml_internal_hash(out_table->symbols[out_table->length - 1].name, tmp_length);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_INTEGER_VALUE;
				} else if ((isalnum(c) || ispunct(c)) && word.start != NULL && is_whitespace(prevc)) {
					printf("Invalid variable name (line %llu: column %llu)\n  | ", line + 1, column);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column - 1);
					printf("\n");
					return 13;
				}

				continue;
			case KOML_PARSER_STATE_INTEGER_VALUE:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == '@') {
					koml_symbol_t * ptr;
					++i;
					c = buffer[i];
					word.start = &buffer[i];
					word.length = 0;
					while (c != ';') {
						++i;
						++word.length;
						if (i >= buffer_length) {
							printf("Variable reference never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 16;
						}
						c = buffer[i];
					}
					ptr = koml_table_symbol_word(out_table, word.start, word.length);
					if (ptr == NULL) {
						printf("Variable reference to undefined symbol (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 17;
					}

					if (ptr->type != KOML_TYPE_INT && ptr->type != KOML_TYPE_FLOAT) {
						printf("Invalid type of variable reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 18;
					}

					if (ptr->type == KOML_TYPE_FLOAT) {
						out_table->symbols[out_table->length - 1].data.i32 = ptr->data.f32;
					} else {
						out_table->symbols[out_table->length - 1].data.i32 = ptr->data.i32;
					}

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (c == ';') {
					int value = wtoi(word.start, word.length, 10);
					out_table->symbols[out_table->length - 1].data.i32 = value;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (!is_num(c)) {
					if (c == '"') {
						printf("A string literal is not a valid integer value (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 8;
					}
					printf("Invalid integer value (line %llu: column %llu)\n  | ", line + 1, column + 1);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column);
					printf("\n");
					return 8;
				}

				if (word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
				}
				++word.length;
				continue;
			case KOML_PARSER_STATE_FLOAT_WAIT:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				state = KOML_PARSER_STATE_FLOAT_NAME;
				continue;
			case KOML_PARSER_STATE_FLOAT_NAME:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					out_table->symbols[out_table->length - 1].name = NULL;
					unsigned long long int tmp_length = 0;
					if (section.start != NULL) {
						tmp_length = word.length + section.length + 1;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL || section.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}

						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';

						memcpy(out_table->symbols[out_table->length - 1].name, section.start, section.length);
						out_table->symbols[out_table->length - 1].name[section.length] = ':';
						memcpy(&out_table->symbols[out_table->length - 1].name[section.length + 1], word.start, word.length);
					} else {
						tmp_length = word.length;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}
						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';
						memcpy(out_table->symbols[out_table->length - 1].name, word.start, tmp_length);
					}
					if (out_table->symbols[out_table->length - 1].name == NULL) {
						printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 1;
					}

					out_table->hashes[out_table->length - 1] = koml_internal_hash(out_table->symbols[out_table->length - 1].name, tmp_length);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_FLOAT_VALUE;
				} else if ((isalnum(c) || ispunct(c)) && word.start != NULL && is_whitespace(prevc)) {
					printf("Invalid section name (line %llu: column %llu)\n  | ", line + 1, column);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column - 1);
					printf("\n");
					return 14;
				}

				continue;
			case KOML_PARSER_STATE_FLOAT_VALUE:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == '@') {
					koml_symbol_t * ptr;
					++i;
					c = buffer[i];
					word.start = &buffer[i];
					word.length = 0;
					while (c != ';') {
						++i;
						++word.length;
						if (i >= buffer_length) {
							printf("Variable reference never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 16;
						}
						c = buffer[i];
					}
					ptr = koml_table_symbol_word(out_table, word.start, word.length);
					if (ptr == NULL) {
						printf("Variable reference to undefined symbol (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 17;
					}

					if (ptr->type != KOML_TYPE_INT && ptr->type != KOML_TYPE_FLOAT) {
						printf("Invalid type of variable reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 18;
					}

					if (ptr->type == KOML_TYPE_FLOAT) {
						out_table->symbols[out_table->length - 1].data.f32 = ptr->data.f32;
					} else {
						out_table->symbols[out_table->length - 1].data.f32 = ptr->data.i32;
					}

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (c == ';') {
					float value = wtof(word.start, word.length);
					out_table->symbols[out_table->length - 1].data.f32 = value;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (!is_num(c) && c != '.') {
					if (c == '"') {
						printf("A string literal is not a valid float value (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 9;
					}

					printf("Invalid float value (line %llu: column %llu)\n  | ", line + 1, column + 1);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column);
					printf("\n");
					return 9;
				}

				if (word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
				}
				++word.length;
				continue;
			case KOML_PARSER_STATE_STRING_WAIT:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				state = KOML_PARSER_STATE_STRING_NAME;
				continue;
			case KOML_PARSER_STATE_STRING_NAME:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					out_table->symbols[out_table->length - 1].name = NULL;
					unsigned long long int tmp_length = 0;
					if (section.start != NULL) {
						tmp_length = word.length + section.length + 1;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL || section.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}

						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';

						memcpy(out_table->symbols[out_table->length - 1].name, section.start, section.length);
						out_table->symbols[out_table->length - 1].name[section.length] = ':';
						memcpy(&out_table->symbols[out_table->length - 1].name[section.length + 1], word.start, word.length);
					} else {
						tmp_length = word.length;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}
						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';
						memcpy(out_table->symbols[out_table->length - 1].name, word.start, tmp_length);
					}
					if (out_table->symbols[out_table->length - 1].name == NULL) {
						printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 1;
					}

					out_table->hashes[out_table->length - 1] = koml_internal_hash(out_table->symbols[out_table->length - 1].name, tmp_length);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_STRING_VALUE;
				} else if ((isalnum(c) || ispunct(c)) && word.start != NULL && is_whitespace(prevc)) {
					printf("Invalid section name (line %llu: column %llu)\n  | ", line + 1, column);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column - 1);
					printf("\n");
					return 14;
				}

				continue;
			case KOML_PARSER_STATE_STRING_VALUE:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == '"') {
					++i;
					c = buffer[i];
					word.start = &buffer[i];
					word.length = 0;
					while (c != '"') {
						++i;
						++word.length;
						if (i >= buffer_length) {
							printf("String literal never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 2;
						}
						c = buffer[i];
					}

					out_table->symbols[out_table->length - 1].stride = word.length;
					out_table->symbols[out_table->length - 1].data.string = malloc(word.length + 1);
					if (out_table->symbols[out_table->length - 1].data.string == NULL) {
						printf("Failed to allocate string buffer (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 3;
					}
					out_table->symbols[out_table->length - 1].data.string[word.length] = '\0';
					memcpy(out_table->symbols[out_table->length - 1].data.string, word.start, word.length);
				} else if (c == ';') {
					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				} else if (c == '@') {
					koml_symbol_t * ptr;
					++i;
					c = buffer[i];
					word.start = &buffer[i];
					word.length = 0;
					while (c != ';') {
						++i;
						++word.length;
						if (i >= buffer_length) {
							printf("Variable reference never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 16;
						}
						c = buffer[i];
					}

					ptr = koml_table_symbol_word(out_table, word.start, word.length);
					if (ptr == NULL) {
						printf("Variable reference to undefined symbol (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 17;
					}

					if (ptr->type != KOML_TYPE_STRING) {
						printf("Invalid type of variable reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 18;
					}

					out_table->symbols[out_table->length - 1].data.string = malloc(ptr->stride + 1);
					if (out_table->symbols[out_table->length - 1].data.string == NULL) {
						printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 1;
					}

					out_table->symbols[out_table->length - 1].data.string[ptr->stride] = '\0';
					memcpy(out_table->symbols[out_table->length - 1].data.string, ptr->data.string, ptr->stride);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				} else {
					printf("Invalid string literal (line %llu: column %llu)\n  | ", line + 1, column + 1);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column);
					printf("\n");
					return 10;
				}

				if (word.start != NULL) {
					++word.length;
				}
				continue;
			case KOML_PARSER_STATE_BOOLEAN_WAIT:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				state = KOML_PARSER_STATE_BOOLEAN_NAME;
				continue;
			case KOML_PARSER_STATE_BOOLEAN_NAME:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					out_table->symbols[out_table->length - 1].name = NULL;
					unsigned long long int tmp_length = 0;
					if (section.start != NULL) {
						tmp_length = word.length + section.length + 1;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL || section.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}

						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';

						memcpy(out_table->symbols[out_table->length - 1].name, section.start, section.length);
						out_table->symbols[out_table->length - 1].name[section.length] = ':';
						memcpy(&out_table->symbols[out_table->length - 1].name[section.length + 1], word.start, word.length);
					} else {
						tmp_length = word.length;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}
						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';
						memcpy(out_table->symbols[out_table->length - 1].name, word.start, tmp_length);
					}
					if (out_table->symbols[out_table->length - 1].name == NULL) {
						printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 1;
					}

					out_table->hashes[out_table->length - 1] = koml_internal_hash(out_table->symbols[out_table->length - 1].name, tmp_length);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_BOOLEAN_VALUE;
				} else if ((isalnum(c) || ispunct(c)) && word.start != NULL && is_whitespace(prevc)) {
					printf("Invalid section name (line %llu: column %llu)\n  | ", line + 1, column);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column - 1);
					printf("\n");
					return 14;
				}

				continue;
			case KOML_PARSER_STATE_BOOLEAN_VALUE:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == '@') {
					koml_symbol_t * ptr;
					++i;
					c = buffer[i];
					word.start = &buffer[i];
					word.length = 0;
					while (c != ';') {
						++i;
						++word.length;
						if (i >= buffer_length) {
							printf("Variable reference never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 16;
						}
						c = buffer[i];
					}
					ptr = koml_table_symbol_word(out_table, word.start, word.length);
					if (ptr == NULL) {
						printf("Variable reference to undefined symbol (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 17;
					}

					if (ptr->type != KOML_TYPE_BOOLEAN) {
						printf("Invalid type of variable reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 18;
					}

					out_table->symbols[out_table->length - 1].data.boolean = ptr->data.boolean;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (c == ';') {
					if (!is_boolean(word.start, word.length)) {
						printf("Invalid boolean value (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 5;
					}
					unsigned char value = wtotf(word.start, word.length);
					out_table->symbols[out_table->length - 1].data.boolean = value;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
				}
				++word.length;
				continue;
			case KOML_PARSER_STATE_ARRAY_TYPE_WAIT:
				if (is_whitespace(c)) {
					continue;
				}
				if (c == 'i') {
					out_table->symbols[out_table->length - 1].data.array.length = 0;
					out_table->symbols[out_table->length - 1].data.array.type = KOML_TYPE_INT;
					state = KOML_PARSER_STATE_ARRAY_WAIT;
				}
				if (c == 'f') {
					out_table->symbols[out_table->length - 1].data.array.length = 0;
					out_table->symbols[out_table->length - 1].data.array.type = KOML_TYPE_FLOAT;
					state = KOML_PARSER_STATE_ARRAY_WAIT;
				}
				if (c == 's') {
					out_table->symbols[out_table->length - 1].data.array.length = 0;
					out_table->symbols[out_table->length - 1].data.array.type = KOML_TYPE_STRING;
					state = KOML_PARSER_STATE_ARRAY_WAIT;
				}
				if (c == 'b') {
					out_table->symbols[out_table->length - 1].data.array.length = 0;
					out_table->symbols[out_table->length - 1].data.array.type = KOML_TYPE_BOOLEAN;
					state = KOML_PARSER_STATE_ARRAY_WAIT;
				}

				if (state == KOML_PARSER_STATE_ARRAY_TYPE_WAIT && !is_whitespace(c)) {
					printf("Invalid array type (line %llu: column %llu)\n  | ", line + 1, column + 1);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column);
					printf("\n");
					return 12;
				}
				continue;
			case KOML_PARSER_STATE_ARRAY_WAIT:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				state = KOML_PARSER_STATE_ARRAY_NAME;
				continue;
			case KOML_PARSER_STATE_ARRAY_NAME:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					out_table->symbols[out_table->length - 1].name = NULL;
					unsigned long long int tmp_length = 0;
					if (section.start != NULL) {
						tmp_length = word.length + section.length + 1;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL || section.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}

						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';

						memcpy(out_table->symbols[out_table->length - 1].name, section.start, section.length);
						out_table->symbols[out_table->length - 1].name[section.length] = ':';
						memcpy(&out_table->symbols[out_table->length - 1].name[section.length + 1], word.start, word.length);
					} else {
						tmp_length = word.length;
						out_table->symbols[out_table->length - 1].name = malloc(tmp_length + 1);
						if (out_table->symbols[out_table->length - 1].name == NULL || word.start == NULL) {
							printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 1;
						}
						out_table->symbols[out_table->length - 1].name[tmp_length] = '\0';
						memcpy(out_table->symbols[out_table->length - 1].name, word.start, tmp_length);
					}
					if (out_table->symbols[out_table->length - 1].name == NULL) {
						printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 1;
					}

					out_table->hashes[out_table->length - 1] = koml_internal_hash(out_table->symbols[out_table->length - 1].name, tmp_length);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_ARRAY_VALUE;
				} else if ((isalnum(c) || ispunct(c)) && word.start != NULL && is_whitespace(prevc)) {
					printf("Invalid array name (line %llu: column %llu)\n  | ", line + 1, column);
					koml_printline(buffer, line, column);
					printf("\n  | ");
					koml_printcursor(column - 1);
					printf("\n");
					return 15;
				}

				continue;
			case KOML_PARSER_STATE_ARRAY_VALUE:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == '@') {
					koml_symbol_t * ptr;
					++i;
					c = buffer[i];
					word.start = &buffer[i];
					word.length = 0;
					while (c != ';') {
						++i;
						++word.length;
						if (i >= buffer_length) {
							printf("Array reference never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 16;
						}
						c = buffer[i];
					}
					ptr = koml_table_symbol_word(out_table, word.start, word.length);
					if (ptr == NULL) {
						printf("Array reference to undefined symbol (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 17;
					}

					if (ptr->type != KOML_TYPE_ARRAY) {
						printf("Invalid type of array reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 18;
					}

					switch (out_table->symbols[out_table->length - 1].data.array.type) {
						case KOML_TYPE_INT:
							if (ptr->data.array.type != KOML_TYPE_INT && ptr->data.array.type != KOML_TYPE_FLOAT) {
								printf("Invalid type of array reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 18;
							}

							if (ptr->data.array.type == KOML_TYPE_INT) {
								koml_array_alloc_new_amount(&out_table->symbols[out_table->length - 1].data.array, ptr->data.array.length);
								memcpy(out_table->symbols[out_table->length - 1].data.array.elements.i32, ptr->data.array.elements.i32, ptr->data.array.length * 4);
								memcpy(out_table->symbols[out_table->length - 1].data.array.strides, ptr->data.array.strides, ptr->data.array.length * 4);
							} else {
								koml_array_alloc_new_amount(&out_table->symbols[out_table->length - 1].data.array, ptr->data.array.length);
								for (unsigned long long int i = 0; i < ptr->data.array.length; ++i) {
									out_table->symbols[out_table->length - 1].data.array.elements.i32[i] = ptr->data.array.elements.f32[i];
								}
							}
							break;
						case KOML_TYPE_FLOAT:
							if (ptr->data.array.type != KOML_TYPE_INT && ptr->data.array.type != KOML_TYPE_FLOAT) {
								printf("Invalid type of array reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 18;
							}

							if (ptr->data.array.type == KOML_TYPE_FLOAT) {
								koml_array_alloc_new_amount(&out_table->symbols[out_table->length - 1].data.array, ptr->data.array.length);
								memcpy(out_table->symbols[out_table->length - 1].data.array.elements.f32, ptr->data.array.elements.f32, ptr->data.array.length * 4);
								memcpy(out_table->symbols[out_table->length - 1].data.array.strides, ptr->data.array.strides, ptr->data.array.length * 4);
							} else {
								koml_array_alloc_new_amount(&out_table->symbols[out_table->length - 1].data.array, ptr->data.array.length);
								for (unsigned long long int i = 0; i < ptr->data.array.length; ++i) {
									out_table->symbols[out_table->length - 1].data.array.elements.f32[i] = ptr->data.array.elements.i32[i];
								}
							}
							break;
						case KOML_TYPE_STRING:
							if (ptr->data.array.type != KOML_TYPE_STRING) {
								printf("Invalid type of array reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 18;
							}

							koml_array_alloc_new_amount(&out_table->symbols[out_table->length - 1].data.array, ptr->data.array.length);
							memcpy(out_table->symbols[out_table->length - 1].data.array.strides, ptr->data.array.strides, ptr->data.array.length * 4);
							for (unsigned long long int i = 0; i < ptr->data.array.length; ++i) {
								out_table->symbols[out_table->length - 1].data.array.elements.string[i] = malloc(ptr->data.array.strides[i] + 1);
								if (out_table->symbols[out_table->length - 1].data.array.elements.string[i] == NULL) {
									printf("Internal error (line %llu: column %llu)\n  | ", line + 1, column + 1);
									koml_printline(buffer, line, column);
									printf("\n  | ");
									koml_printcursor(column);
									printf("\n");
									return 1;
								}

								memcpy(out_table->symbols[out_table->length - 1].data.array.elements.string[i], ptr->data.array.elements.string[i], ptr->data.array.strides[i]);
								out_table->symbols[out_table->length - 1].data.array.elements.string[i][ptr->data.array.strides[i]] = '\0';
							}
							
							break;
						case KOML_TYPE_BOOLEAN:
							if (ptr->data.array.type != KOML_TYPE_BOOLEAN) {
								printf("Invalid type of array reference (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 18;
							}

							koml_array_alloc_new_amount(&out_table->symbols[out_table->length - 1].data.array, ptr->data.array.length);
							memcpy(out_table->symbols[out_table->length - 1].data.array.elements.boolean, ptr->data.array.elements.boolean, ptr->data.array.length);
							memcpy(out_table->symbols[out_table->length - 1].data.array.strides, ptr->data.array.strides, ptr->data.array.length * 4);
							
							break;
						default:
							printf("Invalid array reference type (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 19;
					}

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					state = KOML_PARSER_STATE_NONE;
					continue;
				}

				if (isalnum(c) && word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
					word.hash = 0;
				}

				if (out_table->symbols[out_table->length - 1].data.array.type != KOML_TYPE_STRING) {
					if (c == ',' || c == ';') {
						koml_array_alloc_new(&out_table->symbols[out_table->length - 1].data.array);
					} else {
						++word.length;
					}
				} else {
					if (c == '"') {
						koml_array_alloc_new(&out_table->symbols[out_table->length - 1].data.array);
					} else {
						++word.length;
					}
				}

				switch (out_table->symbols[out_table->length - 1].data.array.type) {
					case KOML_TYPE_INT:
						if (c == ',') {
							int value = wtoi(word.start, word.length, 10);
							out_table->symbols[out_table->length - 1].data.array.elements.i32[out_table->symbols[out_table->length - 1].data.array.length - 1] = value;

							word.start = NULL;
							word.length = 0;
							word.hash = 0;
						} else if (c == ';') {
							int value = wtoi(word.start, word.length, 10);
							out_table->symbols[out_table->length - 1].data.array.elements.i32[out_table->symbols[out_table->length - 1].data.array.length - 1] = value;

							word.start = NULL;
							word.length = 0;
							word.hash = 0;

							state = KOML_PARSER_STATE_NONE;
						} else if (!is_num(c)) {
							printf("Invalid integer value (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 11;
						}
						continue;
					case KOML_TYPE_FLOAT:
						if (c == ',') {
							float value = wtof(word.start, word.length);
							out_table->symbols[out_table->length - 1].data.array.elements.f32[out_table->symbols[out_table->length - 1].data.array.length - 1] = value;

							word.start = NULL;
							word.length = 0;
							word.hash = 0;
						} else if (c == ';') {
							float value = wtof(word.start, word.length);
							out_table->symbols[out_table->length - 1].data.array.elements.f32[out_table->symbols[out_table->length - 1].data.array.length - 1] = value;

							word.start = NULL;
							word.length = 0;
							word.hash = 0;

							state = KOML_PARSER_STATE_NONE;
						} else if (!is_num(c) && c != '.') {
							printf("Invalid float value (line %llu: column %llu)\n  | ", line + 1, column + 1);
							koml_printline(buffer, line, column);
							printf("\n  | ");
							koml_printcursor(column);
							printf("\n");
							return 10;
						}

						continue;
					case KOML_TYPE_STRING:
						if (is_whitespace(c)) {
							continue;
						}

						if (c == '"') {
							++i;
							c = buffer[i];
							word.start = &buffer[i];
							word.length = 0;
							while (c != '"') {
								++i;
								++word.length;
								if (i >= buffer_length) {
									printf("String literal never ended (line %llu: column %llu)\n  | ", line + 1, column + 1);
									koml_printline(buffer, line, column);
									printf("\n  | ");
									koml_printcursor(column);
									printf("\n");
									return 2;
								}
								c = buffer[i];
							}

							out_table->symbols[out_table->length - 1].data.array.strides[out_table->symbols[out_table->length - 1].data.array.length - 1] = word.length;
							out_table->symbols[out_table->length - 1].data.array.elements.string[out_table->symbols[out_table->length - 1].data.array.length - 1] = malloc(word.length + 1);
							if (out_table->symbols[out_table->length - 1].data.array.elements.string[out_table->symbols[out_table->length - 1].data.array.length - 1] == NULL) {
								printf("Failed to allocate string buffer (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 3;
							}

							out_table->symbols[out_table->length - 1].data.array.elements.string[out_table->symbols[out_table->length - 1].data.array.length - 1][word.length] = '\0';
							memcpy(out_table->symbols[out_table->length - 1].data.array.elements.string[out_table->symbols[out_table->length - 1].data.array.length - 1], word.start, word.length);
						}

						if (c == ',') {
							word.start = NULL;
							word.length = 0;
							word.hash = 0;
							continue;
						}

						if (c == ';') {
							word.start = NULL;
							word.length = 0;
							word.hash = 0;
							state = KOML_PARSER_STATE_NONE;
							continue;
						}

						if (word.start != NULL) {
							++word.length;
						}
						continue;
					case KOML_TYPE_BOOLEAN:
						if (c == ',') {
							if (!is_boolean(word.start, word.length)) {
								printf("Invalid boolean value (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 5;
							}

							unsigned char value = wtotf(word.start, word.length);
							out_table->symbols[out_table->length - 1].data.array.elements.boolean[out_table->symbols[out_table->length - 1].data.array.length - 1] = value;

							word.start = NULL;
							word.length = 0;
							word.hash = 0;
						}
						if (c == ';') {
							if (!is_boolean(word.start, word.length)) {
								printf("Invalid boolean value (line %llu: column %llu)\n  | ", line + 1, column + 1);
								koml_printline(buffer, line, column);
								printf("\n  | ");
								koml_printcursor(column);
								printf("\n");
								return 5;
							}

							unsigned char value = wtotf(word.start, word.length);
							out_table->symbols[out_table->length - 1].data.array.elements.boolean[out_table->symbols[out_table->length - 1].data.array.length - 1] = value;

							word.start = NULL;
							word.length = 0;
							word.hash = 0;

							state = KOML_PARSER_STATE_NONE;
						}
						continue;
					case KOML_TYPE_ARRAY:
						printf("Arrays of arrays are not supported (line %llu: column %llu)\n  | ", line + 1, column + 1);
						koml_printline(buffer, line, column);
						printf("\n  | ");
						koml_printcursor(column);
						printf("\n");
						return 6;
					case KOML_TYPE_UNKNOWN:
					default:
						printf("Unknown type\n");
						return 7;
				}

				continue;
			case 0:
			default:
				break;
		}

		if (word.start != NULL) {
			word.start = NULL;
			word.length = 0;
		}

		if (c == '[') {
			state = KOML_PARSER_STATE_SECTION_WAIT;
		}

		if (c == 'i') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 4;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_INT;
			state = KOML_PARSER_STATE_INTEGER_WAIT;
		}
		if (c == 'f') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 4;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_FLOAT;
			state = KOML_PARSER_STATE_FLOAT_WAIT;
		}
		if (c == 's') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 0;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_STRING;
			state = KOML_PARSER_STATE_STRING_WAIT;
		}
		if (c == 'b') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 1;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_BOOLEAN;
			state = KOML_PARSER_STATE_BOOLEAN_WAIT;
		}
		if (c == 'a') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 0;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_ARRAY;
			state = KOML_PARSER_STATE_ARRAY_TYPE_WAIT;
		}

		if (state == KOML_PARSER_STATE_NONE && !is_whitespace(c)) {
			printf("Unexpected token (line %llu: column %llu)\n  | ", line + 1, column + 1);
			koml_printline(buffer, line, column);
			printf("\n  | ");
			koml_printcursor(column);
			printf("\n");
			return 9;
		}
	}

	return 0;
}

koml_symbol_t * koml_table_symbol(koml_table_t * table, char * name) {
	unsigned long long int hash = koml_internal_hash(name, strlen(name));

	for (unsigned long long int i = 0; i < table->length; ++i) {
		if (table->hashes[i] == hash) {
			return &table->symbols[i];
		}
	}

	return NULL;
}

koml_symbol_t * koml_table_symbol_word(koml_table_t * table, char * name, unsigned long long int name_length) {
	unsigned long long int hash = koml_internal_hash(name, name_length);

	for (unsigned long long int i = 0; i < table->length; ++i) {
		if (table->hashes[i] == hash) {
			return &table->symbols[i];
		}
	}

	return NULL;
}

int koml_table_destroy(koml_table_t * table) {
	if (table->hashes != NULL) {
		free(table->hashes);
	}

	if (table->symbols != NULL) {
		free(table->symbols);
	}

	return 0;
}
