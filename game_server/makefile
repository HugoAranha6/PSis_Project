all: server.o 
	gcc -o ../lizardsNroaches-server server.o bot.pb-c.o -lncurses -lzmq -pthread -lprotobuf-c

server.o: server.c bot.pb-c.c
	protoc-c --c_out=. bot.proto
	gcc -g -c  server.c bot.pb-c.c -lncurses -lzmq

clean:
	rm *.o bot.pb-c.h bot.pb-c.c