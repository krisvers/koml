CFLAGS=-O2 -march=native -pipe -Wall

main: koml/koml.o koml/koml.h

koml/koml.o: koml/koml.c koml/koml.h
