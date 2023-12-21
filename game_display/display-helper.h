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

#define BOARD_SIZE 30
#define WINDOW_SIZE (BOARD_SIZE+2)
#define BODY_SIZE 5


//#define user_server_com "tcp://127.0.0.1:5555"
//#define server_user_com "tcp://*:5555"
//#define display_server_com "tcp://127.0.0.1:5556"
//#define server_display_com "tcp://*:5556"


// Variable to store direction movement, 0-3 UP,DOWN,LEFT,RIGHT, 4 no move
// -1 connected user, -2 roach
typedef enum direction_t {
    UP=0,
    DOWN=1,
    LEFT=2,
    RIGHT=3,
    HOLD=4,
    s_lizard=-1,
    s_roach=-2
}direction_t;

// Structure to store data on the server, both users and bots
typedef struct client_info{
    int ch;
    int pos_x, pos_y;
    int score;
    int token;
    direction_t direction; //This variable will take up,down,left,right, -1(connect user), -2(roach)
} client_info; 


// Server to Display messages
typedef struct display_data{
    int ch;
    int pos_x0, pos_y0;     // Old position
    int pos_x1, pos_y1;     // New position
    int score;
    int token;
    direction_t direction;
}display_data;

// Structure used to store lizard char and score for ordering
typedef struct CharScorePair{
        int ch;
        int score;
}CharScorePair;

/*
NOTE: Communication with server procedure.
      Initial connection (REQ-REP mode):
        1. Send id of who is contacting: "display"
        2. Receive subscription token
        3. Receive current grid information
        4. Receive "update" signaling the end of this cycle
      Subscriber mode (PUB-SUB mode):
        . SUBSCRIBER mode with subscription option token
        1. Receive token 
        2. Receive incoming type: "lizard", "bot" or "update"
        3. "lizard" - receive data corresponding to a lizard
           "bot" - receive data corresponding to a bot
           "update" - receive nothing, just update the screen display
*/


/*
Inputs: Three pointers to window points: title, game board and score windows
Output: -
Function that initializes the remote-display interface
*/
void display_interface(WINDOW **title_win, WINDOW **my_win, WINDOW** score_win){
    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();	
    curs_set(0);
    refresh();		    

    /* creates a window and draws a border */
    *title_win = newwin(1, 30, 1, 1);
    mvwprintw(*title_win, 0, 0, "Remote display");
    wrefresh(*title_win);


    *my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 2, 1);
    box(*my_win, 0 , 0);	
	wrefresh(*my_win);

    *score_win = newwin(30, 10, 2, WINDOW_SIZE+5);
    box(*score_win, 0 , 0);
    mvwprintw(*score_win, 1, 1, "Score:");	
	wrefresh(*score_win);
    
}

/*
Function to use as compare settings in qsort().
Two option either order by lizard char or by lizard
scores
*/
int compareLizard_ch(const void *a, const void *b) {
    return ((struct CharScorePair *)a)->ch - ((struct CharScorePair *)b)->ch;
}
int compareLizard_score(const void *a, const void *b) {
    return ((struct CharScorePair *)b)->score - ((struct CharScorePair *)a)->score;
}

/*
Input: display information, windows to print game board and score
Output:-
Update the current visual display. Goes through display[][] and
prints the content.
*/
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
                                if (display[i][j].score<50){
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
                                if (display[i][j].score<50){
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
                                if (display[i][j].score<50){
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
                                if (display[i][j].score<50){
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
        grid[new_data.pos_x1][new_data.pos_y1]=new_data;
        *n_lizards=*n_lizards+1;
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

/*-------------------------------------------------------------
Input: pointer to a socket to be initializaed, current grid,
       and pointers to two windows, board and score windows
Output: returns the current number of connected lizards
Connects to the server in REQ mode to initialize grid and obtain
subscription token. Initialize SUB mode socket and set option to
token.
-------------------------------------------------------------*/
int display_initialize (void **subscriber, display_data grid[][WINDOW_SIZE],WINDOW** game_win, WINDOW** score_win,
                        int argc, char* argv[]){
    int token;
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    *subscriber = zmq_socket (context, ZMQ_SUB);

    if(argc!=3){
        endwin();
        printf("Wrong number of command line arguments, check instructions!\n");
        exit(0);
    }
    char user_server_com[50];
    strcpy(user_server_com, "tcp://");
    strcat(user_server_com, argv[1]);

    char display_server_com[50];
    strcpy(display_server_com, "tcp://");
    strcat(display_server_com, argv[2]);

    // Connect and obtain token
    zmq_connect (requester, user_server_com);
    s_send(requester,"display");
    zmq_recv(requester, &token, sizeof(token),0);

    // SUB socket and set option to token
    char token_char[100];
    sprintf(token_char,"%d",token);
    zmq_connect (*subscriber, display_server_com);
    zmq_setsockopt (*subscriber, ZMQ_SUBSCRIBE,token_char, strlen(token_char));
   
    // Initial information pull of current game state
    char id[100]="\0";
    int print=0;
    int n_lizards=0;
    do
    {            
        zmq_recv (requester, &id, sizeof(id), 0);
        if(strcmp(id,"data")==0){
            display_data new_data;
            zmq_recv (requester, &new_data, sizeof(new_data), 0);
            grid[new_data.pos_x0][new_data.pos_y0]=new_data;
            if (new_data.direction!=-2){
                n_lizards++;
            }
            
            print=1;
        }
    } while (strcmp(id,"update")!=0);
    if(print==1){
        printDisplay(grid,game_win,score_win,n_lizards);
    }
    zmq_close(requester);
    return n_lizards; 
}
