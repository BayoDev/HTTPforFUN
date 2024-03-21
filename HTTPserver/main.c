#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

// Local imports
#include "http_lib.h"
#include "http_config.h"

void fail_errno(char* data){
    perror(data);
    exit(EXIT_FAILURE);
}

int setup_tcp_connection(){
    struct addrinfo hints;
    struct addrinfo *result;
    int ret_code,tcp_socket;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // Get address information
    ret_code = getaddrinfo(NULL,PORT,&hints,&result);
    if(ret_code!=0) fail_errno("An error occurred in getaddrinfo");

    // Create TCP socket
    tcp_socket = socket(result->ai_family,result->ai_socktype,result->ai_protocol);
    if(tcp_socket==-1) fail_errno("Unable to open the socket");

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
    // Start accepting connections
    // TODO: check if NULL is right as third parameter
    for(;;){
        // Accept socket connection
        int conn_socket = accept(tcp_socket,NULL,NULL);
        if(conn_socket==-1) break;
        debug("\n\n[DEBUG] New connection established\n");

        // TODO this shouldnt work since the conn_socket could be changed
        // pthread_t thread_instance;
        // if(pthread_create(&thread_instance,NULL,handle_request,&conn_socket)!=0) return;
        
        handle_request(&conn_socket);

        close(conn_socket);
    }
    fail_errno("Something went wrong in accept");
}

int main(){

    // Open socket
    int tcp_socket = setup_tcp_connection();
    printf("Page root folder at: %s\n",ROOT_FOLDER);
    printf("Socket established, now accepting connections at 127.0.0.1:%s ...\n",PORT);

    serve_loop(tcp_socket);

    return 0;
}