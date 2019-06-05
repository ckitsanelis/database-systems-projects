

all: execHT execEH



execEH: EH_main.o exhash.o

	gcc -o execEH exhash.o BF_64.a EH_main.o -lm



execHT: HT_main.o hash.o

	gcc -o execHT hash.o BF_64.a HT_main.o



EH_main.o: EH_main.c
	
	gcc -c -Wall EH_main.c



HT_main.o: HT_main.c

	gcc -c -Wall HT_main.c



exhash.o: exhash.c

	gcc -c -Wall -lm exhash.c



hash.o: hash.c

	gcc -c -Wall hash.c



clean:
	
	rm -f execHT HT_main.o hash.o
	rm -f execEH EH_main.o exhash.o

