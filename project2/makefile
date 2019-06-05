

all: execSort



execSort: main.o sort.o algorithms.o

	gcc -o execSort algorithms.o sort.o BF_64.a main.o -lm



main.o: main.c

	gcc -c -Wall main.c



sort.o: sort.c

	gcc -c -Wall sort.c



algorithms.o: algorithms.c

	gcc -c -Wall algorithms.c


clean:
	
	rm -f execSort main.o sort.o algorithms.o
