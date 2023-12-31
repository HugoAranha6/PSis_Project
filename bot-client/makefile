all: roaches-client.o 
	gcc -o ../roaches-client roaches-client.o bot.pb-c.o -lncurses -lzmq -pthread -lprotobuf-c

roaches-client.o: roaches-client.c bot.proto
	protoc-c --c_out=. bot.proto
	gcc -g -c  roaches-client.c bot.pb-c.c -lncurses -lzmq


clean:
	rm *.o bot.pb-c.h bot.pb-c.c