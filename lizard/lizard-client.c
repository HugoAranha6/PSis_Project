#include "lizard-helper.h"



int score = 0;
WINDOW *title_win, *score_win, *game_win,*game_score;
// Display data 2D array corresponding to a DISPLAY
display_data grid[WINDOW_SIZE][WINDOW_SIZE]={[0 ... WINDOW_SIZE-1] = {[0 ... WINDOW_SIZE-1] = {.ch=' '}}};
void* requester;
void* subscriber;
client m;
int n_lizards;


void* input_thread(void* arg){   
 
    // User controlling the lizard
    user_input(requester,&m,&score,&title_win,&score_win);

    zmq_close(requester);
    return NULL;
}

void* display_thread(void* arg){
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
            pthread_mutex_lock(&mutex_print);
            printDisplay(grid,&game_win,&score_win,n_lizards);
            pthread_mutex_unlock(&mutex_print);
        }
    }  
    return NULL;
}




int main(int argc, char *argv[])
{
    
    
    // Initialize and connect to server
    n_lizards = user_initialize(&requester,&subscriber,&m,argc,argv,grid);

    // Initialize interface
    user_interface(&title_win,&score_win,&game_win);
    printDisplay(grid,&game_win,&score_win,n_lizards);
    
        
    pthread_t thread_id[2];
    pthread_create(&thread_id[0], NULL, input_thread, NULL);
    pthread_create(&thread_id[1],NULL,display_thread,NULL);
    pthread_join(thread_id[0], NULL);
    pthread_cancel(thread_id[1]);
    endwin();
    // Display final user score
    printf("Your final score: %d\n",score);

    
	return 0;
}