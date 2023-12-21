#include "lizard-helper.h"

int main(int argc, char *argv[])
{ 
    void *requester;
    client m;
    int score = 0;

    // Initialize and connect to server
    user_initialize(&requester,&m,argc,argv);

    // User controlling the lizard
    user_input(requester,&m,&score);

    // Display final user score
    printf("Your final score: %d\n",score);

    zmq_close(requester);
	return 0;
}