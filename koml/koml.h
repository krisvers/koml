#ifndef KRISVERS_KOML_H
#define KRISVERS_KOML_H

typedef enum KOMLType {
	KOML_TYPE_UNKNOWN = 0,
	KOML_TYPE_INT = 1,
	KOML_TYPE_FLOAT = 2,
	KOML_TYPE_STRING = 3,
	KOML_TYPE_BOOLEAN = 4,
	KOML_TYPE_ARRAY = 5,
} koml_type_enum;

typedef struct KOMLArray {
	unsigned long long int length;
	unsigned long long int * strides;
	union {
		int * i32;
		float * f32;
		char ** string;
		unsigned char * boolean;
		void * voidptr;
	} elements;
	koml_type_enum type;
} koml_array_t;

typedef struct KOMLSymbol {
	char * name;
	unsigned long long int stride;
	koml_type_enum type;
	union {
		int i32;
		float f32;
		char * string;
		unsigned char boolean;
		koml_array_t array;
	} data;
} koml_symbol_t;

typedef struct KOMLTable {
	koml_symbol_t * symbols;
	unsigned long long int * hashes;
	unsigned long long int length;
} koml_table_t;

void koml_symbol_print(koml_symbol_t * symbol);
void koml_table_print(koml_table_t * table);
int koml_table_load(koml_table_t * out_table, char * buffer, unsigned long long int buffer_length);
koml_symbol_t * koml_table_symbol(koml_table_t * table, char * name);
koml_symbol_t * koml_table_symbol_word(koml_table_t * table, char * name, unsigned long long int name_length);
int koml_table_destroy(koml_table_t * table);

#endif
