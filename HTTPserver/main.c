#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>

#define _GNU_SOURCE
#include <pthread.h>

// Local imports
#include "c_log/c_log.h"
#include "http/http_config.h"
#include "http/http_lib.h"
#include "http/http_data_structures.h"
#include "multithreading/multithreading.h"

void fail_errno(char* data){
    perror(data);
    exit(EXIT_FAILURE);
}

int init_log(){
    int fd = open("log.txt",O_WRONLY|O_CREAT|O_TRUNC,0777);
    if(fd==-1) return -1;

    // init_logging(fd,true,LOG_INFO,false);
    return 0;
}

int setup_tcp_connection(){
    struct addrinfo hints;
    struct addrinfo *result;
    int ret_code, tcp_socket, optval = 1;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // Get address information
    ret_code = getaddrinfo(NULL,PORT_PARAM,&hints,&result);
    if(ret_code!=0) fail_errno("An error occurred in getaddrinfo");

    // Create TCP socket
    tcp_socket = socket(result->ai_family,result->ai_socktype,result->ai_protocol);
    if(tcp_socket==-1) fail_errno("Unable to open the socket");

    // Ignore TIME_WAIT state
    if(setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        fail_errno("Unable to set socket options");
    }

    // Bind socket to address
    ret_code = bind(tcp_socket,result->ai_addr,result->ai_addrlen);
    if(ret_code==-1) fail_errno("An error occurred in bind");

    // Free getaddrinfo result
    freeaddrinfo(result);

    // Enable listening on socket
    ret_code = listen(tcp_socket,MAX_QUEUE);
    if(ret_code==-1) fail_errno("An error occurred in listen");

    return tcp_socket;
}


void usage_monitor(){
    printf("\n");
    while(true){
        int avail = num_available_threads();
        unsigned int perc = (THREAD_POOL_SIZE-avail)/(((float)THREAD_POOL_SIZE)/100);
        printf("\rTHREAD USAGE [");
        int step = 1;
        for(int i=0;i<perc/step;i++){
            printf("=");
        }
        for(int i=0;i<(100-perc)/step;i++){
            printf(" ");
        }
        printf("]  %3d%%  %5d/%5d",perc,THREAD_POOL_SIZE-avail,THREAD_POOL_SIZE); 
        fflush(stdout);
        sleep(1);
    }
}

void init_monitor(){
    pthread_t thr;
    if(pthread_create(&thr,NULL,(void*)(void *) usage_monitor,NULL)!=0){
        log_error("Error occured while initializing monitor");
    }
}


void serve_loop(int tcp_socket){

    // Create server info struct
    struct server_config_data* server_info = init_server_config(
        ROOT_FOLDER_PARAM,
        PORT_PARAM,
        HTTP_VERSION_PARAM,
        SERVER_ID_PARAM
    );

    // Start accepting connections
    // TODO: check if NULL is right as third parameter
    for(;;){

        // Accept socket connection
        int volatile conn_socket = accept(tcp_socket,NULL,NULL);
        if(conn_socket==-1) continue;
        log_debug("New connection established");

        
        pthread_t* thread_instance;
        int thr_resp;
        do{
            join_done_threads();
            thr_resp = request_thread(&thread_instance);
        }while(thr_resp==-1);

        struct arg_struct* args = malloc(sizeof(struct arg_struct));
        args->socket_fd = conn_socket;
        args->server_info = server_info;
        
        if(pthread_create(thread_instance,NULL,(void*)(void *) handle_request,(void*) args)!=0){
            free(args);
            close(conn_socket);
        }

        // Uncomment to test without concurrency
        // pthread_joing(thread_instance,NULL);

        // Very unsafe, dont have control over numbers of threads
        // pthread_detach(thread_instance);

    }
    fail_errno("Something went wrong in accept");
}

int main(){

    // Init logging
    init_log();

    // Open socket
    int tcp_socket = setup_tcp_connection();
    log_info("Page root folder at: %s",ROOT_FOLDER_PARAM);
    log_info("Socket established, now accepting connections at 127.0.0.1:%s ...",PORT_PARAM);

    init_monitor();

    // Set socket timeout to 30 seconds
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set receive timeout");
    }
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set send timeout");
    }

    serve_loop(tcp_socket);

    return 0;
}