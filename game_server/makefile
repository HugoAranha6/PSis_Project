all: server.o 
	gcc -o ../lizardsNroaches-server server.o -lncurses -lzmq

server.o: server.c
	gcc -g -c  server.c -lncurses -lzmq

clean:
	rm *.o 