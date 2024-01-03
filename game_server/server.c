#include "server-helper.h"




// Variable to store grid data, grid composed by pointers, pointing to client_info structures	
client_info* grid[WINDOW_SIZE][WINDOW_SIZE]={[0 ... WINDOW_SIZE-1] = {[0 ... WINDOW_SIZE-1] = NULL}};

client_info* lizard_data; // Varibale to store lizard data
client_info* roaches_data;// Variable to store roaches data
client_info* wasp_data;   // Variable to store wasp data

char user_char[MAX_USERS]; // Variable to store lizard char, a...z

int n_clients = 0; // Store number of clients
int n_roaches = 0;    // Store number of bots
int n_wasps = 0;

// Initialize server sockets for communication and lizard data
void *responder_bot, *publisher, *context;

// User interface variable: title, game window and score window
WINDOW *title_win, *game_win, *score_win;

void* time_thread(void* arg){
    void *pusher = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(pusher, "inproc://display_sink");

    while(1){
        // Reassign invisible bots that were eaten
        pthread_rwlock_wrlock(&rwlock_grid);
        lizard_data = user_timeout(&n_clients,lizard_data,grid,user_char,pusher);
        pthread_rwlock_unlock(&rwlock_grid);
        pthread_rwlock_wrlock(&rwlock_grid);
        bot_reconnect(n_roaches,roaches_data,grid,pusher);
        pthread_rwlock_unlock(&rwlock_grid);
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
        pthread_mutex_lock(&mutex_display);
        pthread_cond_wait(&cond_pushes_recv,&mutex_display);
        pthread_rwlock_rdlock(&rwlock_grid);
        char id[100]="\0";
        s_sendmore(publisher,token_char);
        while(strcmp(id,"update")!=0){
            zmq_recv (sock_recv, &id, sizeof(id), 0);
            if(strcmp(id,"update")==0){
                s_send(publisher, id);
                // Update server display
                printGrid(grid,&game_win,&score_win,lizard_data,n_clients);
                pthread_mutex_unlock(&mutex_display);
                pthread_rwlock_unlock(&rwlock_grid);
            }else{
                display_data new_data;
                zmq_recv(sock_recv, &new_data, sizeof(new_data), 0);
                s_sendmore(publisher, id);
                zmq_send(publisher, &new_data, sizeof(display_data), ZMQ_SNDMORE);
            }
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
                pthread_rwlock_wrlock(&rwlock_grid);
                if (n_clients<MAX_USERS){
                    // Add new lizard to client list and grid if possible
                    lizard_data = user_connect(lizard_data,grid,n_clients,user_char);
                    n_clients++;
                    // PUSH information for Displays
                    send_display_user(lizard_data[n_clients-1],pusher,0,0,0);
                    
                    // Send char and token to user
                    zmq_send(responder, &lizard_data[n_clients-1].ch, sizeof(ch), ZMQ_SNDMORE);
                    zmq_send (responder,&(lizard_data[n_clients-1].token),sizeof(int),0);
                    
                }else{
                    // Case number of lizards has capped
                    ch = -1;
                    zmq_send(responder, &ch, sizeof(ch), ZMQ_SNDMORE);
                }
                pthread_rwlock_unlock(&rwlock_grid);
                break;
            case MOVE:{
                // client m_move = {.ch=0,.direction=0,.token=0};
                client m_move;
                m_move.ch = 0;
                m_move.direction = 0;
                m_move.token = 0;

                zmq_recv (responder, &m_move, sizeof(m_move), 0);
                // Lizard movement message
                // Find lizard in the stored data based on the char and token received
                pthread_rwlock_rdlock(&rwlock_grid);
                ch_pos = find_ch_info(lizard_data, n_clients, m_move.ch,m_move.token);
                pthread_rwlock_unlock(&rwlock_grid);
                if(ch_pos != -1){
                    pthread_rwlock_wrlock(&rwlock_grid);
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
                            eat_roach(roaches_data,n_roaches,&lizard_data[ch_pos]);    
                        }
                        // Publish to Displays
                        send_display_user(lizard_data[ch_pos],pusher,pos_x0,pos_y0,0);
                    }
                    pthread_rwlock_unlock(&rwlock_grid);
                    // Send current score to client
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                }else{
                    int time_o=INT_MIN;
                    zmq_send(responder,&time_o,sizeof(time_o),0);
                }
                break;}
            case DISCONNECT:{
                client_disconnect m_disc;
                zmq_recv (responder, &m_disc, sizeof(m_disc), 0); 
                // Lizard disconnect message
                // Find lizard in the stored data based on the char and token received
                pthread_rwlock_rdlock(&rwlock_grid);
                ch_pos = find_ch_info(lizard_data, n_clients, m_disc.ch,m_disc.token);
                pthread_rwlock_unlock(&rwlock_grid);
                if(ch_pos!=-1){
                    // Send final score
                    zmq_send (responder, &(lizard_data[ch_pos].score), sizeof(int), 0);
                    
                    // Remove lizard from the server data and grid
                    pthread_rwlock_wrlock(&rwlock_grid);
                    // PUSH to display thread
                    send_display_user(lizard_data[ch_pos],pusher,0,0,1);

                    lizard_data = removeClient(lizard_data,&n_clients,ch_pos,grid,user_char);
                    pthread_rwlock_unlock(&rwlock_grid);
                }else{
                    int time_o=INT_MIN;
                    zmq_send(responder,&time_o,sizeof(time_o),0);
                }
                break;}
            default:
                break;
            }
            send_display_update(pusher,0);
            pthread_cond_signal(&cond_pushes_recv);
        }
        // Display message
        if(strcmp(id,"display")==0){
            // Send the SUB mode token
            zmq_send(responder, &token, sizeof(token),ZMQ_SNDMORE);
            // Send the current game situation
            pthread_rwlock_rdlock(&rwlock_grid);
            sync_display(responder,grid);
            pthread_rwlock_unlock(&rwlock_grid);
        }
           
    }
    
}

void* bot_thread(void* arg){
    void *pusher = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(pusher, "inproc://display_sink");

    void* responder = responder_bot;
    
    while(1){
        int ch=0, pos_x=0, pos_y=0;
        int ch_pos, new_bots, n_bytes;
        char* msg = NULL;
        // Receive message identifier: Client, Bot or Display
        char id[10]="\0";
        zmq_recv (responder, &id,sizeof(id),0);
        
        if(strcmp(id,"roaches")==0){
            msg_type m_type;
            zmq_recv (responder, &m_type, sizeof(m_type), 0);
            switch (m_type)
            {
            case CONNECT:{
                // Bot connect message
                BotConnect* m_connect = bot_connect_proto(responder);
                new_bots = m_connect->n_score;
                pthread_rwlock_wrlock(&rwlock_grid);
                if(new_bots+n_roaches+n_wasps<=MAX_ROACHES){
                    ch = new_bots;

                    // Send to client number of bots
                    zmq_send(responder,&(ch),sizeof(int),ZMQ_SNDMORE);
                    ConnectRepply m_repply = CONNECT_REPPLY__INIT;
                    m_repply.n_id = new_bots;
                    m_repply.n_token = new_bots;
                    m_repply.id = malloc (sizeof(int32_t) * new_bots);
                    m_repply.token = malloc (sizeof(int32_t) * new_bots);
                    // Add #new_bots new bots to the data structure and grid
                    roaches_data=bot_connect(n_roaches,roaches_data,new_bots,m_connect,grid,pusher);
                    for (size_t i = 0; i < new_bots; i++){
                        m_repply.id[i] = roaches_data[i+n_roaches].ch;
                        m_repply.token[i] = roaches_data[i+n_roaches].token;
                    }
                    
                    n_roaches = n_roaches+new_bots;

                    // Send to client a message with IDs and tokens
                    n_bytes = connect_repply__get_packed_size(&m_repply);
                    do{
                        msg = calloc(1, n_bytes);
                    }while(msg==NULL);
                    connect_repply__pack(&m_repply,msg);
                    zmq_send(responder, msg, n_bytes, 0);
                    free(msg);
                }else{
                    // Case number of bots requested exceeds the maximum
                    ch=-1;
                    zmq_send(responder,&ch,sizeof(int),0);
                }
                pthread_rwlock_unlock(&rwlock_grid);
                break;}
            case MOVE:{
                // Bot movement message
                BotMovement *m_movement = bot_movement_proto(responder);
                pthread_rwlock_wrlock(&rwlock_grid);
                for (size_t i = 0; i < m_movement->n_id; i++){
                    // Find the bot to move in the data
                    ch_pos = find_ch_info(roaches_data,n_roaches,m_movement->id[i],m_movement->token[i]);
                    if(ch_pos!=-1 && roaches_data[ch_pos].visible==0 && m_movement->movement[i]!=4){
                        
                        pos_x = roaches_data[ch_pos].pos_x;
                        pos_y = roaches_data[ch_pos].pos_y;
                        int pos_x0=pos_x, pos_y0 = pos_y;

                        // Calculate new position
                        new_position(&pos_x, &pos_y, m_movement->movement[i]);
                        // Check grid availability for movement
                        int* check = checkGrid_roach(grid,&pos_x,&pos_y,&roaches_data[ch_pos]);

                        // Publish information in SUB mode
                        send_display_bot(roaches_data[ch_pos],pos_x0,pos_y0,pusher);

                        // Check for roaches that were possibly hidden underneath the moved one
                        if(check[0]!=0 && check[1]!=0){
                            for (size_t i = 0; i < n_roaches; i++){
                                if(roaches_data[i].pos_x==check[0] && roaches_data[i].pos_y==check[1]){
                                    grid[check[0]][check[1]]=&roaches_data[i];

                                    // Publish message if an hidden bot was found
                                    send_display_bot(roaches_data[i],pos_x0,pos_y0,pusher);
                                    break;
                                }
                            } 
                        }
                    }
                }
                pthread_rwlock_unlock(&rwlock_grid);
                // Send to client a check up message
                int check = 1;
                zmq_send(responder,&check,sizeof(int),0);
                break;}
            case DISCONNECT:{
                BotDisconnect* m_disc = bot_disc_proto(responder);
                pthread_rwlock_wrlock(&rwlock_grid);
                for (size_t i = 0; i < m_disc->n_id; i++){
                    // Find the bot to move in the data
                    ch_pos = find_ch_info(roaches_data,n_roaches,m_disc->id[i],m_disc->token[i]);
                    if(ch_pos!=-1){
                        int pos_x0 = roaches_data[ch_pos].pos_x;
                        int pos_y0 = roaches_data[ch_pos].pos_y;
                        send_display_bot(roaches_data[ch_pos],WINDOW_SIZE,WINDOW_SIZE,pusher);
                        roaches_data = removeRoach(roaches_data,&n_roaches,ch_pos,grid);

                        for (size_t i = 0; i < n_roaches; i++){
                            if(roaches_data[i].pos_x==pos_y0 && roaches_data[i].pos_y==pos_x0){
                                grid[pos_x0][pos_x0]=&roaches_data[i];
                                // Publish message if an hidden bot was found
                                send_display_bot(roaches_data[i],pos_x0,pos_y0,pusher);
                                break;
                            }
                        } 
                    }
                }
                pthread_rwlock_unlock(&rwlock_grid);
                int check = 1;
                zmq_send(responder,&check,sizeof(int),0);
                break;}
            default:
                break;
            }
            send_display_update(pusher,0);
            pthread_cond_signal(&cond_pushes_recv);
        }
        if(strcmp(id,"wasps")==0){
            msg_type m_type;
            zmq_recv (responder, &m_type, sizeof(m_type), 0);
            switch (m_type)
            {
            case CONNECT:{
                // Bot connect message
                int new_wasps;
                zmq_recv (responder, &new_wasps, sizeof(n_wasps), 0);
                pthread_rwlock_wrlock(&rwlock_grid);
                if(new_wasps+n_roaches+n_wasps<=MAX_ROACHES){
                    ch = new_wasps;

                    // Send to client number of bots
                    zmq_send(responder,&(ch),sizeof(int),ZMQ_SNDMORE);
                    ConnectRepply m_repply = CONNECT_REPPLY__INIT;
                    m_repply.n_id = new_wasps;
                    m_repply.n_token = new_wasps;
                    m_repply.id = malloc (sizeof(int32_t) * new_wasps);
                    m_repply.token = malloc (sizeof(int32_t) * new_wasps);
                    // Add #new_bots new bots to the data structure and grid
                    wasp_data=wasp_connect(n_wasps,wasp_data,new_wasps,grid,pusher);
                    for (size_t i = 0; i < new_wasps; i++){
                        m_repply.id[i] = wasp_data[i+n_wasps].ch;
                        m_repply.token[i] = wasp_data[i+n_wasps].token;
                    }
                    
                    n_wasps = n_wasps+new_wasps;

                    // Send to client a message with IDs and tokens
                    n_bytes = connect_repply__get_packed_size(&m_repply);
                    do{
                        msg = calloc(1, n_bytes);
                    }while(msg==NULL);
                    connect_repply__pack(&m_repply,msg);
                    zmq_send(responder, msg, n_bytes, 0);
                    free(msg);
                }else{
                    // Case number of bots requested exceeds the maximum
                    ch=-1;
                    zmq_send(responder,&ch,sizeof(int),0);
                }
                pthread_rwlock_unlock(&rwlock_grid);
                break;}
            case MOVE:{
                // Bot movement message
                BotMovement *m_movement = bot_movement_proto(responder);
                pthread_rwlock_wrlock(&rwlock_grid);
                for (size_t i = 0; i < m_movement->n_id; i++){
                    // Find the bot to move in the data
                    ch_pos = find_ch_info(wasp_data,n_wasps,m_movement->id[i],m_movement->token[i]);
                    if(ch_pos!=-1 && wasp_data[ch_pos].visible==0 && m_movement->movement[i]!=4){
                        
                        pos_x = wasp_data[ch_pos].pos_x;
                        pos_y = wasp_data[ch_pos].pos_y;
                        int pos_x0=pos_x, pos_y0 = pos_y;

                        // Calculate new position
                        new_position(&pos_x, &pos_y, m_movement->movement[i]);
                        // Check grid availability for movement
                        checkGrid_wasp(grid,&pos_x,&pos_y,wasp_data[ch_pos], pusher);

                        // Publish information in SUB mode
                        send_display_bot(wasp_data[ch_pos],pos_x0,pos_y0,pusher);
                       
                    }
                }
                pthread_rwlock_unlock(&rwlock_grid);
                // Send to client a check up message
                int check = 1;
                zmq_send(responder,&check,sizeof(int),0);
                break;}
            case DISCONNECT:{
                BotDisconnect* m_disc = bot_disc_proto(responder);
                pthread_rwlock_wrlock(&rwlock_grid);
                for (size_t i = 0; i < m_disc->n_id; i++){
                    // Find the bot to move in the data
                    ch_pos = find_ch_info(wasp_data,n_wasps,m_disc->id[i],m_disc->token[i]);
                    if(ch_pos!=-1){
                        int pos_x0 = wasp_data[ch_pos].pos_x;
                        int pos_y0 = wasp_data[ch_pos].pos_y;
                        send_display_bot(wasp_data[ch_pos],WINDOW_SIZE,WINDOW_SIZE,pusher);
                        wasp_data = removeRoach(wasp_data,&n_wasps,ch_pos,grid);

                        for (size_t i = 0; i < n_roaches; i++){
                            if(wasp_data[i].pos_x==pos_y0 && wasp_data[i].pos_y==pos_x0){
                                grid[pos_x0][pos_x0]=&wasp_data[i];
                                // Publish message if an hidden bot was found
                                send_display_bot(wasp_data[i],pos_x0,pos_y0,pusher);
                                break;
                            }
                        } 
                    }
                }
                pthread_rwlock_unlock(&rwlock_grid);
                int check = 1;
                zmq_send(responder,&check,sizeof(int),0);
                break;}
            default:
                break;
            }
            send_display_update(pusher,0);
            pthread_cond_signal(&cond_pushes_recv);
        }
    }

    
}






int main()
{
    void *responder_lizard, *backend;
    //int* visible=(int*)malloc(0*sizeof(int)); // Variable to store bot visibility, 0=visible
    context = zmq_ctx_new();
    
    int token = server_initialize(context,&responder_lizard,&responder_bot,&publisher,&backend,user_char);
    
	server_interface(&title_win,&game_win,&score_win);
    lizard_data = malloc(0);
    roaches_data = malloc(0);
    pthread_t thread_id[7];
    pthread_create(&thread_id[0],NULL,time_thread,NULL);   
    pthread_create(&thread_id[1],NULL,display_thread,(void *)&token);
    for (size_t i = 0; i < 1; i++){
        pthread_create(&thread_id[i+2],NULL,lizard_thread,(void *)&token); 
    }
    pthread_create(&thread_id[6],NULL,bot_thread,NULL);

    zmq_proxy (responder_lizard, backend, NULL);

  	endwin();			
    zmq_close (publisher);
    zmq_close (responder_lizard);
    free(lizard_data);
    free(roaches_data);
	return 0;
}