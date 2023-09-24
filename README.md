# koml
krisvers' (not so) obvious markup language parser.

### example usage (in C):
```koml
[section]
i integer = 9451;
  [section.sub]
  b boolean = true;
[settings]
f float = 6.2;
as names = "Todd", "Jennifer", "Nathan", "Anna";
```
```c
char * buffer;
unsigned long long int buffer_length;

/* load koml-compliant config into buffer and store the length in buffer_length */

koml_table_t ktable;
if (koml_table_load(&ktable, buffer, buffer_length) != 0) {
  // failed to load/parse the table
}

free(buffer);

koml_symbol_t * integer_ptr = koml_table_symbol(&ktable, "section:integer");
if (integer_ptr == NULL) {
  // failed to find symbol inside the pre-parsed table
}

koml_symbol_t * boolean_ptr = koml_table_symbol(&ktable, "section.sub:boolean");
if (boolean_ptr == NULL) {
  // failed to find symbol inside the pre-parsed table
}

koml_symbol_t * float_ptr = koml_table_symbol(&ktable, "settings:float");
if (float_ptr == NULL) {
  // failed to find symbol inside the pre-parsed table
}

koml_symbol_t * names_ptr = koml_table_symbol(&ktable, "settings:names");
if (names_ptr == NULL) {
  // failed to find symbol inside the pre-parsed table
}

int integer = integer_ptr->data.i32;
unsigned char boolean = integer_ptr->data.boolean;
float floating = float_ptr->data.f32;
printf("integer: %i, boolean: %u, float: %f\n", integer, boolean, floating);

koml_array_t * names = &names_ptr->data.array;
for (unsigned long long int i = 0; i < names->length; ++i) {
  printf("%s, ", names->elements.string[i]);
}
printf("\n");

koml_table_destroy(&ktable);
```
