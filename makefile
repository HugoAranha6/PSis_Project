all:
	cd bot-client && $(MAKE)
	cd lizard && $(MAKE)
	cd game_display && $(MAKE)
	cd game_server && $(MAKE)
debug: all
	gdb ./roaches-client --args 127.0.0.1:5557

clean:
	cd lizard && $(MAKE) clean
	cd bot-client && $(MAKE) clean
	cd game_display && $(MAKE) clean
	cd game_server && $(MAKE) clean
	rm display-app lizard-client roaches-client lizardsNroaches-server

