#ifndef KRISVERS_KOML_H
#define KRISVERS_KOML_H

typedef enum KOMLType {
	KOML_TYPE_UNKNOWN = 0,
	KOML_TYPE_INT = 1,
	KOML_TYPE_FLOAT = 2,
	KOML_TYPE_STRING = 3,
	KOML_TYPE_BOOLEAN = 4,
} koml_type_enum;

typedef struct KOMLSymbol {
	unsigned long long int stride;
	koml_type_enum type;
	union {
		int i32;
		float f32;
		char * string;
		unsigned char boolean;
	} data;
} koml_symbol_t;

typedef struct KOMLTable {
	koml_symbol_t * symbols;
	unsigned long long int * hashes;
	unsigned long long int length;
} koml_table_t;

int koml_table_load(koml_table_t * out_table, char * buffer, unsigned long long int buffer_length);
koml_symbol_t * koml_table_symbol(koml_table_t * table, char * name);
int koml_table_destroy(koml_table_t * table);

#endif
