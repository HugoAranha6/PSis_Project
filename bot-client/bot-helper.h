#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <zmq.h>
#include <ctype.h> 
#include <assert.h>
#include <ncurses.h>
#include "zhelpers.h"


#define MAX_BOTS 10

//FOR TESTING PURPOSES WITHOUT ARGC AND ARGV
//#define user_server_com "tcp://127.0.0.1:5555"
//#define server_user_com "tcp://*:5555"
//#define display_server_com "tcp://127.0.0.1:5556"
//#define server_display_com "tcp://*:5556"


// Movement direction, if 4 imples no movement,others not used
typedef enum direction_t {
    UP=0,
    DOWN=1,
    LEFT=2,
    RIGHT=3,
    HOLD=4,
    s_lizard=-1,
    s_roach=-2
}direction_t;

// Type of message to be used in bots is only CONNECT OR MOVE
typedef enum msg_type {CONNECT, MOVE, DISCONNECT} msg_type;

// Structure used to send messages of bots
typedef struct bot{   
    msg_type type;                    // Identify message type 0-Connect/1-Move
    int id[MAX_BOTS];                 // Identify bot id
    direction_t direction[MAX_BOTS] ; // Identify movement direction, 4=No move
    int token[MAX_BOTS];              // Identify bot, received from server on connect
}bot;

/*--------------------------------------------------------------
NOTE: Communication with server procedure.
      Connect:
        1. Send id of who is contacting: "roaches"
        2. Send number of bots to be connected
        3. Send bot communication message with scores
        4. Receive whether connection was successful or not
        5. Received atrributed bot ids and tokens
      Movement:
        1. Send id of who is contacting: "roaches"
        2. Send bot message with movements assigned to each bot
        3. Receive integer on success or failure
----------------------------------------------------------------*/


/*
Input: Pointer to a ZMQ socket pointer, bot type structure to
       exchange information, argc and argv.
Output: Integer, number of bots to be controlled. 
Initialize communication ZMQ socket, initialize bot score
which is sent in the id field when connecting,
score must be 0 if the bot is not assigned, bot connection
messages exchange with server.
*/
int roaches_initialize(void **requester, bot* bots, int argc, char*argv[]){
    if(argc==1){
        printf("Wrong number of command line arguments, check instructions!\n");
        exit(0);
    }
    char user_server_com[50];
    strcpy(user_server_com, "tcp://");
    strcat(user_server_com, argv[1]);
    
    // Initialize communication to server socket
    void *context = zmq_ctx_new ();
    *requester = zmq_socket (context, ZMQ_REQ);
    int rc = zmq_connect (*requester, user_server_com);
    assert(rc==0);

    srand(getpid());
    int n_bots = rand()%(MAX_BOTS)+1;
    (*bots).type=0;

    int i=0;
    // Atribute bot score
    while (i<MAX_BOTS){
        if (i<n_bots){
            (*bots).id[i]=rand()%(5)+1;  
        }else{
            (*bots).id[i]=0;
        } 
        i++;
    }
    
    // Server connection message exchange
    assert(s_sendmore(*requester,"roaches")!=-1);
    assert(zmq_send (*requester, bots, sizeof(bot), ZMQ_SNDMORE)!=-1);
    rc = zmq_send(*requester, &n_bots, sizeof(n_bots), 0);
    assert(rc!=-1);

    int check=0;
    rc = zmq_recv (*requester, &check, sizeof(int), 0); // check=-1 connect failed
    assert(rc!=-1);
    if(check == -1){
        printf("Server full, try again later!\n");
        exit(0);
    }else{
        rc = zmq_recv (*requester, bots, sizeof(bot), 0); // Receive message with bot ids and tokens
    }
    return n_bots;
}

/*
Input: REQ type socket pointer, structure for bot message,
       number of controlled bots
Output: -
Generates random movement direction between 0-4, where 0-3
is UP-DOW-LEFT-RIGHT and 4 is no move, and send to the server.
*/
void bot_input(void* requester, bot* roaches,int number_bots){
    srand(getpid());
    (*roaches).type=1;
   while(1){ 
        //random direction assignment for each controlled bot  
        for(size_t i=0; i<number_bots; i++){
            (*roaches).direction[i]=rand()%5;
        }
        sleep(1);
        // Server communication according to defined procedure
        assert(s_sendmore(requester,"roaches")!=-1);
        int rc = zmq_send(requester, roaches, sizeof(bot), 0);
        assert(rc!=-1);
        assert(zmq_recv(requester,&rc,sizeof(int),0)!=-1);          
   } 
}

