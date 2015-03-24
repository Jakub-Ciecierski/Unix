all: main.o unix.o
	gcc -Wall -o main main.o unix.o

main.o: main.c unix.h
	gcc -Wall -o main.o -c main.c

unix.o: unix.c unix.h
	gcc -Wall -o unix.o -c unix.c
	
.PHONY: clean

clean:
	rm main main.o unix.o
