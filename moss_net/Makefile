CC=gcc
CFLAGS=-I./include

elfload_test: moss_elfload.o elfload_test.o
	$(CC) -o elfload_test moss_elfload.o elfload_test.o $(CFLAGS)

moss_elfload.o: src/moss_elfload.c 
	$(CC) -c -o moss_elfload.o src/moss_elfload.c $(CFLAGS)

elfload_test.o: elfload_test.c 
	$(CC) -c -o elfload_test.o elfload_test.c $(CFLAGS)