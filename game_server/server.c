#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <zmq.h>
#include "server-helper.h"



int main()
{
    // Variable to store grid data, grid composed by pointers, pointing to client_info structures	
    client_info* grid[WINDOW_SIZE][WINDOW_SIZE]={[0 ... WINDOW_SIZE-1] = {[0 ... WINDOW_SIZE-1] = NULL}};;
    
    client_info* lizard_data; // Varibale to store lizard data
    
    client_info* bot_data; // Variable to store roaches data

    int* visible=(int*)malloc(0*sizeof(int)); // Variable to store bot visibility, 0=visible
    char user_char[MAX_USERS];                // Variable to store lizard char, a...z

    int n_clients = 0; // Store number of clients
    int n_bots = 0;    // Store number of bots
    
    // Initialize server sockets for communication and lizard data
    void *responder, *publisher;
    int token = server_initialize(&responder,&publisher,user_char);
    // Initialize and print user interface: game board, score and title
    WINDOW *title_win, *game_win, *score_win;
	server_interface(&title_win,&game_win,&score_win);
        
    while (1){
        int ch=0, pos_x=0, pos_y=0;
        int ch_pos; int new_bots;

        // Receive message identifier: Client, Bot or Display
        char id[10]="\0";
        zmq_recv (responder, &id,sizeof(id),0);

        // Reassign invisible bots that were eaten
        bot_reconnect(visible,n_bots,bot_data,grid,publisher,token);

        // Client message
        if(strcmp(id,"client")==0){
            client m = {.ch=0,.direction=0,.token=0,.type=-1};
            zmq_recv (responder, &m, sizeof(m), 0);
            switch (m.type){
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
                // Lizard movement message
                // Find lizard in the stored data based on the char and token received
                ch_pos = find_ch_info(lizard_data, n_clients, m.ch,m.token);
                if(ch_pos != -1){
                    pos_x = lizard_data[ch_pos].pos_x;
                    pos_y = lizard_data[ch_pos].pos_y;
                    int pos_y0 = pos_y, pos_x0=pos_x;
                    lizard_data[ch_pos].direction=m.direction;
                    
                    // Calculate future position
                    new_position(&pos_x, &pos_y, lizard_data[ch_pos].direction);

                    //Checks grid for roaches or lizard in new position
                    if(pos_x!=lizard_data[ch_pos].pos_x || pos_y!=lizard_data[ch_pos].pos_y){
                        if(checkGrid_lizard(grid,&pos_x,&pos_y,lizard_data[ch_pos],publisher,token)==1){
                            eat_roach(bot_data,n_bots,&lizard_data[ch_pos],visible);    
                        }
                        // Publish to Displays
                        send_display_user(lizard_data[ch_pos],publisher,pos_x0,pos_y0,0,token);
                    }
                    // Send current score to client
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                } 
                break;
            case DISCONNECT: 
                // Lizard disconnect message
                // Find lizard in the stored data based on the char and token received
                ch_pos = find_ch_info(lizard_data, n_clients, m.ch,m.token);
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
        }

        // Bot message
        if(strcmp(id,"roaches")==0){
            bot roach_message;
            zmq_recv (responder, &roach_message, sizeof(bot), 0);
            switch (roach_message.type)
            {
            case CONNECT:
                // Bot connect message
                zmq_recv(responder, &new_bots,sizeof(int),0);
                if(new_bots+n_bots<=MAX_ROACHES){
                    ch = new_bots;

                    // Send to client number of bots
                    zmq_send(responder,&(ch),sizeof(int),ZMQ_SNDMORE);

                    // Add #new_bots new bots to the data structure and grid
                    bot_data=bot_connect(n_bots,bot_data,new_bots,&roach_message,grid,publisher,token,&visible);
                    n_bots = n_bots+new_bots;

                    // Send to client a message with IDs and tokens
                    zmq_send(responder,&roach_message,sizeof(bot),0);
                }else{
                    // Case number of bots requested exceeds the maximum
                    ch=-1;
                    zmq_send(responder,&ch,sizeof(int),0);
                }
                break;
            case MOVE:
                // Bot movement message
                for (size_t i = 0; i < MAX_BOT && roach_message.id[i]!=0; i++){
                    // Find the bot to move in the data
                    ch_pos = find_ch_info(bot_data,n_bots,roach_message.id[i],roach_message.token[i]);

                    if(ch_pos!=-1 && visible[ch_pos]==0 && roach_message.direction[i]!=4){
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
                zmq_send(responder,&check,sizeof(int),0);
                break;
            default:
                break;
            }
        }

        // Display message
        if(strcmp(id,"display")==0){
            // Send the SUB mode token
            zmq_send(responder, &token, sizeof(token),ZMQ_SNDMORE);
            // Send the current game situation
            sync_display(responder,grid);
        }

        // Update server display
        printGrid(grid,&game_win,&score_win,lizard_data,n_clients);
        // Publish "UPDATE" message
        send_display_update(publisher,token);    
    }

  	endwin();			
    zmq_close (publisher);
    zmq_close (responder);
    free(lizard_data);
    free(bot_data);
    free(visible);
	return 0;
}

