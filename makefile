all: my_ls

my_ls: main.o
	gcc main.o -o my_ls

main.o: main.c
	gcc -Wall -g -c main.c

clean:
	rm -fr *.o my_ls
