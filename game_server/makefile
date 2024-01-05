all: server.o
	gcc -o ../lizardsNroaches-server server.o bot.pb-c.o -lncurses -lzmq -pthread -lprotobuf-c

server.o: server.c bot.pb-c.c
	gcc -g -c  server.c bot.pb-c.c -lncurses -lzmq
bot.pb-c.c: bot.proto
	protoc-c --c_out=. bot.proto

clean:
	rm *.o bot.pb-c.h bot.pb-c.c

