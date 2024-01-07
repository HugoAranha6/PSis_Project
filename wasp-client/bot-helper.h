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
#include "bot.pb-c.h"
#include <pthread.h>


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
    s_roach=-2,
    s_wasp = -3
}direction_t;

// Type of message to be used in bots is only CONNECT OR MOVE
typedef enum msg_type {CONNECT, MOVE, DISCONNECT} msg_type;

pthread_mutex_t requester_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t ctrl_c_pressed = 0;

/*--------------------------------------------------------------
NOTE: Communication with server procedure.
      Connect:
        1. Send id of who is contacting: "wasps"
        2. Send number of bots to be connected
        3. Receive whether connection was successful or not
        4. Received atrributed bot ids and tokens
      Movement:
        1. Send id of who is contacting: "wasps"
        2. Send bot message with movements assigned to each bot
        3. Receive a integer
      Disconnect:
        1 Send id of who contacts: "wasp"
        2 Send disconnect message with ids and tokens
        3 Receive integer
----------------------------------------------------------------*/
 
 
/*
Input: Pointer to a ZMQ socket pointer, bot type structure to
       exchange information, argc and argv.
Output: Integer, number of wasps to be controlled.
Initialize communication ZMQ socket, initialize bot score
which is sent in the id field when connecting, bot connection
messages exchange with server.
*/
int wasps_initialize(void **requester, ConnectRepply** bots, int argc, char*argv[]){
    
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

    // Generate number of bots
    srand(getpid());
    int n_bots = rand()%(MAX_BOTS)+1;
    
    msg_type m_type = CONNECT;

    // Send connection message sequence
    assert(s_sendmore(*requester,"wasps")!=-1);
    assert(zmq_send(*requester, &m_type,sizeof(m_type),ZMQ_SNDMORE)!=-1);
    assert(zmq_send(*requester, &n_bots, sizeof(n_bots),0)!=-1);

    int check=0;
    rc = zmq_recv (*requester, &check, sizeof(int), 0); // check=-1 connect failed
    assert(rc!=-1);
    if(check == -1){
        printf("Server full, try again later!\n");
        exit(0);
    }else{
        // Receive repply with ids and tokens if successful connection
        ConnectRepply* m_repply;
        zmq_msg_t zmq_msg;
        zmq_msg_init(&zmq_msg);
        int n_bytes = zmq_recvmsg(*requester,&zmq_msg,0);
        void* data = zmq_msg_data(&zmq_msg);
        m_repply = connect_repply__unpack(NULL,n_bytes,data);
        *bots = m_repply;
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
void bot_input(void* requester, ConnectRepply* wasps,int number_bots){
    srand(getpid());
    int tmp;
    msg_type m_type = MOVE;
    // Variable for bot movement communication with server
    BotMovement m_movement = BOT_MOVEMENT__INIT;
    m_movement.n_id = number_bots;
    m_movement.n_movement = number_bots;
    m_movement.n_token = number_bots;
    m_movement.id = malloc(sizeof(int32_t)*number_bots);
    m_movement.token = malloc(sizeof(int32_t)*number_bots);
    m_movement.movement = malloc(sizeof(DirectionMove)*number_bots);
    memcpy(m_movement.id,wasps->id,sizeof(int32_t)*number_bots);
    memcpy(m_movement.token,wasps->token,sizeof(int32_t)*number_bots);

   while(1){ 
        //random direction assignment for each controlled bot 
        sleep(1); 
        pthread_mutex_lock(&requester_mutex);
        if(ctrl_c_pressed==1){
            pthread_mutex_unlock(&requester_mutex);
            break;
        }
        for(size_t i=0; i<number_bots; i++){
            // Generate random movements
            m_movement.movement[i]=rand()%5;
        }
        
        int n_bytes = bot_movement__get_packed_size(&m_movement);
        char *msg = (char*)malloc(n_bytes);
        bot_movement__pack(&m_movement,msg);
        // Server communication according to defined procedure
        assert(s_sendmore(requester,"wasps")!=-1);
        assert(zmq_send(requester,&m_type,sizeof(m_type),ZMQ_SNDMORE)!=-1);
        assert(zmq_send(requester,msg,n_bytes,0)!=-1);

        assert(zmq_recv(requester,&tmp,sizeof(int),0)!=-1);
        pthread_mutex_unlock(&requester_mutex);      
   } 
}

