#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <zmq.h>
#include <ctype.h> 
#include <assert.h>
#include <ncurses.h>
#include <curses.h>
#include "zhelpers.h"

// FOR TESTING PURPOSES WITHOUT ARGC AND ARGV
//#define server_user_com "tcp://*:5555"
//#define display_server_com "tcp://127.0.0.1:5556"
//#define server_display_com "tcp://*:5556"


// User movement direction
typedef enum direction_t {
    UP=0,
    DOWN=1,
    LEFT=2,
    RIGHT=3,
    HOLD=4,
    s_lizard=-1,
    s_roach=-2,
    s_wasp =-3
}direction_t;

// Message type, all types available for users
typedef enum msg_type {CONNECT, MOVE, DISCONNECT} msg_type;

// User-Server messages
typedef struct client{   
    msg_type type;          // Identify message type 0/1/2
    char ch;                // Identify user 
    direction_t direction ; // Identify movement direction
    int token;              // Identify user
}client;

/*--------------------------------------------------------------
NOTE: Communication with server procedure.
      Connect:
        1. Send id of who is contacting: "client"
        2. Send message of type CONNECT
        3. Receive an attributed char, char=-1 failled connection
        4. Receive a token, to be used as 2nd verification for moving
      Movement:
        1. Send id of who is contacting: "client"
        2. Send client message of type MOVE with movement input 
        3. Receive current user score
      Disconnect
        1. Send if id of who is contacting: "client"
        2. Send client message of type DISCONNECT
        3. Receive user current score
----------------------------------------------------------------*/

/*
Input: Pointer to ZMQ REQ socket, pointer to client type variable, 
       argc and argv
Ooutput: -
Function to initiliaze user environment: connects the socket based
on command line input, request server for user connection,
receive char and token from server if available slots. On exit
m will have ch and token fields changed according to server info.
*/
void user_initialize(void **requester,client* m, int argc,char* argv[]){
    if(argc==1){
        printf("Wrong number of command line arguments, check instructions!\n");
        exit(0);
    }
    char user_server_com[50];
    strcpy(user_server_com, "tcp://");
    strcat(user_server_com, argv[1]);

    void *context = zmq_ctx_new ();
    
    // Initialize communication to server socket
    *requester = zmq_socket (context, ZMQ_REQ);
    int rc = zmq_connect (*requester, user_server_com);
    assert(rc==0);
    (*m).type=0;
    char ch_tmp;
    do
    {
        printf("Do you want to enter the game? (y/n): ");
        scanf(" %c", &ch_tmp);
        // Clear the input buffer to handle extra characters
        while (getchar() != '\n');
    } while (ch_tmp!='y');
    
    // Connection message exchange
    s_sendmore(*requester,"client");
    rc = zmq_send (*requester, m, sizeof(client), 0);
    assert(rc!=-1);

    assert(zmq_recv (*requester, &ch_tmp, sizeof(char), 0)!=-1);
    
    
    if(ch_tmp == -1){
        // No slots available
        printf("Server full, try again later!\n");
        exit(0);
    }else{
        // Connection successful
        assert(zmq_recv (*requester, &rc, sizeof(rc), 0)!=-1);
    }
    // Update user attributed char and token, to be used for communication
    (*m).ch = ch_tmp;
    (*m).token = rc;
}

/*
Inputs: Two pointers to window points: title and score windows
Output: -
Function that initializes the user interface
*/
void user_interface(WINDOW **title_win, WINDOW** score_win){
    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    
    refresh();
    /* creates windows */
    *title_win = newwin(1, 50, 1, 1);

    *score_win = newwin(3, 30, 2, 1);
    box(*score_win, 0 , 0);	
	wrefresh(*score_win);
}

/*
Input: requester socket and a client message to be sent
Output: integer, current user score received 
User-Server standard communication when sending movements
*/
int user_server_message(void* requester,client m){
    int score;
    // Movement type communication procedure
    assert(s_sendmore(requester,"client")!=-1);
    int rc = zmq_send (requester, &m, sizeof(m), 0);
    assert(rc!=-1);
    // Receive current score
    rc = zmq_recv (requester, &score, sizeof(score), 0);
    assert(rc!=-1);
    return score;
}

/*
Input: requester socket, pointer to client type message
       integer score pointer
Output: -
Function that runs while the user provides control inputs
to its lizard. Gets input from keyboard, send message to
server and receives score. When pressing Q to leave the game,
a DISCONNECT message is sent and variable score will be assigned
to the lizard current score
*/
void user_input(void* requester,client* m,int* score,WINDOW** title_win,WINDOW** score_win){
    
    int key;
    (*m).type = 1;

    
    mvwprintw(*score_win, 1, 1, "Score: %d",*score);
    wrefresh(*score_win);
    do
    {   
        key = getch();	// Keyboard input
        switch (key)
        {
        case KEY_LEFT:
            werase(*title_win);
            mvwprintw(*title_win,0,0,"Left arrow is pressed");
            (*m).direction = LEFT;
            break;
        case KEY_RIGHT:
            werase(*title_win);
            mvwprintw(*title_win,0,0,"Right arrow is pressed");
            (*m).direction = RIGHT;
            break;
        case KEY_DOWN:
            werase(*title_win);
            mvwprintw(*title_win,0,0,"Down arrow is pressed");
            (*m).direction = DOWN;
            break;
        case KEY_UP:
            werase(*title_win);
            mvwprintw(*title_win,0,0,"Up arrow is pressed");
            (*m).direction = UP;
            break;
        case 81:
        case 113:
            // Q or q case for DISCONNECT
            werase(*title_win);
            mvwprintw(*title_win,0,0,"Leaving the game");
            (*m).type = 2; // change message type to DISCONNECT
            break;
        default:
            key = 'x'; 
            break;
        }

         if (key != 'x'){
            *score = user_server_message(requester,*m);
            werase(*score_win);
            mvwprintw(*score_win,1,1,"Score: %d",*score); // Update current score displayed
        }
        /* Print it on screen */
        wrefresh(*title_win);
        wrefresh(*score_win);
    }while(key != 81 && key != 113);
    endwin();
}

