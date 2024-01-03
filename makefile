all:
	cd bot-client && $(MAKE)
	cd lizard && $(MAKE)
	cd game_server && $(MAKE)
	cd wasp-client && $(MAKE) 

clean:
	cd lizard && $(MAKE) clean
	cd bot-client && $(MAKE) clean
	cd game_server && $(MAKE) clean
	cd wasp-client && $(MAKE) clean
	rm lizard-client roaches-client lizardsNroaches-server wasp

