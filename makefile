all:
	cd bot-client && $(MAKE)
	cd lizard && $(MAKE)
	cd game_server && $(MAKE)
	cd wasp-client && $(MAKE) 
debug: all
	gdb ./roaches-client --args 127.0.0.1:5557

clean:
	cd lizard && $(MAKE) clean
	cd bot-client && $(MAKE) clean
	cd game_server && $(MAKE) clean
	cd wasp-client && $(MAKE) clean
	rm lizard-client roaches-client lizardsNroaches-server wasp

