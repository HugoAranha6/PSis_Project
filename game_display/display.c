#include "zhelpers.h"
#include "display-helper.h"

int main (int argc, char *argv [])
{
    // Display data 2D array corresponding to a DISPLAY
    display_data grid[WINDOW_SIZE][WINDOW_SIZE]={[0 ... WINDOW_SIZE-1] = {[0 ... WINDOW_SIZE-1] = {.ch=' '}}};

    void* subscriber; //subscriber socket

    // Initialize interfaces
    WINDOW *title_win, *game_win, *score_win;
	display_interface(&title_win,&game_win,&score_win);

    // Connect to server and pull current information
    int n_lizards = display_initialize(&subscriber,grid,&game_win,&score_win,argc,argv);
    
    // Subscription mode information reception
    while(1){
        char *type = s_recv(subscriber);
        char id[100]="\0";
        zmq_recv (subscriber, &id, sizeof(id), 0);
        if(strcmp(id,"lizard")==0){
            // New lizard information published
            display_data new_data;
            zmq_recv (subscriber, &new_data, sizeof(new_data), 0);
            update_lizard(new_data,grid,&n_lizards);
            
        }else if(strcmp(id,"bot")==0){
            // New bot information published
            display_data new_data;
            zmq_recv (subscriber, &new_data, sizeof(new_data), 0);
            update_bot(new_data,grid);

        }else if(strcmp(id,"update")==0){
            // Published order to update display
            printDisplay(grid,&game_win,&score_win,n_lizards);
        }
    }
    endwin();
    zmq_close (subscriber);
    return 0;
}