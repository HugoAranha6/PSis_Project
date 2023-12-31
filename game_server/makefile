all: server.o 
	gcc -o ../lizardsNroaches-server server.o bot.pb-c.o -lncurses -lzmq -pthread -lprotobuf-c

server.o: server.c bot.pb-c.c
	gcc -g -c  server.c bot.pb-c.c -lncurses -lzmq

clean:
	rm *.o 