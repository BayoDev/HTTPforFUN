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

#include <pthread.h>

// Local imports
#include "c_log/c_log.h"
#include "http/http_config.h"
#include "http/http_lib.h"
#include "http/http_data_structures.h"

void fail_errno(char* data){
    perror(data);
    exit(EXIT_FAILURE);
}

int init_log(){
    int fd = open("log.txt",O_WRONLY|O_CREAT|O_TRUNC,0777);
    if(fd==-1) return -1;

    init_logging(fd,true,LOG_DEBUG);
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
        int conn_socket = accept(tcp_socket,NULL,NULL);
        if(conn_socket==-1) break;
        log_debug("New connection established");

        // TODO this shouldnt work since the conn_socket could be changed
        // pthread_t thread_instance;
        // if(pthread_create(&thread_instance,NULL,handle_request,&conn_socket)!=0) return;
        
        handle_request(&conn_socket, server_info);

        close(conn_socket);
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

    serve_loop(tcp_socket);

    return 0;
}