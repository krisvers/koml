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

	int status = 0;
	unsigned long long int line = 0;
	unsigned long long int column = 0;

	for (unsigned long long int i = 0; i < buffer_length; ++i) {
		c = buffer[i];
		++column;

		if (c == '|') {
			++i;
			c = buffer[i];
			while (c != '|') {
				++i;
				if (i >= buffer_length) {
					printf("Comment never ended (%llu:%llu)\n", line + 1, column + 1);
					return 6;
				}
				c = buffer[i];
			}
		}

		if (c == '\n') {
			++line;
			column = 0;
		}

		switch (status) {
			case 1:
				word.start = &buffer[i];
				word.length = 0;
				status = 2;
				continue;
			case 2:
				++word.length;
				if (c == ']') {
					status = 0;
					section.start = word.start;
					section.length = word.length;
				}
				continue;
			case 3:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				status = 4;
				continue;
			case 4:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					char * tmp = NULL;
					unsigned long long int tmp_length = 0;
					if (section.start != NULL) {
						tmp_length = word.length + section.length + 1;
						tmp = malloc(tmp_length);
						if (tmp == NULL || word.start == NULL || section.start == NULL) {
							printf("Internal error (%llu:%llu)\n", line + 1, column + 1);
							return 1;
						}

						memcpy(tmp, section.start, section.length);
						tmp[section.length] = ':';
						memcpy(&tmp[section.length + 1], word.start, word.length);
					} else {
						tmp = word.start;
						tmp_length = word.length;
					}
					if (tmp == NULL) {
						printf("Internal error (%llu:%llu)\n", line + 1, column + 1);
						return 1;
					}

					out_table->hashes[out_table->length - 1] = koml_internal_hash(tmp, tmp_length);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 5;
				}
				continue;
			case 5:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == ';') {
					int value = wtoi(word.start, word.length, 10);
					out_table->symbols[out_table->length - 1].data.i32 = value;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 0;
					continue;
				}

				if (word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
				}
				++word.length;
				continue;
			case 6:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				status = 7;
				continue;
			case 7:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					char * tmp = malloc(word.length + section.length + 1);
					if (tmp == NULL || word.start == NULL || section.start == NULL) {
						printf("Internal error (%llu:%llu)\n", line + 1, column + 1);
						return 1;
					}

					memcpy(tmp, section.start, section.length);
					tmp[section.length] = ':';
					memcpy(&tmp[section.length + 1], word.start, word.length);

					out_table->hashes[out_table->length - 1] = koml_internal_hash(tmp, word.length + section.length + 1);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 8;
				}
				continue;
			case 8:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == ';') {
					float value = wtof(word.start, word.length);
					out_table->symbols[out_table->length - 1].data.f32 = value;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 0;
					continue;
				}

				if (word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
				}
				++word.length;
				continue;
			case 9:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				status = 10;
				continue;
			case 10:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					char * tmp = malloc(word.length + section.length + 1);
					if (tmp == NULL || word.start == NULL || section.start == NULL) {
						printf("Internal error (%llu:%llu)\n", line + 1, column + 1);
						return 1;
					}

					memcpy(tmp, section.start, section.length);
					tmp[section.length] = ':';
					memcpy(&tmp[section.length + 1], word.start, word.length);

					out_table->hashes[out_table->length - 1] = koml_internal_hash(tmp, word.length + section.length + 1);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 11;
				}
				continue;
			case 11:
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
							printf("String literal never ended (%llu:%llu)\n", line + 1, column + 1);
							return 2;
						}
						c = buffer[i];
					}

					out_table->symbols[out_table->length - 1].stride = word.length;
					out_table->symbols[out_table->length - 1].data.string = malloc(word.length + 1);
					if (out_table->symbols[out_table->length - 1].data.string == NULL) {
						printf("Failed to allocate string buffer (%llu:%llu)\n", line + 1, column + 1);
						return 3;
					}
					out_table->symbols[out_table->length - 1].data.string[word.length] = '\0';
					memcpy(out_table->symbols[out_table->length - 1].data.string, word.start, word.length);
				}


				if (c == ';') {
					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 0;
					continue;
				}

				if (word.start != NULL) {
					++word.length;
				}
				continue;
			case 12:
				if (is_whitespace(c)) {
					continue;
				}

				word.start = &buffer[i];
				word.length = 0;
				status = 13;
				continue;
			case 13:
				if (is_whitespace(c)) {
					continue;
				}

				++word.length;
				if (c == '=') {
					char * tmp = malloc(word.length + section.length + 1);
					if (tmp == NULL || word.start == NULL || section.start == NULL) {
						printf("Invalid boolean value (%llu:%llu)\n", line + 1, column + 1);
						return 1;
					}

					memcpy(tmp, section.start, section.length);
					tmp[section.length] = ':';
					memcpy(&tmp[section.length + 1], word.start, word.length);

					out_table->hashes[out_table->length - 1] = koml_internal_hash(tmp, word.length + section.length + 1);

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 14;
				}
				continue;
			case 14:
				if (is_whitespace(c)) {
					continue;
				}

				if (c == ';') {
					if (!is_boolean(word.start, word.length)) {
						printf("Invalid boolean value (%llu:%llu)\n", line + 1, column + 1);
						return 5;
					}
					unsigned char value = wtotf(word.start, word.length);
					out_table->symbols[out_table->length - 1].data.boolean = value;

					word.start = NULL;
					word.length = 0;
					word.hash = 0;
					status = 0;
					continue;
				}

				if (word.start == NULL) {
					word.start = &buffer[i];
					word.length = 0;
				}
				++word.length;
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
			status = 1;
		}

		if (c == 'i') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 4;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_INT;
			status = 3;
		}

		if (c == 'f') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 4;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_FLOAT;
			status = 6;
		}
		if (c == 's') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 0;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_STRING;
			status = 9;
		}
		if (c == 'b') {
			koml_table_alloc_new(out_table);
			out_table->symbols[out_table->length - 1].stride = 1;
			out_table->symbols[out_table->length - 1].type = KOML_TYPE_BOOLEAN;
			status = 12;
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

int koml_table_destroy(koml_table_t * table) {
	if (table->hashes != NULL) {
		free(table->hashes);
	}

	if (table->symbols != NULL) {
		free(table->symbols);
	}

	return 0;
}
