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
#include <limits.h>

#define BOARD_SIZE 30
#define WINDOW_SIZE (BOARD_SIZE+2)
#define MAX_USERS 26
#define MAX_ROACHES (WINDOW_SIZE-2)*(WINDOW_SIZE-2)/3
#define MAX_BOT 10
#define BODY_SIZE 5
#define TIMEOUT 60

#define user_server_com "tcp://127.0.0.1:5555"
#define server_user_com "tcp://*:5555"
#define display_server_com "tcp://127.0.0.1:5556"
#define server_display_com "tcp://*:5556"
#define server_bot_com "tcp://*:5557"

// Variable to store direction movement, 0-3 UP,DOWN,LEFT,RIGHT, 4 no move
// -1 connected user, -2 roach
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

typedef enum msg_type {CONNECT, MOVE, DISCONNECT} msg_type;

// User to Server messages for movements
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

// Structure to store data on the server, both users and bots
typedef struct client_info{
    int ch;
    int pos_x, pos_y;
    int score;
    int token;
    direction_t direction; //This variable will take up,down,left,right, -1(connect user), -2(roach), -3(wasp)
    int visible;
    int timeout;
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


pthread_rwlock_t rwlock_grid = PTHREAD_RWLOCK_INITIALIZER;


/*
NOTE: Communication with client and display procedure.
      Receive message type id: "client", "roaches" or "display"
      Client type:
        Receive message for user clients
        1. Connection type:
            Repply a char that identifies the user
            Repply a token that identifies the user
            Publish new user information
        2. Movement type:
            Publish moved lizard
            Publish hit lizard (ONLY in collision case)
            Repply current score
        3. Disconnect
            Repply final score
            Publish lizard removal
      Bot type:
        Receive message from bots
        1. Connection type:
            Repply number of bots (case connection is possible)
            Publish new bot information (bot by bot)
            Repply  with a bot message with IDs and tokens
        2. Movement type:
            Publish moved bot
            Publish hidden bot (if exists)
            Repply check up message
      Display type:
        Repply token to be used in SUB mode
        Repply all current grid information (one-by-one)
        Repply "update" to signal end of this cycle

      After all action
        Publish "UPDATE" message to update remote displays            
*/


/*-------------------------------------------------------------
Input: x,y: coordinates of future position,
       direction: movement direction
Output: -
Calculate new position based on the direction. If the new position
touches the borders the position will not change.
-------------------------------------------------------------*/
 void new_position(int* x, int *y, direction_t direction){
    switch (direction)
    {
    case UP:
        (*x) --;
        if(*x ==0)
            *x = 1;
        break;
    case DOWN:
        (*x) ++;
        if(*x ==WINDOW_SIZE-1)
            *x = WINDOW_SIZE-2;
        break;
    case LEFT:
        (*y) --;
        if(*y ==0)
            *y = 1;
        break;
    case RIGHT:
        (*y) ++;
        if(*y ==WINDOW_SIZE-1)
            *y = WINDOW_SIZE-2;
        break;
    default:
        break;
    }
}

/*-------------------------------------------------------------
Input: pointers to a REP and a PUB sockets, array of chars
       to the initialized according to alphabet, argc and argv
Output: integer, token that will be used in the subscription mode
Initialize server. Will start the connections to the sockets for REP-REQ
and PUB-SUB and initializes the usar_char such that the characters are
set from a-z (lizards)
-------------------------------------------------------------*/
int server_initialize (void* context,void **responder_liz,void** responder_bot,
                         void **publisher,void **backend, char user_char[]){
    
    // Client - Server
    *responder_liz = zmq_socket (context, ZMQ_ROUTER);
    int rc = zmq_bind (*responder_liz, server_user_com);
    assert(rc==0);

    // Bot - Server
    *responder_bot = zmq_socket (context, ZMQ_REP);
    rc = zmq_bind (*responder_bot, server_bot_com);
    assert(rc==0);

    // Server - Display
    *publisher = zmq_socket (context, ZMQ_PUB);
    rc = zmq_bind (*publisher, server_display_com);
    assert(rc==0);

    *backend = zmq_socket (context, ZMQ_DEALER);
    rc = zmq_bind (*backend, "inproc://back-end");
    for (size_t i = 0; i < MAX_USERS; i++)
    {
        user_char[i]='a'+i;
    }
    
    srand((unsigned int)time(NULL));
    int token=rand();
    return token;
};

/*-------------------------------------------------------------
Input: pointers to three windows, title, game board and score windows
Output: - 
Initializes server interface windows. title_win is a window to display a
title, my_win will display the game board, and score_win will display
the lizard scores 
-------------------------------------------------------------*/
void server_interface(WINDOW **title_win, WINDOW **my_win, WINDOW** score_win){
    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();	
    curs_set(0);
    refresh();		    

    /* creates a window and draws a border */
    *title_win = newwin(1, 30, 1, 1);
    mvwprintw(*title_win, 0, 0, "Server display");
    wrefresh(*title_win);


    *my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 2, 1);
    box(*my_win, 0 , 0);	
	wrefresh(*my_win);

    *score_win = newwin(30, 10, 2, WINDOW_SIZE+5);
    box(*score_win, 0 , 0);
    mvwprintw(*score_win, 1, 1, "Score:");	
	wrefresh(*score_win);
}



/*-------------------------------------------------------------
Input: arr client_info contains information of lizard clients,
       size is the number of elements in arr[], index to remove
       from arr[], current grid information and available user id
Output: client_info array that stores connected lizards
Function to disconnect a client. Takes the index of the user to remove,
the arr[] will be reduced in size and the entry indexToRemove will be
removed, wherevere the arr[indexToRemove] lizard was in grid will be removed
and id will be djusted such that the char of lizard removed becomes available
-------------------------------------------------------------*/
client_info* removeClient(client_info arr[], int *size, int indexToRemove,client_info* grid[][WINDOW_SIZE], char id[]) {
    client_info* lizard_update = (client_info*)malloc((*size-1)* sizeof(client_info));
    // Case of bugs, index outside boundaries
    if (indexToRemove < 0 || indexToRemove >= *size) {
        free(lizard_update);
        return arr;
    }
    client_info tmp;
    tmp = arr[indexToRemove];
    // Shift elements one position backward starting from the specified index
    int j=0;
    for (int i = 0; i < *size; ++i) {
        if(i==indexToRemove){
        }else{
            lizard_update[j] = arr[i];
            grid[lizard_update[j].pos_x][lizard_update[j].pos_y]=&lizard_update[j];
            j++;
        }
    }
    grid[tmp.pos_x][tmp.pos_y]=NULL;

    // Adjust the vector of available characters for lizards head
    for (size_t i = indexToRemove; i < *size-1; i++)
    {   
        id[i]=id[i+1];
    }
    id[*size-1]=arr[indexToRemove].ch;

    // Decrease the size of the array
    (*size)--;
    free(arr);
    return lizard_update;
    
}

/*-------------------------------------------------------------
Input: array of connected clients information, n_char is the number
       of elements in the array, ch and token are the identifiers of
       the element to be found
Output: integer, position in the array of the element to be found
Utility function to find a client of bot within the lizard or bot data.
Goes through the array and identifies where the element whose
char and token are the ones given as inputs
-------------------------------------------------------------*/
int find_ch_info(client_info char_data[], int n_char, int ch, int token){

    for (int i = 0 ; i < n_char; i++){
        if(ch == char_data[i].ch && token == char_data[i].token){
            return i;
        }
    }
    return -1;
}


/*
Utility function to order lizards according to letters
*/
int compareLizard_ch(const void *a, const void *b) {
    return ((struct client_info *)a)->ch - ((struct client_info *)b)->ch;
}




/*-------------------------------------------------------------
Input: grid information vector, window pointers, board and score
       windows, array of all lizard data and number of lizards
Output: -
Function to update display in server. Takes a vector grid[][] that stores
information of the content within grid slot (x,y) and prints it
in the window game_win. Lizard data is then ordered alphabetically
to print the scores in score_win
-------------------------------------------------------------*/
void printGrid(client_info* grid[][WINDOW_SIZE],WINDOW** game_win, WINDOW** score_win, 
                client_info lizard_data[], int n_lizards){
    
    client_info tmp[n_lizards];
    memcpy(tmp, lizard_data, n_lizards*sizeof(client_info));

    // Clear window
    werase(*game_win);
    box(*game_win, 0 , 0);
    werase(*score_win);
    box(*score_win, 0 , 0);
    mvwprintw(*score_win, 1, 1, "Score:");
    
    // Refresh window
    for (size_t i = 1; i < WINDOW_SIZE-1; i++)
    {
        for (size_t j = 1; j < WINDOW_SIZE-1; j++)
        {
            wmove(*game_win, i, j);
            if(grid[i][j]!=NULL){
                switch (grid[i][j]->direction)
                {
                case UP:
                    waddch(*game_win,grid[i][j]->ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (i+cnt<WINDOW_SIZE-1) {
                            if(grid[i+cnt][j]==NULL){
                                wmove(*game_win, i+cnt, j);
                                if(grid[i][j]->score<0){
                                    break;
                                }else if (grid[i][j]->score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }  
                            }                         
                        }else{
                            break;
                        }
                    }                  
                    break;
                case DOWN:
                    waddch(*game_win,grid[i][j]->ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (i-cnt>0) {
                            if(grid[i-cnt][j]==NULL){
                                wmove(*game_win, i-cnt, j);
                                if(grid[i][j]->score<0){
                                    break;
                                }else if (grid[i][j]->score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }   
                            }                         
                        }else{
                            break;
                        }
                    }  
                    break;
                case LEFT:
                    waddch(*game_win,grid[i][j]->ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (j+cnt<WINDOW_SIZE-1) {
                            if(grid[i][j+cnt]==NULL){
                                wmove(*game_win, i, j+cnt);
                                if(grid[i][j]->score<0){
                                    break;
                                }else if (grid[i][j]->score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }  
                            }                         
                        }else{
                            break;
                        }
                    } 
                    break;
                case RIGHT:
                    waddch(*game_win,grid[i][j]->ch| A_BOLD);
                    for (size_t cnt = 1; cnt <= BODY_SIZE; cnt++){
                        if (j-cnt>0) {
                            if(grid[i][j-cnt]==NULL){
                                wmove(*game_win, i, j-cnt);
                                if(grid[i][j]->score<0){
                                    break;
                                }else if (grid[i][j]->score<50){
                                    waddch(*game_win,'.'| A_BOLD);
                                }else{
                                    waddch(*game_win,'*'| A_BOLD);
                                }  
                            }                         
                        }else{
                            break;
                        }
                    }  
                    break;
                case -1:
                    waddch(*game_win,grid[i][j]->ch| A_BOLD);
                    break;
                case -2:
                    wmove(*game_win, i, j);
                    waddch(*game_win,(grid[i][j]->score+'0'));
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
    // Sort alphabetically lizards
    qsort(tmp, n_lizards, sizeof(client_info), compareLizard_ch);

    // Print scores
    int x=2,y=1;
    for (size_t i = 0; i < n_lizards; i++)
    {
        mvwprintw(*score_win, x, y, "%c: %d",tmp[i].ch,tmp[i].score); 
        x++;
    }
    wrefresh(*game_win);
    wrefresh(*score_win);
}


/*
Input: grid information vector, pointers to integers of the future
       position (x,y) and a pointer to the roach to move
Output: an array of size 2, of the position that becomes available
        in the grid if the roach moves
Function that checks if the given roach movement is possible or not
possible, if there is a lizard in new position, and returns the position
that just became free if there is a movement 
*/
int* checkGrid_roach(client_info* grid[][WINDOW_SIZE], int* x, int* y, client_info* client) {
    int* loc = (int*)malloc(2 * sizeof(int));
    if (loc == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(0);
    }
    loc[0]=0;
    loc[1]=0;

    if(grid[*x][*y] != NULL){
        // Case grid entry is a lizard
        if(grid[*x][*y]->direction!=-2){
        }else if(grid[*x][*y]->ch != (*client).ch){ //case is a (non-self) roach
            
            if(((*client).ch != grid[(*client).pos_x][(*client).pos_y]->ch)){ // case grid slot has other roach
                grid[*x][*y] = client;
                grid[*x][*y]->pos_x = *x;
                grid[*x][*y]->pos_y = *y;
            }else{
                grid[*x][*y] = grid[(*client).pos_x][(*client).pos_y];
                grid[(*client).pos_x][(*client).pos_y] = NULL;
                loc[0]=(*client).pos_x;
                loc[1]=(*client).pos_y;
                grid[*x][*y]->pos_x = *x;
                grid[*x][*y]->pos_y = *y;
            }
        }
    }else{
        if(grid[(*client).pos_x][(*client).pos_y]->ch!=(*client).ch){
            grid[*x][*y] = client;
            grid[*x][*y]->pos_x = *x;
            grid[*x][*y]->pos_y = *y;
        }
        else if(grid[(*client).pos_x][(*client).pos_y]->ch==(*client).ch){
            grid[*x][*y] = grid[(*client).pos_x][(*client).pos_y];
            grid[(*client).pos_x][(*client).pos_y] = NULL;
            loc[0]=(*client).pos_x;
            loc[1]=(*client).pos_y;
            grid[*x][*y]->pos_x = *x;
            grid[*x][*y]->pos_y = *y;
        }else{
            printf("1\n");
        }                
    }
    return loc;    
}

/*
Input: a pointer to an array lizard, current grid information, number of
       connected lizard, n_lizards, and the array of lizard characters
       available
Output: pointer to an array cointaining connected lizards information
Function will add a new lizard to the previous lizard array.
*/
client_info* user_connect(client_info* lizard, client_info* grid[][WINDOW_SIZE],int n_lizard, char user_char[]){

    client_info* lizard_update = (client_info*)malloc((n_lizard+1)* sizeof(client_info));
    int pos_x, pos_y;
    if(lizard_update==NULL){
        endwin();
        printf("Memory alocation failed\n");
        exit(0);
    }
    srand((unsigned int)time(NULL));

    // Add already existing lizards to the new array and adjust grid
    for (size_t i = 0; i < n_lizard; i++)
    {
        lizard_update[i]=lizard[i];
        grid[lizard_update[i].pos_x][lizard_update[i].pos_y]=&lizard_update[i];
    }
    

    // Randomize new lizard token
    lizard_update[n_lizard].token=rand();

    // Randomize user position
    do{
        pos_x = 1 + rand()%(WINDOW_SIZE-2);
        pos_y = 1 + rand()%(WINDOW_SIZE-2);
    }while(grid[pos_x][pos_y]!=NULL);
    
    // Store on server user data
    lizard_update[n_lizard].ch = user_char[n_lizard];
    lizard_update[n_lizard].pos_x = pos_x;
    lizard_update[n_lizard].pos_y = pos_y;
    lizard_update[n_lizard].score = 0;
    lizard_update[n_lizard].direction = -1;
    lizard_update[n_lizard].visible=time(NULL);

    grid[pos_x][pos_y]=&lizard_update[n_lizard];
    
    free(lizard);
    return lizard_update;
}

/*
Input: structure with data of a bot, initial position pos_x0 and pos_y0,
       publisher socket and a token for publishing setting
Output: -
Function will publish a message that contains information of a bot action
*/
void send_display_bot( client_info bot_data, int pos_x0, int pos_y0, void* socket){
    display_data bot;
    bot.ch=bot_data.ch;
    bot.pos_x0=pos_x0;
    bot.pos_y0=pos_y0;
    bot.pos_x1=bot_data.pos_x;
    bot.pos_y1=bot_data.pos_y;
    bot.direction=bot_data.direction;
    bot.token=bot_data.token;
    bot.score = bot_data.score;
    
    s_sendmore(socket, "bot");
    zmq_send(socket, &bot, sizeof(display_data), 0);
    
}

/*
Input: n_bots number of connected bots, pointer to array bot_data,
       new_bots number of new bots to connect, pointer to structure
       of bot type, current grid informaiton, publisher socket, token
       setting for publishing, and array invisible
       NOTE: visible vector refers to an array containing information
       if a roach was eaten
Output: pointer to an array cointaining connected bot information
Function handles the addition of new_bots to the game. 
*/
client_info* bot_connect(int n_bots, client_info* bot_data, int new_bots, BotConnect* m_connect,
                            client_info* grid[][WINDOW_SIZE], void* socket){

    client_info* bot_update = (client_info*)malloc((n_bots+new_bots)* sizeof(client_info));

    int pos_x,pos_y;

    srand((unsigned int)time(NULL));

    // Add previous information to the new bot information array
    for (size_t i = 0; i < n_bots; i++){
        bot_update[i]=bot_data[i];
        if(grid[bot_update[i].pos_x][bot_update[i].pos_y]!=NULL){
            if(grid[bot_update[i].pos_x][bot_update[i].pos_y]->ch==bot_update[i].ch){
                grid[bot_update[i].pos_x][bot_update[i].pos_y]=&bot_update[i];
            }
        }
    }
    
    // For each new bot, generate tokens ids and so on.
    for (size_t i = 0; i < new_bots; i++){
        // Bot score on connection is on roach_message id
        bot_update[n_bots+i].score=m_connect->score[i];

        // Generate id and token
        if (n_bots==0){
            bot_update[n_bots+i].ch = 5000+n_bots+i;
        }else{
            bot_update[n_bots+i].ch = bot_update[n_bots-1].ch+i;
        }
        
        bot_update[n_bots+i].token = rand();
        bot_update[n_bots+i].direction = -2;
        bot_update[n_bots+i].visible = 0;
        bot_update[n_bots+i].timeout = time(NULL);      

        // Generate positon
        do{
            pos_x = 1 + rand()%(WINDOW_SIZE-2);
            pos_y = 1 + rand()%(WINDOW_SIZE-2);
        } while (grid[pos_x][pos_y]!=NULL && grid[pos_x][pos_y]->direction!=-2);
        bot_update[n_bots+i].pos_x = pos_x;
        bot_update[n_bots+i].pos_y = pos_y;

        grid[pos_x][pos_y] = &bot_update[n_bots+i];
        send_display_bot(bot_update[n_bots+i],0,0,socket);
    }
    return bot_update;
}

/*
Intput: array roach_info with server bot information, number of conencted
        roache, n_roaches, pointer to the lizard information that has moved,
        visible vector to adjust when a roach is eaten.
Output: -
Function handles when a roach is eaten. It will go through the connected roaches vector
and check if there are roaches in the current lizard position, therefore eaten. visible
vector will be adjusted when a roach is eaten.
*/
void eat_roach(client_info roach_info[],int n_roaches,client_info* lizard){
    time_t curr_time=time(NULL);
    for (size_t i = 0; i < n_roaches; i++){
        // Eating condition, roach position same as the lizard
        if(roach_info[i].visible==0 && roach_info[i].pos_x==(*lizard).pos_x && roach_info[i].pos_y==(*lizard).pos_y){
            (*lizard).score=(*lizard).score+roach_info[i].score;
            // Roach position is set to (0,0) as a virtual placeholder
            roach_info[i].pos_x=0;
            roach_info[i].pos_y=0;
            roach_info[i].visible=curr_time;
        }
    }
}

/*
Input: array visible containing information of a roach being eaten or not,
       number of bots connected, n_bots, array with current bot_data,
       current grid_information, publisher socket and token publishing setting
Output: -
Function to reassign invisible bots. 5s period is done by comparing current
time of operation with the time inside the visible array (0 indicates that
the bot was nto eaten so nothing to do). grid variable is used to place the
bot in the grid if there is a need to reassign it if the 5s have passed.
Information is then published according to protocols defined.
*/
void bot_reconnect(int n_bots,client_info bot_data[],client_info* grid[][WINDOW_SIZE],void* publisher){    
    time_t current_time=time(NULL);
    for (size_t i = 0; i < n_bots; i++)
    {
        if(bot_data[i].visible!=0){
            // Check if 5s have passed
            if(current_time - bot_data[i].visible>=5){
                bot_data[i].visible=0;
                int pos_x,pos_y;
                srand((unsigned int)time(NULL));
                // Generate random position
                do{
                    pos_x = 1 + rand()%(WINDOW_SIZE-2);
                    pos_y = 1 + rand()%(WINDOW_SIZE-2); 
                }while(grid[pos_x][pos_y]!=NULL && grid[pos_x][pos_y]->direction!=-2);
                bot_data[i].pos_x=pos_x;
                bot_data[i].pos_y=pos_y;

                // Assign to grid
                grid[pos_x][pos_y]=&bot_data[i];

                // Publish information
                send_display_bot(bot_data[i],bot_data[i].pos_x,bot_data[i].pos_y,publisher);
            }
        }
    }
}



/*
Input: structure of client information lizard_data, publisher
       socket, two integers x0 and y0, an integer type
       (type = 1 -> disconnect, type=0 movement or score change),
       token for publishing settings
Output:-
Function handles the publishment of a lizard action. If type is set
to 1, lizard was disconnected and the final position is (0,0)
and current position comes in lizard data.
If type is 0, then x0 and y0 are the initial positions of the lizard and
future positons comes in lizard_data.
*/
void send_display_user(client_info lizard_data, void* publisher, int x0, int y0,int type){
    
    display_data lizard_update={.ch=lizard_data.ch , .score=lizard_data.score,
                    .pos_x0=x0, .pos_y0=y0, .pos_x1=lizard_data.pos_x,
                    .pos_y1=lizard_data.pos_y, .direction=lizard_data.direction,
                    .token=lizard_data.token};

    // Type 1 case, change future position to (0,0) and current to the 
    // one in lizard_data
    if(type==1){
        lizard_update.pos_x0=lizard_data.pos_x;
        lizard_update.pos_y0=lizard_data.pos_y;
        lizard_update.pos_x1=0;
        lizard_update.pos_y1=0;
    }
    // Publishing
    //char mess[100];
    //sprintf(mess, "%d", token);
    //s_sendmore(publisher,mess);
    s_sendmore(publisher,"lizard");
    zmq_send(publisher, &lizard_update, sizeof(lizard_update), 0);
}

/*
Input: current grid information, future possible position x and y,
       structure of the moving entity client, publisher socket and
       token publishing setting
Output: integer that identifies whether a roach was eaten
        0 - no eat, 1 - eat
Function handes the movement of a lizard. Checks grid[][] on the future
position to make a set of comparison to eavluate whether it just moves or
if there is a collision with a lizard of roach.
*/
int checkGrid_lizard(client_info* grid[][WINDOW_SIZE], int* x, int* y, client_info client,void* publisher,int token) {
    int check_roach = 0;
    if(grid[*x][*y] != NULL){
        // Case grid entry is a (non-self) lizard
        if(grid[*x][*y]->direction!=-2 && grid[*x][*y]->direction!=-3){
            if(grid[*x][*y]->ch!=client.ch){
                int tmp= (grid[*x][*y]->score+grid[client.pos_x][client.pos_y]->score)/2;
                // Divide scores
                if(tmp!=grid[client.pos_x][client.pos_y]->score){
                    grid[*x][*y]->score = tmp;
                    grid[client.pos_x][client.pos_y]->score = tmp;
                    // If there was a collision publish information of the hit lizard
                    send_display_user(*(grid[*x][*y]),publisher,*x,*y,0);
                }
            }

            // Case the Lizard encounters a Wasp
        }else if(grid[*x][*y]->direction==-3){
             grid[client.pos_x][client.pos_y]->score= grid[client.pos_x][client.pos_y]->score-10;
              send_display_user(*(grid[client.pos_x][client.pos_y]),publisher,client.pos_x,client.pos_y,0);
        }else{
        // Case future grid entry is a Roach
            grid[*x][*y] = grid[client.pos_x][client.pos_y];
            grid[*x][*y]->pos_x=*x;
            grid[*x][*y]->pos_y=*y;
            grid[client.pos_x][client.pos_y]=NULL;
            check_roach = 1;
        }
    }else{
        // If next position is empty
        grid[*x][*y] = grid[client.pos_x][client.pos_y];
        grid[*x][*y]->pos_x = *x;
        grid[*x][*y]->pos_y = *y;
        grid[client.pos_x][client.pos_y] = NULL;
    }
    return check_roach;
}

/*
Input: puvlisher socket, token for publish setting
Output: -
Function publishes and "update" message
*/
void send_display_update(void* publisher, int token){
    //char mess[100];
    //sprintf(mess,"%d",token);
    /* s_sendmore(publisher,mess);*/
    s_send(publisher,"update");
}

/*
Input: socket responder, current grid information
Output:-
Function handles the burst of information of the current
game state when a new display connects.
*/
void sync_display(void* responder,client_info* grid[][WINDOW_SIZE]){
    display_data new_data;

    // Go through the grid vector and send information if there is 
    // a non NULL entry in grid
    for (size_t i = 0; i < WINDOW_SIZE-1; i++){
        for (size_t j = 0; j < WINDOW_SIZE-1; j++){
            if(grid[i][j]!=NULL){
                new_data.ch=grid[i][j]->ch;
                new_data.direction=grid[i][j]->direction;
                new_data.score=grid[i][j]->score;
                new_data.pos_x0=grid[i][j]->pos_x;
                new_data.pos_x1=grid[i][j]->pos_x;
                new_data.pos_y0=grid[i][j]->pos_y;
                new_data.pos_y1=grid[i][j]->pos_y;
                new_data.token=grid[i][j]->token;
                s_sendmore(responder,"data");
                zmq_send(responder,&new_data,sizeof(new_data),ZMQ_SNDMORE);
            }
        }  
    }
    s_send(responder,"update");    
}


/*
Input: number of lizard clients, lizard data, the grid, id and publisher
Output: returns the updated lizard data
Function handles the disconnection process of a lizard-client if there is inactivity for more than the TIMEOUT period .
*/
client_info* user_timeout(int* n_clients,client_info* lizard_data, client_info* grid[][WINDOW_SIZE],char id[],void* pusher){
    int curr_time = time(NULL);
    int flag_upd = 0;
    for (size_t i = 0; i < *n_clients; i++){
        // If time of last movement was more than 0s ago, remove this user
        if(curr_time - lizard_data[i].visible>TIMEOUT){
            display_data new_data={.ch=lizard_data[i].ch,.pos_x0=lizard_data[i].pos_x, .pos_y0=lizard_data[i].pos_y,.pos_x1=0,.pos_y1=0};
            lizard_data = removeClient(lizard_data,n_clients,i,grid,id);
            i=i-1;
            
            s_send(pusher,"lizard");
            zmq_send(pusher,&new_data,sizeof(new_data),0);
            flag_upd = 1;
        }
    }
    if (flag_upd==1){
        s_send(pusher,"update");
    }
    return lizard_data;
   
}

/*
Input: socket responder
Output: Received message
Function receives and unpacks a roach connection message.
*/
BotConnect* bot_connect_proto(void* responder){
    BotConnect* m_connect;
    zmq_msg_t zmq_msg;
    zmq_msg_init(&zmq_msg);
    int n_bytes = zmq_recvmsg(responder,&zmq_msg,0);
    void* data = zmq_msg_data(&zmq_msg);
    m_connect = bot_connect__unpack(NULL,n_bytes,data);
    zmq_msg_close(&zmq_msg);
    return m_connect;
}

/*
Input: socket responder
Output: Received message
Function receives and unpacks a bot movement message.
*/
BotMovement* bot_movement_proto(void* responder){
    BotMovement* m_movement;
    zmq_msg_t zmq_msg;
    zmq_msg_init(&zmq_msg);
    int n_bytes = zmq_recvmsg(responder,&zmq_msg,0);
    void* data = zmq_msg_data(&zmq_msg);
    m_movement = bot_movement__unpack(NULL,n_bytes,data);
    zmq_msg_close(&zmq_msg);
    return m_movement;
}

/*
Input: socket responder
Output: Received message
Function receives and unpacks a bot disconnect message.
*/
BotDisconnect* bot_disc_proto(void* responder){
    BotDisconnect* m_disc;
    zmq_msg_t zmq_msg;
    zmq_msg_init(&zmq_msg);
    int n_bytes = zmq_recvmsg(responder,&zmq_msg,0);
    void* data = zmq_msg_data(&zmq_msg);
    m_disc = bot_disconnect__unpack(NULL,n_bytes,data);
    zmq_msg_close(&zmq_msg);
    return m_disc;
}

/*
Input: Client information array, the size, the index to remove and the grid.
Output: Returns the roach list updated.
Function removes a roach from the roach list and updates the grid.
*/
client_info* removeRoach(client_info arr[], int *size, int indexToRemove,client_info* grid[][WINDOW_SIZE]){
    client_info* roach_update = (client_info*)malloc((*size-1)* sizeof(client_info));
    // Case of bugs, index outside boundaries
    if (indexToRemove < 0 || indexToRemove >= *size) {
        free(roach_update);
        return arr;
    }
    client_info tmp;
    tmp = arr[indexToRemove];
    // Shift elements one position backward starting from the specified index
    int j=0;
    for (int i = 0; i < *size; ++i) {
        if(i==indexToRemove){
        }else{
            roach_update[j] = arr[i];
            if(roach_update[j].visible==0 && grid[roach_update[j].pos_x][roach_update[j].pos_y]->ch==arr[i].ch && grid[arr[i].pos_x][arr[i].pos_y]->token==arr[i].token){
                grid[roach_update[j].pos_x][roach_update[j].pos_y]=&roach_update[j];
            }
            j++;
        }
    }
    grid[tmp.pos_x][tmp.pos_y]=NULL;

    // Decrease the size of the array
    (*size)--;
    free(arr);
    return roach_update;
}


/*
Input: number of bots, the bot data, numver if new bots, the grid and a socket.
Output: Returns the wasp list updated.
Function establishes the connection of a wasp, generating an id and a token. Then, a position is generated and the grid is updated
*/
client_info* wasp_connect(int n_bots, client_info* bot_data, int new_bots, client_info* grid[][WINDOW_SIZE], void* socket){

    client_info* wasp_update = (client_info*)malloc((n_bots+new_bots)* sizeof(client_info));

    int pos_x,pos_y;

    srand((unsigned int)time(NULL));

    // Add previous information to the new bot information array
    for (size_t i = 0; i < n_bots; i++){
        wasp_update[i]=bot_data[i];
        if(grid[wasp_update[i].pos_x][wasp_update[i].pos_y]!=NULL){
            if(grid[wasp_update[i].pos_x][wasp_update[i].pos_y]->ch==wasp_update[i].ch){
                grid[wasp_update[i].pos_x][wasp_update[i].pos_y]=&wasp_update[i];
            }
        }
    }
    
    // For each new bot, generate tokens ids and so on.
    for (size_t i = 0; i < new_bots; i++){
        // Generate id and token
        if (n_bots==0){
            wasp_update[n_bots+i].ch = 3000+n_bots+i;
        }else{
            wasp_update[n_bots+i].ch = wasp_update[n_bots-1].ch+i;
        }
        wasp_update[n_bots+i].token = rand();
        wasp_update[n_bots+i].direction = -3;
        wasp_update[n_bots+i].visible = 0;
        wasp_update[n_bots+i].timeout = time(NULL);
        // Generate positon
        do{
            pos_x = 1 + rand()%(WINDOW_SIZE-2);
            pos_y = 1 + rand()%(WINDOW_SIZE-2);
        } while (grid[pos_x][pos_y]!=NULL);
        wasp_update[n_bots+i].pos_x = pos_x;
        wasp_update[n_bots+i].pos_y = pos_y;

        grid[pos_x][pos_y] = &wasp_update[n_bots+i];
        send_display_bot(wasp_update[n_bots+i],0,0,socket);
    }
    return wasp_update;
}

/*
Input: grid information vector, pointers to integers of the future
       position (x,y) and a pointer to the wasp to move
Output: there is no output
Function that checks if the given wasp movement is possible or not
possible, if there is a lizard in new position
*/
void checkGrid_wasp(client_info* grid[][WINDOW_SIZE], int* x, int* y, client_info client, void* publisher) {


    if(grid[*x][*y] != NULL){
        // Case grid entry is a wasp
        if(grid[*x][*y]->direction ==-3){// next position wasp
        }else if(grid[*x][*y]->direction ==-2){// next position is a roach
        }else{ // next position lizard
            grid[*x][*y]->score = grid[*x][*y]->score -10;
            send_display_user(*(grid[*x][*y]),publisher,*x,*y,0);
        }

    }else{
        grid[*x][*y] = grid[client.pos_x][client.pos_y];
        grid[*x][*y]->pos_x=*x;
        grid[*x][*y]->pos_y=*y;
        grid[client.pos_x][client.pos_y]=NULL;

    }
}

client_info* wasp_timeout(int* n_clients,client_info* wasp_data, client_info* grid[][WINDOW_SIZE],void* pusher){
    int curr_time = time(NULL);
    int flag_upd = 0;
    for (size_t i = 0; i < *n_clients; i++){
        // If time of last movement was more than 0s ago, remove this user
        if(curr_time - wasp_data[i].timeout>TIMEOUT){
            send_display_bot(wasp_data[i],WINDOW_SIZE,WINDOW_SIZE,pusher);
            wasp_data = removeRoach(wasp_data,n_clients,i,grid);
            i=i-1;           
            flag_upd = 1;
        }
    }
    if (flag_upd==1){
        s_send(pusher,"update");
    }
    return wasp_data;
   
}

client_info* roach_time(int* n_clients,client_info* bot_data, client_info* grid[][WINDOW_SIZE],void* pusher){
    int curr_time = time(NULL);
    int flag_upd = 0;
    for (size_t i = 0; i < *n_clients; i++){
        // If time of last movement was more than 0s ago, remove this user
        if(curr_time - bot_data[i].timeout>TIMEOUT){
            send_display_bot(bot_data[i],WINDOW_SIZE,WINDOW_SIZE,pusher);
            bot_data = removeRoach(bot_data,n_clients,i,grid);
            i=i-1;           
            flag_upd = 1;
        }else if (bot_data[i].visible!=0 && curr_time - bot_data[i].visible>=5){
            bot_data[i].visible=0;
            int pos_x,pos_y;
            srand((unsigned int)time(NULL));
            // Generate random position
            do{
                pos_x = 1 + rand()%(WINDOW_SIZE-2);
                pos_y = 1 + rand()%(WINDOW_SIZE-2); 
            }while(grid[pos_x][pos_y]!=NULL && grid[pos_x][pos_y]->direction!=-2);
            bot_data[i].pos_x=pos_x;
            bot_data[i].pos_y=pos_y;

            // Assign to grid
            grid[pos_x][pos_y]=&bot_data[i];

            // Publish information
            send_display_bot(bot_data[i],bot_data[i].pos_x,bot_data[i].pos_y,pusher);
            flag_upd = 1;
        }
    }
    if (flag_upd==1){
        s_send(pusher,"update");
    }
    return bot_data;
   
}