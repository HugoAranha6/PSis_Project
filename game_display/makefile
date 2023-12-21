all: display.o 
	gcc -o ../display-app display.o -lncurses -lzmq

display.o: display.c
	gcc -g -c  display.c -lncurses -lzmq

clean:
	rm *.o 