#include "lizard-helper.h"
#include "pthread.h"

struct ThreadArgs {
    int argc;
    char **argv;
};
int score = 0;
WINDOW *title_win, *score_win;

void* input_thread(void* arg){
    struct ThreadArgs *thread_args = (struct ThreadArgs *)arg;
    int argc = thread_args->argc;
    char** argv = thread_args->argv;
    
    client m;
    void *requester;

    // Initialize and connect to server
    user_initialize(&requester,&m,argc,argv);

    // Initialize interface
    user_interface(&title_win,&score_win);

    // User controlling the lizard
    user_input(requester,&m,&score,&title_win,&score_win);

    zmq_close(requester);
}




int main(int argc, char *argv[])
{
    pthread_t thread_id[2];
    struct ThreadArgs *args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
    // Populate the structure with argc and argv
    args->argc = argc;
    args->argv = argv;

    pthread_create(&thread_id[0], NULL, input_thread, (void *)args);

    
    pthread_join(thread_id[0], NULL);
    
    // Display final user score
    printf("Your final score: %d\n",score);

    
	return 0;
}