all: roaches-client.o 
	gcc -o ../roaches-client roaches-client.o -lncurses -lzmq

roaches-client.o: roaches-client.c
	gcc -g -c  roaches-client.c -lncurses -lzmq

clean:
	rm *.o 