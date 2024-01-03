#include "bot-helper.h"

void *requester; // Request ZMQ socket
ConnectRepply* wasps;// Struct storing id and token info

// Signal handler function
void handle_ctrl_c(int signum) {
    if (signum == SIGINT) {
        pthread_mutex_lock(&requester_mutex);
        ctrl_c_pressed = 1;
        //printf("Ctrl+C pressed. Sending a message...\n");
        msg_type m_type = 2;
        BotDisconnect m_disc = BOT_DISCONNECT__INIT;
        m_disc.n_id = wasps->n_id;
        m_disc.n_token = wasps->n_token;
        m_disc.token = malloc(sizeof(int32_t)*m_disc.n_token);
        m_disc.id = malloc(sizeof(int32_t)*m_disc.n_id);
        memcpy(m_disc.id,wasps->id,sizeof(int32_t)*m_disc.n_id);
        memcpy(m_disc.token,wasps->token,sizeof(int32_t)*m_disc.n_token);
        int n_bytes = bot_disconnect__get_packed_size(&m_disc);
        char *msg = malloc(n_bytes);
        bot_disconnect__pack(&m_disc,msg);

        assert(s_sendmore(requester,"wasps")!=-1);
        assert(zmq_send(requester,&m_type,sizeof(m_type),ZMQ_SNDMORE)!=-1);
        assert(zmq_send(requester,msg,n_bytes,0)!=-1);

        int tmp;
        assert(zmq_recv(requester,&tmp,sizeof(int),0)!=-1);
        
        // Unlock the mutex after sending the message
        pthread_mutex_unlock(&requester_mutex);
    }
}


int main(int argc, char*argv[]){
    
    // Server connection of #number_bots bots
    int number_bots = wasps_initialize(&requester, &wasps,argc,argv);

    // Handle ctrlc
    signal(SIGINT, handle_ctrl_c);
    // Send control commands for the bots
    bot_input(requester,wasps,number_bots);

    return 0;
}