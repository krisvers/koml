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

	koml_table_t ktable;
	int ret = koml_table_load(&ktable, buffer, length);
	if (ret != 0) {
		free(buffer);
		return ret;
	}

	free(buffer);

	printf("table: ");
	koml_table_print(&ktable);

	koml_table_destroy(&ktable);

	return 0;
}
