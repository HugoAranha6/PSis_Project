#include "lizard-helper.h"
#include "pthread.h"



void* input_thread(void* arg){
    struct ThreadArgs *thread_args = (struct ThreadArgs *)arg;
    int argc = thread_args->argc;
    char** argv = thread_args->argv;
    
    client m;
    void *requester;

    // Initialize and connect to server
    user_initialize(&requester,&m,argc,argv);
    // User controlling the lizard
    user_input(requester,&m,&score);

    zmq_close(requester);
}

struct ThreadArgs {
    int argc;
    char **argv;
};
int score = 0;


int main(int argc, char *argv[])
{ 
    
    int score;
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