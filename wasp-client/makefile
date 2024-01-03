all: wasp-client.o 
	gcc -o ../wasp wasp-client.o bot.pb-c.o -lncurses -lzmq -pthread -lprotobuf-c

wasp-client.o: wasp-client.c bot.proto
	protoc-c --c_out=. bot.proto
	gcc -g -c  wasp-client.c bot.pb-c.c -lncurses -lzmq


clean:
	rm *.o bot.pb-c.h bot.pb-c.c