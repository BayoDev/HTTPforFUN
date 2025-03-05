#include "cgi_interface.h"

void child_process(
    char* filename,
    int file_fd,
    int DATA_SOCKET,
    int* pipe_fd
){
    close(pipe_fd[0]);
    dup2(pipe_fd[1],STDOUT_FILENO);
    close(pipe_fd[1]);
    execlp("php-cgi","php-cgi",filename,NULL);
    perror("Something went wrong while executing php-cgi");
    close(file_fd);
    exit(-1);
}


bool spawn_cgi_process(
    char* filename,
    int file_fd,
    int DATA_SOCKET
){
    int pipe_fd[2]; 

    // Pipe creation
    if(pipe(pipe_fd)==-1){
        perror("Something went wrong while creating pipe");
        close(file_fd);
        return false;
    }

    // Create new process
    pid_t pid = fork();
    if(pid==-1){
        perror("Something went wrong while forking");
        close(file_fd);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return false;
    }

    // Child process
    if(pid==0){
        child_process(filename,file_fd,DATA_SOCKET,pipe_fd);
    }

    // Parent process
    close(pipe_fd[1]);
    char buffer[1024];
    ssize_t read_val;
    while((read_val=read(pipe_fd[0],buffer,1024))>0){
        write(DATA_SOCKET,buffer,read_val);
    }

    close(pipe_fd[0]);
    return true;
}

void prepare_cgi_env(
    struct request_data* rq_data,
    struct server_config_data* server_info
){
    unsetenv("AUTH_TYPE");
    unsetenv("CONTENT_LENGTH");
    unsetenv("CONTENT_TYPE");
    setenv("GATEWAY_INTERFACE","CGI/1.1",1);
    setenv("PATH_INFO","",1);
    setenv("PATH_TRANSLATED",rq_data->PATH_DATA->real_full_path,1);
    setenv("QUERY_STRING",rq_data->PATH_DATA->parameters,1);

    // TODO really implement
    setenv("REMOTE_ADDR","127.0.0.1",1);
    setenv("REMOTE_HOST","localhost",1);

    // Not supported
    unsetenv("REMOTE_IDENT");
    unsetenv("REMOTE_USER");

    setenv("REQUEST_METHOD",rq_data->METHOD,1);

    setenv("SCRIPT_NAME",rq_data->PATH_DATA->filename,1);

    // TODO really implement
    setenv("SERVER_NAME","localhost",1);
    setenv("SERVER_PORT",server_info->PORT,1);

    setenv("SERVER_PROTOCOL",server_info->HTTP_VERSION,1);
    setenv("SERVER_SOFTWARE",server_info->SERVER_ID,1);

    // TODO: do better, this is php-cgi specific
    setenv("REDIRECT_STATUS","200",1);

}

