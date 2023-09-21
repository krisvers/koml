#include <stdio.h>
#include <stdlib.h>
#include "koml/koml.h"

int main(int argc, char ** argv) {
	char * buffer;
	unsigned long long int length;

	{
		FILE * fp = fopen("./test.koml", "rb");
		if (fp == NULL) {
			printf("Failed to open ./test.koml\n");
			return 1;
		}

		fseek(fp, 0L, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		buffer = malloc(length + 1);
		if (buffer == NULL) {
			printf("Failed to allocate buffer\n");
			abort();
		}
		buffer[length] = '\0';

		if (fread(buffer, 1, length, fp) != length) {
			printf("Failed to read from ./test.koml\n");
			return 2;
		}
	}

	printf("%s\n", buffer);

	koml_table_t ktable;
	int ret = koml_table_load(&ktable, buffer, length);
	if (ret != 0) {
		printf("Failed to load table %i\n", ret);
		free(buffer);
		return 3;
	}

	free(buffer);

	koml_symbol_t * aptr = koml_table_symbol(&ktable, "section:a");
	if (aptr == NULL) {
		printf("Failed to load symbol \"section:a\"\n");
		return 4;
	}
	koml_symbol_t * cptr = koml_table_symbol(&ktable, "section.sub:c");
	if (cptr == NULL) {
		printf("Failed to load symbol \"section.sub:c\"\n");
		return 5;
	}
	koml_symbol_t * fptr = koml_table_symbol(&ktable, "section:f");
	if (fptr == NULL) {
		printf("Failed to load symbol \"section:f\"\n");
		return 6;
	}
	koml_symbol_t * sptr = koml_table_symbol(&ktable, "section:s");
	if (sptr == NULL) {
		printf("Failed to load symbol \"section:s\"\n");
		return 7;
	}
	koml_symbol_t * bptr = koml_table_symbol(&ktable, "section.sub:b");
	if (bptr == NULL) {
		printf("Failed to load symbol \"section.sub:b\"\n");
		return 8;
	}
	koml_symbol_t * testptr = koml_table_symbol(&ktable, "test");
	if (testptr == NULL) {
		printf("Failed to load symbol \"section.sub:b\"\n");
		return 8;
	}

	int a = aptr->data.i32;
	int c = cptr->data.i32;
	float f = fptr->data.f32;
	char * str = sptr->data.string;
	unsigned char b = bptr->data.boolean;
	int test = testptr->data.i32;

	printf("a = %i; c = %i; f = %f; b = %u; test = %i\n", a, c, f, b, test);
	printf("s = %s\n", str);

	return 0;
}
