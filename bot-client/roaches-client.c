#include "bot-helper.h"

int main(int argc, char*argv[]){	 
    void *requester; // Request ZMQ socket
    bot roaches;     // Struct from bot-server communication
    
    // Server connection of #number_bots bots
    int number_bots = roaches_initialize(&requester, &roaches,argc,argv);

    // Send control commands for the bots
    bot_input(requester,&roaches,number_bots);

    return 0;
}