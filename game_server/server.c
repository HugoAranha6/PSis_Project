#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <zmq.h>
#include "server-helper.h"


// Variable to store grid data, grid composed by pointers, pointing to client_info structures	
client_info* grid[WINDOW_SIZE][WINDOW_SIZE]={[0 ... WINDOW_SIZE-1] = {[0 ... WINDOW_SIZE-1] = NULL}};

client_info* lizard_data; // Varibale to store lizard data
    
client_info* bot_data; // Variable to store roaches data

char user_char[MAX_USERS]; // Variable to store lizard char, a...z

int n_clients = 0; // Store number of clients
int n_bots = 0;    // Store number of bots

// Initialize server sockets for communication and lizard data
void *responder_lizard, *publisher, *context;

// Initialize and print user interface: game board, score and title
WINDOW *title_win, *game_win, *score_win;

void* time_thread(void* arg){
    void *pusher = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(pusher, "inproc://display_sink");

    while(1){
        // Reassign invisible bots that were eaten
        user_timeout(&n_clients,lizard_data,grid,user_char,pusher);
    }
    return NULL;
}

void* display_thread(void* arg){
    void *sock_recv = zmq_socket(context, ZMQ_PULL);

    // Bind to the inproc address
    zmq_bind(sock_recv, "inproc://display_sink");

    int token = *((int *)arg);
    char token_char[100];
    sprintf(token_char, "%d", token);
    
    while(1){
        char id[100]="\0";
        zmq_recv (sock_recv, &id, sizeof(id), 0); 
        if(strcmp(id,"update")==0){
            s_sendmore(publisher,token_char);
            s_send(publisher, id);
            // Update server display
            printGrid(grid,&game_win,&score_win,lizard_data,n_clients);
        }else{
            display_data new_data;
            zmq_recv(sock_recv, &new_data, sizeof(new_data), 0);
            s_sendmore(publisher,token_char);
            s_sendmore(publisher, id);
            zmq_send(publisher, &new_data, sizeof(display_data), 0);
        }
        
    }
    return NULL;
}

void* lizard_thread(void* arg){
    int token = *((int *)arg);

    void *pusher = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(pusher, "inproc://display_sink");

    void *responder = zmq_socket (context, ZMQ_REP);
    zmq_connect(responder, "inproc://back-end");

    while (1){
        int ch=0, pos_x=0, pos_y=0;
        int ch_pos;

        // Receive message identifier: Client, Bot or Display
        char id[10]="\0";
        zmq_recv (responder, &id,sizeof(id),0);
        // Client message
        if(strcmp(id,"client")==0){
            msg_type m_type;
            zmq_recv(responder,&m_type,sizeof(m_type),0);
            
            switch (m_type){
            case CONNECT: 
                // Lizard connection message
                if (n_clients<MAX_USERS){
                    // Add new lizard to client list and grid if possible
                    lizard_data = user_connect(lizard_data,grid,n_clients,user_char);

                    // Send char and token to user
                    zmq_send(responder, &lizard_data[n_clients].ch, sizeof(ch), ZMQ_SNDMORE);
                    zmq_send (responder,&(lizard_data[n_clients].token),sizeof(int),0);
                    
                    // Publish information for Displays
                    send_display_user(lizard_data[n_clients],pusher,0,0,0,0);
                    n_clients++;
                }else{
                    // Case number of lizards has capped
                    ch = -1;
                    zmq_send(responder, &ch, sizeof(ch), ZMQ_SNDMORE);
                }
                break;
            case MOVE: 
                client m_move = {.ch=0,.direction=0,.token=0};
                zmq_recv (responder, &m_move, sizeof(m_move), 0);
                // Lizard movement message
                // Find lizard in the stored data based on the char and token received
                ch_pos = find_ch_info(lizard_data, n_clients, m_move.ch,m_move.token);
                if(ch_pos != -1){
                    lizard_data[ch_pos].visible=time(NULL);
                    pos_x = lizard_data[ch_pos].pos_x;
                    pos_y = lizard_data[ch_pos].pos_y;
                    int pos_y0 = pos_y, pos_x0=pos_x;
                    lizard_data[ch_pos].direction=m_move.direction;
                    
                    // Calculate future position
                    new_position(&pos_x, &pos_y, lizard_data[ch_pos].direction);

                    //Checks grid for roaches or lizard in new position
                    if(pos_x!=lizard_data[ch_pos].pos_x || pos_y!=lizard_data[ch_pos].pos_y){
                        if(checkGrid_lizard(grid,&pos_x,&pos_y,lizard_data[ch_pos],pusher,0)==1){
                            eat_roach(bot_data,n_bots,&lizard_data[ch_pos]);    
                        }
                        // Publish to Displays
                        send_display_user(lizard_data[ch_pos],pusher,pos_x0,pos_y0,0,0);
                    }
                    // Send current score to client
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                }else{
                    int time_o=INT_MIN;
                    zmq_send(responder,&time_o,sizeof(time_o),0);
                }
                break;
            case DISCONNECT:
                client_disconnect m_disc;
                zmq_recv (responder, &m_disc, sizeof(m_disc), 0); 
                // Lizard disconnect message
                // Find lizard in the stored data based on the char and token received
                ch_pos = find_ch_info(lizard_data, n_clients, m_disc.ch,m_disc.token);
                if(ch_pos!=-1){
                    // Send final score
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                    // Publish to displays
                    send_display_user(lizard_data[ch_pos],pusher,0,0,1,0);
                    // Remove lizard from the server data and grid
                    lizard_data = removeClient(lizard_data,&n_clients,ch_pos,grid,user_char);
                }
                break;
            default:
                break;
            }
            send_display_update(pusher,0);
        }
        // Display message
        if(strcmp(id,"display")==0){
            // Send the SUB mode token
            zmq_send(responder, &token, sizeof(token),ZMQ_SNDMORE);
            // Send the current game situation
            sync_display(responder,grid);
        }
        
    }
    
}

void* bot_thread(void* arg){

    
}




int main()
{
    
    //int* visible=(int*)malloc(0*sizeof(int)); // Variable to store bot visibility, 0=visible
    context = zmq_ctx_new();
    
    
    int token = server_initialize(context,&responder_lizard,&publisher,user_char);
    void *backend = zmq_socket (context, ZMQ_DEALER);
    zmq_bind (backend, "inproc://back-end");
    
	server_interface(&title_win,&game_win,&score_win);



    pthread_t thread_id[2];
    pthread_create(&thread_id[0],NULL,time_thread,NULL);   
    pthread_create(&thread_id[1],NULL,display_thread,(void *)&token); 
    pthread_create(&thread_id[2],NULL,lizard_thread,(void *)&token);   
    pthread_create(&thread_id[3],NULL,lizard_thread,(void *)&token);

    zmq_proxy (responder_lizard, backend, NULL);

    while (1){
        int ch=0, pos_x=0, pos_y=0;
        int ch_pos; int new_bots;

        // Receive message identifier: Client, Bot or Display
        char id[10]="\0";
        zmq_recv (responder_lizard, &id,sizeof(id),0);

        // Reassign invisible bots that were eaten
        bot_reconnect(n_bots,bot_data,grid,publisher,token);

        /*
        // Client message
        if(strcmp(id,"client")==0){
            msg_type m_type;
            zmq_recv(responder_lizard,&m_type,sizeof(m_type),0);
            
            switch (m_type){
            case CONNECT: 
                // Lizard connection message
                if (n_clients<MAX_USERS){
                    // Add new lizard to client list and grid if possible
                    lizard_data = user_connect(lizard_data,grid,n_clients,user_char);

                    // Send char and token to user
                    zmq_send(responder, &lizard_data[n_clients].ch, sizeof(ch), ZMQ_SNDMORE);
                    zmq_send (responder,&(lizard_data[n_clients].token),sizeof(int),0);
                    
                    // Publish information for Displays
                    send_display_user(lizard_data[n_clients],publisher,0,0,0,token);
                    n_clients++;
                }else{
                    // Case number of lizards has capped
                    ch = -1;
                    zmq_send(responder, &ch, sizeof(ch), ZMQ_SNDMORE);
                }
                break;
            case MOVE: 
                client m_move = {.ch=0,.direction=0,.token=0};
                zmq_recv (responder_lizard, &m_move, sizeof(m_move), 0);
                // Lizard movement message
                // Find lizard in the stored data based on the char and token received
                ch_pos = find_ch_info(lizard_data, n_clients, m_move.ch,m_move.token);
                if(ch_pos != -1){
                    lizard_data[ch_pos].visible=time(NULL);
                    pos_x = lizard_data[ch_pos].pos_x;
                    pos_y = lizard_data[ch_pos].pos_y;
                    int pos_y0 = pos_y, pos_x0=pos_x;
                    lizard_data[ch_pos].direction=m_move.direction;
                    
                    // Calculate future position
                    new_position(&pos_x, &pos_y, lizard_data[ch_pos].direction);

                    //Checks grid for roaches or lizard in new position
                    if(pos_x!=lizard_data[ch_pos].pos_x || pos_y!=lizard_data[ch_pos].pos_y){
                        if(checkGrid_lizard(grid,&pos_x,&pos_y,lizard_data[ch_pos],publisher,token)==1){
                            eat_roach(bot_data,n_bots,&lizard_data[ch_pos]);    
                        }
                        // Publish to Displays
                        send_display_user(lizard_data[ch_pos],publisher,pos_x0,pos_y0,0,token);
                    }
                    // Send current score to client
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                }else{
                    int time_o=INT_MIN;
                    zmq_send(responder,&time_o,sizeof(time_o),0);
                }
                break;
            case DISCONNECT:
                client_disconnect m_disc;
                zmq_recv (responder, &m_disc, sizeof(m_disc), 0); 
                // Lizard disconnect message
                // Find lizard in the stored data based on the char and token received
                ch_pos = find_ch_info(lizard_data, n_clients, m_disc.ch,m_disc.token);
                if(ch_pos!=-1){
                    // Send final score
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                    // Publish to displays
                    send_display_user(lizard_data[ch_pos],publisher,0,0,1,token);
                    // Remove lizard from the server data and grid
                    lizard_data = removeClient(lizard_data,&n_clients,ch_pos,grid,user_char);
                }
                break;
            default:
                break;
            }
        }*/

        // Bot message
        if(strcmp(id,"roaches")==0){
            bot roach_message;
            zmq_recv (responder_lizard, &roach_message, sizeof(bot), 0);
            switch (roach_message.type)
            {
            case CONNECT:
                // Bot connect message
                zmq_recv(responder_lizard, &new_bots,sizeof(int),0);
                if(new_bots+n_bots<=MAX_ROACHES){
                    ch = new_bots;

                    // Send to client number of bots
                    zmq_send(responder_lizard,&(ch),sizeof(int),ZMQ_SNDMORE);

                    // Add #new_bots new bots to the data structure and grid
                    bot_data=bot_connect(n_bots,bot_data,new_bots,&roach_message,grid,publisher,token);
                    n_bots = n_bots+new_bots;

                    // Send to client a message with IDs and tokens
                    zmq_send(responder_lizard,&roach_message,sizeof(bot),0);
                }else{
                    // Case number of bots requested exceeds the maximum
                    ch=-1;
                    zmq_send(responder_lizard,&ch,sizeof(int),0);
                }
                break;
            case MOVE:
                // Bot movement message
                for (size_t i = 0; i < MAX_BOT && roach_message.id[i]!=0; i++){
                    // Find the bot to move in the data
                    ch_pos = find_ch_info(bot_data,n_bots,roach_message.id[i],roach_message.token[i]);

                    if(ch_pos!=-1 && bot_data[ch_pos].visible==0 && roach_message.direction[i]!=4){
                        pos_x = bot_data[ch_pos].pos_x;
                        pos_y = bot_data[ch_pos].pos_y;
                        int pos_x0=pos_x, pos_y0 = pos_y;

                        // Calculate new position
                        new_position(&pos_x, &pos_y, roach_message.direction[i]);
                        // Check grid availability for movement
                        int* check = checkGrid_roach(grid,&pos_x,&pos_y,&bot_data[ch_pos]);

                        // Publish information in SUB mode
                        send_display_bot(bot_data[ch_pos],pos_x0,pos_y0,publisher,token);

                        // Check for roaches that were possibly hidden underneath the moved one
                        if(check[0]!=0 && check[1]!=0){
                            for (size_t i = 0; i < n_bots; i++){
                                if(bot_data[i].pos_x==check[0] && bot_data[i].pos_y==check[1]){
                                    grid[check[0]][check[1]]=&bot_data[i];

                                    // Publish message if an hidden bot was found
                                    send_display_bot(bot_data[i],pos_x0,pos_y0,publisher,token);
                                    break;
                                }
                            } 
                        }
                    }
                }
                // Send to client a check up message
                int check = 1;
                zmq_send(responder_lizard,&check,sizeof(int),0);
                break;
            default:
                break;
            }
        }

        /*
        // Display message
        if(strcmp(id,"display")==0){
            // Send the SUB mode token
            zmq_send(responder_lizard, &token, sizeof(token),ZMQ_SNDMORE);
            // Send the current game situation
            sync_display(responder_lizard,grid);
        }
        */

        // Update server display
        printGrid(grid,&game_win,&score_win,lizard_data,n_clients);
        // Publish "UPDATE" message
        send_display_update(publisher,token);    
    }

  	endwin();			
    zmq_close (publisher);
    zmq_close (responder_lizard);
    free(lizard_data);
    free(bot_data);
	return 0;
}

