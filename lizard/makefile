all: lizard-client.o 
	gcc -o ../lizard-client lizard-client.o -lncurses -lzmq

lizard-client.o: lizard-client.c
	gcc -g -c  lizard-client.c -lncurses -lzmq

clean:
	rm *.o 