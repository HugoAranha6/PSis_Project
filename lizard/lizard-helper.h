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
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include "zhelpers.h"
#include <limits.h>

// FOR TESTING PURPOSES WITHOUT ARGC AND ARGV
//#define server_user_com "tcp://*:5555"
//#define display_server_com "tcp://127.0.0.1:5556"
//#define server_display_com "tcp://*:5556"
#define BOARD_SIZE 30
#define WINDOW_SIZE (BOARD_SIZE+2)
#define BODY_SIZE 5

// User movement direction
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

// Message type, all types available for users
typedef enum msg_type {CONNECT, MOVE, DISCONNECT} msg_type;

// User-Server messages for movement
typedef struct client{   
    char ch;                // Identify user 
    direction_t direction ; // Identify movement direction
    int token;              // Identify user
}client;

// User-Server messages for disconnect
typedef struct client_disconnect{   
    char ch;                // Identify user 
    int token;              // Identify user
}client_disconnect;

// Server to Display messages
typedef struct display_data{
    int ch;
    int pos_x0, pos_y0;     // Old position
    int pos_x1, pos_y1;     // New position
    int score;
    int token;
    direction_t direction;
}display_data;

pthread_mutex_t mutex_print = PTHREAD_MUTEX_INITIALIZER;

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
int user_initialize(void **requester,void **subscriber, client* m, int argc,char* argv[], display_data grid[][WINDOW_SIZE]){
    if(argc!=3){
        printf("Wrong number of command line arguments, check instructions!\n");
        exit(0);
    }
    char user_server_com[50];
    strcpy(user_server_com, "tcp://");
    strcat(user_server_com, argv[1]);

    void *context = zmq_ctx_new ();
    
    // Initialize communication to server socket
    *requester = zmq_socket (context, ZMQ_REQ);
    *subscriber = zmq_socket (context, ZMQ_SUB);
    int rc = zmq_connect (*requester, user_server_com);
    assert(rc==0);

    char ch_tmp;
    do
    {
        printf("Do you want to enter the game? (y/n): ");
        scanf(" %c", &ch_tmp);
        // Clear the input buffer to handle extra characters
        while (getchar() != '\n');
    } while (ch_tmp!='y');
    
    msg_type m_type =0;
    // Connection message exchange
    assert(s_sendmore(*requester,"client")!=-1);
    assert(zmq_send (*requester, &m_type, sizeof(msg_type), 0)!=-1);

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


    /*Display initialize*/
    int token_display;
    s_send(*requester,"display");
    zmq_recv(*requester, &token_display, sizeof(token_display),0);

    

    // Initial information pull of current game state
    char id[100]="\0";
    int print=0;
    int n_lizards=0;
    do{            
        zmq_recv (*requester, &id, sizeof(id), 0);
        if(strcmp(id,"data")==0){
            display_data new_data;
            zmq_recv (*requester, &new_data, sizeof(new_data), 0);
            grid[new_data.pos_x0][new_data.pos_y0]=new_data;
            if (new_data.direction!=-2 &&  new_data.direction!=-3){
                n_lizards++;
            }
            print=1;
        }
    } while (strcmp(id,"update")!=0);
    

    char display_server_com[50];
    strcpy(display_server_com, "tcp://");
    strcat(display_server_com, argv[2]);

    char token_char[100];
    sprintf(token_char,"%d",token_display);

    zmq_connect (*subscriber, display_server_com);
    zmq_setsockopt (*subscriber, ZMQ_SUBSCRIBE,token_char, strlen(token_char));

    return n_lizards;
}

/*
Inputs: Two pointers to window points: title and score windows
Output: -
Function that initializes the user interface
*/
void user_interface(WINDOW **title_win, WINDOW** score_win, WINDOW** game_win){
    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    
    refresh();
    /* creates windows */
    *title_win = newwin(1, 50, 1, 1);

    *game_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 2, 1);
    box(*game_win, 0 , 0);	
	wrefresh(*game_win);

    *score_win = newwin(30, 10, 2, WINDOW_SIZE+5);
    box(*score_win, 0 , 0);
    mvwprintw(*score_win, 1, 1, "Score:");	
	wrefresh(*score_win);
}

/*
Input: requester socket and a client message to be sent
Output: integer, current user score received 
User-Server standard communication when sending movements
*/
int user_server_message(void* requester,client m, msg_type m_type){
    int score;
    // Movement type communication procedure
    assert(s_sendmore(requester,"client")!=-1);
    assert(zmq_send(requester,&m_type,sizeof(m_type),ZMQ_SNDMORE)!=-1);
    if(m_type == MOVE){
        assert(zmq_send (requester, &m, sizeof(m), 0)!=-1);
    }else if(m_type==DISCONNECT){
        client_disconnect lizard_disc={.ch=m.ch,.token=m.token};
        assert(zmq_send (requester, &lizard_disc, sizeof(lizard_disc), 0)!=-1);
    }
    

    // Receive current score
    assert(zmq_recv (requester, &score, sizeof(score), 0)!=-1);
    return score;
}

// Flag to indicate if Ctrl+C is pressed
volatile sig_atomic_t ctrl_c_flag = 0;

// Signal handler for Ctrl+C
void handle_ctrl_c(int signum,void* requester,client* m) {
    ctrl_c_flag=1;
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
    
    signal(SIGINT, (void (*)(int))handle_ctrl_c);

    int key;
    int score_tmp;
    msg_type m_type=1;
    do
    {   
        key = getch();	// Keyboard input
        pthread_mutex_lock(&mutex_print);
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
            m_type = 2; // change message type to DISCONNECT
            break;
        default:
            key = 'x'; 
            break;
        }
        if(ctrl_c_flag==1){
            m_type=2;
            score_tmp = user_server_message(requester,*m,m_type);
        }else if (key != 'x'){
            score_tmp = user_server_message(requester,*m,m_type);
        }
        /* Print it on screen */
        wrefresh(*title_win);
        if(score_tmp==INT_MIN){
            break;
        }
        else{
            *score = score_tmp;
        }
        pthread_mutex_unlock(&mutex_print);
    }while(key != 81 && key != 113 && ctrl_c_flag!=1);

    endwin();
    if(score_tmp==INT_MIN){
        printf("User timeout error!\n");
    }
}

// Structure used to store lizard char and score for ordering
typedef struct CharScorePair{
        int ch;
        int score;
}CharScorePair;
int compareLizard_ch(const void *a, const void *b) {
    return ((struct CharScorePair *)a)->ch - ((struct CharScorePair *)b)->ch;
}

void printDisplay( display_data display[][WINDOW_SIZE],WINDOW** game_win,WINDOW** score_win,int n_lizard){
    
    CharScorePair lizard_arr[n_lizard];

    werase(*game_win);
    werase(*score_win);
    box(*game_win, 0 , 0);
    box(*score_win, 0 , 0);
    mvwprintw(*score_win, 1, 1, "Score:");

    int id=0;
    // Refresh game window
    for (size_t i = 1; i < WINDOW_SIZE-1; i++)
    {
        for (size_t j = 1; j < WINDOW_SIZE-1; j++)
        {
            wmove(*game_win, i, j);
            if(display[i][j].ch!=' '){
                switch (display[i][j].direction)
                {
                case UP:
                    waddch(*game_win,display[i][j].ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (i+cnt<WINDOW_SIZE-1) {
                            if(display[i+cnt][j].ch==' '){
                                wmove(*game_win, i+cnt, j);
                                if(display[i][j].score<0){
                                    break;
                                }
                                else if (display[i][j].score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }  
                            }                         
                        }else{
                            break;
                        }
                    } 
                    lizard_arr[id].ch = display[i][j].ch;lizard_arr[id].score = display[i][j].score;
                    id++;               
                    break;
                case DOWN:
                    waddch(*game_win,display[i][j].ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (i-cnt>0) {
                            if(display[i-cnt][j].ch==' '){
                                wmove(*game_win, i-cnt, j);
                                if(display[i][j].score<0){
                                    break;
                                }
                                else if (display[i][j].score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }    
                            }                         
                        }else{
                            break;
                        }
                    }
                    lizard_arr[id].ch = display[i][j].ch;lizard_arr[id].score = display[i][j].score;
                    id++;  
                    break;
                case LEFT:
                    waddch(*game_win,display[i][j].ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (j+cnt<WINDOW_SIZE-1) {
                            if(display[i][j+cnt].ch==' '){
                                wmove(*game_win, i, j+cnt);
                                if(display[i][j].score<0){
                                    break;
                                }
                                else if (display[i][j].score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }    
                            }                         
                        }else{
                            break;
                        }
                    }
                    lizard_arr[id].ch = display[i][j].ch;lizard_arr[id].score = display[i][j].score;
                    id++;
                    break;
                case RIGHT:
                    waddch(*game_win,display[i][j].ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (j-cnt>0) {
                            if(display[i][j-cnt].ch==' '){
                                wmove(*game_win, i, j-cnt);
                                if(display[i][j].score<0){
                                    break;
                                }
                                else if (display[i][j].score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }    
                            }                         
                        }else{
                            break;
                        }
                    }
                    lizard_arr[id].ch = display[i][j].ch;lizard_arr[id].score = display[i][j].score;
                    id++;  
                    break;
                case -1:
                    waddch(*game_win,display[i][j].ch| A_BOLD);
                    lizard_arr[id].ch = display[i][j].ch;lizard_arr[id].score = display[i][j].score;
                    id++;  
                    break;
                case -2:
                    wmove(*game_win, i, j);
                    waddch(*game_win,(display[i][j].score+'0'));
                    break;
                case -3:
                    wmove(*game_win, i, j);
                    waddch(*game_win,'#');
                    break;
                default:
                    break;
                }
                
            }            
        }
        
    }
    // Print scores ordered either by _ch or by _score
    qsort(lizard_arr, n_lizard, sizeof(CharScorePair), compareLizard_ch);
    int x=2,y=1;
    for (size_t i = 0; i < n_lizard; i++)
    {
        mvwprintw(*score_win, x, y, "%c: %d",lizard_arr[i].ch,lizard_arr[i].score); 
        x++;
    }

    wrefresh(*game_win);
    wrefresh(*score_win);
}

/*
Input: received new_data of display_data type and current grid information
Output: -
Based on the content of new_data will update grid[][] content
*/
void update_lizard(display_data new_data, display_data grid[][WINDOW_SIZE],int* n_lizards){
    display_data grid_null={.ch=' ',.direction=-1};
    if(new_data.pos_x0==0 && new_data.pos_y0==0){
        // New lizard connection
        if(grid[new_data.pos_x1][new_data.pos_y1].ch==new_data.ch
            && grid[new_data.pos_x1][new_data.pos_y1].token==new_data.token){
        }else{
            grid[new_data.pos_x1][new_data.pos_y1]=new_data;
            *n_lizards=*n_lizards+1;
        }
    }else{
        if(new_data.pos_x1==0 && new_data.pos_y1==0){
            // Lizard disconnect
            grid[new_data.pos_x0][new_data.pos_y0]=grid_null;
            *n_lizards=*n_lizards-1;
        }else if(grid[new_data.pos_x0][new_data.pos_y0].ch==new_data.ch && 
                grid[new_data.pos_x0][new_data.pos_y0].token == new_data.token){
            // Lizard movement
            grid[new_data.pos_x0][new_data.pos_y0]=grid_null;
            grid[new_data.pos_x1][new_data.pos_y1]=new_data;
        }
    }
}

/*
Input: received new_data of display_data type and current grid information
Output: -
Based on the content of new_data will update grid[][] content
*/
void update_bot(display_data new_data, display_data grid[][WINDOW_SIZE]){
    display_data grid_null={.ch=' ',.direction=-1};
    if(new_data.pos_x0==0 && new_data.pos_y0==0){
        // Bot connect
        grid[new_data.pos_x1][new_data.pos_y1]=new_data;
    }else if(grid[new_data.pos_x0][new_data.pos_y0].ch==new_data.ch && 
                grid[new_data.pos_x0][new_data.pos_y0].token == new_data.token){
            // Non-hidden bot movement
            grid[new_data.pos_x0][new_data.pos_y0]=grid_null;
            grid[new_data.pos_x1][new_data.pos_y1]=new_data;
        }else if(grid[new_data.pos_x0][new_data.pos_y0].ch!=new_data.ch){
            // Hiden bot movement
            grid[new_data.pos_x1][new_data.pos_y1]=new_data;
        }
}

